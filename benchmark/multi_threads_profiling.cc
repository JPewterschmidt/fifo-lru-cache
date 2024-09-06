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

::std::pair<t::nanoseconds, double> multi_thread_profiling_helper(size_t thrnum, ::std::string_view profile_name, worker_type worker)
{
    ::std::latch l{ thrnum };

    ::std::vector<::std::future<::std::pair<t::nanoseconds, double>>> elapses;
    for (size_t i{}; i < thrnum; ++i)
    {
        elapses.emplace_back(::std::async(::std::launch::async, [&]{ return worker(l, thrnum); }));
    }

    auto result = r::fold_left_first(
        elapses | rv::transform([](auto&& future) { return future.get(); }),  
        [](auto&& lhs, auto&& rhs) { 
            return ::std::pair{ lhs.first + rhs.first, lhs.second + rhs.second }; 
        }
    ).value();
    
    result.first /= thrnum;
    result.second /= thrnum;

    ::std::cout << "multi_thread_profiling_helper(" << thrnum << ") Complete. - " << profile_name << ::std::endl;

    return result;
}

void multi_threads_profiling(size_t thrnum, ::std::string_view profile_name, worker_type worker)
{
    ::std::stringstream ss;
    ss << "multi_threads_on_" << profile_name <<  "_latency.csv";
    ::std::string latency_file_name = ss.str();

    ss = {};
    ss << "multi_threads_on_" << profile_name <<  "_hitratio.csv";
    ::std::string hitratio_name = ss.str();

    ::std::ofstream ofs1{ latency_file_name }, ofs2{ hitratio_name };
    csv2::Writer latency_csv(ofs1), hitratio_csv(ofs2);

    auto table_head = rv::iota(1ull, thrnum + 1) 
        | rv::transform([](size_t i){ return ::std::to_string(i); }) 
        | r::to<::std::vector>()
        ;

    latency_csv.write_row(table_head);
    hitratio_csv.write_row(table_head);
    table_head = {};

    auto thrs = rv::iota(1ull, thrnum + 1) | r::to<::std::vector>();
    for (size_t i{}; i < 30; ++i)
    {
        ::std::random_device rd;
        ::std::mt19937 mt_gen(rd());
        r::shuffle(thrs, mt_gen);
        
        auto latency_hitratio_rows = thrs
            | rv::transform([&](auto&& thrnum) { 
                  return ::std::pair{ 
                      thrnum, 
                      multi_thread_profiling_helper(thrnum, profile_name, worker) 
                  }; 
              })
            | r::to<::std::vector>()
            ;
        r::sort(latency_hitratio_rows, [](auto&& lhs, auto&& rhs){ return lhs.first < rhs.first; });

        latency_csv.write_row(latency_hitratio_rows 
            | rv::transform([](auto&& p){ return ::std::to_string(t::duration_cast<t::milliseconds>(p.second.first).count()); }) 
            | r::to<::std::vector>()
            );
        hitratio_csv.write_row(latency_hitratio_rows 
            | rv::transform([](auto&& p){ return ::std::to_string(p.second.second); }) 
            | r::to<::std::vector>()
            );
        ::std::cout << "round " << i << " complete." << ::std::endl;
    }
}

} // namespace nbtlru
