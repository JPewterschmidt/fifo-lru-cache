#include "benchmark_workers.h"
#include "sampling_lru.h"

namespace t = ::std::chrono;

namespace nbtlru
{

static sampling_lru<key_t, value_t> cache(benchmark_cache_size());

t::nanoseconds lockfree_worker(::std::latch& l, size_t thrnum)
{
    l.arrive_and_wait();
    const auto tp = tic();
    [[maybe_unused]] size_t hits{}, misses{};

    for (auto k : gen<zipf, key_t>(benchmark_scale() / thrnum, true))
    {
        benchmark_loop_body(cache, k, hits, misses);
    }

    return toc(tp);
}

} // namespace nbtlru
