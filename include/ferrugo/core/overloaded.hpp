#pragma once

namespace ferrugo
{
namespace core
{

template <class... Ts>
struct overloaded : Ts...
{
    using Ts::operator()...;
};

template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

}  // namespace core
}  // namespace ferrugo
