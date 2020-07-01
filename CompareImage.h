#pragma once
std::vector<void*> search(const std::string& path);
bool comparator(cv::Mat& compared_img, cv::Mat& comparing_img);
int handleError(int status, const char* func_name,
	const char* err_msg, const char* file_name,
	int line, void* userdata);
std::vector<std::filesystem::path> parallel_threads(int i, std::vector<std::filesystem::path> image_path);