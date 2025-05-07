#pragma once

#include <condition_variable>
#include <deque>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <thread>

namespace ferrugo
{
namespace core
{

template <class T>
struct channel
{
    using value_type = T;
    using queue_type = std::deque<T>;

    std::size_t m_capacity;
    queue_type m_queue;
    bool m_is_closed;
    mutable std::mutex m_mutex;
    std::condition_variable m_cond_is_empty;
    std::condition_variable m_cond_is_full;

    explicit channel(std::size_t capacity = 0)
        : m_capacity(capacity)
        , m_queue()
        , m_is_closed(false)
        , m_mutex()
        , m_cond_is_empty()
        , m_cond_is_full()
    {
    }

    channel(const channel&) = delete;
    channel(channel&&) = delete;

    channel& operator=(const channel&) = delete;
    channel& operator=(channel&&) = delete;

    ~channel()
    {
        close();
    }

    void close()
    {
        std::unique_lock lock(m_mutex);
        m_is_closed = true;
        m_cond_is_empty.notify_all();
        m_cond_is_full.notify_all();
    }

    bool is_closed() const
    {
        std::scoped_lock lock(m_mutex);
        return m_is_closed;
    }

    void push(T value)
    {
        std::unique_lock lock(m_mutex);
        m_cond_is_full.wait(lock, [&]() { return m_is_closed || m_capacity == 0 || m_queue.size() < m_capacity; });

        if (m_is_closed)
        {
            throw std::runtime_error{ "sending to a closed channel" };
        }

        m_queue.push_back(value);
        m_cond_is_empty.notify_one();
    }

    template <class Rep, class Period>
    bool push(T value, std::chrono::duration<Rep, Period> timeout)
    {
        std::unique_lock lock(m_mutex);
        const bool status = m_cond_is_full.wait_for(
            lock, timeout, [&]() { return m_is_closed || m_capacity == 0 || m_queue.size() < m_capacity; });

        if (m_is_closed)
        {
            throw std::runtime_error{ "sending to a closed channel" };
        }

        if (!status)
        {
            return false;
        }

        m_queue.push_back(value);
        m_cond_is_empty.notify_one();
        return true;
    }

    std::optional<T> pop()
    {
        std::unique_lock lock(m_mutex);
        m_cond_is_empty.wait(lock, [&]() { return m_is_closed || !m_queue.empty(); });

        if (m_queue.empty())
        {
            return {};
        }

        T value = std::move(m_queue.front());
        m_queue.pop_front();
        m_cond_is_full.notify_one();
        return value;
    }

    template <class Rep, class Period>
    std::optional<T> pop(std::chrono::duration<Rep, Period> timeout)
    {
        std::unique_lock lock(m_mutex);
        m_cond_is_empty.wait_for(lock, timeout, [&]() { return m_is_closed || !m_queue.empty(); });

        if (m_queue.empty())
        {
            return {};
        }

        T value = std::move(m_queue.front());
        m_queue.pop_front();
        m_cond_is_full.notify_one();
        return value;
    }
};

template <class T>
class channel_ref_base
{
protected:
    channel_ref_base(channel<T>& ch) : m_ch{ &ch }
    {
    }

    channel<T>& get() const
    {
        return *m_ch;
    }

public:
    void close()
    {
        get().close();
    }

    bool is_closed() const
    {
        return get().is_closed();
    }

private:
    channel<T>* m_ch;
};

template <class T>
class in_channel_ref : public channel_ref_base<T>
{
public:
    in_channel_ref(channel<T>& ch) : channel_ref_base<T>{ ch }
    {
    }

    std::optional<T> pop()
    {
        return this->get().pop();
    }

    template <class Rep, class Period>
    std::optional<T> pop(std::chrono::duration<Rep, Period> timeout)
    {
        return this->get().pop(timeout);
    }
};

template <class T>
class out_channel_ref : public channel_ref_base<T>
{
public:
    out_channel_ref(channel<T>& ch) : channel_ref_base<T>{ ch }
    {
    }

    void push(T value)
    {
        return this->get().push(std::move(value));
    }

    template <class Rep, class Period>
    bool push(T value, std::chrono::duration<Rep, Period> timeout)
    {
        return this->get().push(std::move(value), timeout);
    }
};

}  // namespace core
}  // namespace ferrugo
