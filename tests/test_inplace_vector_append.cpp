#include <gmock/gmock.h>

#include <qx/inplace_vector.h>

#include "inplace_vector_test_utils.h"

#include <memory>
#include <string>
#include <utility>

// Append (push_back, emplace_back, pop_back)

TEST(InplaceVectorAppend, PushBack)
{
    qx::inplace_vector<5, int> v;
    int const lvalue = 1;
    int& first = v.push_back(lvalue);
    int& second = v.push_back(2);
    EXPECT_EQ(&first, &v[0]);
    EXPECT_EQ(&second, &v[1]);
    EXPECT_THAT(v, ::testing::ElementsAre(1, 2));

    qx::inplace_vector<5, Tracked> t;
    Tracked const tracked(1);
    t.push_back(tracked);
    t.push_back(Tracked(2));
    EXPECT_THAT(t, ::testing::ElementsAre(1, 2));
    EXPECT_EQ(tracked, 1); // lvalue not consumed
}

TEST(InplaceVectorAppend, PushBackSelfElement)
{
    qx::inplace_vector<8, Tracked> v = {1, 2, 3};
    v.push_back(v[0]);
    v.push_back(v.back());
    EXPECT_THAT(v, ::testing::ElementsAre(1, 2, 3, 1, 1));
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
    EXPECT_THAT(v, ::testing::ElementsAre(1, 2));
    v.pop_back();
    v.pop_back();
    EXPECT_THAT(v, ::testing::IsEmpty());

    v.push_back(9); // storage is reusable once emptied
    EXPECT_THAT(v, ::testing::ElementsAre(9));
}

TEST(InplaceVectorAppend, FillToCapacity)
{
    qx::inplace_vector<3, Tracked> v;
    for (int i = 1; i <= 3; ++i)
        v.push_back(i);
    EXPECT_THAT(v, ::testing::ElementsAre(1, 2, 3));
    EXPECT_EQ(v.size(), v.capacity());
}

TEST(InplaceVectorAppend, ExceptionsAndContracts)
{
    qx::inplace_vector<2, Tracked> full = {1, 2};
    Tracked const lvalue(9);
    EXPECT_THROW(full.push_back(lvalue), std::length_error);
    EXPECT_THROW(full.push_back(Tracked(9)), std::length_error);
    EXPECT_THROW(full.emplace_back(9), std::length_error);
    EXPECT_THAT(full, ::testing::ElementsAre(1, 2)); // unchanged

    using V = qx::inplace_vector<2, int>;
    EXPECT_DEATH(
        {
            V empty;
            empty.pop_back();
        },
        "contract violation");
}
