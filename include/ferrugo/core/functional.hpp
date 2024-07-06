#pragma once

#include <functional>

namespace ferrugo
{
namespace core
{

template <class T>
using producer_t = std::function<T()>;

template <class... Args>
using action_t = std::function<void(Args...)>;

template <class T>
using applier_t = action_t<T&>;

}  // namespace core
}  // namespace ferrugo
