#ifndef NBTLRU_FIFO_HYBRID_LRU_H
#define NBTLRU_FIFO_HYBRID_LRU_H

#include <utility>
#include <memory>
#include <functional>
#include <atomic>
#include <cstdint>
#include "hash.h"
#include "cuckoohash_map.hh"
#undef BLOCK_SIZE
#include "concurrentqueue/concurrentqueue.h"

namespace nbtlru
{

template<
    typename KeyType, 
    typename MappedType, 
    typename Hash = murmur_hash_x64_128_xor_shift_to_64<KeyType>, 
    typename KeyEq = ::std::equal_to<KeyType>>
class fifo_hybrid_lru
{
public:
    using kv_type = ::std::pair<KeyType, MappedType>;
    using mapped_sptr = ::std::shared_ptr<MappedType>;
    using result_type = mapped_sptr;

private:
    struct lru_element
    {
        template<typename... Args>
        lru_element(const KeyType& k, Args&&... args)
            : m_user_data{ k, MappedType(::std::forward<Args>(args)...) }
        {
        }

        const KeyType& key() const noexcept { return m_user_data.first; }

        kv_type m_user_data;
        ::std::atomic<::std::shared_ptr<::std::atomic<::std::weak_ptr<lru_element>>>> m_manage_block_ptr{};
    };
    
    using manage_block = ::std::atomic<::std::weak_ptr<lru_element>>;
    using lru_ele_sptr = ::std::shared_ptr<lru_element>;
    using manage_block_sptr = ::std::shared_ptr<manage_block>;

private:
    ::std::atomic_size_t m_size{};
    ::std::size_t m_capacity{};
    ::std::size_t m_fifo_part_capacity{};
    ::std::size_t m_evict_thresh{};
    
    using queue_type = moodycamel::ConcurrentQueue<manage_block_sptr>;

    queue_type m_fifo_policy_q;
    queue_type m_lru_policy_q;
    libcuckoo::cuckoohash_map<KeyType, lru_ele_sptr, Hash, KeyEq> m_hash;

public:
    fifo_hybrid_lru(size_t capacity, double fifo_part_ratio = 0.8, double evict_thresh_ratio = 0.95)
        : m_capacity{ capacity }, 
          m_fifo_part_capacity{ static_cast<size_t>(fifo_part_ratio * m_capacity) }, 
          m_evict_thresh{ m_capacity * evict_thresh_ratio }
    {
        if (m_capacity < 1000)
            m_capacity = 1000;
        if (m_fifo_part_capacity < 800)
            m_fifo_part_capacity = 800;
    }

    result_type get(const KeyType& k)
    {
        lru_ele_sptr ele;
        if (!m_hash.find(k, ele))
        {
            return {};
        }

        manage_block_sptr mb = ele->m_manage_block_ptr.exchange({}, ::std::memory_order_acq_rel);
        mb->store({}, ::std::memory_order_release);
        if (mb) promote(ele);
        
        return { ele, &ele->m_user_data.second };
    }

    template<typename... Args>
    mapped_sptr put(const KeyType& k, Args&&... args)
    {
        evict_if_needed();
        auto ele = make_lru_element(k, ::std::forward<Args>(args)...);
        m_hash.update(k, ele);
        auto mb = make_manage_block();
        mb->store(ele, ::std::memory_order_relaxed);
        m_fifo_policy_q.enqueue(::std::move(mb));
        balance();
        return { ele, &ele->m_user_data.second };
    }

    void erase(const KeyType& k) { m_hash.erase(k); }

    result_type get_then_erase(const KeyType& k)
    {
        lru_ele_sptr ele;
        m_hash.uprase(k, [&ele](auto& entry) { 
            ele = ::std::move(entry);
            return false;
        });
        return { ele, &ele->m_user_data.second };
    }

    size_t size_approx() const noexcept { return m_size.load(::std::memory_order_relaxed); }
    size_t capacity() const noexcept { return m_capacity; }
    size_t fifo_part_capacity() const noexcept { return m_fifo_part_capacity; }
    size_t evict_thresh() const noexcept { return m_evict_thresh; }

    void reset()
    {
        m_fifo_policy_q = queue_type();
        m_lru_policy_q = queue_type();
        m_hash.clear();
        m_size.store(0, ::std::memory_order_relaxed);
    }

private:
    void promote(lru_ele_sptr& ele) 
    {
        auto new_mb = make_manage_block();
        ele->m_manage_block_ptr.store(new_mb, ::std::memory_order_release);
        m_fifo_policy_q.enqueue(::std::move(new_mb));
    }

    manage_block_sptr make_manage_block()
    {
        return ::std::make_shared<manage_block>();
    }

    template<typename... Args>
    lru_ele_sptr make_lru_element(const KeyType& k, Args&&... args)
    {
        return ::std::make_shared<lru_element>(k, ::std::forward<Args>(args)...);
    }

    void evict_if_needed()
    {
        if (size_approx() < evict_thresh())
            return;

        evict_whatever();
    }

    void evict_whatever()
    {
        manage_block_sptr mb;
        while (m_lru_policy_q.try_dequeue(mb))
        {
            auto ele = mb->exchange({}, ::std::memory_order_acq_rel).lock();
            if (ele)
            {
                erase(ele->key());
                break;
            }
        }

        balance();
    }

    void balance()
    {
        while (m_fifo_policy_q.size_approx() >= fifo_part_capacity())
        {
            manage_block_sptr mb;
            if (!m_fifo_policy_q.try_dequeue(mb)) [[unlikely]]
            {
                return;
            }
            auto ele = mb->load(::std::memory_order_acquire).lock();
            if (ele)
            {
                ele->m_manage_block_ptr.store(mb, ::std::memory_order_release);
                m_lru_policy_q.enqueue(::std::move(mb));
                return;
            }
        }
    }
};

} // namespace nbtlru

#endif
