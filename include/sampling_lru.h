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
        bool refered() const noexcept { return m_ref_count.load(::std::memory_order_relaxed); }
        bool is_sampling() const noexcept { return m_being_sampled.test(::std::memory_order_acquire); }
        bool test_and_set_being_sampled() noexcept { return m_being_sampled.test_and_set(::std::memory_order_acq_rel); }
        void clear_being_sampled() noexcept { return m_being_sampled.clear(::std::memory_order_release); }
        uint_fast32_t lru_access_tick() const noexcept { return m_lru_access_tick.load(::std::memory_order_relaxed); }
        const KeyType& key() const noexcept { return m_key; }
        void update_access_tick(uint_fast32_t tick) { m_lru_access_tick.store(tick, ::std::memory_order_relaxed); }
    };

public:
    sampling_lru(size_t capacity, double evict_thresh_ratio = 1)
        : m_capacity{ capacity }, 
          m_evict_thresh{ static_cast<size_t>(m_capacity * evict_thresh_ratio) }, 
          m_storage{ m_capacity }
    {
        if (m_evict_thresh > m_capacity) m_evict_thresh = m_capacity;
    }

    ::std::shared_ptr<MappedType> get(const KeyType& k)
    {
        ::std::shared_ptr<lru_element> tkv{};
        if (m_hash.find(k, tkv))
        {
            if (!tkv || tkv->is_sampling())
                return {};
            tkv->update_access_tick(lru_clock_step_forward());
            return { tkv, &tkv->m_mapped };
        }
        return {};
    }

    template<typename... Args>
        requires (::std::constructible_from<MappedType, Args...>)
    ::std::shared_ptr<MappedType> put(const KeyType& k, Args&&... args)
    {
        evict_if_needed();
        auto handler = m_storage.make_element(lru_clock_step_forward(), k, ::std::forward<Args>(args)...);
        ::std::shared_ptr<lru_element> lsptr{ handler, handler->ele_ptr() };
        m_hash.insert(k, lsptr);
        handler->make_valid();
        m_size.fetch_add(1, ::std::memory_order_relaxed);
        return { lsptr, &lsptr->m_mapped };
    }

    ::std::size_t size_approx() const noexcept
    {
        return m_size.load(::std::memory_order_relaxed);
    }

    ::std::size_t evict_thresh() const noexcept
    {
        return m_evict_thresh;
    }

    void reset()
    {
        m_size.store(0, ::std::memory_order_relaxed);
        m_hash.clear();
        m_storage.reset();
        m_current_tick.store(0, ::std::memory_order_relaxed);
    }
    
private:
    // Return: Samples with age in descending order
    ::std::vector<::std::shared_ptr<lru_element>> sample(size_t n)
    {
        const auto now = lru_clock_step_forward();
        ::std::vector<::std::shared_ptr<lru_element>> result;
        size_t max = m_storage.size_approx() - 1;
        ::std::mt19937_64 rng(::std::random_device{}());
        ::std::uniform_int_distribution<size_t> dist(0, max);

        const auto sz = size_approx();
        n = n < sz ? n : sz;
        size_t i{};
        do
        {
            auto h = m_storage.at(dist(rng));
            if (!h || h->test_and_set_being_sampled())
                continue;

            ++i;
            result.emplace_back(::std::move(h));
        }
        while (i < n);
        
        ::std::ranges::sort(result, 
            [now](const ::std::shared_ptr<lru_element>& lhs, const ::std::shared_ptr<lru_element>& rhs) {
                assert(lhs && rhs);
                return ((now - lhs->lru_access_tick()) % ::std::numeric_limits<decltype(now)>::max())
                     > ((now - rhs->lru_access_tick()) % ::std::numeric_limits<decltype(now)>::max());
            });
        return result;
    }

    ::std::uint_fast32_t lru_clock_step_forward() const
    {
        return m_current_tick.fetch_add(1, ::std::memory_order_relaxed);
    }

    void evict_if_needed()
    {
        if (size_approx() < evict_thresh())
            return;

        auto victims = sample(5);
        assert(!victims.empty());

        const KeyType& key = victims.front()->key();
        m_hash.erase(key);
        for (auto& h : victims)
        {
            h->clear_being_sampled();
        }
    }

private:
    ::std::size_t m_capacity{};
    ::std::size_t m_evict_thresh{};

    mutable ::std::atomic_uint_fast32_t m_current_tick{};
    ::std::atomic_size_t m_size{};
    chunk_array<lru_element, 4096 / (sizeof(lru_element) + sizeof(bool)), true> m_storage;
    libcuckoo::cuckoohash_map<KeyType, ::std::shared_ptr<lru_element>, Hash, KeyEq> m_hash;
};

} // namesapce nbtlru

#endif
