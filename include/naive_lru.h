#ifndef NBTLRU_NAIVE_LRU_H
#define NBTLRU_NAIVE_LRU_H

#include <memory_resource>
#include <utility>
#include <optional>
#include <functional>
#include <cstddef>

namespace nbtlru
{

template<typename LRU_Parent>
class element_proxy 
{
public:
    using key_type = Key;
    using mapped_type = Mapped;
    using key_compare = Compare;

public:
    element_proxy(LRU_Parent* parent) noexcept
        : m_parent{ parent }
    {
        assert(parent);
    }

    // TODO: Special function elements

    template<typename MM>
    mapped_type& operator=(MM&& newvalue)
    {
        // TODO
    }

    bool was_exists() const noexcept;
    
private:
    LRU_Parent* m_parent{};
};

template<
    typename Key, 
    typename Mapped, 
    typename Compare = ::std::less<Key>>
class naive_lru
{
public:
    using key_type = Key;
    using mapped_type = Mapped;
    using key_compare = Compare;

public:
    naive_lru(::std::size_t capacity = 32, ::std::pmr::memory_resource* mem = nullptr)
        : m_capacity{ capacity }, 
          m_mem{ mem == nullptr ? ::std::pmr::get_default_resource() : mem }
    {
    }

    ~naive_lru() noexcept {}
    
    // TODO: Special member functions

    mapped_type& at(const key_type& k)
    {
        // TODO       
    }

    bool empty() const noexcept { return size() == 0; }
    ::std::size_t capacity() const noexcept { return m_capacity; }
    ::std::size_t size() const noexcept { return m_size; }
    bool is_full() const noexcept;

    auto at(const key_type& k)
    {
        return element_proxy{ /*TODO*/ };       
    }


private:
    void evict_if_needed();

    // TODO: A function that returns a pointer represent the element a frozenhot specific unit.
    

private:      
    ::std::size_t m_capacity{};
    ::std::size_t m_size{};
    ::std::pmr::memory_resource* m_mem;
};

} // namespace nbtlru

#endif
