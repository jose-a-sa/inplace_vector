#include <gmock/gmock.h>

#include <qx/inplace_vector.h>

#include "inplace_vector_test_utils.h"

#include <memory>
#include <string>
#include <utility>

using ::testing::ElementsAre;
using ::testing::IsEmpty;

namespace
{

struct Boom
{};

// Every copy/move construction throws; the int constructor does not (used to build fixtures).
struct ThrowOnCopy
{
    ThrowOnCopy() = default;
    explicit ThrowOnCopy(int) {}
    ThrowOnCopy(ThrowOnCopy const&) { throw Boom{}; }
    ThrowOnCopy(ThrowOnCopy&&) noexcept(false) { throw Boom{}; }
};

} // namespace

// Append (push_back, try_push_back, unchecked_push_back, emplace_back, try_emplace_back, unchecked_emplace_back, pop_back)

TEST(InplaceVectorAppend, PushBack)
{
    qx::inplace_vector<5, int> v;
    int const lvalue = 1;
    int& first = v.push_back(lvalue);
    int& second = v.push_back(2);
    EXPECT_EQ(&first, &v[0]);
    EXPECT_EQ(&second, &v[1]);
    EXPECT_THAT(v, ElementsAre(1, 2));

    qx::inplace_vector<5, Tracked> t;
    Tracked const tracked(1);
    t.push_back(tracked);
    t.push_back(Tracked(2));
    EXPECT_THAT(t, ElementsAre(1, 2));
    EXPECT_EQ(tracked, 1); // lvalue not consumed
}

TEST(InplaceVectorAppend, PushBackSelfElement)
{
    qx::inplace_vector<8, Tracked> v = {1, 2, 3};
    v.push_back(v[0]);
    v.push_back(v.back());
    EXPECT_THAT(v, ElementsAre(1, 2, 3, 1, 1));
}

TEST(InplaceVectorAppend, EmplaceBack)
{
    qx::inplace_vector<3, std::pair<int, std::string>> v;
    auto& r = v.emplace_back(1, "one");
    EXPECT_EQ(&r, &v.back());
    v.emplace_back(2, "two");
    ASSERT_EQ(v.size(), 2u);
    EXPECT_EQ(v[0].second, "one");
    EXPECT_EQ(v[1].first, 2);
}

TEST(InplaceVectorAppend, MoveOnlyElement)
{
    qx::inplace_vector<3, std::unique_ptr<int>> v;
    v.push_back(std::make_unique<int>(1));
    v.emplace_back(new int(2));
    ASSERT_EQ(v.size(), 2u);
    EXPECT_EQ(*v[0], 1);
    EXPECT_EQ(*v[1], 2);
}

TEST(InplaceVectorAppend, PopBack)
{
    qx::inplace_vector<5, Tracked> v = {1, 2, 3};
    v.pop_back();
    EXPECT_THAT(v, ElementsAre(1, 2));
    v.pop_back();
    v.pop_back();
    EXPECT_THAT(v, IsEmpty());

    v.push_back(9); // storage is reusable once emptied
    EXPECT_THAT(v, ElementsAre(9));
}

TEST(InplaceVectorAppend, FillToCapacity)
{
    qx::inplace_vector<3, Tracked> v;
    for (int i = 1; i <= 3; ++i)
        v.push_back(i);
    EXPECT_THAT(v, ElementsAre(1, 2, 3));
    EXPECT_EQ(v.size(), v.capacity());
}

TEST(InplaceVectorAppend, TryPushBack)
{
    qx::inplace_vector<3, int> v;
    int const lvalue = 1;
    int* first = v.try_push_back(lvalue);
    int* second = v.try_push_back(2);
    ASSERT_NE(first, nullptr);
    ASSERT_NE(second, nullptr);
    EXPECT_EQ(first, &v[0]);
    EXPECT_EQ(second, &v[1]);
    EXPECT_THAT(v, ElementsAre(1, 2));

    ASSERT_NE(v.try_push_back(3), nullptr); // exactly fills the capacity
    EXPECT_EQ(v.try_push_back(4), nullptr);
    EXPECT_EQ(v.try_push_back(lvalue), nullptr);
    EXPECT_THAT(v, ElementsAre(1, 2, 3)); // unchanged
}

TEST(InplaceVectorAppend, TryPushBackNonTrivial)
{
    qx::inplace_vector<2, Tracked> v;
    Tracked const lvalue(1);
    EXPECT_NE(v.try_push_back(lvalue), nullptr);
    EXPECT_NE(v.try_push_back(Tracked(2)), nullptr);
    EXPECT_EQ(v.try_push_back(Tracked(3)), nullptr);
    EXPECT_EQ(v.try_push_back(lvalue), nullptr);
    EXPECT_THAT(v, ElementsAre(1, 2));
}

TEST(InplaceVectorAppend, TryPushBackFailureDoesNotConsumeArgument)
{
    qx::inplace_vector<1, Tracked> v = {1};
    Tracked x(9);
    EXPECT_EQ(v.try_push_back(std::move(x)), nullptr);
    EXPECT_EQ(x, 9); // not moved-from
}

TEST(InplaceVectorAppend, TryEmplaceBack)
{
    qx::inplace_vector<2, std::pair<int, std::string>> v;
    auto* first = v.try_emplace_back(1, "one");
    ASSERT_NE(first, nullptr);
    EXPECT_EQ(first, &v.back());
    EXPECT_EQ(first->second, "one");

    EXPECT_NE(v.try_emplace_back(2, "two"), nullptr);
    EXPECT_EQ(v.try_emplace_back(3, "three"), nullptr);
    EXPECT_EQ(v.size(), 2u);
    EXPECT_EQ(v[1].first, 2);
}

TEST(InplaceVectorAppend, TryEmplaceBackFailureDoesNotConsumeArgument)
{
    qx::inplace_vector<1, std::unique_ptr<int>> v;
    auto a = std::make_unique<int>(1);
    auto b = std::make_unique<int>(2);

    ASSERT_NE(v.try_emplace_back(std::move(a)), nullptr);
    EXPECT_EQ(a, nullptr); // consumed on success
    EXPECT_EQ(*v[0], 1);

    EXPECT_EQ(v.try_emplace_back(std::move(b)), nullptr);
    ASSERT_NE(b, nullptr); // untouched on failure
    EXPECT_EQ(*b, 2);
}

TEST(InplaceVectorAppend, UncheckedPushBack)
{
    qx::inplace_vector<3, int> v;
    int const lvalue = 1;
    int& first = v.unchecked_push_back(lvalue);
    int& second = v.unchecked_push_back(2);
    EXPECT_EQ(&first, &v[0]);
    EXPECT_EQ(&second, &v[1]);

    v.unchecked_push_back(3); // exactly fills the capacity
    EXPECT_THAT(v, ElementsAre(1, 2, 3));
}

TEST(InplaceVectorAppend, UncheckedPushBackNonTrivial)
{
    qx::inplace_vector<3, Tracked> v;
    Tracked const lvalue(1);
    v.unchecked_push_back(lvalue);
    v.unchecked_push_back(Tracked(2));
    EXPECT_THAT(v, ElementsAre(1, 2));
}

TEST(InplaceVectorAppend, UncheckedEmplaceBack)
{
    qx::inplace_vector<2, std::pair<int, std::string>> v;
    auto& first = v.unchecked_emplace_back(1, "one");
    EXPECT_EQ(&first, &v.back());
    v.unchecked_emplace_back(2, "two");
    ASSERT_EQ(v.size(), 2u);
    EXPECT_EQ(v[0].second, "one");
    EXPECT_EQ(v[1].first, 2);

    qx::inplace_vector<2, std::unique_ptr<int>> m;
    m.unchecked_emplace_back(new int(7));
    m.unchecked_push_back(std::make_unique<int>(8));
    ASSERT_EQ(m.size(), 2u);
    EXPECT_EQ(*m[0], 7);
    EXPECT_EQ(*m[1], 8);
}

TEST(InplaceVectorAppend, PushBackVariantsCopyLvaluesAndMoveRvalues)
{
    qx::inplace_vector<6, Tracked> v;
    Tracked lvalue(1);

    v.push_back(lvalue);
    v.try_push_back(lvalue);
    v.unchecked_push_back(lvalue);
    EXPECT_EQ(lvalue, 1); // copied from, not moved from
    EXPECT_THAT(v, ElementsAre(1, 1, 1));

    Tracked a(2), b(3), c(4);
    v.push_back(std::move(a));
    v.try_push_back(std::move(b));
    v.unchecked_push_back(std::move(c));
    EXPECT_THAT(v, ElementsAre(1, 1, 1, 2, 3, 4));
    EXPECT_EQ(a.id, Tracked::moved_from);
    EXPECT_EQ(b.id, Tracked::moved_from);
    EXPECT_EQ(c.id, Tracked::moved_from);
}

TEST(InplaceVectorAppend, ThrowingConstructionPropagatesAndLeavesVectorUnchanged)
{
    qx::inplace_vector<4, ThrowOnCopy> v;
    v.emplace_back(0);
    ThrowOnCopy x;

    EXPECT_THROW(v.push_back(x), Boom);
    EXPECT_THROW(v.try_push_back(x), Boom);
    EXPECT_THROW(v.unchecked_push_back(x), Boom);
    EXPECT_THROW(v.push_back(std::move(x)), Boom);
    EXPECT_THROW(v.try_push_back(std::move(x)), Boom);
    EXPECT_THROW(v.unchecked_push_back(std::move(x)), Boom);
    EXPECT_THROW(v.emplace_back(x), Boom);
    EXPECT_THROW(v.try_emplace_back(x), Boom);
    EXPECT_THROW(v.unchecked_emplace_back(x), Boom);
    EXPECT_EQ(v.size(), 1u);
}

TEST(InplaceVectorAppend, TryAndUncheckedNoexceptFollowsElementType)
{
    using I = qx::inplace_vector<4, int>;
    using P = qx::inplace_vector<4, std::unique_ptr<int>>;
    using B = qx::inplace_vector<4, ThrowOnCopy>;

    static_assert(noexcept(std::declval<I&>().try_push_back(std::declval<int const&>())));
    static_assert(noexcept(std::declval<I&>().try_push_back(1)));
    static_assert(noexcept(std::declval<I&>().unchecked_push_back(std::declval<int const&>())));
    static_assert(noexcept(std::declval<I&>().unchecked_push_back(1)));
    static_assert(noexcept(std::declval<I&>().try_emplace_back(1)));
    static_assert(noexcept(std::declval<I&>().unchecked_emplace_back(1)));
    static_assert(noexcept(std::declval<P&>().try_push_back(std::declval<std::unique_ptr<int>>())));

    static_assert(!noexcept(std::declval<B&>().try_push_back(std::declval<ThrowOnCopy const&>())));
    static_assert(!noexcept(std::declval<B&>().try_push_back(std::declval<ThrowOnCopy>())));
    static_assert(!noexcept(std::declval<B&>().unchecked_push_back(std::declval<ThrowOnCopy const&>())));
    static_assert(!noexcept(std::declval<B&>().unchecked_push_back(std::declval<ThrowOnCopy>())));
    static_assert(!noexcept(std::declval<B&>().try_emplace_back(std::declval<ThrowOnCopy const&>())));
    static_assert(!noexcept(std::declval<B&>().unchecked_emplace_back(std::declval<ThrowOnCopy const&>())));

    static_assert(!noexcept(std::declval<I&>().push_back(1))); // throws on overflow
    static_assert(!noexcept(std::declval<I&>().emplace_back(1)));
}

TEST(InplaceVectorAppend, ExceptionsAndContracts)
{
    qx::inplace_vector<2, Tracked> full = {1, 2};
    Tracked const lvalue(9);
    EXPECT_THROW(full.push_back(lvalue), std::length_error);
    EXPECT_THROW(full.push_back(Tracked(9)), std::length_error);
    EXPECT_THROW(full.emplace_back(9), std::length_error);
    EXPECT_THAT(full, ElementsAre(1, 2)); // unchanged

    using V = qx::inplace_vector<2, int>;
    EXPECT_DEATH(
        {
            V empty;
            empty.pop_back();
        },
        "contract violation");
}

TEST(InplaceVectorAppend, TryAppendOnFullReturnsNullptrInsteadOfThrowing)
{
    qx::inplace_vector<2, Tracked> full = {1, 2};
    Tracked const lvalue(9);
    EXPECT_NO_THROW(EXPECT_EQ(full.try_push_back(lvalue), nullptr));
    EXPECT_NO_THROW(EXPECT_EQ(full.try_push_back(Tracked(9)), nullptr));
    EXPECT_NO_THROW(EXPECT_EQ(full.try_emplace_back(9), nullptr));
    EXPECT_THAT(full, ElementsAre(1, 2));
}

TEST(InplaceVectorAppend, UncheckedAppendExceptionsAndContracts)
{
    using V = qx::inplace_vector<2, int>;

    EXPECT_DEATH(
        {
            V full(2, 1);
            full.unchecked_push_back(1);
        },
        "contract violation");
    EXPECT_DEATH(
        {
            V full(2, 1);
            int const lvalue = 1;
            full.unchecked_push_back(lvalue);
        },
        "contract violation");
    EXPECT_DEATH(
        {
            V full(2, 1);
            full.unchecked_emplace_back(1);
        },
        "contract violation");
}
