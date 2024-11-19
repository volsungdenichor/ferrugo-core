#pragma once

#include <iostream>
#include <sstream>
#include <stdexcept>

namespace ferrugo
{
namespace core
{

struct assertion_error : std::runtime_error
{
    using base_t = std::runtime_error;
    using base_t::base_t;
};

template <class E, class... Args, std::enable_if_t<std::is_constructible_v<E, Args...>, int> = 0>
E create_exception(Args&&... args)
{
    return E{ std::forward<Args>(args)... };
}

template <
    class E,
    class... Args,
    std::enable_if_t<!std::is_constructible_v<E, Args...> && std::is_constructible_v<E, std::string>, int> = 0>
E create_exception(Args&&... args)
{
    std::stringstream ss;
    (ss << ... << std::forward<Args>(args));
    return E{ ss.str() };
}

template <class E, class... Args>
void raise(Args&&... args)
{
    if (std::current_exception())
    {
        std::throw_with_nested(create_exception<E>(std::forward<Args>(args)...));
    }
    else
    {
        throw create_exception<E>(std::forward<Args>(args)...);
    }
}

template <class E = assertion_error, class... Args>
void assert(bool condition, Args&&... args)
{
    if (!condition)
    {
        raise<E>(std::forward<Args>(args)...);
    }
}

struct exception_proxy
{
    std::exception_ptr m_ex;

    explicit exception_proxy(std::exception_ptr ex) : m_ex(std::move(ex))
    {
    }

    exception_proxy() : exception_proxy(std::current_exception())
    {
    }

    static void print_exception(std::ostream& os, const std::exception& ex, int level = 0, int tab_size = 2)
    {
        os << std::string(level * tab_size, ' ') << ex.what() << '\n';
        try
        {
            std::rethrow_if_nested(ex);
        }
        catch (const std::exception& nested)
        {
            print_exception(os, nested, level + 1, tab_size);
        }
        catch (...)
        {
        }
    }

    friend std::ostream& operator<<(std::ostream& os, const exception_proxy& item)
    {
        if (!item.m_ex)
        {
            return os;
        }

        try
        {
            std::rethrow_exception(item.m_ex);
        }
        catch (const std::exception& ex)
        {
            print_exception(os, ex);
        }
        catch (const char* ex)
        {
            os << ex << '\n';
        }
        catch (...)
        {
            os << "unknown exception" << '\n';
        }
        return os;
    }
};

}  // namespace core
}  // namespace ferrugo
