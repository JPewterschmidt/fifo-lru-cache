#include "rustex.h"
#include "naive_lru.h"
#include "benchmark_workers.h"

namespace t = ::std::chrono;

namespace nbtlru
{

static rustex::mutex<naive_lru<key_t, value_t>> cache(benchmark_cache_size());
 
t::nanoseconds naive_worker(::std::latch& l, size_t thrnum)
{
    l.arrive_and_wait();
    const auto tp = tic();
    [[maybe_unused]] size_t hits{}, misses{};

    for (auto k : gen<zipf, key_t>(benchmark_scale() / thrnum, true))
    {
        auto h = cache.lock_mut();
        auto& cache_ref = *h;
        benchmark_loop_body(cache_ref, k, hits, misses);
    }

    return toc(tp);
}

} // namespace nbtlru
