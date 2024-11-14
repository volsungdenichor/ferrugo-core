#pragma once

#include <string>
#include <string_view>

namespace ferrugo
{
namespace core
{

template <char... Ch>
struct str_t
{
    static std::string_view value()
    {
        static const auto instance = std::string{ Ch... };
        return instance;
    }

    operator std::string_view() const
    {
        return value();
    }

    friend std::ostream& operator<<(std::ostream& os, const str_t)
    {
        return os << value();
    }
};

template <class... Args>
struct concat_result;

template <class... Args>
using concat_result_t = typename concat_result<Args...>::type;

template <class T>
struct concat_result<T>
{
    using type = T;
};

template <char... L, char... R>
struct concat_result<str_t<L...>, str_t<R...>>
{
    using type = str_t<L..., R...>;
};

template <class A, class B, class... Tail>
struct concat_result<A, B, Tail...>
{
    using type = concat_result_t<concat_result_t<A, B>, concat_result_t<Tail...>>;
};

template <class T>
struct trim_result;

template <class T>
using trim_result_t = typename trim_result<T>::type;

template <char... Ch>
struct trim_result<str_t<Ch...>>
{
    using type = str_t<Ch...>;
};

template <char L, char... R>
struct trim_result<str_t<L, R...>>
{
    using type = concat_result_t<str_t<L>, trim_result_t<str_t<R...>>>;
};

template <char... Tail>
struct trim_result<str_t<'\0', Tail...>>
{
    using type = str_t<>;
};

}  // namespace core
}  // namespace ferrugo

#define FERRUGO_IMPL_GET_STR_1(str, i) ((i) + 1 < sizeof(str) ? str[(i)] : '\0')

#define FERRUGO_IMPL_GET_STR_4(str, i)                                                                          \
    FERRUGO_IMPL_GET_STR_1(str, i + 0), FERRUGO_IMPL_GET_STR_1(str, i + 1), FERRUGO_IMPL_GET_STR_1(str, i + 2), \
        FERRUGO_IMPL_GET_STR_1(str, i + 3)

#define FERRUGO_IMPL_GET_STR_16(str, i)                                                                         \
    FERRUGO_IMPL_GET_STR_4(str, i + 0), FERRUGO_IMPL_GET_STR_4(str, i + 4), FERRUGO_IMPL_GET_STR_4(str, i + 8), \
        FERRUGO_IMPL_GET_STR_4(str, i + 12)

#define FERRUGO_GET_SS_64(str, i)                                                                                    \
    FERRUGO_IMPL_GET_STR_16(str, i + 0), FERRUGO_IMPL_GET_STR_16(str, i + 16), FERRUGO_IMPL_GET_STR_16(str, i + 32), \
        FERRUGO_IMPL_GET_STR_16(str, i + 48)

#define FERRUGO_GET_STR(str) FERRUGO_GET_SS_64(str, 0)

#define FERRUGO_STR_T(str) ::ferrugo::core::trim_result_t<::ferrugo::core::str_t<FERRUGO_GET_STR(str)>>
