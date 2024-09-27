#include "gtest/gtest.h"

#include "fifo_hybrid_lru/fifo_hybrid_lru.h"

#include <string>

using namespace fhl;

TEST(fifo_hybrid_lru, basic)
{
    fifo_hybrid_lru<size_t, size_t> cache(10000);
    for (size_t i{}; i < cache.capacity() + 1; ++i)
        cache.put(i, i);

    ASSERT_TRUE(cache.get(1) == nullptr);
}
