#include <print>
#include <vector>
#include <string>
#include <generator>


#include "naive_lru.h"
#include "keys_generator.h"
#include "distributions.h"

#include "benchmark_cases.h"

using namespace nbtlru;

int main()
{
    different_dist_on_naive();   
    multi_threads_on_naive();
    return 0;   
}
