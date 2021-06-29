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
#include <thread>
#include <mutex>
#include <algorithm>
#include <sqlite3.h>
#include "CompareImage.h"
#define CL_HPP_TARGET_OPENCL_VERSION 200 //Target is 2.0, AMD and Intel should be able to run it, opencv will use any available version (Nvidia is a pussy and will only run 1.2)
//Defined fs to avoid using the long string of characters every goddam time
//But I'm dumb enough to not place a using std:: because I'm as dumb as a fucking fish
using namespace boost::filesystem;
cv::Size SIZE(8, 8); //Define size to be used as parameter in resize function, moved to global variable
int max_threads = std::thread::hardware_concurrency(); //Set the max_threads = number of cpu cores, not really important when using OpenCL though
std::mutex mtx;
std::string search_path = "This_shit_better_not_exist_you_fucking_cunt";
std::unordered_map<long int, int> hashmap; //hashmap will store the hash and the position of a path in image_path
std::vector<path> image_path; //Vector of paths in which to store all paths to the images
std::vector<path> loaded_paths; //Vector of paths in which to store the loaded paths from the database
std::vector<path> repeated; //Vector in which to store all paths to REPEATED images
int progress; //Used for progress bar shit
sqlite3* db; //The database var
const char* sql;
char* SqlErrMsg;

void databaseDelete(std::string path) {
	int rc;
	sql = ("DELETE FROM Image_Hashes WHERE PATH=\"" + path + "\"").c_str();
	rc = sqlite3_exec(db, sql, NULL, 0, &SqlErrMsg);
	if (rc != SQLITE_OK) {
		//TODO: Log to file
		std::cout << "Error deleting\n";
		std::cout << SqlErrMsg;
	}
}

//Method to delete the repeated images, if option is true it will delete based on rows and cols, if it's false it will delete based on file size
void removeImages(bool option)
{
	//Check size and delete lower
	int size1;
	int size2;
	std::cout << "Starting deletion of images\n";
	for (int i = 0; i < repeated.size(); i = i + 2)
	{
		if (option)
		{
			size1 = file_size(repeated[i]);
			size2 = file_size(repeated[i + 1]);
		}
		else
		{
			cv::UMat image1 = cv::imread(repeated[i].string(), cv::IMREAD_UNCHANGED).getUMat(cv::ACCESS_READ);
			cv::UMat image2 = cv::imread(repeated[i].string(), cv::IMREAD_UNCHANGED).getUMat(cv::ACCESS_READ);
			size1 = image1.cols + image1.rows;
			size2 = image2.cols + image2.rows;
		}
		if (size1 < size2)
		{
			databaseDelete(repeated[i].string());
			remove(repeated[i]);
		}
		else
		{
			databaseDelete(repeated[i + 1].string());
			remove(repeated[i + 1]);
		}
	}
	std::cout << "Finished removal \n";
}

void manualDelete()
{
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
			databaseDelete(repeated[i].string());
			remove(repeated[i]);
			break;
		case 'r':
			databaseDelete(repeated[i + 1].string());
			remove(repeated[i + 1]);
			break;
		default:
			std::cout << "Wrong char, no image was deleted";
			break;
		}
		std::cout << "\n";
	}
}

// Function that goes through all database data and loads relevant data to teh hashmap
static int databaseLoading(void* NotUsed, int argc, char** argv, char** azColName) {
	int hasher;
	std::unordered_map<long int, int>::iterator it; //Iterator for hashmap
	for (int i = 0; i < argc; i = i + 2) {
		//First check if path of the images is contained on the search_path
		if (strcmp(azColName[i], "PATH") == 0) {
			if (std::string(argv[i]).find(search_path) != std::string::npos) {
				//If the image is in our path check if it still exists (by searching the path inside the image_path)
				hasher = atoi(argv[i + 1]);
				//Add hash value to each path (path is a number of the position of a path in the vector image_path)
				auto result = std::find(image_path.begin(), image_path.end(), argv[i]);
				if (result != image_path.end()) {
					int index = result - image_path.begin();
					it = hashmap.find(hasher);
					if (it != hashmap.end())
					{
						loaded_paths.push_back(image_path[index]);
						repeated.push_back(image_path[it->second]);
						repeated.push_back(image_path[index]);
					}
					else
					{
						hashmap[hasher] = index;
						loaded_paths.push_back(image_path[index]); //Put the loaded paths inside loaded_paths to avoid hashing extra images
					}
				}
			}
		}
	}
	return 0;
}

bool isImage(std::string extension) {
	if ((extension.compare(".jpg")) == 0 || (extension.compare(".jpeg")) == 0 || (extension.compare(".png")) == 0 || (extension.compare(".webp")) == 0 || (extension.compare(".tiff")) == 0 || (extension.compare(".tif")) == 0)
	{
		return true;
	}
	else {
		return false;
	}
}

//Method receives path, searches all images, loads vector and does anything with the images (delete or show)
void search(const int options)
{
	std::string aux; //Little CPU and memory improvement on image checks
	int rc;
	//define nullstream to try and get stderr to not print
	const char* NULLSTREAM = "nul:"; //used to be "/dev/null", moved NULLSTREAM from line 36, bad const practices
	std::vector<std::thread> threads(max_threads); //Vector to store threads creation data
	auto start = std::chrono::high_resolution_clock::now(); //Just to know how long the program execution took, starting... now!
	//Redirect all opencv errors, there's a shitty one that doesn't affect execution but prompts sometimes
	cv::redirectError(handleError);
	auto suppress_error = freopen(NULLSTREAM, "w", stderr);
	//Search for all the files and directories inside the firstly specified path
	std::cout << "Looking for images in folders and loading them...\n";
	for (const auto& el : recursive_directory_iterator(search_path, directory_options::skip_permission_denied))
	{
		if (is_directory(el) != true)
		{
			aux = el.path().extension().string();
			if (isImage(aux))
			{
				image_path.push_back(el.path());
			}
		}
	}
	std::cout << "Found " << image_path.size() << " images \n";
	std::cout << "Starting image processing and searching\n";
	double division = 0;
	progress = 0;

	//load from database
	sql = "SELECT * FROM Image_Hashes";
	rc = sqlite3_exec(db, sql, databaseLoading, 0, &SqlErrMsg);
	if (rc != SQLITE_OK) {
		char option;
		std::cout << "Couldn't load the database data\n";
		std::cout << "Do you wish to continue? (Y/n)\n";
		std::cin >> option;
		if (option != 'Y') {
			std::cout << "Exiting\n";
			exit(-1);
		}
	}

	//start the hash of new or non-database-saved images
	for (int i = 0; i < max_threads; i++)
	{
		threads[i] = std::thread(hash_function, i);
	}

	//Variables for progress bar
	int step = 1;
	int displayNext = step;
	int percent = 0;
	do {
		//Lower CPU usage for this bad boi, goes at start because it would skip last iteration if put at end
		std::this_thread::sleep_for(std::chrono::milliseconds(500));
		percent = ((100 * (progress + 1)) / image_path.size());
		if (percent > 100) {
			percent = 100;
		}
		if (percent >= displayNext)
		{
			std::cout << "\r" << "[" << std::string(percent / 5, (char)254u) << std::string((int)(100 / 5 - percent / 5), ' ') << "]";
			std::cout << percent << "%" << " [Image " << progress << " of " << image_path.size() << "]";
			std::cout.flush();
			displayNext += step;
		}
	} while (progress < image_path.size());

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
		removeImages(false); //false: based on size
		break;
	case 2:
		removeImages(true); //true: based on rows and cols
		break;
	case 3:
		manualDelete(); //Show the user every pair of images and make him choose
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
	//show elapsed time with diferent magnitudes
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
	sqlite3_close(db); //close the database connection
	system("pause");
};

void hash_function(int start_point)
{
	int rc;
	std::string temp_sql;
	const char* local_sql;
	long int hasher = 1; //Store the hashed value of an image
	std::unordered_map<long int, int>::iterator it; //Iterator for hashmap
	for (int i = start_point; i < image_path.size(); i += max_threads)
	{
		//Check if the image has already been loaded by the database
		progress++;
		if (std::find(loaded_paths.begin(), loaded_paths.end(), image_path[i]) == loaded_paths.end()) {
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
					temp_sql = "INSERT INTO Image_Hashes VALUES(\"" + image_path[i].string() + "\"," + std::to_string(hasher) + ")";
					local_sql = temp_sql.c_str();
					rc = sqlite3_exec(db, local_sql, NULL, 0, &SqlErrMsg); //Check if the tables exist
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
					if (rc != SQLITE_OK) {
						std::cout << SqlErrMsg;
						std::this_thread::sleep_for(std::chrono::milliseconds(500));
						//TODO: Log errors to file
					}
				}
			}
			catch (...) {
				//TODO: Add printing to file
			}
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
	int options = 0;
	char sure = 'z';
	int rc; //Error check for database connection
	rc = sqlite3_open(".\database", &db); // Connect to database
	std::ifstream infile("config.cfg");
	if (infile.fail()) { //Error check for config file
		std::cout << "Config file not found!\n";
		std::cout << "Press any button to exit";
		std::cin >> options;
		exit(-1);
	}
	if (rc) { //Error check for database connection
		std::cout << "The database couldn't be reached\n";
		std::cout << "Press any button to exit";
		system("pause");
		exit(-1);
	}
	sql = "SELECT * FROM Image_Hashes";
	rc = sqlite3_exec(db, sql, NULL, 0, &SqlErrMsg); //Check if the tables exist
	if (rc != SQLITE_OK) {
		//Create the tables for the database
		//Primary key: Pair full path + hash
		//No other values
		sql = "CREATE TABLE Image_Hashes("\
			"PATH VARCHAR(128),"\
			"HASH INTEGER(64), "\
			"CONSTRAINT hash_pk PRIMARY KEY(PATH, HASH)"\
			")";
		rc = sqlite3_exec(db, sql, NULL, 0, &SqlErrMsg);
		if (rc != SQLITE_OK) {
			std::cout << "Couldn't create the database tables\n";
			std::cout << "Press any button to exit";
			system("pause");
			exit(-1);
		}
	}
	std::string file_input;
	std::cout << "Welcome to the Repeated Image Predator (RIP) v3.0\nThis program was created by Joan Gil (Linkedin: https://www.linkedin.com/in/joan-gil-rigo-a65536184/) \nand is used for free under the MIT license, check MIT info at: https://en.wikipedia.org/wiki/MIT_License \n";
	std::cout << "Please think about donating: https://www.paypal.me/jgil99 \nFollow the instructions to begin search: \n\n";
	//Oh no, not checking input again...
	do
	{
		std::cout << "Enter the path where you want to search: ";
		std::getline(std::cin, search_path);
		if (!is_directory(search_path))
		{
			std::cout << "Not a valid directory, make sure it's not a path to a single image";
		}
	} while (!is_directory(search_path));
	std::cout << "\nChoose between the different options: \n1 - Automatically delete images based on their size (saves heaviest)\n";
	std::cout << "2 - Automatically delete images based on resolution (saves biggest resolution)\n";
	std::cout << "3 - Manually delete images not recommended, takes a lot of time\n";
	std::cout << "4 - Don't delete just show names of repeated images\n";
	do
	{
		std::cout << "Enter number: ";
		std::cin >> options;
		if (options < 1 || options > 4) {
			std::cout << "Invalid option, check if you used a number between 1 and 4\n";
		}
	} while (options < 1 || options > 4);
	//add file reading check for opencl usage
	std::getline(infile, file_input);
	int initial_pos = file_input.find('=') + 1;
	int last_pos = file_input.find('#', initial_pos) - 1;
	file_input = file_input.substr(initial_pos, last_pos - initial_pos); //get correct part of string
	if (file_input.compare("yes") == 0) {
		cv::ocl::setUseOpenCL(true);
		max_threads = 2; //Use only 2 threads if OpenCL is enabled, no need for more, no improvements
	}
	else {
		cv::ocl::setUseOpenCL(false);
	}

	//Add file read and check threads > 0
	std::getline(infile, file_input);
	initial_pos = file_input.find('=') + 1;
	last_pos = file_input.find('#', initial_pos) - 1;
	file_input = file_input.substr(initial_pos, last_pos - initial_pos); //get correct part of string
	//If config file is not default convert to number, the program will crash if there's no default or a valid number also check if OpenCL is ON, since no real improvements are made by having opencl and more threads
	if (file_input.compare("default") != 0 && !cv::ocl::useOpenCL()) {
		max_threads = std::stoi(file_input);
	}
	infile.close();
	//Time to work OH BOI
	search(options);
}