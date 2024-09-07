#include "benchmark_workers.h"
#include "queue_lru.h"
#include <mutex>

namespace t = ::std::chrono;
namespace r = ::std::ranges;
namespace rv = ::std::ranges::views;

namespace nbtlru
{

static queue_lru<key_t, value_t> cache(benchmark_cache_size());
::std::mutex reset_lock;

::std::pair<t::nanoseconds, double> queue_lru_profiling_worker(::std::latch& l, size_t thrnum)
{
    {
        ::std::lock_guard lk{ reset_lock };
        cache.reset();
    }
    l.arrive_and_wait();
    const auto tp = tic();
    [[maybe_unused]] size_t hits{}, misses{};

    size_t iteration{};
    for (auto k : gen<zipf, key_t>(benchmark_scale(), true) | rv::take(benchmark_scale() / thrnum))
    {
        ++iteration;
        benchmark_loop_body(cache, k, hits, misses);
    }

    auto time_elapsed = toc(tp);
    
    return { time_elapsed, (hits / double(hits + misses)) };
}

} // namespace nbtlru
