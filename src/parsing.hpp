#pragma once

#include <functional>
#include <optional>
#include <string>
#include <string_view>

namespace parsing
{

template <class T>
using parse_result_t = std::pair<T, std::string_view>;

template <class T>
using maybe_parse_result_t = std::optional<parse_result_t<T>>;

template <class T>
struct parser_t : public std::function<maybe_parse_result_t<T>(std::string_view)>
{
    using base_t = std::function<maybe_parse_result_t<T>(std::string_view)>;
    using base_t::base_t;
};

template <class T>
struct parser_combinator_t : public std::function<parser_t<T>(parser_t<T>)>
{
    using base_t = std::function<parser_t<T>(parser_t<T>)>;
    using base_t::base_t;
};

using char_predicate_t = std::function<bool(char)>;
using count_predicate_t = std::function<bool(std::size_t)>;

inline auto eq(char v) -> char_predicate_t
{
    return [=](char ch) { return ch == v; };
}

inline auto ne(char v) -> char_predicate_t
{
    return [=](char ch) { return ch != v; };
}

inline auto one_of(std::string chars) -> char_predicate_t
{
    return [=](char ch) { return chars.find(ch) != std::string::npos; };
}

const inline char_predicate_t is_space = [](char ch) { return std::isspace(ch); };

constexpr inline struct slice_fn
{
    struct impl_t
    {
        std::optional<std::ptrdiff_t> begin;
        std::optional<std::ptrdiff_t> end;

        static auto adjust(std::ptrdiff_t index, std::ptrdiff_t size) -> std::ptrdiff_t
        {
            return std::clamp<std::ptrdiff_t>(index >= 0 ? index : index + size, 0, size);
        };

        constexpr auto calculate(std::ptrdiff_t s) const -> std::pair<std::ptrdiff_t, std::ptrdiff_t>
        {
            const std::ptrdiff_t b = begin ? adjust(*begin, s) : std::ptrdiff_t{ 0 };
            const std::ptrdiff_t e = end ? adjust(*end, s) : s;
            return { b, std::max(std::ptrdiff_t{ 0 }, e - b) };
        }

        friend auto operator|(std::string_view str, const impl_t& self) -> std::string_view
        {
            const auto [start, size] = self.calculate(str.size());
            return str.substr(start, size);
        }

        friend auto operator|(std::string str, const impl_t& self) -> std::string
        {
            const auto [start, size] = self.calculate(str.size());
            return str.substr(start, size);
        }
    };

    constexpr auto operator()(std::optional<std::ptrdiff_t> begin, std::optional<std::ptrdiff_t> end) const -> impl_t
    {
        return impl_t{ begin, end };
    };
} slice{};

constexpr inline struct character_fn
{
    auto operator()(char_predicate_t pred) const -> parser_t<std::string>
    {
        return [=](std::string_view text) -> maybe_parse_result_t<std::string>
        {
            if (!text.empty() && pred(text[0]))
            {
                return { { std::string{ text | slice({}, 1) }, text | slice(1, {}) } };
            }
            return {};
        };
    }
} character{};

static const inline auto whitespace = character(is_space);
static const inline auto digit = character([](char ch) { return std::isdigit(ch); });
static const inline auto alnum = character([](char ch) { return std::isalnum(ch); });
static const inline auto alpha = character([](char ch) { return std::isalpha(ch); });
static const inline auto upper = character([](char ch) { return std::isupper(ch); });
static const inline auto lower = character([](char ch) { return std::islower(ch); });

constexpr inline struct string_fn
{
    auto operator()(std::string str) const -> parser_t<std::string>
    {
        return [=](std::string_view text) -> maybe_parse_result_t<std::string>
        {
            if ((text | slice({}, str.size())) == str)
            {
                return { { std::string{ text | slice({}, str.size()) }, text | slice(str.size(), {}) } };
            }
            return {};
        };
    }
} string{};

constexpr inline struct any_fn
{
    auto operator()(std::vector<parser_t<std::string>> parsers) const -> parser_t<std::string>
    {
        return [=](std::string_view text) -> maybe_parse_result_t<std::string>
        {
            for (const parser_t<std::string>& parser : parsers)
            {
                if (const auto res = parser(text))
                {
                    return *res;
                }
            }
            return {};
        };
    }

    template <class... Tail>
    auto operator()(parser_t<std::string> head, Tail... tail) const -> parser_t<std::string>
    {
        return (*this)(std::vector<parser_t<std::string>>{ std::move(head), std::move(tail)... });
    }
} any{};

constexpr inline struct sequence_fn
{
    auto operator()(std::vector<parser_t<std::string>> parsers) const -> parser_t<std::string>
    {
        return [=](std::string_view text) -> maybe_parse_result_t<std::string>
        {
            std::string result = {};
            std::string_view remainder = text;
            for (const parser_t<std::string>& parser : parsers)
            {
                if (const auto res = parser(remainder))
                {
                    result += res->first;
                    remainder = res->second;
                }
                else
                {
                    return {};
                }
            }
            return { { std::move(result), remainder } };
        };
    }

    template <class... Tail>
    auto operator()(parser_t<std::string> head, Tail... tail) const -> parser_t<std::string>
    {
        return (*this)(std::vector<parser_t<std::string>>{ std::move(head), std::move(tail)... });
    }
} sequence{};

constexpr inline struct repeat_fn
{
    auto operator()(count_predicate_t pred) const -> parser_combinator_t<std::string>
    {
        return [=](parser_t<std::string> parser) -> parser_t<std::string>
        {
            return [=](std::string_view text) -> maybe_parse_result_t<std::string>
            {
                std::size_t count = 0;
                std::string result = {};
                std::string_view remainder = text;
                while (!remainder.empty())
                {
                    if (const auto res = parser(remainder))
                    {
                        result += res->first;
                        remainder = res->second;
                        count += 1;
                    }
                    else
                    {
                        break;
                    }
                }
                if (pred(count))
                {
                    return { { std::move(result), remainder } };
                }
                return {};
            };
        };
    }
} repeat{};

const inline parser_combinator_t<std::string> many = repeat([](std::size_t v) { return v >= 0; });

inline auto at_least(std::size_t count)
{
    return repeat([=](std::size_t v) { return v >= count; });
}

inline auto at_most(std::size_t count)
{
    return repeat([=](std::size_t v) { return v <= count; });
}

inline auto times(std::size_t lo, std::size_t up)
{
    return repeat([=](std::size_t v) { return lo <= v && v <= up; });
}

inline auto times(std::size_t count)
{
    return repeat([=](std::size_t v) { return v == count; });
}

constexpr inline struct optional_fn
{
    auto operator()(parser_t<std::string> parser) const -> parser_t<std::string>
    {
        return [=](std::string_view text) -> maybe_parse_result_t<std::string>
        {
            if (auto res = parser(text))
            {
                return *res;
            }
            return { { "", text } };
        };
    }
} optional{};

constexpr inline struct transform_fn
{
    auto operator()(std::function<std::string(std::string)> func) const -> parser_combinator_t<std::string>
    {
        return [=](parser_t<std::string> parser) -> parser_t<std::string>
        {
            return [=](std::string_view text) -> maybe_parse_result_t<std::string>
            {
                if (auto res = parser(text))
                {
                    return { { func(std::move(res->first)), res->second } };
                }
                return std::nullopt;
            };
        };
    }
} transform{};

constexpr inline struct then_fn
{
    auto operator()(parser_t<std::string> second) const -> parser_combinator_t<std::string>
    {
        return [=](parser_t<std::string> first) -> parser_t<std::string>
        {
            return [=](std::string_view text) -> maybe_parse_result_t<std::string>
            {
                if (auto fst = first(text))
                {
                    return second(fst->second);
                }
                return {};
            };
        };
    }
} then{};

constexpr inline struct skip_fn
{
    auto operator()(parser_t<std::string> second) const -> parser_combinator_t<std::string>
    {
        return [=](parser_t<std::string> first) -> parser_t<std::string>
        {
            return [=](std::string_view text) -> maybe_parse_result_t<std::string>
            {
                if (auto fst = first(text))
                {
                    std::string result = fst->first;
                    if (auto sec = second(fst->second))
                    {
                        return { { std::move(result), sec->second } };
                    }
                }
                return {};
            };
        };
    }
} skip{};

inline auto operator>>(parser_t<std::string> lhs, parser_t<std::string> rhs) -> parser_t<std::string>
{
    return then(std::move(rhs))(std::move(lhs));
}

inline auto operator<<(parser_t<std::string> lhs, parser_t<std::string> rhs) -> parser_t<std::string>
{
    return skip(std::move(rhs))(std::move(lhs));
}

inline auto quoted_string() -> parser_t<std::string>
{
    static const auto replace_with = [](std::string value) -> std::function<std::string(std::string)>
    {  //
        return [=](std::string) -> std::string { return value; };
    };
    return character(eq('"')) >> many(any(string("\\") >> string("\""), character(ne('"')))) << character(eq('"'));
}

inline auto csv(char separator) -> parser_t<std::string>
{
    const auto sep = sequence(  //
        many(whitespace),
        character(eq(separator)),
        many(whitespace));

    const auto item = many(any(quoted_string(), character(ne(separator))));

    return item << sep;
}

constexpr inline struct tokenize_fn
{
    template <class State, class Func>
    auto operator()(std::string_view text, const parser_t<std::string>& parser, State state, Func func) const -> State
    {
        while (!text.empty())
        {
            if (const auto res = parser(text))
            {
                state = std::invoke(func, std::move(state), std::move(res->first));
                text = res->second;
            }
            else
            {
                break;
            }
        }
        return state;
    }

    template <class Out>
    auto operator()(std::string_view text, const parser_t<std::string>& parser, Out out) const -> Out
    {
        const auto func = [](Out o, std::string token) -> Out
        {
            *o++ = std::move(token);
            return o;
        };
        return (*this)(text, parser, std::move(out), func);
    }

    auto operator()(std::string_view text, const parser_t<std::string>& parser) const -> std::vector<std::string>
    {
        std::vector<std::string> out;
        (*this)(text, parser, std::back_inserter(out));
        return out;
    }
} tokenize{};

}  // namespace parsing
