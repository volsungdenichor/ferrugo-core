#pragma once

#include <cstdint>
#include <functional>
#include <iostream>
#include <limits>
#include <map>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace ferrugo
{
namespace lisp
{

template <class... Args>
auto str(Args&&... args) -> std::string
{
    std::stringstream ss;
    (ss << ... << std::forward<Args>(args));
    return ss.str();
}

struct nil_t
{
    friend std::ostream& operator<<(std::ostream& os, const nil_t);
};

static constexpr inline auto nil = nil_t{};

struct value_t;

using boolean_t = bool;
using byte_t = unsigned char;
using character_t = char;
using integer_t = std::int32_t;
using real_t = double;
using string_t = std::string;

struct symbol_t : public std::string
{
    using base_t = std::string;
    using base_t::base_t;
};

struct list_t;
struct map_t;
struct callable_t;

namespace detail
{

using value_impl_t = std::variant<  //
    nil_t,
    boolean_t,
    byte_t,
    character_t,
    integer_t,
    real_t,
    string_t,
    symbol_t,
    list_t,
    map_t,
    callable_t>;

}  // namespace detail

enum class type_t
{
    nil,
    boolean,
    byte,
    character,
    integer,
    real,
    string,
    symbol,
    list,
    map,
    callable
};

inline std::ostream& operator<<(std::ostream& os, const type_t item)
{
#define CASE(x) \
    case type_t::x: return os << #x
    switch (item)
    {
        CASE(nil);
        CASE(boolean);
        CASE(byte);
        CASE(character);
        CASE(integer);
        CASE(real);
        CASE(string);
        CASE(symbol);
        CASE(list);
        CASE(map);
        CASE(callable);
        default: return os;
    }
#undef CASE
    return os;
}

struct list_t : public std::vector<value_t>
{
    using base_t = std::vector<value_t>;
    using base_t::base_t;

    friend std::ostream& operator<<(std::ostream& os, const list_t& item);
};

struct map_t : public std::vector<std::tuple<value_t, value_t>>
{
    using base_t = std::vector<std::tuple<value_t, value_t>>;
    using base_t::base_t;

    friend std::ostream& operator<<(std::ostream& os, const map_t& item);
};

struct callable_t : public std::function<value_t(const std::vector<value_t>&)>
{
    using base_t = std::function<value_t(const std::vector<value_t>&)>;
    using base_t::base_t;

    friend std::ostream& operator<<(std::ostream& os, const callable_t& item);
};

namespace detail
{
struct ostream_visitor
{
    std::ostream& m_os;

    void operator()(const nil_t& v) const
    {
        m_os << v;
    }
    void operator()(const boolean_t& v) const
    {
        m_os << (v ? "true" : "false");
    }
    void operator()(const byte_t& v) const
    {
        m_os << v;
    }
    void operator()(const character_t& v) const
    {
        m_os << v;
    }
    void operator()(const integer_t& v) const
    {
        m_os << v;
    }
    void operator()(const real_t& v) const
    {
        m_os << v;
    }
    void operator()(const string_t& v) const
    {
        m_os << '"' << v << '"';
    }
    void operator()(const symbol_t& v) const
    {
        m_os << v;
    }
    void operator()(const list_t& v) const
    {
        m_os << v;
    }
    void operator()(const map_t& v) const
    {
        m_os << v;
    }
    void operator()(const callable_t& v) const
    {
        m_os << v;
    }
};

struct eq_visitor
{
    bool operator()(const nil_t&, const nil_t&) const
    {
        return true;
    }
    bool operator()(const boolean_t& lhs, const boolean_t& rhs) const
    {
        return lhs == rhs;
    }
    bool operator()(const byte_t& lhs, const byte_t& rhs) const
    {
        return lhs == rhs;
    }
    bool operator()(const character_t& lhs, const character_t& rhs) const
    {
        return lhs == rhs;
    }
    bool operator()(const integer_t& lhs, const integer_t& rhs) const
    {
        return lhs == rhs;
    }
    bool operator()(const real_t& lhs, const real_t& rhs) const
    {
        return std::abs(lhs - rhs) < std::numeric_limits<real_t>::epsilon();
    }
    bool operator()(const string_t& lhs, const string_t& rhs) const
    {
        return lhs == rhs;
    }
    bool operator()(const symbol_t& lhs, const symbol_t& rhs) const
    {
        return lhs == rhs;
    }
    bool operator()(const list_t& lhs, const list_t& rhs) const
    {
        return lhs == rhs;
    }
    bool operator()(const map_t& lhs, const map_t& rhs) const
    {
        return lhs == rhs;
    }
    bool operator()(const callable_t&, const callable_t&) const
    {
        return false;
    }

    template <class L, class R>
    bool operator()(const L&, const R&) const
    {
        return false;
    }
};

}  // namespace detail

struct value_t
{
    detail::value_impl_t m_impl;

    value_t(nil_t) : m_impl{ std::in_place_type<nil_t>, nil_t{} }
    {
    }

    value_t(boolean_t v) : m_impl{ std::in_place_type<boolean_t>, v }
    {
    }

    value_t(byte_t v) : m_impl{ std::in_place_type<byte_t>, v }
    {
    }

    value_t(character_t v) : m_impl{ std::in_place_type<character_t>, v }
    {
    }

    value_t(integer_t v) : m_impl{ std::in_place_type<integer_t>, v }
    {
    }

    value_t(real_t v) : m_impl{ std::in_place_type<real_t>, v }
    {
    }

    value_t(string_t v) : m_impl{ std::in_place_type<string_t>, std::move(v) }
    {
    }

    value_t(symbol_t v) : m_impl{ std::in_place_type<symbol_t>, std::move(v) }
    {
    }

    value_t(list_t v) : m_impl{ std::in_place_type<list_t>, std::move(v) }
    {
    }

    value_t(map_t v) : m_impl{ std::in_place_type<map_t>, std::move(v) }
    {
    }

    value_t(callable_t v) : m_impl{ std::in_place_type<callable_t>, std::move(v) }
    {
    }

    value_t(const char* v) : value_t(std::string{ v })
    {
    }

    value_t() : value_t{ nil }
    {
    }

    value_t(const value_t&) = default;
    value_t(value_t&&) noexcept = default;

    value_t& operator=(value_t other)
    {
        std::swap(m_impl, other.m_impl);
        return *this;
    }

    type_t type() const
    {
        if (std::holds_alternative<nil_t>(m_impl))
        {
            return type_t::nil;
        }
        if (std::holds_alternative<boolean_t>(m_impl))
        {
            return type_t::boolean;
        }
        if (std::holds_alternative<byte_t>(m_impl))
        {
            return type_t::byte;
        }
        if (std::holds_alternative<character_t>(m_impl))
        {
            return type_t::character;
        }
        if (std::holds_alternative<integer_t>(m_impl))
        {
            return type_t::integer;
        }
        if (std::holds_alternative<real_t>(m_impl))
        {
            return type_t::real;
        }
        if (std::holds_alternative<string_t>(m_impl))
        {
            return type_t::string;
        }
        if (std::holds_alternative<symbol_t>(m_impl))
        {
            return type_t::symbol;
        }
        if (std::holds_alternative<list_t>(m_impl))
        {
            return type_t::list;
        }
        if (std::holds_alternative<map_t>(m_impl))
        {
            return type_t::map;
        }
        if (std::holds_alternative<callable_t>(m_impl))
        {
            return type_t::callable;
        }
        throw std::runtime_error{ "unhandled type" };
    }

    const nil_t* if_nil() const
    {
        return std::get_if<nil_t>(&m_impl);
    }
    const boolean_t* if_boolean() const
    {
        return std::get_if<boolean_t>(&m_impl);
    }
    const byte_t* if_byte() const
    {
        return std::get_if<byte_t>(&m_impl);
    }
    const character_t* if_character() const
    {
        return std::get_if<character_t>(&m_impl);
    }
    const integer_t* if_integer() const
    {
        return std::get_if<integer_t>(&m_impl);
    }
    const real_t* if_real() const
    {
        return std::get_if<real_t>(&m_impl);
    }
    const string_t* if_string() const
    {
        return std::get_if<string_t>(&m_impl);
    }
    const symbol_t* if_symbol() const
    {
        return std::get_if<symbol_t>(&m_impl);
    }
    const list_t* if_list() const
    {
        return std::get_if<list_t>(&m_impl);
    }
    const map_t* if_map() const
    {
        return std::get_if<map_t>(&m_impl);
    }
    const callable_t* if_callable() const
    {
        return std::get_if<callable_t>(&m_impl);
    }

    bool is_nil() const
    {
        return std::holds_alternative<nil_t>(m_impl);
    }
    bool is_boolean() const
    {
        return std::holds_alternative<boolean_t>(m_impl);
    }
    bool is_byte() const
    {
        return std::holds_alternative<byte_t>(m_impl);
    }
    bool is_character() const
    {
        return std::holds_alternative<character_t>(m_impl);
    }
    bool is_integer() const
    {
        return std::holds_alternative<integer_t>(m_impl);
    }
    bool is_real() const
    {
        return std::holds_alternative<real_t>(m_impl);
    }
    bool is_string() const
    {
        return std::holds_alternative<string_t>(m_impl);
    }
    bool is_symbol() const
    {
        return std::holds_alternative<symbol_t>(m_impl);
    }
    bool is_list() const
    {
        return std::holds_alternative<list_t>(m_impl);
    }
    bool is_map() const
    {
        return std::holds_alternative<map_t>(m_impl);
    }
    bool is_callable() const
    {
        return std::holds_alternative<callable_t>(m_impl);
    }

    const nil_t& as_nil() const
    {
        return get_if<nil_t>(type_t::nil);
    }
    const boolean_t& as_boolean() const
    {
        return get_if<boolean_t>(type_t::boolean);
    }
    const byte_t& as_byte() const
    {
        return get_if<byte_t>(type_t::byte);
    }
    const character_t& as_character() const
    {
        return get_if<character_t>(type_t::character);
    }
    const integer_t& as_integer() const
    {
        return get_if<integer_t>(type_t::integer);
    }
    const real_t& as_real() const
    {
        return get_if<real_t>(type_t::real);
    }
    const string_t& as_string() const
    {
        return get_if<string_t>(type_t::string);
    }
    const symbol_t& as_symbol() const
    {
        return get_if<symbol_t>(type_t::symbol);
    }
    const list_t& as_list() const
    {
        return get_if<list_t>(type_t::list);
    }
    const map_t& as_map() const
    {
        return get_if<map_t>(type_t::map);
    }
    const callable_t& as_callable() const
    {
        return get_if<callable_t>(type_t::callable);
    }

    friend std::ostream& operator<<(std::ostream& os, const value_t& item)
    {
        std::visit(detail::ostream_visitor{ os }, item.m_impl);
        return os;
    }

    friend bool operator==(const value_t& lhs, const value_t& rhs)
    {
        return std::visit(detail::eq_visitor{}, lhs.m_impl, rhs.m_impl);
    }

    friend bool operator!=(const value_t& lhs, const value_t& rhs)
    {
        return !(lhs == rhs);
    }

private:
    template <class T>
    const T& get_if(type_t expected_type) const
    {
        const T* maybe = std::get_if<T>(&m_impl);
        if (!maybe)
        {
            throw std::runtime_error{ str("invalid type: expected ", expected_type, ", got ", type()) };
        }
        return *maybe;
    }
};

inline std::ostream& operator<<(std::ostream& os, const nil_t)
{
    return os << "nil";
}

inline std::ostream& operator<<(std::ostream& os, const list_t& item)
{
    os << "(";
    for (std::size_t i = 0; i < item.size(); ++i)
    {
        if (i != 0)
        {
            os << " ";
        }
        os << item[i];
    }
    os << ")";
    return os;
}

inline std::ostream& operator<<(std::ostream& os, const map_t& item)
{
    os << "{";
    for (std::size_t i = 0; i < item.size(); ++i)
    {
        if (i != 0)
        {
            os << " ";
        }
        os << std::get<0>(item[i]) << " " << std::get<1>(item[i]);
    }
    os << "}";
    return os;
}

inline std::ostream& operator<<(std::ostream& os, const callable_t&)
{
    return os << "<< callable >>";
}

using token_t = std::string;

namespace detail
{

struct string_view : public std::string_view
{
    using base_t = std::string_view;
    using base_t::base_t;
    using base_t::operator=;

    string_view(iterator b, iterator e)
        : base_t{ b == e ? base_t{} : base_t{ b, static_cast<size_type>(std::distance(b, e)) } }
    {
    }

    auto ssize() const -> std::ptrdiff_t
    {
        return size();
    }

    auto take(std::ptrdiff_t n) const -> string_view
    {
        return string_view(begin(), begin() + std::min(n, ssize()));
    }

    auto drop(std::ptrdiff_t n) -> string_view
    {
        return string_view(begin() + std::min(n, ssize()), end());
    }
};

struct tokenizer_fn
{
    struct result_t
    {
        token_t token;
        std::string_view remainder;

        result_t(token_t t, std::string_view r) : token(std::move(t)), remainder(r)
        {
        }

        result_t(std::string_view t, std::string_view r) : result_t{ token_t{ t }, r }
        {
        }
    };

    static auto read_quoted_string(string_view text) -> std::optional<result_t>
    {
        if (text.empty() || text[0] != '"')
        {
            return {};
        }
        token_t result = "\"";
        auto it = std::begin(text) + 1;
        while (it != std::end(text))
        {
            if (it[0] == '\\' && std::distance(it, text.end()) > 1 && it[1] == '"')
            {
                result += '"';
                it += 2;
            }
            else if (it[0] == '"')
            {
                result += *it++;
                break;
            }
            else
            {
                result += *it++;
            }
        }
        return result_t{ result, string_view(it, text.end()) };
    }

    static auto read_character(string_view text) -> std::optional<result_t>
    {
        if (text.empty() || text[0] != '\\')
        {
            return {};
        }
        return result_t{ text.drop(2), text.drop(2) };
    }

    static auto read_token(string_view text) -> std::optional<result_t>
    {
        static const auto is_bracket
            = [](char ch) { return ch == '(' || ch == ')' || ch == '[' || ch == ']' || ch == '{' || ch == '}'; };
        static const auto is_space = [](char ch) { return std::isspace(ch); };

        if (text.empty())
        {
            return {};
        }
        if (is_bracket(text[0]))
        {
            return result_t{ text.take(1), text.drop(1) };
        }
        const auto maybe_quoted_string = read_quoted_string(text);
        if (maybe_quoted_string)
        {
            return maybe_quoted_string;
        }
        const auto maybe_character = read_character(text);
        if (maybe_character)
        {
            return maybe_character;
        }

        const auto b = std::begin(text);
        const auto e = std::end(text);

        const auto iter = std::find_if(b, e, [](char ch) { return is_space(ch) || is_bracket(ch); });

        if (iter == e)
        {
            return result_t{ text, string_view(e, e) };
        }
        return is_space(*iter) ? result_t{ token_t{ b, iter }, string_view(iter + 1, e) }
                               : result_t{ token_t{ b, iter }, string_view(iter, e) };
    }

    template <class Out>
    auto operator()(string_view text, Out out) const -> Out
    {
        while (true)
        {
            const auto res = read_token(text);
            if (!res)
            {
                break;
            }
            if (!res->token.empty())
            {
                *out++ = res->token;
            }
            text = res->remainder;
        }
        return out;
    }

    auto operator()(string_view text) const -> std::vector<token_t>
    {
        std::vector<token_t> result;
        (*this)(text, std::back_inserter(result));
        return result;
    }
};

static constexpr inline auto tokenize = detail::tokenizer_fn{};

struct parser_fn
{
    static auto read_atom(const token_t& tok) -> value_t
    {
        if (tok == "nil")
        {
            return nil;
        }
        else if (tok == "#t")
        {
            return value_t{ true };
        }
        else if (tok == "#f")
        {
            return value_t{ false };
        }
        else if (tok.front() == '\"' && tok.back() == '\"')
        {
            return value_t{ string_t{ tok.begin() + 1, tok.end() - 1 } };
        }
        else if (tok.front() == '\\')
        {
            return value_t{ character_t{ tok.at(1) } };
        }
        else if (std::all_of(std::begin(tok), std::end(tok), [](char ch) { return std::isdigit(ch); }))
        {
            return value_t{ static_cast<integer_t>(std::atoi(tok.c_str())) };
        }
        else
        {
            std::stringstream ss;
            ss << tok;
            real_t res;
            ss >> res;
            if (ss)
            {
                return value_t{ res };
            }
        }
        return value_t{ symbol_t(tok.c_str()) };
    }

    static auto pop_front(std::vector<token_t>& v) -> token_t
    {
        if (v.empty())
        {
            throw std::runtime_error{ "Cannot pop from empty vector" };
        }
        token_t result = v.front();
        v.erase(std::begin(v));
        return result;
    }

    static auto read_list(std::vector<token_t>& tokens, const std::string& closing_bracket) -> list_t
    {
        auto result = list_t{};
        if (tokens.empty())
        {
            throw std::runtime_error{ "Invalid parentheses " };
        }
        while (!tokens.empty() && tokens.front() != closing_bracket)
        {
            result.push_back(read_from(tokens));
        }
        pop_front(tokens);
        return result;
    }

    static auto try_as_cons(list_t list) -> value_t
    {
        if (list.size() == 3)
        {
            if (const auto maybe_symbol = list[1].if_symbol())
            {
                if (*maybe_symbol == symbol_t{ "." })
                {
                    return list_t{ std::move(list[0]), std::move(list[2]) };
                }
            }
        }
        return list;
    }

    static auto read_hash_map(std::vector<token_t>& tokens) -> value_t
    {
        auto temp = list_t{};
        if (tokens.empty())
        {
            throw std::runtime_error{ "Invalid parentheses " };
        }
        while (!tokens.empty() && tokens.front() != "}")
        {
            temp.push_back(read_from(tokens));
        }
        pop_front(tokens);
        if (temp.size() % 2 != 0)
        {
            throw std::runtime_error{ "Number of keys/values in map should be even" };
        }

        auto result = map_t{};
        for (std::size_t i = 0; i < temp.size(); i += 2)
        {
            result.emplace_back(temp[i + 0], temp[i + 1]);
        }
        return result;
    }

    static auto read_from(std::vector<token_t>& tokens) -> value_t
    {
        if (tokens.empty())
        {
            return nil;
        }
        const token_t front = pop_front(tokens);
        if (front == "'")
        {
            return list_t{ symbol_t{ "quote" }, read_from(tokens) };
        }
        if (front == "(")
        {
            return try_as_cons(read_list(tokens, ")"));
        }
        else if (front == "[")
        {
            return try_as_cons(read_list(tokens, "]"));
        }
        else if (front == "{")
        {
            return read_hash_map(tokens);
        }
        else
        {
            return read_atom(front);
        }
    }

    auto operator()(string_view text) const -> value_t
    {
        std::vector<token_t> tokens = tokenize(text);
        return read_from(tokens);
    }
};

}  // namespace detail

static constexpr inline auto parse = detail::parser_fn{};

struct stack_t
{
    using frame_type = std::map<symbol_t, value_t>;

    frame_type frame;
    stack_t* outer;

    stack_t(frame_type frame, stack_t* outer = {}) : frame{ std::move(frame) }, outer{ outer }
    {
    }

    const value_t& insert(const symbol_t& s, const value_t& v)
    {
        frame.emplace(s, v);
        return v;
    }

    const value_t& get(const symbol_t& s) const
    {
        const auto iter = frame.find(s);
        if (iter != frame.end())
        {
            return iter->second;
        }
        if (outer)
        {
            return outer->get(s);
        }

        throw std::runtime_error{ str("unrecognized symbol '", s, "'") };
    }

    const value_t& operator[](const symbol_t& s) const
    {
        return get(s);
    }
};

namespace detail
{
struct evaluate_fn
{
    auto eval_list(const list_t& list, stack_t* stack) const -> value_t
    {
        if (list.empty())
        {
            return nil;
        }
        // if (list.size() == 3)
        // {
        //     if (list[0] == symbol_t{ "define" })
        //     {
        //         return stack->insert(list.at(1).as_symbol(), (*this)(list.at(2), stack));
        //     }
        // }

        const callable_t op = (*this)(list[0], stack).as_callable();

        std::vector<value_t> args;
        args.reserve(list.size() - 1);
        std::transform(
            list.begin() + 1,
            list.end(),
            std::back_inserter(args),
            [&](const value_t& v) -> value_t { return (*this)(v, stack); });

        return op(args);
    }

    auto eval_let(const list_t& args, const std::vector<value_t>& exprs, stack_t* stack) const -> value_t
    {
        stack_t::frame_type frame = {};
        for (const value_t& arg : args)
        {
            const auto& pair = arg.as_list();
            frame.insert({ pair.at(0).as_symbol(), (*this)(pair.at(1), stack) });
        }
        stack_t next_stack{ frame, stack };
        value_t result = {};
        for (const value_t& expr : exprs)
        {
            result = (*this)(expr, &next_stack);
        }
        return result;
    }

    auto operator()(const value_t& expr, stack_t* stack) const -> value_t
    {
        if (const symbol_t* maybe_symbol = expr.if_symbol())
        {
            return (*stack)[*maybe_symbol];
        }
        if (const list_t* maybe_list = expr.if_list())
        {
            if (maybe_list->at(0) == symbol_t{ "let" })
            {
                return eval_let(maybe_list->at(1).as_list(), { maybe_list->begin() + 2, maybe_list->end() }, stack);
            }
            return eval_list(*maybe_list, stack);
        }
        if (const map_t* maybe_map = expr.if_map())
        {
            map_t result;
            result.reserve(maybe_map->size());
            for (const auto& item : *maybe_map)
            {
                result.emplace_back(std::get<0>(item), (*this)(std::get<1>(item), stack));
            }
            return result;
        }
        return expr;
    }
};

}  // namespace detail

static constexpr inline auto evaluate = detail::evaluate_fn{};

}  // namespace lisp
}  // namespace ferrugo
