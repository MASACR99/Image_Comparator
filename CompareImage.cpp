//This class shall receive a directory and start comparing images from that directory including any other folder inside of it
//It will compare images using the OpenCV library
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/core/ocl.hpp> //Get openCL from openCV library
#include <png.h>
#include <boost/filesystem.hpp> //WINDOWS IS A NIGHTMARE A NIGHTMARE A NIGHTMARE A NIGHTMARE A NIGHTMARE A NIGHTMARE A NIGHTMARE A NIGHTMARE A NIGHTMARE A NIGHTMARE A NIGHTMARE
#include <iostream>
#include <string>
#include <chrono>
#include <unordered_map>
#include <mutex>
#include "CompareImage.h"
#define CL_HPP_TARGET_OPENCL_VERSION 200 //Target is 2.0, AMD, NVIDIA and Intel should be able to run it, opencv will use any available version
//Defined fs to avoid using the long string of characters every goddam time
//But I'm dumb enough to not place a using std:: because I'm as dumb as a fucking fish
using namespace boost::filesystem;
cv::Size SIZE(8, 8); //Define size to be used as parameter in resize function, moved to global variable
int max_threads = 0; //start at 0 for user input checks :P
std::mutex mtx;
std::unordered_map<long int, int> hashmap; //hashmap will store the hash and the position of a path in image_path
std::vector<path> image_path; //Vector of paths in which to store all paths to the images
std::vector<path> repeated; //Vector in which to store all paths to REPEATED images
int progress; //Used for progress bar shit

//Method receives path, searches all images, loads vector and does anything with the images (delete or show)
void search(const std::string& searc_path, const int options)
{
	std::string aux; //Little CPU and memory improvement on image checks
	//define nullstream to try and get stderr to not print
	const char* NULLSTREAM = "nul:"; //used to be "/dev/null", moved NULLSTREAM from line 36, bad const practices
	std::vector<std::thread> threads(max_threads); //Vector to store threads creation data
	auto start = std::chrono::high_resolution_clock::now(); //Just to know how long the program execution took, starting... now!
	//Redirect all opencv errors, there's a shitty one that doesn't affect execution but prompts sometimes
	cv::redirectError(handleError);
	freopen(NULLSTREAM, "w", stderr);
	//Search for all the files and directories inside the firstly specified path
	std::cout << "Looking for images in folders and loading them...\n";
	for (const auto& el : recursive_directory_iterator(searc_path, directory_options::skip_permission_denied))
	{
		if (is_directory(el) != true)
		{
			aux = el.path().extension().string();
			if ((aux.compare(".jpg")) == 0 || (aux.compare(".jpeg")) == 0 || (aux.compare(".png")) == 0 || (aux.compare(".webp")) == 0  || (aux.compare(".tiff")) == 0 ||(aux.compare(".tif")) == 0)
			{
				image_path.push_back(el.path());
			}
		}
	}
	std::cout << "Found " << image_path.size() << " images \n";
	std::cout << "Starting image processing and searching\n";
	double division = 0;
	progress = 0;
	for (int i = 0; i < max_threads; i++)
	{
		threads[i] = std::thread(hash_function, i);
	}

	//Variables for progress bar
	int step = 1;
	int displayNext = step;
	int percent = 0;

	for (; progress < image_path.size();)
	{
		//Lower CPU usage for this bad boi, goes at start because it would skip last iteration if put at end
		std::this_thread::sleep_for(std::chrono::milliseconds(2500));
		percent = (100 * (progress + 1)) / image_path.size();
		if (percent >= displayNext)
		{
			std::cout << "\r" << "[" << std::string(percent / 5, (char)254u) << std::string(100 / 5 - percent / 5, ' ') << "]";
			std::cout << percent << "%" << " [Image " << progress << " of " << image_path.size() << "]";
			std::cout.flush();
			displayNext += step;
		}
	}

	//Wait for all threads to end
	//this code isn't that important, since all threads are gonna end before getting here due to the
	//progress bar, anyway is good code practice to join, and it releases kinda of "a lot" of memory space
	for (int i = 0; i < max_threads; i++)
	{
		threads[i].join();
	}

	std::cout << "\nNumber of repeated images found " << repeated.size() / 2 << "\n"; //Hey I got it, kinda
	switch (options)
	{
	case 1:
		//Check size and delete lower
		std::cout << "Starting deletion of images\n";
		for (int i = 0; i < repeated.size(); i = i + 2)
		{
			if ((file_size(repeated[i])) < (file_size(repeated[i + 1])))
			{
				remove(repeated[i]);
			}
			else
			{
				remove(repeated[i + 1]);
			}
		}
		std::cout << "Finished removal \n";
		break;
	case 2:
		std::cout << "Starting deletion of images\n";
		for (int i = 0; i < repeated.size(); i = i + 2)
		{
			cv::UMat image1 = cv::imread(repeated[i].string(), cv::IMREAD_UNCHANGED).getUMat(cv::ACCESS_READ);
			cv::UMat image2 = cv::imread(repeated[i].string(), cv::IMREAD_UNCHANGED).getUMat(cv::ACCESS_READ);
			if ((image1.cols + image1.rows) < (image2.cols + image2.rows))
			{
				remove(repeated[i]);
			}
			else
			{
				remove(repeated[i + 1]);
			}
		}
		std::cout << "Finished removal\n";
		break;
	case 3:
		//Start showing every pair of images and ask Right (R) or Left(L)
		char choice;
		std::cout << "Showing name of images, choose using L (left) or R (right), any other character will skip it, use Ctrl+C to skip all";
		for (int i = 0; i < repeated.size(); i = i + 2)
		{
			std::cout << "L= " << repeated[i] << "of size: " << file_size(repeated[i]) << "\n"
				<< "  R= " << repeated[i + 1] << "of size: " << file_size(repeated[i + 1]);
			std::cin >> choice;
			choice = std::tolower(choice);
			switch (choice)
			{
			case 'l':
				remove(repeated[i]);
				break;
			case 'r':
				remove(repeated[i + 1]);
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
			std::cout << "First found: " << repeated[i] << "\nSecond found: " << repeated[i + 1] << "\n\n";
		}
		break;
	}
	auto end = std::chrono::high_resolution_clock::now(); //Za warudo (WEEEEEEEEEEB SHIIIIIIIIIIIT)
	std::cout << "Program ending, have a nice day :3\n";
	auto time_elapsed = end - start;
	int tot_time = std::chrono::duration_cast<std::chrono::seconds>(time_elapsed).count();
	//show elapsed time with diferent 
	if (tot_time < 30)
	{
		std::cout << "Elapsed computation time: " << std::chrono::duration_cast<std::chrono::milliseconds>(time_elapsed).count() << "ms\n";
	}
	else if (tot_time < 300) {
		std::cout << "Elapsed computation time: " << tot_time << "s\n";
	}
	else if (tot_time < 3600) {
		std::cout << "Elapsed computation time: " << std::chrono::duration_cast<std::chrono::minutes>(time_elapsed).count()
			<< "minutes and " << tot_time << "seconds\n";
	}
	else {
		std::cout << "Elapsed computation time: " << std::chrono::duration_cast<std::chrono::hours>(time_elapsed).count()
			<< "hours and " << std::chrono::duration_cast<std::chrono::minutes>(time_elapsed).count() << "minutes\n";
		std::cout << "shit that was a long time, hope it was all okay <3\n";
	}
	system("pause");
};

void hash_function(int start_point)
{
	long int hasher = 1; //Store the hashed value of an image
	std::unordered_map<long int, int>::iterator it; //Iterator for hashmap
	for (int i = start_point; i < image_path.size(); i += max_threads)
	{
		progress++;
		try {
			cv::UMat processing_image = cv::imread(image_path[i].string(), cv::IMREAD_COLOR).getUMat(cv::ACCESS_READ); //had to change from image_path[i].u8string() to .string() due to a library update
			hasher = 1; //Must reset hasher to 1
			if (processing_image.dims != 0)
			{
				cv::UMat gray_image; //Define gray image to create gray scale images
				cv::cvtColor(processing_image, gray_image, cv::COLOR_BGR2GRAY);
				cv::resize(gray_image, gray_image, SIZE);
				for (int j = 0; j < (gray_image.cols - 1); j++)
				{
					for (int l = 0; l < gray_image.rows; l++)
					{
						hasher = hasher + gray_image.getMat(cv::ACCESS_READ).at<uchar>(l, j); //will leave it like this for now, seems to work
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
						repeated.push_back(image_path[i]);
					}
					else
					{
						//Add hash value to each path (path is a number of the position of a path in the vector image_path)
						hashmap[hasher] = i;
					}
				}
				mtx.unlock();
				//No more important thingys :D
			}
			else
			{
				hasher = -1;
			}
		}
		catch (...) {
		}
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
	std::string path = "This_shit_better_not_exist_you_fucking_cunt";
	int options = 0;
	char sure = 'z';
	std::ifstream infile("../config.cfg");
	if (infile.fail()) {
		std::cout << "Config file not found";
		exit(0);
	}
	std::string file_input;
	std::cout << "Welcome to the Repeated Image Predator (RIP) v2.1\nThis program was created by Joan Gil (Linkedin: https://www.linkedin.com/in/joan-gil-rigo-a65536184/) \nand is used for free under the MIT license, check MIT info at: https://en.wikipedia.org/wiki/MIT_License \n";
	std::cout << "Please think about donating: https://www.paypal.me/jgil99 \nFollow the instructions to begin search: \n\n";
	//Oh no, not checking input again...
	while (!is_directory(path))
	{
		std::cout << "Enter the path where you want to search: ";
		std::getline(std::cin, path);
		if (!is_directory(path))
		{
			std::cout << "Not a valid directory, make sure it's not a path to a single image";
		}
	}
	std::cout << "\nChoose between the different options: \n1 - Automatically delete images based on their size (saves heaviest)\n";
	std::cout << "2 - Automatically delete images based on resolution (saves biggest resolution)\n";
	std::cout << "3 - Manually delete images not recommended, takes a lot of time\n";
	std::cout << "4 - Don't delete just show names of repeated images\n";
	while (options < 1 || options > 4)
	{
		std::cout << "Enter number: ";
		std::cin >> options;
		if (options < 1 || options > 4) {
			std::cout << "Invalid option, check if you used a number between 1 and 4\n";
		}
	}
	//add file reading check for opencl usage
	std::getline(infile,file_input);
	int initial_pos = file_input.find('=');
	int last_pos = file_input.find('#',initial_pos);
	file_input = file_input.substr(initial_pos+1, last_pos-2-initial_pos); //get correct part of string
	if (file_input.compare("yes") == 0) {
		cv::ocl::setUseOpenCL(true);
	}
	else {
		cv::ocl::setUseOpenCL(false);
	}

	//Add file read and check threads > 0
	std::getline(infile, file_input);
	initial_pos = file_input.find('=');
	last_pos = file_input.find('#', initial_pos);
	file_input = file_input.substr(initial_pos+1, last_pos-2 - initial_pos); //get correct part of string
	max_threads = std::stoi(file_input);
	infile.close();
	//Time to work OH BOI
	search(path, options);
}