#pragma once

#include <ferrugo/core/either.hpp>
#include <functional>
#include <iostream>
#include <type_traits>

namespace ferrugo
{
namespace core
{

struct in_place_t
{
};

struct none_t
{
};

template <class T>
struct maybe_base
{
    using storage_type = either_storage<none_t, T>;
    using value_type = T;

    storage_type m_storage;

    maybe_base() : m_storage(in_place_left)
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
            m_storage.construct(in_place_right, *other);
        }
    }

    template <class U, class = std::enable_if_t<std::is_constructible_v<T, U>>>
    maybe_base(maybe_base<U>&& other) : maybe_base()
    {
        if (other.has_value())
        {
            m_storage.construct(in_place_right, *std::move(other));
        }
    }

    bool has_value() const
    {
        return m_storage.is_right();
    }

    const value_type& operator*() const&
    {
        ensure_has_value();
        return m_storage.get_right();
    }

    value_type& operator*() &
    {
        ensure_has_value();
        return m_storage.get_right();
    }

    value_type&& operator*() &&
    {
        ensure_has_value();
        return std::move(m_storage).get_right();
    }

    const value_type&& operator*() const&&
    {
        ensure_has_value();
        return std::move(m_storage).get_right();
    }

    const value_type* operator->() const&
    {
        return &**this;
    }

    value_type* operator->() &
    {
        return &**this;
    }

    void swap(maybe_base& other)
    {
        m_storage.swap(other.m_storage);
    }

    void reset()
    {
        m_storage.construct(in_place_left);
    }

private:
    void ensure_has_value() const
    {
        if (!has_value())
        {
            throw std::runtime_error{ "maybe: missing value" };
        }
    }
};

template <class T>
struct maybe_base<T&>
{
    T* m_value;

    maybe_base() : m_value()
    {
    }

    maybe_base(T& value) : m_value(&value)
    {
    }

    maybe_base(const maybe_base& other) : m_value(other.m_value)
    {
    }

    bool has_value() const
    {
        return m_value;
    }

    T& operator*() const
    {
        ensure_has_value();
        return *m_value;
    }

    T* operator->() const
    {
        return &**this;
    }

    void swap(maybe_base& other)
    {
        std::swap(m_value, other.m_value);
    }

private:
    void ensure_has_value() const
    {
        if (!has_value())
        {
            throw std::runtime_error{ "maybe: missing value" };
        }
    }
};

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
auto or_else(Self&& self, Func&& func) -> decltype(std::invoke(std::forward<Func>(func)))
{
    return self ? std::forward<Self>(self) : std::invoke(std::forward<Func>(func));
}

template <class Self, class U>
auto value_or(Self&& self, U&& default_value) -> std::decay_t<U>
{
    return self ? *std::forward<Self>(self) : std::forward<U>(default_value);
}

}  // namespace impl

template <class T>
struct maybe : maybe_base<T>
{
    using base_t = maybe_base<T>;
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

    friend std::ostream& operator<<(std::ostream& os, const maybe& item)
    {
        return item ? os << "some{" << *item << "}" : os << "none";
    }
};

}  // namespace core
}  // namespace ferrugo
