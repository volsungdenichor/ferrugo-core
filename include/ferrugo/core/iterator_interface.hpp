#pragma once

#include <ferrugo/core/type_traits.hpp>

namespace ferrugo
{

namespace core
{

namespace detail
{

template <class T>
using has_deref = decltype(std::declval<T>().deref());

template <class T>
using has_inc = decltype(std::declval<T>().inc());

template <class T>
using has_dec = decltype(std::declval<T>().dec());

template <class T>
using has_advance = decltype(std::declval<T>().advance(std::declval<convertible_to<std::is_integral>>()));

template <class T>
using has_is_equal = decltype(std::declval<T>().is_equal(std::declval<T>()));

template <class T>
using has_is_less = decltype(std::declval<T>().is_less(std::declval<T>()));

template <class T>
using has_distance_to = decltype(std::declval<T>().distance_to(std::declval<T>()));

template <class T, class = std::void_t<>>
struct difference_type_impl
{
    using type = std::ptrdiff_t;
};

template <class T>
struct difference_type_impl<T, std::void_t<has_distance_to<T>>>
{
    using type = decltype(std::declval<T>().distance_to(std::declval<T>()));
};

}  // namespace detail

template <class T>
using difference_type = typename detail::difference_type_impl<T>::type;

template <class T>
struct pointer_proxy
{
    T item;

    T* operator->()
    {
        return std::addressof(item);
    }
};

template <class Impl>
struct iterator_interface
{
    Impl m_impl;

    template <class... Args, require<std::is_constructible<Impl, Args...>{}> = 0>
    iterator_interface(Args&&... args) : m_impl{ std::forward<Args>(args)... }
    {
    }

    iterator_interface() = default;
    iterator_interface(const iterator_interface&) = default;
    iterator_interface(iterator_interface&&) = default;

    iterator_interface& operator=(iterator_interface other)
    {
        std::swap(m_impl, other.m_impl);
        return *this;
    }

    static_assert(satisfies<Impl, detail::has_deref>{}, ".deref required");
    static_assert(std::is_default_constructible<Impl>{}, "iterator_interface: default constructible Impl required");

    using reference = decltype(m_impl.deref());

private:
    template <class R = reference, require<std::is_reference<R>{}> = 0>
    auto get_pointer() const -> std::add_pointer_t<reference>
    {
        return std::addressof(**this);
    }

    template <class R = reference, require<!std::is_reference<R>{}> = 0>
    auto get_pointer() const -> pointer_proxy<reference>
    {
        return { **this };
    }

    template <class I = Impl, require<satisfies_any<I, detail::has_advance, detail::has_inc>{}> = 0>
    void inc()
    {
        if constexpr (satisfies<I, detail::has_inc>{})
        {
            m_impl.inc();
        }
        else
        {
            m_impl.advance(+1);
        }
    }

    template <class I = Impl, require<satisfies_any<I, detail::has_advance, detail::has_dec>{}> = 0>
    void dec()
    {
        if constexpr (satisfies<I, detail::has_dec>{})
        {
            m_impl.dec();
        }
        else
        {
            m_impl.advance(-1);
        }
    }

    template <class I = Impl, require<satisfies_any<I, detail::has_is_equal, detail::has_distance_to>{}> = 0>
    bool is_equal(const Impl& other) const
    {
        if constexpr (satisfies<I, detail::has_is_equal>{})
        {
            return m_impl.is_equal(other);
        }
        else
        {
            return m_impl.distance_to(other) == 0;
        }
    }

    template <class I = Impl, require<satisfies_any<I, detail::has_is_less, detail::has_distance_to>{}> = 0>
    bool is_less(const Impl& other) const
    {
        if constexpr (satisfies<I, detail::has_is_less>{})
        {
            return m_impl.is_less(other);
        }
        else
        {
            return m_impl.distance_to(other) > 0;
        }
    }

public:
    reference operator*() const
    {
        return m_impl.deref();
    }

    auto operator->() const -> decltype(get_pointer())
    {
        return get_pointer();
    }

    template <class I = Impl, require<satisfies_any<I, detail::has_advance, detail::has_inc>{}> = 0>
    iterator_interface& operator++()
    {
        inc();
        return *this;
    }

    template <class I = Impl, require<satisfies_any<I, detail::has_advance, detail::has_inc>{}> = 0>
    iterator_interface operator++(int)
    {
        iterator_interface tmp{ *this };
        ++(*this);
        return tmp;
    }

    template <class I = Impl, require<satisfies_any<I, detail::has_advance, detail::has_dec>{}> = 0>
    iterator_interface& operator--()
    {
        dec();
        return *this;
    }

    template <class I = Impl, require<satisfies_any<I, detail::has_advance, detail::has_dec>{}> = 0>
    iterator_interface operator--(int)
    {
        iterator_interface tmp{ *this };
        --(*this);
        return tmp;
    }

    template <class D, class I = Impl, require<satisfies<I, detail::has_advance>{} && std::is_integral<D>{}> = 0>
    friend iterator_interface& operator+=(iterator_interface& it, D offset)
    {
        it.m_impl.advance(offset);
        return it;
    }

    template <class D, class I = Impl, require<satisfies<I, detail::has_advance>{} && std::is_integral<D>{}> = 0>
    friend iterator_interface operator+(iterator_interface it, D offset)
    {
        return it += offset;
    }

    template <class D, class I = Impl, require<satisfies<I, detail::has_advance>{} && std::is_integral<D>{}> = 0>
    friend iterator_interface operator+(D offset, iterator_interface it)
    {
        return it + offset;
    }

    template <class D, class I = Impl, require<satisfies<I, detail::has_advance>{} && std::is_integral<D>{}> = 0>
    friend iterator_interface& operator-=(iterator_interface& it, D offset)
    {
        return it += -offset;
    }

    template <class D, class I = Impl, require<satisfies<I, detail::has_advance>{} && std::is_integral<D>{}> = 0>
    friend iterator_interface operator-(iterator_interface it, D offset)
    {
        return it -= offset;
    }

    template <class D, class I = Impl, require<satisfies<I, detail::has_advance>{} && std::is_integral<D>{}> = 0>
    reference operator[](D offset) const
    {
        return *(*this + offset);
    }

    template <class I = Impl, require<satisfies_any<I, detail::has_is_equal, detail::has_distance_to>{}> = 0>
    friend bool operator==(const iterator_interface& lhs, const iterator_interface& rhs)
    {
        return lhs.is_equal(rhs.m_impl);
    }

    template <class I = Impl, require<satisfies_any<I, detail::has_is_equal, detail::has_distance_to>{}> = 0>
    friend bool operator!=(const iterator_interface& lhs, const iterator_interface& rhs)
    {
        return !(lhs == rhs);
    }

    template <class I = Impl, require<satisfies_any<I, detail::has_is_equal, detail::has_distance_to>{}> = 0>
    friend bool operator<(const iterator_interface& lhs, const iterator_interface& rhs)
    {
        return lhs.is_less(rhs.m_impl);
    }

    template <class I = Impl, require<satisfies_any<I, detail::has_is_less, detail::has_distance_to>{}> = 0>
    friend bool operator>(const iterator_interface& lhs, const iterator_interface& rhs)
    {
        return rhs < lhs;
    }

    template <class I = Impl, require<satisfies_any<I, detail::has_is_less, detail::has_distance_to>{}> = 0>
    friend bool operator<=(const iterator_interface& lhs, const iterator_interface& rhs)
    {
        return !(lhs > rhs);
    }

    template <class I = Impl, require<satisfies_any<I, detail::has_is_less, detail::has_distance_to>{}> = 0>
    friend bool operator>=(const iterator_interface& lhs, const iterator_interface& rhs)
    {
        return !(lhs < rhs);
    }

    template <class I = Impl, require<satisfies<I, detail::has_distance_to>{}> = 0>
    friend auto operator-(const iterator_interface& lhs, const iterator_interface& rhs) -> difference_type<I>
    {
        return rhs.m_impl.distance_to(lhs.m_impl);
    }
};

template <class T, class = std::void_t<>>
struct iterator_category_impl
{
    using type = std::conditional_t<
        satisfies_all<T, detail::has_advance, detail::has_distance_to>{},
        std::random_access_iterator_tag,
        std::conditional_t<
            satisfies_any<T, detail::has_dec, detail::has_advance>{},
            std::bidirectional_iterator_tag,
            std::forward_iterator_tag>>;
};

template <class T>
struct iterator_category_impl<T, std::void_t<typename T::iterator_category>>
{
    using type = typename T::iterator_category;
};

}  // namespace core

}  // namespace ferrugo

namespace std
{

template <class Impl>
struct iterator_traits<::ferrugo::core::iterator_interface<Impl>>
{
    using it = ::ferrugo::core::iterator_interface<Impl>;
    using reference = decltype(std::declval<it>().operator*());
    using pointer = decltype(std::declval<it>().operator->());
    using value_type = std::decay_t<reference>;
    using difference_type = typename ::ferrugo::core::difference_type<Impl>;
    using iterator_category = typename ::ferrugo::core::iterator_category_impl<Impl>::type;
};

}  // namespace std
