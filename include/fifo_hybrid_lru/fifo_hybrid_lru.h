#ifndef NBTLRU_FIFO_HYBRID_LRU_H
#define NBTLRU_FIFO_HYBRID_LRU_H

#include <utility>
#include <memory>
#include <functional>
#include <atomic>
#include <cstdint>
#include "cuckoohash_map.hh"
#undef BLOCK_SIZE
#include "concurrentqueue/concurrentqueue.h"

namespace fhl 
{

template<
    typename KeyType, 
    typename MappedType, 
    typename Hash = ::std::hash<KeyType>, 
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
    using lru_ele_wptr = ::std::weak_ptr<lru_element>;
    using manage_block_sptr = ::std::shared_ptr<manage_block>;

private:
    ::std::atomic_size_t m_size{};
    ::std::size_t m_capacity{};
    ::std::size_t m_fifo_part_capacity{};
    ::std::size_t m_evict_thresh{};
    
    using lru_queue_type = moodycamel::ConcurrentQueue<manage_block_sptr>;
    using fifo_queue_type = moodycamel::ConcurrentQueue<lru_ele_wptr>;

    fifo_queue_type m_fifo_policy_q;
    lru_queue_type m_lru_policy_q;
    libcuckoo::cuckoohash_map<KeyType, lru_ele_sptr, Hash, KeyEq> m_hash;

public:
    fifo_hybrid_lru(size_t capacity, double fifo_part_ratio = 0.7, double evict_thresh_ratio = 0.95)
        : m_capacity{ capacity }, 
          m_fifo_part_capacity{ static_cast<size_t>(fifo_part_ratio * m_capacity) }, 
          m_evict_thresh{ static_cast<size_t>(m_capacity * evict_thresh_ratio) }
    {
        if (evict_thresh_ratio <= 0.0) 
            throw ::std::invalid_argument{ "fifo_part_ratio's ctor: Your `evict_thresh_ratio` is <= 0" };

        if (m_capacity < 1000)
            m_capacity = 1000;
        if (m_fifo_part_capacity < m_capacity * fifo_part_ratio)
            m_fifo_part_capacity = m_capacity * fifo_part_ratio;
    }

    result_type get(const KeyType& k)
    {
        auto ele = phantom_get_impl(k);
        if (!ele) return {};
        promote(ele);
        return { ele, &ele->m_user_data.second };
    }

    result_type phantom_get(const KeyType& k)
    {
        auto ele = phantom_get_impl(k);
        if (!ele) return {};
        return { ele, &ele->m_user_data.second };
    }

    template<typename... Args>
    mapped_sptr put(const KeyType& k, Args&&... args)
    {
        evict_if_needed();
        auto ele = make_lru_element(k, ::std::forward<Args>(args)...);
        m_hash.upsert(k, [&] (auto& entry) { entry = ele; }, ele);
        m_fifo_policy_q.enqueue(ele);
        m_size.fetch_add(1, ::std::memory_order_relaxed);
        balance();
        return { ele, &ele->m_user_data.second };
    }

    void erase(const KeyType& k) 
    { 
        m_hash.erase(k); 
        m_size.fetch_sub(1, ::std::memory_order_relaxed);
    }

    result_type get_then_erase(const KeyType& k)
    {
        lru_ele_sptr ele;
        m_hash.uprase(k, [&ele, this](auto& entry) { 
            ele = ::std::move(entry);
            m_size.fetch_sub(1, ::std::memory_order_relaxed);
            return false;
        });
        return { ele, &ele->m_user_data.second };
    }

    size_t size_approx() const noexcept { return m_size.load(::std::memory_order_relaxed); }
    size_t capacity() const noexcept { return m_capacity; }
    size_t fifo_part_capacity() const noexcept { return m_fifo_part_capacity; }
    size_t evict_thresh() const noexcept { return m_evict_thresh; }

    void unsafe_reset()
    {
        m_fifo_policy_q = fifo_queue_type();
        m_lru_policy_q = lru_queue_type();
        m_hash.clear();
        m_size.store(0, ::std::memory_order_relaxed);
    }

private:
    lru_ele_sptr phantom_get_impl(const KeyType& k)
    {
        lru_ele_sptr ele;
        if (!m_hash.find(k, ele))
            return {};
        return ele;
    }

    void promote(lru_ele_sptr& ele) 
    {
        manage_block_sptr mb = ele->m_manage_block_ptr.exchange({}, ::std::memory_order_acq_rel);
        if (mb) 
        {
            mb->store({}, ::std::memory_order_release);
            m_fifo_policy_q.enqueue(ele);
        }
    }

    manage_block_sptr make_manage_block(lru_ele_sptr& ele)
    {
        auto result = ::std::make_shared<manage_block>();
        result->store(ele, ::std::memory_order_relaxed);
        return result;
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
    }

    void balance()
    {
        while (m_fifo_policy_q.size_approx() >= fifo_part_capacity())
        {
            lru_ele_wptr wele;
            if (!m_fifo_policy_q.try_dequeue(wele)) [[unlikely]]
            {
                return;
            }
            if (auto ele = wele.lock(); ele)
            {
                auto mb = make_manage_block(ele);
                ele->m_manage_block_ptr.store(mb, ::std::memory_order_release);
                m_lru_policy_q.enqueue(::std::move(mb));
                return;
            }
        }
    }
};

} // namespace fhl 

#endif
