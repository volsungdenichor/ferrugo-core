#include <array>
#include <ferrugo/core/backtrace.hpp>
#include <ferrugo/core/channel.hpp>
#include <ferrugo/core/dimensions.hpp>
#include <ferrugo/core/error_handling.hpp>
#include <ferrugo/core/functional.hpp>
#include <ferrugo/core/iterator_range.hpp>
#include <ferrugo/core/ostream_utils.hpp>
#include <ferrugo/core/overloaded.hpp>
#include <ferrugo/core/sequence.hpp>
#include <ferrugo/core/sexpr.hpp>
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
#include <unordered_set>
#include <variant>
#include <vector>

#define L(...) [&](auto&& it) -> decltype((__VA_ARGS__)) { return (__VA_ARGS__); }

template <class Map, class Key>
auto map_equal_range(Map& map, const Key& key)
{
    const auto [b, e] = map.equal_range(key);
    return ferrugo::core::view(b, e);
}

template <class Map, class Key>
auto map_values(Map& map, const Key& key)
{
    return map_equal_range(map, key).transform(ferrugo::core::get_second);
}

template <class Map, class Key>
auto map_maybe_at(Map& map, const Key& key)
{
    return map_values(map, key).maybe_front();
}

template <class Map, class Key>
auto map_at(Map& map, const Key& key) -> decltype(auto)
{
    return map_values(map, key).maybe_front().value("map_at: key '", key, "' not found");
}

template <class Map, class Key = typename Map::key_type>
auto map_keys(const Map& map) -> ferrugo::core::sequence<const Key&>
{
    typename Map::const_iterator iter = map.begin();
    return ferrugo::core::sequence<const Key&>{ [&map, iter]() mutable -> ferrugo::core::iteration_result_t<const Key&>
                                                {
                                                    if (iter != map.end())
                                                    {
                                                        const Key& res = iter->first;
                                                        iter = map.upper_bound(iter->first);
                                                        return res;
                                                    }
                                                    return {};
                                                } };
}

template <class Map>
auto map_items(const Map& map)
{
    return map_keys(map).transform([&](const auto& key) { return std::pair{ key, map_values(map, key) }; });
}

void print(std::ostream& os, const ferrugo::core::sequence<int> seq)
{
    os << ferrugo::core::delimit(seq, ", ") << "\n";
}

ferrugo::core::sequence<int> number()
{
    return ferrugo::core::vec(10, 9, 8, 7);
}

void run()
{
    using namespace ferrugo::core;

    const auto f = fn(std::bind(std::plus<>{}, std::placeholders::_1, 1))
        |= fn(std::bind(std::multiplies<>{}, std::placeholders::_1, 10));
    std::cout << f(3) << "\n";
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
