#ifndef NBTLRU_SAMPLING_LRU_H
#define NBTLRU_SAMPLING_LRU_H

#include <random>
#include <atomic>
#include <cstdint>
#include <ranges>
#include <utility>
#include <cassert>
#include <algorithm>
#include "cuckoohash_map.hh"
#include "chunk_array.h"
#include "hash.h"

namespace nbtlru
{

template<
    typename KeyType, 
    typename MappedType, 
    typename Hash = murmur_hash_x64_128_xor_shift_to_64<KeyType>, 
    typename KeyEq = ::std::equal_to<KeyType>>
class sampling_lru
{
private:
    struct lru_element
    {
        template<typename KK, typename... MMs>
        lru_element(::std::uint_fast32_t tick, KK&& key, MMs&&... mapped)
            : m_lru_access_tick{ tick }, 
              m_key{ ::std::forward<KK>(key) }, 
              m_mapped(::std::forward<MMs>(mapped)...)
        {
        }

        ::std::atomic_uint_fast32_t m_lru_access_tick{};
        ::std::atomic_ptrdiff_t m_ref_count{};
        ::std::atomic_flag m_being_sampled = ATOMIC_FLAG_INIT;
        KeyType m_key;
        MappedType m_mapped;

        size_t ref() noexcept { return m_ref_count.fetch_add(1, ::std::memory_order_relaxed); }
        size_t unref() noexcept { return m_ref_count.fetch_sub(1, ::std::memory_order_acq_rel); }
        bool refered() const noexcept { return m_ref_count.load(::std::memory_order_acq_rel); }
        bool is_sampling() const noexcept { return m_being_sampled.test(::std::memory_order_acq_rel); }
    };

    class element_handler
    {
    public:
        constexpr element_handler() noexcept = default;

        element_handler(lru_element* ele, sampling_lru* parent) noexcept
            : m_ele{ ele }, m_parent{ parent }
        {
            m_ele->ref();   
        }

        ~element_handler() noexcept { reset(); }

        element_handler(element_handler&& other) noexcept 
            : m_ele{ ::std::exchange(other.m_ele, nullptr) },
              m_parent{ ::std::exchange(other.m_parent, nullptr) }
        {
        }

        element_handler& operator=(element_handler&& other) noexcept 
        {
            reset();
            m_ele = ::std::exchange(other.m_ele, nullptr);
            m_parent = ::std::exchange(other.m_parent, nullptr);
            return *this;
        }

        element_handler(const element_handler& other) noexcept 
            : m_ele{ other.m_ele },
              m_parent{ other.m_parent }
        {
            if (m_ele) m_ele->ref();
        }

        element_handler& operator=(const element_handler& other) noexcept 
        {
            reset();
            m_ele = other.m_ele;
            m_parent = other.m_parent;
            m_ele->ref();
            return *this;
        }

        void reset() noexcept 
        { 
            if (m_ele) 
            {
                if (m_ele->unref() == 1)
                {
                    m_parent->demake(m_ele);
                }
                m_ele = nullptr;
            }
        }

        const MappedType* operator ->() const noexcept { return &m_ele->m_mapped; }
        const KeyType& key() const noexcept { return m_ele->m_key; }
        ::std::uint_fast32_t lru_access_tick() const noexcept { return m_ele->m_lru_access_tick.load(::std::memory_order_acq_rel); }

        operator bool() const noexcept { return m_ele != nullptr; }

    private:
        friend class sampling_lru;
        void update_access_tick(::std::uint_fast32_t val) noexcept { m_ele->m_lru_access_tick.store(val, ::std::memory_order_acquire); }
        bool is_sampling() const noexcept { return m_ele->is_sampling(); }
        lru_element* ele() noexcept { return m_ele; }
        const lru_element* ele() const noexcept { return m_ele; }
        
    private:
        lru_element* m_ele{};
        sampling_lru* m_parent{};
    };

public:
    sampling_lru(size_t capacity, double evict_thresh_ratio = 1)
        : m_capacity{ capacity }, 
          m_evict_thresh{ static_cast<size_t>(m_capacity * evict_thresh_ratio) }, 
          m_storage{ m_capacity }
    {
        if (m_evict_thresh > m_capacity) m_evict_thresh = m_capacity;
    }

    element_handler get(const KeyType& k)
    {
        element_handler tkv{};
        if (!m_hash.find(k, tkv) || !tkv || tkv.is_sampling())
        {
            return {};
        }
        tkv.update_access_tick(lru_clock_step_forward());
        return tkv;
    }

    template<typename... Args>
        requires (::std::constructible_from<MappedType, Args...>)
    element_handler put(const KeyType& k, Args&&... args)
    {
        auto* ptr = m_storage.make_element(lru_clock_step_forward(), k, ::std::forward<Args>(args)...);
        assert(ptr);
        evict_if_needed();
        element_handler result{ ptr, this };
        m_hash.insert(k, result);
        m_size.fetch_add(1, ::std::memory_order_acquire);
        return result;
    }

    ::std::size_t size_approx() const noexcept
    {
        return m_size.load(::std::memory_order_release);
    }

    ::std::size_t evict_thresh() const noexcept
    {
        return m_evict_thresh;
    }
    
private:
    // Return: Samples with age in descending order
    ::std::vector<element_handler> sample(size_t n)
    {
        const auto now = lru_clock_step_forward();
        ::std::vector<element_handler> result;
        size_t max = m_storage.size_approx() - 1;
        ::std::mt19937_64 rng(::std::random_device{}());
        ::std::uniform_int_distribution<size_t> dist(0, max);

        const auto sz = size_approx();
        n = n < sz ? n : sz;
        size_t i{};
        while (i < n)
        {
            lru_element* ptr{};
            while (!(ptr = m_storage.at(dist(rng))))
                ;
            
            element_handler h{ ptr, this };
            if (!h || h.m_ele->m_being_sampled.test_and_set(::std::memory_order_acquire))
                continue;

            ++i;
            result.emplace_back(::std::move(h));
        }
        
        ::std::ranges::sort(result, 
            [now](const element_handler& lhs, const element_handler& rhs) {
                assert(lhs && rhs);
                return ((now - lhs.lru_access_tick()) % ::std::numeric_limits<decltype(now)>::max())
                     > ((now - rhs.lru_access_tick()) % ::std::numeric_limits<decltype(now)>::max());
            });
        return result;
    }

    void demake(lru_element* ele)
    {
        m_storage.demake_element(ele);
    }

    ::std::uint_fast32_t lru_clock_step_forward() const
    {
        return m_current_tick.fetch_add(1, ::std::memory_order_acquire);
    }

    void evict_if_needed()
    {
        if (size_approx() < evict_thresh())
            return;

        auto victims = sample(5);
        assert(!victims.empty());
        m_hash.erase(victims.front().key());
        for (auto& h : victims)
        {
            h.m_ele->m_being_sampled.clear(::std::memory_order_release);
        }
    }

private:
    ::std::size_t m_capacity{};
    ::std::size_t m_evict_thresh{};

    mutable ::std::atomic_uint_fast32_t m_current_tick{};
    ::std::atomic_size_t m_size{};
    chunk_array<lru_element, 4096 / (sizeof(lru_element) + sizeof(bool)), true> m_storage;
    libcuckoo::cuckoohash_map<KeyType, element_handler, Hash, KeyEq> m_hash;
};

} // namesapce nbtlru

#endif
