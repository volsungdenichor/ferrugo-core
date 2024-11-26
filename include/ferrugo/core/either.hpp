#pragma once

#include <iostream>
#include <type_traits>

namespace ferrugo
{
namespace core
{

struct in_place_left_t
{
};

struct in_place_right_t
{
};

inline constexpr in_place_left_t in_place_left{};
inline constexpr in_place_right_t in_place_right{};

template <class L, class R, class = void>
struct either_storage_base
{
    union
    {
        L m_left;
        R m_right;
    };

    bool m_is_left;

    template <class... Args>
    either_storage_base(in_place_left_t, Args&&... args) : m_left(std::forward<Args>(args)...)
                                                         , m_is_left(true)
    {
    }

    template <class... Args>
    either_storage_base(in_place_right_t, Args&&... args) : m_right(std::forward<Args>(args)...)
                                                          , m_is_left(false)
    {
    }

    either_storage_base() : either_storage_base(in_place_left)
    {
    }

    ~either_storage_base()
    {
        if (m_is_left)
        {
            m_left.~L();
        }
        else
        {
            m_right.~R();
        }
    }
};

template <class L, class R>
struct either_storage_base<
    L,
    R,
    std::enable_if_t<std::is_trivially_destructible_v<L> && std::is_trivially_destructible_v<R>>>
{
    union
    {
        L m_left;
        R m_right;
    };

    bool m_is_left;

    template <class... Args>
    either_storage_base(in_place_left_t, Args&&... args) : m_left(std::forward<Args>(args)...)
                                                         , m_is_left(true)
    {
    }

    template <class... Args>
    either_storage_base(in_place_right_t, Args&&... args) : m_right(std::forward<Args>(args)...)
                                                          , m_is_left(false)
    {
    }

    either_storage_base() : either_storage_base(in_place_left)
    {
    }

    ~either_storage_base() = default;
};

template <class L, class R>
struct either_storage : either_storage_base<L, R>
{
    static_assert(std::is_default_constructible_v<L>, "default constructible left type required");

    using base_t = either_storage_base<L, R>;

    using base_t::base_t;

    ~either_storage() = default;

    constexpr bool is_left() const noexcept
    {
        return this->m_is_left;
    }

    constexpr bool is_right() const noexcept
    {
        return !is_left();
    }

    constexpr const L& get_left() const&
    {
        assert(is_left());
        return this->m_left;
    }

    constexpr L& get_left() &
    {
        assert(is_left());
        return this->m_left;
    }

    constexpr L&& get_left() &&
    {
        assert(is_left());
        return std::move(this->m_left);
    }

    constexpr const L&& get_left() const&&
    {
        assert(is_left());
        return std::move(this->m_left);
    }

    constexpr const R& get_right() const&
    {
        assert(is_right());
        return this->m_right;
    }

    constexpr R& get_right() &
    {
        assert(is_right());
        return this->m_right;
    }

    constexpr R&& get_right() &&
    {
        assert(is_right());
        return std::move(this->m_right);
    }

    constexpr const R&& get_right() const&&
    {
        assert(is_right());
        return std::move(this->m_right);
    }

    void destroy()
    {
        if (is_left())
        {
            this->m_left.~L();
        }
        else
        {
            this->m_right.~R();
        }
    }

    void reset()
    {
        construct(in_place_left);
    }

    template <class... Args>
    void construct(in_place_left_t, Args&&... args)
    {
        destroy();
        new (&this->m_left) L(std::forward<Args>(args)...);
        this->m_is_left = true;
    }

    template <class... Args>
    void construct(in_place_right_t, Args&&... args)
    {
        destroy();
        new (&this->m_right) R(std::forward<Args>(args)...);
        this->m_is_left = false;
    }
};

template <class L, class R>
void swap(either_storage<L, R>& lhs, either_storage<L, R>& rhs)
{
    if (lhs.is_left() && rhs.is_left())
    {
        L lhs_val = std::move(lhs).get_left();
        L rhs_val = std::move(rhs).get_left();
        lhs.construct(in_place_left, std::move(rhs_val));
        rhs.construct(in_place_left, std::move(lhs_val));
    }
    else if (lhs.is_right() && rhs.is_right())
    {
        R lhs_val = std::move(lhs).get_right();
        R rhs_val = std::move(rhs).get_right();
        lhs.construct(in_place_right, std::move(rhs_val));
        rhs.construct(in_place_right, std::move(lhs_val));
    }
    else if (lhs.is_left() && rhs.is_right())
    {
        L lhs_val = std::move(lhs).get_left();
        R rhs_val = std::move(rhs).get_right();
        lhs.construct(in_place_right, std::move(rhs_val));
        rhs.construct(in_place_left, std::move(lhs_val));
    }
    else if (lhs.is_right() && rhs.is_left())
    {
        R lhs_val = std::move(lhs).get_right();
        L rhs_val = std::move(rhs).get_left();
        lhs.construct(in_place_left, std::move(rhs_val));
        rhs.construct(in_place_right, std::move(lhs_val));
    }
}

template <class L, class R>
class either
{
public:
    using left_type = L;
    using right_type = R;
    using storage_type = either_storage<left_type, right_type>;

    template <class... Args>
    either(in_place_left_t, Args&&... args) : m_storage(in_place_left, std::forward<Args>(args)...)
    {
    }

    template <class... Args>
    either(in_place_right_t, Args&&... args) : m_storage(in_place_right, std::forward<Args>(args)...)
    {
    }

    either() : either(in_place_left)
    {
    }

    either(const either& other) : either()
    {
        if (other.is_left())
        {
            m_storage.construct(in_place_left, other.get_left());
        }
        else
        {
            m_storage.construct(in_place_right, other.get_right());
        }
    }

    either(either&& other) : either()
    {
        if (other.is_left())
        {
            m_storage.construct(in_place_left, std::move(other).get_left());
        }
        else
        {
            m_storage.construct(in_place_right, std::move(other).get_right());
        }
    }

    ~either() = default;

    either& operator=(either other)
    {
        swap(other);
        return *this;
    }

    template <class... Args>
    void emplace(in_place_left_t, Args&&... args)
    {
        m_storage.construct(in_place_left, std::forward<Args>(args)...);
    }

    template <class... Args>
    void emplace(in_place_right_t, Args&&... args)
    {
        m_storage.construct(in_place_right, std::forward<Args>(args)...);
    }

    void swap(either& other)
    {
        ferrugo::core::swap(m_storage, other.m_storage);
    }

    constexpr bool is_left() const noexcept
    {
        return m_storage.is_left();
    }

    constexpr bool is_right() const noexcept
    {
        return m_storage.is_right();
    }

    constexpr const left_type& get_left() const&
    {
        return m_storage.get_left();
    }

    constexpr left_type& get_left() &
    {
        return m_storage.get_left();
    }

    constexpr left_type&& get_left() &&
    {
        return std::move(m_storage).get_left();
    }

    constexpr const left_type&& get_left() const&&
    {
        return std::move(m_storage).get_left();
    }

    constexpr const right_type& get_right() const&
    {
        return m_storage.get_right();
    }

    constexpr right_type& get_right() &
    {
        return m_storage.get_right();
    }

    constexpr right_type&& get_right() &&
    {
        return std::move(m_storage).get_right();
    }

    constexpr const right_type&& get_right() const&&
    {
        return std::move(m_storage).get_right();
    }

private:
    storage_type m_storage;
};

template <class L, class R>
std::ostream& operator<<(std::ostream& os, const either<L, R>& item)
{
    if (item.is_left())
    {
        return os << "left( " << item.get_left() << " )";
    }
    else
    {
        return os << "right( " << item.get_right() << " )";
    }
}

}  // namespace core
}  // namespace ferrugo