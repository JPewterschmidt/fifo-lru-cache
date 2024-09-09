#include "benchmark_cases.h"
#include "benchmark_workers.h"

#include <vector>

using namespace nbtlru;

int main()
{
    ::std::vector<::std::jthread> threads;
    ::std::latch l{12};
    for (size_t i{}; i < 12; ++i)
    {
        threads.emplace_back([&]{ 
            fifo_hybrid_lru_profiling_worker(l, /*thrnum=*/12, /*enable_penalty*/false); 
        });
    }

    return 0;
}
