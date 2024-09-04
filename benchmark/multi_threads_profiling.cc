#include <thread>
#include <atomic>
#include <random>
#include <chrono>
#include <future>
#include <ranges>
#include <vector>
#include <sstream>
#include <iostream>
#include <algorithm>

#include "csv2/writer.hpp"

#include "benchmark_cases.h"

namespace r = ::std::ranges;
namespace rv = r::views;
namespace t = ::std::chrono;

namespace nbtlru
{

t::nanoseconds multi_thread_profiling_helper(size_t thrnum, ::std::string_view profile_name, worker_type worker)
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

    ::std::cout << "multi_thread_profiling_helper(" << thrnum << ") Complete. - " << profile_name << ::std::endl;

    return result;
}

void multi_threads_profiling(size_t thrnum, ::std::string_view profile_name, worker_type worker)
{
    ::std::stringstream ss;
    ss << "multi_threads_on_" << profile_name <<  "_in_total.csv";
    ::std::string file_name = ss.str();

    ::std::ofstream ofs{ file_name };
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
            | rv::transform([&](auto&& thrnum) { 
                  return ::std::pair{ 
                      thrnum, 
                      multi_thread_profiling_helper(thrnum, profile_name, worker) 
                  }; 
              })
            | rv::transform([](auto&& p) { 
                  return ::std::pair{ 
                      p.first,  
                      ::std::to_string(t::duration_cast<t::milliseconds>(p.second).count()) 
                  }; 
              })
            | r::to<::std::vector>()
            ;
        r::sort(result_pair_rows, [](auto&& lhs, auto&& rhs){ return lhs.first < rhs.first; });
        w.write_row(result_pair_rows | rv::transform([](auto&& p){ return p.second; }) | r::to<::std::vector>());
        ::std::cout << "round " << i << " complete." << ::std::endl;
    }
}

} // namespace nbtlru
