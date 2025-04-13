#pragma once

#include <cxxabi.h>
#include <dlfcn.h>
#include <execinfo.h>

#include <iomanip>
#include <iostream>
#include <memory>
#include <regex>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

namespace ferrugo
{
namespace core
{

struct frame_t
{
    std::string file;
    std::string function;
    void* addr;
};

struct stack_t : public std::vector<frame_t>
{
    friend std::ostream& operator<<(std::ostream& os, const stack_t& item)
    {
        for (std::size_t i = 0; i < item.size(); ++i)
        {
            const frame_t& f = item[i];
            os << "#" << std::right << std::setfill('0') << std::setw(3) << i        //
               << " " << std::right << std::setfill(' ') << std::setw(16) << f.addr  //
               << " " << std::left << std::setw(32) << std::setfill(' ') << f.file   //
               << " " << std::left << std::setfill(' ') << std::setw(32) << f.function << "\n";
        }
        return os;
    }
};

inline auto replace(std::regex regex, std::string to)
{
    return [=](const std::string& text) -> std::string
    {
        std::string result;
        std::regex_replace(std::back_inserter(result), text.begin(), text.end(), regex, to);
        return result;
    };
}

inline auto replace_type_names(std::string type) -> std::string
{
    static const std::vector<std::tuple<std::regex, std::string>> patterns
        = { { std::regex("std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >"), "std::string" },
            { std::regex("std::vector<(.*), std::allocator<(.*)> >"), "std::vector<$1>" } };

    for (const auto& [from, to] : patterns)
    {
        type = replace(from, to)(type);
    }
    return type;
}

inline auto backtrace(std::size_t frames_to_skip = 0, std::size_t n = 128) -> stack_t
{
    static constexpr auto demangle = [](const char* s) -> std::string
    {
        int status = -4;
        std::unique_ptr<char, void (*)(void*)> demangled{ abi::__cxa_demangle(s, nullptr, nullptr, &status), std::free };
        return status == 0 ? demangled.get() : s;
    };

    static constexpr auto get_file_name = [](const std::string& path) -> std::string
    {
        const auto delimiter = path.rfind('/');
        return delimiter != std::string::npos ? path.substr(delimiter + 1) : path;
    };

    static constexpr auto trim = [](const std::size_t max_size, std::string_view ellipsis = " (...)")
    {
        return [=](const std::string& text)
        {
            if (text.size() <= max_size)
            {
                return text;
            }
            else
            {
                return text.substr(0, max_size - ellipsis.size()) + std::string{ ellipsis };
            }
            return text;
        };
    };

    static constexpr auto to_frame = [](void* addr) -> frame_t
    {
        Dl_info info;
        return dladdr(addr, &info) ? frame_t{ info.dli_fname ? get_file_name(info.dli_fname) : "",
                                              info.dli_sname ? trim(128)(demangle(info.dli_sname)) : "",
                                              addr }
                                   : frame_t{ "", "", addr };
    };

    std::vector<void*> callstack(n + 1);
    const std::size_t size = static_cast<std::size_t>(::backtrace(callstack.data(), callstack.size()));

    stack_t result;
    result.reserve(size);
    std::transform(
        std::begin(callstack) + 1 + frames_to_skip, std::begin(callstack) + size, std::back_inserter(result), to_frame);
    return result;
}

}  // namespace core
}  // namespace ferrugo
