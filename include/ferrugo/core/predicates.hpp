#pragma once

#include <ferrugo/core/ostream_utils.hpp>
#include <ferrugo/core/static_string.hpp>
#include <ferrugo/core/type_traits.hpp>
#include <functional>
#include <variant>

namespace ferrugo
{
namespace core
{

namespace detail
{

template <class L, class R>
using is_equality_comparable = decltype(std::declval<L>() == std::declval<R>());

template <class Pred, class T>
constexpr bool invoke_pred(Pred&& pred, T&& item)
{
    if constexpr (std::is_invocable_v<Pred, T>)
    {
        return std::invoke(std::forward<Pred>(pred), std::forward<T>(item));
    }
    else if constexpr (core::is_detected<is_equality_comparable, T, Pred>{})
    {
        return pred == item;
    }
}

struct all_tag
{
};
struct any_tag
{
};

template <class Tag, class Name>
struct compound_fn
{
    template <class... Preds>
    struct impl
    {
        std::tuple<Preds...> m_preds;

        template <class U>
        constexpr bool operator()(U&& item) const
        {
            if constexpr (std::is_same_v<Tag, any_tag>)
            {
                return std::apply(
                    [&](const auto&... preds) { return (... || invoke_pred(preds, std::forward<U>(item))); }, m_preds);
            }
            else if constexpr (std::is_same_v<Tag, all_tag>)
            {
                return std::apply(
                    [&](const auto&... preds) { return (... && invoke_pred(preds, std::forward<U>(item))); }, m_preds);
            }
        }

        friend std::ostream& operator<<(std::ostream& os, const impl& item)
        {
            static const auto name = Name{};

            os << "(" << name;
            std::apply(
                [&](const auto&... preds) { ((os << " " << ::ferrugo::core::safe_format(preds)), ...); }, item.m_preds);
            os << ")";
            return os;
        }
    };

    template <class Pipe>
    auto to_tuple(Pipe pipe) const -> std::tuple<Pipe>
    {
        return std::tuple<Pipe>{ std::move(pipe) };
    }

    template <class... Pipes>
    auto to_tuple(impl<Pipes...> pipe) const -> std::tuple<Pipes...>
    {
        return pipe.m_preds;
    }

    template <class... Pipes>
    auto from_tuple(std::tuple<Pipes...> tuple) const -> impl<Pipes...>
    {
        return impl<Pipes...>{ std::move(tuple) };
    }

    template <class... Pipes>
    auto operator()(Pipes... pipes) const -> decltype(from_tuple(std::tuple_cat(to_tuple(std::move(pipes))...)))
    {
        return from_tuple(std::tuple_cat(to_tuple(std::move(pipes))...));
    }
};

struct negate_fn
{
    template <class Pred>
    struct impl
    {
        Pred pred;

        template <class U>
        bool operator()(U&& item) const
        {
            return !invoke_pred(pred, std::forward<U>(item));
        }

        friend std::ostream& operator<<(std::ostream& os, const impl& item)
        {
            return os << "(not " << item.pred << ")";
        }
    };

    template <class Pred>
    auto operator()(Pred&& pred) const -> impl<std::decay_t<Pred>>
    {
        return impl<std::decay_t<Pred>>{ std::forward<Pred>(pred) };
    }
};

template <class Op, class Name>
struct compare_fn
{
    template <class T>
    struct impl
    {
        T value;

        template <class U>
        bool operator()(U&& item) const
        {
            static const auto op = Op{};
            return op(std::forward<U>(item), value);
        }

        friend std::ostream& operator<<(std::ostream& os, const impl& item)
        {
            static const auto name = Name{};
            return os << "(" << name << " " << item.value << ")";
        }
    };

    template <class T>
    auto operator()(T&& value) const -> impl<std::decay_t<T>>
    {
        return impl<std::decay_t<T>>{ std::forward<T>(value) };
    }
};

struct size_is_fn
{
    template <class Pred>
    struct impl
    {
        Pred pred;

        template <class U>
        bool operator()(U&& item) const
        {
            return invoke_pred(pred, std::distance(std::begin(item), std::end(item)));
        }

        friend std::ostream& operator<<(std::ostream& os, const impl& item)
        {
            return os << "(size_is " << item.pred << ")";
        }
    };

    template <class Pred>
    auto operator()(Pred&& pred) const -> impl<std::decay_t<Pred>>
    {
        return impl<std::decay_t<Pred>>{ std::forward<Pred>(pred) };
    }
};

struct is_empty_fn
{
    struct impl
    {
        template <class U>
        bool operator()(U&& item) const
        {
            return std::begin(item) == std::end(item);
        }

        friend std::ostream& operator<<(std::ostream& os, const impl& item)
        {
            return os << "(is_empty)";
        }
    };

    auto operator()() const -> impl
    {
        return impl{};
    }
};

struct each_fn
{
    template <class Pred>
    struct impl
    {
        Pred pred;

        template <class U>
        bool operator()(U&& item) const
        {
            return std::all_of(
                std::begin(item), std::end(item), [&](auto&& v) { return invoke_pred(pred, std::forward<decltype(v)>(v)); });
        }

        friend std::ostream& operator<<(std::ostream& os, const impl& item)
        {
            return os << "(each " << item.pred << ")";
        }
    };

    template <class Pred>
    auto operator()(Pred&& pred) const -> impl<std::decay_t<Pred>>
    {
        return impl<std::decay_t<Pred>>{ std::forward<Pred>(pred) };
    }
};

struct contains_fn
{
    template <class Pred>
    struct impl
    {
        Pred pred;

        template <class U>
        bool operator()(U&& item) const
        {
            return std::any_of(
                std::begin(item), std::end(item), [&](auto&& v) { return invoke_pred(pred, std::forward<decltype(v)>(v)); });
        }

        friend std::ostream& operator<<(std::ostream& os, const impl& item)
        {
            return os << "(contains " << item.pred << ")";
        }
    };

    template <class Pred>
    auto operator()(Pred&& pred) const -> impl<std::decay_t<Pred>>
    {
        return impl<std::decay_t<Pred>>{ std::forward<Pred>(pred) };
    }
};

}  // namespace detail

static constexpr inline auto any = detail::compound_fn<detail::any_tag, static_string<'a', 'n', 'y'>>{};
static constexpr inline auto all = detail::compound_fn<detail::all_tag, static_string<'a', 'l', 'l'>>{};
static constexpr inline auto negate = detail::negate_fn{};

static constexpr inline auto each = detail::each_fn{};
static constexpr inline auto contains = detail::contains_fn{};
static constexpr inline auto size_is = detail::size_is_fn{};
static constexpr inline auto is_empty = detail::is_empty_fn{};

static constexpr inline auto eq = detail::compare_fn<std::equal_to<>, static_string<'e', 'q'>>{};
static constexpr inline auto ne = detail::compare_fn<std::not_equal_to<>, static_string<'n', 'e'>>{};
static constexpr inline auto lt = detail::compare_fn<std::less<>, static_string<'l', 't'>>{};
static constexpr inline auto gt = detail::compare_fn<std::greater<>, static_string<'g', 't'>>{};
static constexpr inline auto le = detail::compare_fn<std::less_equal<>, static_string<'l', 'e'>>{};
static constexpr inline auto ge = detail::compare_fn<std::greater_equal<>, static_string<'g', 'e'>>{};

}  // namespace core
}  // namespace ferrugo