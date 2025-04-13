#pragma once

#include <memory>
#include <string>

#ifdef __GNUG__
#include <cxxabi.h>

#include <cstdlib>
#include <memory>

#endif

namespace ferrugo
{
namespace core
{

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

#endif

template <class T>
std::string_view type_name()
{
    static const std::string result = demangle(typeid(T).name());
    return result;
}

template <class T>
std::string_view type_name(const T&)
{
    return type_name<T>();
}
}  // namespace core
}  // namespace ferrugo
