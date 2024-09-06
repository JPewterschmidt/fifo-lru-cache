#ifndef NBTLRU_BENCHMARK_CASES_H
#define NBTLRU_BENCHMARK_CASES_H

#include <cstddef>
#include <functional>
#include <chrono>
#include <latch>
#include <string>
#include <string_view>
#include <utility>
#include <thread>

#include <immintrin.h>

namespace nbtlru
{

struct  value_t { uint64_t dummy[2]; };
using   key_t = uint64_t;

static inline consteval size_t benchmark_scale() noexcept { return 1'000'000; }
static inline consteval size_t benchmark_cache_size() noexcept { return   100'000; }

//static inline consteval size_t benchmark_scale() noexcept { return 100; }
//static inline consteval size_t benchmark_cache_size() noexcept { return   30; }

inline void penalty(size_t n = 10)
{
    for (size_t i{}; i < n; ++i)
        ::_mm_pause();
}

void benchmark_loop_body(auto& cache, key_t k, size_t& hits, size_t& misses)
{
    using namespace ::std::chrono_literals;
    auto val_opt = cache.get(k);
    if (val_opt)
    {
        ++hits;
    }
    else
    {
        cache.put(k, value_t{});
        ++misses;
        penalty();
    }
}

using worker_type = ::std::function<::std::pair<::std::chrono::nanoseconds, double>(::std::latch&, size_t)>;

inline auto tic() { return ::std::chrono::high_resolution_clock::now(); }
::std::chrono::nanoseconds toc(auto tp) { return ::std::chrono::duration_cast<::std::chrono::nanoseconds>(tic() - tp); }

void different_dist();
void multi_threads_profiling(size_t thrnum, ::std::string_view profile_name, worker_type worker);

void multi_threads_on_naive_in_total(size_t thrnum = 12);
void multi_threads_on_lockfree_in_total(size_t thrnum = 12);
void multi_threads_on_sampling_in_total(size_t thrnum = 12);

} // namespace nbtlru

#endif
