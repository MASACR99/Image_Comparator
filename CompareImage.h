std::vector<void*> search(const std::string& path);
int handleError(int status, const char* func_name,
	const char* err_msg, const char* file_name,
	int line, void* userdata);
bool already_in(std::filesystem::path searched, std::vector<std::filesystem::path> search_in);