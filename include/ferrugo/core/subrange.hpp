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

struct reverse_fn
{
    template <class Iter>
    auto operator()(subrange<Iter> item) const -> subrange<std::reverse_iterator<Iter>>
    {
        return { std::reverse_iterator<Iter>{ std::end(item) }, std::reverse_iterator<Iter>{ std::begin(item) } };
    }

    template <class Iter>
    auto operator()(subrange<std::reverse_iterator<Iter>> item) const -> subrange<Iter>
    {
        return { std::end(item).base(), std::begin(item).base() };
    }

    template <class Range>
    auto operator()(Range&& range) const
    {
        return (*this)(subrange{ std::forward<Range>(range) });
    }
};

template <bool Before>
struct advance_while_fn
{
    template <class Pred>
    struct impl
    {
        Pred m_pred;

        template <class Iter>
        auto operator()(subrange<Iter> item) const -> subrange<Iter>
        {
            const auto b = std::begin(item);
            const auto e = std::end(item);
            const auto iter = std::find_if_not(b, e, std::ref(m_pred));

            if constexpr (Before)
            {
                return { b, iter };
            }
            else
            {
                return { iter, e };
            }
        }

        template <class Range>
        auto operator()(Range&& range) const
        {
            return (*this)(subrange{ std::forward<Range>(range) });
        }
    };

    template <class Pred>
    auto operator()(Pred&& pred) const -> pipe_t<impl<std::decay_t<Pred>>>
    {
        return { { std::forward<Pred>(pred) } };
    }
};

}  // namespace detail

static constexpr inline auto slice = detail::slice_fn{};
static constexpr inline auto reverse = pipe(detail::reverse_fn{});
static constexpr inline auto take_while = detail::advance_while_fn<true>{};
static constexpr inline auto drop_while = detail::advance_while_fn<false>{};

}  // namespace core

}  // namespace ferrugo
