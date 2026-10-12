#include <gmock/gmock.h>

#include <qx/inplace_vector.h>

#include "inplace_vector_test_utils.h"

#include <memory>
#include <string>
#include <type_traits>
#include <utility>

// [inplace.vector.overview]: each special member of inplace_vector<T, N> is trivial when the corresponding member of T is:
//  - copy/move constructor: if T is trivially copy/move constructible
//  - destructor: if T is trivially destructible
//  - copy/move assignment: if T is trivially destructible, copy/move constructible and copy/move assignable
// and inplace_vector<T, 0> is trivially copyable, trivially default constructible and empty. The rules are checked in both
// directions: a non-trivial member must not be trivial, since a bytewise copy would be wrong.

namespace
{

struct NoAssign
{
    int const x = 0;
};

template <class T, std::size_t N>
constexpr bool follows_triviality_rules()
{
    using IV = qx::inplace_vector<N, T>;
    constexpr bool dtor = std::is_trivially_destructible_v<T>;
    constexpr bool copy_assign = dtor && std::is_trivially_copy_constructible_v<T> && std::is_trivially_copy_assignable_v<T>;
    constexpr bool move_assign = dtor && std::is_trivially_move_constructible_v<T> && std::is_trivially_move_assignable_v<T>;
    return std::is_trivially_destructible_v<IV> == dtor &&
           std::is_trivially_copy_constructible_v<IV> == std::is_trivially_copy_constructible_v<T> &&
           std::is_trivially_move_constructible_v<IV> == std::is_trivially_move_constructible_v<T> &&
           std::is_trivially_copy_assignable_v<IV> == copy_assign && std::is_trivially_move_assignable_v<IV> == move_assign &&
           (!std::is_trivially_copyable_v<T> || std::is_trivially_copyable_v<IV>);
}

template <class T>
constexpr bool follows_triviality_rules_for_all_capacities()
{
    return follows_triviality_rules<T, 1>() && follows_triviality_rules<T, 2>() && follows_triviality_rules<T, 17>();
}

static_assert(follows_triviality_rules_for_all_capacities<int>());
static_assert(follows_triviality_rules_for_all_capacities<double>());
static_assert(follows_triviality_rules_for_all_capacities<int*>());
static_assert(follows_triviality_rules_for_all_capacities<NoDefault>());
static_assert(follows_triviality_rules_for_all_capacities<Noisy>());
static_assert(follows_triviality_rules_for_all_capacities<NoAssign>());
static_assert(follows_triviality_rules_for_all_capacities<std::pair<int, int>>());
static_assert(follows_triviality_rules_for_all_capacities<Tracked>());
static_assert(follows_triviality_rules_for_all_capacities<CopyCounted>());
static_assert(follows_triviality_rules_for_all_capacities<MoveCounted>());
static_assert(follows_triviality_rules_for_all_capacities<std::string>());
static_assert(follows_triviality_rules_for_all_capacities<std::unique_ptr<int>>());
static_assert(follows_triviality_rules_for_all_capacities<std::pair<int, std::string>>());

// the cases that the rules exist for
static_assert(std::is_trivially_destructible_v<qx::inplace_vector<4, CopyCounted>>);
static_assert(!std::is_trivially_copy_constructible_v<qx::inplace_vector<4, CopyCounted>>);
static_assert(std::is_trivially_copy_constructible_v<qx::inplace_vector<4, MoveCounted>>);
static_assert(!std::is_trivially_move_constructible_v<qx::inplace_vector<4, MoveCounted>>);
static_assert(!std::is_trivially_copyable_v<qx::inplace_vector<4, CopyCounted>>);
static_assert(!std::is_trivially_copyable_v<qx::inplace_vector<4, MoveCounted>>);

// operations that T does not have are not claimed by the container
static_assert(!std::is_copy_constructible_v<qx::inplace_vector<4, std::unique_ptr<int>>>);
static_assert(!std::is_copy_assignable_v<qx::inplace_vector<4, std::unique_ptr<int>>>);
static_assert(std::is_move_constructible_v<qx::inplace_vector<4, std::unique_ptr<int>>>);
static_assert(std::is_move_assignable_v<qx::inplace_vector<4, std::unique_ptr<int>>>);
static_assert(std::is_copy_constructible_v<qx::inplace_vector<4, NoAssign>>);
static_assert(!std::is_copy_assignable_v<qx::inplace_vector<4, NoAssign>>);
static_assert(std::is_trivially_copyable_v<qx::inplace_vector<4, NoAssign>>); // as T is

// N == 0
template <class T>
constexpr bool empty_specialization_contract()
{
    using IV = qx::inplace_vector<0, T>;
    return std::is_empty_v<IV> && std::is_trivially_copyable_v<IV> && std::is_trivially_default_constructible_v<IV> &&
           std::is_trivially_destructible_v<IV> && std::is_nothrow_default_constructible_v<IV>;
}

static_assert(empty_specialization_contract<int>());
static_assert(empty_specialization_contract<Tracked>());
static_assert(empty_specialization_contract<std::string>());
static_assert(empty_specialization_contract<std::unique_ptr<int>>());
static_assert(empty_specialization_contract<NoDefault>());

// noexcept
struct ThrowingSwap
{
    ThrowingSwap() = default;
    ThrowingSwap(ThrowingSwap&&) noexcept {}
    ThrowingSwap& operator=(ThrowingSwap&&) noexcept(false) { return *this; }
};

struct ThrowingMove
{
    ThrowingMove() = default;
    ThrowingMove(ThrowingMove&&) noexcept(false) {}
    ThrowingMove& operator=(ThrowingMove&&) noexcept { return *this; }
};

template <class IV>
constexpr bool swaps_noexcept = noexcept(std::declval<IV&>().swap(std::declval<IV&>())) &&
                                noexcept(swap(std::declval<IV&>(), std::declval<IV&>()));

static_assert(swaps_noexcept<qx::inplace_vector<0, ThrowingSwap>>); // N == 0 never throws
static_assert(swaps_noexcept<qx::inplace_vector<0, ThrowingMove>>);
static_assert(!swaps_noexcept<qx::inplace_vector<1, ThrowingSwap>>);
static_assert(!swaps_noexcept<qx::inplace_vector<1, ThrowingMove>>);
static_assert(swaps_noexcept<qx::inplace_vector<1, int>>);

static_assert(std::is_nothrow_move_constructible_v<qx::inplace_vector<4, MoveCounted>>);
static_assert(std::is_nothrow_move_constructible_v<qx::inplace_vector<4, std::string>>);
static_assert(!std::is_nothrow_move_constructible_v<qx::inplace_vector<4, ThrowingMove>>);
static_assert(!std::is_nothrow_move_assignable_v<qx::inplace_vector<4, ThrowingMove>>);
static_assert(!std::is_nothrow_move_assignable_v<qx::inplace_vector<4, ThrowingSwap>>);
static_assert(std::is_nothrow_move_assignable_v<qx::inplace_vector<4, std::string>>);

// member types
static_assert(std::is_same_v<qx::inplace_vector<4, int>::size_type, std::size_t>);
static_assert(std::is_same_v<qx::inplace_vector<4, int>::difference_type, std::ptrdiff_t>);
static_assert(std::is_same_v<qx::inplace_vector<4, int>::value_type, int>);
static_assert(std::is_same_v<qx::inplace_vector<4, int>::reference, int&>);
static_assert(std::is_same_v<qx::inplace_vector<4, int>::const_reference, int const&>);
static_assert(std::is_same_v<qx::inplace_vector<4, int>::pointer, int*>);
static_assert(std::is_same_v<qx::inplace_vector<4, int>::const_pointer, int const*>);
static_assert(std::is_same_v<qx::inplace_vector<4, int>::reverse_iterator, std::reverse_iterator<int*>>);
static_assert(std::is_same_v<qx::inplace_vector<4, int>::const_reverse_iterator, std::reverse_iterator<int const*>>);

// the empty vector is usable in constant expressions
constexpr qx::inplace_vector<4, int> constexpr_empty;
static_assert(constexpr_empty.empty());
static_assert(constexpr_empty.size() == 0);
static_assert(constexpr_empty.capacity() == 4 && constexpr_empty.max_size() == 4);
static_assert(constexpr_empty.begin() == constexpr_empty.end());
static_assert(constexpr_empty.cbegin() == constexpr_empty.cend());
static_assert(constexpr_empty.rbegin() == constexpr_empty.rend());
static_assert(qx::inplace_vector<0, int>{}.empty());
static_assert(qx::inplace_vector<0, Tracked>{}.capacity() == 0);

} // namespace

// Behaviour of the layers: where a special member is not trivial, it is the one that runs.

TEST(InplaceVectorTraits, CopyCountedIsCopiedElementByElement)
{
    qx::inplace_vector<8, CopyCounted> const a = {1, 2, 3};
    CopyCounted::copies = 0;

    qx::inplace_vector<8, CopyCounted> b(a);
    EXPECT_EQ(CopyCounted::copies, 3); // only the live elements, not the capacity
    EXPECT_THAT(b, ::testing::ElementsAre(1, 2, 3));

    CopyCounted::copies = 0;
    qx::inplace_vector<8, CopyCounted> c = {9, 9, 9, 9, 9};
    CopyCounted::copies = 0;
    c = a; // assigns the common prefix, destroys the rest: no copy construction
    EXPECT_EQ(CopyCounted::copies, 0);
    EXPECT_THAT(c, ::testing::ElementsAre(1, 2, 3));

    qx::inplace_vector<8, CopyCounted> d = {7};
    CopyCounted::copies = 0;
    d = a; // one assigned, two constructed
    EXPECT_EQ(CopyCounted::copies, 2);
    EXPECT_THAT(d, ::testing::ElementsAre(1, 2, 3));
}

TEST(InplaceVectorTraits, CopyCountedMovesAsCopies)
{
    qx::inplace_vector<8, CopyCounted> a = {1, 2, 3};
    CopyCounted::copies = 0;
    qx::inplace_vector<8, CopyCounted> b(std::move(a)); // no move constructor: the copy constructor is the fallback
    EXPECT_EQ(CopyCounted::copies, 3);
    EXPECT_THAT(b, ::testing::ElementsAre(1, 2, 3));
}

TEST(InplaceVectorTraits, MoveCountedIsMovedElementByElement)
{
    qx::inplace_vector<8, MoveCounted> a = {1, 2, 3};
    MoveCounted::moves = 0;

    qx::inplace_vector<8, MoveCounted> const copy(a); // trivially copy constructible: no element move
    EXPECT_EQ(MoveCounted::moves, 0);
    EXPECT_THAT(copy, ::testing::ElementsAre(1, 2, 3));

    MoveCounted::moves = 0; // the matchers above move-construct MoveCounted values themselves
    qx::inplace_vector<8, MoveCounted> b(std::move(a));
    EXPECT_EQ(MoveCounted::moves, 3);
    EXPECT_THAT(b, ::testing::ElementsAre(1, 2, 3));

    qx::inplace_vector<8, MoveCounted> c = {9};
    MoveCounted::moves = 0;
    c = std::move(b); // one move assigned (trivial), two move constructed
    EXPECT_EQ(MoveCounted::moves, 2);
    EXPECT_THAT(c, ::testing::ElementsAre(1, 2, 3));
}

TEST(InplaceVectorTraits, TriviallyDestructibleElementsNeedNoDestruction)
{
    // the destructor is trivial, so the vector can be abandoned (e.g. placement-new'd storage) without leaking anything
    using V = qx::inplace_vector<4, CopyCounted>;
    alignas(V) unsigned char buffer[sizeof(V)];
    V* v = ::new (static_cast<void*>(buffer)) V{1, 2};
    EXPECT_THAT(*v, ::testing::ElementsAre(1, 2));
}

TEST(InplaceVectorTraits, EmptySpecializationHasNoState)
{
    qx::inplace_vector<0, Tracked> a;
    qx::inplace_vector<0, Tracked> b = a;
    b = a;
    b = std::move(a);
    EXPECT_EQ(b.data(), nullptr);
    EXPECT_EQ(sizeof(b), 1u);
}
