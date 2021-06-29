void databaseDelete(std::string path);
std::vector<void*> search(const std::string& path);
void hash_function(int start_point);
int handleError(int status, const char* func_name,
	const char* err_msg, const char* file_name,
	int line, void* userdata);