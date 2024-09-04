#ifndef NBTLRU_BENCHMARK_WORKERS_H
#define NBTLRU_BENCHMARK_WORKERS_H

#include <latch>
#include <chrono>
#include <cstddef>

#include "distributions.h"
#include "benchmark_cases.h"
#include "keys_generator.h"

namespace nbtlru
{

::std::chrono::nanoseconds naive_worker(::std::latch&, size_t);
::std::chrono::nanoseconds lockfree_worker(::std::latch&, size_t);
::std::chrono::nanoseconds sampling_worker(::std::latch&, size_t);

} // namespace nbtlru

#endif
