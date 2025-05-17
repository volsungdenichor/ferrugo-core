#pragma once

#include <algorithm>
#include <functional>
#include <iostream>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <type_traits>
#include <variant>
#ifdef __GNUG__
#include <cxxabi.h>

#include <cstdlib>
#include <memory>

#endif  // __GNUG__

namespace core
{

namespace detail
{

template <class AlwaysVoid, template <class...> class Op, class... Args>
struct detector_impl : std::false_type
{
};

template <template <class...> class Op, class... Args>
struct detector_impl<std::void_t<Op<Args...>>, Op, Args...> : std::true_type
{
};

template <typename T>
constexpr bool should_pass_by_value = sizeof(T) <= 2 * sizeof(void*) && std::is_trivially_copy_constructible_v<T>;

}  // namespace detail

template <class T>
using in_t = std::conditional_t<detail::should_pass_by_value<T>, const T, const T&>;

template <class T>
using return_t = std::conditional_t<detail::should_pass_by_value<T>, T, const T&>;

template <bool C>
using require = std::enable_if_t<C, int>;

template <class T>
struct type_identity
{
    using type = T;
};

template <class T>
using type_identity_t = typename type_identity<T>::type;

template <class...>
struct always_false : std::false_type
{
};

template <template <class...> class Op, class... Args>
struct is_detected : detail::detector_impl<std::void_t<>, Op, Args...>
{
};

template <class T>
using iterator_t = decltype(std::begin(std::declval<T&>()));

template <class T>
using iter_category_t = typename std::iterator_traits<T>::iterator_category;

template <class T>
using iter_reference_t = typename std::iterator_traits<T>::reference;

template <class T>
using iter_value_t = typename std::iterator_traits<T>::value_type;

template <class T>
using iter_difference_t = typename std::iterator_traits<T>::difference_type;

template <class T>
using range_category_t = iter_category_t<iterator_t<T>>;

template <class T>
using range_reference_t = iter_reference_t<iterator_t<T>>;

template <class T>
using range_value_t = iter_value_t<iterator_t<T>>;

template <class T>
using range_difference_t = iter_difference_t<iterator_t<T>>;

namespace detail
{
template <class Category, class T>
struct iterator_of_category : std::is_base_of<Category, typename std::iterator_traits<T>::iterator_category>
{
};
}  // namespace detail

template <class T>
struct is_input_iterator : detail::iterator_of_category<std::input_iterator_tag, T>
{
};

template <class T>
struct is_forward_iterator : detail::iterator_of_category<std::forward_iterator_tag, T>
{
};

template <class T>
struct is_bidirectional_iterator : detail::iterator_of_category<std::bidirectional_iterator_tag, T>
{
};

template <class T>
struct is_random_access_iterator : detail::iterator_of_category<std::random_access_iterator_tag, T>
{
};

template <class T>
struct is_input_range : is_input_iterator<iterator_t<T>>
{
};

template <class T>
struct is_forward_range : is_forward_iterator<iterator_t<T>>
{
};

template <class T>
struct is_bidirectional_range : is_bidirectional_iterator<iterator_t<T>>
{
};

template <class T>
struct is_random_access_range : is_random_access_iterator<iterator_t<T>>
{
};

template <class Os, class T>
using has_ostream_operator_impl = decltype(std::declval<Os>() << std::declval<T>());

template <class T, class Os = std::ostream>
struct has_ostream_operator : is_detected<has_ostream_operator_impl, Os, T>
{
};

#ifdef __GNUG__

inline std::string demangle(const char* name)
{
    int status = -4;
    std::unique_ptr<char, void (*)(void*)> res{ abi::__cxa_demangle(name, NULL, NULL, &status), std::free };
    return (status == 0) ? res.get() : name;
}

#else

// does nothing if not g++
inline std::string demangle(const char* name)
{
    return name;
}

#endif  // __GNUG__

template <class T, class = void>
struct formatter;

namespace detail
{

static constexpr inline struct format_to_fn
{
    template <class T>
    using has_formatter_impl = decltype(formatter<T>{});

    template <class T>
    static void do_format(std::ostream& os, const T& item)
    {
        if constexpr (is_detected<has_formatter_impl, T>::value)
        {
            formatter<T>{}.format(os, item);
        }
        else if constexpr (has_ostream_operator<T>::value)
        {
            os << item;
        }
        else
        {
            os << "[" << demangle(typeid(T).name()) << "]";
        }
    }

    template <class... Args>
    std::ostream& operator()(std::ostream& os, const Args&... args) const
    {
        (do_format(os, args), ...);
        return os;
    }
} format_to;

static constexpr inline struct format_fn
{
    template <class... Args>
    auto operator()(const Args&... args) const -> std::string
    {
        std::stringstream ss;
        format_to(ss, args...);
        return std::move(ss).str();
    }
} format;

static constexpr inline struct print_fn
{
    template <class... Args>
    std::ostream& operator()(const Args&... args) const
    {
        return format_to(std::cout, args...);
    }
} print;

static constexpr inline struct println_fn
{
    template <class... Args>
    std::ostream& operator()(const Args&... args) const
    {
        return print(args...) << std::endl;
    }
} println;

}  // namespace detail

using detail::format;
using detail::format_to;
using detail::print;
using detail::println;
static constexpr inline auto str = detail::format_fn{};

template <>
struct formatter<std::exception_ptr>
{
    void format(std::ostream& os, const std::exception_ptr& item) const
    {
        try
        {
            std::rethrow_exception(item);
        }
        catch (const std::exception& ex)
        {
#ifdef __GNUG__
            format_to(
                os, "std::exception_ptr<", demangle(abi::__cxa_current_exception_type()->name()), ">(\"", ex.what(), "\")");
#else
            os << "exception<"
               << "std::exception"
               << ">(" << ex.what() << ")";
#endif  // __GNUG__
        }

        catch (const std::string& ex)
        {
            format_to(os, "std::exception_ptr<std::string>(\"", ex, "\")");
        }
        catch (const char* ex)
        {
            format_to(os, "std::exception_ptr<const char*>(\"", ex, "\")");
        }
        catch (...)
        {
            format_to(os, "std::exception_ptr<...>(\"\")");
        }
    }
};

template <class T, class E>
struct result;

template <class T>
struct maybe;

namespace detail
{

template <class T>
struct is_result : std::false_type
{
};

template <class T, class E>
struct is_result<result<T, E>> : std::true_type
{
};

template <class E>
struct error_wrapper
{
    E m_error;
};

template <class E>
constexpr auto error(E&& error) -> error_wrapper<std::decay_t<E>>
{
    return error_wrapper<std::decay_t<E>>{ std::forward<E>(error) };
};

template <class Result>
struct to_result
{
    template <class Func, class... Args>
    constexpr auto operator()(Func&& func, Args&&... args) const -> Result
    {
        return Result{ std::invoke(std::forward<Func>(func), std::forward<Args>(args)...) };
    }
};

template <class E>
struct to_result<result<void, E>>
{
    template <class Func, class... Args>
    constexpr auto operator()(Func&& func, Args&&... args) const -> result<void, E>
    {
        std::invoke(std::forward<Func>(func), std::forward<Args>(args)...);
        return result<void, E>{};
    }
};

template <class Result, class Func, class... Args>
constexpr auto to_ok(Func&& func, Args&&... args) -> Result
{
    return to_result<Result>{}(std::forward<Func>(func), std::forward<Args>(args)...);
}

template <class Result, class In>
constexpr auto to_error(In&& in) -> Result
{
    return Result{ error(std::forward<In>(in).error()) };
}

}  // namespace detail

using detail::error;
using detail::error_wrapper;

template <class T, class E>
struct result
{
    using value_type = T;
    using error_type = E;
    using value_storage = value_type;
    using error_storage = error_wrapper<error_type>;

    constexpr result()
    {
    }

    constexpr result(value_type value) : m_storage(std::in_place_type<value_storage>, std::move(value))
    {
    }

    template <class Err, class = std::enable_if_t<std::is_constructible_v<error_type>>>
    constexpr result(const error_wrapper<Err>& error)
        : m_storage(std::in_place_type<error_storage>, error_storage{ error.m_error })
    {
    }

    template <class Err, class = std::enable_if_t<std::is_constructible_v<error_type>>>
    constexpr result(error_wrapper<Err>&& error)
        : m_storage(std::in_place_type<error_storage>, error_storage{ std::move(error.m_error) })
    {
    }

    constexpr result(const result&) = default;
    constexpr result(result&&) noexcept = default;

    constexpr explicit operator bool() const
    {
        return std::holds_alternative<value_storage>(m_storage);
    }

    constexpr const value_type& value() const&
    {
        return std::get<value_storage>(m_storage);
    }

    constexpr value_type& value() &
    {
        return std::get<value_storage>(m_storage);
    }

    constexpr value_type&& value() &&
    {
        return std::get<value_storage>(std::move(m_storage));
    }

    constexpr const value_type& operator*() const&
    {
        return std::get<value_storage>(m_storage);
    }

    constexpr value_type& operator*() &
    {
        return std::get<value_storage>(m_storage);
    }

    constexpr value_type&& operator*() &&
    {
        return std::get<value_storage>(std::move(m_storage));
    }

    constexpr const value_type* operator->() const&
    {
        return &**this;
    }

    constexpr value_type* operator->() &
    {
        return &**this;
    }

    constexpr const error_type& error() const&
    {
        return std::get<error_storage>(m_storage).m_error;
    }

    constexpr error_type& error() &
    {
        return std::get<error_storage>(m_storage).m_error;
    }

    constexpr error_type&& error() &&
    {
        return std::get<error_storage>(std::move(m_storage)).m_error;
    }

    constexpr bool has_value() const noexcept
    {
        return static_cast<bool>(*this);
    }

    constexpr bool has_error() const noexcept
    {
        return !has_value();
    }

    template <class Func, class FuncResult = std::invoke_result_t<Func, const T&>, class Result = FuncResult>
    constexpr auto and_then(Func&& func) const& -> Result
    {
        static_assert(
            detail::is_result<FuncResult>::value, "and_then: function result type needs to be of `result<T, E>` type");
        return *this  //
                   ? detail::to_ok<Result>(std::forward<Func>(func), value())
                   : detail::to_error<Result>(*this);
    }

    template <class Func, class FuncResult = std::invoke_result_t<Func, T&&>, class Result = FuncResult>
    constexpr auto and_then(Func&& func) && -> Result
    {
        static_assert(
            detail::is_result<FuncResult>::value, "and_then: function result type needs to be of `result<T, E>` type");
        return *this  //
                   ? detail::to_ok<Result>(std::forward<Func>(func), std::move(*this).value())
                   : detail::to_error<Result>(std::move(*this));
    }

    template <class Func, class FuncResult = std::invoke_result_t<Func, const T&>, class Result = result<FuncResult, E>>
    constexpr auto transform(Func&& func) const& -> Result
    {
        return *this  //
                   ? detail::to_ok<Result>(std::forward<Func>(func), value())
                   : detail::to_error<Result>(*this);
    }

    template <class Func, class FuncResult = std::invoke_result_t<Func, T&&>, class Result = result<FuncResult, E>>
    constexpr auto transform(Func&& func) && -> Result
    {
        return *this  //
                   ? detail::to_ok<Result>(std::forward<Func>(func), std::move(*this).value())
                   : detail::to_error<Result>(std::move(*this));
    }

    template <
        class Func,
        class FuncResult = std::invoke_result_t<Func, const E&>,
        class Result = std::conditional_t<std::is_void_v<FuncResult>, result<T, E>, FuncResult>>
    constexpr auto or_else(Func&& func) const& -> Result
    {
        if constexpr (std::is_void_v<FuncResult>)
        {
            return *this  //
                       ? Result{ *this }
                       : (std::invoke(std::forward<Func>(func), error()), Result{ *this });
        }
        else
        {
            static_assert(
                detail::is_result<FuncResult>::value, "or_else: function result type needs to be of `result<T, E>` type");
            return *this  //
                       ? Result{ *this }
                       : Result{ std::invoke(std::forward<Func>(func), error()) };
        }
    }

    template <
        class Func,
        class FuncResult = std::invoke_result_t<Func, const E&>,
        class Result = std::conditional_t<std::is_void_v<FuncResult>, result<T, E>, FuncResult>>
    constexpr auto or_else(Func&& func) && -> Result
    {
        if constexpr (std::is_void_v<FuncResult>)
        {
            return *this  //
                       ? Result{ std::move(*this) }
                       : (std::invoke(std::forward<Func>(func), error()), Result{ std::move(*this) });
        }
        else
        {
            static_assert(
                detail::is_result<FuncResult>::value, "or_else: function result type needs to be of `result<T, E>` type");
            return *this  //
                       ? Result{ std::move(*this) }
                       : Result{ std::invoke(std::forward<Func>(func), std::move(*this).error()) };
        }
    }

    template <class Func, class FuncResult = std::invoke_result_t<Func, const E&>, class Result = result<T, FuncResult>>
    constexpr auto transform_error(Func&& func) const& -> Result
    {
        return *this  //
                   ? Result{ value() }
                   : Result{ error(std::invoke(std::forward<Func>(func), error())) };
    }

    template <class Func, class FuncResult = std::invoke_result_t<Func, const E&>, class Result = result<T, FuncResult>>
    constexpr auto transform_error(Func&& func) && -> Result
    {
        return *this  //
                   ? Result{ std::move(*this).value() }
                   : Result{ error(std::invoke(std::forward<Func>(func), std::move(*this).error())) };
    }

private:
    std::variant<value_storage, error_storage> m_storage;
};

template <class T, class E>
struct result<T&, E>
{
    using value_type = T;
    using error_type = E;
    using value_storage = std::reference_wrapper<value_type>;
    using error_storage = error_wrapper<error_type>;

    constexpr result(value_type& value) : m_storage(std::in_place_type<value_storage>, value)
    {
    }

    template <class Err, class = std::enable_if_t<std::is_constructible_v<error_type>>>
    constexpr result(const error_wrapper<Err>& error)
        : m_storage(std::in_place_type<error_storage>, error_storage{ error.m_error })
    {
    }

    template <class Err, class = std::enable_if_t<std::is_constructible_v<error_type>>>
    constexpr result(error_wrapper<Err>&& error)
        : m_storage(std::in_place_type<error_storage>, error_storage{ std::move(error.m_error) })
    {
    }

    constexpr result(const result&) = default;
    constexpr result(result&&) noexcept = default;

    constexpr explicit operator bool() const
    {
        return std::holds_alternative<value_storage>(m_storage);
    }

    constexpr value_type& operator*() const
    {
        return std::get<value_storage>(m_storage);
    }

    constexpr value_type* operator->() const
    {
        return &**this;
    }

    constexpr value_type& value() const
    {
        return std::get<value_storage>(m_storage);
    }

    constexpr const error_type& error() const&
    {
        return std::get<error_storage>(m_storage).m_error;
    }

    constexpr error_type& error() &
    {
        return std::get<error_storage>(m_storage).m_error;
    }

    constexpr error_type&& error() &&
    {
        return std::get<error_storage>(std::move(m_storage)).m_error;
    }

    constexpr bool has_value() const noexcept
    {
        return static_cast<bool>(*this);
    }

    constexpr bool has_error() const noexcept
    {
        return !has_value();
    }

    template <class Func, class FuncResult = std::invoke_result_t<Func, T&>, class Result = FuncResult>
    constexpr auto and_then(Func&& func) const& -> Result
    {
        static_assert(
            detail::is_result<FuncResult>::value, "and_then: function result type needs to be of `result<T, E>` type");
        return *this  //
                   ? detail::to_ok<Result>(std::forward<Func>(func), value())
                   : detail::to_error<Result>(*this);
    }

    template <class Func, class FuncResult = std::invoke_result_t<Func, T&>, class Result = FuncResult>
    constexpr auto and_then(Func&& func) && -> Result
    {
        static_assert(
            detail::is_result<FuncResult>::value, "and_then: function result type needs to be of `result<T, E>` type");
        return *this  //
                   ? detail::to_ok<Result>(std::forward<Func>(func), std::move(*this).value())
                   : detail::to_error<Result>(std::move(*this));
    }

    template <class Func, class FuncResult = std::invoke_result_t<Func, T&>, class Result = result<FuncResult, E>>
    constexpr auto transform(Func&& func) const& -> Result
    {
        return *this  //
                   ? detail::to_ok<Result>(std::forward<Func>(func), value())
                   : detail::to_error<Result>(*this);
    }

    template <class Func, class FuncResult = std::invoke_result_t<Func, T&>, class Result = result<FuncResult, E>>
    constexpr auto transform(Func&& func) && -> Result
    {
        return *this  //
                   ? detail::to_ok<Result>(std::forward<Func>(func), value())
                   : detail::to_error<Result>(std::move(*this));
    }

    template <
        class Func,
        class FuncResult = std::invoke_result_t<Func, const E&>,
        class Result = std::conditional_t<std::is_void_v<FuncResult>, result<T, E>, FuncResult>>
    constexpr auto or_else(Func&& func) const& -> Result
    {
        if constexpr (std::is_void_v<FuncResult>)
        {
            return *this  //
                       ? Result{ *this }
                       : (std::invoke(std::forward<Func>(func), error()), Result{ *this });
        }
        else
        {
            static_assert(
                detail::is_result<FuncResult>::value, "or_else: function result type needs to be of `result<T, E>` type");
            return *this  //
                       ? Result{ *this }
                       : Result{ std::invoke(std::forward<Func>(func), error()) };
        }
    }

    template <
        class Func,
        class FuncResult = std::invoke_result_t<Func, const E&>,
        class Result = std::conditional_t<std::is_void_v<FuncResult>, result<T, E>, FuncResult>>
    constexpr auto or_else(Func&& func) && -> Result
    {
        if constexpr (std::is_void_v<FuncResult>)
        {
            return *this  //
                       ? Result{ std::move(*this) }
                       : (std::invoke(std::forward<Func>(func), error()), Result{ std::move(*this) });
        }
        else
        {
            static_assert(
                detail::is_result<FuncResult>::value, "or_else: function result type needs to be of `result<T, E>` type");
            return *this  //
                       ? Result{ std::move(*this) }
                       : Result{ std::invoke(std::forward<Func>(func), std::move(*this).error()) };
        }
    }

    template <class Func, class FuncResult = std::invoke_result_t<Func, const E&>, class Result = result<T, FuncResult>>
    constexpr auto transform_error(Func&& func) const& -> Result
    {
        return *this  //
                   ? Result{ value() }
                   : Result{ error(std::invoke(std::forward<Func>(func), error())) };
    }

    template <class Func, class FuncResult = std::invoke_result_t<Func, const E&>, class Result = result<T, FuncResult>>
    constexpr auto transform_error(Func&& func) && -> Result
    {
        return *this  //
                   ? Result{ std::move(*this).value() }
                   : Result{ error(std::invoke(std::forward<Func>(func), std::move(*this).error())) };
    }

private:
    std::variant<value_storage, error_storage> m_storage;
};

template <class E>
struct result<void, E>
{
    using value_type = void;
    using error_type = E;

    constexpr result() : m_storage{}
    {
    }

    template <class Err, class = std::enable_if_t<std::is_constructible_v<error_type>>>
    constexpr result(const error_wrapper<Err>& error) : m_storage(error.m_error)
    {
    }

    template <class Err, class = std::enable_if_t<std::is_constructible_v<error_type>>>
    constexpr result(error_wrapper<Err>&& error) : m_storage(std::move(error.m_error))
    {
    }

    constexpr result(const result&) = default;
    constexpr result(result&&) noexcept = default;

    constexpr explicit operator bool() const
    {
        return !m_storage.has_value();
    }

    constexpr const error_type& error() const&
    {
        return *m_storage;
    }

    constexpr error_type& error() &
    {
        return *m_storage;
    }

    constexpr error_type&& error() &&
    {
        return *std::move(m_storage);
    }

    constexpr bool has_value() const noexcept
    {
        return static_cast<bool>(*this);
    }

    constexpr bool has_error() const noexcept
    {
        return !has_value();
    }

    template <class Func, class FuncResult = std::invoke_result_t<Func>, class Result = FuncResult>
    constexpr auto and_then(Func&& func) const& -> Result
    {
        static_assert(
            detail::is_result<FuncResult>::value, "and_then: function result type needs to be of `result<T, E>` type");
        return *this  //
                   ? detail::to_ok<Result>(std::forward<Func>(func))
                   : detail::to_error<Result>(*this);
    }

    template <class Func, class FuncResult = std::invoke_result_t<Func>, class Result = FuncResult>
    constexpr auto and_then(Func&& func) && -> Result
    {
        static_assert(
            detail::is_result<FuncResult>::value, "and_then: function result type needs to be of `result<T, E>` type");
        return *this  //
                   ? detail::to_ok<Result>(std::forward<Func>(func))
                   : detail::to_error<Result>(std::move(*this));
    }

    template <class Func, class FuncResult = std::invoke_result_t<Func>, class Result = result<FuncResult, E>>
    constexpr auto transform(Func&& func) const& -> Result
    {
        return *this  //
                   ? detail::to_ok<Result>(std::forward<Func>(func))
                   : detail::to_error<Result>(*this);
    }

    template <class Func, class FuncResult = std::invoke_result_t<Func>, class Result = result<FuncResult, E>>
    constexpr auto transform(Func&& func) && -> Result
    {
        return *this  //
                   ? detail::to_ok<Result>(std::forward<Func>(func))
                   : detail::to_error<Result>(std::move(*this));
    }

    template <
        class Func,
        class FuncResult = std::invoke_result_t<Func, const E&>,
        class Result = std::conditional_t<std::is_void_v<FuncResult>, result<void, E>, FuncResult>>
    constexpr auto or_else(Func&& func) const& -> Result
    {
        if constexpr (std::is_void_v<FuncResult>)
        {
            return *this  //
                       ? Result{ *this }
                       : (std::invoke(std::forward<Func>(func), error()), Result{ *this });
        }
        else
        {
            static_assert(
                detail::is_result<FuncResult>::value, "or_else: function result type needs to be of `result<T, E>` type");
            return *this  //
                       ? Result{ *this }
                       : Result{ std::invoke(std::forward<Func>(func), error()) };
        }
    }

    template <
        class Func,
        class FuncResult = std::invoke_result_t<Func, const E&>,
        class Result = std::conditional_t<std::is_void_v<FuncResult>, result<void, E>, FuncResult>>
    constexpr auto or_else(Func&& func) && -> Result
    {
        if constexpr (std::is_void_v<FuncResult>)
        {
            return *this  //
                       ? Result{ std::move(*this) }
                       : (std::invoke(std::forward<Func>(func), error()), Result{ std::move(*this) });
        }
        else
        {
            static_assert(
                detail::is_result<FuncResult>::value, "or_else: function result type needs to be of `result<T, E>` type");
            return *this  //
                       ? Result{ std::move(*this) }
                       : Result{ std::invoke(std::forward<Func>(func), std::move(*this).error()) };
        }
    }

    template <class Func, class FuncResult = std::invoke_result_t<Func, const E&>, class Result = result<void, FuncResult>>
    constexpr auto transform_error(Func&& func) const& -> Result
    {
        return *this  //
                   ? Result{}
                   : Result{ error(std::invoke(std::forward<Func>(func), error())) };
    }

    template <class Func, class FuncResult = std::invoke_result_t<Func, const E&>, class Result = result<void, FuncResult>>
    constexpr auto transform_error(Func&& func) && -> Result
    {
        return *this  //
                   ? Result{}
                   : Result{ error(std::invoke(std::forward<Func>(func), std::move(*this).error())) };
    }

private:
    std::optional<error_type> m_storage;
};

template <class E>
struct formatter<result<void, E>>
{
    void format(std::ostream& os, const result<void, E>& item) const
    {
        if (item.has_value())
        {
            format_to(os, "ok()");
        }
        else
        {
            format_to(os, "error(", item.error(), ")");
        }
    }
};

template <class T, class E>
struct formatter<result<T, E>>
{
    void format(std::ostream& os, const result<T, E>& item) const
    {
        if (item.has_value())
        {
            format_to(os, "ok(", item.value(), ")");
        }
        else
        {
            format_to(os, "error(", item.error(), ")");
        }
    }
};

template <class T, class E>
std::ostream& operator<<(std::ostream& os, const result<T, E>& item)
{
    return format_to(os, item);
}

template <class Func, class... Args>
auto try_invoke(Func&& func, Args&&... args) -> result<std::invoke_result_t<Func, Args...>, std::exception_ptr>
{
    try
    {
        return std::invoke(std::forward<Func>(func), std::forward<Args>(args)...);
    }
    catch (...)
    {
        return error(std::current_exception());
    }
}

template <class T, class... Args>
auto try_create(Args&&... args) -> result<T, std::exception_ptr>
{
    try
    {
        return T{ std::forward<Args>(args)... };
    }
    catch (...)
    {
        return error(std::current_exception());
    }
}

namespace detail
{

template <class T>
struct is_maybe : std::false_type
{
};

template <class T>
struct is_maybe<maybe<T>> : std::true_type
{
};

}  // namespace detail

template <class T>
struct maybe
{
    using value_type = T;

    constexpr maybe() : m_storage()
    {
    }

    constexpr maybe(value_type value) : m_storage(std::move(value))
    {
    }

    constexpr maybe(const maybe&) = default;
    constexpr maybe(maybe&&) noexcept = default;

    constexpr explicit operator bool() const
    {
        return m_storage.has_value();
    }

    constexpr const value_type& operator*() const&
    {
        return *m_storage;
    }

    constexpr value_type& operator*() &
    {
        return *m_storage;
    }

    constexpr value_type&& operator*() &&
    {
        return *std::move(m_storage);
    }

    constexpr const value_type* operator->() const&
    {
        return &**this;
    }

    constexpr value_type* operator->() &
    {
        return &**this;
    }

    constexpr const value_type& value() const&
    {
        return *m_storage;
    }

    constexpr value_type& value() &
    {
        return *m_storage;
    }

    constexpr value_type&& value() &&
    {
        return *std::move(m_storage);
    }

    constexpr bool has_value() const noexcept
    {
        return static_cast<bool>(*this);
    }

    template <class Func, class FuncResult = std::invoke_result_t<Func, const T&>, class Result = FuncResult>
    constexpr auto and_then(Func&& func) const& -> Result
    {
        static_assert(detail::is_maybe<FuncResult>::value, "and_then: function result type needs to be of `maybe<T>` type");
        return *this  //
                   ? Result{ std::invoke(std::forward<Func>(func), value()) }
                   : Result{};
    }

    template <class Func, class FuncResult = std::invoke_result_t<Func, T&&>, class Result = FuncResult>
    constexpr auto and_then(Func&& func) && -> Result
    {
        static_assert(detail::is_maybe<FuncResult>::value, "and_then: function result type needs to be of `maybe<T>` type");
        return *this  //
                   ? Result{ std::invoke(std::forward<Func>(func), std::move(*this).value()) }
                   : Result{};
    }

    template <class Func, class FuncResult = std::invoke_result_t<Func, const T&>, class Result = maybe<FuncResult>>
    constexpr auto transform(Func&& func) const& -> Result
    {
        return *this  //
                   ? Result{ std::invoke(std::forward<Func>(func), value()) }
                   : Result{};
    }

    template <class Func, class FuncResult = std::invoke_result_t<Func, T&&>, class Result = maybe<FuncResult>>
    constexpr auto transform(Func&& func) && -> Result
    {
        return *this  //
                   ? Result{ std::invoke(std::forward<Func>(func), std::move(*this).value()) }
                   : Result{};
    }

    template <
        class Func,
        class FuncResult = std::invoke_result_t<Func>,
        class Result = std::conditional_t<std::is_void_v<FuncResult>, maybe<T>, FuncResult>>
    constexpr auto or_else(Func&& func) const& -> Result
    {
        if constexpr (std::is_void_v<FuncResult>)
        {
            return *this  //
                       ? Result{ *this }
                       : (std::invoke(std::forward<Func>(func)), Result{ *this });
        }
        else
        {
            static_assert(
                detail::is_maybe<FuncResult>::value, "or_else: function result type needs to be of `maybe<T>` type");
            return *this  //
                       ? Result{ *this }
                       : Result{ std::invoke(std::forward<Func>(func)) };
        }
    }

    template <
        class Func,
        class FuncResult = std::invoke_result_t<Func>,
        class Result = std::conditional_t<std::is_void_v<FuncResult>, maybe<T>, FuncResult>>
    constexpr auto or_else(Func&& func) && -> Result
    {
        if constexpr (std::is_void_v<FuncResult>)
        {
            return *this  //
                       ? Result{ std::move(*this) }
                       : (std::invoke(std::forward<Func>(func)), Result{ std::move(*this) });
        }
        else
        {
            static_assert(
                detail::is_maybe<FuncResult>::value, "or_else: function result type needs to be of `maybe<T>` type");
            return *this  //
                       ? Result{ std::move(*this) }
                       : Result{ std::invoke(std::forward<Func>(func)) };
        }
    }

    template <class Pred>
    constexpr auto filter(Pred&& pred) const& -> maybe<T>
    {
        return std::invoke(std::forward<Pred>(pred), value())  //
                   ? maybe<T>{ *this }
                   : maybe<T>{};
    }

    template <class Pred>
    constexpr auto filter(Pred&& pred) && -> maybe<T>
    {
        return std::invoke(std::forward<Pred>(pred), value())  //
                   ? maybe<T>{ std::move(*this) }
                   : maybe<T>{};
    }

private:
    std::optional<value_type> m_storage;
};

template <class T>
struct maybe<T&>
{
    using value_type = T;

    constexpr maybe() : m_storage()
    {
    }

    constexpr maybe(value_type& value) : m_storage(&value)
    {
    }

    constexpr maybe(const maybe&) = default;
    constexpr maybe(maybe&&) noexcept = default;

    constexpr explicit operator bool() const
    {
        return static_cast<bool>(m_storage);
    }

    constexpr value_type& operator*() const
    {
        return *m_storage;
    }

    constexpr value_type* operator->() const
    {
        return &**this;
    }

    constexpr value_type& value() const
    {
        return *m_storage;
    }

    constexpr bool has_value() const noexcept
    {
        return static_cast<bool>(*this);
    }

    template <class Func, class FuncResult = std::invoke_result_t<Func, T&>, class Result = FuncResult>
    constexpr auto and_then(Func&& func) const -> Result
    {
        static_assert(detail::is_maybe<FuncResult>::value, "and_then: function result type needs to be of `maybe<T>` type");
        return *this  //
                   ? Result{ std::invoke(std::forward<Func>(func), value()) }
                   : Result{};
    }

    template <class Func, class FuncResult = std::invoke_result_t<Func, T&>, class Result = maybe<FuncResult>>
    constexpr auto transform(Func&& func) const -> Result
    {
        return *this  //
                   ? Result{ std::invoke(std::forward<Func>(func), value()) }
                   : Result{};
    }

    template <
        class Func,
        class FuncResult = std::invoke_result_t<Func>,
        class Result = std::conditional_t<std::is_void_v<FuncResult>, maybe<T>, FuncResult>>
    constexpr auto or_else(Func&& func) const -> Result
    {
        if constexpr (std::is_void_v<FuncResult>)
        {
            return *this  //
                       ? Result{ *this }
                       : (std::invoke(std::forward<Func>(func)), Result{ *this });
        }
        else
        {
            static_assert(
                detail::is_maybe<FuncResult>::value, "or_else: function result type needs to be of `maybe<T>` type");
            return *this  //
                       ? Result{ *this }
                       : Result{ std::invoke(std::forward<Func>(func)) };
        }
    }

    template <class Pred>
    constexpr auto filter(Pred&& pred) const -> maybe<T&>
    {
        return std::invoke(std::forward<Pred>(pred), value())  //
                   ? maybe<T>{ *this }
                   : maybe<T>{};
    }

private:
    value_type* m_storage;
};

template <class T>
struct formatter<maybe<T>>
{
    void format(std::ostream& os, const maybe<T>& item) const
    {
        if (item.has_value())
        {
            format_to(os, "some(", item.value(), ")");
        }
        else
        {
            format_to(os, "none");
        }
    }
};

template <class T>
std::ostream& operator<<(std::ostream& os, const maybe<T>& item)
{
    return format_to(os, item);
}

namespace detail
{

template <class Iter>
struct reverse_iterator_impl
{
    using type = std::reverse_iterator<Iter>;
};

template <class Iter>
struct reverse_iterator_impl<std::reverse_iterator<Iter>>
{
    using type = Iter;
};

template <class Iter>
using reverse_iterator_t = typename reverse_iterator_impl<Iter>::type;

template <class Iter>
auto make_reverse(Iter b, Iter e) -> std::pair<reverse_iterator_t<Iter>, reverse_iterator_t<Iter>>
{
    return { reverse_iterator_t<Iter>(e), reverse_iterator_t<Iter>(b) };
}

template <class Iter>
auto make_reverse(std::reverse_iterator<Iter> b, std::reverse_iterator<Iter> e) -> std::pair<Iter, Iter>
{
    return { e.base(), b.base() };
}

}  // namespace detail

struct slice_t
{
    maybe<std::ptrdiff_t> begin;
    maybe<std::ptrdiff_t> end;
};

template <class Iter>
class iterator_range
{
public:
    using iterator = Iter;
    using const_iterator = iterator;

    using reference = typename std::iterator_traits<iterator>::reference;
    using difference_type = typename std::iterator_traits<iterator>::difference_type;
    using size_type = difference_type;

    using maybe_reference = maybe<reference>;

    using reverse_type = iterator_range<detail::reverse_iterator_t<iterator>>;

    constexpr iterator_range() = default;
    constexpr iterator_range(const iterator_range&) = default;
    constexpr iterator_range(iterator_range&&) noexcept = default;

    constexpr iterator_range(iterator b, iterator e) : m_begin(b), m_end(e)
    {
    }

    constexpr iterator_range(const std::pair<iterator, iterator>& pair)
        : iterator_range(std::get<0>(pair), std::get<1>(pair))
    {
    }

    constexpr iterator_range(iterator b, size_type n) : iterator_range(b, std::next(b, n))
    {
    }

    template <class Range, class = iterator_t<Range>>
    constexpr iterator_range(Range&& range) : iterator_range(std::begin(range), std::end(range))
    {
    }

    iterator_range& operator=(iterator_range other) noexcept
    {
        std::swap(m_begin, other.m_begin);
        std::swap(m_end, other.m_end);
        return *this;
    }

    constexpr auto begin() const noexcept -> iterator
    {
        return m_begin;
    }

    constexpr auto end() const noexcept -> iterator
    {
        return m_end;
    }

    template <class Container, require<std::is_constructible_v<Container, iterator, iterator>> = 0>
    operator Container() const
    {
        return Container{ begin(), end() };
    }

    auto empty() const -> bool
    {
        return begin() == end();
    }

    template <class It = iterator, require<is_random_access_iterator<It>::value> = 0>
    auto ssize() const -> difference_type
    {
        return std::distance(begin(), end());
    }

    template <class It = iterator, require<is_random_access_iterator<It>::value> = 0>
    auto size() const -> difference_type
    {
        return std::distance(begin(), end());
    }

    auto front() const -> reference
    {
        if (empty())
        {
            throw std::out_of_range{ "iterator_range::front - empty range" };
        }
        return *begin();
    }

    template <class It = iterator, require<is_bidirectional_iterator<It>::value> = 0>
    auto back() const -> reference
    {
        if (empty())
        {
            throw std::out_of_range{ "iterator_range::back - empty range" };
        }
        return *std::prev(end());
    }

    auto maybe_front() const -> maybe_reference
    {
        if (empty())
        {
            return {};
        }
        return *begin();
    }

    template <class It = iterator, require<is_bidirectional_iterator<It>::value> = 0>
    auto maybe_back() const -> maybe_reference
    {
        if (empty())
        {
            return {};
        }
        return *std::prev(end());
    }

    template <class It = iterator, require<is_random_access_iterator<It>::value> = 0>
    auto at(difference_type n) const -> reference
    {
        if (!(0 <= n && n < size()))
        {
            throw std::out_of_range{ "iterator_range::at - index out of range" };
        }
        return *std::next(begin(), n);
    }

    template <class It = iterator, require<is_random_access_iterator<It>::value> = 0>
    auto operator[](difference_type n) const -> reference
    {
        return at(n);
    }

    template <class It = iterator, require<is_random_access_iterator<It>::value> = 0>
    auto maybe_at(difference_type n) const -> maybe_reference
    {
        if (!(0 <= n && n < size()))
        {
            return {};
        }
        return *std::next(begin(), n);
    }

    template <class It = iterator, require<is_bidirectional_iterator<It>::value> = 0>
    auto reverse() const -> reverse_type
    {
        return detail::make_reverse(begin(), end());
    }

    auto take(difference_type n) const -> iterator_range
    {
        return iterator_range(begin(), advance(n));
    }

    auto drop(difference_type n) const -> iterator_range
    {
        return iterator_range(advance(n), end());
    }

    template <class It = iterator, require<is_bidirectional_iterator<It>::value> = 0>
    auto take_back(difference_type n) const -> iterator_range
    {
        return reverse().take(n).reverse();
    }

    template <class It = iterator, require<is_bidirectional_iterator<It>::value> = 0>
    auto drop_back(difference_type n) const -> iterator_range
    {
        return reverse().drop(n).reverse();
    }

    template <class Pred>
    auto take_while(Pred&& pred) const -> iterator_range
    {
        const iterator found = std::find_if_not(begin(), end(), std::forward<Pred>(pred));
        return iterator_range(begin(), found);
    }

    template <class Pred>
    auto drop_while(Pred&& pred) const -> iterator_range
    {
        const iterator found = std::find_if_not(begin(), end(), std::forward<Pred>(pred));
        return iterator_range(found, end());
    }

    template <class Pred, class It = iterator, require<is_bidirectional_iterator<It>::value> = 0>
    auto take_back_while(Pred&& pred) const -> iterator_range
    {
        return reverse().take_while(std::forward<Pred>(pred)).reverse();
    }

    template <class Pred, class It = iterator, require<is_bidirectional_iterator<It>::value> = 0>
    auto drop_back_while(Pred&& pred) const -> iterator_range
    {
        return reverse().drop_while(std::forward<Pred>(pred)).reverse();
    }

    template <class Pred>
    auto find_if(Pred&& pred) const -> maybe_reference
    {
        const iterator found = std::find_if(begin(), end(), std::forward<Pred>(pred));
        return found != end() ? maybe_reference{ *found } : maybe_reference{};
    }

    template <class It = iterator, require<is_random_access_iterator<It>::value> = 0>
    auto slice(const slice_t& info) const -> iterator_range
    {
        static const auto adjust = [](difference_type index, size_type size) -> size_type {  //
            return std::clamp<size_type>(index >= 0 ? index : index + size, 0, size);
        };
        const size_type s = size();
        const size_type b = info.begin ? adjust(*info.begin, s) : size_type{ 0 };
        const size_type e = info.end ? adjust(*info.end, s) : s;
        return iterator_range{ begin() + b, std::max(size_type{ 0 }, e - b) };
    }

private:
    template <class It = iterator, require<is_random_access_iterator<It>::value> = 0>
    auto advance(difference_type n) const -> iterator
    {
        return begin() + std::min(ssize(), n);
    }

    template <class It = iterator, require<!is_random_access_iterator<It>::value> = 0>
    auto advance(difference_type n) const -> iterator
    {
        iterator it = begin();
        while (it != end() && n > 0)
        {
            --n;
            ++it;
        }
        return it;
    }

    iterator m_begin;
    iterator m_end;
};

template <class Container>
using subrange = iterator_range<iterator_t<Container>>;

template <class T>
using span = iterator_range<const T*>;

template <class T>
using mut_span = iterator_range<T*>;

struct identity
{
    template <class T>
    constexpr T&& operator()(T&& item) const noexcept
    {
        return std::forward<T>(item);
    }
};

template <class... Args>
struct formatter<std::tuple<Args...>>
{
    void format(std::ostream& os, const std::tuple<Args...>& item) const
    {
        format_to(os, "(");
        std::apply(
            [&os](const auto&... args)
            {
                auto n = 0u;
                ((format_to(os, args) << (++n != sizeof...(args) ? ", " : "")), ...);
            },
            item);
        format_to(os, ")");
    }
};

template <class F, class S>
struct formatter<std::pair<F, S>>
{
    void format(std::ostream& os, const std::pair<F, S>& item) const
    {
        format_to(os, "(", item.first, ", ", item.second, ")");
    }
};

template <class... Pipes>
struct pipe_t
{
    std::tuple<Pipes...> m_pipes;

    constexpr pipe_t(std::tuple<Pipes...> pipes) : m_pipes{ std::move(pipes) }
    {
    }

    constexpr pipe_t(Pipes... pipes) : pipe_t{ std::tuple<Pipes...>{ std::move(pipes)... } }
    {
    }

private:
    template <std::size_t I, class... Args>
    constexpr auto invoke(Args&&... args) const -> decltype(std::invoke(std::get<I>(m_pipes), std::forward<Args>(args)...))
    {
        return std::invoke(std::get<I>(m_pipes), std::forward<Args>(args)...);
    }

    template <std::size_t I, class... Args, std::enable_if_t<(I + 1) == sizeof...(Pipes), int> = 0>
    constexpr auto call(Args&&... args) const -> decltype(invoke<I>(std::forward<Args>(args)...))
    {
        return invoke<I>(std::forward<Args>(args)...);
    }

    template <std::size_t I, class... Args, std::enable_if_t<(I + 1) < sizeof...(Pipes), int> = 0>
    constexpr auto call(Args&&... args) const -> decltype(call<I + 1>(invoke<I>(std::forward<Args>(args)...)))
    {
        return call<I + 1>(invoke<I>(std::forward<Args>(args)...));
    }

public:
    template <class... Args>
    constexpr auto operator()(Args&&... args) const -> decltype(call<0>(std::forward<Args>(args)...))
    {
        return call<0>(std::forward<Args>(args)...);
    }
};

template <class... Pipes>
struct formatter<pipe_t<Pipes...>>
{
    void format(std::ostream& os, const pipe_t<Pipes...>& item) const
    {
        format_to(os, "pipe(");
        std::apply(
            [&os](const auto&... args)
            {
                auto n = 0u;
                ((format_to(os, args) << (++n != sizeof...(args) ? ", " : "")), ...);
            },
            item.m_pipes);
        format_to(os, ")");
    }
};

namespace detail
{

struct pipe_fn
{
private:
    template <class Pipe>
    constexpr auto to_tuple(Pipe pipe) const -> std::tuple<Pipe>
    {
        return std::tuple<Pipe>{ std::move(pipe) };
    }

    template <class... Pipes>
    constexpr auto to_tuple(pipe_t<Pipes...> pipe) const -> std::tuple<Pipes...>
    {
        return pipe.m_pipes;
    }

    template <class... Pipes>
    constexpr auto from_tuple(std::tuple<Pipes...> tuple) const -> pipe_t<Pipes...>
    {
        return pipe_t<Pipes...>{ std::move(tuple) };
    }

public:
    template <class... Pipes>
    constexpr auto operator()(Pipes&&... pipes) const
        -> decltype(from_tuple(std::tuple_cat(to_tuple(std::forward<Pipes>(pipes))...)))
    {
        return from_tuple(std::tuple_cat(to_tuple(std::forward<Pipes>(pipes))...));
    }
};

static constexpr inline struct do_all_fn
{
    template <class... Funcs>
    struct impl
    {
        std::tuple<Funcs...> m_funcs;

        template <class... Args>
        void operator()(Args&&... args) const
        {
            call(std::index_sequence_for<Funcs...>{}, std::forward<Args>(args)...);
        }

        template <class... Args, std::size_t... I>
        void call(std::index_sequence<I...>, Args&&... args) const
        {
            (std::invoke(std::get<I>(m_funcs), std::forward<Args>(args)...), ...);
        }
    };

    template <class... Funcs>
    auto operator()(Funcs&&... funcs) const -> impl<std::decay_t<Funcs>...>
    {
        return { std::forward_as_tuple(std::forward<Funcs>(funcs)...) };
    }
} do_all;

static constexpr struct apply_fn
{
    template <class Func>
    struct impl
    {
        Func m_func;

        template <class T>
        T& operator()(T& item) const
        {
            std::invoke(m_func, item);
            return item;
        }
    };

    template <class... Funcs>
    auto operator()(Funcs&&... funcs) const -> pipe_t<impl<decltype(do_all(std::forward<Funcs>(funcs)...))>>
    {
        return { { do_all(std::forward<Funcs>(funcs)...) } };
    }
} apply;

static constexpr inline struct with_fn
{
    template <class Func>
    struct impl
    {
        Func m_func;

        template <class T>
        T operator()(T item) const
        {
            std::invoke(m_func, item);
            return item;
        }
    };

    template <class... Funcs>
    auto operator()(Funcs&&... funcs) const -> pipe_t<impl<decltype(do_all(std::forward<Funcs>(funcs)...))>>
    {
        return { { do_all(std::forward<Funcs>(funcs)...) } };
    }
} with;

}  // namespace detail

static constexpr inline auto pipe = detail::pipe_fn{};
using detail::apply;
using detail::do_all;
using detail::with;

}  // namespace core
