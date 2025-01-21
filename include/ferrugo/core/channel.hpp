#pragma once

#include <condition_variable>
#include <list>
#include <optional>
#include <thread>

namespace ferrugo
{
namespace core
{

namespace detail
{

template <class Queue>
class basic_channel
{
public:
    using queue_type = Queue;

    bool empty() const
    {
        std::unique_lock<std::mutex> lock{ m_mutex };
        return m_queue.empty();
    }

protected:
    basic_channel() : m_queue(), m_mutex{}, m_condition_var{}
    {
    }

    queue_type m_queue;
    mutable std::mutex m_mutex;
    std::condition_variable m_condition_var;
};

}  // namespace detail

template <class T, class Queue = std::list<T>>
class ichannel : public virtual detail::basic_channel<Queue>
{
private:
    using base_type = detail::basic_channel<Queue>;

public:
    ichannel() = default;

    ichannel(const ichannel&) = delete;
    ichannel& operator=(const ichannel&) = delete;

    template <class R, class P>
    std::optional<T> try_pop(std::chrono::duration<R, P> timeout)
    {
        std::unique_lock<std::mutex> lock{ this->m_mutex };

        if (!this->m_condition_var.wait_for(lock, timeout, [&]() { return !this->m_queue.empty(); }))
        {
            return {};
        }

        return do_pop();
    }

    T pop()
    {
        std::unique_lock<std::mutex> lock{ this->m_mutex };

        this->m_condition_var.wait(lock, [&]() { return !this->m_queue.empty(); });
        return do_pop();
    }

private:
    T do_pop()
    {
        T item = std::move(this->m_queue.front());
        this->m_queue.pop_front();
        return item;
    }
};

template <class T, class Queue = std::list<T>>
class ochannel : public virtual detail::basic_channel<Queue>
{
private:
    using base_type = detail::basic_channel<Queue>;

public:
    ochannel() = default;

    ochannel(const ochannel&) = delete;
    ochannel& operator=(const ochannel&) = delete;

    void push(T item)
    {
        std::unique_lock<std::mutex> lock{ this->m_mutex };

        this->m_queue.push_back(std::move(item));
        this->m_condition_var.notify_one();
    }
};

template <class T, class Queue = std::list<T>>
class iochannel : public ichannel<T, Queue>, public ochannel<T, Queue>
{
public:
    iochannel() = default;

    iochannel(const iochannel&) = delete;
    iochannel& operator=(const iochannel&) = delete;
};

template <class T, class Queue = std::list<T>>
using channel = iochannel<T, Queue>;

}  // namespace core
}  // namespace ferrugo