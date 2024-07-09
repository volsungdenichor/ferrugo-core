#pragma once

#include <algorithm>
#include <ferrugo/core/pipe.hpp>
#include <ferrugo/core/range_interface.hpp>
#include <optional>

namespace ferrugo
{

namespace core
{

namespace detail
{
template <class Iter>
struct subrange_impl
{
    std::pair<Iter, Iter> m_iterators;

    subrange_impl() : subrange_impl(Iter{}, Iter{})
    {
    }

    subrange_impl(Iter b, Iter e) : m_iterators{ b, e }
    {
    }

    subrange_impl(Iter b, std::ptrdiff_t n) : subrange_impl(b, std::next(b, n))
    {
    }

    template <class Range>
    subrange_impl(Range&& range) : subrange_impl(std::begin(range), std::end(range))
    {
    }

    Iter begin() const
    {
        return std::get<0>(m_iterators);
    }

    Iter end() const
    {
        return std::get<1>(m_iterators);
    }
};
}  // namespace detail

template <class Iter>
struct subrange : range_interface<detail::subrange_impl<Iter>>
{
    using base_t = range_interface<detail::subrange_impl<Iter>>;

    using base_t::base_t;
};

template <class Iter>
subrange(Iter, Iter) -> subrange<Iter>;

template <class Range>
subrange(Range&&) -> subrange<iterator_t<Range>>;

namespace detail
{

struct slice_fn
{
    struct impl
    {
        std::optional<std::ptrdiff_t> m_begin;
        std::optional<std::ptrdiff_t> m_end;

        static std::ptrdiff_t adjust(std::ptrdiff_t index, std::ptrdiff_t size)
        {
            return std::clamp<std::ptrdiff_t>(index >= 0 ? index : index + size, 0, size);
        }

        template <class Iter>
        auto operator()(subrange<Iter> item) const -> subrange<Iter>
        {
            const std::ptrdiff_t size = item.size();
            const std::ptrdiff_t b = m_begin ? adjust(*m_begin, size) : std::ptrdiff_t{ 0 };
            const std::ptrdiff_t e = m_end ? adjust(*m_end, size) : size;
            return subrange<Iter>{ std::begin(item) + b, std::max(std::ptrdiff_t{ 0 }, e - b) };
        }

        template <class Range>
        auto operator()(Range&& range) const
        {
            return (*this)(subrange{ std::forward<Range>(range) });
        }
    };

    auto operator()(std::optional<std::ptrdiff_t> begin, std::optional<std::ptrdiff_t> end) const -> pipe_t<impl>
    {
        return { impl{ begin, end } };
    }
};

}  // namespace detail

static constexpr inline auto slice = detail::slice_fn{};

}  // namespace core

}  // namespace ferrugo
