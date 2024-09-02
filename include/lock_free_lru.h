#ifndef NBTLRU_LOCK_FREE_LRU_H
#define NBTLRU_LOCK_FREE_LRU_H

#include <atomic>
#include "cuckoohash_map.hh"
#include "lock_free_list.h"

namespace nbtlru
{
    
template<typename Key, typename Mapped>
class lock_free_lru
{
public:
    using list_type = lock_free_list<::std::pair<Key, Mapped>>;
    using hashmap_type = cuckoohash_map<Key, list_type::node_wptr, Hash, KeyEqual>;
    using result_type = ::std::shared_ptr<Mapped>;

public:
    lock_free_lru(size_t capasity, double evict_thresh_ratio)
        : m_capasity{ capasity }, m_evict_thresh{ capasity * ratio }
    {
        if (m_evict_thresh > m_capasity) m_evict_thresh = m_capacity;
    }

    result_type get(const Key& k)
    {
        list_type::node_wptr wnode;
        if (!m_hash.find(k, node))
            return {};

        auto node = wnode.lock();
        if (!node) return {};

        auto result_kv = node.element_ptr();
        if (m_list.detach(node))
        {
            m_list.insert_front(node);
        }
        return { result_kv, &result_kv->second };
    }

    template<typename... Args>
        requires (::std::constructible_from<Mapped, Args...>)
    result_type insert(const Key& k, Args&&... args)
    {
        auto node = m_list.insert_front(::std::forward<Args>(args)...);
        evict_if_needed();
        m_hash.insert(k, node);
        return node;
    }

    size_t size_approx() const noexcept { return m_size.load(::std::memory_order_relaxed); }
    size_t capasity() const noexcept { return m_capasity; }
    size_t evict_thresh() const noexcept { return m_evict_thresh; }

private:
    void evict_if_needed()
    {
        if (size_approx() < evict_thresh())
            return;
        auto node = m_list.detach_last_node();
        if (!node) return;
        m_hash.erase(node->element_ptr()->first);
        m_size.fetch_sub(1, ::std::memory_order_relaxed);
    }

private:
    size_t m_capasity{};
    size_t m_evict_thresh{};
    ::std::atomic_size_t m_size{};
    list_type m_list;
    hashmap_type m_hash;
};

}; // namespace nbtlru

#endif
