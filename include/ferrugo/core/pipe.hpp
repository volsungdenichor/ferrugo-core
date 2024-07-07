#pragma once

#include <ferrugo/core/type_traits.hpp>
#include <functional>
#include <tuple>

namespace ferrugo
{
namespace core
{

template <class... Pipes>
struct pipe_t
{
    std::tuple<Pipes...> m_pipes;

    pipe_t(std::tuple<Pipes...> pipes) : m_pipes{ std::move(pipes) }
    {
    }

    pipe_t(Pipes... pipes) : pipe_t{ std::tuple<Pipes...>{ std::move(pipes)... } }
    {
    }

private:
    template <std::size_t I, class... Args>
    auto invoke(Args&&... args) const -> decltype(std::invoke(std::get<I>(m_pipes), std::forward<Args>(args)...))
    {
        return std::invoke(std::get<I>(m_pipes), std::forward<Args>(args)...);
    }

    template <std::size_t I, class... Args, require<(I + 1) == sizeof...(Pipes)> = 0>
    auto call(Args&&... args) const -> decltype(invoke<I>(std::forward<Args>(args)...))
    {
        return invoke<I>(std::forward<Args>(args)...);
    }

    template <std::size_t I, class... Args, require<(I + 1) < sizeof...(Pipes)> = 0>
    auto call(Args&&... args) const -> decltype(call<I + 1>(invoke<I>(std::forward<Args>(args)...)))
    {
        return call<I + 1>(invoke<I>(std::forward<Args>(args)...));
    }

public:
    template <class... Args>
    auto operator()(Args&&... args) const -> decltype(call<0>(std::forward<Args>(args)...))
    {
        return call<0>(std::forward<Args>(args)...);
    }
};

template <class... Pipes>
struct compose_t
{
    std::tuple<Pipes...> m_pipes;

    compose_t(std::tuple<Pipes...> pipes) : m_pipes{ std::move(pipes) }
    {
    }

private:
    template <std::size_t I, class... Args>
    auto invoke(Args&&... args) const -> decltype(std::invoke(std::get<I>(m_pipes), std::forward<Args>(args)...))
    {
        return std::invoke(std::get<I>(m_pipes), std::forward<Args>(args)...);
    }

    template <std::size_t I, class... Args, require<(I == 0)> = 0>
    auto call(Args&&... args) const -> decltype(invoke<0>(std::forward<Args>(args)...))
    {
        return invoke<0>(std::forward<Args>(args)...);
    }

    template <std::size_t I, class... Args, require<(I > 0)> = 0>
    auto call(Args&&... args) const -> decltype(call<I - 1>(invoke<I>(std::forward<Args>(args)...)))
    {
        return call<I - 1>(invoke<I>(std::forward<Args>(args)...));
    }

public:
    template <class... Args>
    auto operator()(Args&&... args) const -> decltype(call<sizeof...(Pipes) - 1>(std::forward<Args>(args)...))
    {
        return call<sizeof...(Pipes) - 1>(std::forward<Args>(args)...);
    }
};

namespace detail
{

template <class T>
struct is_pipeline : std::false_type
{
};

template <class... Args>
struct is_pipeline<pipe_t<Args...>> : std::true_type
{
};

template <template <class...> class Type>
struct function_sequence_fn
{
private:
    template <class Pipe>
    auto to_tuple(Pipe pipe) const -> std::tuple<Pipe>
    {
        return std::tuple<Pipe>{ std::move(pipe) };
    }

    template <class... Pipes>
    auto to_tuple(Type<Pipes...> pipe) const -> std::tuple<Pipes...>
    {
        return pipe.m_pipes;
    }

    template <class... Pipes>
    auto from_tuple(std::tuple<Pipes...> tuple) const -> Type<Pipes...>
    {
        return Type<Pipes...>{ std::move(tuple) };
    }

public:
    template <class... Pipes>
    auto operator()(Pipes&&... pipes) const -> decltype(from_tuple(std::tuple_cat(to_tuple(std::forward<Pipes>(pipes))...)))
    {
        return from_tuple(std::tuple_cat(to_tuple(std::forward<Pipes>(pipes))...));
    }
};

}  // namespace detail

static constexpr inline auto pipe = detail::function_sequence_fn<pipe_t>{};
static constexpr inline auto fn = pipe;
static constexpr inline auto compose = detail::function_sequence_fn<compose_t>{};

template <class... L, class... R>
auto operator|=(pipe_t<L...> lhs, pipe_t<R...> rhs) -> decltype(pipe(std::move(lhs), std::move(rhs)))
{
    return pipe(std::move(lhs), std::move(rhs));
}

template <class T, class... Pipes, require<!detail::is_pipeline<std::decay_t<T>>{}> = 0>
auto operator|=(T&& item, const pipe_t<Pipes...>& p) -> decltype(p(std::forward<T>(item)))
{
    return p(std::forward<T>(item));
}

}  // namespace core
}  // namespace ferrugo
