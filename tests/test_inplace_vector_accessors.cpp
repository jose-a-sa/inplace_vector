#include <gmock/gmock.h>

#include <qx/inplace_vector.h>

#include "inplace_vector_test_utils.h"

using ::testing::ElementsAre;

// Element Access & Iterators

TEST(InplaceVectorAccess, ElementAccess)
{
    qx::inplace_vector<5, int> v = {1, 2, 3};
    EXPECT_EQ(v[0], 1);
    EXPECT_EQ(v.at(1), 2);
    EXPECT_EQ(v.front(), 1);
    EXPECT_EQ(v.back(), 3);
    EXPECT_EQ(v.data(), &v[0]);

    qx::inplace_vector<5, Tracked> t = {1, 2, 3};
    EXPECT_EQ(t[2], 3);
    EXPECT_EQ(t.at(0), 1);
    EXPECT_EQ(t.front(), 1);
    EXPECT_EQ(t.back(), 3);
}

TEST(InplaceVectorAccess, ConstElementAccess)
{
    qx::inplace_vector<5, int> const v = {1, 2, 3};
    static_assert(std::is_same_v<decltype(v[0]), int const&>);
    static_assert(std::is_same_v<decltype(v.at(0)), int const&>);
    static_assert(std::is_same_v<decltype(v.front()), int const&>);
    static_assert(std::is_same_v<decltype(v.back()), int const&>);
    static_assert(std::is_same_v<decltype(v.data()), int const*>);
    EXPECT_EQ(v[2], 3);
    EXPECT_EQ(v.at(1), 2);
    EXPECT_EQ(v.front(), 1);
    EXPECT_EQ(v.back(), 3);
}

TEST(InplaceVectorAccess, FrontBackAtMutable)
{
    qx::inplace_vector<5, int> v = {1, 2, 3};
    v.front() = 10;
    v.back() = 30;
    v.at(1) = 20;
    EXPECT_THAT(v, ElementsAre(10, 20, 30));

    v.data()[0] = 11;
    EXPECT_THAT(v, ElementsAre(11, 20, 30));
}

TEST(InplaceVectorAccess, AtOutOfRangeThrows)
{
    qx::inplace_vector<5, int> v = {1, 2};
    qx::inplace_vector<5, int> const& cv = v;

    EXPECT_THROW(v.at(2), std::out_of_range);
    EXPECT_THROW(cv.at(2), std::out_of_range);
    EXPECT_THROW(v.at(5), std::out_of_range);
    EXPECT_THROW(v.at(static_cast<std::size_t>(-1)), std::out_of_range);
    EXPECT_EQ(v.at(1), 2); // one below the boundary: succeeds

    qx::inplace_vector<5, int> empty;
    EXPECT_THROW(empty.at(0), std::out_of_range);
}

TEST(InplaceVectorAccess, AtDoesNotReachIntoUnusedCapacity)
{
    qx::inplace_vector<5, int> v = {1, 2, 3};
    v.pop_back();
    EXPECT_THROW(v.at(2), std::out_of_range);
}

TEST(InplaceVectorAccess, Iterators)
{
    qx::inplace_vector<5, int> v = {1, 2, 3};
    static_assert(std::is_same_v<qx::inplace_vector<5, int>::iterator, int*>);
    static_assert(std::is_same_v<qx::inplace_vector<5, int>::const_iterator, int const*>);

    EXPECT_EQ(v.end() - v.begin(), 3);
    EXPECT_EQ(v.begin(), v.data());
    EXPECT_EQ(*v.cbegin(), 1);
    EXPECT_EQ(*v.crbegin(), 3);

    *v.begin() = 10;
    *v.rbegin() = 30;
    EXPECT_THAT(v, ElementsAre(10, 2, 30));
    EXPECT_THAT(std::vector<int>(v.rbegin(), v.rend()), ElementsAre(30, 2, 10));
}

TEST(InplaceVectorAccess, ConstIterators)
{
    qx::inplace_vector<5, int> const v = {1, 2, 3};
    static_assert(std::is_same_v<decltype(v.begin()), int const*>);
    static_assert(std::is_same_v<decltype(v.end()), int const*>);

    EXPECT_EQ(v.end() - v.begin(), 3);
    EXPECT_EQ(v.cbegin(), v.begin());
    EXPECT_EQ(v.cend(), v.end());
    EXPECT_EQ(*v.rbegin(), 3);
    EXPECT_EQ(v.crend() - v.crbegin(), 3);
}

TEST(InplaceVectorAccess, EmptyIterators)
{
    qx::inplace_vector<5, int> v;
    EXPECT_EQ(v.begin(), v.end());
    EXPECT_EQ(v.cbegin(), v.cend());
    EXPECT_EQ(v.rbegin(), v.rend());
    EXPECT_EQ(v.crbegin(), v.crend());
}

TEST(InplaceVectorAccess, ExceptionsAndContracts)
{
    using V = qx::inplace_vector<10, int>;

    EXPECT_DEATH(
        {
            V const v(3, 1);
            int const volatile x = v[3];
            (void)x;
        },
        "contract violation");
    EXPECT_DEATH(
        {
            V v(3, 1);
            v[10] = 1;
        },
        "contract violation");
    EXPECT_DEATH(
        {
            V const v;
            int const volatile x = v.front();
            (void)x;
        },
        "contract violation");
    EXPECT_DEATH(
        {
            V v;
            v.front() = 1;
        },
        "contract violation");
    EXPECT_DEATH(
        {
            V const v;
            int const volatile x = v.back();
            (void)x;
        },
        "contract violation");
    EXPECT_DEATH(
        {
            V v;
            v.back() = 1;
        },
        "contract violation");
}
