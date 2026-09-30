#include <gmock/gmock.h>

#include <qx/inplace_vector.h>

#include "inplace_vector_test_utils.h"

#include <cstring>
#include <forward_list>
#include <iterator>
#include <list>
#include <memory>
#include <sstream>
#include <vector>

// A trivially copyable T must give a trivially copyable inplace_vector (any capacity > 0); N == 0 always does.
template <class T, std::size_t... Ns>
constexpr bool trivially_copyable_contract_v =
    ((!std::is_trivially_copyable_v<T> || std::is_trivially_copyable_v<qx::inplace_vector<Ns, T>> || Ns == 0) && ...);

static_assert(trivially_copyable_contract_v<int, 0, 1, 2, 8, 64>);
static_assert(trivially_copyable_contract_v<double, 0, 1, 2, 8, 64>);
static_assert(trivially_copyable_contract_v<NoDefault, 0, 1, 2, 8, 64>);
static_assert(trivially_copyable_contract_v<Noisy, 0, 1, 2, 8, 64>);
static_assert(trivially_copyable_contract_v<Tracked, 0, 1, 2, 8, 64>);
static_assert(trivially_copyable_contract_v<std::string, 0, 1, 2, 8, 64>);
static_assert(trivially_copyable_contract_v<std::unique_ptr<int>, 0, 1, 2, 8, 64>);

static_assert(std::is_trivially_copyable_v<qx::inplace_vector<8, int>>);
static_assert(std::is_trivially_destructible_v<qx::inplace_vector<8, int>>);
static_assert(!std::is_trivially_copyable_v<qx::inplace_vector<8, Tracked>>);
static_assert(!std::is_trivially_destructible_v<qx::inplace_vector<8, Tracked>>);
static_assert(std::is_trivially_copyable_v<qx::inplace_vector<0, Tracked>>);
static_assert(std::is_nothrow_default_constructible_v<qx::inplace_vector<8, int>>);
static_assert(std::is_nothrow_default_constructible_v<qx::inplace_vector<8, Tracked>>);
static_assert(std::is_nothrow_move_constructible_v<qx::inplace_vector<8, Tracked>>);
static_assert(std::is_nothrow_move_assignable_v<qx::inplace_vector<8, Tracked>>);

// Constructors & Initialization

TEST(InplaceVectorInit, DefaultConstructor)
{
    qx::inplace_vector<10, int> const v;
    EXPECT_TRUE(v.empty());
    EXPECT_EQ(v.size(), 0u);
    EXPECT_EQ(v.capacity(), 10u);
    EXPECT_EQ(v.max_size(), 10u);
    EXPECT_EQ(v.begin(), v.end());
}

TEST(InplaceVectorInit, DefaultConstructorDoesNotConstructElements)
{
    Noisy::constructed = 0;
    qx::inplace_vector<100, Noisy> v;
    EXPECT_EQ(Noisy::constructed, 0);

    v.emplace_back(1);
    EXPECT_EQ(Noisy::constructed, 1);
}

TEST(InplaceVectorInit, NonDefaultConstructibleElement)
{
    qx::inplace_vector<4, NoDefault> v;
    v.emplace_back(3);

    qx::inplace_vector<4, NoDefault> const copy(v);
    ASSERT_EQ(copy.size(), 1u);
    EXPECT_EQ(copy[0].x, 3);
}

TEST(InplaceVectorInit, CountConstructorValueInitializes)
{
    qx::inplace_vector<8, int> const v(3);
    EXPECT_THAT(v, ::testing::ElementsAre(0, 0, 0));

    qx::inplace_vector<8, Tracked> const t(3);
    EXPECT_THAT(t, ::testing::ElementsAre(0, 0, 0));
}

TEST(InplaceVectorInit, CountConstructorValueInitializesOverStaleStorage)
{
    using V = qx::inplace_vector<8, int>;
    alignas(V) unsigned char buffer[sizeof(V)];
    std::memset(buffer, 0xAB, sizeof buffer);

    V* v = ::new (static_cast<void*>(buffer)) V(4);
    EXPECT_THAT(*v, ::testing::ElementsAre(0, 0, 0, 0));
    v->~V();
}

TEST(InplaceVectorInit, CountValueConstructor)
{
    qx::inplace_vector<8, int> const v(4, 7);
    EXPECT_THAT(v, ::testing::ElementsAre(7, 7, 7, 7));

    qx::inplace_vector<8, Tracked> const t(2, Tracked(5));
    EXPECT_THAT(t, ::testing::ElementsAre(5, 5));
}

TEST(InplaceVectorInit, CountValueConstructorWithTwoIntegersIsNotARange)
{
    qx::inplace_vector<8, int> const v(3, 4);
    EXPECT_THAT(v, ::testing::ElementsAre(4, 4, 4));
}

TEST(InplaceVectorInit, CountConstructorsCapacityBoundary)
{
    EXPECT_EQ((qx::inplace_vector<4, int>(4).size()), 4u);
    EXPECT_EQ((qx::inplace_vector<4, int>(4, 1).size()), 4u);
    EXPECT_THROW((qx::inplace_vector<4, int>(5)), std::length_error);
    EXPECT_THROW((qx::inplace_vector<4, int>(5, 1)), std::length_error);
    EXPECT_THROW((qx::inplace_vector<4, Tracked>(5)), std::length_error);
    EXPECT_THROW((qx::inplace_vector<4, Tracked>(5, Tracked(1))), std::length_error);
}

TEST(InplaceVectorInit, RangeConstructorContiguousIterator)
{
    std::vector<int> src = {1, 2, 3, 4};
    qx::inplace_vector<10, int> const v(src.begin(), src.end());
    EXPECT_THAT(v, ::testing::ElementsAre(1, 2, 3, 4));

    qx::inplace_vector<10, int> const from_pointers(src.data(), src.data() + 2);
    EXPECT_THAT(from_pointers, ::testing::ElementsAre(1, 2));

    qx::inplace_vector<10, int> const empty(src.begin(), src.begin());
    EXPECT_THAT(empty, ::testing::IsEmpty());
}

TEST(InplaceVectorInit, RangeConstructorNonContiguousForwardIterator)
{
    std::list<int> lst{1, 2, 3};
    qx::inplace_vector<10, int> const v(lst.begin(), lst.end());
    EXPECT_THAT(v, ::testing::ElementsAre(1, 2, 3));

    std::forward_list<int> flst{4, 5};
    qx::inplace_vector<10, Tracked> const t(flst.begin(), flst.end());
    EXPECT_THAT(t, ::testing::ElementsAre(4, 5));
}

TEST(InplaceVectorInit, RangeConstructorForwardOverflowThrows)
{
    std::list<int> lst{1, 2, 3, 4};
    EXPECT_THROW((qx::inplace_vector<3, int>(lst.begin(), lst.end())), std::length_error);
    EXPECT_THROW((qx::inplace_vector<3, Tracked>(lst.begin(), lst.end())), std::length_error);
}

TEST(InplaceVectorInit, PureInputIterator)
{
    std::istringstream stream("1 2 3");
    std::istream_iterator<int> start(stream);
    std::istream_iterator<int> end;

    qx::inplace_vector<10, int> const v(start, end);
    EXPECT_THAT(v, ::testing::ElementsAre(1, 2, 3));
}

TEST(InplaceVectorInit, PureInputIteratorOverflowThrows)
{
    std::istringstream stream("1 2 3 4 5 6");
    std::istream_iterator<int> start(stream);
    std::istream_iterator<int> end;

    EXPECT_THROW((qx::inplace_vector<4, Tracked>(start, end)), std::length_error);
}

TEST(InplaceVectorInit, InitializerListConstructor)
{
    qx::inplace_vector<10, int> const v = {1, 2, 3};
    EXPECT_THAT(v, ::testing::ElementsAre(1, 2, 3));

    qx::inplace_vector<3, Tracked> const full = {1, 2, 3};
    EXPECT_THAT(full, ::testing::ElementsAre(1, 2, 3));

    qx::inplace_vector<10, int> const empty{};
    EXPECT_THAT(empty, ::testing::IsEmpty());
}

TEST(InplaceVectorInit, InitializerListOverflowThrows)
{
    EXPECT_THROW((qx::inplace_vector<2, int>{1, 2, 3}), std::length_error);
    EXPECT_THROW((qx::inplace_vector<2, Tracked>{1, 2, 3}), std::length_error);
}

TEST(InplaceVectorInit, CopyConstructor)
{
    qx::inplace_vector<10, int> const orig = {1, 2, 3};
    qx::inplace_vector<10, int> copy(orig);
    EXPECT_THAT(copy, ::testing::ElementsAre(1, 2, 3));

    copy[0] = 9; // independent storage
    EXPECT_THAT(orig, ::testing::ElementsAre(1, 2, 3));

    qx::inplace_vector<10, Tracked> const t_orig = {1, 2, 3};
    qx::inplace_vector<10, Tracked> const t_copy(t_orig);
    EXPECT_THAT(t_copy, ::testing::ElementsAre(1, 2, 3));
    EXPECT_THAT(t_orig, ::testing::ElementsAre(1, 2, 3));

    qx::inplace_vector<10, Tracked> const empty;
    qx::inplace_vector<10, Tracked> const empty_copy(empty);
    EXPECT_THAT(empty_copy, ::testing::IsEmpty());
}

TEST(InplaceVectorInit, MoveConstructor)
{
    qx::inplace_vector<10, int> a = {1, 2, 3};
    qx::inplace_vector<10, int> const b(std::move(a));
    EXPECT_THAT(b, ::testing::ElementsAre(1, 2, 3));

    qx::inplace_vector<10, Tracked> t = {1, 2, 3};
    qx::inplace_vector<10, Tracked> const moved(std::move(t));
    EXPECT_THAT(moved, ::testing::ElementsAre(1, 2, 3));
    EXPECT_EQ(t.size(), 3u); // like std::inplace_vector: the source keeps its size, its elements are moved-from
    EXPECT_THAT(t, ::testing::Each(Tracked::moved_from));
}

TEST(InplaceVectorInit, MoveOnlyElement)
{
    qx::inplace_vector<4, std::unique_ptr<int>> a;
    a.push_back(std::make_unique<int>(1));
    a.push_back(std::make_unique<int>(2));

    qx::inplace_vector<4, std::unique_ptr<int>> b(std::move(a));
    ASSERT_EQ(b.size(), 2u);
    EXPECT_EQ(*b[0], 1);
    EXPECT_EQ(*b[1], 2);

    qx::inplace_vector<4, std::unique_ptr<int>> c;
    c = std::move(b);
    ASSERT_EQ(c.size(), 2u);
    EXPECT_EQ(*c[1], 2);

    qx::inplace_vector<4, std::unique_ptr<int>> const nulls(3);
    for (auto const& p : nulls)
        EXPECT_EQ(p, nullptr);
}

TEST(InplaceVectorInit, TriviallyCopyableStorageSurvivesMemcpy)
{
    qx::inplace_vector<8, int> src = {1, 2, 3};
    qx::inplace_vector<8, int> dst;
    std::memcpy(static_cast<void*>(&dst), static_cast<void const*>(&src), sizeof src);
    EXPECT_THAT(dst, ::testing::ElementsAre(1, 2, 3));
}
