#include <gmock/gmock.h>

#include <qx/inplace_vector.h>

#include "inplace_vector_test_utils.h"

#include <iterator>
#include <list>
#include <memory>
#include <sstream>
#include <vector>

// Insert (insert, emplace)

TEST(InplaceVectorInsert, BasicInsert)
{
    qx::inplace_vector<10, int> v = {1, 2, 3, 4};
    int const x = 9;

    auto it = v.insert(v.begin() + 1, x);
    EXPECT_THAT(v, ::testing::ElementsAre(1, 9, 2, 3, 4));
    EXPECT_EQ(it, v.begin() + 1);

    v.insert(v.begin(), 8);
    EXPECT_THAT(v, ::testing::ElementsAre(8, 1, 9, 2, 3, 4));

    it = v.insert(v.end(), 7);
    EXPECT_THAT(v, ::testing::ElementsAre(8, 1, 9, 2, 3, 4, 7));
    EXPECT_EQ(it, v.end() - 1);
}

TEST(InplaceVectorInsert, BasicInsertIntoEmpty)
{
    qx::inplace_vector<10, int> v;
    v.insert(v.begin(), 1);
    EXPECT_THAT(v, ::testing::ElementsAre(1));
}

TEST(InplaceVectorInsert, BasicInsertNonTrivial)
{
    qx::inplace_vector<10, Tracked> v = {1, 2, 3, 4};
    Tracked const x(9);

    v.insert(v.begin() + 1, x);
    EXPECT_THAT(v, ::testing::ElementsAre(1, 9, 2, 3, 4));
    v.insert(v.begin(), Tracked(8));
    EXPECT_THAT(v, ::testing::ElementsAre(8, 1, 9, 2, 3, 4));
    v.insert(v.end(), Tracked(7));
    EXPECT_THAT(v, ::testing::ElementsAre(8, 1, 9, 2, 3, 4, 7));
}

TEST(InplaceVectorInsert, InsertElementOfSelf)
{
    qx::inplace_vector<10, int> v = {1, 2, 3, 4};
    v.insert(v.begin() + 1, v[2]); // the source element is shifted by the insertion
    EXPECT_THAT(v, ::testing::ElementsAre(1, 3, 2, 3, 4));

    qx::inplace_vector<10, Tracked> t = {1, 2, 3, 4};
    t.insert(t.begin(), t[2]);
    EXPECT_THAT(t, ::testing::ElementsAre(3, 1, 2, 3, 4));
    t.insert(t.end(), t[0]);
    EXPECT_THAT(t, ::testing::ElementsAre(3, 1, 2, 3, 4, 3));
}

TEST(InplaceVectorInsert, InsertCountValue)
{
    qx::inplace_vector<10, int> v = {1, 2, 3, 4};
    int const x = 9;

    v.insert(v.begin() + 1, 2, x); // fewer new elements than the tail
    EXPECT_THAT(v, ::testing::ElementsAre(1, 9, 9, 2, 3, 4));

    v = {1, 2, 3, 4};
    v.insert(v.begin() + 3, 5, x); // more new elements than the tail
    EXPECT_THAT(v, ::testing::ElementsAre(1, 2, 3, 9, 9, 9, 9, 9, 4));

    v = {1, 2, 3, 4};
    v.insert(v.end(), 3, x);
    EXPECT_THAT(v, ::testing::ElementsAre(1, 2, 3, 4, 9, 9, 9));

    v.insert(v.begin(), 0, x); // n == 0: no-op
    EXPECT_THAT(v, ::testing::ElementsAre(1, 2, 3, 4, 9, 9, 9));
}

TEST(InplaceVectorInsert, InsertCountValueNonTrivial)
{
    qx::inplace_vector<10, Tracked> v = {1, 2, 3, 4};
    Tracked const x(9);

    v.insert(v.begin() + 1, 2, x);
    EXPECT_THAT(v, ::testing::ElementsAre(1, 9, 9, 2, 3, 4));

    v = {1, 2, 3, 4};
    v.insert(v.begin() + 3, 5, x);
    EXPECT_THAT(v, ::testing::ElementsAre(1, 2, 3, 9, 9, 9, 9, 9, 4));
}

TEST(InplaceVectorInsert, InsertCountValueWithTwoIntegersIsNotARange)
{
    qx::inplace_vector<10, int> v = {9};
    v.insert(v.begin(), 3, 4);
    EXPECT_THAT(v, ::testing::ElementsAre(4, 4, 4, 9));
}

TEST(InplaceVectorInsert, InsertCountOfElementOfSelf)
{
    qx::inplace_vector<10, int> v = {1, 2, 3, 4};
    v.insert(v.begin() + 1, 2, v[2]); // fewer new elements than the tail
    EXPECT_THAT(v, ::testing::ElementsAre(1, 3, 3, 2, 3, 4));

    v = {1, 2, 3, 4};
    v.insert(v.begin() + 2, 5, v[3]); // more new elements than the tail
    EXPECT_THAT(v, ::testing::ElementsAre(1, 2, 4, 4, 4, 4, 4, 3, 4));

    qx::inplace_vector<10, Tracked> t = {1, 2, 3, 4};
    t.insert(t.begin() + 2, 5, t[3]);
    EXPECT_THAT(t, ::testing::ElementsAre(1, 2, 4, 4, 4, 4, 4, 3, 4));
}

TEST(InplaceVectorInsert, InsertIterators)
{
    std::vector<int> src = {7, 8, 9};
    qx::inplace_vector<10, int> v = {1, 2, 3, 4};

    auto it = v.insert(v.begin() + 1, src.begin(), src.end()); // fewer new elements than the tail
    EXPECT_THAT(v, ::testing::ElementsAre(1, 7, 8, 9, 2, 3, 4));
    EXPECT_EQ(it, v.begin() + 1);

    v = {1, 2, 3, 4};
    v.insert(v.begin() + 3, src.begin(), src.end()); // more new elements than the tail
    EXPECT_THAT(v, ::testing::ElementsAre(1, 2, 3, 7, 8, 9, 4));

    v = {1, 2, 3, 4};
    v.insert(v.end(), src.begin(), src.end());
    EXPECT_THAT(v, ::testing::ElementsAre(1, 2, 3, 4, 7, 8, 9));
}

TEST(InplaceVectorInsert, InsertIteratorsNonContiguous)
{
    std::list<int> lst = {7, 8, 9};
    qx::inplace_vector<10, Tracked> v = {1, 2, 3, 4};
    v.insert(v.begin() + 1, lst.begin(), lst.end());
    EXPECT_THAT(v, ::testing::ElementsAre(1, 7, 8, 9, 2, 3, 4));

    v = {1, 2, 3, 4};
    v.insert(v.begin() + 3, lst.begin(), lst.end());
    EXPECT_THAT(v, ::testing::ElementsAre(1, 2, 3, 7, 8, 9, 4));
}

TEST(InplaceVectorInsert, InsertIteratorsEmptyRange)
{
    std::vector<int> empty;
    qx::inplace_vector<10, int> v = {1, 2, 3};
    auto it = v.insert(v.begin() + 1, empty.begin(), empty.end());
    EXPECT_THAT(v, ::testing::ElementsAre(1, 2, 3));
    EXPECT_EQ(*it, 2);
}

TEST(InplaceVectorInsert, InsertPureInputIterator)
{
    std::istringstream stream("7 8");
    qx::inplace_vector<10, int> v = {1, 2, 3};
    auto it = v.insert(v.begin() + 1, std::istream_iterator<int>(stream), std::istream_iterator<int>());
    EXPECT_THAT(v, ::testing::ElementsAre(1, 7, 8, 2, 3));
    EXPECT_EQ(it, v.begin() + 1);

    std::istringstream none("");
    v.insert(v.begin(), std::istream_iterator<int>(none), std::istream_iterator<int>());
    EXPECT_THAT(v, ::testing::ElementsAre(1, 7, 8, 2, 3));
}

TEST(InplaceVectorInsert, InsertInitializerList)
{
    qx::inplace_vector<10, Tracked> v = {1, 2, 3};
    auto it = v.insert(v.begin() + 1, {50, 51});
    EXPECT_THAT(v, ::testing::ElementsAre(1, 50, 51, 2, 3));
    EXPECT_EQ(it, v.begin() + 1);
}

TEST(InplaceVectorInsert, Emplace)
{
    qx::inplace_vector<10, int> v = {1, 2, 3};
    auto it = v.emplace(v.begin() + 1, 9);
    EXPECT_THAT(v, ::testing::ElementsAre(1, 9, 2, 3));
    EXPECT_EQ(it, v.begin() + 1);

    v.emplace(v.end(), 8);
    v.emplace(v.begin(), 7);
    EXPECT_THAT(v, ::testing::ElementsAre(7, 1, 9, 2, 3, 8));

    qx::inplace_vector<10, std::pair<int, std::string>> p;
    p.emplace(p.begin(), 1, "one");
    p.emplace(p.begin(), 0, "zero");
    ASSERT_EQ(p.size(), 2u);
    EXPECT_EQ(p[0].second, "zero");
    EXPECT_EQ(p[1].second, "one");
}

TEST(InplaceVectorInsert, EmplaceArgumentAliasesElement)
{
    qx::inplace_vector<10, Tracked> v = {1, 2, 3, 4};
    v.emplace(v.begin(), v[3].id); // the argument is read before the elements shift
    EXPECT_THAT(v, ::testing::ElementsAre(4, 1, 2, 3, 4));
}

TEST(InplaceVectorInsert, MoveOnlyElement)
{
    qx::inplace_vector<4, std::unique_ptr<int>> v;
    v.emplace(v.begin(), new int(1));
    v.insert(v.begin(), std::make_unique<int>(0));
    v.insert(v.begin() + 1, std::make_unique<int>(5));
    ASSERT_EQ(v.size(), 3u);
    EXPECT_EQ(*v[0], 0);
    EXPECT_EQ(*v[1], 5);
    EXPECT_EQ(*v[2], 1);
}

TEST(InplaceVectorInsert, InputIteratorOverflowKeepsInsertedPrefixAtPosition)
{
    qx::inplace_vector<8, int> v = {1, 2, 3, 4, 5, 6}; // room for two
    std::istringstream stream("100 101 102 103");

    EXPECT_THROW(v.insert(v.begin() + 1, std::istream_iterator<int>(stream), std::istream_iterator<int>()), std::length_error);
    EXPECT_THAT(v, ::testing::ElementsAre(1, 100, 101, 2, 3, 4, 5, 6));
}

TEST(InplaceVectorInsert, ExceptionsAndContracts)
{
    qx::inplace_vector<4, Tracked> full = {1, 2, 3, 4};
    Tracked const x(9);
    std::vector<int> one = {9};
    std::list<int> one_list = {9};

    EXPECT_THROW(full.insert(full.begin() + 2, x), std::length_error);
    EXPECT_THROW(full.insert(full.begin() + 2, Tracked(9)), std::length_error);
    EXPECT_THROW(full.insert(full.end(), x), std::length_error);
    EXPECT_THROW(full.emplace(full.begin() + 2, 9), std::length_error);
    EXPECT_THROW(full.insert(full.begin() + 2, 1, x), std::length_error);
    EXPECT_THROW(full.insert(full.begin() + 2, one.begin(), one.end()), std::length_error);
    EXPECT_THROW(full.insert(full.begin() + 2, one_list.begin(), one_list.end()), std::length_error);
    EXPECT_THROW(full.insert(full.begin() + 2, {9}), std::length_error);
    EXPECT_THAT(full, ::testing::ElementsAre(1, 2, 3, 4)); // sized overloads throw before touching the vector

    qx::inplace_vector<4, Tracked> v = {1, 2, 3};
    EXPECT_THROW(v.insert(v.begin(), 2, x), std::length_error);
    EXPECT_THAT(v, ::testing::ElementsAre(1, 2, 3));
}
