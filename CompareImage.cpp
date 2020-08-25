//This class shall receive a directory and start comparing images from that directory including any other folder inside of it
//It will compare images using the OpenCV library
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <png.h>
#include <boost/filesystem.hpp> //WINDOWS IS A NIGHTMARE A NIGHTMARE A NIGHTMARE A NIGHTMARE A NIGHTMARE A NIGHTMARE A NIGHTMARE A NIGHTMARE A NIGHTMARE A NIGHTMARE A NIGHTMARE
#include <iostream>
#include <string>
#include <chrono>
#include <unordered_map>
#include <mutex>
#include "CompareImage.h"
//Defined fs to avoid using the long string of characters every goddam time
//But I'm dumb enough to not place a usinn std:: because I'm as dumb as a fucking fish
using namespace boost::filesystem;
cv::Size SIZE(8, 8); //Define size to be used as parameter in resize function, moved to global variable
int THREADS;
std::mutex mtx;
std::unordered_map<long int, int> hashmap; //hashmap will store the hash and the position of a path in image_path
std::vector<path> image_path; //Vector of paths in which to store all paths to the images
std::vector<path> repeated; //Vector in which to store all paths to REPEATED images
auto count = 0;
int progress = 0; //Used for progress bar shit

//Method receives path, searches all images, loads vector and does anything with the images (delete or show)
void search(const std::string& searc_path, const int options)
{
	//define nullstream to try and get stderr to not print
	const char* nullStream = "/dev/null";
	std::vector<path> deleted; //Vector in which to store paths before deleting, solves exception
	std::vector<std::thread> threads(THREADS);
	auto start = std::chrono::high_resolution_clock::now();
	//Variables for progress bar
	int step = 1;
	int displayNext = step;
	int percent = 0;
	//Redirect all opencv errors, there's a shitty one that doesn't affect execution but prompts sometimes
	cv::redirectError(handleError);
	//This should stop libpng warnings...
	nullStream = "nul:";
	freopen(nullStream,"w",stderr);
	//Search for all the files and directories inside the firstly specified path
	std::cout << "Looking for images in folders and loading them...\n";
	for (const auto& el : recursive_directory_iterator(searc_path, directory_options::skip_permission_denied))
	{
		if (is_directory(el) != true)
		{
			if ((el.path().extension().string().compare(".jpg")) == 0 || (el.path().extension().string().compare(".jpeg")) == 0 || (el.path().extension().string().compare(".png")) == 0)
			{
				image_path.push_back(el.path());
				count++;
			}
		}
	}
	std::cout << "Found " << count << " images \n";
	std::cout << "Starting image processing and searching\n";
	double division = 0;
	count = 0;

	//Start all threads
	for(int i = 0; i < THREADS;i++)
	{
		threads[i] =  std::thread(hash_function,i);
	}

	//Gotta re-add the % bar
	for (; progress < image_path.size();)
	{
		percent = (100 * (progress + 1)) / image_path.size();
		if (percent >= displayNext)
		{
			std::cout << "\r" << "[" << std::string(percent / 5, (char)254u) << std::string(100 / 5 - percent / 5, ' ') << "]";
			std::cout << percent << "%" << " [Image " << progress + 1 << " of " << image_path.size() << "]";
			std::cout.flush();
			displayNext += step;
		}
	}

	//Wait for all threads to end
	for(int i = 0; i < THREADS;i++)
	{
		threads[i].join();
	}

	std::cout << "\nNumber of repeated images found " << count/2 << "\n"; //Gotta check count variable and see why I dont know how to count
	switch (options)
	{
	case 1:
		//Check size and delete lower
		int j;
		std::cout << "Starting deletion of images\n";
		for (int i = 0; i < repeated.size(); i = i + 2)
		{
			j = i + 1;
			if ((file_size(repeated[i])) < (file_size(repeated[j])))
			{
				deleted.push_back(repeated[i]);
			}
			else
			{
				deleted.push_back(repeated[j]);
			}
		}
		for (int i = 0; i < deleted.size(); i++)
		{
			remove(deleted[i]);
		}
		std::cout << "Finished removal \n";
		break;
	case 2:
		std::cout << "Starting deletion of images\n";
		for (int i = 0; i < repeated.size(); i = i + 2)
		{
			j = i + 1;
			cv::Mat image1 = cv::imread(repeated[i].string(), cv::IMREAD_UNCHANGED);
			cv::Mat image2 = cv::imread(repeated[i].string(), cv::IMREAD_UNCHANGED);
			if ((image1.cols + image1.rows) < (image2.cols + image2.rows))
			{
				deleted.push_back(repeated[i]);
			}
			else
			{
				deleted.push_back(repeated[j]);
			}
		}
		for (int i = 0; i < deleted.size(); i++)
		{
			remove(deleted[i]);
		}
		std::cout << "Finished removal\n";
		break;
	case 3:
		//Start showing every pair of images and ask Right (R) or Left(L)
		char choice;
		std::cout << "Showing name of images, choose using L (left) or R (right), any other character will skip it, use Ctrl+C to skip all";
		for (int i = 0; i < repeated.size(); i = i + 2)
		{
			int j = i + 1;
			std::cout << "L= " << repeated[i] << "of size: " << file_size(repeated[i]) << "  R= " << repeated[j] <<
				"of size: " << file_size(repeated[j]);
			std::cin >> choice;
			choice = std::tolower(choice);
			switch (choice)
			{
			case 'l':
				remove(repeated[i]);
				break;
			case 'r':
				remove(repeated[j]);
				break;
			default:
				std::cout << "Wrong char, no image was deleted";
				break;
			}
			std::cout << "\n";
		}
		break;
	default:
		//Make a for to show all names
		for (int i = 0; i < repeated.size(); i = i + 2)
		{
			int j = i + 1;
			std::cout << "First found: " << repeated[i] << "\nSecond found: " << repeated[j] << "\n\n";
		}
		break;
	}
	auto end = std::chrono::high_resolution_clock::now();
	std::cout << "Program ending, have a nice day :3\n";
	auto time_elapsed = end - start;
	std::cout << "Elapsed computation time: " << std::chrono::duration_cast<std::chrono::milliseconds>(time_elapsed).count() << "ms\n";
	system("pause");
};

void hash_function(int start_point)
{
	long int hasher = 1; //Store the hashed value of an image
	std::unordered_map<long int, int>::iterator it; //Iterator for hashmap
	for (int i = start_point; i < image_path.size(); i += THREADS)
	{
		cv::Mat processing_image = cv::imread(image_path[i].string(), cv::IMREAD_COLOR); //had to change from image_path[i].u8string() to .string() due to a library update
		hasher = 1; //Must reset hasher to 1
		if (processing_image.dims != 0)
		{
			cv::Mat gray_image; //Define gray image to create gray scale images

			cv::cvtColor(processing_image, gray_image, cv::COLOR_BGR2GRAY);
			cv::resize(gray_image, gray_image, SIZE);
			for (int j = 0; j < (gray_image.cols - 1); j++)
			{
				for (int l = 0; l < gray_image.rows; l++)
				{
					hasher = hasher + gray_image.at<uchar>(l, j); //will leave it like this for now, seems to work
				}
				hasher = hasher * hasher; //just to make hasher even more unique
			}
			//Mutex this bad boi
			mtx.lock();
			//Extremely important things here 
			if (hashmap.empty() == true)
			{
				hashmap[hasher] = i;
				//Add hash value to each path (path is a number of the position of a path in the vector image_path)
			}
			else
			{
				//Solved the counting error by simply skipping the already in check
				it = hashmap.find(hasher);
				if (it != hashmap.end())
				{
					repeated.push_back(image_path[it->second]);
					count++;
					repeated.push_back(image_path[i]);
					count++;
				}
				else
				{
					//Add hash value to each path (path is a number of the position of a path in the vector image_path)
					hashmap[hasher] = i;
				}
			
			}
			mtx.unlock();
			//No more important thingys :D
		}else
		{
			hasher = -1;
		}
		progress++;
	}
}

int handleError(int status, const char* func_name,
                const char* err_msg, const char* file_name,
                int line, void* userdata)
{
	//Do nothing -- will suppress console output
	return 0; //Return value is not used
}


//I can't be bothered to make a UI for now so console will be
int main()
{
	std::string path;
	int options;
	char sure;
	std::cout << "Welcome to the Repeated Image Predator (RIP)\nThis program was created by Joan Gil (Linkedin: https://www.linkedin.com/in/joan-gil-rigo-a65536184/) \nand is used for free under the MIT license, check MIT info at: https://en.wikipedia.org/wiki/MIT_License \n";
	std::cout << "Please think about donating: https://www.paypal.me/jgil99 \nFollow the instructions to begin search: \n\n";
	std::cout << "Enter the path where you want to search: ";
	std::getline(std::cin, path);
	std::cout << "\nChoose between the different options: \n1 - Automatically delete images based on their size (saves heaviest)\n";
	std::cout << "2 - Automatically delete images based on resolution (saves biggest resolution)\n";
	std::cout << "3 - Manually delete images not recommended, takes a lot of time\n";
	std::cout << "4 - Don't delete just show names of repeated images\n";
	std::cout << "Enter number: ";
	std::cin >> options;
	std::cout << "How many threads would you like to create? (The more threads the faster it will be but the more it will consume CPU and RAM)\n";
	std::cout << "Recommended your CPU cores times 2, in case of not knowing use 4, not recommended to use more than 128\n";
	std::cin >> THREADS;
	if (THREADS < 1)
	{
		THREADS = 4;
	}else if (THREADS > 128)
	{
		std::cout << "You chose more than 128 threads, are you sure? (Y/n)\n";
		std::cin >> sure;
		sure = std::tolower(sure);
		if(sure != 'y')
		{
			exit(0);
		}
	}
	search(path, options);
}