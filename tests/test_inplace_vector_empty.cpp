#include <gmock/gmock.h>

#include <qx/inplace_vector.h>

#include "inplace_vector_test_utils.h"

#include <iterator>
#include <list>
#include <sstream>
#include <stdexcept>
#include <type_traits>
#include <vector>

using S = qx::inplace_vector<0, int>;
using TS = qx::inplace_vector<0, Tracked>;

TEST(InplaceVectorEmpty, IsEmptyType)
{
    EXPECT_TRUE(std::is_empty_v<S>);
    EXPECT_TRUE(std::is_empty_v<TS>);
    EXPECT_TRUE(std::is_trivially_copyable_v<TS>);
}

TEST(InplaceVectorEmpty, CapacityAndState)
{
    S s;
    EXPECT_TRUE(s.empty());
    EXPECT_EQ(s.size(), 0u);
    EXPECT_EQ(s.capacity(), 0u);
    EXPECT_EQ(s.max_size(), 0u);
}

TEST(InplaceVectorEmpty, ConstructorsAcceptZero)
{
    EXPECT_TRUE(S(0).empty());
    EXPECT_TRUE(S(0, 1).empty());
    EXPECT_TRUE(S{}.empty());

    std::vector<int> none;
    EXPECT_TRUE(S(none.begin(), none.end()).empty());
    EXPECT_TRUE(TS(0, Tracked(1)).empty());
}

TEST(InplaceVectorEmpty, ConstructorsFailOnNonEmpty)
{
    std::vector<int> one = {1};
    std::list<int> one_list = {1};
    std::istringstream stream("1");

    EXPECT_THROW(S(1), std::length_error);
    EXPECT_THROW(S(1, 1), std::length_error);
    EXPECT_THROW(S(one.begin(), one.end()), std::length_error);
    EXPECT_THROW(S(one_list.begin(), one_list.end()), std::length_error);
    EXPECT_THROW(S(std::istream_iterator<int>(stream), std::istream_iterator<int>()), std::length_error);
    EXPECT_THROW((S{1}), std::length_error);
    EXPECT_THROW(TS(1), std::length_error);
}

TEST(InplaceVectorEmpty, AssignmentAllowsEmpty)
{
    S s;
    S s2;
    std::vector<int> none;

    s = s2;
    s = S{};
    s = {};
    s.assign(0, 1);
    s.assign(none.begin(), none.end());
    s.assign({});
    EXPECT_TRUE(s.empty());
}

TEST(InplaceVectorEmpty, AssignmentFailsOnNonEmpty)
{
    S s;
    std::vector<int> one = {1};
    EXPECT_THROW(s = {1}, std::length_error);
    EXPECT_THROW(s.assign(1, 1), std::length_error);
    EXPECT_THROW(s.assign(one.begin(), one.end()), std::length_error);
    EXPECT_THROW(s.assign({1}), std::length_error);
}

TEST(InplaceVectorEmpty, AtThrowsOutOfRange)
{
    S s;
    S const& cs = s;
    EXPECT_THROW(s.at(0), std::out_of_range);
    EXPECT_THROW(cs.at(0), std::out_of_range);
}

TEST(InplaceVectorEmpty, Iterators)
{
    S s;
    S const& cs = s;

    EXPECT_EQ(s.begin(), s.end());
    EXPECT_EQ(cs.begin(), cs.end());
    EXPECT_EQ(s.cbegin(), s.cend());
    EXPECT_EQ(s.rbegin(), s.rend());
    EXPECT_EQ(cs.rbegin(), cs.rend());
    EXPECT_EQ(s.crbegin(), s.crend());
}

TEST(InplaceVectorEmpty, ModifiersOnEmpty)
{
    S s;
    S s2;
    std::vector<int> none;

    s.clear();
    s.resize(0);
    s.resize(0, 1);
    s.reserve(0);
    s.insert(s.begin(), 0, 1);
    s.insert(s.begin(), none.begin(), none.end());
    EXPECT_EQ(s.erase(s.begin(), s.end()), s.end());
    EXPECT_TRUE(s.empty());

    s.swap(s2);
    using std::swap;
    swap(s, s2);
    EXPECT_TRUE(s.empty());
    EXPECT_TRUE(s2.empty());

    S copy(s);
    copy = s2;
    S moved(std::move(copy));
    moved = std::move(s2);
    EXPECT_TRUE(moved.empty());
}

TEST(InplaceVectorEmpty, ModifiersFailOutOfBounds)
{
    S s;
    std::vector<int> one = {1};
    EXPECT_THROW(s.resize(1), std::length_error);
    EXPECT_THROW(s.resize(1, 1), std::length_error);
    EXPECT_THROW(s.reserve(1), std::length_error);
    EXPECT_THROW(s.push_back(1), std::length_error);
    EXPECT_THROW(s.emplace_back(1), std::length_error);
    EXPECT_THROW(s.emplace(s.begin(), 1), std::length_error);
    EXPECT_THROW(s.insert(s.begin(), 1), std::length_error);
    EXPECT_THROW(s.insert(s.begin(), 1, 1), std::length_error);
    EXPECT_THROW(s.insert(s.begin(), one.begin(), one.end()), std::length_error);
    EXPECT_THROW(s.insert(s.begin(), {1}), std::length_error);
    EXPECT_TRUE(s.empty());
}

TEST(InplaceVectorEmpty, Compare)
{
    S s;
    S s2;
    EXPECT_TRUE(s == s2);
    EXPECT_FALSE(s != s2);
    EXPECT_FALSE(s < s2);
    EXPECT_TRUE(s <= s2);
    EXPECT_FALSE(s > s2);
    EXPECT_TRUE(s >= s2);
}

TEST(InplaceVectorEmpty, ExceptionsAndContracts)
{
    EXPECT_DEATH(
        {
            S s;
            s.pop_back();
        },
        "contract violation");
    EXPECT_DEATH(
        {
            S s;
            (void)s.front();
        },
        "contract violation");
}
