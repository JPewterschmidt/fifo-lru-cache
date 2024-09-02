#include "gtest/gtest.h"
#include "lock_free_list.h"

#include <vector>
#include <iterator>
#include <ranges>

using namespace nbtlru;
namespace r = ::std::ranges;
namespace rv = ::std::ranges::views;

TEST(lock_free_list, basic)
{
    lock_free_list<int> ilist;
    for (int i{9}; i >= 0; --i)
        ilist.insert_front(i);   
    ::std::vector<int> ivec;
    ilist.unsafe_dup(::std::back_inserter(ivec));
    
    ASSERT_EQ(ivec, rv::iota(0, 10) | r::to<::std::vector>());
}
