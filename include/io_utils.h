#ifndef _IO_UTILS_H__
#define _IO_UTILS_H__

#include <string>

std::string get_exec_directory(char *current_path)
{
    std::string current_path_string(current_path);
#ifdef _WIN32
    auto last_slash_index = current_path_string.find_last_of("\\");
#else
    auto last_slash_index = current_path_string.find_last_of("/");
#endif
    return current_path_string.substr(0, last_slash_index > 1 ? last_slash_index + 1 : 0);
}
#endif