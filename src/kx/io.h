#pragma once

#include <iostream>
#include <string>
#include <utility>
#include <mutex>
#include <optional>

namespace kx { namespace io {

/** all printing functions and flush() are thread-safe assuming no one else
 *  is using stdout besides this file
 */

extern std::mutex print_mutex;

template<class T> void print(T &&s)
{
    std::lock_guard<std::mutex> lg(print_mutex);
    std::cout << std::forward<T>(s);
}
template<class T> void println(T &&s)
{
    std::lock_guard<std::mutex> lg(print_mutex);
    std::cout << std::forward<T>(s) << std::endl;
}
void println();
void flush();

bool folder_exists(std::string_view path);
void make_folder(std::string_view path); ///the path may or may not end in ('/' or '\\')

///thread-safe as long as the file isn't being modified by someone else
std::optional<std::string> read_binary_file(std::string_view file_path);

}}
