#include "gtest/gtest.h"
#include "naive_lru.h"

using namespace nbtlru;

TEST(naive_lru, basic_functionality)
{
    naive_lru<int, int> cache(2);

    cache.put(1, 10);
    cache.put(2, 20);

    EXPECT_EQ(cache.get(1).value(), 10);
    EXPECT_EQ(cache.get(2).value(), 20);

    cache.put(3, 30);

    EXPECT_FALSE(cache.get(1).has_value());

    EXPECT_EQ(cache.get(2).value(), 20);
    EXPECT_EQ(cache.get(3).value(), 30);
}

TEST(naive_lru, obverser)
{
    naive_lru<int, int> cache(10);
    ASSERT_EQ(cache.capacity(), 10);
    for (int i{}; i < 5; ++i)
    {
        cache.put(i, i * 2);
    }
    ASSERT_EQ(cache.size(), 5);
    for (int i{5}; i < 20; ++i)
    {
        cache.put(i, i * 2);
    }
    ASSERT_EQ(cache.size(), 10);
}

TEST(naive_lru, special_member_func)
{
    naive_lru<int, int> cache(10);
    for (int i{}; i < 5; ++i)
        cache.put(i, i * 2);

    const auto old_sz = cache.size();
    const auto old_capa = cache.capacity();
    auto new_cache = ::std::move(cache);
    
    ASSERT_EQ(new_cache.size(), old_sz);
    ASSERT_EQ(new_cache.capacity(), old_capa);
    ASSERT_EQ(cache.size(), 0);

    auto copied = new_cache;
    ASSERT_EQ(copied.size(), new_cache.size());
    ASSERT_EQ(copied.size(), new_cache.size());
    ASSERT_NE(new_cache.size(), 0);
}

TEST(naive_lru, clear)
{
    naive_lru<int, int> cache(10);
    for (int i{}; i < 5; ++i)
        cache.put(i, i * 2);

    ASSERT_EQ(cache.size(), 5);
    cache.clear();
    ASSERT_EQ(cache.size(), 0);
}

