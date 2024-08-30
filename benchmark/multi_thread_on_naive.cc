#include <latch>
#include <thread>
#include <chrono>
#include <future>
#include <ranges>
#include <vector>
#include <algorithm>

#include "rustex.h"
#include "csv2/writer.hpp"

#include "naive_lru.h"
#include "benchmark_cases.h"
#include "distributions.h"
#include "keys_generator.h"

namespace r = ::std::ranges;
namespace rv = r::views;
namespace t = ::std::chrono;

namespace nbtlru
{

static rustex::mutex<naive_lru<key_t, value_t>> cache(benchmark_cache_size());
 
t::nanoseconds worker(::std::latch& l)
{
    l.arrive_and_wait();
    const auto tp = tic();
    [[maybe_unused]] size_t hits{}, misses{};

    for (auto k : gen<zipf, key_t>(benchmark_scale()))
    {
        auto h = cache.lock_mut();
        auto& cache_ref = *h;
        benchmark_loop_body(cache_ref, k, hits, misses);
    }

    return toc(tp);
}

t::nanoseconds multi_threads_on_naive_helper(size_t thrnum)
{
    ::std::latch l{ thrnum };

    ::std::vector<::std::future<t::nanoseconds>> elapses;
    for (size_t i{}; i < thrnum; ++i)
    {
        elapses.emplace_back(::std::async(::std::launch::async, [&]{ return worker(l); }));
    }

    return r::fold_left_first(
        elapses | rv::transform([](auto&& future) { return future.get(); }),  
        ::std::plus{}
    ).value() / thrnum;
}

void multi_threads_on_naive(size_t thrnum)
{
    auto result_rows = rv::iota(1ull, thrnum + 1) 
        | rv::transform(multi_threads_on_naive_helper)
        | rv::enumerate
        | rv::transform([](auto&& p){ 
              auto [i, val] = p;
              return ::std::vector{ 
                  ::std::to_string(unsigned(i + 1)), 
                  ::std::to_string(t::duration_cast<t::milliseconds>(val).count()) 
              }; 
          })
        | r::to<::std::vector>()
        ;

    ::std::ofstream ofs{"multi_threads_on_naive.csv"};
    csv2::Writer w(ofs);
    w.write_row(::std::vector<::std::string>{"thrnum", "cost"});
    w.write_rows(result_rows);
}

} // namespace nbtlru
