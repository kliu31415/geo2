#include "kx/log.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fstream>
#include <mutex>
#include <iostream>
#include <optional>

namespace kx { namespace io {

std::mutex print_mutex;

void println()
{
    std::lock_guard<std::mutex> lg(print_mutex);
    std::cout << std::endl;
}
void flush()
{
    std::lock_guard<std::mutex> lg(print_mutex);
    std::cout.flush();
}
bool folder_exists(std::string_view path)
{
    struct stat info;

    if(stat(path.data(), &info) != 0) {
        //no permission to access
        return false;
    } else if(info.st_mode & S_IFDIR) {
        //is a directory
        return true;
    } else {
        //not a directory
        return false;
    }
}
void make_folder(std::string_view path) //the path may or may not end in ('/' or '\\')
{
    //We can only create one level of folders at a time, so we have to create each level
    //of folders one at a time. In addition, don't try recreating any folders that already
    //exist; it'll fail and result in an annoying log_error statement (it shouldn't cause
    //bugs though).
    for(size_t i=0; i<=path.size(); i++) {
        if((i==path.size() || path[i]=='/' || path[i]=='\\') &&
           !folder_exists(path.substr(0, i)))
        {
            #ifdef _WIN32
            if(mkdir(path.substr(0, i).data()) != 0)
                log_error("failed to make folder " + (std::string)path.substr(0, i));
            #else
            //UNTESTED! (but should work)
            if(mkdir(path.substr(0, i).data(), S_IRWXU | S_IRWXG | S_IRWXO) != 0)
                log_error("failed to make folder " + (std::string)path.substr(0, i));
            #endif
        }
    }
}
std::optional<std::string> read_binary_file(std::string_view file_path)
{
    std::ifstream file(file_path.data(), std::ios::binary);
    if(file.fail()) {
        log_error("failed to open file \"" + to_str(file_path) + "\"");
        return {};
    }
    file.seekg(0, std::ios::end);
    auto file_len = file.tellg();
    file.seekg(0, std::ios::beg);
    std::string file_contents;
    file_contents.resize(file_len);
    file.read((char*)file_contents.data(), file_len);
    return file_contents;
}

}}
