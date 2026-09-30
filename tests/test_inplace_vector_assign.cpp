#include <gmock/gmock.h>

#include <qx/inplace_vector.h>

#include "inplace_vector_test_utils.h"

#include <iterator>
#include <list>
#include <sstream>
#include <vector>

// Assignment (operator=, assign)

TEST(InplaceVectorAssign, CopyAssignment)
{
    qx::inplace_vector<8, int> longer = {1, 2, 3, 4, 5};
    qx::inplace_vector<8, int> shorter = {6, 7};
    qx::inplace_vector<8, int> empty;

    qx::inplace_vector<8, int> v = {9, 9, 9};
    v = longer; // grows
    EXPECT_THAT(v, ::testing::ElementsAre(1, 2, 3, 4, 5));
    v = shorter; // shrinks
    EXPECT_THAT(v, ::testing::ElementsAre(6, 7));
    v = empty;
    EXPECT_THAT(v, ::testing::IsEmpty());
    v = longer;
    EXPECT_THAT(v, ::testing::ElementsAre(1, 2, 3, 4, 5));
    EXPECT_THAT(longer, ::testing::ElementsAre(1, 2, 3, 4, 5)); // source untouched
}

TEST(InplaceVectorAssign, CopyAssignmentNonTrivial)
{
    qx::inplace_vector<8, Tracked> const longer = {1, 2, 3, 4, 5};
    qx::inplace_vector<8, Tracked> const shorter = {6, 7};
    qx::inplace_vector<8, Tracked> const same_size = {8, 9};

    qx::inplace_vector<8, Tracked> v = {9, 9, 9};
    v = longer;
    EXPECT_THAT(v, ::testing::ElementsAre(1, 2, 3, 4, 5));
    v = shorter;
    EXPECT_THAT(v, ::testing::ElementsAre(6, 7));
    v = same_size;
    EXPECT_THAT(v, ::testing::ElementsAre(8, 9));
    v = qx::inplace_vector<8, Tracked>{};
    EXPECT_THAT(v, ::testing::IsEmpty());
}

TEST(InplaceVectorAssign, MoveAssignment)
{
    qx::inplace_vector<8, int> v = {9, 9};
    v = qx::inplace_vector<8, int>{1, 2, 3};
    EXPECT_THAT(v, ::testing::ElementsAre(1, 2, 3));

    qx::inplace_vector<8, Tracked> t = {9, 9, 9, 9};
    t = qx::inplace_vector<8, Tracked>{1, 2};
    EXPECT_THAT(t, ::testing::ElementsAre(1, 2));
    t = qx::inplace_vector<8, Tracked>{3, 4, 5, 6, 7};
    EXPECT_THAT(t, ::testing::ElementsAre(3, 4, 5, 6, 7));
}

TEST(InplaceVectorAssign, SelfAssignment)
{
    qx::inplace_vector<8, Tracked> v = {1, 2, 3};
    auto& alias = v;
    v = alias;
    EXPECT_THAT(v, ::testing::ElementsAre(1, 2, 3));
    v = std::move(alias);
    EXPECT_THAT(v, ::testing::ElementsAre(1, 2, 3));
}

TEST(InplaceVectorAssign, OperatorAssignInitializerList)
{
    qx::inplace_vector<8, int> v = {9, 9, 9, 9, 9};
    v = {1, 2};
    EXPECT_THAT(v, ::testing::ElementsAre(1, 2));
    v = {1, 2, 3, 4, 5, 6};
    EXPECT_THAT(v, ::testing::ElementsAre(1, 2, 3, 4, 5, 6));
    v = {};
    EXPECT_THAT(v, ::testing::IsEmpty());
}

TEST(InplaceVectorAssign, AssignCountValue)
{
    qx::inplace_vector<8, int> v = {1, 2, 3};
    v.assign(5, 7); // grows
    EXPECT_THAT(v, ::testing::ElementsAre(7, 7, 7, 7, 7));
    v.assign(2, 4); // shrinks
    EXPECT_THAT(v, ::testing::ElementsAre(4, 4));
    v.assign(0, 1);
    EXPECT_THAT(v, ::testing::IsEmpty());
    v.assign(8, 1); // exactly the capacity
    EXPECT_EQ(v.size(), 8u);

    qx::inplace_vector<8, Tracked> t = {1, 2, 3};
    t.assign(5, Tracked(7));
    EXPECT_THAT(t, ::testing::ElementsAre(7, 7, 7, 7, 7));
    t.assign(1, Tracked(4));
    EXPECT_THAT(t, ::testing::ElementsAre(4));
}

TEST(InplaceVectorAssign, AssignCountValueWithTwoIntegersIsNotARange)
{
    qx::inplace_vector<8, int> v = {9, 9};
    v.assign(3, 4);
    EXPECT_THAT(v, ::testing::ElementsAre(4, 4, 4));
}

TEST(InplaceVectorAssign, AssignIteratorPair)
{
    std::vector<int> src = {1, 2, 3, 4};
    qx::inplace_vector<8, int> v = {9, 9, 9, 9, 9, 9};
    v.assign(src.begin(), src.end()); // shrinks
    EXPECT_THAT(v, ::testing::ElementsAre(1, 2, 3, 4));

    v = {9};
    v.assign(src.begin(), src.end()); // grows
    EXPECT_THAT(v, ::testing::ElementsAre(1, 2, 3, 4));

    v.assign(src.begin(), src.begin());
    EXPECT_THAT(v, ::testing::IsEmpty());

    qx::inplace_vector<8, Tracked> t = {9, 9};
    t.assign(src.begin(), src.end());
    EXPECT_THAT(t, ::testing::ElementsAre(1, 2, 3, 4));
}

TEST(InplaceVectorAssign, AssignIteratorPairNonContiguousForward)
{
    std::list<int> lst{1, 2, 3};
    qx::inplace_vector<8, int> v = {9, 9, 9, 9, 9};
    v.assign(lst.begin(), lst.end());
    EXPECT_THAT(v, ::testing::ElementsAre(1, 2, 3));

    qx::inplace_vector<8, Tracked> t = {9};
    t.assign(lst.begin(), lst.end());
    EXPECT_THAT(t, ::testing::ElementsAre(1, 2, 3));
}

TEST(InplaceVectorAssign, AssignPureInputIterator)
{
    std::istringstream shrink("1 2");
    qx::inplace_vector<8, int> v = {9, 9, 9, 9};
    v.assign(std::istream_iterator<int>(shrink), std::istream_iterator<int>());
    EXPECT_THAT(v, ::testing::ElementsAre(1, 2));

    std::istringstream grow("1 2 3 4 5");
    v.assign(std::istream_iterator<int>(grow), std::istream_iterator<int>());
    EXPECT_THAT(v, ::testing::ElementsAre(1, 2, 3, 4, 5));

    std::istringstream none("");
    v.assign(std::istream_iterator<int>(none), std::istream_iterator<int>());
    EXPECT_THAT(v, ::testing::IsEmpty());
}

TEST(InplaceVectorAssign, AssignInitializerList)
{
    qx::inplace_vector<8, Tracked> t = {9, 9, 9, 9};
    t.assign({1, 2, 3});
    EXPECT_THAT(t, ::testing::ElementsAre(1, 2, 3));
    t.assign({1, 2, 3, 4, 5});
    EXPECT_THAT(t, ::testing::ElementsAre(1, 2, 3, 4, 5));
}

TEST(InplaceVectorAssign, ExceptionsAndContracts)
{
    qx::inplace_vector<4, Tracked> v = {1, 2};
    std::vector<int> big = {1, 2, 3, 4, 5};
    std::list<int> big_list(big.begin(), big.end());

    // Sized overloads throw before touching the vector.
    EXPECT_THROW(v.assign(5, Tracked(9)), std::length_error);
    EXPECT_THAT(v, ::testing::ElementsAre(1, 2));
    EXPECT_THROW(v.assign(big.begin(), big.end()), std::length_error);
    EXPECT_THAT(v, ::testing::ElementsAre(1, 2));
    EXPECT_THROW(v.assign(big_list.begin(), big_list.end()), std::length_error);
    EXPECT_THAT(v, ::testing::ElementsAre(1, 2));
    EXPECT_THROW(v.assign({1, 2, 3, 4, 5}), std::length_error);
    EXPECT_THAT(v, ::testing::ElementsAre(1, 2));
    EXPECT_THROW((v = {1, 2, 3, 4, 5}), std::length_error);
    EXPECT_THAT(v, ::testing::ElementsAre(1, 2));

    // Single-pass input cannot be sized up front: the elements that fit are kept, then it throws.
    std::istringstream stream("1 2 3 4 5 6");
    EXPECT_THROW(v.assign(std::istream_iterator<int>(stream), std::istream_iterator<int>()), std::length_error);
    EXPECT_THAT(v, ::testing::ElementsAre(1, 2, 3, 4));
}
