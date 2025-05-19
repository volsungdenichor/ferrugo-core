#pragma once

#include <chrono>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <ratio>

namespace ferrugo
{
namespace core
{

namespace units
{

using nanoseconds = std::ratio<1, 1'000'000'000>;
using microseconds = std::ratio<1, 1'000'000>;
using milliseconds = std::ratio<1, 1000>;
using seconds = std::ratio<1, 1>;
using minutes = std::ratio<60, 1>;
using hours = std::ratio<60 * 60, 1>;
using days = std::ratio<60 * 60 * 24, 1>;
using weeks = std::ratio<60 * 60 * 24 * 7, 1>;

}  // namespace units

template <class R>
struct ratio_writer
{
    friend std::ostream& operator<<(std::ostream& os, const ratio_writer)
    {
        if (R::den != 1)
        {
            return os << R::num << "/" << R::den;
        }
        else
        {
            return os << R::num;
        }
    }
};

template <char... Ch>
struct ratio_writer_base
{
    friend std::ostream& operator<<(std::ostream& os, const ratio_writer_base)
    {
        static const auto instance = std::string{ Ch... };
        return os << instance;
    }
};

template <>
struct ratio_writer<units::nanoseconds> : ratio_writer_base<'n', 's'>
{
};

template <>
struct ratio_writer<units::microseconds> : ratio_writer_base<'u', 's'>
{
};

template <>
struct ratio_writer<units::milliseconds> : ratio_writer_base<'m', 's'>
{
};

template <>
struct ratio_writer<units::seconds> : ratio_writer_base<'s'>
{
};

template <>
struct ratio_writer<units::minutes> : ratio_writer_base<'m'>
{
};

template <>
struct ratio_writer<units::hours> : ratio_writer_base<'h'>
{
};

template <>
struct ratio_writer<units::days> : ratio_writer_base<'d'>
{
};

template <>
struct ratio_writer<units::weeks> : ratio_writer_base<'w'>
{
};

template <class Ratio, class T>
struct duration_t
{
    using value_type = T;
    value_type m_value;

    constexpr duration_t() : m_value()
    {
    }

    constexpr explicit duration_t(value_type value) : m_value(value)
    {
    }

    template <class OtherRatio, class U>
    constexpr duration_t(duration_t<OtherRatio, U> other) : duration_t()
    {
        using ratio = std::ratio_divide<OtherRatio, Ratio>;
        m_value = static_cast<value_type>(ratio::num * other.m_value) / ratio::den;
    }

    constexpr value_type get() const noexcept
    {
        return m_value;
    }

    friend std::ostream& operator<<(std::ostream& os, const duration_t& item)
    {
        return os << std::fixed << item.m_value << " " << ratio_writer<Ratio>{};
    }
};

template <class L, class R>
using common_ratio = std::conditional_t<std::ratio_less_v<L, R>, L, R>;

template <class Ratio, class T>
constexpr auto operator+(duration_t<Ratio, T> item) -> duration_t<Ratio, T>
{
    return item;
}

template <class Ratio, class T>
constexpr auto operator-(duration_t<Ratio, T> item) -> duration_t<Ratio, T>
{
    return duration_t<Ratio, T>{ -item.m_value };
}

template <
    class LRatio,
    class L,
    class RRatio,
    class R,
    class OutRatio = common_ratio<LRatio, RRatio>,
    class Out = std::invoke_result_t<std::plus<>, L, R>,
    class Res = duration_t<OutRatio, Out>>
constexpr auto operator+(duration_t<LRatio, L> lhs, duration_t<RRatio, R> rhs) -> Res
{
    auto lt = Res{ lhs };
    auto rt = Res{ rhs };
    return Res{ lt.m_value + rt.m_value };
}

template <
    class LRatio,
    class L,
    class RRatio,
    class R,
    class OutRatio = common_ratio<LRatio, RRatio>,
    class Out = std::invoke_result_t<std::minus<>, L, R>,
    class Res = duration_t<OutRatio, Out>>
constexpr auto operator-(duration_t<LRatio, L> lhs, duration_t<RRatio, R> rhs) -> Res
{
    auto lt = Res{ lhs };
    auto rt = Res{ rhs };
    return Res{ lt.m_value - rt.m_value };
}

template <
    class L,
    class RRatio,
    class R,
    class Out = std::invoke_result_t<std::multiplies<>, L, R>,
    class Res = duration_t<RRatio, Out>>
constexpr auto operator*(L lhs, duration_t<RRatio, R> rhs) -> Res
{
    return Res{ lhs * rhs.m_value };
}

template <
    class L,
    class LRatio,
    class R,
    class Out = std::invoke_result_t<std::multiplies<>, L, R>,
    class Res = duration_t<LRatio, Out>>
constexpr auto operator*(duration_t<LRatio, L> lhs, R rhs) -> Res
{
    return Res{ lhs.m_value * rhs };
}

template <
    class L,
    class LRatio,
    class R,
    class Out = std::invoke_result_t<std::divides<>, L, R>,
    class Res = duration_t<LRatio, Out>>
constexpr auto operator/(duration_t<LRatio, L> lhs, R rhs) -> Res
{
    return Res{ lhs.m_value / rhs };
}

template <class LRatio, class L, class RRatio, class R, class Out = std::invoke_result_t<std::divides<>, L, R>>
constexpr auto operator/(duration_t<LRatio, L> lhs, duration_t<RRatio, R> rhs) -> Out
{
    using OutRatio = common_ratio<LRatio, RRatio>;
    using Res = duration_t<OutRatio, Out>;
    auto lt = Res{ lhs };
    auto rt = Res{ rhs };
    return static_cast<Out>(lt.m_value) / rt.m_value;
}

template <class Ratio, class T>
constexpr auto operator==(duration_t<Ratio, T> lhs, duration_t<Ratio, T> rhs) -> bool
{
    return lhs.m_value == rhs.m_value;
}

template <class Ratio, class T>
constexpr auto operator!=(duration_t<Ratio, T> lhs, duration_t<Ratio, T> rhs) -> bool
{
    return lhs.m_value != rhs.m_value;
}

template <class Ratio, class T>
constexpr auto operator<(duration_t<Ratio, T> lhs, duration_t<Ratio, T> rhs) -> bool
{
    return lhs.m_value < rhs.m_value;
}

template <class Ratio, class T>
constexpr auto operator>(duration_t<Ratio, T> lhs, duration_t<Ratio, T> rhs) -> bool
{
    return lhs.m_value > rhs.m_value;
}

template <class Ratio, class T>
constexpr auto operator<=(duration_t<Ratio, T> lhs, duration_t<Ratio, T> rhs) -> bool
{
    return lhs.m_value <= rhs.m_value;
}

template <class Ratio, class T>
constexpr auto operator>=(duration_t<Ratio, T> lhs, duration_t<Ratio, T> rhs) -> bool
{
    return lhs.m_value >= rhs.m_value;
}

template <class T = double>
using nanoseconds_t = duration_t<units::nanoseconds, T>;

template <class T = double>
using microseconds_t = duration_t<units::microseconds, T>;

template <class T = double>
using milliseconds_t = duration_t<units::milliseconds, T>;

template <class T = double>
using seconds_t = duration_t<units::seconds, T>;

template <class T = double>
using minutes_t = duration_t<units::minutes, T>;

template <class T = double>
using hours_t = duration_t<units::hours, T>;

template <class T = double>
using days_t = duration_t<units::days, T>;

template <class T = double>
using weeks_t = duration_t<units::weeks, T>;

namespace literals
{

inline auto operator""_ns(long double v) -> nanoseconds_t<double>
{
    return nanoseconds_t<double>{ static_cast<double>(v) };
}

inline auto operator""_ns(unsigned long long v) -> nanoseconds_t<long>
{
    return nanoseconds_t<long>{ static_cast<long>(v) };
}

inline auto operator""_us(long double v) -> microseconds_t<double>
{
    return microseconds_t<double>{ static_cast<double>(v) };
}

inline auto operator""_us(unsigned long long v) -> microseconds_t<long>
{
    return microseconds_t<long>{ static_cast<long>(v) };
}

inline auto operator""_ms(long double v) -> milliseconds_t<double>
{
    return milliseconds_t<double>{ static_cast<double>(v) };
}

inline auto operator""_ms(unsigned long long v) -> milliseconds_t<long>
{
    return milliseconds_t<long>{ static_cast<long>(v) };
}

inline auto operator""_s(long double v) -> seconds_t<double>
{
    return seconds_t<double>{ static_cast<double>(v) };
}

inline auto operator""_s(unsigned long long v) -> seconds_t<long>
{
    return seconds_t<long>{ static_cast<long>(v) };
}

inline auto operator""_m(long double v) -> minutes_t<double>
{
    return minutes_t<double>{ static_cast<double>(v) };
}

inline auto operator""_m(unsigned long long v) -> minutes_t<long>
{
    return minutes_t<long>{ static_cast<long>(v) };
}

inline auto operator""_h(long double v) -> hours_t<double>
{
    return hours_t<double>{ static_cast<double>(v) };
}

inline auto operator""_h(unsigned long long v) -> hours_t<long>
{
    return hours_t<long>{ static_cast<long>(v) };
}
}  // namespace literals

struct time_only_t
{
    milliseconds_t<double> m_value;

    explicit constexpr time_only_t(milliseconds_t<double> value = {}) : m_value(value)
    {
    }

    constexpr time_only_t(int h, int m, int s = 0, int ms = 0)
        : time_only_t(hours_t<int>{ h } + minutes_t<int>{ m } + seconds_t<int>{ s } + milliseconds_t<int>{ ms })
    {
    }

    constexpr int hours() const
    {
        return static_cast<int>(hours_t<>{ m_value }.get());
    }

    constexpr int minutes() const
    {
        return static_cast<int>(minutes_t<>{ m_value }.get()) % 60;
    }

    constexpr int seconds() const
    {
        return static_cast<int>(seconds_t<>{ m_value }.get()) % 60;
    }

    constexpr int milliseconds() const
    {
        return static_cast<int>(m_value.get()) % 1000;
    }

    constexpr milliseconds_t<double> get() const
    {
        return m_value;
    }

    friend std::ostream& operator<<(std::ostream& os, const time_only_t& item)
    {
        static const auto zeros = std::setfill('0');
        return os << zeros << std::setw(2) << item.hours()           //
                  << ":" << zeros << std::setw(2) << item.minutes()  //
                  << ":" << zeros << std::setw(2) << item.seconds()  //
                  << "." << zeros << std::setw(3) << item.milliseconds();
    }
};

struct julian_date_t
{
    days_t<double> m_value;

    constexpr explicit julian_date_t(days_t<double> value = {}) : m_value(value)
    {
    }

    friend std::ostream& operator<<(std::ostream& os, const julian_date_t item)
    {
        return os << std::fixed << item.m_value.get() << " JD";
    }
};

constexpr auto operator-(julian_date_t lhs, julian_date_t rhs) -> days_t<double>
{
    return lhs.m_value - rhs.m_value;
}

constexpr auto operator+(julian_date_t lhs, days_t<double> rhs) -> julian_date_t
{
    return julian_date_t{ lhs.m_value + rhs };
}

constexpr auto operator-(julian_date_t lhs, days_t<double> rhs) -> julian_date_t
{
    return julian_date_t{ lhs.m_value - rhs };
}

constexpr auto operator+(days_t<double> lhs, julian_date_t rhs) -> julian_date_t
{
    return julian_date_t{ lhs + rhs.m_value };
}

constexpr auto operator==(julian_date_t lhs, julian_date_t rhs) -> bool
{
    return lhs.m_value == rhs.m_value;
}

constexpr auto operator!=(julian_date_t lhs, julian_date_t rhs) -> bool
{
    return lhs.m_value != rhs.m_value;
}

constexpr auto operator<(julian_date_t lhs, julian_date_t rhs) -> bool
{
    return lhs.m_value < rhs.m_value;
}

constexpr auto operator>(julian_date_t lhs, julian_date_t rhs) -> bool
{
    return lhs.m_value > rhs.m_value;
}

constexpr auto operator<=(julian_date_t lhs, julian_date_t rhs) -> bool
{
    return lhs.m_value <= rhs.m_value;
}

constexpr auto operator>=(julian_date_t lhs, julian_date_t rhs) -> bool
{
    return lhs.m_value >= rhs.m_value;
}

struct modified_julian_date_t
{
    days_t<double> m_value;

    constexpr explicit modified_julian_date_t(days_t<double> value) : m_value(value)
    {
    }

    constexpr modified_julian_date_t(julian_date_t jd) : modified_julian_date_t(jd.m_value - offset)
    {
    }

    constexpr operator julian_date_t() const
    {
        return julian_date_t{ m_value + offset };
    }

    friend std::ostream& operator<<(std::ostream& os, const modified_julian_date_t item)
    {
        return os << std::fixed << item.m_value.get() << " MJD";
    }

    static constexpr inline days_t<double> offset = days_t<double>{ 2'400'000.5 };
};

struct unix_time_t
{
    milliseconds_t<double> m_value;

    constexpr explicit unix_time_t(milliseconds_t<double> value) : m_value(value)
    {
    }

    constexpr unix_time_t(julian_date_t jd) : unix_time_t(jd.m_value - unix_epoch)
    {
    }

    constexpr operator julian_date_t() const
    {
        return julian_date_t{ unix_epoch + m_value };
    }

    friend std::ostream& operator<<(std::ostream& os, const unix_time_t item)
    {
        return os << "(unix_time " << std::fixed << item.m_value << ")";
    }

    static auto now() -> unix_time_t
    {
        const std::chrono::system_clock::time_point tp = std::chrono::system_clock::now();
        return unix_time_t(milliseconds_t<double>{
            static_cast<double>(std::chrono::duration_cast<std::chrono::milliseconds>(tp.time_since_epoch()).count()) });
    }

    static constexpr inline days_t<double> unix_epoch = days_t<double>{ 2'440'587.5 };
};

struct utc_time_t
{
    struct date_type
    {
        int year;
        int month;
        int day;

        friend std::ostream& operator<<(std::ostream& os, const date_type& item)
        {
            return os << item.year                                               //
                      << "." << std::setfill('0') << std::setw(2) << item.month  //
                      << "." << std::setfill('0') << std::setw(2) << item.day;
        }

        friend bool operator==(const date_type& lhs, const date_type& rhs)
        {
            return std::tie(lhs.year, lhs.month, lhs.day) == std::tie(rhs.year, rhs.month, rhs.day);
        }
    };

    date_type date;
    time_only_t time;

    constexpr utc_time_t(julian_date_t jd) : date(), time()
    {
        const auto [d, t] = split(jd.m_value.get());
        date = get_date(d);
        time = time_only_t{ days_t<>{ t } };
    }

    constexpr operator julian_date_t() const
    {
        return julian_date_t{ days_t<double>{ gregorian_to_jd(date.year, date.month, date.day) } + time.get() };
    }

    friend std::ostream& operator<<(std::ostream& os, const utc_time_t& item)
    {
        return os << item.date << " " << item.time;
    }

    static constexpr auto split(double value) -> std::pair<double, double>
    {
        const auto d = std::floor(value - 0.5) + 0.5;
        return std::make_pair(d, value - d);
    }

    static constexpr inline double epoch = 1'721'425.5;

    static constexpr int div_floor(double a, double b)
    {
        return std::floor(a / b);
    }

    static constexpr double mod(double a, double b)
    {
        return a - (b * div_floor(a, b));
    }

    static constexpr auto div(double a, double b) -> std::tuple<int, double>
    {
        return { div_floor(a, b), mod(a, b) };
    }

    static constexpr bool leap_gregorian(int year)
    {
        return year % 4 == 0 && !(year % 100 == 0 && year % 400 != 0);
    }

    static constexpr auto gregorian_to_jd(int year, int month, int day) -> double
    {
        return (epoch - 1)                   //
               + (365 * (year - 1))          //
               + div_floor((year - 1), 4)    //
               - div_floor((year - 1), 100)  //
               + div_floor((year - 1), 400)  //
               + div_floor(((367 * month) - 362), 12)
               + ((month <= 2)  //
                      ? 0
                      : leap_gregorian(year)  //
                            ? -1
                            : -2)
               + day;
    }

    static constexpr auto get_date(double jd) -> utc_time_t::date_type
    {
        const auto wjd = std::floor(jd - 0.5) + 0.5;
        const auto depoch = wjd - epoch;
        const auto [quadricent, d_quadricent] = div(depoch, 146'097);
        const auto [cent, d_cent] = div(d_quadricent, 36'524);
        const auto [quad, d_quad] = div(d_cent, 1'461);
        const auto year_index = div_floor(d_quad, 365);
        const auto year = (quadricent * 400)  //
                          + (cent * 100)      //
                          + (quad * 4)        //
                          + year_index        //
                          + (cent != 4 && year_index != 4 ? 1 : 0);
        const auto yearday = wjd - gregorian_to_jd(year, 1, 1);
        const auto leap_adj
            = ((wjd < gregorian_to_jd(year, 3, 1))  //
                   ? 0
                   : (leap_gregorian(year)  //
                          ? 1
                          : 2));
        const auto month = div_floor((((yearday + leap_adj) * 12) + 373), 367);
        const auto day = (wjd - gregorian_to_jd(year, month, 1)) + 1;

        return utc_time_t::date_type{ static_cast<int>(year), static_cast<int>(month), static_cast<int>(day) };
    }
};

struct local_time_t
{
    utc_time_t utc;
    minutes_t<int> offset;

    friend std::ostream& operator<<(std::ostream& os, const local_time_t item)
    {
        os << utc_time_t{ item.utc + item.offset };
        int offset_min = item.offset.get();
        os << (offset_min >= 0 ? "+" : "-");
        offset_min = std::abs(offset_min);
        os << std::setfill('0') << std::setw(2) << (offset_min / 60);
        os << ":" << std::setfill('0') << std::setw(2) << (offset_min % 60);
        return os;
    }
};

}  // namespace core
}  // namespace ferrugo
