#pragma once

#include <type_traits>

namespace ferrugo
{
namespace core
{

template <typename T>
constexpr bool should_pass_by_value = sizeof(T) <= 2 * sizeof(void*) && std::is_trivially_copy_constructible_v<T>;

template <class T>
using in_t = std::conditional_t<should_pass_by_value<T>, T const, const T&>;

template <class T>
using return_t = std::conditional_t<should_pass_by_value<T>, T, const T&>;

}  // namespace core
}  // namespace ferrugo
