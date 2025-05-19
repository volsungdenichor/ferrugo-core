
#include <array>
#include <cmath>
#include <deque>
#include <ferrugo/core/channel.hpp>
#include <ferrugo/core/chrono.hpp>
#include <ferrugo/core/core.hpp>
#include <ferrugo/core/dimensions.hpp>
#include <ferrugo/core/error_handling.hpp>
#include <ferrugo/core/functional.hpp>
#include <ferrugo/core/iterator_range.hpp>
#include <ferrugo/core/ostream_utils.hpp>
#include <ferrugo/core/overloaded.hpp>
#include <ferrugo/core/rational.hpp>
#include <ferrugo/core/sequence.hpp>
#include <ferrugo/core/std_ostream.hpp>
#include <ferrugo/core/type_name.hpp>
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
#include <typeindex>
#include <unordered_set>
#include <variant>
#include <vector>

#define IT(...) [](auto&& it) -> decltype((__VA_ARGS__)) { return (__VA_ARGS__); }

void run()
{
    core::println(ferrugo::core::utc_time_t{ ferrugo::core::julian_date_t{} }.date);
}

int main()
{
    try
    {
        run();
    }
    catch (...)
    {
        std::cout << "\n"
                  << "Error:"
                  << "\n"
                  << ferrugo::core::exception_proxy{};
    }
}
