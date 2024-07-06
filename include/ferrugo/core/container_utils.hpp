#pragma once

#include <vector>

namespace ferrugo
{
namespace core
{

namespace detail
{

struct vec_fn
{
    template <class Head, class... Tail>
    auto operator()(Head head, Tail&&... tail) const -> std::vector<Head>
    {
        return std::vector<Head>{ std::move(head), std::forward<Tail>(tail)... };
    }
};

struct concat_fn
{
    template <class T>
    static void append(std::vector<T>&)
    {
    }

    template <class T, class Head, class... Tail>
    static void append(std::vector<T>& vec, Head&& head, Tail&&... tail)
    {
        vec.insert(std::end(vec), std::begin(head), std::end(head));
        append(vec, std::forward<Tail>(tail)...);
    }

    template <class T, class... Tail>
    auto operator()(std::vector<T> head, Tail&&... tail) const -> std::vector<T>
    {
        append(head, std::forward<Tail>(tail)...);
        return head;
    }
};

}  // namespace detail

static constexpr inline auto vec = detail::vec_fn{};
static constexpr inline auto concat = detail::concat_fn{};

}  // namespace core
}  // namespace ferrugo
