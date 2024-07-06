#pragma once

#include <functional>
#include <sstream>
#include <string_view>

namespace ferrugo
{
namespace core
{

namespace detail
{
struct str_fn
{
    template <class... Args>
    std::string operator()(const Args&... args) const
    {
        std::stringstream ss;
        (ss << ... << args);
        return std::move(ss).str();
    }
};

struct delimit_fn
{
    template <class Iter>
    struct impl
    {
        Iter m_begin;
        Iter m_end;
        std::string_view m_separator;

        friend std::ostream& operator<<(std::ostream& os, const impl& item)
        {
            if (item.m_begin == item.m_end)
            {
                return os;
            }

            auto b = item.m_begin;
            os << *b++;
            for (; b != item.m_end; ++b)
            {
                os << item.m_separator << *b;
            }
            return os;
        }
    };

    template <class Range>
    auto operator()(Range&& range, std::string_view separator = {}) const -> impl<decltype(std::begin(range))>
    {
        return { std::begin(range), std::end(range), separator };
    }
};

}  // namespace detail

struct ostream_iterator
{
    using iterator_category = std::output_iterator_tag;
    using reference = void;
    using pointer = void;
    using value_type = void;
    using difference_type = void;

    std::ostream* os;
    std::string_view separator;

    ostream_iterator(std::ostream& os, std::string_view separator = {}) : os{ &os }, separator{ separator }
    {
    }

    ostream_iterator& operator*()
    {
        return *this;
    }

    ostream_iterator& operator++()
    {
        return *this;
    }

    ostream_iterator& operator++(int)
    {
        return *this;
    }

    template <class T>
    ostream_iterator& operator=(const T& item)
    {
        *os << item << separator;
        return *this;
    }
};

struct ostream_applier : public std::function<void(std::ostream&)>
{
    using base_type = std::function<void(std::ostream&)>;
    using base_type::base_type;

    friend std::ostream& operator<<(std::ostream& os, const ostream_applier& item)
    {
        item(os);
        return os;
    }
};

static constexpr inline auto delimit = detail::delimit_fn{};
static constexpr inline auto str = detail::str_fn{};

}  // namespace core
}  // namespace ferrugo
