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
#include "CompareImage.h"
//Defined fs to avoid using the long string of characters every goddam time
namespace fs = std::filesystem;
//Method receives directory, searches all images, loads pointers into array and starts comparing

/* P will return an array of pointers, pointing to repeated images so that the main function will be able to show them
to the user and the user will have the ability to choose what to do with them, delete, save, move... As well
as compare repeated images to choose which one to save (by weight of file, resolution or manually choose)*/
std::vector<void*> search(const std::string& path, const int options)
{
	std::vector<void*> p; //Vector in which to store all pointers
	std::vector<fs::path> image_path; //Vector of paths in which to store all paths to the images
	std::vector<fs::path> repeated; //Vector in which to store all paths to REPEATED images
	std::vector<fs::path> deleted;	//Vector in which to store paths before deleting, solves exception
	auto k = 0;
	cv::redirectError(handleError);
	//Search for all the files and directories inside the firstly specified path
	std::cout << "Looking for images in folders and loading them...\n";
	for (const auto& el : fs::recursive_directory_iterator(path, fs::directory_options::skip_permission_denied))
	{
		if (is_directory(el) != true)
		{
			if ((el.path().extension().compare(".jpg")) == 0 || (el.path().extension().compare(".jpeg")) == 0) //|| ".png" not yet, need to check how to reformat images to avoid warnings of library
			{
				image_path.push_back(el.path());
				k++;
			}
		}
	}
	std::cout << "Found " << k << " images \n";
	std::cout << "Checking images THIS MAY TAKE A WHILE!";
	//Since loading all the images in memory could consume too much memory I opted for a slower approach, load the first image in memory and access
	//all other images via a thread, while the images are being compared a second thread will load the next image into memory
	//Thread will be coming later oops
	bool found = false;
	for (auto i = 0; i < image_path.size(); i++)
	{
		//Implement threads here, i = 0: thread 1, i = 1: thread 2...
		//or maybe start threads before and start i = <whatever> and that starts threads... gotta check
		//Should also put something to show that the program is still doing things
		cv::Mat compared_image = cv::imread(image_path[i].u8string());
		for (auto j = i + 1; ((j < image_path.size()) && (found == false)); j++)
		{
			cv::Mat comparing_image = cv::imread(image_path[j].u8string());
			if (comparator(compared_image, comparing_image) == true)
			{
				//add paths to variable
				repeated.push_back(image_path[i]);
				repeated.push_back(image_path[j]);
				found = true;
			}
		}
		std::cout << "i equals: " << i << "\n";
		found = false;
		if((i != 0) && (i % 100 == 0))
		{
			std::cout << "Already checked: " << i << " number of images \n";
		}
	}
	std::cout << "Number of repeated images found " << repeated.size() / 2 << " is it equal to size?: " << "\n";
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
		for(int i = 0; i < deleted.size();i++)
		{
			fs::remove(deleted[i]);
		}
		std::cout << "Finished removal, have a nice day \n";
		break;;
	case 2:
		//Check original resolutions and delete lower
		break;;
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
				break;;
			case 'l':
				fs::remove(repeated[i]);
				break;;
			case 'R':
				fs::remove(repeated[j]);
				break;;
			case 'r':
				fs::remove(repeated[j]);
				break;;
			default:
				std::cout << "Wrong char, no image was deleted";
				break;;
			}
			std::cout << "\n";
		}
		break;;
	default:
		//Make a for to show all names
		for (int i = 0; i < repeated.size(); i = i + 2)
		{
			int j = i + 1;
			std::cout << "First found: " << repeated[i] << "\nSecond found: " << repeated[j] << "\n\n";
		}
		break;;
	}
	std::cout << "Program ending, have a nice day :3";
	std::this_thread::sleep_for(std::chrono::seconds(5));
	return p;
};

int handleError(int status, const char* func_name,
	const char* err_msg, const char* file_name,
	int line, void* userdata)
{
	//Do nothing -- will suppress console output
	return 0;   //Return value is not used
}

//Method to compare 2 images, to simplify right now it will avoid looking for different resolutions
bool comparator(cv::Mat& compared_img, cv::Mat& comparing_img)
{
	auto equal = true;
	if ((compared_img.cols == comparing_img.cols) && (compared_img.rows == comparing_img.rows))
	{
		for (auto i = 0; ((i < compared_img.rows) && (equal == true)); i++)
		{
			for (auto j = 0; ((j < compared_img.cols) && (equal == true)); j++)
			{
				if(((i + j) % 2) == 0) //skip a pixel every pixel read. It shouldn't be a problem except on very slow resolution images
				{
					if (compared_img.at<cv::Vec3b>(i, j) != comparing_img.at<cv::Vec3b>(i, j))
					{
						equal = false;
					}
				}
			}
		}
	}
	else
	{
		equal = false;
	}
	return equal;
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
	std::getline(std::cin,path);
	std::cout <<
		"\nChoose between the different options: \n1 - Automatically delete images based on their size (saves heaviest)\n";
	std::cout <<
		"2 - Automatically delete images based on resolution (saves biggest resolution) -not yet implemented-\n";
	std::cout << "3 - Manually delete images not recommended, takes a lot of time\n";
	std::cout << "4 - Don't delete just show names\n";
	std::cout << "Enter number: ";
	std::cin >> options;
	search(path, options);
}
