#pragma once

#include <string_view>

namespace ferrugo
{
namespace core
{
template <char... Ch>
struct static_string
{
    static constexpr std::size_t size = sizeof...(Ch);
    static constexpr std::array<char, size> value = { Ch... };

    operator std::string_view() const
    {
        return std::string_view{ value.data(), value.size() };
    }

    friend std::ostream& operator<<(std::ostream& os, const static_string& item)
    {
        return os << static_cast<std::string_view>(item);
    }
};
}  // namespace core
}  // namespace ferrugo