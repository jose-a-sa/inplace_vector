#include <gmock/gmock.h>

#include <qx/inplace_vector.h>

#include "inplace_vector_test_utils.h"

// Compare (==, !=, <, >, <=, >=)

TEST(InplaceVectorCompare, EqualityOperators)
{
    qx::inplace_vector<10, int> const a = {1, 2, 3};
    qx::inplace_vector<10, int> const b = {1, 2, 3};
    qx::inplace_vector<10, int> const c = {1, 2, 4};
    qx::inplace_vector<10, int> const shorter = {1, 2};

    EXPECT_TRUE(a == b);
    EXPECT_FALSE(a != b);
    EXPECT_FALSE(a == c);
    EXPECT_TRUE(a != c);
    EXPECT_FALSE(a == shorter);
    EXPECT_TRUE(a != shorter);
}

TEST(InplaceVectorCompare, RelationalOperators)
{
    qx::inplace_vector<10, int> const a = {1, 2, 3};
    qx::inplace_vector<10, int> const b = {1, 2, 3};
    qx::inplace_vector<10, int> const bigger = {1, 3};
    qx::inplace_vector<10, int> const shorter = {1, 2};

    EXPECT_TRUE(a < bigger);
    EXPECT_TRUE(bigger > a);
    EXPECT_TRUE(shorter < a); // a prefix orders first
    EXPECT_TRUE(a > shorter);

    EXPECT_FALSE(a < b);
    EXPECT_FALSE(a > b);
    EXPECT_TRUE(a <= b);
    EXPECT_TRUE(a >= b);
    EXPECT_TRUE(a <= bigger);
    EXPECT_FALSE(bigger <= a);
    EXPECT_FALSE(shorter >= a);
}

TEST(InplaceVectorCompare, EmptyEdgeCases)
{
    qx::inplace_vector<10, int> const e1;
    qx::inplace_vector<10, int> const e2;
    qx::inplace_vector<10, int> const a = {1};

    EXPECT_TRUE(e1 == e2);
    EXPECT_FALSE(e1 < e2);
    EXPECT_TRUE(e1 <= e2);
    EXPECT_TRUE(e1 < a);
    EXPECT_TRUE(a > e1);
}

TEST(InplaceVectorCompare, IgnoresUnusedCapacity)
{
    qx::inplace_vector<10, int> a = {1, 2, 3, 4, 5};
    a.resize(3);
    qx::inplace_vector<10, int> const b = {1, 2, 3};
    EXPECT_TRUE(a == b);
}

TEST(InplaceVectorCompare, NonTrivial)
{
    qx::inplace_vector<10, Tracked> const a = {1, 2, 3};
    qx::inplace_vector<10, Tracked> const b = {1, 2, 3};
    qx::inplace_vector<10, Tracked> const c = {1, 2, 4};

    EXPECT_TRUE(a == b);
    EXPECT_TRUE(a != c);
    EXPECT_TRUE(a < c);
    EXPECT_TRUE(c >= a);
}
