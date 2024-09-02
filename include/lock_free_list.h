#ifndef NBTLRU_LOCK_FREE_LIST_H
#define NBTLRU_LOCK_FREE_LIST_H

#include <atomic>
#include <memory>
#include <utility>
#include <cassert>
#include <concepts>

namespace nbtlru
{

template<typename T>
class lock_free_list
{
public:
    class node : public ::std::enable_shared_from_this<node>
    {
    public:
        node() = default;

    private:
        node(::std::unique_ptr<T> obj) noexcept
            : m_obj{ ::std::move(obj) }
        {
        }

        template<typename... Args>
            requires (::std::constructible_from<T, Args...>)
        void construct_object(Args&&... args)
        {
            m_obj = ::std::make_unique<T>(::std::forward<Args>(args)...);
        }

        template<typename> friend class lock_free_list;

        ::std::unique_ptr<T> m_obj{};
        ::std::atomic<::std::shared_ptr<node>> m_next{};
        ::std::atomic<::std::weak_ptr<node>> m_prev{};

    public:
        ::std::shared_ptr<T> element_ptr() noexcept
        {
            if (!m_obj) return {};
            return { this->shared_from_this(), m_obj.get() };
        }

        ::std::shared_ptr<const T> element_ptr() const noexcept
        {
            if (!m_obj) return {};
            return { this->shared_from_this(), m_obj.get() };
        }
    };

    using node_sptr = ::std::shared_ptr<node>;
    using node_wptr = ::std::weak_ptr<node>;

public:
    lock_free_list()
    {
        m_head = ::std::make_shared<node>();
        m_tail = ::std::make_shared<node>();
        m_head->m_next.store(m_tail, ::std::memory_order_relaxed);
        m_tail->m_prev.store(m_head, ::std::memory_order_relaxed);
    }

    ~lock_free_list() noexcept
    {
        auto cur = m_head;
        while (cur)
        {
            ::std::exchange(cur, cur->m_next.load(::std::memory_order_relaxed)).reset();
        }
    }

    lock_free_list(lock_free_list&& other) noexcept
        : m_head{ ::std::exchange(other.m_head, nullptr) }, 
          m_tail{ ::std::exchange(other.m_tail, nullptr) }
    {
    }

    lock_free_list& operator=(lock_free_list&& other) noexcept
    {
        lock_free_list temp(::std::move(*this));
        m_head = ::std::exchange(other.m_head, nullptr);
        m_tail = ::std::exchange(other.m_tail, nullptr);

        return *this;
    }

    template<typename... Args>
        requires (::std::constructible_from<T, Args...>)
    node_sptr insert_front(Args&&... args)
    {
        auto newnode = ::std::make_shared<node>();
        newnode->construct_object(::std::forward<Args>(args)...);
        insert_front(newnode);
        return newnode;
    }

    void insert_front(node_sptr new_node) noexcept
    {
        node_sptr old_front = m_head->m_next.load(::std::memory_order_acquire);
        new_node->m_next.store(old_front, ::std::memory_order_relaxed);
        new_node->m_prev.store(m_head, ::std::memory_order_relaxed);

        while (!m_head->m_next.compare_exchange_weak(
               old_front, new_node, 
               ::std::memory_order_release, ::std::memory_order_relaxed))
        {
            new_node->m_next.store(old_front, ::std::memory_order_relaxed);
        }

        old_front->m_prev.store(new_node, ::std::memory_order_release);
    }

    void detach_node(node_sptr node) noexcept
    {
        assert(node && node != m_head && node != m_tail);
        node_sptr prev_node = node->m_prev.load(::std::memory_order_acquire).lock();
        node_sptr next_node = node->m_next.load(::std::memory_order_acquire);

        if (prev_node)
        {
            prev_node->m_next.compare_exchange_weak(
                node, next_node, 
                ::std::memory_order_release, 
                ::std::memory_order_relaxed
                );
        }
        if (next_node)
        {
            next_node->m_prevq.compare_exchange_weak(
                node, prev_node, 
                ::std::memory_order_release, 
                ::std::memory_order_relaxed
                );
        }
        
        node->m_next.store(nullptr, ::std::memory_order_relaxed);
        node->m_prev.store(nullptr, ::std::memory_order_relaxed);
    }

    node_sptr detach_last_node() noexcept
    {
        node_sptr last_node{};
        node_sptr prev_node{};
        do
        {
            last_node = m_tail->m_prev.load(::std::memory_order_acquire).lock();
            if (last_node == m_head) return {};
            prev_node = last_node->m_prev.load(::std::memory_order_acquire).lock();
        }
        while ( !m_tail->owner_before(last_node) &&
                !m_tail->m_prev.compare_exchange_weak(
                    last_node, prev_node, 
                    ::std::memory_order_release, 
                    ::std::memory_order_relaxed
                    ));
        prev_node->m_next.store(m_tail, ::std::memory_order_release);
        last_node->m_next.store(nullptr, ::std::memory_order_release);
        last_node->m_prev.store(nullptr, ::std::memory_order_release);

        return last_node;
    }

    template<typename Iter>
    void unsafe_dup(Iter beg) const
    {
        auto cur = m_head->m_next.load(::std::memory_order_relaxed);
        for (Iter i = beg; cur && cur != m_tail; ++i, cur = cur->m_next.load(::std::memory_order_relaxed))
        {
            *i = *cur->m_obj;
        }
    }

private:
    node_sptr m_head;
    node_sptr m_tail;
};

} // namespace nbtlru

#endif
