#include <vector>
#include <string>
#include <generator>
#include <iostream>


#include "naive_lru.h"
#include "keys_generator.h"
#include "distributions.h"

#include "benchmark_cases.h"
#include "benchmark_workers.h"

#include "sampling_lru.h"

using namespace nbtlru;

int main(int argc, char** argv)
{
    //different_dist_on_naive();   
    //multi_threads_profiling(12, "naive", naive_worker);
    //multi_threads_profiling(12, "lockfree", lockfree_worker);
    multi_threads_profiling(12, "sampling", sampling_worker);

    return 0;   
}
