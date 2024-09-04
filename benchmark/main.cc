#include <vector>
#include <string>
#include <generator>
#include <iostream>


#include "naive_lru.h"
#include "keys_generator.h"
#include "distributions.h"

#include "benchmark_cases.h"

using namespace nbtlru;

// The two hardest things in CS are naming and cache-validating
void foo1(int argc, char** argv)
{
    int thrnum{2};
    if (argc <= 1)
    {
        ::std::cerr << "You ain't provide no argument, gonna use 2 as the `thrnum`" << ::std::endl;
    }
    else
    {
        thrnum = ::std::atoi(argv[1]);
        ::std::cerr << "Gonna use " << thrnum << "as the `thrnum`" << ::std::endl;
    }

    for (size_t i{}; i < 10; ++i)
        multi_threads_on_naive(thrnum);
}

int main(int argc, char** argv)
{
    different_dist_on_naive();   
    multi_threads_on_naive_in_total(12);
    multi_threads_on_lockfree_in_total(12);

    return 0;   
}
