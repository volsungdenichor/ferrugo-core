#pragma once

#include <cassert>
#include <cstdint>
#include <functional>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <variant>
#include <vector>

struct sexpr
{
    class symbol : public std::string
    {
        using base_t = std::string;
        using base_t::base_t;
    };

    using nil_type = std::monostate;
    using string_type = std::string;
    using symbol_type = symbol;
    using integer_type = std::int32_t;
    using real_type = double;
    using boolean_type = bool;

    using list_type = std::vector<sexpr>;

    struct callable_type
    {
        using function_t = std::function<sexpr(const std::vector<sexpr>&)>;
        std::string_view name;
        function_t fn;

        callable_type(std::string_view name, function_t fn);

        sexpr call(const std::vector<sexpr>& args) const;
        sexpr operator()(const std::vector<sexpr>& args) const;
    };

    struct atom_type
    {
        using value_type = std::variant<nil_type, string_type, symbol_type, integer_type, real_type, boolean_type>;

        atom_type();
        atom_type(string_type v);
        atom_type(const char* v);
        atom_type(symbol_type v);
        atom_type(integer_type v);
        atom_type(real_type v);
        atom_type(boolean_type v);

        friend bool operator==(const atom_type& lhs, const atom_type& rhs);
        friend bool operator!=(const atom_type& lhs, const atom_type& rhs);

        template <class T>
        const T* get_if() const
        {
            return std::get_if<T>(&value);
        }

        template <class T>
        const T& get() const
        {
            return std::get<T>(value);
        }

        template <class T>
        bool is() const
        {
            return get_if<T>();
        }

        friend std::ostream& operator<<(std::ostream& os, const atom_type& item);

        value_type value;
    };

    sexpr();
    sexpr(string_type v);
    sexpr(symbol_type v);
    sexpr(const char* v);
    sexpr(integer_type v);
    sexpr(real_type v);
    sexpr(boolean_type v);
    sexpr(atom_type v);

    sexpr(const sexpr&) = default;
    sexpr(sexpr&&) = default;

    sexpr& operator=(sexpr other);

    static auto list(list_type v) -> sexpr;

    template <class... Tail>
    static auto list(sexpr head, Tail&&... tail) -> sexpr
    {
        return list(list_type{ std::move(head), std::forward<Tail>(tail)... });
    }

    static auto list() -> sexpr;

    static auto callable(std::string_view name, callable_type::function_t fn) -> sexpr;

    template <class T>
    const T* get_if() const
    {
        return std::get_if<T>(&m_data);
    }

    template <class T>
    const T& get() const
    {
        return std::get<T>(m_data);
    }

    template <class T>
    bool is() const
    {
        return get_if<T>();
    }

    friend bool operator==(const sexpr& lhs, const sexpr& rhs);
    friend bool operator!=(const sexpr& lhs, const sexpr& rhs);

    friend std::ostream& operator<<(std::ostream& os, const sexpr& item);

private:
    using data_type = std::variant<atom_type, callable_type, list_type>;
    sexpr(data_type data);
    data_type m_data;
};

struct format_visitor
{
    std::ostream& os;

    void operator()(const sexpr::nil_type&) const
    {
        os << "nil";
    }

    void operator()(const sexpr::integer_type& item) const
    {
        os << item;
    }

    void operator()(const sexpr::real_type& item) const
    {
        os << std::fixed << item;
    }

    void operator()(const sexpr::boolean_type& item) const
    {
        os << (item ? "#t" : "#f");
    }

    void operator()(const sexpr::string_type& item) const
    {
        os << std::quoted(item);
    }

    void operator()(const sexpr::symbol_type& item) const
    {
        os << item;
    }

    void operator()(const sexpr::atom_type& item) const
    {
        std::visit(std::ref(*this), item.value);
    }

    void operator()(const sexpr::callable_type& item) const
    {
        os << item.name;
    }

    void operator()(const sexpr::list_type& item) const
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
    }
};

struct eq_visitor
{
    bool operator()(const sexpr::integer_type& lhs, const sexpr::integer_type& rhs) const
    {
        return lhs == rhs;
    }

    bool operator()(const sexpr::real_type& lhs, const sexpr::real_type& rhs) const
    {
        return std::abs(lhs - rhs) < std::numeric_limits<sexpr::real_type>::epsilon();
    }

    bool operator()(const sexpr::boolean_type& lhs, const sexpr::boolean_type& rhs) const
    {
        return lhs == rhs;
    }

    bool operator()(const sexpr::string_type& lhs, const sexpr::string_type& rhs) const
    {
        return lhs == rhs;
    }

    bool operator()(const sexpr::symbol_type& lhs, const sexpr::symbol_type& rhs) const
    {
        return lhs == rhs;
    }

    bool operator()(const sexpr::atom_type& lhs, const sexpr::atom_type& rhs) const
    {
        return std::visit(std::ref(*this), lhs.value, rhs.value);
    }

    bool operator()(const sexpr::callable_type& lhs, const sexpr::callable_type& rhs) const
    {
        return lhs.name == rhs.name;
    }

    bool operator()(const sexpr::list_type& lhs, const sexpr::list_type& rhs) const
    {
        if (lhs.size() != rhs.size())
        {
            return false;
        }
        for (std::size_t i = 0; i < lhs.size(); ++i)
        {
            if (lhs[i] != rhs[i])
            {
                return false;
            }
        }
        return true;
    }

    template <class L, class R>
    bool operator()(const L&, const R&) const
    {
        return false;
    }
};

sexpr::callable_type::callable_type(std::string_view name, function_t fn) : name(name), fn(std::move(fn))
{
}

sexpr sexpr::callable_type::call(const std::vector<sexpr>& args) const
{
    return fn(args);
}

sexpr sexpr::callable_type::operator()(const std::vector<sexpr>& args) const
{
    return call(args);
}

bool operator==(const sexpr::atom_type& lhs, const sexpr::atom_type& rhs)
{
    return std::visit(eq_visitor{}, lhs.value, rhs.value);
}

bool operator!=(const sexpr::atom_type& lhs, const sexpr::atom_type& rhs)
{
    return !(lhs == rhs);
}

std::ostream& operator<<(std::ostream& os, const sexpr::atom_type& item)
{
    std::visit(format_visitor{ os }, item.value);
    return os;
}

sexpr::atom_type::atom_type() : value{ std::monostate{} }
{
}

sexpr::atom_type::atom_type(string_type v) : value{ std::move(v) }
{
}

sexpr::atom_type::atom_type(const char* v) : atom_type{ std::string(v) }
{
}

sexpr::atom_type::atom_type(symbol_type v) : value{ std::move(v) }
{
}

sexpr::atom_type::atom_type(integer_type v) : value{ std::move(v) }
{
}

sexpr::atom_type::atom_type(real_type v) : value{ std::move(v) }
{
}

sexpr::atom_type::atom_type(boolean_type v) : value{ std::move(v) }
{
}

sexpr::sexpr() : sexpr(atom_type{})
{
}

sexpr::sexpr(atom_type v) : m_data(std::in_place_type<atom_type>, std::move(v))
{
}

sexpr::sexpr(data_type data) : m_data(std::move(data))
{
}

sexpr::sexpr(string_type v) : sexpr(atom_type{ std::move(v) })
{
}

sexpr::sexpr(symbol_type v) : sexpr(atom_type{ std::move(v) })
{
}

sexpr::sexpr(const char* v) : sexpr(string_type(v))
{
}

sexpr::sexpr(integer_type v) : sexpr(atom_type{ std::move(v) })
{
}

sexpr::sexpr(real_type v) : sexpr(atom_type{ std::move(v) })
{
}

sexpr::sexpr(boolean_type v) : sexpr(atom_type{ std::move(v) })
{
}

sexpr& sexpr::operator=(sexpr other)
{
    std::swap(m_data, other.m_data);
    return *this;
}

auto sexpr::list(list_type v) -> sexpr
{
    return sexpr(data_type{ std::in_place_type<list_type>, std::move(v) });
}

auto sexpr::list() -> sexpr
{
    return list(list_type{});
}

auto sexpr::callable(std::string_view name, callable_type::function_t fn) -> sexpr
{
    return sexpr(data_type{ std::in_place_type<callable_type>, callable_type{ name, std::move(fn) } });
}

bool operator==(const sexpr& lhs, const sexpr& rhs)
{
    return std::visit(eq_visitor{}, lhs.m_data, rhs.m_data);
}

bool operator!=(const sexpr& lhs, const sexpr& rhs)
{
    return !(lhs == rhs);
}

std::ostream& operator<<(std::ostream& os, const sexpr& item)
{
    std::visit(format_visitor{ os }, item.m_data);
    return os;
}

inline auto operator""_sym(const char* ptr, std::size_t size) -> sexpr::symbol
{
    return sexpr::symbol(std::string(ptr, size).c_str());
}

struct stack_t
{
    using frame_t = std::map<sexpr::symbol_type, sexpr>;
    frame_t frame;
    stack_t* outer;

    const sexpr& insert(sexpr::symbol_type s, const sexpr& v)
    {
        frame.emplace(std::move(s), v);
        return v;
    }

    const sexpr& get(const sexpr::symbol_type& s) const
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

        throw std::runtime_error{ "Unrecognized symbol '" + s + "'" };
    }

    const sexpr& operator[](const sexpr::symbol_type& s) const
    {
        return get(s);
    }
};

struct eval_fn
{
    sexpr operator()(const std::vector<sexpr>& lst, stack_t* stack) const
    {
        sexpr result;
        for (const sexpr& e : lst)
        {
            result = (*this)(e, stack);
        }
        return result;
    }

    sexpr operator()(const sexpr& e, stack_t* stack) const
    {
        if (const auto atom = e.get_if<sexpr::atom_type>())
        {
            if (const auto symbol = atom->get_if<sexpr::symbol>())
            {
                return (*stack)[*symbol];
            }
        }
        else if (const auto list = e.get_if<sexpr::list_type>())
        {
            if (list->size() == 3 && list->at(0) == sexpr::symbol{ "define" })
            {
                return stack->insert(list->at(1).get<sexpr::atom_type>().get<sexpr::symbol>(), (*this)(list->at(2), stack));
            }
            if (list->size() >= 3 && list->at(0) == sexpr::symbol{ "lambda" })
            {
                const sexpr::list_type& params = list->at(1).get<sexpr::list_type>();
                const auto body = std::vector<sexpr>(std::begin(*list) + 2, std::end(*list));
                const sexpr::callable_type::function_t lambda = [=](const std::vector<sexpr>& args) -> sexpr
                {
                    auto new_frame = stack_t::frame_t{};
                    for (std::size_t i = 0; i < params.size(); ++i)
                    {
                        new_frame.emplace(params.at(i).get<sexpr::atom_type>().get<sexpr::symbol>(), args.at(i));
                    }

                    auto new_stack = stack_t{ new_frame, stack };
                    return (*this)(body, &new_stack);
                };
                return sexpr::callable("lambda", std::move(lambda));
            }
            const sexpr op = (*this)(list->at(0), stack);
            std::vector<sexpr> arg_values;
            arg_values.reserve(list->size() - 1);
            std::transform(
                std::begin(*list) + 1,
                std::end(*list),
                std::back_inserter(arg_values),
                [&](const sexpr& v) { return (*this)(v, stack); });
            const auto& callable = op.get<sexpr::callable_type>();
            return callable(arg_values);
        }
        return e;
    }
};

static constexpr inline auto eval = eval_fn{};

template <class Op>
struct binary_op
{
    static auto op(const sexpr::atom_type& lhs, const sexpr::atom_type& rhs) -> sexpr::atom_type
    {
        static constexpr auto oper = Op{};

        {
            const auto lt = lhs.get_if<sexpr::integer_type>();
            const auto rt = rhs.get_if<sexpr::integer_type>();
            if (lt && rt)
            {
                return oper(*lt, *rt);
            }
        }
        {
            const auto lt = lhs.get_if<sexpr::real_type>();
            const auto rt = rhs.get_if<sexpr::integer_type>();
            if (lt && rt)
            {
                return oper(*lt, *rt);
            }
        }
        {
            const auto lt = lhs.get_if<sexpr::integer_type>();
            const auto rt = rhs.get_if<sexpr::real_type>();
            if (lt && rt)
            {
                return oper(*lt, *rt);
            }
        }
        throw std::runtime_error{ "Cannot" };
    }

    sexpr operator()(const sexpr& lhs, const sexpr& rhs) const
    {
        return op(lhs.get<sexpr::atom_type>(), rhs.get<sexpr::atom_type>());
    }

    sexpr operator()(const std::vector<sexpr>& args) const
    {
        return std::accumulate(std::begin(args) + 1, std::end(args), args.at(0), std::ref(*this));
    }
};

using add = binary_op<std::plus<>>;
using sub = binary_op<std::minus<>>;
using mul = binary_op<std::multiplies<>>;