#pragma once

#include <iostream>
#include <numeric>

namespace ferrugo
{
namespace core
{

class rational
{
public:
    using value_type = int;
    value_type m_n, m_d;

    constexpr rational() : m_n(0), m_d(1)
    {
    }

    constexpr rational(value_type n, value_type d = 1) : m_n(n), m_d(d)
    {
        if (m_d == 0)
        {
            throw std::runtime_error{ "division by zero" };
        }
        reduce();
    }

    constexpr rational(const rational&) = default;
    constexpr rational(rational&&) noexcept = default;

    rational& operator=(rational other)
    {
        swap(other);
        return *this;
    }

    template <class F, class = std::enable_if_t<std::is_floating_point_v<F>>>
    static constexpr auto from_float(F v, int precision) -> rational
    {
        return rational{ static_cast<value_type>(v * std::pow(10, precision)),
                         static_cast<value_type>(std::pow(10, precision)) };
    }

    template <class F, class = std::enable_if_t<std::is_floating_point_v<F>>>
    operator F() const
    {
        return static_cast<F>(numerator()) / denominator();
    }

    constexpr void reduce()
    {
        const int div = std::gcd(m_d, m_n);
        m_n /= div;
        m_d /= div;
        if (m_d < 0)
        {
            m_n = -m_n;
            m_d = -m_d;
        }
    }

    void swap(rational& other) noexcept
    {
        std::swap(m_n, other.m_n);
        std::swap(m_d, other.m_d);
    }

    constexpr auto numerator() const noexcept -> value_type
    {
        return m_n;
    }

    constexpr auto denominator() const noexcept -> value_type
    {
        return m_d;
    }

    friend std::ostream& operator<<(std::ostream& os, const rational& item)
    {
        return os << item.m_n << "/" << item.m_d;
    }

    constexpr auto reciprocal() const -> rational
    {
        return rational{ denominator(), numerator() };
    }

    friend constexpr auto operator+(rational item) -> rational
    {
        return rational{ item.numerator(), item.denominator() };
    }

    friend constexpr auto operator-(rational item) -> rational
    {
        return rational{ -item.numerator(), item.denominator() };
    }

    friend constexpr auto operator+(rational lhs, rational rhs) -> rational
    {
        return lhs.denominator() == rhs.denominator()
                   ? rational{ lhs.numerator() + rhs.numerator(), lhs.denominator() }
                   : rational{ lhs.numerator() * rhs.denominator() + rhs.numerator() * lhs.denominator(),
                               lhs.denominator() * rhs.denominator() };
    }

    friend auto operator+=(rational& lhs, rational rhs) -> rational&
    {
        return lhs = lhs + rhs;
    }

    friend constexpr auto operator-(rational lhs, rational rhs) -> rational
    {
        return lhs + (-rhs);
    }

    friend auto operator-=(rational& lhs, rational rhs) -> rational&
    {
        return lhs = lhs - rhs;
    }

    friend constexpr auto operator*(rational lhs, rational rhs) -> rational
    {
        return rational{ lhs.numerator() * rhs.numerator(), lhs.denominator() * rhs.denominator() };
    }

    friend auto operator*=(rational& lhs, rational rhs) -> rational&
    {
        return lhs = lhs * rhs;
    }

    friend constexpr auto operator/(rational lhs, rational rhs) -> rational
    {
        return lhs * rhs.reciprocal();
    }

    friend auto operator/=(rational& lhs, rational rhs) -> rational&
    {
        return lhs = lhs / rhs;
    }

    friend constexpr auto operator==(rational lhs, rational rhs) -> bool
    {
        return lhs.numerator() * rhs.denominator() == rhs.numerator() * lhs.denominator();
    }

    friend constexpr auto operator!=(rational lhs, rational rhs) -> bool
    {
        return !(lhs == rhs);
    }

    friend constexpr auto operator<(rational lhs, rational rhs) -> bool
    {
        return lhs.numerator() * rhs.denominator() < rhs.numerator() * lhs.denominator();
    }

    friend constexpr auto operator>(rational lhs, rational rhs) -> bool
    {
        return rhs < lhs;
    }

    friend constexpr auto operator<=(rational lhs, rational rhs) -> bool
    {
        return !(lhs > rhs);
    }

    friend constexpr auto operator>=(rational lhs, rational rhs) -> bool
    {
        return !(lhs < rhs);
    }
};

}  // namespace core
}  // namespace ferrugo
