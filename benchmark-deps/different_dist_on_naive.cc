#include <fstream>
#include <iterator>
#include <filesystem>
#include <ranges>

#include "csv2/writer.hpp"

#include "naive_lru.h"
#include "fifo_hybrid_lru.h"
#include "sampling_lru.h"
#include "keys_generator.h"
#include "distributions.h"
#include "benchmark_cases.h"

namespace fs = ::std::filesystem;
namespace r = ::std::ranges;
namespace rv = ::std::ranges::views;

namespace nbtlru
{

template<template<typename> typename Dist>
void different_dist_worker(auto& results_out, auto cache_initer)
{
    size_t hits{}, misses{};

    ::std::vector<size_t> hitss;
    ::std::vector<size_t> missess;

    for (size_t i{}; i < 30; ++i)
    {
        auto cache = cache_initer();
        for (auto k : gen<Dist, key_t>(benchmark_scale(), true))
            benchmark_loop_body(cache, k, hits, misses, false);
        hitss.push_back(hits);
        missess.push_back(misses);
        ::std::cout << "iteration " << i << " complete." << ::std::endl;
    }

    hits = r::fold_left_first(hitss, ::std::plus{}).value() / 30.0;
    misses = r::fold_left_first(missess, ::std::plus{}).value() / 30.0;

    results_out.emplace_back(::std::vector{
        ::std::string(Dist<key_t>::name()),
        ::std::to_string(hits), 
        ::std::to_string(misses), 
        ::std::to_string(hits / double(hits + misses))
    });
};

static void different_dist_helper(const fs::path& output_filename, auto cache_initer)
{
    ::std::cout << "Start profiling and generating " << output_filename << ::std::endl;
    ::std::vector<::std::vector<::std::string>> results{ 
        { "dist", "hits", "misses", "hit-ratio" }, 
    };
    
    ::std::cout << "Zipfian" << ::std::endl;
    different_dist_worker<  zipf        >(results, cache_initer);

    ::std::cout << "Uniform" << ::std::endl;
    different_dist_worker<  uniform     >(results, cache_initer);

    ::std::ofstream ofs{ output_filename };
    csv2::Writer<csv2::delimiter<','>> w(ofs);
    w.write_rows(results);
    ::std::cout << "Complete profile and generate " << output_filename << ::std::endl;
}

void different_dist()
{
    different_dist_helper("different_dist_on_naive.csv", []{ 
        return nbtlru::naive_lru<key_t, value_t>(benchmark_cache_size());
    });
    different_dist_helper("different_dist_on_fifo-hybrid.csv", []{ 
        return nbtlru::fifo_hybrid_lru<key_t, value_t>(benchmark_cache_size());
    });
    different_dist_helper("different_dist_on_sampling.csv", []{ 
        return nbtlru::sampling_lru<key_t, value_t>(benchmark_cache_size());
    });
}

} // namesapce nbtlru
