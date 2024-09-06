#ifndef NBTLRU_CHUNK_ARRAY_H
#define NBTLRU_CHUNK_ARRAY_H

#include <mutex>
#include <list>
#include <vector>
#include <atomic>
#include <exception>
#include <shared_mutex>
#undef BLOCK_SIZE
#include "concurrentqueue/concurrentqueue.h"

namespace nbtlru
{
    
template<typename T, ::std::size_t ChunkSize, bool EnableFixedSize = false>
class chunk_array
{
public:
    using value_type = T;
    using reference = value_type&;

    const T* at(size_t index) const noexcept
    {
        if constexpr (EnableFixedSize)
        {
            auto lk = shr_lock();
            if (index > unsafe_max_index())
                return {};
            return at_impl(index)->ele_ptr();       
        }
        else
        {
            if (index > unsafe_max_index())
                return {};
            return at_impl(index)->ele_ptr();       
        }
    }

    T* at(size_t index) noexcept
    {
        if constexpr (EnableFixedSize)
        {
            auto lk = shr_lock();
            if (index > unsafe_max_index())
                return {};
            return const_cast<T*>(at_impl(index)->ele_ptr());
        }
        else
        {
            if (index > unsafe_max_index())
                return {};
            return const_cast<T*>(at_impl(index)->ele_ptr());
        }
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

    void reserve(size_t number_of_element)
    {
        while (expand() < number_of_element)
            ;
    }

    chunk_array(size_t num_reserve)
        : m_num_reserve{ num_reserve }
    {
        reserve(num_reserve);
    }

    void reset()
    {
        m_chunks = {};
        m_new_index.store(0, ::std::memory_order_relaxed);
        m_max_index = {};
        m_recycle = {};
        if (m_num_reserve)
        {
            reserve(m_num_reserve);
        }
    }

private:
    auto uni_lock() const { return ::std::unique_lock{ m_expand_mutex }; }
    auto shr_lock() const { return ::std::shared_lock{ m_expand_mutex }; }

    struct value_cell
    {
        ::std::byte m_buffer[sizeof(T)];
        bool m_valid{};

        T*       ele_ptr()       noexcept { return reinterpret_cast<T*>(&m_buffer[0]); }
        const T* ele_ptr() const noexcept { return reinterpret_cast<const T*>(&m_buffer[0]); }
    };

    struct alignas(4096) chunk
    {
        value_cell m_cells[ChunkSize];   

        value_cell* cell_at(size_t cell_id) noexcept { return &m_cells[cell_id]; }
        T* ele_at(size_t cell_id) noexcept { return cell_at(cell_id)->ele_ptr(); }

        const value_cell* cell_at(size_t cell_id) const noexcept { return &m_cells[cell_id]; }
        const T* ele_at(size_t cell_id) const noexcept { return cell_at(cell_id)->ele_ptr(); }
    };

    T* allocate()
    {
        value_cell* result_cell{};
        if (m_recycle.try_dequeue(result_cell))
        {
            return result_cell->ele_ptr();
        }

        const size_t new_index = m_new_index.fetch_add(1, ::std::memory_order_acquire);
        if constexpr (!EnableFixedSize)
        {
            while (new_index >= unsafe_max_index())
                expand();           
        }

        auto result = const_cast<value_cell*>(at_impl(new_index));
        if constexpr (EnableFixedSize)
        {
            if (!result)
            {
                ::std::cerr << "chunk_array: full and you've set EnableFixedSize = true, "
                               "qsystem won't allocate any memory. " 
                               "exiting..." << ::std::endl;
                throw ::std::bad_alloc{};
            }
        }
        result->m_valid = true;
        return result->ele_ptr();
    }

    void deallocate(T* space)
    {
        value_cell* cell = reinterpret_cast<value_cell*>(space);
        cell->m_valid = false;
        m_recycle.enqueue(cell);       
    }

    const value_cell* at_impl(size_t index) const
    {
        const size_t chunk_id = index / ChunkSize;
        const size_t cell_id = index % ChunkSize;
        if (m_chunks.size() <= chunk_id) return {};
        auto& chunk = m_chunks[chunk_id];
        if (!chunk) return {};
        return chunk->cell_at(cell_id);
    }

    size_t expand()
    {
        if constexpr (EnableFixedSize)
        {
            auto lk = uni_lock();
            m_chunks.push_back(::std::make_unique<chunk>());
            return m_max_index += ChunkSize - 1;
        }
        else
        {
            m_chunks.push_back(::std::make_unique<chunk>());
            return m_max_index += ChunkSize - 1;
        }
    }

    size_t unsafe_max_index() const
    {
        return m_max_index;
    }

private:
    ::std::vector<::std::unique_ptr<chunk>> m_chunks;      
    ::std::atomic_size_t m_new_index{};
    ::std::size_t m_max_index{};
    ::std::size_t m_num_reserve{};
    mutable ::std::shared_mutex m_expand_mutex;
    moodycamel::ConcurrentQueue<value_cell*> m_recycle;
};

} // namespace nbtlru

#endif
