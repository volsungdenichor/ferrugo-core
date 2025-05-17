#include <sys/select.h>
#include <unistd.h>

#include <array>
// #include <ferrugo/core/backtrace.hpp>
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
#include <typeindex>
#include <unordered_set>
#include <variant>
#include <vector>

#include "edn.hpp"

#define LAMBDA(...) [](auto&& it) -> decltype((__VA_ARGS__)) { return (__VA_ARGS__); }

auto print(edn::span<edn::value_t> args) -> edn::value_t
{
    for (const edn::value_t& arg : args)
    {
        arg.format(std::cout, edn::value_t::format_mode_t::str);
    }
    std::cout << std::endl;
    return {};
}

auto debug(edn::span<edn::value_t> args) -> edn::value_t
{
    for (const edn::value_t& arg : args)
    {
        arg.format(std::cout, edn::value_t::format_mode_t::repr);
    }
    std::cout << std::endl;
    return {};
}

auto inc(edn::span<edn::value_t> args) -> edn::value_t
{
    if (const auto v = args.at(0).if_integer())
    {
        return *v + 1;
    }
    if (const auto v = args.at(0).if_floating_point())
    {
        return *v + 1.0;
    }
    return {};
}

auto map(edn::span<edn::value_t> args) -> edn::value_t
{
    edn::value_t::list_t result;
    if (const auto callable = args.at(0).if_callable())
    {
        if (const auto v = args.at(1).if_vector())
        {
            for (const edn::value_t& item : *v)
            {
                result.push_back((*callable)(edn::span<edn::value_t>(&item, 1)));
            }
        }
        if (const auto v = args.at(1).if_list())
        {
            for (const edn::value_t& item : *v)
            {
                result.push_back((*callable)(edn::span<edn::value_t>(&item, 1)));
            }
        }
    }
    return result;
}

template <class Op>
struct binary_op
{
    auto operator()(edn::span<edn::value_t> args) const -> edn::value_t
    {
        if (args.size() != 2)
        {
            throw std::runtime_error{ "binary_op: two arguments expected" };
        }
        static const auto op = Op{};
        {
            const auto lhs = args.at(0).if_integer();
            const auto rhs = args.at(1).if_integer();
            if (lhs && rhs)
            {
                return op(*lhs, *rhs);
            }
        }

        {
            const auto lhs = args.at(0).if_floating_point();
            const auto rhs = args.at(1).if_integer();
            if (lhs && rhs)
            {
                return op(*lhs, *rhs);
            }
        }

        {
            const auto lhs = args.at(0).if_integer();
            const auto rhs = args.at(1).if_floating_point();
            if (lhs && rhs)
            {
                return op(*lhs, *rhs);
            }
        }

        {
            const auto lhs = args.at(0).if_floating_point();
            const auto rhs = args.at(1).if_floating_point();
            if (lhs && rhs)
            {
                return op(*lhs, *rhs);
            }
        }

        return {};
    }
};

struct S
{
    S(int x)
    {
        if (x > 100)
        {
            throw std::logic_error{ "No more than 100" };
        }
    }
};

void run()
{
#if 0
    auto stack = std::invoke(
        [&]() -> edn::stack_t
        {
            edn::stack_t result{ nullptr };
            result.insert(edn::value_t::symbol_t{ "print" }, edn::value_t::callable_t{ &print });
            result.insert(edn::value_t::symbol_t{ "debug" }, edn::value_t::callable_t{ &debug });

            result.insert(edn::value_t::symbol_t{ "+" }, edn::value_t::callable_t{ binary_op<std::plus<>>{} });
            result.insert(edn::value_t::symbol_t{ "-" }, edn::value_t::callable_t{ binary_op<std::minus<>>{} });
            result.insert(edn::value_t::symbol_t{ "*" }, edn::value_t::callable_t{ binary_op<std::multiplies<>>{} });
            result.insert(edn::value_t::symbol_t{ "/" }, edn::value_t::callable_t{ binary_op<std::divides<>>{} });

            result.insert(edn::value_t::symbol_t{ "=" }, edn::value_t::callable_t{ binary_op<std::equal_to<>>{} });
            result.insert(edn::value_t::symbol_t{ "!=" }, edn::value_t::callable_t{ binary_op<std::not_equal_to<>>{} });
            result.insert(edn::value_t::symbol_t{ "/=" }, edn::value_t::callable_t{ binary_op<std::not_equal_to<>>{} });
            result.insert(edn::value_t::symbol_t{ "<" }, edn::value_t::callable_t{ binary_op<std::less<>>{} });
            result.insert(edn::value_t::symbol_t{ ">" }, edn::value_t::callable_t{ binary_op<std::greater<>>{} });
            result.insert(edn::value_t::symbol_t{ "<=" }, edn::value_t::callable_t{ binary_op<std::less_equal<>>{} });
            result.insert(edn::value_t::symbol_t{ ">=" }, edn::value_t::callable_t{ binary_op<std::greater_equal<>>{} });

            result.insert(edn::value_t::symbol_t{ "inc" }, edn::value_t::callable_t{ &inc });
            result.insert(edn::value_t::symbol_t{ "map" }, edn::value_t::callable_t{ &map });
            return result;
        });

    const edn::value_t value = edn::parse(R"(
        (do
            (def one "ONE")
            (def adam-mickiewicz {:name "Adam" :birth 1798 :death 1855})
            (defn variadic [& args] (print "VARIADIC: " args))
            (defn procedure [a b & tail]
                (print "procedure: " "a=" a ", b=" b ", tail=" tail)
                (print #{12 2 3 9 (+ 1 2)})
                (cond
                    (= a 1) one
                    (= a 2) "TWO"
                    (= a 3) "THREE"
                    :else "NO IDEA"))
            (let [x 1 y (* x 10)]

                (procedure x y)
            )
                (variadic)
                (variadic #{:A :B :C})
                (variadic 0 {:a 3 :b 5.1})
                (variadic 0 2 "A")
                (variadic [0 2 "A"])
            (print (procedure 3 4 10 2))
            (debug adam-mickiewicz)
            (print (map inc [1 2 3 11 12 13]))
        )
    )");

    std::cout << value << "\n\n";
    const auto result = edn::evaluate(value, stack);
    std::cout << "> " << result << "\n";
#endif
    core::println(core::try_create<S>(32));
    core::println("A", std::vector<int>{}, std::array<int, 3>{});
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
