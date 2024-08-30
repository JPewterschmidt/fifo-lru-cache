#ifndef NBTLRU_BENCHMARK_CASES_H
#define NBTLRU_BENCHMARK_CASES_H

#include <cstdint>
#include <cstddef>

namespace nbtlru
{

struct  value_t { uint64_t dummy[2]; };
using   key_t = uint64_t;

static inline consteval size_t benchmark_scale() noexcept { return 1'000'000; }
static inline consteval size_t benchmark_cache_size() noexcept { return   100'000; }

void benchmark_loop_body(auto& cache, key_t k, size_t& hits, size_t& misses)
{
    auto val_opt = cache.get(k);
    if (val_opt.has_value())
    {
        ++hits;
    }
    else
    {
        cache.put(k, value_t{});
        ++misses;
    }
}

inline auto tic() { return ::std::chrono::high_resolution_clock::now(); }
::std::chrono::nanoseconds toc(auto tp) { return ::std::chrono::duration_cast<::std::chrono::nanoseconds>(tic() - tp); }

void different_dist_on_naive();
void multi_threads_on_naive(size_t thrnum = 12);

} // namespace nbtlru

#endif
