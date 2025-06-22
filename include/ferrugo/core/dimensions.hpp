#pragma once

#include <array>
#include <functional>

namespace ferrugo
{
namespace core
{

namespace detail
{

template <int... Dims>
struct quantity_type
{
    static constexpr std::array<int, sizeof...(Dims)> dimensions = { Dims... };
};

template <class T>
struct is_quantity : std::false_type
{
};

template <int... Dims>
struct is_quantity<quantity_type<Dims...>> : std::true_type
{
};

template <class Quant>
struct get_size;

template <int... Dims>
struct get_size<quantity_type<Dims...>>
{
    static constexpr std::size_t value = sizeof...(Dims);
};

template <std::size_t D, class Q>
struct get
{
    static constexpr int value = D < Q::dimensions.size() ? Q::dimensions[D] : 0;
};

template <class L, class R, class>
struct mul_result_base;

template <class L, class R, class>
struct div_result_base;

template <class Q, int N, int D, class>
struct pow_result_base;

template <class L, class R, std::size_t... I>
struct mul_result_base<L, R, std::index_sequence<I...>>
{
    using type = quantity_type<(get<I, L>::value + get<I, R>::value)...>;
};

template <class L, class R, std::size_t... I>
struct div_result_base<L, R, std::index_sequence<I...>>
{
    using type = quantity_type<(get<I, L>::value - get<I, R>::value)...>;
};

template <class Q, int N, int D, std::size_t... I>
struct pow_result_base<Q, N, D, std::index_sequence<I...>>
{
    using type = quantity_type<(get<I, Q>::value * N / D)...>;
};

template <class L, class R>
struct mul_result : mul_result_base<L, R, std::make_index_sequence<std::max(get_size<L>::value, get_size<R>::value)>>
{
};

template <class L, class R>
struct div_result : div_result_base<L, R, std::make_index_sequence<std::max(get_size<L>::value, get_size<R>::value)>>
{
};

template <class Q, int N, int D>
struct pow_result : pow_result_base<Q, N, D, std::make_index_sequence<get_size<Q>::value>>
{
};

}  // namespace detail

using detail::quantity_type;

template <class L, class R>
using mul_result_t = typename detail::mul_result<L, R>::type;

template <class L, class R>
using div_result_t = typename detail::div_result<L, R>::type;

template <class Q, int Exp>
using pow_t = typename detail::pow_result<Q, Exp, 1>::type;

template <class Q>
using inv_t = pow_t<Q, -1>;

template <class Q>
using sqr_root_t = typename detail::pow_result<Q, 1, 2>::type;

template <class T, class Q>
struct quant_value
{
    using value_type = T;
    using quantity = Q;
    value_type m_value;

    constexpr explicit quant_value(T value) : m_value(value)
    {
    }

    constexpr explicit quant_value() : quant_value(value_type{})
    {
    }

    template <class U, class = std::enable_if_t<std::is_constructible_v<value_type, U>>>
    constexpr explicit quant_value(quant_value<U, quantity> other) : quant_value(other.get())
    {
    }

    template <class U, class OtherQ>
    constexpr quant_value(quant_value<U, OtherQ>) = delete;

    constexpr value_type get() const noexcept
    {
        return m_value;
    }

    template <class Os>
    friend Os& operator<<(Os& os, const quant_value& item)
    {
        os << item.m_value;
        os << " {";
        for (auto d : quantity::dimensions)
        {
            os << " " << d;
        }
        os << " }";
        return os;
    }
};

template <class T, class Q>
constexpr auto operator+(quant_value<T, Q> item) -> quant_value<T, Q>
{
    return quant_value<T, Q>{ item.get() };
}

template <class T, class Q>
constexpr auto operator-(quant_value<T, Q> item) -> quant_value<T, Q>
{
    return quant_value<T, Q>{ -item.get() };
}

template <class L, class R, class Q, class Res = quant_value<std::invoke_result_t<std::plus<>, L, R>, Q>>
constexpr auto operator+(quant_value<L, Q> lhs, quant_value<R, Q> rhs) -> Res
{
    return Res{ lhs.get() + rhs.get() };
}

template <class L, class R, class Q, class Res = quant_value<std::invoke_result_t<std::minus<>, L, R>, Q>>
constexpr auto operator-(quant_value<L, Q> lhs, quant_value<R, Q> rhs) -> Res
{
    return Res{ lhs.get() - rhs.get() };
}

template <
    class L,
    class LQ,
    class R,
    class RQ,
    class Res = quant_value<std::invoke_result_t<std::multiplies<>, L, R>, mul_result_t<LQ, RQ>>>
constexpr auto operator*(quant_value<L, LQ> lhs, quant_value<R, RQ> rhs) -> Res
{
    return Res{ lhs.get() * rhs.get() };
}

template <class L, class R, class RQ, class Res = quant_value<std::invoke_result_t<std::multiplies<>, L, R>, RQ>>
constexpr auto operator*(L lhs, quant_value<R, RQ> rhs) -> Res
{
    return Res{ lhs * rhs.get() };
}

template <class L, class LQ, class R, class Res = quant_value<std::invoke_result_t<std::multiplies<>, L, R>, LQ>>
constexpr auto operator*(quant_value<L, LQ> lhs, R rhs) -> Res
{
    return Res{ lhs.get() * rhs };
}

template <
    class L,
    class LQ,
    class R,
    class RQ,
    class Res = quant_value<std::invoke_result_t<std::divides<>, L, R>, div_result_t<LQ, RQ>>>
constexpr auto operator/(quant_value<L, LQ> lhs, quant_value<R, RQ> rhs) -> Res
{
    return Res{ lhs.get() / rhs.get() };
}

template <class L, class LQ, class R, class Res = quant_value<std::invoke_result_t<std::divides<>, L, R>, LQ>>
constexpr auto operator/(quant_value<L, LQ> lhs, R rhs) -> Res
{
    return Res{ lhs.get() / rhs };
}

template <class L, class R, class RQ, class Res = quant_value<std::invoke_result_t<std::divides<>, L, R>, inv_t<RQ>>>
constexpr auto operator/(L lhs, quant_value<R, RQ> rhs) -> Res
{
    return Res{ lhs / rhs.get() };
}

template <class L, class R, class Q, class = std::invoke_result_t<std::plus<>, L, R>>
constexpr auto operator+=(quant_value<L, Q>& lhs, quant_value<R, Q> rhs) -> quant_value<L, Q>&
{
    return lhs = lhs + rhs;
}

template <class L, class R, class Q, class = std::invoke_result_t<std::minus<>, L, R>>
constexpr auto operator-=(quant_value<L, Q>& lhs, quant_value<R, Q> rhs) -> quant_value<L, Q>&
{
    return lhs = lhs - rhs;
}

template <class L, class R, class Q, class = std::invoke_result_t<std::multiplies<>, L, R>>
constexpr auto operator*=(quant_value<L, Q>& lhs, R rhs) -> quant_value<L, Q>&
{
    return lhs = lhs * rhs;
}

template <class L, class R, class Q, class = std::invoke_result_t<std::divides<>, L, R>>
constexpr auto operator/=(quant_value<L, Q>& lhs, R rhs) -> quant_value<L, Q>&
{
    return lhs = lhs / rhs;
}

template <class T, class Q, class = std::invoke_result_t<std::plus<>, T, T>>
constexpr auto operator++(quant_value<T, Q>& item) -> quant_value<T, Q>&
{
    return item = quant_value<T, Q>{ item.get() + T(1) };
}

template <class T, class Q, class = std::invoke_result_t<std::plus<>, T, T>>
constexpr auto operator++(quant_value<T, Q>& item, int) -> quant_value<T, Q>
{
    const quant_value<T, Q> copy = item;
    ++item;
    return copy;
}

template <class T, class Q, class = std::invoke_result_t<std::minus<>, T, T>>
constexpr auto operator--(quant_value<T, Q>& item) -> quant_value<T, Q>&
{
    return item = quant_value<T, Q>{ item.get() + T(1) };
}

template <class T, class Q, class = std::invoke_result_t<std::minus<>, T, T>>
constexpr auto operator--(quant_value<T, Q>& item, int) -> quant_value<T, Q>
{
    const quant_value<T, Q> copy = item;
    --item;
    return copy;
}

template <class L, class R, class Q, class = std::invoke_result_t<std::equal_to<>, L, R>>
constexpr auto operator==(quant_value<L, Q> lhs, quant_value<R, Q> rhs) -> bool
{
    return lhs.get() == rhs.get();
}

template <class L, class R, class Q, class = std::invoke_result_t<std::not_equal_to<>, L, R>>
constexpr auto operator!=(quant_value<L, Q> lhs, quant_value<R, Q> rhs) -> bool
{
    return lhs.get() != rhs.get();
}

template <class L, class R, class Q, class = std::invoke_result_t<std::less<>, L, R>>
constexpr auto operator<(quant_value<L, Q> lhs, quant_value<R, Q> rhs) -> bool
{
    return lhs.get() < rhs.get();
}

template <class L, class R, class Q, class = std::invoke_result_t<std::greater<>, L, R>>
constexpr auto operator>(quant_value<L, Q> lhs, quant_value<R, Q> rhs) -> bool
{
    return lhs.get() > rhs.get();
}

template <class L, class R, class Q, class = std::invoke_result_t<std::less_equal<>, L, R>>
constexpr auto operator<=(quant_value<L, Q> lhs, quant_value<R, Q> rhs) -> bool
{
    return lhs.get() <= rhs.get();
}

template <class L, class R, class Q, class = std::invoke_result_t<std::greater_equal<>, L, R>>
constexpr auto operator>=(quant_value<L, Q> lhs, quant_value<R, Q> rhs) -> bool
{
    return lhs.get() >= rhs.get();
}

namespace quantities
{

using scalar = quantity_type<0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0>;
using length = quantity_type<1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0>;
using location = length;
using mass = quantity_type<0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0>;
using time = quantity_type<0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0>;
using coords = quantity_type<0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0>;
using temperature = quantity_type<0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0>;
using angle = quantity_type<0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0>;
using current = quantity_type<0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0>;
using luminous_intensity = quantity_type<0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0>;
using solid_angle = quantity_type<0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0>;
using amount = quantity_type<0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0>;

using area = pow_t<length, 2>;
using volume = pow_t<length, 3>;

using velocity = div_result_t<length, time>;
using acceleration = div_result_t<velocity, time>;

using momentum = mul_result_t<mass, velocity>;
using force = mul_result_t<mass, acceleration>;
using energy = mul_result_t<force, length>;
using power = div_result_t<energy, time>;

using mass_flow_rate = div_result_t<mass, time>;

using linear_density = div_result_t<mass, length>;
using area_density = div_result_t<mass, area>;
using density = div_result_t<mass, volume>;
using pressure = div_result_t<force, area>;
using frequency = inv_t<time>;

using charge = mul_result_t<current, time>;
using voltage = div_result_t<power, current>;

using resistance = mul_result_t<voltage, current>;
using conductance = inv_t<resistance>;

using capacitance = div_result_t<charge, voltage>;

using magnetic_flux = mul_result_t<voltage, time>;
using magnetic_flux_density = div_result_t<magnetic_flux, area>;
using inductance = div_result_t<magnetic_flux, current>;

using luminous_flux = mul_result_t<luminous_intensity, solid_angle>;
using illuminance = div_result_t<luminous_flux, area>;

using inertia = mul_result_t<mass, area>;
using angular_velocity = div_result_t<angle, time>;
using angular_acceleration = div_result_t<angular_velocity, time>;

using angular_momentum = mul_result_t<inertia, angular_velocity>;
using torque = mul_result_t<inertia, angular_acceleration>;
using angular_energy = mul_result_t<torque, angle>;

using resolution = div_result_t<coords, length>;

}  // namespace quantities

#define FERRUGO_DEFINE_QUANTITY(NAME)                  \
                                                       \
    template <class T>                                 \
    using NAME##_t = quant_value<T, quantities::NAME>; \
                                                       \
    namespace literals                                 \
    {                                                  \
                                                       \
    auto operator"" _##NAME(double long v)             \
    {                                                  \
        return NAME##_t<double>(double(v));            \
    }                                                  \
                                                       \
    auto operator"" _##NAME(unsigned long long v)      \
    {                                                  \
        return NAME##_t<double>(double(v));            \
    }                                                  \
    }  // namespace literals

FERRUGO_DEFINE_QUANTITY(scalar)
FERRUGO_DEFINE_QUANTITY(length)
FERRUGO_DEFINE_QUANTITY(location)
FERRUGO_DEFINE_QUANTITY(mass)
FERRUGO_DEFINE_QUANTITY(time)
FERRUGO_DEFINE_QUANTITY(coords)
FERRUGO_DEFINE_QUANTITY(temperature)
FERRUGO_DEFINE_QUANTITY(angle)
FERRUGO_DEFINE_QUANTITY(current)
FERRUGO_DEFINE_QUANTITY(luminous_intensity)
FERRUGO_DEFINE_QUANTITY(solid_angle)
FERRUGO_DEFINE_QUANTITY(amount)

FERRUGO_DEFINE_QUANTITY(area)
FERRUGO_DEFINE_QUANTITY(volume)

FERRUGO_DEFINE_QUANTITY(velocity)
FERRUGO_DEFINE_QUANTITY(acceleration)

FERRUGO_DEFINE_QUANTITY(momentum)
FERRUGO_DEFINE_QUANTITY(force)
FERRUGO_DEFINE_QUANTITY(energy)
FERRUGO_DEFINE_QUANTITY(power)

FERRUGO_DEFINE_QUANTITY(mass_flow_rate)

FERRUGO_DEFINE_QUANTITY(linear_density)
FERRUGO_DEFINE_QUANTITY(area_density)
FERRUGO_DEFINE_QUANTITY(density)
FERRUGO_DEFINE_QUANTITY(pressure)
FERRUGO_DEFINE_QUANTITY(frequency)

FERRUGO_DEFINE_QUANTITY(charge)
FERRUGO_DEFINE_QUANTITY(voltage)

FERRUGO_DEFINE_QUANTITY(resistance)
FERRUGO_DEFINE_QUANTITY(conductance)

FERRUGO_DEFINE_QUANTITY(capacitance)

FERRUGO_DEFINE_QUANTITY(magnetic_flux)
FERRUGO_DEFINE_QUANTITY(magnetic_flux_density)
FERRUGO_DEFINE_QUANTITY(inductance)

FERRUGO_DEFINE_QUANTITY(luminous_flux)
FERRUGO_DEFINE_QUANTITY(illuminance)

FERRUGO_DEFINE_QUANTITY(inertia)
FERRUGO_DEFINE_QUANTITY(angular_velocity)
FERRUGO_DEFINE_QUANTITY(angular_acceleration)

FERRUGO_DEFINE_QUANTITY(angular_momentum)
FERRUGO_DEFINE_QUANTITY(torque)
FERRUGO_DEFINE_QUANTITY(angular_energy)

#undef FERRUGO_DEFINE_QUANTITY

}  // namespace core
}  // namespace ferrugo
