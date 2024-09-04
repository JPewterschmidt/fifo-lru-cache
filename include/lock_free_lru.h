#ifndef NBTLRU_LOCK_FREE_LRU_H
#define NBTLRU_LOCK_FREE_LRU_H

#include <atomic>
#include <utility>
#include <cstddef>
#include <unordered_map>
#include "cuckoohash_map.hh"
#include "hash.h"
#undef BLOCK_SIZE
#include "concurrentqueue/concurrentqueue.h"

namespace nbtlru
{
    
template<
    typename KeyType, 
    typename MappedType, 
    typename Hash = murmur_hash_x64_128_xor_shift_to_64<KeyType>, 
    typename KeyEq = ::std::equal_to<KeyType>>
class lock_free_lru
{
public:
    using kv_type = ::std::pair<KeyType, MappedType>;
    using value_type = ::std::shared_ptr<kv_type>;
    using weak_value_type = ::std::weak_ptr<kv_type>;
    using list_type = moodycamel::ConcurrentQueue<value_type>;
    using hashmap_type = libcuckoo::cuckoohash_map<KeyType, weak_value_type, Hash, KeyEq>;
    using result_type = ::std::shared_ptr<MappedType>;

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
        weak_value_type weak_kv;
        value_type kv;
        if (m_hash.find(k, weak_kv))
        {
            kv = weak_kv.lock();
            if (!kv)
            {
                m_hash.erase(k);
                return {};
            }
            evict_whatever();
            m_list.enqueue(kv);
            return { kv, &kv->second };
        }
        return {};
    }
 
    template<typename... Args>
        requires (::std::constructible_from<MappedType, Args...>)
    result_type put(const KeyType& k, Args&&... args)
    {
        value_type kv = ::std::make_shared<kv_type>(k, MappedType(::std::forward<Args>(args)...));
        m_hash.insert(k, kv);
        evict_if_needed();
        m_list.enqueue(kv);
        m_size.fetch_add(1, ::std::memory_order_relaxed);

        return { kv, &kv->second };
    }

    size_t size_approx() const noexcept { return m_size.load(::std::memory_order_relaxed); }
    size_t capacity() const noexcept { return m_capacity; }
    size_t evict_thresh() const noexcept { return m_evict_thresh; }

private:
    void evict_if_needed()
    {
        if (size_approx() < evict_thresh())
            return;
        evict_whatever();
    }

    void evict_whatever()
    {
        value_type temp;
        if (m_list.try_dequeue(temp))
        {
            m_hash.erase(temp->first);
        }
    }
};

} // namespace nbtlru

#endif
