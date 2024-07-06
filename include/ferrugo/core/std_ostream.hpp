#include <optional>
#include <ostream>
#include <tuple>

namespace std
{
template <class T>
ostream& operator<<(ostream& os, const optional<T>& item)
{
    if (item)
        os << "some(" << *item << ")";
    else
        os << "none";
    return os;
}

template <class T>
ostream& operator<<(ostream& os, const reference_wrapper<T>& item)
{
    return os << item.get();
}

template <class... Args>
ostream& operator<<(ostream& os, const tuple<Args...>& item)
{
    os << "(";
    std::apply(
        [&os](const auto&... args)
        {
            auto n = 0u;
            ((os << args << (++n != sizeof...(args) ? ", " : "")), ...);
        },
        item);
    os << ")";
    return os;
}

template <class A, class B>
ostream& operator<<(ostream& os, const pair<A, B>& item)
{
    return os << "(" << item.first << ", " << item.second << ")";
}
}  // namespace std