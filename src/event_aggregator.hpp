#pragma once

template <class Map, class Pred>
void erase_if(Map& map, Pred pred)
{
    for (auto it = map.begin(); it != map.end();)
    {
        if (std::invoke(pred, *it))
        {
            it = map.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

struct event_aggregator_t
{
    using subscription_id_t = int;

    class context_t
    {
        bool m_should_unsubscribe = false;

    public:
        void unsubscribe()
        {
            m_should_unsubscribe = true;
        }

        friend event_aggregator_t;
    };

    using action_t = std::function<void()>;

    template <class T>
    using event_handler_t = std::function<void(context_t&, const T&)>;

    using event_handler_ptr = std::function<void(context_t&, const void*)>;

    struct subscription_info_t
    {
        subscription_id_t id;
        event_handler_ptr event_handler;
    };

    template <class E>
    struct event_handler_wrapper_t
    {
        event_handler_t<E> m_event_handler;

        void operator()(context_t& ctx, const void* ptr) const
        {
            m_event_handler(ctx, *static_cast<const E*>(ptr));
        }
    };

    subscription_id_t m_next_id = 0;
    std::multimap<std::type_index, subscription_info_t> m_subscriptions;
    ferrugo::core::channel<action_t> m_queue;
    bool m_is_running;
    std::thread m_thread;

    event_aggregator_t() : m_next_id{ 0 }, m_subscriptions{}, m_queue{}, m_is_running{ true }, m_thread{}
    {
        m_thread = std::thread{ [&]()
                                {
                                    while (m_is_running)
                                    {
                                        handle_all_enqueued_events(std::chrono::microseconds{ 100 });
                                    }
                                } };
    }

    ~event_aggregator_t()
    {
        m_is_running = false;
        m_thread.join();
    }

    template <class E>
    auto subscribe(event_handler_t<E> event_handler) -> subscription_id_t
    {
        subscription_id_t sub_id = m_next_id++;
        m_subscriptions.emplace(  //
            get_type_index<E>(),
            subscription_info_t{ sub_id, event_handler_wrapper_t<E>{ std::move(event_handler) } });
        return sub_id;
    }

    void unsubscribe(subscription_id_t id)
    {
        erase_if(m_subscriptions, [&](const auto& item) { return item.second.id == id; });
    }

    template <class E>
    void unsubscribe_all()
    {
        erase_if(m_subscriptions, [](const auto& item) { return item.first == get_type_index<E>(); });
    }

    template <class E>
    void publish_sync(const E& event)
    {
        std::vector<subscription_id_t> ids_pending_unscubscription
            = do_publish(get_type_index<E>(), static_cast<const void*>(&event));
        erase_if(
            m_subscriptions,
            [&](const auto& item)
            {
                return std::find(ids_pending_unscubscription.begin(), ids_pending_unscubscription.end(), item.second.id)
                       != ids_pending_unscubscription.end();
            });
    }

    template <class E>
    void publish_async(E event)
    {
        m_queue.push([this, e = std::move(event)]() { this->publish_sync(e); });
    }

    bool handle_enqueued_event(std::chrono::microseconds us)
    {
        const std::optional<action_t> action = m_queue.pop(us);
        if (action)
        {
            (*action)();
            return true;
        }
        return false;
    }

    void handle_all_enqueued_events(std::chrono::microseconds us)
    {
        while (true)
        {
            if (!handle_enqueued_event(us))
            {
                return;
            }
        }
    }

private:
    auto do_publish(std::type_index type, const void* event_ptr) -> std::vector<subscription_id_t>
    {
        const auto [b, e] = m_subscriptions.equal_range(type);
        std::vector<subscription_id_t> ids_pending_unscubscription;
        for (auto it = b; it != e; ++it)
        {
            auto ctx = context_t{};
            it->second.event_handler(ctx, event_ptr);
            if (ctx.m_should_unsubscribe)
            {
                ids_pending_unscubscription.push_back(it->second.id);
            }
        }
        return ids_pending_unscubscription;
    }

    template <class T>
    static constexpr auto get_type_index() -> std::type_index
    {
        return std::type_index{ typeid(T) };
    }
};