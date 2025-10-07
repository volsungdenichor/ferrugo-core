
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
value encode(const T& in);

template <class T>
T decode(const value& in);

template <class T>
void decode(T& out, const value& in);

struct value
{
    enum class type
    {
        string,
        array,
        map
    };

    friend std::ostream& operator<<(std::ostream& os, type item)
    {
#define CASE(x) \
    case type::x: return os << #x
        switch (item)
        {
            CASE(string);
            CASE(array);
            CASE(map);
        }
#undef CASE
        return os;
    }

    using string_type = std::string;

    struct array : public std::vector<value>
    {
        using base_type = std::vector<value>;
        using base_type::base_type;
        using base_type::operator[];

        friend std::ostream& operator<<(std::ostream& os, const array item)
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
    struct map : public std::vector<std::pair<string_type, value>>
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
            throw std::runtime_error{ str("map: key '", key, "' not found") };
        }

        const value& operator[](const char* key) const
        {
            return (*this)[string_type(key)];
        }

        void emplace(string_type key, value val)
        {
            base_type::emplace_back(std::move(key), std::move(val));
        }

        friend std::ostream& operator<<(std::ostream& os, const map& item)
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
            os << "}";
            return os;
        }
    };

    type m_type;
    union
    {
        string_type m_string;
        array m_array;
        map m_map;
    };

    void reset()
    {
        do_reset();
        m_type = type::string;
        new (&m_string) string_type();
    }

    ~value()
    {
        do_reset();
    }

    value() : value(string_type{})
    {
    }

    value(string_type v) : m_type(type::string), m_string(std::move(v))
    {
    }

    value(const char* v) : value(string_type(v))
    {
    }

    value(array v) : m_type(type::array), m_array(std::move(v))
    {
    }

    value(map v) : m_type(type::map), m_map(std::move(v))
    {
    }

    value(const value& other) : m_type(other.m_type)
    {
        do_emplace(other);
    }

    value(value&& other) noexcept : m_type(other.m_type)
    {
        do_emplace(std::move(other));
    }

    value& operator=(const value& other)
    {
        do_reset();
        do_emplace(other);
        return *this;
    }

    value& operator=(value&& other)
    {
        do_reset();
        do_emplace(std::move(other));
        return *this;
    }

    friend std::ostream& operator<<(std::ostream& os, const value& item)
    {
        switch (item.m_type)
        {
            case type::string: os << item.m_string; break;
            case type::array: os << item.m_array; break;
            case type::map: os << item.m_map; break;
        }
        return os;
    }

    const string_type* if_string() const
    {
        return m_type == type::string ? &m_string : nullptr;
    }

    const string_type& as_string() const
    {
        return m_type == type::string ? m_string : throw std::runtime_error{ error_msg(type::string, m_type) };
    }

    const array* if_array() const
    {
        return m_type == type::array ? &m_array : nullptr;
    }

    const array& as_array() const
    {
        return m_type == type::array ? m_array : throw std::runtime_error{ error_msg(type::array, m_type) };
    }

    const map* if_map() const
    {
        return m_type == type::map ? &m_map : nullptr;
    }

    const map& as_map() const
    {
        return m_type == type::map ? m_map : throw std::runtime_error{ error_msg(type::map, m_type) };
    }

    const value& operator[](const string_type& key) const
    {
        return as_map()[key];
    }

    const value& operator[](const char* key) const
    {
        return (*this)[string_type(key)];
    }

    template <class I, class = std::enable_if_t<std::is_integral_v<I>>>
    const value& operator[](I index) const
    {
        return as_array().at(static_cast<std::size_t>(index));
    }

private:
    template <class T>
    static void destroy(T& item)
    {
        item.~T();
    }

    template <class T>
    static void copy(T& dst, const T& src)
    {
        new (&dst) T(src);
    }

    template <class T>
    static void move(T& dst, T&& src)
    {
        new (&dst) T(std::move(src));
    }

    void do_reset()
    {
        switch (m_type)
        {
            case type::string: destroy(m_string); break;
            case type::array: destroy(m_array); break;
            case type::map: destroy(m_map); break;
        }
    }

    void do_emplace(const value& other)
    {
        m_type = other.m_type;
        switch (other.m_type)
        {
            case type::string: copy(m_string, other.m_string); break;
            case type::array: copy(m_array, other.m_array); break;
            case type::map: copy(m_map, other.m_map); break;
        }
    }

    void do_emplace(value&& other)
    {
        m_type = other.m_type;
        switch (other.m_type)
        {
            case type::string: move(m_string, std::move(other.m_string)); break;
            case type::array: move(m_array, std::move(other.m_array)); break;
            case type::map: move(m_map, std::move(other.m_map)); break;
        }
    }

    static std::string error_msg(type expected, type actual)
    {
        return str("value: type mismatch - expected ", expected, ", actual ", actual);
    }
};

template <class T>
value encode(const T& in)
{
    return codec_instance<T>::get().encode(in);
}

template <class T>
T decode(const value& in)
{
    return codec_instance<T>::get().decode(in);
}

template <class T>
void decode(T& out, const value& in)
{
    out = decode<T>(in);
}

template <>
struct codec<std::string>
{
    value encode(const std::string& in) const
    {
        return in;
    }

    std::string decode(const value& in) const
    {
        return in.as_string();
    }
};

template <class T>
struct as_string_codec
{
    value encode(const T& in) const
    {
        std::stringstream ss;
        ss << in;
        return value::string_type{ ss.str() };
    }

    T decode(const value& in) const
    {
        std::stringstream ss;
        const value::string_type& s = in.as_string();
        ss << s;
        T res;
        ss >> res;
        if (!ss)
        {
            throw std::runtime_error{ str("cannot decode '", s, "' to type ", typeid(T).name()) };
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
    value encode(char in) const
    {
        return std::string(1, in);
    }

    char decode(const value& in) const
    {
        return in.as_string().at(0);
    }
};

template <>
struct codec<bool>
{
    value encode(bool in) const
    {
        return in ? "true" : "false";
    }

    bool decode(const value& in) const
    {
        const auto& s = in.as_string();
        if (s == "true")
        {
            return true;
        }
        else if (s == "false")
        {
            return false;
        }
        throw std::runtime_error{ str("cannot decode ", s, " to boolean") };
    }
};

template <class T>
struct codec<std::vector<T>>
{
    value encode(const std::vector<T>& in) const
    {
        value::array out;
        out.reserve(in.size());
        for (const T& item : in)
        {
            out.push_back(nested_text::encode(item));
        }
        return out;
    }

    std::vector<T> decode(const value& in) const
    {
        const auto& a = in.as_array();
        std::vector<T> out;
        out.reserve(a.size());
        for (const value& v : a)
        {
            out.push_back(nested_text::decode<T>(v));
        }
        return out;
    }
};

template <class T>
struct codec<std::set<T>>
{
    value encode(const std::set<T>& in) const
    {
        value::array out;
        out.reserve(in.size());
        for (const T& item : in)
        {
            out.push_back(nested_text::encode(item));
        }
        return out;
    }

    std::set<T> decode(const value& in) const
    {
        const auto& a = in.as_array();
        std::set<T> out;
        for (const value& v : a)
        {
            out.insert(nested_text::decode<T>(v));
        }
        return out;
    }
};

template <class K, class V>
struct codec<std::map<K, V>>
{
    value encode(const std::map<K, V>& in) const
    {
        value::map out;
        for (const auto& item : in)
        {
            out.emplace(nested_text::encode(item.first).as_string(), nested_text::encode(item.second));
        }
        return out;
    }

    std::map<K, V> decode(const value& in) const
    {
        const auto& m = in.as_map();
        std::map<K, V> out;
        for (const auto& item : m)
        {
            out.insert({ nested_text::decode<K>(item.first), nested_text::decode<V>(item.second) });
        }
        return out;
    }
};

template <class E>
struct enum_codec
{
    std::vector<std::pair<E, std::string>> m_values;

    enum_codec(std::vector<std::pair<E, std::string>> values) : m_values(std::move(values))
    {
    }

    value encode(E in) const
    {
        for (const auto& pair : m_values)
        {
            if (pair.first == in)
            {
                return value::string_type{ pair.second };
            }
        }
        throw std::runtime_error{ str("On encoding enum: unregistered value") };
    }

    E decode(const value& in) const
    {
        const auto& s = in.as_string();
        for (const auto& pair : m_values)
        {
            if (pair.second == s)
            {
                return pair.first;
            }
        }
        throw std::runtime_error{ str("On decoding enum: unknown value '", s, "'") };
    }
};

template <class T>
struct struct_codec
{
    struct field_info
    {
        std::string name;
        std::function<void(value::map&, const T&, const std::string&)> encode;
        std::function<void(T&, const value::map&, const std::string&)> decode;

        template <class Type>
        field_info(std::string n, Type T::*field)
            : name(std::move(n))
            , encode{ [=](value::map& out, const T& in, const std::string& n)
                      { out.emplace(n, nested_text::encode(in.*field)); } }
            , decode{ [=](T& out, const value::map& in, const std::string& n) { nested_text::decode(out.*field, in[n]); } }
        {
        }
    };

    std::vector<field_info> m_fields;

    struct_codec(std::vector<field_info> fields) : m_fields(std::move(fields))
    {
    }

    value encode(const T& in) const
    {
        value::map dct;
        for (const field_info& field : m_fields)
        {
            try
            {
                field.encode(dct, in, field.name);
            }
            catch (const std::exception& ex)
            {
                throw std::runtime_error{ str("On encoding field '", field.name, "': ", ex.what()) };
            }
        }
        return dct;
    }

    T decode(const value& in) const
    {
        static_assert(std::is_default_constructible_v<T>, "Default constructible type required");
        const auto& m = in.as_map();
        T res = {};
        for (const field_info& field : m_fields)
        {
            try
            {
                field.decode(res, m, field.name);
            }
            catch (const std::exception& ex)
            {
                throw std::runtime_error{ str("On decoding field '", field.name, "': ", ex.what()) };
            }
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
    nested_text::value lst = nested_text::value::array{ nested_text::encode(Person{ "Edek", Sex::male, 8 }),
                                                        nested_text::encode(Person{ "Wanda", Sex::female, 6 }) };

    std::cout << lst << "\n";

    std::cout << lst[0]["name"] << std::endl;

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
