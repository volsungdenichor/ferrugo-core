#pragma once

#include <ferrugo/core/types.hpp>
#include <functional>
#include <tuple>

namespace ferrugo
{
namespace core
{

template <class T>
using producer_t = std::function<T()>;

template <class... Args>
using action_t = std::function<void(Args...)>;

template <class T>
using applier_t = action_t<T&>;

template <class... Args>
using predicate_t = std::function<bool(in_t<Args>...)>;

template <class T>
using compare_t = predicate_t<T, T>;

struct identity
{
    template <class T>
    constexpr T operator()(T&& item) const noexcept
    {
        return std::forward<T>(item);
    }
};

namespace detail
{

struct applied_fn
{
    template <class Func>
    struct impl
    {
        Func m_func;

        template <class Tuple>
        auto operator()(Tuple&& tuple) const -> decltype(std::apply(m_func, std::forward<Tuple>(tuple)))
        {
            return std::apply(m_func, std::forward<Tuple>(tuple));
        }
    };

    template <class Func>
    auto operator()(Func&& func) const -> impl<std::decay_t<Func>>
    {
        return { std::forward<Func>(func) };
    }
};

struct proj_fn
{
    template <class Func, class Proj>
    struct impl
    {
        Func m_func;
        Proj m_proj;

        template <class... Args>
        constexpr auto operator()(Args&&... args) const -> decltype(std::invoke(m_func, std::invoke(m_proj, args)...))
        {
            return std::invoke(m_func, std::invoke(m_proj, args)...);
        }
    };

    template <class Func, class Proj>
    auto operator()(Func&& func, Proj&& proj) const -> impl<std::decay_t<Func>, std::decay_t<Proj>>
    {
        return { std::forward<Func>(func), std::forward<Proj>(proj) };
    }
};

template <std::size_t I>
struct get_element_fn
{
    template <class T>
    auto operator()(T&& item) const -> decltype(std::get<I>(std::forward<T>(item)))
    {
        return std::get<I>(std::forward<T>(item));
    }
};

struct do_all_fn
{
    template <class... Funcs>
    struct impl
    {
        std::tuple<Funcs...> m_funcs;

        template <class... Args>
        void operator()(Args&&... args) const
        {
            call(std::index_sequence_for<Funcs...>{}, std::forward<Args>(args)...);
        }

        template <class... Args, std::size_t... I>
        void call(std::index_sequence<I...>, Args&&... args) const
        {
            (std::invoke(std::get<I>(m_funcs), std::forward<Args>(args)...), ...);
        }
    };

    template <class... Funcs>
    auto operator()(Funcs&&... funcs) const -> impl<std::decay_t<Funcs>...>
    {
        return { std::forward_as_tuple(std::forward<Funcs>(funcs)...) };
    }
};

struct apply_fn
{
    template <class Func>
    struct impl
    {
        Func m_func;

        template <class T>
        T& operator()(T& item) const
        {
            std::invoke(m_func, item);
            return item;
        }
    };

    template <class... Funcs>
    auto operator()(Funcs&&... funcs) const -> impl<decltype(do_all_fn{}(std::forward<Funcs>(funcs)...))>
    {
        return { do_all_fn{}(std::forward<Funcs>(funcs)...) };
    }
};

struct with_fn
{
    template <class Func>
    struct impl
    {
        Func m_func;

        template <class T>
        T operator()(T item) const
        {
            std::invoke(m_func, item);
            return item;
        }
    };

    template <class... Funcs>
    auto operator()(Funcs&&... funcs) const -> impl<decltype(do_all_fn{}(std::forward<Funcs>(funcs)...))>
    {
        return { do_all_fn{}(std::forward<Funcs>(funcs)...) };
    }
};

}  // namespace detail

static constexpr auto applied = detail::applied_fn{};
static constexpr auto proj = detail::proj_fn{};

template <std::size_t I>
static constexpr inline auto get_element = detail::get_element_fn<I>{};

static constexpr inline auto get_first = get_element<0>;
static constexpr inline auto get_second = get_element<1>;
static constexpr inline auto get_key = get_element<0>;
static constexpr inline auto get_value = get_element<1>;

static constexpr inline auto apply = detail::apply_fn{};
static constexpr inline auto with = detail::with_fn{};
;

}  // namespace core
}  // namespace ferrugo
