#ifndef NBTLRU_BENCHMARK_CASES_H
#define NBTLRU_BENCHMARK_CASES_H

namespace nbtlru
{

struct  value_t { uint64_t dummy[2]; };
using   key_t = uint64_t;

static inline consteval size_t benchmark_scale() noexcept { return 1'000'000; }
static inline consteval size_t benchmark_cache_size() noexcept { return   100'000; }

void different_dist_on_naive();

} // namespace nbtlru

#endif
