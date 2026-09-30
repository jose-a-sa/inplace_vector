#include <gmock/gmock.h>

#include <qx/inplace_vector.h>

#include "inplace_vector_test_utils.h"

#include <algorithm>
#include <numeric>

// Utilities, Capacity & Swap

TEST(InplaceVectorUtils, Resize)
{
    qx::inplace_vector<10, int> v = {1, 2, 3};
    v.resize(5, 9);
    EXPECT_THAT(v, ::testing::ElementsAre(1, 2, 3, 9, 9));

    v.resize(2);
    EXPECT_THAT(v, ::testing::ElementsAre(1, 2));

    v.resize(2, 7); // same size: no-op
    EXPECT_THAT(v, ::testing::ElementsAre(1, 2));

    v.resize(0);
    EXPECT_THAT(v, ::testing::IsEmpty());

    v.resize(10);
    EXPECT_EQ(v.size(), 10u);
}

TEST(InplaceVectorUtils, ResizeValueInitializes)
{
    qx::inplace_vector<10, int> v = {1, 2, 3};
    v.resize(5);
    EXPECT_THAT(v, ::testing::ElementsAre(1, 2, 3, 0, 0));

    qx::inplace_vector<10, Tracked> t = {1, 2};
    t.resize(4);
    EXPECT_THAT(t, ::testing::ElementsAre(1, 2, 0, 0));
}

TEST(InplaceVectorUtils, ResizeValueInitializesOverStaleElements)
{
    qx::inplace_vector<8, int> v = {1, 2, 3, 4};
    v.resize(0);
    v.resize(4);
    EXPECT_THAT(v, ::testing::ElementsAre(0, 0, 0, 0));
}

TEST(InplaceVectorUtils, ResizeNonTrivial)
{
    qx::inplace_vector<10, Tracked> v = {1, 2, 3};
    v.resize(5, Tracked(9));
    EXPECT_THAT(v, ::testing::ElementsAre(1, 2, 3, 9, 9));
    v.resize(1);
    EXPECT_THAT(v, ::testing::ElementsAre(1));
}

TEST(InplaceVectorUtils, ResizeValueAliasesElement)
{
    qx::inplace_vector<10, Tracked> v = {1, 2, 3};
    v.resize(6, v[1]);
    EXPECT_THAT(v, ::testing::ElementsAre(1, 2, 3, 2, 2, 2));
}

TEST(InplaceVectorUtils, ResizeOverflowThrows)
{
    qx::inplace_vector<5, Tracked> v = {1, 2, 3};
    EXPECT_THROW(v.resize(6), std::length_error);
    EXPECT_THROW(v.resize(6, Tracked(9)), std::length_error);
    EXPECT_THAT(v, ::testing::ElementsAre(1, 2, 3));
}

TEST(InplaceVectorUtils, ReserveAndCapacity)
{
    qx::inplace_vector<10, int> v = {1, 2, 3};
    static_assert(qx::inplace_vector<10, int>::capacity() == 10);
    static_assert(qx::inplace_vector<10, int>::max_size() == 10);

    v.reserve(0);
    v.reserve(10);
    EXPECT_THROW(v.reserve(11), std::length_error);

    v.shrink_to_fit();
    EXPECT_EQ(v.capacity(), 10u);
    EXPECT_EQ(v.size(), 3u);
    EXPECT_THAT(v, ::testing::ElementsAre(1, 2, 3));
}

TEST(InplaceVectorUtils, Swap)
{
    qx::inplace_vector<10, int> a = {1, 2, 3};
    qx::inplace_vector<10, int> b = {4, 5, 6, 7, 8};

    a.swap(b); // shorter.swap(longer)
    EXPECT_THAT(a, ::testing::ElementsAre(4, 5, 6, 7, 8));
    EXPECT_THAT(b, ::testing::ElementsAre(1, 2, 3));

    a.swap(b); // longer.swap(shorter)
    EXPECT_THAT(a, ::testing::ElementsAre(1, 2, 3));
    EXPECT_THAT(b, ::testing::ElementsAre(4, 5, 6, 7, 8));

    qx::inplace_vector<10, int> empty;
    a.swap(empty);
    EXPECT_THAT(a, ::testing::IsEmpty());
    EXPECT_THAT(empty, ::testing::ElementsAre(1, 2, 3));

    using std::swap;
    swap(a, empty); // ADL swap
    EXPECT_THAT(a, ::testing::ElementsAre(1, 2, 3));
    EXPECT_THAT(empty, ::testing::IsEmpty());
}

TEST(InplaceVectorUtils, SwapNonTrivial)
{
    qx::inplace_vector<10, Tracked> a = {1, 2, 3};
    qx::inplace_vector<10, Tracked> b = {4, 5, 6, 7, 8};

    a.swap(b);
    EXPECT_THAT(a, ::testing::ElementsAre(4, 5, 6, 7, 8));
    EXPECT_THAT(b, ::testing::ElementsAre(1, 2, 3));

    b.swap(a);
    EXPECT_THAT(a, ::testing::ElementsAre(1, 2, 3));
    EXPECT_THAT(b, ::testing::ElementsAre(4, 5, 6, 7, 8));

    qx::inplace_vector<10, Tracked> equal_size = {7, 8, 9};
    a.swap(equal_size);
    EXPECT_THAT(a, ::testing::ElementsAre(7, 8, 9));
    EXPECT_THAT(equal_size, ::testing::ElementsAre(1, 2, 3));
}

TEST(InplaceVectorUtils, SwapWithStdSwapAndSelf)
{
    qx::inplace_vector<10, Tracked> a = {1, 2};
    qx::inplace_vector<10, Tracked> b = {3, 4, 5};

    std::swap(a, b);
    EXPECT_THAT(a, ::testing::ElementsAre(3, 4, 5));
    EXPECT_THAT(b, ::testing::ElementsAre(1, 2));

    a.swap(a);
    EXPECT_THAT(a, ::testing::ElementsAre(3, 4, 5));
}

TEST(InplaceVectorUtils, SwapNoexceptFollowsElementType)
{
    struct ThrowingSwap
    {
        ThrowingSwap() = default;
        ThrowingSwap(ThrowingSwap&&) noexcept {}
        ThrowingSwap& operator=(ThrowingSwap&&) noexcept(false) { return *this; }
    };

    using I = qx::inplace_vector<4, int>;
    using T = qx::inplace_vector<4, Tracked>;
    using S = qx::inplace_vector<4, ThrowingSwap>;
    static_assert(noexcept(std::declval<I&>().swap(std::declval<I&>())));
    static_assert(noexcept(std::declval<T&>().swap(std::declval<T&>())));
    static_assert(!noexcept(std::declval<S&>().swap(std::declval<S&>())));
}

TEST(InplaceVectorUtils, WorksWithStdAlgorithms)
{
    qx::inplace_vector<10, int> v = {5, 3, 4, 1, 2};
    std::sort(v.begin(), v.end());
    EXPECT_THAT(v, ::testing::ElementsAre(1, 2, 3, 4, 5));
    EXPECT_EQ(std::accumulate(v.begin(), v.end(), 0), 15);
    std::reverse(v.begin(), v.end());
    EXPECT_THAT(v, ::testing::ElementsAre(5, 4, 3, 2, 1));
}
