#pragma once

#include <ferrugo/core/type_traits.hpp>
#include <iostream>
#include <optional>

namespace ferrugo
{

namespace core
{

namespace detail
{

struct none_t
{
    struct constructor_tag
    {
    };
    constexpr explicit none_t(constructor_tag)
    {
    }
};
}  // namespace detail

static constexpr inline auto none = detail::none_t{ detail::none_t::constructor_tag{} };

template <class T>
class optional;

template <class T>
class optional
{
public:
    using value_type = T;

    constexpr optional() noexcept = default;

    constexpr optional(detail::none_t) noexcept : optional()
    {
    }

    template <class U, require<std::is_constructible<value_type, U>{}> = 0>
    constexpr optional(U&& value) : m_storage{ std::forward<U>(value) }
    {
    }

    constexpr optional(const optional&) = default;
    constexpr optional(optional&&) noexcept = default;

    constexpr optional& operator=(optional other)
    {
        swap(other);
        return *this;
    }

    constexpr explicit operator bool() const noexcept
    {
        return static_cast<bool>(m_storage);
    }

    constexpr const value_type& operator*() const& noexcept
    {
        return *m_storage;
    }

    constexpr value_type& operator*() & noexcept
    {
        return *m_storage;
    }

    constexpr value_type&& operator*() && noexcept
    {
        return *std::move(m_storage);
    }

    constexpr const value_type* operator->() const& noexcept
    {
        return &**this;
    }

    constexpr value_type* operator->() & noexcept
    {
        return &**this;
    }

    constexpr value_type* operator->() && noexcept
    {
        return &**this;
    }

    constexpr void swap(optional& other) noexcept
    {
        std::swap(m_storage, other.m_storage);
    }

private:
    std::optional<T> m_storage;
};

template <class T>
class optional<T&>
{
public:
    using value_type = T;

    constexpr optional() noexcept : m_storage{}
    {
    }

    constexpr optional(detail::none_t) noexcept : optional()
    {
    }

    constexpr optional(T& value) : m_storage{ &value }
    {
    }

    constexpr optional(const optional& other) = default;

    constexpr optional(optional&& other) noexcept = default;

    constexpr optional& operator=(optional other) noexcept
    {
        swap(other);
        return *this;
    }

    constexpr explicit operator bool() const noexcept
    {
        return static_cast<bool>(m_storage);
    }

    constexpr value_type& operator*() const noexcept
    {
        return *m_storage;
    }

    constexpr value_type* operator->() const noexcept
    {
        return m_storage;
    }

    constexpr void swap(optional& other) noexcept
    {
        std::swap(m_storage, other.m_storage);
    }

private:
    T* m_storage;
};

template <class T>
std::ostream& operator<<(std::ostream& os, const optional<T>& item)
{
    if (item)
    {
        return os << "some(" << *item << ")";
    }
    else
    {
        return os << "none";
    }
}

template <class L, class R, class = std::invoke_result_t<std::equal_to<>, L, R>>
constexpr bool operator==(const optional<L>& lhs, const optional<R>& rhs)
{
    return (!lhs && !rhs) || (lhs && rhs && *lhs == *rhs);
}

template <class L, class R, class = std::invoke_result_t<std::equal_to<>, L, R>>
constexpr bool operator!=(const optional<L>& lhs, const optional<R>& rhs)
{
    return !(lhs == rhs);
}

template <class L, class R, class = std::invoke_result_t<std::equal_to<>, L, R>>
constexpr bool operator==(const optional<L>& lhs, const R& rhs)
{
    return lhs && (*lhs == rhs);
}

template <class L, class R, class = std::invoke_result_t<std::equal_to<>, L, R>>
constexpr bool operator!=(const optional<L>& lhs, const R& rhs)
{
    return !(lhs == rhs);
}

template <class L, class R, class = std::invoke_result_t<std::equal_to<>, L, R>>
constexpr bool operator==(const L& lhs, const optional<R>& rhs)
{
    return rhs == lhs;
}

template <class L, class R, class = std::invoke_result_t<std::equal_to<>, L, R>>
constexpr bool operator!=(const L& lhs, const optional<R>& rhs)
{
    return !(lhs == rhs);
}

template <class L>
constexpr bool operator==(const optional<L>& lhs, detail::none_t)
{
    return !static_cast<bool>(lhs);
}

template <class L>
constexpr bool operator!=(const optional<L>& lhs, detail::none_t)
{
    return static_cast<bool>(lhs);
}

template <class R>
constexpr bool operator==(detail::none_t, const optional<R>& rhs)
{
    return !static_cast<bool>(rhs);
}

template <class R>
constexpr bool operator!=(detail::none_t, const optional<R>& rhs)
{
    return static_cast<bool>(rhs);
}

template <class T>
struct optional_underlying_type;

template <class T>
using optional_underlying_type_t = typename optional_underlying_type<T>::type;

template <class T>
struct optional_underlying_type<optional<T>>
{
    using type = T;
};

}  // namespace core

}  // namespace ferrugo
