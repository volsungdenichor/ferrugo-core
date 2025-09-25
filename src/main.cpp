
#include <array>
#include <cmath>
#include <cstdint>
#include <cuchar>
#include <deque>
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
#include <unordered_set>
#include <variant>
#include <vector>

namespace nested_text
{

template <class... Args>
std::string str(Args&&... args)
{
    std::stringstream ss;
    (ss << ... << std::forward<Args>(args));
    return ss.str();
}

struct value;

template <class T>
struct codec;

template <class T>
struct codec_instance
{
    static const codec<T>& get()
    {
        static const codec<T> m_instance = {};
        return m_instance;
    }
};

template <class T>
value serialize(const T& in);

template <class T>
T deserialize(const value& in);

template <class T>
void deserialize(T& out, const value& in);

using string_view_type = const char*;

struct value
{
    enum class type
    {
        empty,
        string,
        list,
        dictionary
    };

    friend std::ostream& operator<<(std::ostream& os, type item)
    {
#define CASE(x) \
    case type::x: return os << #x
        switch (item)
        {
            CASE(empty);
            CASE(string);
            CASE(list);
            CASE(dictionary);
        }
#undef CASE
        return os;
    }
    struct empty_type
    {
    };

    using string_type = std::string;

    struct list_type : public std::vector<value>
    {
        using base_type = std::vector<value>;
        using base_type::base_type;
        using base_type::operator[];

        friend std::ostream& operator<<(std::ostream& os, const list_type item)
        {
            os << "[";
            for (std::size_t i = 0; i < item.size(); ++i)
            {
                if (i != 0)
                {
                    os << ", ";
                }
                os << item[i];
            }
            os << "]";
            return os;
        }
    };
    struct dictionary_type : public std::vector<std::pair<string_type, value>>
    {
        using base_type = std::vector<std::pair<string_type, value>>;
        using base_type::base_type;
        using base_type::operator[];

        const value& operator[](const string_type& key) const
        {
            for (const auto& it : *this)
            {
                if (it.first == key)
                {
                    return it.second;
                }
            }
            throw std::runtime_error{ str("dictionary: key '", key, "' not found") };
        }

        const value& operator[](const char* key) const
        {
            return (*this)[string_type(key)];
        }

        void emplace(string_type key, value val)
        {
            base_type::emplace_back(std::move(key), std::move(val));
        }

        friend std::ostream& operator<<(std::ostream& os, const dictionary_type& item)
        {
            os << "{";
            for (std::size_t i = 0; i < item.size(); ++i)
            {
                if (i != 0)
                {
                    os << ", ";
                }
                os << item[i].first << ": " << item[i].second;
            }
            os << " }";
            return os;
        }
    };

    type m_type;
    union
    {
        empty_type m_empty;
        string_type m_string;
        list_type m_list;
        dictionary_type m_dictionary;
    };

    void reset()
    {
        switch (m_type)
        {
            case type::empty: break;
            case type::string: m_string.~string_type(); break;
            case type::list: m_list.~list_type(); break;
            case type::dictionary: m_dictionary.~dictionary_type(); break;
        }
        m_type = type::empty;
        m_empty = empty_type();
    }

    ~value()
    {
        reset();
    }

    value() : m_type(type::empty), m_empty(empty_type{})
    {
    }

    value(string_type v) : m_type(type::string)
    {
        new (&m_string) string_type(std::move(v));
    }

    value(string_view_type v) : value(string_type(v))
    {
    }

    value(list_type v) : m_type(type::list)
    {
        new (&m_list) list_type(std::move(v));
    }

    value(dictionary_type v) : m_type(type::dictionary)
    {
        new (&m_dictionary) dictionary_type(std::move(v));
    }

    value(const value& other) : m_type(other.m_type)
    {
        switch (m_type)
        {
            case type::empty: m_empty = other.m_empty; break;
            case type::string: new (&m_string) string_type(other.m_string); break;
            case type::list: new (&m_list) list_type(other.m_list); break;
            case type::dictionary: new (&m_dictionary) dictionary_type(other.m_dictionary); break;
        }
    }

    value(value&& other) noexcept : m_type(other.m_type)
    {
        switch (m_type)
        {
            case type::empty: m_empty = other.m_empty; break;
            case type::string: new (&m_string) string_type(std::move(other.m_string)); break;
            case type::list: new (&m_list) list_type(std::move(other.m_list)); break;
            case type::dictionary: new (&m_dictionary) dictionary_type(std::move(other.m_dictionary)); break;
        }
    }

    template <class T>
    value(const T& item) : value(serialize(item))
    {
    }

    value& operator=(const value& other)
    {
        reset();
        m_type = other.m_type;
        switch (m_type)
        {
            case type::empty: m_empty = other.m_empty; break;
            case type::string: new (&m_string) string_type(other.m_string); break;
            case type::list: new (&m_list) list_type(other.m_list); break;
            case type::dictionary: new (&m_dictionary) dictionary_type(other.m_dictionary); break;
        }
        return *this;
    }

    value& operator=(value&& other)
    {
        reset();
        m_type = other.m_type;
        switch (m_type)
        {
            case type::empty: m_empty = other.m_empty; break;
            case type::string: new (&m_string) string_type(std::move(other.m_string)); break;
            case type::list: new (&m_list) list_type(std::move(other.m_list)); break;
            case type::dictionary: new (&m_dictionary) dictionary_type(std::move(other.m_dictionary)); break;
        }
        return *this;
    }

    friend std::ostream& operator<<(std::ostream& os, const value& item)
    {
        switch (item.m_type)
        {
            case type::empty: os << "<< empty >>"; break;
            case type::string: os << item.m_string; break;
            case type::list: os << item.m_list; break;
            case type::dictionary: os << item.m_dictionary; break;
        }
        return os;
    }

    static std::string error_msg(type expected, type actual)
    {
        return str("value: type mismatch - expected ", expected, ", actual ", actual);
    }

    const string_type* if_string() const
    {
        return m_type == type::string ? &m_string : nullptr;
    }

    const string_type& as_string() const
    {
        return m_type == type::string ? m_string : throw std::runtime_error{ error_msg(type::string, m_type) };
    }

    const list_type* if_list() const
    {
        return m_type == type::list ? &m_list : nullptr;
    }

    const list_type& as_list() const
    {
        return m_type == type::list ? m_list : throw std::runtime_error{ error_msg(type::list, m_type) };
    }

    const dictionary_type* if_dictionary() const
    {
        return m_type == type::dictionary ? &m_dictionary : nullptr;
    }

    const dictionary_type& as_dictionary() const
    {
        return m_type == type::dictionary ? m_dictionary : throw std::runtime_error{ error_msg(type::dictionary, m_type) };
    }

    template <class T>
    operator T() const
    {
        return deserialize<T>(*this);
    }

    const value& operator[](const string_type& key) const
    {
        return as_dictionary()[key];
    }

    const value& operator[](const char* key) const
    {
        return (*this)[string_type(key)];
    }

    const value& operator[](std::size_t index) const
    {
        return as_list().at(index);
    }
};

template <class T>
value serialize(const T& in)
{
    return codec_instance<T>::get().serialize(in);
}

template <class T>
T deserialize(const value& in)
{
    return codec_instance<T>::get().deserialize(in);
}

template <class T>
void deserialize(T& out, const value& in)
{
    out = deserialize<T>(in);
}

template <>
struct codec<std::string>
{
    value serialize(const std::string& in) const
    {
        return in;
    }

    std::string deserialize(const value& in) const
    {
        return in.as_string();
    }
};

template <class T>
struct as_string_codec
{
    value serialize(const T& in) const
    {
        std::stringstream ss;
        ss << in;
        return value::string_type{ ss.str() };
    }

    T deserialize(const value& in) const
    {
        std::stringstream ss;
        ss << in.as_string();
        T res;
        ss >> res;
        if (!ss)
        {
            throw std::runtime_error{ str("cannot decode ", in.as_string(), " to ", typeid(T).name()) };
        }
        return res;
    }
};

template <>
struct codec<int> : as_string_codec<int>
{
};

template <>
struct codec<char>
{
    value serialize(char in) const
    {
        return std::string(1, in);
    }

    char deserialize(const value& in) const
    {
        return in.as_string().at(0);
    }
};

template <>
struct codec<bool>
{
    value serialize(bool in) const
    {
        return in ? "true" : "false";
    }

    bool deserialize(const value& in) const
    {
        const auto& s = in.as_string();
        if (s == "true")
        {
            return true;
        }
        else if (s == "false")
        {
            return true;
        }
        throw std::runtime_error{ str("cannot decode ", s, " to boolean") };
    }
};

template <class T>
struct codec<std::vector<T>>
{
    value serialize(const std::vector<T>& in) const
    {
        value::list_type out;
        out.reserve(in.size());
        for (const T& item : in)
        {
            out.push_back(nested_text::serialize(item));
        }
        return out;
    }

    std::vector<T> deserialize(const value& in) const
    {
        const auto& lst = in.as_list();
        std::vector<T> out;
        out.reserve(lst.size());
        for (const auto& v : lst)
        {
            out.push_back(nested_text::deserialize<T>(v));
        }
        return out;
    }
};

template <class T>
struct codec<std::set<T>>
{
    value serialize(const std::set<T>& in) const
    {
        value::list_type out;
        out.reserve(in.size());
        for (const T& item : in)
        {
            out.push_back(nested_text::serialize(item));
        }
        return out;
    }

    std::set<T> deserialize(const value& in) const
    {
        const auto& lst = in.as_list();
        std::set<T> out;
        for (const auto& v : lst)
        {
            out.insert(nested_text::deserialize<T>(v));
        }
        return out;
    }
};

template <class K, class V>
struct codec<std::map<K, V>>
{
    value serialize(const std::map<K, V>& in) const
    {
        value::dictionary_type out;
        for (const auto& item : in)
        {
            out.emplace(nested_text::serialize(item.first).as_string(), nested_text::serialize(item.second));
        }
        return out;
    }

    std::map<K, V> deserialize(const value& in) const
    {
        const auto& dct = in.as_dictionary();
        std::map<K, V> out;
        for (const auto& item : dct)
        {
            out.insert({ nested_text::deserialize<K>(item.first), nested_text::deserialize<V>(item.second) });
        }
        return out;
    }
};

template <class E>
struct enum_codec
{
    std::vector<std::pair<E, string_view_type>> m_values;

    enum_codec(std::vector<std::pair<E, string_view_type>> values) : m_values(std::move(values))
    {
    }

    value serialize(E in) const
    {
        for (const auto& pair : m_values)
        {
            if (pair.first == in)
            {
                return value::string_type{ pair.second };
            }
        }
        throw std::runtime_error{ "unregistered value" };
    }

    E deserialize(const value& in) const
    {
        const auto& s = in.as_string();
        for (const auto& pair : m_values)
        {
            if (pair.second == s)
            {
                return pair.first;
            }
        }
        throw std::runtime_error{ str("On deserializing enum: unknown value '", s, "'") };
    }
};

template <class T>
struct struct_codec
{
    struct field_info
    {
        string_view_type name;
        std::function<void(value::dictionary_type&, const T&, string_view_type n)> serialize;
        std::function<void(T&, const value::dictionary_type&, string_view_type n)> deserialize;

        template <class Type>
        field_info(string_view_type n, Type T::*field)
            : name(n)
            , serialize{ [=](value::dictionary_type& out, const T& in, string_view_type n)
                         { out.emplace(n, nested_text::serialize(in.*field)); } }
            , deserialize{ [=](T& out, const value::dictionary_type& in, string_view_type n)
                           { nested_text::deserialize(out.*field, in[n]); } }
        {
        }
    };

    std::vector<field_info> m_fields;

    struct_codec(std::vector<field_info> fields) : m_fields(std::move(fields))
    {
    }

    value serialize(const T& in) const
    {
        value::dictionary_type dct;
        for (const auto& field : m_fields)
        {
            field.serialize(dct, in, field.name);
        }
        return dct;
    }

    T deserialize(const value& in) const
    {
        const auto& dct = in.as_dictionary();
        T res = {};
        for (const auto& field : m_fields)
        {
            field.deserialize(res, dct, field.name);
        }
        return res;
    }
};

}  // namespace nested_text

enum class Sex
{
    male,
    female
};

std::ostream& operator<<(std::ostream& os, Sex item)
{
    switch (item)
    {
        case Sex::male: return os << "male";
        case Sex::female: return os << "female";
    }
    return os;
}
struct Person
{
    std::string name;
    Sex sex;
    int age;

    friend std::ostream& operator<<(std::ostream& os, const Person& item)
    {
        return os << item.name << " " << item.sex << " " << item.age;
    }
};

template <>
struct nested_text::codec<Sex> : enum_codec<Sex>
{
    codec() : enum_codec{ { { Sex::male, "male" }, { Sex::female, "female" } } }
    {
    }
};

template <>
struct nested_text::codec<Person> : struct_codec<Person>
{
    codec()
        : struct_codec({
            { "name", &Person::name },
            { "sex", &Person::sex },
            { "age", &Person::age },
        })
    {
    }
};

int run(const std::vector<std::string_view>& args)
{
    const auto p = std::vector{ Person{ "ala", Sex::female, 45 }, Person{ "jas", Sex::male, 42 } };
    std::cout << nested_text::value(p) << "\n";
    std::cout << nested_text::value(p)[1]["name"] << std::endl;

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
