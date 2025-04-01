#include <array>
#include <ferrugo/core/backtrace.hpp>
#include <ferrugo/core/channel.hpp>
#include <ferrugo/core/demangle.hpp>
#include <ferrugo/core/error_handling.hpp>
#include <ferrugo/core/iterator_range.hpp>
#include <ferrugo/core/overloaded.hpp>
#include <ferrugo/core/sequence.hpp>
#include <ferrugo/core/sexpr.hpp>
#include <forward_list>
#include <fstream>
#include <functional>
#include <future>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <list>
#include <memory>
#include <mutex>
#include <numeric>
#include <optional>
#include <set>
#include <unordered_set>
#include <variant>
#include <vector>

#define L(...) [&](auto&& it) -> decltype((__VA_ARGS__)) { return (__VA_ARGS__); }

void run()
{
    using namespace ferrugo::core;
    const auto seq = range(20)
                         .drop_while(L(it < 5))
                         .take_while(L(it < 15))
                         .transform(L(std::to_string(it)))
                         .drop(1)
                         .transform(L("_" + it + "_"));

    for (const std::string& item : seq)
    {
        std::cout << item << "\n";
    }
}

int main()
{
    try
    {
        run();
    }
    catch (...)
    {
        std::cerr << "\n"
                  << "Error:"
                  << "\n"
                  << ferrugo::core::exception_proxy{};
    }
}
