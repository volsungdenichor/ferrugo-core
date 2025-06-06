#pragma once

#include <cstdint>
#include <cuchar>
#include <ferrugo/core/zx.hpp>
#include <iterator>

namespace ferrugo
{
namespace core
{

struct glyph
{
    char32_t m_data;

    friend std::ostream& operator<<(std::ostream& os, const glyph& item)
    {
        std::array<char, 4> data;
        const zx::span<char> v = std::invoke(
            [&]() -> zx::span<char>
            {
                auto state = std::mbstate_t{};
                const std::uint8_t size = std::c32rtomb(data.data(), item.m_data, &state);
                if (size == std::size_t(-1))
                {
                    throw std::runtime_error{ "u32_to_mb: error in conversion" };
                }
                return zx::span<char>{ data.data(), data.data() + size };
            });
        std::copy(v.begin(), v.end(), std::ostream_iterator<char>{ os });
        return os;
    }

    static auto read(std::string_view txt) -> zx::maybe<std::pair<glyph, std::string_view>>
    {
        std::setlocale(LC_ALL, "en_US.utf8");
        std::mbstate_t state{};
        char32_t c32 = {};
        std::size_t rc = std::mbrtoc32(&c32, txt.begin(), txt.size(), &state);
        if (rc == std::size_t(-3))
        {
            throw std::runtime_error{ "u32_to_mb: error in conversion" };
        }
        if (rc == std::size_t(-1))
        {
            return {};
        }
        if (rc == std::size_t(-2))
        {
            return {};
        }
        txt.remove_prefix(rc);
        return std::pair{ glyph{ c32 }, txt };
    }

    static auto to_glyphs(std::string_view text) -> zx::sequence<glyph>
    {
        return zx::sequence<glyph>{ [=]() mutable -> zx::iteration_result_t<glyph>
                                    {
                                        if (auto n = read(text))
                                        {
                                            const auto [ch, remainder] = *n;
                                            text = remainder;
                                            return ch;
                                        }
                                        return {};
                                    } };
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

}  // namespace core
}  // namespace ferrugo
