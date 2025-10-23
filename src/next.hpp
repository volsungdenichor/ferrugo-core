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

template <class A, class L, class M>
struct value_base_t
{
    using atom_type = A;
    using list_type = L;
    using map_type = M;
    using data_type = std::variant<atom_type, list_type, map_type>;
    data_type m_data;

    template <class T, class = std::enable_if_t<std::is_constructible_v<data_type, T>>>
    value_base_t(T&& val) : m_data(std::forward<T>(val))
    {
    }

    value_base_t() : value_base_t(atom_type{})
    {
    }

    const atom_type* if_atom() const
    {
        return std::get_if<atom_type>(&m_data);
    }

    const list_type* if_list() const
    {
        return std::get_if<list_type>(&m_data);
    }

    const map_type* if_map() const
    {
        return std::get_if<map_type>(&m_data);
    }
};

struct value_t;

struct list_t : public std::vector<value_t>
{
    using base_t = std::vector<value_t>;
    using base_t::base_t;

    friend std::ostream& operator<<(std::ostream& os, const list_t& item);
};

struct map_t : public std::unordered_map<string_t, value_t>
{
    using base_t = std::unordered_map<string_t, value_t>;
    using base_t::base_t;

    friend std::ostream& operator<<(std::ostream& os, const map_t& item);
};

struct value_t : value_base_t<string_t, list_t, map_t>
{
    using base_t = value_base_t<string_t, list_t, map_t>;
    using base_t::base_t;

    friend std::ostream& operator<<(std::ostream& os, const value_t& item);
};

struct print_visitor
{
    std::ostream& os;
    void operator()(const string_t& item) const
    {
        if (!item.empty()
            && std::none_of(
                item.begin(), item.end(), [](char ch) { return parsing::is_space(ch) || parsing::one_of("\"[]{}")(ch); }))
        {
            os << item;
        }
        else
        {
            os << std::quoted(item);
        }
    }

    void operator()(const list_t& item) const
    {
        os << item;
    }

    void operator()(const map_t& item) const
    {
        os << item;
    }
};

inline std::ostream& operator<<(std::ostream& os, const value_t& item)
{
    std::visit(print_visitor{ os }, item.m_data);
    return os;
}

struct eq_visitor
{
    template <class T>
    bool operator()(const T& lt, const T& rt) const
    {
        return lt == rt;
    }

    template <class L, class R>
    bool operator()(const L& lt, const R& rt) const
    {
        return false;
    }
};

struct lt_visitor
{
    template <class T>
    bool operator()(const T& lt, const T& rt) const
    {
        return lt < rt;
    }

    template <class L, class R>
    bool operator()(const L& lt, const R& rt) const
    {
        return false;
    }
};

inline bool operator==(const value_t& lhs, const value_t& rhs)
{
    return std::visit(eq_visitor{}, lhs.m_data, rhs.m_data);
}

inline bool operator!=(const value_t& lhs, const value_t& rhs)
{
    return !(lhs == rhs);
}

inline bool operator<(const value_t& lhs, const value_t& rhs)
{
    return std::visit(lt_visitor{}, lhs.m_data, rhs.m_data);
}

inline bool operator>(const value_t& lhs, const value_t& rhs)
{
    return rhs < lhs;
}

inline bool operator<=(const value_t& lhs, const value_t& rhs)
{
    return !(lhs > rhs);
}

inline bool operator>=(const value_t& lhs, const value_t& rhs)
{
    return !(lhs < rhs);
}

inline std::ostream& operator<<(std::ostream& os, const list_t& item)
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

inline std::ostream& operator<<(std::ostream& os, const map_t& item)
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

constexpr inline struct tokenize_fn
{
    auto operator()(std::string_view text) const -> std::vector<std::string>
    {
        static const auto parser = create_parser();
        return parsing::tokenize(text, parser);
    }

private:
    static auto create_parser() -> parsing::parser_t<std::string>
    {
        namespace p = parsing;
        static const auto is_parenthesis = p::one_of("{}[]");
        static const auto literal
            = p::at_least(1)(p::character([](char ch) { return !(p::is_space(ch) || is_parenthesis(ch) || ch == '"'); }));
        return (p::many(p::whitespace)) >> p::any(p::character(is_parenthesis), p::quoted_string(), literal);
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
} parse{};

}  // namespace next
