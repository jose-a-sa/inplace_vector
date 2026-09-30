#include <gmock/gmock.h>

#include <qx/inplace_vector.h>

#include "inplace_vector_test_utils.h"

// Erase (erase, clear, pop_back)

TEST(InplaceVectorErase, EraseSingleIterator)
{
    qx::inplace_vector<10, int> v = {1, 2, 3, 4, 5};

    auto it = v.erase(v.begin() + 2); // middle
    EXPECT_THAT(v, ::testing::ElementsAre(1, 2, 4, 5));
    EXPECT_EQ(*it, 4);

    it = v.erase(v.begin()); // first
    EXPECT_THAT(v, ::testing::ElementsAre(2, 4, 5));
    EXPECT_EQ(*it, 2);

    it = v.erase(v.end() - 1); // last
    EXPECT_THAT(v, ::testing::ElementsAre(2, 4));
    EXPECT_EQ(it, v.end());
}

TEST(InplaceVectorErase, EraseSingleIteratorNonTrivial)
{
    qx::inplace_vector<10, Tracked> v = {1, 2, 3, 4, 5};

    auto it = v.erase(v.begin() + 1);
    EXPECT_THAT(v, ::testing::ElementsAre(1, 3, 4, 5));
    EXPECT_EQ(*it, 3);

    v.erase(v.end() - 1);
    EXPECT_THAT(v, ::testing::ElementsAre(1, 3, 4));
}

TEST(InplaceVectorErase, EraseIteratorRange)
{
    qx::inplace_vector<10, int> v = {1, 2, 3, 4, 5, 6};

    auto it = v.erase(v.begin() + 1, v.begin() + 3); // remove {2, 3}
    EXPECT_THAT(v, ::testing::ElementsAre(1, 4, 5, 6));
    EXPECT_EQ(*it, 4);

    it = v.erase(v.begin() + 2, v.end()); // remove the tail
    EXPECT_THAT(v, ::testing::ElementsAre(1, 4));
    EXPECT_EQ(it, v.end());

    it = v.erase(v.begin(), v.end()); // remove everything
    EXPECT_THAT(v, ::testing::IsEmpty());
    EXPECT_EQ(it, v.end());
}

TEST(InplaceVectorErase, EraseEmptyRange)
{
    qx::inplace_vector<10, Tracked> v = {1, 2, 3};
    auto it = v.erase(v.begin() + 1, v.begin() + 1);
    EXPECT_THAT(v, ::testing::ElementsAre(1, 2, 3));
    EXPECT_EQ(*it, 2);

    it = v.erase(v.end(), v.end());
    EXPECT_THAT(v, ::testing::ElementsAre(1, 2, 3));
    EXPECT_EQ(it, v.end());
}

TEST(InplaceVectorErase, EraseIteratorRangeNonTrivial)
{
    qx::inplace_vector<10, Tracked> v = {1, 2, 3, 4, 5, 6};
    v.erase(v.begin() + 1, v.begin() + 4);
    EXPECT_THAT(v, ::testing::ElementsAre(1, 5, 6));
    v.erase(v.begin(), v.end());
    EXPECT_THAT(v, ::testing::IsEmpty());
}

TEST(InplaceVectorErase, Clear)
{
    qx::inplace_vector<10, int> v = {1, 2, 3};
    v.clear();
    EXPECT_TRUE(v.empty());
    EXPECT_EQ(v.begin(), v.end());
    v.clear(); // idempotent

    v.push_back(4); // reusable after clear
    EXPECT_THAT(v, ::testing::ElementsAre(4));

    qx::inplace_vector<10, Tracked> t = {1, 2, 3};
    t.clear();
    EXPECT_THAT(t, ::testing::IsEmpty());
}

TEST(InplaceVectorErase, ExceptionsAndContracts)
{
    using V = qx::inplace_vector<10, int>;

    EXPECT_DEATH(
        {
            V v(3, 1);
            v.erase(v.end());
        },
        "contract violation");
    EXPECT_DEATH(
        {
            V v(3, 1);
            v.erase(v.begin() + 2, v.begin() + 1);
        },
        "contract violation");
}
