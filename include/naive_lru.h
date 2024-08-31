#ifndef NBTLRU_NAIVE_LRU_H
#define NBTLRU_NAIVE_LRU_H

#include <list>
#include <cstddef>
#include <cassert>
#include <utility>
#include <optional>
#include <stdexcept>
#include <functional>
#include <unordered_map>
#include <memory_resource>

#include "hash.h"

namespace nbtlru
{

template<
    typename KeyType, 
    typename ValueType, 
    typename Hash = murmur_hash_x64_128_xor_shift_to_64<KeyType>, 
    typename KeyEq = ::std::equal_to<KeyType>>
class naive_lru 
{
public:
    naive_lru(size_t capacity, ::std::pmr::memory_resource* mem = nullptr) 
        : m_capacity{ capacity }, 
          m_mem{ mem ? mem : ::std::pmr::get_default_resource() }, 
          m_cache_map(typename cache_map_type::allocator_type{ m_mem }), 
          m_cache_list(typename cache_list_type::allocator_type{ m_mem })
    {
        if (capacity == 0)
        {
            throw std::invalid_argument("Capacity must be a positive integer.");
        }
    }

    void reset()
    {
        m_cache_map.clear();
        m_cache_list.clear();
    }

    bool contains(const KeyType& key) const noexcept
    {
        return m_cache_map.contains(key);
    }

    std::optional<ValueType> get(const KeyType& key) noexcept
    {
        ::std::optional<ValueType> result{};
        auto it = m_cache_map.find(key);
        if (it != m_cache_map.end())
        {
            m_cache_list.splice(m_cache_list.begin(), m_cache_list, it->second);
            return result.emplace(it->second->second);
        }
        return result; 
    }

    template<std::convertible_to<KeyType>   K, 
             std::convertible_to<ValueType> V>
    void put(K&& key, V&& value)
    {
        auto it = m_cache_map.find(key);

        if (it != m_cache_map.end())
        {
            it->second->second = std::forward<V>(value);
            m_cache_list.splice(m_cache_list.begin(), m_cache_list, it->second);
        }
        else
        {
            evict_if_needed();

            m_cache_list.emplace_front(std::forward<K>(key), std::forward<V>(value));
            m_cache_map[m_cache_list.front().first] = m_cache_list.begin();
        }
    }

    size_t capacity() const noexcept { return m_capacity; }
    size_t size()     const noexcept { return m_cache_map.size(); }
    
    void clear() noexcept
    {
        m_cache_map = {};
        m_cache_list = {};
    }

private:
    void evict_if_needed() noexcept
    {
        if (m_cache_map.size() < m_capacity) return;
        assert(!m_cache_list.empty());

        const KeyType& last_key = m_cache_list.back().first;
        m_cache_map.erase(last_key);
        m_cache_list.pop_back();
    }

private:
    using cache_list_type = std::list<
        std::pair<KeyType, ValueType>, 
        ::std::pmr::polymorphic_allocator<std::pair<KeyType, ValueType>>
    >;

    using cache_map_type = std::unordered_map<
        KeyType, 
        typename cache_list_type::iterator, 
        Hash, 
        KeyEq, 
        ::std::pmr::polymorphic_allocator<::std::pair<const KeyType, typename cache_list_type::iterator>>
    >;

    using cache_map_iterator = typename cache_map_type::iterator;

    size_t m_capacity{};
    ::std::pmr::memory_resource* m_mem{};
    cache_map_type  m_cache_map;
    cache_list_type m_cache_list;
};

} // namespace nbtlru

#endif
