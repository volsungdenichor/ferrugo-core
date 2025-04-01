#pragma once

#include <ferrugo/core/either.hpp>
#include <ferrugo/core/error_handling.hpp>
#include <functional>
#include <iostream>
#include <type_traits>

namespace ferrugo
{
namespace core
{

struct none_t
{
};

inline constexpr none_t none = {};

struct bad_maybe_access : std::runtime_error
{
    bad_maybe_access() : std::runtime_error("bad maybe access")
    {
    }

    bad_maybe_access(const std::string& msg) : std::runtime_error(msg)
    {
    }
};

namespace detail
{

template <class T>
struct maybe_base
{
    using storage_type = either_storage<none_t, T>;
    using value_type = T;

    storage_type m_storage;

    maybe_base() : m_storage(in_place_left)
    {
    }

    maybe_base(const T& value) : m_storage(in_place_right, value)
    {
    }

    maybe_base(T&& value) : m_storage(in_place_right, std::move(value))
    {
    }

    template <class U, class = std::enable_if_t<std::is_constructible_v<T, U>>>
    maybe_base(U&& value) : m_storage(in_place_right, std::forward<U>(value))
    {
    }

    template <class U, class = std::enable_if_t<std::is_constructible_v<T, U>>>
    maybe_base(const maybe_base<U>& other) : maybe_base()
    {
        if (other.has_value())
        {
            emplace(*other);
        }
    }

    template <class U, class = std::enable_if_t<std::is_constructible_v<T, U>>>
    maybe_base(maybe_base<U>&& other) : maybe_base()
    {
        if (other.has_value())
        {
            emplace(*std::move(other));
        }
    }

    maybe_base(const maybe_base& other) : maybe_base()
    {
        if (other.has_value())
        {
            emplace(*other);
        }
    }

    maybe_base(maybe_base&& other) : maybe_base()
    {
        if (other.has_value())
        {
            emplace(*std::move(other));
        }
    }

    void swap(maybe_base& other)
    {
        m_storage.swap(other.m_storage);
    }

    void reset()
    {
        m_storage.emplace_left(none);
    }

    template <class... Args>
    void emplace(Args&&... args)
    {
        m_storage.emplace_right(std::forward<Args>(args)...);
    }

    bool has_value() const
    {
        return m_storage.is_right();
    }

    template <class E = bad_maybe_access, class... Args>
    const value_type& value(Args&&... args) const&
    {
        ensure<E>(has_value(), std::forward<Args>(args)...);
        return m_storage.get_right();
    }

    template <class E = bad_maybe_access, class... Args>
    value_type& value(Args&&... args) &
    {
        ensure<E>(has_value(), std::forward<Args>(args)...);
        return m_storage.get_right();
    }

    template <class E = bad_maybe_access, class... Args>
    value_type&& value(Args&&... args) &&
    {
        ensure<E>(has_value(), std::forward<Args>(args)...);
        return std::move(m_storage).get_right();
    }

    template <class E = bad_maybe_access, class... Args>
    const value_type&& value(Args&&... args) const&&
    {
        ensure<E>(has_value(), std::forward<Args>(args)...);
        return std::move(m_storage).get_right();
    }

    const value_type& operator*() const&
    {
        return (*this).value();
    }

    value_type& operator*() &
    {
        return (*this).value();
    }

    value_type&& operator*() &&
    {
        return std::move(*this).value();
    }

    const value_type&& operator*() const&&
    {
        return std::move(*this).value();
    }

    const value_type* operator->() const&
    {
        return &**this;
    }

    value_type* operator->() &
    {
        return &**this;
    }
};

template <class T>
struct maybe_base<T&>
{
    using storage_type = T*;
    storage_type m_storage;

    maybe_base() : m_storage(nullptr)
    {
    }

    maybe_base(T& value) : m_storage(&value)
    {
    }

    maybe_base(const maybe_base& other) : m_storage(other.m_value)
    {
    }

    void swap(maybe_base& other)
    {
        std::swap(m_storage, other.m_storage);
    }

    void reset()
    {
        m_storage = nullptr;
    }

    bool has_value() const
    {
        return m_storage;
    }

    template <class E = bad_maybe_access, class... Args>
    T& value(Args&&... args) const
    {
        ensure<E>(has_value(), std::forward<Args>(args)...);
        return *m_storage;
    }

    T& operator*() const
    {
        return (*this).value();
    }

    T* operator->() const
    {
        return &**this;
    }
};

}  // namespace detail

template <class T>
struct maybe;

template <class T>
struct is_maybe : std::false_type
{
};

template <class T>
struct is_maybe<maybe<T>> : std::true_type
{
};

template <class T>
struct maybe_underlying_type;

template <class T>
using maybe_underlying_type_t = typename maybe_underlying_type<T>::type;

template <class T>
struct maybe_underlying_type<maybe<T>>
{
    using type = T;
};

namespace impl
{

template <class T>
auto some(T&& value) -> maybe<std::decay_t<T>>
{
    return std::forward<T>(value);
}

template <class Self, class Pred>
auto filter(Self&& self, Pred&& pred) -> std::decay_t<Self>
{
    if (self && std::invoke(std::forward<Pred>(pred), *std::forward<Self>(self)))
    {
        return std::forward<Self>(self);
    }
    return {};
}

template <class Self, class Func>
auto and_then(Self&& self, Func&& func) -> decltype(std::invoke(std::forward<Func>(func), *std::forward<Self>(self)))
{
    if (self)
    {
        return std::invoke(std::forward<Func>(func), *std::forward<Self>(self));
    }
    return {};
}

template <class Self, class Func>
auto transform(Self&& self, Func&& func) -> decltype(some(std::invoke(std::forward<Func>(func), *std::forward<Self>(self))))
{
    if (self)
    {
        return some(std::invoke(std::forward<Func>(func), *std::forward<Self>(self)));
    }
    return {};
}

template <class Self, class Func>
auto or_else(Self&& self, Func&& func) -> std::decay_t<Self>
{
    if constexpr (std::is_void_v<std::invoke_result_t<Func>>)
    {
        if (!self)
        {
            std::invoke(std::forward<Func>(func));
        }
        return std::forward<Self>(self);
    }
    else
    {
        return self ? std::forward<Self>(self) : std::invoke(std::forward<Func>(func));
    }
}

template <class Self, class U>
auto value_or(Self&& self, U&& default_value) -> std::decay_t<U>
{
    return self ? *std::forward<Self>(self) : std::forward<U>(default_value);
}

}  // namespace impl

template <class T>
struct maybe : detail::maybe_base<T>
{
    using base_t = detail::maybe_base<T>;
    using base_t::base_t;

    maybe(none_t) : maybe()
    {
    }

    maybe& operator=(maybe other)
    {
        this->swap(other);
        return *this;
    }

    explicit operator bool() const
    {
        return this->has_value();
    }

    // ---

    template <class Pred>
    auto filter(Pred&& pred) const& -> decltype(impl::filter(*this, std::forward<Pred>(pred)))
    {
        return impl::filter(*this, std::forward<Pred>(pred));
    }

    template <class Pred>
    auto filter(Pred&& pred) && -> decltype(impl::filter(std::move(*this), std::forward<Pred>(pred)))
    {
        return impl::filter(std::move(*this), std::forward<Pred>(pred));
    }

    // ---

    template <class Func>
    auto and_then(Func&& func) const& -> decltype(impl::and_then(*this, std::forward<Func>(func)))
    {
        return impl::and_then(*this, std::forward<Func>(func));
    }

    template <class Func>
    auto and_then(Func&& func) && -> decltype(impl::and_then(std::move(*this), std::forward<Func>(func)))
    {
        return impl::and_then(std::move(*this), std::forward<Func>(func));
    }

    // ---

    template <class Func>
    auto transform(Func&& func) const& -> decltype(impl::transform(*this, std::forward<Func>(func)))
    {
        return impl::transform(*this, std::forward<Func>(func));
    }

    template <class Func>
    auto transform(Func&& func) && -> decltype(impl::transform(std::move(*this), std::forward<Func>(func)))
    {
        return impl::transform(std::move(*this), std::forward<Func>(func));
    }

    // ---

    template <class Func>
    auto or_else(Func&& func) const& -> decltype(impl::or_else(*this, std::forward<Func>(func)))
    {
        return impl::or_else(*this, std::forward<Func>(func));
    }

    template <class Func>
    auto or_else(Func&& func) && -> decltype(impl::or_else(std::move(*this), std::forward<Func>(func)))
    {
        return impl::or_else(std::move(*this), std::forward<Func>(func));
    }

    // ---

    template <class U>
    auto value_or(U&& default_value) const& -> decltype(impl::value_or(*this, std::forward<U>(default_value)))
    {
        return impl::value_or(*this, std::forward<U>(default_value));
    }

    template <class U>
    auto value_or(U&& default_value) && -> decltype(impl::value_or(std::move(*this), std::forward<U>(default_value)))
    {
        return impl::value_or(std::move(*this), std::forward<U>(default_value));
    }
};

template <class T>
std::ostream& operator<<(std::ostream& os, const maybe<T>& item)
{
    return item ? os << "some(" << *item << ")" : os << "none";
}

template <class L, class R>
bool operator==(const maybe<L>& lhs, const maybe<R>& rhs)
{
    return (!lhs && !rhs) || (lhs && rhs && *lhs == *rhs);
}

template <class L, class R>
bool operator==(const maybe<L>& lhs, const R& rhs)
{
    return lhs && *lhs == rhs;
}

template <class L, class R>
bool operator==(const L& lhs, const maybe<R>& rhs)
{
    return rhs == lhs;
}

template <class L>
bool operator==(const maybe<L>& lhs, none_t)
{
    return !static_cast<bool>(lhs);
}

template <class R>
bool operator==(none_t, const maybe<R>& rhs)
{
    return !static_cast<bool>(rhs);
}

inline bool operator==(none_t, none_t)
{
    return true;
}

template <class L, class R>
bool operator!=(const maybe<L>& lhs, const maybe<R>& rhs)
{
    return !(lhs == rhs);
}

template <class L, class R>
bool operator!=(const maybe<L>& lhs, const R& rhs)
{
    return !(lhs == rhs);
}

template <class L, class R>
bool operator!=(const L& lhs, const maybe<R>& rhs)
{
    return !(lhs == rhs);
}

template <class L>
bool operator!=(const maybe<L>& lhs, none_t)
{
    return static_cast<bool>(lhs);
}

template <class R>
bool operator!=(none_t, const maybe<R>& rhs)
{
    return static_cast<bool>(rhs);
}

inline bool operator!=(none_t, none_t)
{
    return false;
}

}  // namespace core
}  // namespace ferrugo
