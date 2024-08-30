#include <random>
#include <print>
#include <generator>

#include "benchmark/benchmark.h"
#include "toolpex/lru_cache.h"

#include "dirty_zipfian_int_distribution.h"

namespace 
{
    constexpr size_t g_scale{ 1'000'000 };
    constexpr size_t g_cachesz{ 1 << 16 };

    struct value_t { uint64_t dummy[2]; };
    using key_t = uint64_t;

    ::std::generator<key_t> keys(size_t scale = g_scale)
    {
        ::std::mt19937 rng(42);
        dirtyzipf::dirty_zipfian_int_distribution<key_t> dist(1, scale);
        for (size_t i{}; i < scale; ++i)
        {
            co_yield dist(rng);
        }
    }

    void test_toolpex()
    {
        toolpex::lru_cache<key_t, value_t> cache(g_cachesz);

        size_t hits{}, misses{};
        
        for (const auto& k : keys())
        {
            if (auto val_opt = cache.get(k); val_opt.has_value())
            {
                ++hits;
            }
            else
            {
                cache.put(k, value_t{});
                ++misses;
            }
        }
        
        ::std::print("hits: {}, miesses:{}\n hits ratio:{}%\n", 
                     hits, misses, 
                     static_cast<double>(hits) / g_scale * 100);
    }
}

int main()
{
    test_toolpex();   
    return 0;   
}
