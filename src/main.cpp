
#include <array>
#include <cmath>
#include <cstdint>
#include <cuchar>
#include <deque>
#include <ferrugo/core/channel.hpp>
#include <ferrugo/core/chrono.hpp>
#include <ferrugo/core/dimensions.hpp>
#include <ferrugo/core/error_handling.hpp>
#include <ferrugo/core/functional.hpp>
#include <ferrugo/core/iterator_range.hpp>
#include <ferrugo/core/ostream_utils.hpp>
#include <ferrugo/core/overloaded.hpp>
#include <ferrugo/core/rational.hpp>
#include <ferrugo/core/sequence.hpp>
#include <ferrugo/core/std_ostream.hpp>
#include <ferrugo/core/type_name.hpp>
#include <forward_list>
#include <fstream>
#include <functional>
#include <future>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <numeric>
#include <optional>
#include <set>
#include <typeindex>
#include <unordered_set>
#include <variant>
#include <vector>

template <class T>
struct pointer_proxy
{
    T item;

    T* operator->()
    {
        return std::addressof(item);
    }
};

template <class T>
struct collection_ref
{
    struct iterator
    {
        using reference = T;
        using value_type = std::decay_t<T>;
        using pointer
            = std::conditional_t<std::is_reference_v<reference>, std::add_pointer_t<reference>, pointer_proxy<reference>>;
        using difference_type = std::ptrdiff_t;
        using iterator_category = std::random_access_iterator_tag;

        using getter_type = reference (*)(void*, difference_type);

        void* m_owner;
        getter_type m_getter;
        difference_type m_index;

        iterator(void* owner, getter_type getter, difference_type index)
            : m_owner(owner)
            , m_getter(std::move(getter))
            , m_index(index)
        {
        }

        iterator(const iterator&) = default;
        iterator(iterator&&) noexcept = default;

        iterator& operator=(iterator other)
        {
            std::swap(m_owner, other.m_owner);
            std::swap(m_getter, other.m_getter);
            std::swap(m_index, other.m_index);
            return *this;
        }

        reference operator[](difference_type offset) const
        {
            return m_getter(m_owner, m_index + offset);
        }

        reference operator*() const
        {
            return (*this)[0];
        }

        pointer operator->() const
        {
            if constexpr (std::is_reference_v<reference>)
            {
                return &**this;
            }
            else
            {
                return pointer{ **this };
            }
        }

        iterator& operator++()
        {
            ++m_index;
            return *this;
        }

        iterator operator++(int)
        {
            iterator temp(*this);
            ++(*this);
            return temp;
        }

        iterator& operator--()
        {
            --m_index;
            return *this;
        }

        iterator operator--(int)
        {
            iterator temp(*this);
            --(*this);
            return temp;
        }

        iterator& operator+=(difference_type offset)
        {
            m_index += offset;
            return *this;
        }

        iterator& operator-=(difference_type offset)
        {
            m_index -= offset;
            return *this;
        }

        iterator operator-(difference_type offset) const
        {
            return iterator{ *this } -= offset;
        }

        friend bool operator==(const iterator& lhs, const iterator& rhs)
        {
            return lhs.m_index == rhs.m_index;
        }

        friend bool operator!=(const iterator& lhs, const iterator& rhs)
        {
            return lhs.m_index != rhs.m_index;
        }

        friend bool operator<(const iterator& lhs, const iterator& rhs)
        {
            return lhs.m_index < rhs.m_index;
        }

        friend bool operator>(const iterator& lhs, const iterator& rhs)
        {
            return lhs.m_index > rhs.m_index;
        }

        friend bool operator<=(const iterator& lhs, const iterator& rhs)
        {
            return lhs.m_index <= rhs.m_index;
        }

        friend bool operator>=(const iterator& lhs, const iterator& rhs)
        {
            return lhs.m_index >= rhs.m_index;
        }

        friend difference_type operator-(const iterator& lhs, const iterator& rhs)
        {
            return lhs.m_index - rhs.m_index;
        }
    };

    using reference = typename iterator::reference;
    using value_type = typename iterator::value_type;
    using pointer = typename iterator::pointer;
    using difference_type = typename iterator::difference_type;
    using size_type = std::size_t;
    using getter_type = typename iterator::getter_type;

    void* m_owner;
    getter_type m_getter;
    size_type m_size;

    collection_ref(void* owner, size_type size, getter_type getter)
        : m_owner(owner)
        , m_getter(std::move(getter))
        , m_size(size)
    {
    }

    iterator begin() const
    {
        return iterator{ m_owner, m_getter, 0 };
    }

    iterator end() const
    {
        return iterator{ m_owner, m_getter, ssize() };
    }

    bool empty() const
    {
        return size() == 0;
    }

    size_type size() const
    {
        return m_size;
    }

    difference_type ssize() const
    {
        return static_cast<difference_type>(size());
    }

    reference at(difference_type offset) const
    {
        return *(begin() + offset);
    }

    reference operator[](difference_type offset) const
    {
        return at(offset);
    }

    reference front() const
    {
        return at(0);
    }

    reference back() const
    {
        return at(ssize() - 1);
    }
};

int run(const std::vector<std::string_view>& args)
{
    std::vector<int> v = { 1, 2, 3, 99, 100, 999 };

    collection_ref<const int&> col{ &v, v.size(), [](void* self, std::ptrdiff_t n) -> const int& {
                                       return static_cast<const std::vector<int>*>(self)->at(n);
                                   } };

    for (const int& val : col)
    {
        std::cout << val << "\n";
    }

    std::cout << std::distance(col.begin(), col.end());

    return 0;
}

int main(int argc, char* argv[])
{
    try
    {
        return run({ argv, argv + argc });
    }
    catch (const std::exception& ex)
    {
        std::cerr << ex.what() << '\n';
        return -1;
    }
}
