#pragma once

#include <cstdint>
#include <cuchar>
#include <ferrugo/core/iterator_range.hpp>
#include <functional>
#include <vector>

namespace ferrugo
{
namespace core
{

inline void decode(ferrugo::core::span<char> txt, const std::function<void(std::size_t n, char32_t)>& func)
{
    std::setlocale(LC_ALL, "en_US.utf8");
    char32_t c32 = {};
    const char* ptr = txt.begin();
    const char* end = txt.end();
    std::mbstate_t state{};
    std::size_t n = 0;
    while (std::size_t rc = std::mbrtoc32(&c32, ptr, end - ptr, &state))
    {
        if (rc == std::size_t(-3))
        {
            throw std::runtime_error{ "u32_to_mb: error in conversion" };
        }
        if (rc == std::size_t(-1))
        {
            break;
        }
        if (rc == std::size_t(-2))
        {
            break;
        }
        func(n, c32);
        ptr += rc;
        n += 1;
    }
}

inline auto encode(char32_t ch) -> std::pair<std::uint8_t, std::array<char, 4>>
{
    std::array<char, 4> data;
    auto state = std::mbstate_t{};
    const std::uint8_t size = std::c32rtomb(data.data(), ch, &state);
    if (size == std::size_t(-1))
    {
        throw std::runtime_error{ "u32_to_mb: error in conversion" };
    }
    return { size, std::move(data) };
}

struct glyph
{
    char32_t m_data;

    glyph() = default;

    explicit glyph(char32_t data) : m_data(data)
    {
    }

    explicit glyph(std::string_view txt)
    {
        decode(
            txt,
            [&](std::size_t n, char32_t c)
            {
                if (n > 0)
                {
                    throw std::runtime_error{ "too many characters to create a single glyph" };
                }
                m_data = c;
            });
    }

    glyph(char ch) : glyph(std::string_view{ &ch, 1 })
    {
    }

    glyph(const glyph&) = default;
    glyph(glyph&&) noexcept = default;

    glyph& operator=(glyph other)
    {
        std::swap(m_data, other.m_data);
        return *this;
    }

    friend std::ostream& operator<<(std::ostream& os, const glyph& item)
    {
        const auto [size, data] = encode(item.m_data);
        std::copy(data.data(), data.data() + size, std::ostream_iterator<char>{ os });
        return os;
    }

    friend bool operator==(const glyph& lhs, const glyph& rhs)
    {
        return lhs.m_data == rhs.m_data;
    }

    friend bool operator!=(const glyph& lhs, const glyph& rhs)
    {
        return !(lhs == rhs);
    }

    friend bool operator<(const glyph& lhs, const glyph& rhs)
    {
        return lhs.m_data < rhs.m_data;
    }

    friend bool operator>(const glyph& lhs, const glyph& rhs)
    {
        return rhs < lhs;
    }

    friend bool operator<=(const glyph& lhs, const glyph& rhs)
    {
        return !(lhs > rhs);
    }

    friend bool operator>=(const glyph& lhs, const glyph& rhs)
    {
        return !(lhs < rhs);
    }
};

struct string_view : public ferrugo::core::span<glyph>
{
    using base_t = ferrugo::core::span<glyph>;

    using base_t::base_t;

    friend std::ostream& operator<<(std::ostream& os, const string_view& item)
    {
        std::copy(item.begin(), item.end(), std::ostream_iterator<glyph>{ os });
        return os;
    }
};

struct string : public std::vector<glyph>
{
    using base_t = std::vector<glyph>;

    using base_t::empty;
    using base_t::size;
    using base_t::operator[];
    using base_t::at;
    using base_t::begin;
    using base_t::cbegin;
    using base_t::cend;
    using base_t::end;
    using base_t::push_back;
    using base_t::rbegin;
    using base_t::rend;

    string() = default;

    string(std::string_view txt)
    {
        decode(txt, [&](std::size_t, char32_t c) { this->push_back(glyph{ c }); });
    }

    template <class Iter>
    string(Iter b, Iter e) : base_t(b, e)
    {
    }

    operator string_view() const
    {
        return string_view{ this->data(), this->data() + this->size() };
    }

    string(std::size_t n, glyph g)
    {
        this->resize(n);
        std::fill(this->begin(), this->end(), g);
    }

    friend std::ostream& operator<<(std::ostream& os, const string& item)
    {
        std::copy(std::begin(item), std::end(item), std::ostream_iterator<glyph>{ os });
        return os;
    }

    friend string& operator+=(string& lhs, const string& rhs)
    {
        lhs.insert(lhs.end(), rhs.begin(), rhs.end());
        return lhs;
    }

    friend string operator+(string lhs, const string& rhs)
    {
        return lhs += rhs;
    }

    friend string& operator+=(string& lhs, const glyph& rhs)
    {
        lhs.push_back(rhs);
        return lhs;
    }

    friend string operator+(string lhs, const glyph& rhs)
    {
        return lhs += rhs;
    }

    friend string operator+(const glyph& lhs, const string& rhs)
    {
        return string(1, lhs) + rhs;
    }
};

}  // namespace core
}  // namespace ferrugo
