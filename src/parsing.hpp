#pragma once

#include <functional>
#include <optional>
#include <string>
#include <string_view>

namespace parsing
{

using parse_result_t = std::pair<std::string, std::string_view>;

using parser_t = std::function<std::optional<parse_result_t>(std::string_view)>;
struct parser_combinator_t : public std::function<parser_t(parser_t)>
{
    using base_t = std::function<parser_t(parser_t)>;
    using base_t::base_t;

    friend auto operator|(parser_t parser, const parser_combinator_t& self) -> parser_t
    {
        return self(std::move(parser));
    }
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

inline auto any_of(std::string chars) -> char_predicate_t
{
    return [=](char ch) { return chars.find(ch) != std::string::npos; };
}

const inline char_predicate_t is_space = [](char ch) { return std::isspace(ch); };

constexpr inline auto drop = [](std::string_view str, std::size_t n) -> std::string_view
{
    str.remove_prefix(std::min(str.size(), n));
    return str;
};

constexpr inline struct character_fn
{
    auto operator()(char_predicate_t pred) const -> parser_t
    {
        return [=](std::string_view text) -> std::optional<parse_result_t>
        {
            if (!text.empty() && pred(text[0]))
            {
                return { { std::string(1, text[0]), drop(text, 1) } };
            }
            return {};
        };
    }
} character{};

constexpr inline struct string_fn
{
    auto operator()(std::string str) const -> parser_t
    {
        return [=](std::string_view text) -> std::optional<parse_result_t>
        {
            if (text.size() >= str.size() && text.substr(0, str.size()) == str)
            {
                return { { std::string(text.substr(0, str.size())), drop(text, str.size()) } };
            }
            return {};
        };
    }
} string{};

constexpr inline struct any_fn
{
    auto operator()(std::vector<parser_t> parsers) const -> parser_t
    {
        return [=](std::string_view text) -> std::optional<parse_result_t>
        {
            for (const parser_t& parser : parsers)
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
    auto operator()(parser_t head, Tail... tail) const -> parser_t
    {
        return (*this)(std::vector<parser_t>{ std::move(head), std::move(tail)... });
    }
} any{};

constexpr inline struct sequence_fn
{
    auto operator()(std::vector<parser_t> parsers) const -> parser_t
    {
        return [=](std::string_view text) -> std::optional<parse_result_t>
        {
            std::string result = {};
            std::string_view remainder = text;
            for (const parser_t& parser : parsers)
            {
                if (const auto res = parser(remainder))
                {
                    const auto& [val, rem] = *res;
                    result += val;
                    remainder = rem;
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
    auto operator()(parser_t head, Tail... tail) const -> parser_t
    {
        return (*this)(std::vector<parser_t>{ std::move(head), std::move(tail)... });
    }
} sequence{};

constexpr inline struct repeat_fn
{
    auto operator()(count_predicate_t pred) const -> parser_combinator_t
    {
        return [=](parser_t parser) -> parser_t
        {
            return [=](std::string_view text) -> std::optional<parse_result_t>
            {
                std::size_t count = 0;
                std::string result = {};
                std::string_view remainder = text;
                while (!remainder.empty())
                {
                    if (const auto res = parser(remainder))
                    {
                        const auto& [val, rem] = *res;
                        result += val;
                        remainder = rem;
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

const inline parser_combinator_t zero_or_more = repeat([](std::size_t v) { return v >= 0; });
const inline parser_combinator_t one_or_more = repeat([](std::size_t v) { return v >= 1; });

constexpr inline struct optional_fn
{
    auto operator()(parser_t parser) const -> parser_t
    {
        return [=](std::string_view text) -> std::optional<parse_result_t>
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
    auto operator()(std::function<std::string(std::string)> func) const -> parser_combinator_t
    {
        return [=](parser_t parser) -> parser_t
        {
            return [=](std::string_view text) -> std::optional<parse_result_t>
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
    auto operator()(parser_t second) const -> parser_combinator_t
    {
        return [=](parser_t first) -> parser_t
        {
            return [=](std::string_view text) -> std::optional<parse_result_t>
            {
                if (auto v = first(text))
                {
                    text = v->second;
                }
                return second(text);
            };
        };
    }
} then{};

inline auto quoted_string() -> parser_t
{
    static const auto replace = [](std::string value) -> std::function<std::string(std::string)>
    {  //
        return [=](std::string) -> std::string { return value; };
    };
    static const auto quote = character(eq('"')) | transform(replace(""));
    return sequence(  //
        quote,
        any(string("\\\"") | transform(replace("\"")), character(ne('"'))) | zero_or_more,
        quote);
}

inline auto csv(char separator) -> parsing::parser_t
{
    const auto sep = sequence(  //
        character(is_space) | zero_or_more,
        character(eq(separator)),
        character(is_space) | zero_or_more);

    const auto item = any(quoted_string(), character(ne(separator)) | zero_or_more);

    return sep | then(item);
}

constexpr inline struct tokenize_fn
{
    template <class Out>
    auto operator()(Out out, std::string_view text, const parser_t& parser) const -> Out
    {
        while (!text.empty())
        {
            if (const auto res = parser(text))
            {
                auto [token, remainder] = *res;
                *out++ = std::move(token);
                text = remainder;
            }
            else
            {
                break;
            }
        }
        return out;
    }

    auto operator()(std::string_view text, const parser_t& parser) const -> std::vector<std::string>
    {
        std::vector<std::string> out;
        (*this)(std::back_inserter(out), text, parser);
        return out;
    }
} tokenize{};

}  // namespace parsing
