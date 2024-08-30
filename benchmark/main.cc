#include <print>
#include <vector>
#include <string>
#include <generator>

#include "csv2/writer.hpp"

#include "naive_lru.h"
#include "keys_generator.h"
#include "distributions.h"

using namespace nbtlru;

namespace 
{
    constexpr size_t g_scale{ 1'000'000 };
    constexpr size_t g_cachesz{ 1 << 16 };

    struct value_t { uint64_t dummy[2]; };
    using key_t = uint64_t;

    template<template<typename> typename Dist>
    void different_dist_on_naive_helper(auto& results_out)
    {
        size_t hits{}, misses{};
        nbtlru::naive_lru<key_t, value_t> cache(g_cachesz);
        for (const auto& k : gen<Dist, key_t>(g_scale))
        {
            if (auto val_opt = cache.get(k); val_opt.has_value())
            {
                ++hits;
            }
            else
            {
                cache.put(k, value_t{});
                ++misses;
            }
        }
        results_out.emplace_back(::std::vector{
            ::std::string(Dist<key_t>::name()),
            ::std::to_string(hits), 
            ::std::to_string(misses), 
            ::std::to_string(hits / double(hits + misses))
        });
    };

    void differnent_dist_on_naive()
    {
        ::std::vector<::std::vector<::std::string>> results{ 
            { "dist", "hits", "misses", "hit-ratio" }, 
        };
        different_dist_on_naive_helper<  zipf        >(results);
        different_dist_on_naive_helper<  uniform     >(results);
        different_dist_on_naive_helper<  normal      >(results);
        different_dist_on_naive_helper<  lognormal   >(results);

        ::std::ofstream ofs{ "differnent_dist_on_naive.csv" };
        csv2::Writer<csv2::delimiter<','>> w(ofs);
        w.write_rows(results);
        ::std::cout << ::std::endl;
    }
}

int main()
{
    differnent_dist_on_naive();   
    return 0;   
}
