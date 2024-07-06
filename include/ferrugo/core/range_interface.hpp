#pragma once

#include <ferrugo/core/type_traits.hpp>

namespace ferrugo
{

namespace core
{

template <class Impl>
struct range_interface
{
    Impl m_impl;

    using begin_iterator = decltype(std::declval<const Impl&>().begin());
    using end_iterator = decltype(std::declval<const Impl&>().end());

    using iterator = begin_iterator;
    using const_iterator = iterator;

    using reference = iter_reference_t<iterator>;
    using difference_type = iter_difference_t<iterator>;
    using size_type = difference_type;

    template <class... Args, require<std::is_constructible<Impl, Args...>{}> = 0>
    range_interface(Args&&... args) : m_impl{ std::forward<Args>(args)... }
    {
    }

    range_interface(const range_interface&) = default;
    range_interface(range_interface&&) = default;

    const Impl& get_impl() const
    {
        return m_impl;
    }

    iterator begin() const
    {
        return m_impl.begin();
    }

    iterator end() const
    {
        return m_impl.end();
    }

    template <class Container, require<std::is_constructible<Container, iterator, iterator>{}> = 0>
    operator Container() const
    {
        return Container{ begin(), end() };
    }

    bool empty() const
    {
        return begin() == end();
    }

    reference front() const
    {
        return *begin();
    }

    template <class It = iterator, require<is_bidirectional_iterator<It>{}> = 0>
    reference back() const
    {
        return *std::prev(end());
    }

    template <class It = iterator, require<is_random_access_iterator<It>{}> = 0>
    reference operator[](difference_type n) const
    {
        return *std::next(begin(), n);
    }

    template <class It = iterator, require<is_random_access_iterator<It>{}> = 0>
    reference at(difference_type n) const
    {
        if (0 <= n && n < size())
        {
            return (*this)[n];
        }
        throw std::out_of_range{ "index out of range" };
    }

    template <class It = iterator, require<is_random_access_iterator<It>{}> = 0>
    size_type size() const
    {
        return std::distance(begin(), end());
    }
};

template <class T>
struct is_range_interface : std::false_type
{
};

template <class Impl>
struct is_range_interface<range_interface<Impl>> : std::true_type
{
};

}  // namespace core

}  // namespace ferrugo