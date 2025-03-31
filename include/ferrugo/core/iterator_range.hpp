#pragma once

#include <algorithm>
#include <ferrugo/core/maybe.hpp>

namespace ferrugo
{
namespace core
{

template <class Category, class Iter>
struct iterator_of_category : std::is_base_of<Category, typename std::iterator_traits<Iter>::iterator_category>
{
};

template <bool C>
using require = std::enable_if_t<C, int>;

namespace detail
{
template <class Iter>
struct reverse_iterator_impl
{
    using type = std::reverse_iterator<Iter>;
};

template <class Iter>
struct reverse_iterator_impl<std::reverse_iterator<Iter>>
{
    using type = Iter;
};

template <class Iter>
using reverse_iterator_t = typename reverse_iterator_impl<Iter>::type;

template <class Iter>
auto make_reverse(Iter b, Iter e) -> std::pair<reverse_iterator_t<Iter>, reverse_iterator_t<Iter>>
{
    return { reverse_iterator_t<Iter>(e), reverse_iterator_t<Iter>(b) };
}

template <class Iter>
auto make_reverse(std::reverse_iterator<Iter> b, std::reverse_iterator<Iter> e) -> std::pair<Iter, Iter>
{
    return { e.base(), b.base() };
}

}  // namespace detail

struct slice_t
{
    maybe<std::ptrdiff_t> begin;
    maybe<std::ptrdiff_t> end;
};

template <class Iter>
class iterator_range
{
public:
    using iterator = Iter;
    using const_iterator = iterator;

    using reference = typename std::iterator_traits<iterator>::reference;
    using difference_type = typename std::iterator_traits<iterator>::difference_type;
    using size_type = difference_type;

    using maybe_reference = maybe<reference>;

    iterator_range() = default;
    iterator_range(const iterator_range&) = default;
    iterator_range(iterator_range&&) noexcept = default;

    iterator_range(iterator b, iterator e) : m_begin(b), m_end(e)
    {
    }

    iterator_range(const std::pair<iterator, iterator>& pair) : iterator_range(std::get<0>(pair), std::get<1>(pair))
    {
    }

    iterator_range(iterator b, size_type n) : iterator_range(b, std::next(b, n))
    {
    }

    iterator_range& operator=(iterator_range other) noexcept
    {
        std::swap(m_begin, other.m_begin);
        std::swap(m_end, other.m_end);
        return *this;
    }

    auto begin() const -> iterator
    {
        return m_begin;
    }

    auto end() const -> iterator
    {
        return m_end;
    }

    template <class Container, require<std::is_constructible_v<Container, iterator, iterator>> = 0>
    operator Container() const
    {
        return Container{ begin(), end() };
    }

    auto empty() const -> bool
    {
        return begin() == end();
    }

    template <class It = iterator, require<iterator_of_category<std::random_access_iterator_tag, It>::value> = 0>
    auto ssize() const -> difference_type
    {
        return std::distance(begin(), end());
    }

    template <class It = iterator, require<iterator_of_category<std::random_access_iterator_tag, It>::value> = 0>
    auto size() const -> difference_type
    {
        return std::distance(begin(), end());
    }

    auto front() const -> reference
    {
        if (empty())
        {
            throw std::out_of_range{ "iterator_range::front - empty range" };
        }
        return *begin();
    }

    template <class It = iterator, require<iterator_of_category<std::bidirectional_iterator_tag, It>::value> = 0>
    auto back() const -> reference
    {
        if (empty())
        {
            throw std::out_of_range{ "iterator_range::back - empty range" };
        }
        return *std::prev(end());
    }

    auto maybe_front() const -> maybe_reference
    {
        if (empty())
        {
            return {};
        }
        return *begin();
    }

    template <class It = iterator, require<iterator_of_category<std::bidirectional_iterator_tag, It>::value> = 0>
    auto maybe_back() const -> maybe_reference
    {
        if (empty())
        {
            return {};
        }
        return *std::prev(end());
    }

    template <class It = iterator, require<iterator_of_category<std::random_access_iterator_tag, It>::value> = 0>
    auto at(difference_type n) const -> reference
    {
        if (!(0 <= n && n < size()))
        {
            throw std::out_of_range{ "iterator_range::at - index out of range" };
        }
        return *std::next(begin(), n);
    }

    template <class It = iterator, require<iterator_of_category<std::random_access_iterator_tag, It>::value> = 0>
    auto operator[](difference_type n) const -> reference
    {
        return at(n);
    }

    template <class It = iterator, require<iterator_of_category<std::random_access_iterator_tag, It>::value> = 0>
    auto maybe_at(difference_type n) const -> maybe_reference
    {
        if (!(0 <= n && n < size()))
        {
            return {};
        }
        return *std::next(begin(), n);
    }

    template <class It = iterator, require<iterator_of_category<std::bidirectional_iterator_tag, It>::value> = 0>
    auto reverse() const -> iterator_range<detail::reverse_iterator_t<iterator>>
    {
        return detail::make_reverse(begin(), end());
    }

    auto take(difference_type n) const -> iterator_range
    {
        return iterator_range(begin(), advance(n));
    }

    auto drop(difference_type n) const -> iterator_range
    {
        return iterator_range(advance(n), end());
    }

    template <class It = iterator, require<iterator_of_category<std::bidirectional_iterator_tag, It>::value> = 0>
    auto take_back(difference_type n) const -> iterator_range
    {
        return reverse().take(n).reverse();
    }

    template <class It = iterator, require<iterator_of_category<std::bidirectional_iterator_tag, It>::value> = 0>
    auto drop_back(difference_type n) const -> iterator_range
    {
        return reverse().drop(n).reverse();
    }

    template <class Pred>
    auto take_while(Pred&& pred) const -> iterator_range
    {
        const iterator found = std::find_if_not(begin(), end(), std::forward<Pred>(pred));
        return iterator_range(begin(), found);
    }

    template <class Pred>
    auto drop_while(Pred&& pred) const -> iterator_range
    {
        const iterator found = std::find_if_not(begin(), end(), std::forward<Pred>(pred));
        return iterator_range(found, end());
    }

    template <class Pred, class It = iterator, require<iterator_of_category<std::bidirectional_iterator_tag, It>::value> = 0>
    auto take_back_while(Pred&& pred) const -> iterator_range
    {
        return reverse().take_while(std::forward<Pred>(pred)).reverse();
    }

    template <class Pred, class It = iterator, require<iterator_of_category<std::bidirectional_iterator_tag, It>::value> = 0>
    auto drop_back_while(Pred&& pred) const -> iterator_range
    {
        return reverse().drop_while(std::forward<Pred>(pred)).reverse();
    }

    template <class Pred>
    auto find_if(Pred&& pred) const -> maybe_reference
    {
        const iterator found = std::find_if(begin(), end(), std::forward<Pred>(pred));
        return found != end() ? maybe_reference{ *found } : maybe_reference{};
    }

    template <class It = iterator, require<iterator_of_category<std::random_access_iterator_tag, It>::value> = 0>
    auto slice(const slice_t& info) const -> iterator_range
    {
        static const auto adjust = [](difference_type index, size_type size) -> size_type {  //
            return std::clamp<size_type>(index >= 0 ? index : index + size, 0, size);
        };
        const size_type s = size();
        const size_type b = info.begin ? adjust(*info.begin, s) : size_type{ 0 };
        const size_type e = info.end ? adjust(*info.end, s) : s;
        return iterator_range{ begin() + b, std::max(size_type{ 0 }, e - b) };
    }

private:
    template <class It = iterator, require<iterator_of_category<std::random_access_iterator_tag, It>::value> = 0>
    auto advance(difference_type n) const -> iterator
    {
        return begin() + std::min(ssize(), n);
    }

    template <class It = iterator, require<!iterator_of_category<std::random_access_iterator_tag, It>::value> = 0>
    auto advance(difference_type n) const -> iterator
    {
        iterator it = begin();
        while (it != end() && n > 0)
        {
            --n;
            ++it;
        }
        return it;
    }

    iterator m_begin;
    iterator m_end;
};

template <class Container>
using subrange = iterator_range<decltype(std::begin(std::declval<Container>()))>;

template <class T>
using span = iterator_range<T*>;

}  // namespace core

}  // namespace ferrugo
