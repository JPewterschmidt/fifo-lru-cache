#ifndef NBTLRU_LOCK_FREE_LRU_H
#define NBTLRU_LOCK_FREE_LRU_H

#include <atomic>
#include <utility>
#include <cstddef>
#include <unordered_map>
#include "cuckoohash_map.hh"
#include "lock_free_list.h"
#include "hash.h"

namespace nbtlru
{
    
template<
    typename KeyType, 
    typename ValueType, 
    typename Hash = murmur_hash_x64_128_xor_shift_to_64<KeyType>, 
    typename KeyEq = ::std::equal_to<KeyType>>
class lock_free_lru
{
public:
    using list_type = lock_free_list<::std::pair<KeyType, ValueType>>;
    using hashmap_type = libcuckoo::cuckoohash_map<KeyType, typename list_type::node_wptr, Hash, KeyEq>;
    using result_type = ::std::shared_ptr<ValueType>;
    using node_sptr = typename list_type::node_sptr;
    using node_wptr = typename list_type::node_wptr;

private:
    size_t m_capacity{};
    size_t m_evict_thresh{};
    ::std::atomic_size_t m_size{};
    list_type m_list;
    hashmap_type m_hash;

public:
    lock_free_lru(size_t capacity, double evict_thresh_ratio = 0.95)
        : m_capacity{ capacity }, 
          m_evict_thresh{ static_cast<size_t>(m_capacity * evict_thresh_ratio) }
    {
        if (m_evict_thresh > m_capacity) m_evict_thresh = m_capacity;
    }

    result_type get(const KeyType& k)
    {
        node_wptr wnode;
        if (!m_hash.find(k, wnode))
            return {};

        auto node = wnode.lock();
        if (!node) return {};

        auto result_kv = node->element_ptr();
        if (m_list.detach_node(node))
        {
            m_list.insert_front(node);
        }
        return { result_kv, &result_kv->second };
    }
 
    template<typename... Args>
        requires (::std::constructible_from<ValueType, Args...>)
    result_type put(const KeyType& k, Args&&... args)
    {
        auto node = m_list.insert_front(k, ValueType(::std::forward<Args>(args)...));
        this->evict_if_needed();
        m_hash.insert(k, node);

        return { node, &node->element_ptr()->second };
    }

    size_t size_approx() const noexcept { return m_size.load(::std::memory_order_relaxed); }
    size_t capacity() const noexcept { return m_capacity; }
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
};

} // namespace nbtlru

#endif
