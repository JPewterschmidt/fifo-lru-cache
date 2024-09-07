#ifndef NBTLRU_CHUNK_ARRAY_H
#define NBTLRU_CHUNK_ARRAY_H

#include <mutex>
#include <list>
#include <vector>
#include <atomic>
#include <memory>
#include <cassert>
#include <exception>
#include <shared_mutex>
#undef BLOCK_SIZE
#include "concurrentqueue/concurrentqueue.h"

namespace nbtlru
{
    
template<typename T, ::std::size_t ChunkSize, bool EnableFixedSize = false>
class chunk_array
{
    struct value_cell
    {
        ::std::byte m_buffer[sizeof(T)];
        ::std::atomic_bool m_valid{};
        ::std::weak_ptr<value_cell> m_self;

        T*       ele_ptr()       noexcept { return reinterpret_cast<T*>(&m_buffer[0]); }
        const T* ele_ptr() const noexcept { return reinterpret_cast<const T*>(&m_buffer[0]); }
        ::std::shared_ptr<value_cell> self_sptr() { return m_self.lock(); }
        void make_valid() { m_valid.store(true, ::std::memory_order_release); }
    };

public:
    using element_sptr = ::std::shared_ptr<T>;
    using cell_sptr = ::std::shared_ptr<value_cell>;
    using value_type = T;
    using reference = value_type&;

    element_sptr at(size_t index) const noexcept
    {
        if constexpr (EnableFixedSize)
        {
            if (index > unsafe_max_index())
                return {};
            value_cell* cell_rawp = at_impl(index);
            if (!cell_rawp->m_valid.load(::std::memory_order_acquire))
            {
                return {};
            }
            auto h = cell_rawp->self_sptr().lock();
            return { h, h->ele_ptr() };
        }
        else
        {
            auto lk = shr_lock();
            if (index > unsafe_max_index())
                return {};
            value_cell* cell_rawp = at_impl(index);
            if (!cell_rawp->m_valid.load(::std::memory_order_acquire))
            {
                return {};
            }
            auto h = cell_rawp->self_sptr().lock();
            return { h, h->ele_ptr() };
        }
    }

    element_sptr at(size_t index) noexcept
    {
        if constexpr (EnableFixedSize)
        {
            if (index > unsafe_max_index())
                return {};
            value_cell* cell_rawp = const_cast<value_cell*>(at_impl(index));
            if (!cell_rawp->m_valid.load(::std::memory_order_acquire))
            {
                return {};
            }
            auto h = cell_rawp->self_sptr();
            return { h, h->ele_ptr() };
        }
        else
        {
            auto lk = shr_lock();
            if (index > unsafe_max_index())
                return {};
            value_cell* cell_rawp = const_cast<value_cell*>(at_impl(index));
            if (!cell_rawp->m_valid.load(::std::memory_order_acquire))
            {
                return {};
            }
            auto h = cell_rawp->self_sptr().lock();
            return { h, h->ele_ptr() };
        }
    }

    ::std::size_t size_approx() const noexcept { return m_new_index.load(::std::memory_order_release); }

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
        m_chunks.clear();
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


    struct alignas(4096) chunk
    {
        value_cell m_cells[ChunkSize];   

        value_cell* cell_at(size_t cell_id) noexcept { return &m_cells[cell_id]; }
        T* ele_at(size_t cell_id) noexcept { return cell_at(cell_id)->ele_ptr(); }

        const value_cell* cell_at(size_t cell_id) const noexcept { return &m_cells[cell_id]; }
        const T* ele_at(size_t cell_id) const noexcept { return cell_at(cell_id)->ele_ptr(); }
    };

    struct smart_ptr_deleter
    {
    public:
        smart_ptr_deleter(chunk_array* parent) noexcept
            : m_parent{ parent }
        {
            assert(m_parent);
        }

        void operator()(value_cell* cell) noexcept
        {
            cell->ele_ptr()->~T();
            m_parent->deallocate(cell);
        }

    private:
        chunk_array* m_parent;
    };

    value_cell* allocate()
    {
        value_cell* result_cell{};
        if (m_recycle.try_dequeue(result_cell))
        {
            return result_cell;
        }

        const size_t new_index = m_new_index.fetch_add(1, ::std::memory_order_release);
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
        return result;
    }

    void deallocate(value_cell* cell)
    {
        cell->m_valid.store(false, ::std::memory_order_release);
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
            //auto lk = uni_lock();
            m_chunks.push_back(::std::make_unique<chunk>());
            return m_max_index += ChunkSize - 1;
        }
        else
        {
            auto lk = uni_lock();
            m_chunks.push_back(::std::make_unique<chunk>());
            return m_max_index += ChunkSize - 1;
        }
    }

    size_t unsafe_max_index() const
    {
        return m_max_index;
    }

public:
    template<typename... Args>
    cell_sptr make_element(Args&&... args)
    {
        value_cell* c = allocate();
        new (c->ele_ptr()) T(::std::forward<Args>(args)...);
        cell_sptr sptr{ c, smart_ptr_deleter{ this } };
        sptr->m_self = sptr;
        return sptr;
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
