#pragma once

#include <ferrugo/core/maybe.hpp>
#include <ferrugo/core/type_traits.hpp>
#include <functional>

namespace ferrugo
{
namespace core
{

template <class T>
using iteration_result_t = maybe<T>;

template <class T>
using next_function_t = std::function<iteration_result_t<T>()>;

template <class T>
struct sequence;

template <class T>
struct inspect_mixin
{
    template <class Func>
    struct next_function
    {
        Func m_func;
        next_function_t<T> m_next;

        auto operator()() const -> iteration_result_t<T>
        {
            iteration_result_t<T> next = m_next();
            if (next)
            {
                std::invoke(m_func, *next);
            }
            return next;
        }
    };

    template <class Func>
    auto inspect(Func&& func) const& -> sequence<T>
    {
        return sequence<T>{ next_function<std::decay_t<Func>>{
            std::forward<Func>(func), static_cast<const sequence<T>&>(*this).get_next_function() } };
    }

    template <class Func>
    auto inspect(Func&& func) && -> sequence<T>
    {
        return sequence<T>{ next_function<std::decay_t<Func>>{ std::forward<Func>(func),
                                                               static_cast<sequence<T>&&>(*this).get_next_function() } };
    }
};

template <class T>
struct inspect_indexed_mixin
{
    template <class Func>
    struct next_function
    {
        Func m_func;
        next_function_t<T> m_next;
        mutable std::ptrdiff_t m_index = 0;

        auto operator()() const -> iteration_result_t<T>
        {
            iteration_result_t<T> next = m_next();
            if (next)
            {
                std::invoke(m_func, m_index++, *next);
            }
            return next;
        }
    };

    template <class Func>
    auto inspect_indexed(Func&& func) const& -> sequence<T>
    {
        return sequence<T>{ next_function<std::decay_t<Func>>{
            std::forward<Func>(func), static_cast<const sequence<T>&>(*this).get_next_function() } };
    }

    template <class Func>
    auto inspect_indexed(Func&& func) && -> sequence<T>
    {
        return sequence<T>{ next_function<std::decay_t<Func>>{ std::forward<Func>(func),
                                                               static_cast<sequence<T>&&>(*this).get_next_function() } };
    }
};

template <class T>
struct transform_mixin
{
    template <class Func, class Out>
    struct next_function
    {
        Func m_func;
        next_function_t<T> m_next;

        auto operator()() const -> iteration_result_t<Out>
        {
            iteration_result_t<T> next = m_next();
            if (next)
            {
                return std::invoke(m_func, *std::move(next));
            }
            return {};
        }
    };

    template <class Func, class Res = std::invoke_result_t<Func, T>>
    auto transform(Func&& func) const& -> sequence<Res>
    {
        return sequence<Res>{ next_function<std::decay_t<Func>, Res>{
            std::forward<Func>(func), static_cast<const sequence<T>&>(*this).get_next_function() } };
    }

    template <class Func, class Res = std::invoke_result_t<Func, T>>
    auto transform(Func&& func) && -> sequence<Res>
    {
        return sequence<Res>{ next_function<std::decay_t<Func>, Res>{
            std::forward<Func>(func), static_cast<sequence<T>&&>(*this).get_next_function() } };
    }
};

template <class T>
struct transform_indexed_mixin
{
    template <class Func, class Out>
    struct next_function
    {
        Func m_func;
        next_function_t<T> m_next;
        mutable std::ptrdiff_t m_index = 0;

        auto operator()() const -> iteration_result_t<Out>
        {
            iteration_result_t<T> next = m_next();
            if (next)
            {
                return std::invoke(m_func, m_index++, *std::move(next));
            }
            return {};
        }
    };

    template <class Func, class Res = std::invoke_result_t<Func, std::ptrdiff_t, T>>
    auto transform_indexed(Func&& func) const& -> sequence<Res>
    {
        return sequence<Res>{ next_function<std::decay_t<Func>, Res>{
            std::forward<Func>(func), static_cast<const sequence<T>&>(*this).get_next_function() } };
    }

    template <class Func, class Res = std::invoke_result_t<Func, std::ptrdiff_t, T>>
    auto transform_indexed(Func&& func) && -> sequence<Res>
    {
        return sequence<Res>{ next_function<std::decay_t<Func>, Res>{
            std::forward<Func>(func), static_cast<sequence<T>&&>(*this).get_next_function() } };
    }
};

template <class T>
struct transform_maybe_mixin
{
    template <class Func, class Out>
    struct next_function
    {
        Func m_func;
        next_function_t<T> m_next;

        auto operator()() const -> iteration_result_t<Out>
        {
            while (true)
            {
                iteration_result_t<T> res = m_next();
                if (!res)
                {
                    break;
                }

                iteration_result_t<Out> r = std::invoke(m_func, *std::move(res));
                if (r)
                {
                    return r;
                }
            }
            return {};
        }
    };

    template <class Func, class Res = maybe_underlying_type_t<std::invoke_result_t<Func, T>>>
    auto transform_maybe(Func&& func) const& -> sequence<Res>
    {
        return sequence<Res>{ next_function<std::decay_t<Func>, Res>{
            std::forward<Func>(func), static_cast<const sequence<T>&>(*this).get_next_function() } };
    }

    template <class Func, class Res = maybe_underlying_type_t<std::invoke_result_t<Func, T>>>
    auto transform_maybe(Func&& func) && -> sequence<Res>
    {
        return sequence<Res>{ next_function<std::decay_t<Func>, Res>{
            std::forward<Func>(func), static_cast<sequence<T>&&>(*this).get_next_function() } };
    }
};

template <class T>
struct transform_maybe_indexed_mixin
{
    template <class Func, class Out>
    struct next_function
    {
        Func m_func;
        next_function_t<T> m_next;
        mutable std::ptrdiff_t m_index;

        auto operator()() const -> iteration_result_t<Out>
        {
            while (true)
            {
                iteration_result_t<T> res = m_next();
                if (!res)
                {
                    break;
                }

                iteration_result_t<Out> r = std::invoke(m_func, m_index++, *std::move(res));
                if (r)
                {
                    return r;
                }
            }
            return {};
        }
    };

    template <class Func, class Res = maybe_underlying_type_t<std::invoke_result_t<Func, std::ptrdiff_t, T>>>
    auto transform_maybe_indexed(Func&& func) const& -> sequence<Res>
    {
        return sequence<Res>{ next_function<std::decay_t<Func>, Res>{
            std::forward<Func>(func), static_cast<const sequence<T>&>(*this).get_next_function() } };
    }

    template <class Func, class Res = maybe_underlying_type_t<std::invoke_result_t<Func, std::ptrdiff_t, T>>>
    auto transform_maybe_indexed(Func&& func) && -> sequence<Res>
    {
        return sequence<Res>{ next_function<std::decay_t<Func>, Res>{
            std::forward<Func>(func), static_cast<sequence<T>&&>(*this).get_next_function() } };
    }
};

template <class T>
struct filter_mixin
{
    template <class Pred>
    struct next_function
    {
        Pred m_pred;
        next_function_t<T> m_next;

        auto operator()() const -> iteration_result_t<T>
        {
            while (true)
            {
                iteration_result_t<T> res = m_next();
                if (!res)
                {
                    break;
                }

                if (std::invoke(m_pred, *res))
                {
                    return res;
                }
            }
            return {};
        }
    };

    template <class Pred>
    auto filter(Pred&& pred) const& -> sequence<T>
    {
        return sequence<T>{ next_function<std::decay_t<Pred>>{
            std::forward<Pred>(pred), static_cast<const sequence<T>&>(*this).get_next_function() } };
    }

    template <class Pred>
    auto filter(Pred&& pred) && -> sequence<T>
    {
        return sequence<T>{ next_function<std::decay_t<Pred>>{ std::forward<Pred>(pred),
                                                               static_cast<sequence<T>&&>(*this).get_next_function() } };
    }
};

template <class T>
struct filter_indexed_mixin
{
    template <class Pred>
    struct next_function
    {
        Pred m_pred;
        next_function_t<T> m_next;
        mutable std::ptrdiff_t m_index = 0;

        auto operator()() const -> iteration_result_t<T>
        {
            while (true)
            {
                iteration_result_t<T> res = m_next();
                if (!res)
                {
                    break;
                }

                if (std::invoke(m_pred, m_index++, *res))
                {
                    return res;
                }
            }
            return {};
        }
    };

    template <class Pred>
    auto filter_indexed(Pred&& pred) const& -> sequence<T>
    {
        return sequence<T>{ next_function<std::decay_t<Pred>>{
            std::forward<Pred>(pred), static_cast<const sequence<T>&>(*this).get_next_function() } };
    }

    template <class Pred>
    auto filter_indexed(Pred&& pred) && -> sequence<T>
    {
        return sequence<T>{ next_function<std::decay_t<Pred>>{ std::forward<Pred>(pred),
                                                               static_cast<sequence<T>&&>(*this).get_next_function() } };
    }
};

template <class T>
struct drop_while_mixin
{
    template <class Pred>
    struct next_function
    {
        Pred m_pred;
        next_function_t<T> m_next;
        mutable bool m_init = true;

        auto operator()() const -> iteration_result_t<T>
        {
            if (m_init)
            {
                while (true)
                {
                    iteration_result_t<T> res = m_next();
                    if (!res)
                    {
                        return {};
                    }
                    if (!std::invoke(m_pred, *res))
                    {
                        m_init = false;
                        return res;
                    }
                }
            }
            return m_next();
        }
    };

    template <class Pred>
    auto drop_while(Pred&& pred) const& -> sequence<T>
    {
        return sequence<T>{ next_function<std::decay_t<Pred>>{
            std::forward<Pred>(pred), static_cast<const sequence<T>&>(*this).get_next_function() } };
    }

    template <class Pred>
    auto drop_while(Pred&& pred) && -> sequence<T>
    {
        return sequence<T>{ next_function<std::decay_t<Pred>>{ std::forward<Pred>(pred),
                                                               static_cast<sequence<T>&&>(*this).get_next_function() } };
    }
};

template <class T>
struct drop_while_indexed_mixin
{
    template <class Pred>
    struct next_function
    {
        Pred m_pred;
        next_function_t<T> m_next;
        mutable bool m_init = true;
        mutable std::ptrdiff_t m_index = 0;

        auto operator()() const -> iteration_result_t<T>
        {
            if (m_init)
            {
                while (true)
                {
                    iteration_result_t<T> res = m_next();
                    if (!res)
                    {
                        return {};
                    }
                    if (!std::invoke(m_pred, m_index++, *res))
                    {
                        m_init = false;
                        return res;
                    }
                }
            }
            return m_next();
        }
    };

    template <class Pred>
    auto drop_while_indexed(Pred&& pred) const& -> sequence<T>
    {
        return sequence<T>{ next_function<std::decay_t<Pred>>{
            std::forward<Pred>(pred), static_cast<const sequence<T>&>(*this).get_next_function() } };
    }

    template <class Pred>
    auto drop_while_indexed(Pred&& pred) && -> sequence<T>
    {
        return sequence<T>{ next_function<std::decay_t<Pred>>{ std::forward<Pred>(pred),
                                                               static_cast<sequence<T>&&>(*this).get_next_function() } };
    }
};

template <class T>
struct take_while_mixin
{
    template <class Pred>
    struct next_function
    {
        Pred m_pred;
        next_function_t<T> m_next;

        auto operator()() const -> iteration_result_t<T>
        {
            iteration_result_t<T> res = m_next();
            if (!(res && std::invoke(m_pred, *res)))
            {
                return {};
            }
            return res;
        }
    };

    template <class Pred>
    auto take_while(Pred&& pred) const& -> sequence<T>
    {
        return sequence<T>{ next_function<std::decay_t<Pred>>{
            std::forward<Pred>(pred), static_cast<const sequence<T>&>(*this).get_next_function() } };
    }

    template <class Pred>
    auto take_while(Pred&& pred) && -> sequence<T>
    {
        return sequence<T>{ next_function<std::decay_t<Pred>>{ std::forward<Pred>(pred),
                                                               static_cast<sequence<T>&&>(*this).get_next_function() } };
    }
};

template <class T>
struct take_while_indexed_mixin
{
    template <class Pred>
    struct next_function
    {
        Pred m_pred;
        next_function_t<T> m_next;
        mutable std::ptrdiff_t m_index = 0;

        auto operator()() const -> iteration_result_t<T>
        {
            iteration_result_t<T> res = m_next();
            if (!(res && std::invoke(m_pred, m_index++, *res)))
            {
                return {};
            }
            return res;
        }
    };

    template <class Pred>
    auto take_while_indexed(Pred&& pred) const& -> sequence<T>
    {
        return sequence<T>{ next_function<std::decay_t<Pred>>{
            std::forward<Pred>(pred), static_cast<const sequence<T>&>(*this).get_next_function() } };
    }

    template <class Pred>
    auto take_while_indexed(Pred&& pred) && -> sequence<T>
    {
        return sequence<T>{ next_function<std::decay_t<Pred>>{ std::forward<Pred>(pred),
                                                               static_cast<sequence<T>&&>(*this).get_next_function() } };
    }
};

template <class T>
struct drop_mixin
{
    struct next_function
    {
        mutable std::ptrdiff_t m_count;
        next_function_t<T> m_next;
        mutable bool m_init = false;

        auto operator()() const -> iteration_result_t<T>
        {
            if (!m_init)
            {
                while (m_count > 0)
                {
                    --m_count;
                    m_next();
                }
                m_init = true;
            }
            return m_next();
        }
    };

    auto drop(std::ptrdiff_t n) const& -> sequence<T>
    {
        return sequence<T>{ next_function{ n, static_cast<const sequence<T>&>(*this).get_next_function() } };
    }

    auto drop(std::ptrdiff_t n) && -> sequence<T>
    {
        return sequence<T>{ next_function{ n, static_cast<sequence<T>&&>(*this).get_next_function() } };
    }
};

template <class T>
struct take_mixin
{
    struct next_function
    {
        mutable std::ptrdiff_t m_count;
        next_function_t<T> m_next;

        auto operator()() const -> iteration_result_t<T>
        {
            if (m_count == 0)
            {
                return {};
            }
            --m_count;
            return m_next();
        }
    };

    auto take(std::ptrdiff_t n) const& -> sequence<T>
    {
        return sequence<T>{ next_function{ n, static_cast<const sequence<T>&>(*this).get_next_function() } };
    }

    auto take(std::ptrdiff_t n) && -> sequence<T>
    {
        return sequence<T>{ next_function{ n, static_cast<sequence<T>&&>(*this).get_next_function() } };
    }
};

template <class T>
struct step_mixin
{
    struct next_function
    {
        mutable std::ptrdiff_t m_count;
        next_function_t<T> m_next;
        mutable std::ptrdiff_t m_index = 0;

        auto operator()() const -> iteration_result_t<T>
        {
            while (true)
            {
                iteration_result_t<T> res = m_next();
                if (!res)
                {
                    break;
                }

                if (m_index++ % m_count == 0)
                {
                    return res;
                }
            }
            return {};
        }
    };

    auto step(std::ptrdiff_t n) const& -> sequence<T>
    {
        return sequence<T>{ next_function{ n, static_cast<const sequence<T>&>(*this).get_next_function() } };
    }

    auto step(std::ptrdiff_t n) && -> sequence<T>
    {
        return sequence<T>{ next_function{ n, static_cast<sequence<T>&&>(*this).get_next_function() } };
    }
};

template <class T>
struct empty_sequence
{
    auto operator()() const -> iteration_result_t<T>
    {
        return {};
    }
};

template <class To, class From>
struct cast_sequence
{
    next_function_t<From> m_from;

    auto operator()() const -> iteration_result_t<To>
    {
        iteration_result_t<From> value = m_from();
        if (value)
        {
            return static_cast<To>(*value);
        }
        return {};
    }
};

template <class T>
struct pointer_proxy
{
    T item;

    T* operator->()
    {
        return std::addressof(item);
    }
};

template <class T>
struct sequence_iterator
{
    using next_function_type = next_function_t<T>;
    using reference = T;
    using difference_type = std::ptrdiff_t;
    using value_type = std::decay_t<reference>;
    using pointer
        = std::conditional_t<std::is_reference_v<reference>, std::add_pointer_t<reference>, pointer_proxy<reference>>;
    using iterator_category = std::forward_iterator_tag;

    next_function_type m_next_fn;
    iteration_result_t<reference> m_current;
    difference_type m_index;

    sequence_iterator() : m_next_fn{}, m_current{}, m_index{ std::numeric_limits<difference_type>::max() }
    {
    }

    sequence_iterator(const next_function_type& next_fn) : m_next_fn{ next_fn }, m_current{ m_next_fn() }, m_index{ 0 }
    {
    }

    sequence_iterator(const sequence_iterator&) = default;
    sequence_iterator(sequence_iterator&&) noexcept = default;

    sequence_iterator& operator=(sequence_iterator other)
    {
        std::swap(m_next_fn, m_next_fn);
        return *this;
    }

    reference operator*() const
    {
        return *m_current;
    }

    pointer operator->() const
    {
        if constexpr (std::is_reference_v<reference>)
        {
            return std::addressof(**this);
        }
        else
        {
            return pointer{ **this };
        }
    }

    sequence_iterator& operator++()
    {
        m_current = m_next_fn();
        ++m_index;
        return *this;
    }

    sequence_iterator operator++(int)
    {
        sequence_iterator temp{ *this };
        ++(*this);
        return temp;
    }

    friend bool operator==(const sequence_iterator& lhs, const sequence_iterator& rhs)
    {
        return (!lhs.m_current && !rhs.m_current) || (lhs.m_current && rhs.m_current && lhs.m_index == rhs.m_index);
    }

    friend bool operator!=(const sequence_iterator& lhs, const sequence_iterator& rhs)
    {
        return !(lhs == rhs);
    }
};

template <class T>
struct sequence : inspect_mixin<T>,
                  inspect_indexed_mixin<T>,
                  transform_mixin<T>,
                  transform_indexed_mixin<T>,
                  filter_mixin<T>,
                  filter_indexed_mixin<T>,
                  transform_maybe_mixin<T>,
                  transform_maybe_indexed_mixin<T>,
                  drop_while_mixin<T>,
                  drop_while_indexed_mixin<T>,
                  take_while_mixin<T>,
                  take_while_indexed_mixin<T>,
                  drop_mixin<T>,
                  take_mixin<T>,
                  step_mixin<T>
{
    using iterator = sequence_iterator<T>;
    using next_function_type = typename iterator::next_function_type;
    using value_type = typename iterator::value_type;
    using reference = typename iterator::reference;
    using pointer = typename iterator::pointer;

    next_function_type m_next_fn;

    explicit sequence(next_function_type next_fn) : m_next_fn(std::move(next_fn))
    {
    }

    template <class U, std::enable_if_t<std::is_constructible_v<T, U>, int> = 0>
    sequence(const sequence<U>& other) : sequence(cast_sequence<T, U>{ other.get_next() })
    {
    }

    sequence() : sequence(empty_sequence<T>{})
    {
    }

    template <class Container, std::enable_if_t<std::is_constructible_v<Container, iterator, iterator>, int> = 0>
    operator Container() const
    {
        return Container{ begin(), end() };
    }

    iterator begin() const
    {
        return iterator{ m_next_fn };
    }

    iterator end() const
    {
        return iterator{};
    }

    const next_function_type& get_next_function() const&
    {
        return m_next_fn;
    }

    next_function_type&& get_next_function() &&
    {
        return std::move(m_next_fn);
    }
};

namespace detail
{

struct iota_fn
{
    template <class In>
    struct next_function
    {
        mutable In m_current;
        auto operator()() const -> maybe<In>
        {
            return m_current++;
        }
    };

    template <class T>
    auto operator()(T init) const -> iteration_result_t<T>
    {
        return sequence<T>{ next_function<T>{ init } };
    }
};

struct range_fn
{
    template <class In>
    struct next_function
    {
        mutable In m_current;
        In m_upper;

        auto operator()() const -> iteration_result_t<In>
        {
            if (m_current >= m_upper)
            {
                return {};
            }
            return m_current++;
        }
    };

    template <class T>
    auto operator()(T lower, T upper) const -> sequence<T>
    {
        return sequence<T>{ next_function<T>{ lower, upper } };
    }

    template <class T>
    auto operator()(T upper) const -> sequence<T>
    {
        return (*this)(T{}, upper);
    }
};

struct unfold_fn
{
    template <class Func, class S, class Out>
    struct next_function
    {
        Func m_func;
        mutable S m_state;

        auto operator()() const -> iteration_result_t<Out>
        {
            auto res = m_func(m_state);
            if (!res)
            {
                return {};
            }
            auto&& [value, new_state] = *std::move(res);
            m_state = std::move(new_state);
            return value;
        }
    };

    template <
        class S,
        class Func,
        class OptRes = std::invoke_result_t<Func, const S&>,
        class Res = maybe_underlying_type_t<OptRes>,
        class Out = std::tuple_element_t<0, Res>>
    auto operator()(S state, Func&& func) const -> sequence<Out>
    {
        return sequence<Out>{ next_function<std::decay_t<Func>, S, Out>{ std::forward<Func>(func), std::move(state) } };
    }
};

struct view_fn
{
    template <class Iter, class Out>
    struct next_function
    {
        mutable Iter m_iter;
        Iter m_end;

        next_function(Iter begin, Iter end) : m_iter(begin), m_end(end)
        {
        }

        auto operator()() const -> iteration_result_t<Out>
        {
            if (m_iter == m_end)
            {
                return {};
            }
            return *m_iter++;
        }
    };

    template <class Range, class Out = range_reference_t<Range>>
    auto operator()(Range& range) const -> sequence<Out>
    {
        return sequence<Out>{ next_function<iterator_t<Range>, Out>{ std::begin(range), std::end(range) } };
    }

    template <class T>
    auto operator()(const sequence<T>& seq) const -> sequence<T>
    {
        return seq;
    }
};

}  // namespace detail

static constexpr inline auto iota = detail::iota_fn{};
static constexpr inline auto range = detail::range_fn{};
static constexpr inline auto unfold = detail::unfold_fn{};
static constexpr inline auto view = detail::view_fn{};

}  // namespace core
}  // namespace ferrugo
