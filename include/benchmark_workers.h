#ifndef NBTLRU_BENCHMARK_WORKERS_H
#define NBTLRU_BENCHMARK_WORKERS_H

#include <latch>
#include <chrono>
#include <cstddef>
#include <utility>

#include "distributions.h"
#include "benchmark_cases.h"
#include "keys_generator.h"

namespace nbtlru
{

::std::pair<::std::chrono::nanoseconds, double> naive_worker(::std::latch&, size_t);
::std::pair<::std::chrono::nanoseconds, double> lockfree_worker(::std::latch&, size_t);
::std::pair<::std::chrono::nanoseconds, double> sampling_worker(::std::latch&, size_t);

} // namespace nbtlru

#endif
