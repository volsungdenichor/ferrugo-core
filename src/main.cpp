
#include <array>
#include <cmath>
#include <cstdint>
#include <cuchar>
#include <deque>
#include <ferrugo/core/backtrace.hpp>
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
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>

#include "next.hpp"
#include "parsing.hpp"

struct value_predicate_impl_t
{
    using ptr_t = std::unique_ptr<value_predicate_impl_t>;
    virtual ~value_predicate_impl_t() = default;
    virtual ptr_t clone() const = 0;
    virtual bool match(const next::value_t& v) const = 0;
    virtual next::value_t format() const = 0;
};

struct value_predicate_t
{
    value_predicate_impl_t::ptr_t m_impl;

    explicit value_predicate_t(value_predicate_impl_t::ptr_t impl) : m_impl(std::move(impl))
    {
    }

    value_predicate_t(const value_predicate_t& other) : value_predicate_t(other.m_impl->clone())
    {
    }

    value_predicate_t(value_predicate_t&&) noexcept = default;

    value_predicate_t& operator=(value_predicate_t other)
    {
        std::swap(m_impl, other.m_impl);
        return *this;
    }

    bool operator()(const next::value_t& v) const
    {
        return m_impl->match(v);
    }

    next::value_t format() const
    {
        return m_impl->format();
    }

    friend std::ostream& operator<<(std::ostream& os, const value_predicate_t& item)
    {
        return os << item.format();
    }
};

template <class Self>
struct value_predicate_impl_base_t : value_predicate_impl_t
{
    ptr_t clone() const override
    {
        return std::make_unique<Self>(*static_cast<const Self*>(this));
    }
};

template <class Op, char... Name>
struct logical_fn
{
    struct impl : value_predicate_impl_base_t<impl>
    {
        std::vector<value_predicate_t> m_preds;

        explicit impl(std::vector<value_predicate_t> p) : m_preds(std::move(p))
        {
        }

        bool match(const next::value_t& v) const override
        {
            static const auto op = Op{};
            return op(m_preds.begin(), m_preds.end(), [&](const value_predicate_t& p) { return p(v); });
        }

        next::value_t format() const override
        {
            static const std::string name = { Name... };
            next::list_t result;
            result.push_back(name);
            for (const auto& p : m_preds)
            {
                result.push_back(p.format());
            }
            return result;
        }
    };

    auto operator()(std::vector<value_predicate_t> preds) const -> value_predicate_t
    {
        return value_predicate_t(std::make_unique<impl>(std::move(preds)));
    }
};

template <class Op, char... Name>
struct cmp_fn
{
    struct impl : value_predicate_impl_base_t<impl>
    {
        next::value_t m_value;

        explicit impl(next::value_t v) : m_value(std::move(v))
        {
        }

        bool match(const next::value_t& v) const override
        {
            static const auto op = Op{};
            return op(v, m_value);
        }
        next::value_t format() const override
        {
            static const std::string name = { Name... };
            return next::list_t{ name, m_value };
        }
    };

    auto operator()(next::value_t value) const -> value_predicate_t
    {
        return value_predicate_t(std::make_unique<impl>(std::move(value)));
    }
};

struct all_of_impl
{
    template <class Iter, class Pred>
    bool operator()(Iter b, Iter e, Pred pred) const
    {
        return std::all_of(b, e, std::ref(pred));
    }
};

struct any_of_impl
{
    template <class Iter, class Pred>
    bool operator()(Iter b, Iter e, Pred pred) const
    {
        return std::any_of(b, e, std::ref(pred));
    }
};

struct none_of_impl
{
    template <class Iter, class Pred>
    bool operator()(Iter b, Iter e, Pred pred) const
    {
        return std::none_of(b, e, std::ref(pred));
    }
};

static constexpr inline auto all_of = logical_fn<all_of_impl, 'a', 'l', 'l', '_', 'o', 'f'>{};
static constexpr inline auto any_of = logical_fn<any_of_impl, 'a', 'n', 'y', '_', 'o', 'f'>{};
static constexpr inline auto none_of = logical_fn<none_of_impl, 'n', 'o', 'n', 'e', '_', 'o', 'f'>{};

static constexpr inline auto eq = cmp_fn<std::equal_to<>, '='>{};
static constexpr inline auto ne = cmp_fn<std::not_equal_to<>, '!', '='>{};
static constexpr inline auto lt = cmp_fn<std::less<>, '<'>{};
static constexpr inline auto gt = cmp_fn<std::greater<>, '>'>{};
static constexpr inline auto le = cmp_fn<std::less_equal<>, '<', '='>{};
static constexpr inline auto ge = cmp_fn<std::greater_equal<>, '>', '='>{};

int run(const std::vector<std::string_view>& args)
{
    std::cout << all_of({ eq("Ala ma kota"), ne("Antifa ma HIV-a") }) << "\n";
    std::cout << next::parse(R"([and [>= "Ala ma kota"] [<= "Antifa ma HIV-a"]])");
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
