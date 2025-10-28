#include <cassert>
#include <functional>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "next.hpp"
#include "parsing.hpp"

enum class loop_state_t
{
    loop_continue,
    loop_break,
    loop_return
};

// Base struct for all control results
template <class T = void>
struct step_t
{
    using value_type = std::optional<T>;
    loop_state_t control;
    value_type value;

    constexpr step_t(loop_state_t c) : control(c)
    {
    }

    constexpr step_t(loop_state_t c, T val) : control(c), value(std::move(val))
    {
    }
};

template <>
struct step_t<void>
{
    using value_type = void;
    loop_state_t control;

    constexpr step_t(loop_state_t c) : control(c)
    {
        assert(c != loop_state_t::loop_return);
    }

    template <class T>
    constexpr operator step_t<T>() const
    {
        return step_t<T>{ control };
    }
};

template <class T>
struct is_step : std::false_type
{
};

template <class T>
struct is_step<step_t<T>> : std::true_type
{
};

template <class T>
struct step_underlying_type_impl;

template <class T>
struct step_underlying_type_impl<step_t<T>>
{
    using type = typename step_t<T>::value_type;
};

template <class T>
using step_underlying_type = typename step_underlying_type_impl<T>::type;

constexpr inline auto loop_continue = step_t<>{ loop_state_t::loop_continue };
constexpr inline auto loop_break = step_t<>{ loop_state_t::loop_break };

template <typename T>
inline constexpr auto loop_return(T val) -> step_t<T>
{
    return { loop_state_t::loop_return, std::move(val) };
}

struct for_each_fn
{
    template <class Iter, class Func>
    auto operator()(Iter b, Iter e, Func&& func) const
    {
        using reference = typename std::iterator_traits<Iter>::reference;
        using invoke_result = std::invoke_result_t<Func, reference>;
        static_assert(is_step<invoke_result>::value, "step_t<T> type required");

        using return_type = step_underlying_type<invoke_result>;
        if constexpr (!std::is_void_v<return_type>)
        {
            for (auto it = b; it != e; ++it)
            {
                auto res = std::invoke(func, *it);
                switch (res.control)
                {
                    case loop_state_t::loop_break: return return_type{};
                    case loop_state_t::loop_continue: continue;
                    case loop_state_t::loop_return: return res.value;
                }
            }
            return return_type{};
        }
        else
        {
            for (auto it = b; it != e; ++it)
            {
                auto res = std::invoke(func, *it);
                if (res.control == loop_state_t::loop_break)
                {
                    break;
                }
            }
        }
    }

    template <class Range, class Func>
    auto operator()(Range&& range, Func&& func) const
    {
        return (*this)(std::begin(range), std::end(range), std::forward<Func>(func));
    }
} for_each{};

constexpr inline auto iterate = for_each;

int run(const std::vector<std::string_view>& args)
{
    std::vector<int> v = { 1, 3, 3, 9, -1 };
    iterate(
        v,
        [](int item) -> step_t<>
        {
            if (item == 5)
            {
                return loop_break;
            }
            std::cout << item << "\n";
            return loop_continue;
        });
    std::cout << "Total ";
    return 0;
}

int main(int argc, char* argv[])
{
    try
    {
        return run({ argv, argv + argc });
    }
    catch (const std::exception& ex)
    {
        std::cerr << ex.what() << '\n';
        return -1;
    }
}
