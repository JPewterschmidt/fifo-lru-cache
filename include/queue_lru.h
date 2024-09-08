#ifndef NBTLRU_QUEUE_LRU_H
#define NBTLRU_QUEUE_LRU_H

#include <atomic>
#include <utility>
#include <cstddef>
#include <unordered_map>
#include <immintrin.h>
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
class queue_lru 
{
public:
    using kv_type = ::std::pair<const KeyType, MappedType>;

    using kv_sptr = ::std::shared_ptr<kv_type>;
    using a_kv_sptr = ::std::atomic<kv_sptr>;
    using sa_kv_sptr = ::std::shared_ptr<a_kv_sptr>;
    using wa_kv_sptr = ::std::weak_ptr<a_kv_sptr>;

    using queue_type = moodycamel::ConcurrentQueue<sa_kv_sptr>;
    using hashmap_type = libcuckoo::cuckoohash_map<KeyType, sa_kv_sptr, Hash, KeyEq>;
    using result_type = ::std::shared_ptr<MappedType>;

private:
    size_t m_capacity{};
    size_t m_evict_thresh{};
    ::std::atomic_size_t m_size{};

    queue_type m_queue;
    hashmap_type m_hash;

public:
    queue_lru(size_t capacity, double evict_thresh_ratio = 1)
        : m_capacity{ capacity }, 
          m_evict_thresh{ static_cast<size_t>(m_capacity * evict_thresh_ratio) }
    {
        if (m_evict_thresh > m_capacity) m_evict_thresh = m_capacity;
    }

    result_type get(const KeyType& k)
    {
        kv_sptr kv{};
        for (;;)
        {
            sa_kv_sptr akv{};
            
            if (!m_hash.find(k, akv)) 
                return {};

            if (!akv) continue;

            kv = akv->exchange({}, ::std::memory_order_acq_rel);
            if (kv) break;
            ::_mm_pause();
        }

        // promotion
        m_hash.update_fn(k, [&, this](auto& entry) {
            sa_kv_sptr newakv = ::std::make_shared<a_kv_sptr>();
            newakv->store(::std::move(kv), ::std::memory_order_relaxed);
            enqueue_new_node(newakv);
            assert(newakv && newakv->load());
            entry = newakv;
            return true;
        });

        return { kv, &kv->second };
    }
 
    template<typename... Args>
        requires (::std::constructible_from<MappedType, Args...>)
    result_type put(const KeyType& k, Args&&... args)
    {
        auto [akv, result] = make_node(k, ::std::forward<Args>(args)...);

        evict_if_needed();
        enqueue_new_node(akv);
        assert(akv && akv->load());
        m_hash.upsert(k, [&](auto& entry) { entry = akv; }, akv);       

        m_size.fetch_add(1, ::std::memory_order_relaxed);

        return result;
    }

    size_t size_approx() const noexcept { return m_size.load(::std::memory_order_relaxed); }
    size_t capacity() const noexcept { return m_capacity; }
    size_t evict_thresh() const noexcept { return m_evict_thresh; }

    void reset()
    {
        m_size.store(0, ::std::memory_order_relaxed);
        m_queue = {};
        m_hash.clear();
    }

private:
    void evict_if_needed()
    {
        while (size_approx() >= evict_thresh() && !evict_whatever())
            ;
    }

    bool evict_whatever()
    {
        for (;;)
        {
            sa_kv_sptr temp;
            if (!m_queue.try_dequeue(temp))
            {
                return false;
            }
            kv_sptr victim = temp->exchange({}, ::std::memory_order_acq_rel);
            if (victim)
            {
                m_hash.erase(victim->first);
                m_size.fetch_sub(1, ::std::memory_order_relaxed);
                return true;
            }
        }
        return {};
    }

    template<typename... Args>
    ::std::pair<sa_kv_sptr, result_type> 
    make_node(const KeyType& k, Args&&... args)
    {
        kv_sptr kv = ::std::make_shared<kv_type>(k, MappedType(::std::forward<Args>(args)...));
        sa_kv_sptr result = ::std::make_shared<a_kv_sptr>();
        assert(result);
        result_type mapped(kv, &kv->second);
        result->store(::std::move(kv), ::std::memory_order_relaxed);
        return { result, mapped };
    }

    void enqueue_new_node(sa_kv_sptr ptr)
    {
        m_queue.enqueue(::std::move(ptr));
    }
}; 

} // namespace nbtlru

#endif
