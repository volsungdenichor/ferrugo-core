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

namespace result_details
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

template <class T, class E>
struct has_value_mixin
{
    constexpr bool has_value() const noexcept
    {
        const auto& self = static_cast<const result<T, E>&>(*this);
        return static_cast<bool>(self);
    }

    constexpr bool has_error() const noexcept
    {
        return !has_value();
    }
};

template <class T, class E>
struct and_then_mixin
{
    template <class Func, class FuncResult = std::invoke_result_t<Func, const T&>, class Result = FuncResult>
    constexpr auto and_then(Func&& func) const& -> Result
    {
        static_assert(is_result<FuncResult>::value, "and_then: function result type needs to be of `result<T, E>` type");
        const auto& self = static_cast<const result<T, E>&>(*this);
        return self  //
                   ? to_ok<Result>(std::forward<Func>(func), self.value())
                   : to_error<Result>(self);
    }

    template <class Func, class FuncResult = std::invoke_result_t<Func, T&&>, class Result = FuncResult>
    constexpr auto and_then(Func&& func) && -> Result
    {
        static_assert(is_result<FuncResult>::value, "and_then: function result type needs to be of `result<T, E>` type");
        auto& self = static_cast<result<T, E>&>(*this);
        return self  //
                   ? to_ok<Result>(std::forward<Func>(func), std::move(self).value())
                   : to_error<Result>(std::move(self));
    }
};

template <class T, class E>
struct and_then_mixin<T&, E>
{
    template <class Func, class FuncResult = std::invoke_result_t<Func, T&>, class Result = FuncResult>
    constexpr auto and_then(Func&& func) const& -> Result
    {
        static_assert(is_result<FuncResult>::value, "and_then: function result type needs to be of `result<T, E>` type");
        const auto& self = static_cast<const result<T&, E>&>(*this);
        return self  //
                   ? to_ok<Result>(std::forward<Func>(func), self.value())
                   : to_error<Result>(self);
    }

    template <class Func, class FuncResult = std::invoke_result_t<Func, T&>, class Result = FuncResult>
    constexpr auto and_then(Func&& func) && -> Result
    {
        static_assert(is_result<FuncResult>::value, "and_then: function result type needs to be of `result<T, E>` type");
        auto& self = static_cast<result<T&, E>&>(*this);
        return self  //
                   ? to_ok<Result>(std::forward<Func>(func), std::move(self).value())
                   : to_error<Result>(std::move(self));
    }
};

template <class E>
struct and_then_mixin<void, E>
{
    template <class Func, class FuncResult = std::invoke_result_t<Func>, class Result = FuncResult>
    constexpr auto and_then(Func&& func) const& -> Result
    {
        static_assert(is_result<FuncResult>::value, "and_then: function result type needs to be of `result<T, E>` type");
        const auto& self = static_cast<const result<void, E>&>(*this);
        return self  //
                   ? to_ok<Result>(std::forward<Func>(func))
                   : to_error<Result>(self);
    }

    template <class Func, class FuncResult = std::invoke_result_t<Func>, class Result = FuncResult>
    constexpr auto and_then(Func&& func) && -> Result
    {
        static_assert(is_result<FuncResult>::value, "and_then: function result type needs to be of `result<T, E>` type");
        auto& self = static_cast<result<void, E>&>(*this);
        return self  //
                   ? to_ok<Result>(std::forward<Func>(func))
                   : to_error<Result>(std::move(self));
    }
};

template <class T, class E>
struct transform_mixin
{
    template <class Func, class FuncResult = std::invoke_result_t<Func, const T&>, class Result = result<FuncResult, E>>
    constexpr auto transform(Func&& func) const& -> Result
    {
        const auto& self = static_cast<const result<T, E>&>(*this);
        return self  //
                   ? to_ok<Result>(std::forward<Func>(func), self.value())
                   : to_error<Result>(self);
    }

    template <class Func, class FuncResult = std::invoke_result_t<Func, T&&>, class Result = result<FuncResult, E>>
    constexpr auto transform(Func&& func) && -> Result
    {
        auto& self = static_cast<result<T, E>&>(*this);
        return self  //
                   ? to_ok<Result>(std::forward<Func>(func), std::move(self).value())
                   : to_error<Result>(std::move(self));
    }
};

template <class T, class E>
struct transform_mixin<T&, E>
{
    template <class Func, class FuncResult = std::invoke_result_t<Func, T&>, class Result = result<FuncResult, E>>
    constexpr auto transform(Func&& func) const& -> Result
    {
        const auto& self = static_cast<const result<T&, E>&>(*this);
        return self  //
                   ? to_ok<Result>(std::forward<Func>(func), self.value())
                   : to_error<Result>(self);
    }

    template <class Func, class FuncResult = std::invoke_result_t<Func, T&>, class Result = result<FuncResult, E>>
    constexpr auto transform(Func&& func) && -> Result
    {
        auto& self = static_cast<result<T&, E>&>(*this);
        return self  //
                   ? to_ok<Result>(std::forward<Func>(func), self.value())
                   : to_error<Result>(std::move(self));
    }
};

template <class E>
struct transform_mixin<void, E>
{
    template <class Func, class FuncResult = std::invoke_result_t<Func>, class Result = result<FuncResult, E>>
    constexpr auto transform(Func&& func) const& -> Result
    {
        const auto& self = static_cast<const result<void, E>&>(*this);
        return self  //
                   ? to_ok<Result>(std::forward<Func>(func))
                   : to_error<Result>(self);
    }

    template <class Func, class FuncResult = std::invoke_result_t<Func>, class Result = result<FuncResult, E>>
    constexpr auto transform(Func&& func) && -> Result
    {
        auto& self = static_cast<result<void, E>&>(*this);
        return self  //
                   ? to_ok<Result>(std::forward<Func>(func))
                   : to_error<Result>(std::move(self));
    }
};

template <class T, class E>
struct or_else_mixin
{
    template <
        class Func,
        class FuncResult = std::invoke_result_t<Func, const E&>,
        class Result = std::conditional_t<std::is_void_v<FuncResult>, result<T, E>, FuncResult>>
    constexpr auto or_else(Func&& func) const& -> Result
    {
        const auto& self = static_cast<const result<T, E>&>(*this);
        if constexpr (std::is_void_v<FuncResult>)
        {
            return self  //
                       ? Result{ self }
                       : (std::invoke(std::forward<Func>(func), self.error()), Result{ self });
        }
        else
        {
            static_assert(is_result<FuncResult>::value, "or_else: function result type needs to be of `result<T, E>` type");
            return self  //
                       ? Result{ self }
                       : Result{ std::invoke(std::forward<Func>(func), self.error()) };
        }
    }

    template <
        class Func,
        class FuncResult = std::invoke_result_t<Func, const E&>,
        class Result = std::conditional_t<std::is_void_v<FuncResult>, result<T, E>, FuncResult>>
    constexpr auto or_else(Func&& func) && -> Result
    {
        auto& self = static_cast<result<T, E>&>(*this);
        if constexpr (std::is_void_v<FuncResult>)
        {
            return self  //
                       ? Result{ std::move(self) }
                       : (std::invoke(std::forward<Func>(func), self.error()), Result{ std::move(self) });
        }
        else
        {
            static_assert(is_result<FuncResult>::value, "or_else: function result type needs to be of `result<T, E>` type");
            return self  //
                       ? Result{ std::move(self) }
                       : Result{ std::invoke(std::forward<Func>(func), std::move(self).error()) };
        }
    }
};

template <class T, class E>
struct transform_error_mixin
{
    template <class Func, class FuncResult = std::invoke_result_t<Func, const E&>, class Result = result<T, FuncResult>>
    constexpr auto transform_error(Func&& func) const& -> Result
    {
        const auto& self = static_cast<const result<T, E>&>(*this);
        return self  //
                   ? Result{ self.value() }
                   : Result{ error(std::invoke(std::forward<Func>(func), self.error())) };
    }

    template <class Func, class FuncResult = std::invoke_result_t<Func, const E&>, class Result = result<T, FuncResult>>
    constexpr auto transform_error(Func&& func) && -> Result
    {
        auto& self = static_cast<result<T, E>&>(*this);
        return self  //
                   ? Result{ std::move(self).value() }
                   : Result{ error(std::invoke(std::forward<Func>(func), std::move(self).error())) };
    }
};

}  // namespace result_details

using result_details::error;
using result_details::error_wrapper;

template <class T, class E>
struct result : result_details::has_value_mixin<T, E>,
                result_details::and_then_mixin<T, E>,
                result_details::transform_mixin<T, E>,
                result_details::or_else_mixin<T, E>,
                result_details::transform_error_mixin<T, E>
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

private:
    std::variant<value_storage, error_storage> m_storage;
};

template <class T, class E>
struct result<T&, E> : result_details::has_value_mixin<T&, E>,
                       result_details::and_then_mixin<T&, E>,
                       result_details::transform_mixin<T&, E>,
                       result_details::or_else_mixin<T&, E>,
                       result_details::transform_error_mixin<T&, E>
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

private:
    std::variant<value_storage, error_storage> m_storage;
};

template <class E>
struct result<void, E> : result_details::has_value_mixin<void, E>,
                         result_details::and_then_mixin<void, E>,
                         result_details::transform_mixin<void, E>,
                         result_details::or_else_mixin<void, E>,
                         result_details::transform_error_mixin<void, E>
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

constexpr inline auto to_exception_message = [](const std::exception_ptr& ptr) -> std::string
{
    try
    {
        std::rethrow_exception(ptr);
    }
    catch (const std::exception& ex)
    {
        return ex.what();
    }
    catch (const std::string& ex)
    {
        return ex;
    }
    catch (const char* ex)
    {
        return ex;
    }
    catch (...)
    {
        return "unknown exception";
    }
};

namespace maybe_detail
{

template <class T>
struct is_maybe : std::false_type
{
};

template <class T>
struct is_maybe<maybe<T>> : std::true_type
{
};

template <class T>
struct has_value_mixin
{
    constexpr bool has_value() const noexcept
    {
        const auto& self = static_cast<const maybe<T>&>(*this);
        return static_cast<bool>(self);
    }
};

template <class T>
struct and_then_mixin
{
    template <class Func, class FuncResult = std::invoke_result_t<Func, const T&>, class Result = FuncResult>
    constexpr auto and_then(Func&& func) const& -> Result
    {
        static_assert(is_maybe<FuncResult>::value, "and_then: function result type needs to be of `maybe<T>` type");
        const auto& self = static_cast<const maybe<T>&>(*this);
        return self  //
                   ? Result{ std::invoke(std::forward<Func>(func), self.value()) }
                   : Result{};
    }

    template <class Func, class FuncResult = std::invoke_result_t<Func, T&&>, class Result = FuncResult>
    constexpr auto and_then(Func&& func) && -> Result
    {
        static_assert(is_maybe<FuncResult>::value, "and_then: function result type needs to be of `maybe<T>` type");
        auto& self = static_cast<maybe<T>&>(*this);
        return self  //
                   ? Result{ std::invoke(std::forward<Func>(func), std::move(self).value()) }
                   : Result{};
    }
};

template <class T>
struct and_then_mixin<T&>
{
    template <class Func, class FuncResult = std::invoke_result_t<Func, T&>, class Result = FuncResult>
    constexpr auto and_then(Func&& func) const& -> Result
    {
        static_assert(is_maybe<FuncResult>::value, "and_then: function result type needs to be of `maybe<T>` type");
        const auto& self = static_cast<const maybe<T>&>(*this);
        return self  //
                   ? Result{ std::invoke(std::forward<Func>(func), self.value()) }
                   : Result{};
    }
};

template <class T>
struct transform_mixin
{
    template <class Func, class FuncResult = std::invoke_result_t<Func, const T&>, class Result = maybe<FuncResult>>
    constexpr auto transform(Func&& func) const& -> Result
    {
        const auto& self = static_cast<const maybe<T>&>(*this);
        return self  //
                   ? Result{ std::invoke(std::forward<Func>(func), self.value()) }
                   : Result{};
    }

    template <class Func, class FuncResult = std::invoke_result_t<Func, T&&>, class Result = maybe<FuncResult>>
    constexpr auto transform(Func&& func) && -> Result
    {
        auto& self = static_cast<maybe<T>&>(*this);
        return self  //
                   ? Result{ std::invoke(std::forward<Func>(func), std::move(self).value()) }
                   : Result{};
    }
};

template <class T>
struct transform_mixin<T&>
{
    template <class Func, class FuncResult = std::invoke_result_t<Func, T&>, class Result = maybe<FuncResult>>
    constexpr auto transform(Func&& func) const& -> Result
    {
        const auto& self = static_cast<const maybe<T>&>(*this);
        return self  //
                   ? Result{ std::invoke(std::forward<Func>(func), self.value()) }
                   : Result{};
    }
};

template <class T>
struct or_else_mixin
{
    template <
        class Func,
        class FuncResult = std::invoke_result_t<Func>,
        class Result = std::conditional_t<std::is_void_v<FuncResult>, maybe<T>, FuncResult>>
    constexpr auto or_else(Func&& func) const& -> Result
    {
        const auto& self = static_cast<const maybe<T>&>(*this);
        if constexpr (std::is_void_v<FuncResult>)
        {
            return self  //
                       ? Result{ self }
                       : (std::invoke(std::forward<Func>(func)), Result{ self });
        }
        else
        {
            static_assert(is_maybe<FuncResult>::value, "or_else: function result type needs to be of `maybe<T>` type");
            return self  //
                       ? Result{ self }
                       : Result{ std::invoke(std::forward<Func>(func)) };
        }
    }

    template <
        class Func,
        class FuncResult = std::invoke_result_t<Func>,
        class Result = std::conditional_t<std::is_void_v<FuncResult>, maybe<T>, FuncResult>>
    constexpr auto or_else(Func&& func) && -> Result
    {
        const auto& self = static_cast<const maybe<T>&>(*this);
        if constexpr (std::is_void_v<FuncResult>)
        {
            return self  //
                       ? Result{ std::move(self) }
                       : (std::invoke(std::forward<Func>(func)), Result{ std::move(self) });
        }
        else
        {
            static_assert(is_maybe<FuncResult>::value, "or_else: function result type needs to be of `maybe<T>` type");
            return self  //
                       ? Result{ std::move(self) }
                       : Result{ std::invoke(std::forward<Func>(func)) };
        }
    }
};

template <class T>
struct filter_mixin
{
    template <class Pred>
    constexpr auto filter(Pred&& pred) const& -> maybe<T>
    {
        const auto& self = static_cast<const maybe<T>&>(*this);
        return std::invoke(std::forward<Pred>(pred), self.value())  //
                   ? maybe<T>{ self }
                   : maybe<T>{};
    }

    template <class Pred>
    constexpr auto filter(Pred&& pred) && -> maybe<T>
    {
        auto& self = static_cast<maybe<T>&>(*this);
        return std::invoke(std::forward<Pred>(pred), self.value())  //
                   ? maybe<T>{ std::move(self) }
                   : maybe<T>{};
    }
};

}  // namespace maybe_detail

template <class T>
struct maybe : maybe_detail::has_value_mixin<T>,
               maybe_detail::and_then_mixin<T>,
               maybe_detail::transform_mixin<T>,
               maybe_detail::or_else_mixin<T>,
               maybe_detail::filter_mixin<T>
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

private:
    std::optional<value_type> m_storage;
};

template <class T>
struct maybe<T&> : maybe_detail::has_value_mixin<T&>,
                   maybe_detail::and_then_mixin<T&>,
                   maybe_detail::transform_mixin<T&>,
                   maybe_detail::or_else_mixin<T&>,
                   maybe_detail::filter_mixin<T&>
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

}  // namespace core
