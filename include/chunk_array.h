#ifndef NBTLRU_CHUNK_ARRAY_H
#define NBTLRU_CHUNK_ARRAY_H

#include <mutex>
#include <list>
#include <vector>
#include <atomic>
#undef BLOCK_SIZE
#include "concurrentqueue/concurrentqueue.h"

namespace nbtlru
{
    
template<typename T, ::std::size_t chunk_size>
class chunk_array
{
public:
    using value_type = T;
    using reference = value_type&;

    const T* at(size_t index) const 
    {
        auto lk = shr_lock();
        if (index > unsafe_max_index())
            return {};
        at_impl(index)->ele_ptr();       
    }

    template<typename... Args>
    T* make_element(Args&&... args)
    {
        T* raw_space = allocate();
        return new (raw_space) T(::std::forward<Args>(args)...);
    }

    void demake_element(T* value)
    {
        value->~T();
        deallocate(value);
    }

    ::std::size_t size_approx() const noexcept { return m_new_index.load(::std::memory_order_relaxed); }

private:
    auto uni_lock() { return ::std::unique_lock{ m_expand_mutex }; }
    auto shr_lock() { return ::std::shared_lock{ m_expand_mutex }; }

    struct value_cell
    {
        ::std::byte m_buffer[sizeof(T)];
        bool m_valid{};

        T*       ele_ptr()       noexcept { return reinterpret_cast<T*>(&m_buffer[0]); }
        const T* ele_ptr() const noexcept { return reinterpret_cast<T*>(&m_buffer[0]); }
    };

    struct alignas(4096) chunk
    {
        value_cell m_cells[chunk_size];   

        value_cell*         cell_at(size_t cell_id)       { return &m_cells[cell_id]; }
        const value_cell*   cell_at(size_t cell_id) const { return &m_cells[cell_id]; }

        T*       ele_at(size_t cell_id)         { return cell_at(cell_id)->ele_ptr(); }
        const T* ele_at(size_t cell_id) const   { return cell_at(cell_id)->ele_ptr(); }
    };

    T* allocate()
    {
        value_cell* result{};
        if (m_recycle.try_dequeue(result))
        {
            return result->ele_ptr();
        }

        const size_t new_index = m_new_index.fetch_add(1, ::std::memory_order_acquire);
        while (new_index >= unsafe_max_index())
            expand();           

        auto result = at_impl(new_index);
        result->m_valid = true;
        return result->ele_ptr();
    }

    void deallocate(T* space)
    {
        value_cell* cell = reinterpret_cast<value_cell*>(space);
        cell->m_valid = false;
        m_recycle.enqueue(cell);       
    }

    value_cell* at_impl(size_t index)
    {
        const size_t chunk_id = index / chunk_size;
        const size_t cell_id = index % chunk_size;
        if (m_chunks.size() <= chunk_id) return {};
        auto& chunk = m_chunks[chunk_id];
        if (!chunk) return {};
        return chunk->cell_at(cell_id);
    }

    void expand()
    {
        auto lk = uni_lock();
        m_chunks.push_back(::std::make_unique<chunk>());
        m_max_index += chunk_size - 1;
    }

    size_t unsafe_max_index() const
    {
        return m_max_index;
    }

private:
    ::std::vector<::std::unique_ptr<chunk>> m_chunks;      
    ::std::atomic_size_t m_new_index{};
    ::std::size_t m_max_index{};
    mutable ::std::shared_mutex m_expand_mutex;
    moodycamel::ConcurrentQueue<value_cell*> m_recycle;
};

} // namespace nbtlru

#endif
