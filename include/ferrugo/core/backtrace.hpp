#pragma once

#include <cxxabi.h>
#include <dlfcn.h>
#include <execinfo.h>

#include <iomanip>
#include <memory>
#include <string>
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
            os << "#"                                              //
               << std::right << std::setfill('0') << std::setw(3)  //
               << i << " "                                         //
               << std::left << std::setfill(' ')                   //
               << std::setw(16) << f.addr                          //
               << std::setw(32) << f.file                          //
               << std::setw(32) << f.function << "\n";
        }
        return os;
    }
};

inline auto backtrace(std::size_t n = 128) -> stack_t
{
    static const auto demangle = [](const char* s) -> std::string
    {
        int status = -4;
        std::unique_ptr<char, void (*)(void*)> demangled{ abi::__cxa_demangle(s, NULL, NULL, &status), std::free };
        return status == 0 ? demangled.get() : s;
    };

    std::vector<void*> callstack(n + 1);
    const int size = ::backtrace(callstack.data(), callstack.size());

    stack_t result;
    result.reserve(size);
    for (int i = 1; i < size; ++i)
    {
        void* addr = callstack[i];
        Dl_info info;
        if (!dladdr(addr, &info) || !info.dli_sname)
        {
            break;
        }
        result.push_back(frame_t{ info.dli_fname, demangle(info.dli_sname), addr });
    }
    return result;
}

}  // namespace core
}  // namespace ferrugo