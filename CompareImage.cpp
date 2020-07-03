//This class shall receive a directory and start comparing images from that directory including any other folder inside of it
//It will compare images using the OpenCV library
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <filesystem>
#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <unordered_map>
#include "CompareImage.h"
//Defined fs to avoid using the long string of characters every goddam time
namespace fs = std::filesystem;

//Method receives path, searches all images, loads vector and does anything with the images (delete or show)
std::vector<void*> search(const std::string& path, const int options)
{
	std::vector<void*> p; //Vector in which to store all pointers
	std::vector<fs::path> image_path; //Vector of paths in which to store all paths to the images
	std::vector<fs::path> repeated; //Vector in which to store all paths to REPEATED images
	std::vector<fs::path> deleted; //Vector in which to store paths before deleting, solves exception
	std::vector<cv::Mat> processed_imgs; //Save processed images before getting hashmap
	cv::Mat gray_image; //Define gray image to create gray scale images
	cv::Size size(8, 8); //Define size to be used as parameter in resize function
	long int hasher; //Hasher will help store the hash of each image before being added to a vector
	std::unordered_map<long int, int> hashmap; //hashmap will store the hash and the position of a path in image_path
	auto count = 0;
	cv::redirectError(handleError);
	//Search for all the files and directories inside the firstly specified path
	std::cout << "Looking for images in folders and loading them...\n";
	for (const auto& el : fs::recursive_directory_iterator(path, fs::directory_options::skip_permission_denied))
	{
		if (is_directory(el) != true)
		{
			if ((el.path().extension().compare(".jpg")) == 0 || (el.path().extension().compare(".jpeg")) == 0 || (el.path().extension().compare(".png")) == 0)
				//|| ".png" not yet, need to check how to reformat images to avoid warnings of library
			{
				image_path.push_back(el.path());
				count++;
			}
		}
	}
	std::cout << "Found " << count << " images \n";
	std::cout << "Starting image processing and searching\n";
	count = 0;
	//Move to function
	for (int i = 0; i < image_path.size(); i++)
	{
		cv::Mat processing_image = cv::imread(image_path[i].u8string(), cv::IMREAD_COLOR); //cv::IMREAD_UNCHANGED used to be
		std::unordered_map<long int, int>::iterator it; //Iterator for hashmap
		if (processing_image.dims != 0)
		{
			if(processing_image.channels())
			{
				
			}
			cv::cvtColor(processing_image, gray_image, cv::COLOR_BGR2GRAY);
			cv::resize(gray_image, gray_image, size);
			hasher = 1;
			for (int j = 0; j < (gray_image.cols - 1); j++)
			{
				for (int l = 0; l < gray_image.rows; l++)
				{
					hasher = hasher + gray_image.at<uchar>(l, j); //will leave it like this for now
				}
			}
			
			if (hashmap.empty() == true)
			{
				hashmap[hasher] = i;
				//Add hash value to each path (path is a number of the position of a path in the vector image_path)
			}
			else
			{
				it = hashmap.find(hasher);
				if (it != hashmap.end())
				{
					//Add to repeated vector
					repeated.push_back(image_path[it->second]);
					repeated.push_back(image_path[i]);
					count = count + 2;
					
				}
				else
				{
					//Add hash value to each path (path is a number of the position of a path in the vector image_path)
					hashmap[hasher] = i;
				}
			}
		}
	}
	std::cout << "Number of repeated images found " << count << "\n";
	switch (options)
	{
	case 1:
		//Check size and delete lower
		int j;
		std::cout << "Starting deletion of images\n";
		for (int i = 0; i < repeated.size(); i = i + 2)
		{
			j = i + 1;
			if ((fs::file_size(repeated[i])) < (fs::file_size(repeated[j])))
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
			fs::remove(deleted[i]);
		}
		std::cout << "Finished removal, have a nice day \n";
		break;
	case 2:
		std::cout << "Starting deletion of images\n";
		for (int i = 0; i < repeated.size(); i = i + 2)
		{
			j = i + 1;
			cv::Mat image1 = cv::imread(repeated[i].u8string(), cv::IMREAD_UNCHANGED);
			cv::Mat image2 = cv::imread(repeated[i].u8string(), cv::IMREAD_UNCHANGED);
			if ((image1.cols + image1.rows) > (image2.cols + image2.rows))
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
			fs::remove(deleted[i]);
		}
		std::cout << "Finished removal, have a nice day \n";
		break;
	case 3:
		//Start showing every pair of images and ask Right (R) or Left(L)
		char choice;
		std::cout << "Showing name of images, choose using L (left) or R (right)";
		for (int i = 0; i < repeated.size(); i = i + 2)
		{
			int j = i + 1;
			std::cout << "L= " << repeated[i] << "of size: " << fs::file_size(repeated[i]) << "  R= " << repeated[j] <<
				"of size: " << fs::file_size(repeated[j]);
			std::cin >> choice;
			switch (choice)
			{
			case 'L':
				fs::remove(repeated[i]);
				break;
			case 'l':
				fs::remove(repeated[i]);
				break;
			case 'R':
				fs::remove(repeated[j]);
				break;
			case 'r':
				fs::remove(repeated[j]);
				break;
			default:
				std::cout << "Wrong char, no image was deleted";
				break;
			}
			std::cout << "\n";
		}
		break;
	default:
		//Make a for to show all name
		for (int i = 0; i < repeated.size(); i = i + 2)
		{
			int j = i + 1;
			std::cout << "First found: " << repeated[i] << "\nSecond found: " << repeated[j] << "\n\n";
		}
		break;
	}
	std::cout << "Program ending, have a nice day :3\n";
	system("pause");
	return p;
};

bool already_in(const fs::path searched, std::vector<fs::path> search_in)
{
	bool ret = false;
	for (int i = 0; ((i < search_in.size()) && (ret == false)); i++)
	{
		if (searched.compare(search_in[i]) == 0)
		{
			ret = true;
		}
	}
	return ret;
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
	std::cout <<
		"Welcome to the Repeated Image Predator (RIP)\nThis program was created by Joan Gil (Linkedin: https://www.linkedin.com/in/joan-gil-rigo-a65536184/) \nand is used for free under the MIT license, check MIT info at: https://en.wikipedia.org/wiki/MIT_License \n";
	std::cout <<
		"Please think about donating: https://www.paypal.me/jgil99 \nFollow the instructions to begin search: \n\n";
	std::cout << "Enter the path where you want to search: ";
	std::getline(std::cin, path);
	std::cout <<
		"\nChoose between the different options: \n1 - Automatically delete images based on their size (saves heaviest)\n";
	std::cout <<
		"2 - Automatically delete images based on resolution (saves biggest resolution)\n";
	std::cout << "3 - Manually delete images not recommended, takes a lot of time\n";
	std::cout << "4 - Don't delete just show names of repeated images\n";
	std::cout << "Enter number: ";
	std::cin >> options;
	search(path, options);
}
