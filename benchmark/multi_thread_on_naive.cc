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
 
static t::nanoseconds worker(::std::latch& l, size_t thrnum)
{
    //cache.lock_mut()->reset();
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

t::nanoseconds multi_threads_on_naive(size_t thrnum)
{
    ::std::latch l{ thrnum };

    ::std::vector<::std::future<t::nanoseconds>> elapses;
    for (size_t i{}; i < thrnum; ++i)
    {
        elapses.emplace_back(::std::async(::std::launch::async, [&]{ return worker(l, thrnum); }));
    }

    const auto result = r::fold_left_first(
        elapses | rv::transform([](auto&& future) { return future.get(); }),  
        ::std::plus{}
    ).value() / thrnum;

    ::std::cout << "multi_threads_on_naive(" << thrnum << ") Complete." << ::std::endl;

    return result;
}

void multi_threads_on_naive_in_total(size_t thrnum)
{
    ::std::ofstream ofs{"multi_threads_on_naive_in_total.csv"};
    csv2::Writer w(ofs);
    w.write_row(rv::iota(1ull, thrnum + 1) 
        | rv::transform([](size_t i){ return ::std::to_string(i); }) 
        | r::to<::std::vector>()
        );
    auto thrs = rv::iota(1ull, thrnum + 1) | r::to<::std::vector>();
    for (size_t i{}; i < 30; ++i)
    {
        ::std::random_device rd;
        ::std::mt19937 mt_gen(rd());
        r::shuffle(thrs, mt_gen);
        
        auto result_pair_rows = thrs
            | rv::transform([](auto&& thrnum)   { return ::std::pair{ thrnum,   multi_threads_on_naive(thrnum) }; })
            | rv::transform([](auto&& p)        { return ::std::pair{ p.first,  ::std::to_string(t::duration_cast<t::milliseconds>(p.second).count()) }; })
            | r::to<::std::vector>()
            ;
        r::sort(result_pair_rows, [](auto&& lhs, auto&& rhs){ return lhs.first < rhs.first; });
        w.write_row(result_pair_rows | rv::transform([](auto&& p){ return p.second; }) | r::to<::std::vector>());
        ::std::cout << "round " << i << " complete." << ::std::endl;
    }
}

} // namespace nbtlru
