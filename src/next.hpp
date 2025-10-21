#pragma once

#include <iomanip>
#include <iostream>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

#include "parsing.hpp"

namespace next
{

using string_t = std::string;

struct print_visitor_t
{
    std::ostream& os;

    template <class T>
    void operator()(const T& v) const
    {
        os << v;
    }

    void operator()(const string_t& v) const
    {
        if (!v.empty()
            && std::none_of(
                v.begin(), v.end(), [](char ch) { return parsing::is_space(ch) && parsing::any_of("\"[]{}")(ch); }))
        {
            os << v;
        }
        else
        {
            os << std::quoted(v);
        }
    }
};

template <class T>
struct list_base_t : public std::vector<T>
{
    using base_t = std::vector<T>;
    using base_t::base_t;

    friend std::ostream& operator<<(std::ostream& os, const list_base_t& item)
    {
        os << "[";
        for (auto it = item.begin(); it != item.end(); ++it)
        {
            if (it != item.begin())
            {
                os << " ";
            }
            os << *it;
        }
        os << "]";
        return os;
    }
};

template <class T>
struct map_base_t : public std::unordered_map<string_t, T>
{
    using base_t = std::unordered_map<string_t, T>;
    using base_t::base_t;

    friend std::ostream& operator<<(std::ostream& os, const map_base_t& item)
    {
        os << "{";
        for (auto it = item.begin(); it != item.end(); ++it)
        {
            if (it != item.begin())
            {
                os << " ";
            }
            os << it->first << " " << it->second;
        }
        os << "}";
        return os;
    }
};

template <class A, class L, class M>
struct value_base_t
{
    using data_type = std::variant<A, L, M>;
    data_type m_data;

    template <class T, class = std::enable_if_t<std::is_constructible_v<data_type, T>>>
    value_base_t(T&& val) : m_data(std::forward<T>(val))
    {
    }

    value_base_t() : value_base_t(A{})
    {
    }

    const A* if_atom() const
    {
        return std::get_if<A>(&m_data);
    }

    const L* if_list() const
    {
        return std::get_if<L>(&m_data);
    }

    const M* if_map() const
    {
        return std::get_if<M>(&m_data);
    }

    friend std::ostream& operator<<(std::ostream& os, const value_base_t& item)
    {
        std::visit(print_visitor_t{ os }, item.m_data);
        return os;
    }
};

struct list_t;
struct map_t;

using value_t = value_base_t<string_t, list_t, map_t>;

struct list_t : list_base_t<value_t>
{
    using base_t = list_base_t<value_t>;
    using base_t::base_t;
};

struct map_t : map_base_t<value_t>
{
    using base_t = map_base_t<value_t>;
    using base_t::base_t;
};

template <class... Args>
auto list(Args&&... args) -> list_t
{
    return list_t{ { std::forward<Args>(args)... } };
}

constexpr inline struct tokenize_fn
{
    auto operator()(std::string_view text) const -> std::vector<std::string>
    {
        static const auto parser = create_parser();
        return parsing::tokenize(text, parser);
    }

private:
    static auto create_parser() -> parsing::parser_t
    {
        namespace p = parsing;
        static const auto is_parenthesis = p::any_of("{}[]");
        static const auto literal
            = p::character([](char ch) { return !p::is_space(ch) && !is_parenthesis(ch) && ch != '"'; }) | p::one_or_more;

        return (p::character(p::is_space) | p::zero_or_more)
               | p::then(p::any(p::character(is_parenthesis), p::quoted_string(), literal));
    }
} tokenize{};

constexpr inline struct parse_fn
{
    auto operator()(std::string_view text) const -> value_t
    {
        std::vector<std::string> tokens = tokenize(text);
        std::vector<value_t> values = {};
        while (!tokens.empty())
        {
            values.push_back(read_from(tokens));
        }
        if (values.size() != 1)
        {
            throw std::runtime_error{ "Exactly one value required" };
        }
        return values.at(0);
    }

private:
    template <class T>
    static T pop_front(std::vector<T>& v)
    {
        if (v.empty())
        {
            throw std::runtime_error{ "Cannot pop from empty vector" };
        }
        T result = v.front();
        v.erase(std::begin(v));
        return result;
    }

    static auto to_map(const std::vector<value_t>& items) -> value_t
    {
        if (items.size() % 2 != 0)
        {
            throw std::runtime_error{ "Map expects to have even number of elements" };
        }
        map_t result = {};
        for (std::size_t i = 0; i < items.size(); i += 2)
        {
            result.emplace(*items[i + 0].if_atom(), items[i + 1]);
        }
        return result;
    }

    static auto to_list(const std::vector<value_t>& items) -> value_t
    {
        return list_t{ items.begin(), items.end() };
    }

    static auto read_until(std::vector<std::string>& tokens, const std::string& delimiter) -> std::vector<value_t>
    {
        if (tokens.empty())
        {
            throw std::runtime_error{ "invalid parentheses" };
        }
        std::vector<value_t> result = {};
        while (!tokens.empty() && tokens.front() != delimiter)
        {
            result.push_back(read_from(tokens));
        }
        pop_front(tokens);
        return result;
    }

    static auto read_from(std::vector<std::string>& tokens) -> value_t
    {
        static const auto parentheses
            = std::vector<std::tuple<std::string, std::string, value_t (*)(const std::vector<value_t>&)>>{
                  { "[", "]", &parse_fn::to_list },
                  { "{", "}", &parse_fn::to_map },
              };

        if (tokens.empty())
        {
            return value_t();
        }
        const string_t front = pop_front(tokens);
        for (const auto& [opening, closing, func] : parentheses)
        {
            if (front == opening)
            {
                return func(read_until(tokens, closing));
            }
        }
        return front;
    }
} parse{};

}  // namespace next
