
#include <array>
#include <cmath>
#include <cstdint>
#include <cuchar>
#include <deque>
#include <ferrugo/core/channel.hpp>
#include <ferrugo/core/chrono.hpp>
#include <ferrugo/core/dimensions.hpp>
#include <ferrugo/core/error_handling.hpp>
#include <ferrugo/core/functional.hpp>
#include <ferrugo/core/iterator_range.hpp>
#include <ferrugo/core/ostream_utils.hpp>
#include <ferrugo/core/overloaded.hpp>
#include <ferrugo/core/rational.hpp>
#include <ferrugo/core/sequence.hpp>
#include <ferrugo/core/std_ostream.hpp>
#include <ferrugo/core/type_name.hpp>
#include <forward_list>
#include <fstream>
#include <functional>
#include <future>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <numeric>
#include <optional>
#include <set>
#include <typeindex>
#include <unordered_set>
#include <variant>
#include <vector>

template <std::size_t D>
std::array<std::size_t, D> get_stride(const std::array<std::size_t, D>& sizes)
{
    std::array<std::size_t, D> result;
    result[0] = 1;
    for (std::size_t d = 1; d < D; ++d)
    {
        result[d] = result[d - 1] * sizes[d];
    }
    std::reverse(result.begin(), result.end());
    return result;
}

template <class T, std::size_t... Sizes>
struct value_t
{
    using value_type = T;
    static constexpr std::size_t dimension = sizeof...(Sizes);
    static constexpr std::array<std::size_t, dimension> sizes = { Sizes... };
    static const inline std::array<std::size_t, dimension> strides = get_stride(sizes);
    static constexpr std::size_t volume = (Sizes * ...);

    using location_type = value_t<std::ptrdiff_t, dimension>;
    using shape_type = value_t<std::ptrdiff_t, dimension>;
    using data_type = std::array<value_type, volume>;
    data_type m_data;

    constexpr value_t() : m_data{}
    {
    }

    constexpr value_t(const value_t&) = default;
    constexpr value_t(value_t&&) noexcept = default;

    template <class... Tail>
    constexpr value_t(value_type head, Tail&&... tail) : m_data{ head, std::forward<Tail>(tail)... }
    {
        static_assert(sizeof...(tail) + 1 == volume, "all values required");
    }

    constexpr std::size_t get_offset(const location_type& loc) const
    {
        std::size_t result = 0;
        for (std::size_t d = 0; d < dimension; ++d)
        {
            result += strides[d] * loc.m_data[d];
        }
        return result;
    }

    constexpr const value_type& operator[](const location_type& loc) const
    {
        return m_data.at(get_offset(loc));
    }

    constexpr value_type& operator[](const location_type& loc)
    {
        return m_data.at(get_offset(loc));
    }

    friend std::ostream& operator<<(std::ostream& os, const value_t& item)
    {
        os << "[" << ferrugo::core::delimit(item.m_data, ", ") << "]";
        return os;
    }

    constexpr shape_type shape() const
    {
        return { Sizes... };
    }
};

template <class T>
struct is_value : std::false_type
{
};

template <class T, std::size_t... Sizes>
struct is_value<value_t<T, Sizes...>> : std::true_type
{
};

template <class To, class UnaryFunc>
constexpr void map(To& to, UnaryFunc func)
{
    std::transform(to.begin(), to.end(), to.begin(), std::ref(func));
}

template <class To, class From, class UnaryFunc>
constexpr void map(To& to, const From& from, UnaryFunc func)
{
    std::transform(from.begin(), from.end(), to.begin(), std::ref(func));
}

template <class To, class L, class R, class BinaryFunc>
constexpr void map(To& to, const L& lhs, const R& rhs, BinaryFunc func)
{
    std::transform(lhs.begin(), lhs.end(), rhs.begin(), to.begin(), std::ref(func));
}

template <class T, std::size_t... Sizes>
constexpr auto operator+(value_t<T, Sizes...> item) -> value_t<T, Sizes...>
{
    return item;
}

template <class T, std::size_t... Sizes>
constexpr auto operator-(value_t<T, Sizes...> item) -> value_t<T, Sizes...>
{
    map(item.m_data, std::negate<>{});
    return item;
}

template <
    class L,
    class R,
    std::size_t... Sizes,
    class O = std::invoke_result_t<std::plus<>, L, R>,
    class Out = value_t<O, Sizes...>>
constexpr auto operator+(value_t<L, Sizes...> lhs, value_t<R, Sizes...> rhs) -> Out
{
    Out out{};
    map(out.m_data, lhs.m_data, rhs.m_data, std::plus<>{});
    return out;
}

template <
    class L,
    class R,
    std::size_t... Sizes,
    class O = std::invoke_result_t<std::minus<>, L, R>,
    class Out = value_t<O, Sizes...>>
constexpr auto operator-(value_t<L, Sizes...> lhs, value_t<R, Sizes...> rhs) -> Out
{
    Out out{};
    map(out.m_data, lhs.m_data, rhs.m_data, std::minus<>{});
    return out;
}

template <
    class L,
    class R,
    std::size_t... Sizes,
    class = std::enable_if_t<!is_value<L>::value>,
    class O = std::invoke_result_t<std::multiplies<>, L, R>,
    class Out = value_t<O, Sizes...>>
constexpr auto operator*(L lhs, value_t<R, Sizes...> rhs) -> Out
{
    Out out{};
    map(out.m_data, rhs.m_data, std::bind(std::multiplies<>{}, lhs, std::placeholders::_1));
    return out;
}

template <
    class L,
    class R,
    std::size_t... Sizes,
    class = std::enable_if_t<!is_value<R>::value>,
    class O = std::invoke_result_t<std::multiplies<>, L, R>,
    class Out = value_t<O, Sizes...>>
constexpr auto operator*(value_t<L, Sizes...> lhs, R rhs) -> Out
{
    Out out{};
    map(out.m_data, lhs.m_data, std::bind(std::multiplies<>{}, std::placeholders::_1, rhs));
    return out;
}

template <
    class L,
    class R,
    std::size_t... Sizes,
    class = std::enable_if_t<!is_value<R>::value>,
    class O = std::invoke_result_t<std::divides<>, L, R>,
    class Out = value_t<O, Sizes...>>
constexpr auto operator/(value_t<L, Sizes...> lhs, R rhs) -> Out
{
    Out out{};
    map(out.m_data, lhs.m_data, std::bind(std::divides<>{}, std::placeholders::_1, rhs));
    return out;
}

template <class L, class R, std::size_t... Sizes, class O = std::invoke_result_t<std::plus<>, L, R>>
constexpr auto operator+=(value_t<L, Sizes...>& lhs, value_t<R, Sizes...> rhs) -> value_t<L, Sizes...>&
{
    map(lhs.m_data, lhs.m_data, rhs.m_data, std::plus<>{});
    return lhs;
}

template <class L, class R, std::size_t... Sizes, class O = std::invoke_result_t<std::minus<>, L, R>>
constexpr auto operator-=(value_t<L, Sizes...>& lhs, value_t<R, Sizes...> rhs) -> value_t<L, Sizes...>&
{
    map(lhs.m_data, lhs.m_data, rhs.m_data, std::minus<>{});
    return lhs;
}

template <
    class L,
    class R,
    std::size_t... Sizes,
    class = std::enable_if_t<!is_value<R>::value>,
    class O = std::invoke_result_t<std::multiplies<>, L, R>>
constexpr auto operator*=(value_t<L, Sizes...>& lhs, R rhs) -> value_t<L, Sizes...>&
{
    map(lhs.m_data, std::bind(std::multiplies<>{}, std::placeholders::_1, rhs));
    return lhs;
}

template <
    class L,
    class R,
    std::size_t... Sizes,
    class = std::enable_if_t<!is_value<R>::value>,
    class O = std::invoke_result_t<std::divides<>, L, R>>
constexpr auto operator/=(value_t<L, Sizes...>& lhs, R rhs) -> value_t<L, Sizes...>&
{
    map(lhs.m_data, std::bind(std::divides<>{}, std::placeholders::_1, rhs));
    return lhs;
}

template <
    class L,
    class R,
    std::size_t LR,
    std::size_t D,
    std::size_t RC,
    class O = std::invoke_result_t<std::multiplies<>, L, R>,
    class Out = value_t<O, LR, RC>>
constexpr auto operator*(value_t<L, LR, D> lhs, value_t<R, D, RC> rhs) -> Out
{
    Out out = {};

    for (std::ptrdiff_t r = 0; r < LR; ++r)
    {
        for (std::ptrdiff_t c = 0; c < RC; ++c)
        {
            O sum = {};
            for (std::ptrdiff_t d = 0; d < D; ++d)
            {
                sum += lhs[{ r, d }] * rhs[{ d, c }];
            }

            out[{ r, c }] = sum;
        }
    }

    return out;
}

template <class T, std::size_t R, std::size_t C>
using matrix = value_t<T, R, C>;

template <class T, std::size_t D>
using square_matrix = matrix<T, D, D>;

template <class T, std::size_t D>
using vector = value_t<T, D>;

template <class T>
using square_matrix_2d = square_matrix<T, 3>;

template <class T>
using square_matrix_3d = square_matrix<T, 4>;

template <class T>
using vector_2d = vector<T, 2>;

template <class T>
using vector_3d = vector<T, 3>;

int run(const std::vector<std::string_view>& args)
{
    static constexpr auto delimit = ferrugo::core::delimit;
    const auto a = matrix<int, 2, 3>{ 1, 2, 3, 4, 5, 6 };
    const auto b = matrix<int, 3, 2>{ 7, 8, 9, 10, 11, 12 };

    std::cout << (a * b) << "\n";
    std::cout << (b * a) << "\n";

    return 0;
}

int main(int argc, char* argv[])
{
    try
    {
        return run({ argv, argv + argc });
    }
    catch (const std::exception& ex)
    {
        std::cerr << ex.what() << '\n';
        return -1;
    }
}
