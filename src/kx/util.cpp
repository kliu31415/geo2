#include "kx/io.h"
#include "kx/debug.h"

#include <string>
#include <vector>
#include <cmath>

namespace kx {

ScopeGuard::ScopeGuard(std::function<void()> func_) noexcept:
    func(std::move(func_))
{
    k_expects(func != nullptr);
}

ScopeGuard::~ScopeGuard()
{
    k_expects(func != nullptr);
    func();
}

std::vector<std::string> split_str(std::string_view str, char by)
{
    std::vector<std::string> parts;
    size_t i = 0;
    size_t j = 0;
    for(; i<str.size(); i++) {
        if(str[i] == by) {
            parts.push_back((std::string)str.substr(j, i-j));
            j = i+1;
        }
    }
    parts.push_back((std::string)str.substr(j, i-j));
    return parts;
}

int leading_digit(double val, int base)
{
    k_expects(val > 0);
    k_expects(base > 1);
    k_expects(!std::isnan(val));

    while(val < 1)
        val *= base;

    while(val >= base)
        val /= base;

    return (int)val;
}

}
