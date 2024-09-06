#include "rustex.h"
#include "naive_lru.h"
#include "benchmark_workers.h"

namespace t = ::std::chrono;
namespace r = ::std::ranges;
namespace rv = ::std::ranges::views;

namespace nbtlru
{

static rustex::mutex<naive_lru<key_t, value_t>> cache(benchmark_cache_size());
 
::std::pair<t::nanoseconds, double> naive_worker(::std::latch& l, size_t thrnum)
{
    {
        auto h = cache.lock_mut();
        h->reset();
    }
    l.arrive_and_wait();
    const auto tp = tic();
    [[maybe_unused]] size_t hits{}, misses{};

    for (auto k : gen<zipf, key_t>(benchmark_scale(), true) | rv::take(benchmark_scale() / thrnum))
    {
        auto h = cache.lock_mut();
        auto& cache_ref = *h;
        benchmark_loop_body(cache_ref, k, hits, misses);
    }

    auto time_elapsed = toc(tp);
    
    return { time_elapsed, (hits / double(hits + misses)) };
}

} // namespace nbtlru
