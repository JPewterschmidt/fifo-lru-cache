#include <print>
#include <vector>
#include <string>
#include <generator>

#include "dirty_zipfian_int_distribution.h"
#include "csv2/writer.hpp"

#include "naive_lru.h"
#include "keys_generator.h"


namespace 
{
    constexpr size_t g_scale{ 1'000'000 };
    constexpr size_t g_cachesz{ 1 << 16 };

    struct value_t { uint64_t dummy[2]; };
    using key_t = uint64_t;

    void differnent_dist_on_naive()
    {
        auto profling_body = [](auto gen, const ::std::string& dist_name, auto& results_out)
        {
            size_t hits{}, misses{};
            nbtlru::naive_lru<key_t, value_t> cache(g_cachesz);
            for (const auto& k : gen)
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
                dist_name, 
                ::std::to_string(hits), 
                ::std::to_string(misses)
            });
        };

        ::std::vector<::std::vector<::std::string>> results{ 
            { "dist", "hits", "misses" }, 
        };
        profling_body(nbtlru::keys::gen<dirtyzipf::dirty_zipfian_int_distribution>(g_scale), "Zipfian", results);
        profling_body(nbtlru::keys::gen<::std::uniform_int_distribution>(g_scale), "Uniform", results);

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
