#include <gmock/gmock.h>

#include <qx/inplace_vector.h>

#if QX_WITH_CXX23_RANGES

#include "inplace_vector_test_utils.h"

#include <forward_list>
#include <iterator>
#include <list>
#include <memory>
#include <new>
#include <numeric>
#include <ranges>
#include <span>
#include <sstream>
#include <vector>

// Ranges (from_range, assign_range, append_range, insert_range)

namespace
{

// A sized range that is single-pass with a move-only iterator: it cannot be copied or traversed twice.
struct SizedInput : std::ranges::view_base
{
    int first;
    int n;

    struct iterator
    {
        using iterator_concept = std::input_iterator_tag;
        using value_type = int;
        using difference_type = std::ptrdiff_t;

        int cur = 0;
        int end = 0;

        iterator() = default;
        iterator(int c, int e)
            : cur(c)
            , end(e)
        {}
        iterator(iterator&&) = default;
        iterator& operator=(iterator&&) = default;

        int operator*() const { return cur; }
        iterator& operator++()
        {
            ++cur;
            return *this;
        }
        void operator++(int) { ++cur; }
        friend bool operator==(iterator const& i, std::default_sentinel_t) { return i.cur == i.end; }
    };

    iterator begin() { return iterator(first, first + n); }
    std::default_sentinel_t end() { return {}; }
    std::size_t size() const { return static_cast<std::size_t>(n); }
};

static_assert(std::ranges::input_range<SizedInput> && !std::ranges::forward_range<SizedInput> && std::ranges::sized_range<SizedInput>);

constexpr std::size_t cap = 6;

template <class T>
using IV = qx::inplace_vector<cap, T>;

template <class T>
IV<T> make(std::size_t n, int first = 1)
{
    IV<T> v;
    for (std::size_t i = 0; i < n; ++i)
        v.emplace_back(first + static_cast<int>(i));
    return v;
}

template <class T>
std::vector<int> ids_of(IV<T> const& v)
{
    std::vector<int> r;
    for (auto const& x : v)
        r.push_back(id_of(x));
    return r;
}

std::vector<int> seq(std::size_t n, int first = 1)
{
    std::vector<int> r(n);
    std::iota(r.begin(), r.end(), first);
    return r;
}

auto stream_view(std::istringstream& in)
{
    return std::ranges::istream_view<int>(in);
}

std::istringstream stream_of(std::vector<int> const& ids)
{
    std::ostringstream o;
    for (int i : ids)
        o << i << ' ';
    return std::istringstream(o.str());
}

} // namespace

template <class T>
struct InplaceVectorRanges : ::testing::Test
{};

using RangeTypes = ::testing::Types<int, Tracked>;
TYPED_TEST_SUITE(InplaceVectorRanges, RangeTypes);

TYPED_TEST(InplaceVectorRanges, FromRange)
{
    using T = TypeParam;
    auto const ids = seq(4, 10);

    EXPECT_EQ(ids_of(IV<T>(std::from_range, ids)), ids);                                           // contiguous, sized
    EXPECT_EQ(ids_of(IV<T>(std::from_range, std::list<int>(ids.begin(), ids.end()))), ids);        // bidirectional
    EXPECT_EQ(ids_of(IV<T>(std::from_range, std::views::iota(10, 14))), ids);                      // sized, random access
    EXPECT_EQ(ids_of(IV<T>(std::from_range, std::views::iota(10, 14) | std::views::filter([](int) { return true; }))), ids); // forward, unsized
    EXPECT_EQ(ids_of(IV<T>(std::from_range, SizedInput{{}, 10, 4})), ids);                          // single-pass, sized
    EXPECT_THAT(IV<T>(std::from_range, std::vector<int>{}), ::testing::IsEmpty());

    auto in = stream_of(ids);
    EXPECT_EQ(ids_of(IV<T>(std::from_range, stream_view(in))), ids); // single-pass, unsized

    std::vector<long> wider = {1, 2, 3}; // constructible from the range's reference type
    EXPECT_EQ(ids_of(IV<T>(std::from_range, wider)), seq(3));
}

TYPED_TEST(InplaceVectorRanges, FromRangeOverflow)
{
    using T = TypeParam;
    auto const ids = seq(cap + 1);

    EXPECT_THROW(IV<T>(std::from_range, ids), std::length_error);
    EXPECT_THROW(IV<T>(std::from_range, std::list<int>(ids.begin(), ids.end())), std::length_error);
    EXPECT_THROW(IV<T>(std::from_range, std::views::iota(0, static_cast<int>(cap) + 1)), std::length_error);
    EXPECT_THROW(IV<T>(std::from_range, SizedInput{{}, 0, static_cast<int>(cap) + 1}), std::length_error);
    auto in = stream_of(ids);
    EXPECT_THROW(IV<T>(std::from_range, stream_view(in)), std::length_error);

    EXPECT_NO_THROW(IV<T>(std::from_range, seq(cap)));
    EXPECT_THROW((qx::inplace_vector<0, T>(std::from_range, seq(1))), std::length_error);
    EXPECT_NO_THROW((qx::inplace_vector<0, T>(std::from_range, std::vector<int>{})));
}

TYPED_TEST(InplaceVectorRanges, AssignRange)
{
    using T = TypeParam;
    for (std::size_t size = 0; size <= cap; ++size)
        for (std::size_t n = 0; n <= cap; ++n)
        {
            SCOPED_TRACE(::testing::Message() << "size=" << size << " n=" << n);
            auto const ids = seq(n, 100);

            auto a = make<T>(size);
            a.assign_range(ids);
            EXPECT_EQ(ids_of(a), ids);

            auto b = make<T>(size);
            b.assign_range(std::views::iota(100, 100 + static_cast<int>(n)));
            EXPECT_EQ(ids_of(b), ids);

            auto c = make<T>(size);
            c.assign_range(SizedInput{{}, 100, static_cast<int>(n)});
            EXPECT_EQ(ids_of(c), ids);

            auto d = make<T>(size);
            auto in = stream_of(ids);
            d.assign_range(stream_view(in));
            EXPECT_EQ(ids_of(d), ids);

            auto e = make<T>(size);
            e.assign_range(std::list<int>(ids.begin(), ids.end()));
            EXPECT_EQ(ids_of(e), ids);
        }
}

TYPED_TEST(InplaceVectorRanges, AssignRangeOverflow)
{
    using T = TypeParam;
    auto const ids = seq(cap + 1, 100);

    auto a = make<T>(2);
    EXPECT_THROW(a.assign_range(ids), std::length_error); // a forward range is sized up front: nothing changes
    EXPECT_EQ(ids_of(a), seq(2));

    auto b = make<T>(2);
    auto in = stream_of(ids);
    EXPECT_THROW(b.assign_range(stream_view(in)), std::length_error); // single-pass: what fit was kept
    EXPECT_EQ(ids_of(b), seq(cap, 100));

    auto c = make<T>(2);
    EXPECT_THROW(c.assign_range(SizedInput{{}, 100, static_cast<int>(cap) + 1}), std::length_error);
    EXPECT_EQ(ids_of(c), seq(cap, 100));
}

TYPED_TEST(InplaceVectorRanges, AppendRange)
{
    using T = TypeParam;
    for (std::size_t size = 0; size <= cap; ++size)
        for (std::size_t n = 0; n <= cap - size; ++n)
        {
            SCOPED_TRACE(::testing::Message() << "size=" << size << " n=" << n);
            auto expected = seq(size);
            auto const ids = seq(n, 100);
            expected.insert(expected.end(), ids.begin(), ids.end());

            auto a = make<T>(size);
            a.append_range(ids);
            EXPECT_EQ(ids_of(a), expected);

            auto b = make<T>(size);
            b.append_range(std::views::iota(100, 100 + static_cast<int>(n)));
            EXPECT_EQ(ids_of(b), expected);

            auto c = make<T>(size);
            c.append_range(SizedInput{{}, 100, static_cast<int>(n)});
            EXPECT_EQ(ids_of(c), expected);

            auto d = make<T>(size);
            auto in = stream_of(ids);
            d.append_range(stream_view(in));
            EXPECT_EQ(ids_of(d), expected);

            auto e = make<T>(size);
            e.append_range(std::forward_list<int>(ids.begin(), ids.end()));
            EXPECT_EQ(ids_of(e), expected);
        }
}

TYPED_TEST(InplaceVectorRanges, AppendRangeOverflow)
{
    using T = TypeParam;
    auto const ids = seq(4, 100);

    auto a = make<T>(4);
    EXPECT_THROW(a.append_range(ids), std::length_error); // 4 + 4 > cap: forward ranges are sized up front, nothing changes
    EXPECT_EQ(ids_of(a), seq(4));

    auto b = make<T>(4);
    auto in = stream_of(ids);
    EXPECT_THROW(b.append_range(stream_view(in)), std::length_error); // single-pass: what fit was kept, no staging buffer
    EXPECT_EQ(ids_of(b), (std::vector<int>{1, 2, 3, 4, 100, 101}));

    auto c = make<T>(cap);
    EXPECT_NO_THROW(c.append_range(std::vector<int>{})); // an empty range fits in a full vector
    auto in2 = stream_of({});
    EXPECT_NO_THROW(c.append_range(stream_view(in2)));
    EXPECT_EQ(ids_of(c), seq(cap));
}

TYPED_TEST(InplaceVectorRanges, InsertRange)
{
    using T = TypeParam;
    for (std::size_t size = 0; size <= cap; ++size)
        for (std::size_t pos = 0; pos <= size; ++pos)
            for (std::size_t n = 0; n <= cap - size; ++n)
            {
                SCOPED_TRACE(::testing::Message() << "size=" << size << " pos=" << pos << " n=" << n);
                auto expected = seq(size);
                auto const ids = seq(n, 100);
                expected.insert(expected.begin() + static_cast<std::ptrdiff_t>(pos), ids.begin(), ids.end());
                auto const where = static_cast<std::ptrdiff_t>(pos);

                auto a = make<T>(size);
                auto ia = a.insert_range(a.begin() + where, ids);
                EXPECT_EQ(ids_of(a), expected);
                EXPECT_EQ(ia - a.begin(), where);

                auto b = make<T>(size);
                b.insert_range(b.begin() + where, std::views::iota(100, 100 + static_cast<int>(n)));
                EXPECT_EQ(ids_of(b), expected);

                auto c = make<T>(size);
                auto ic = c.insert_range(c.begin() + where, SizedInput{{}, 100, static_cast<int>(n)});
                EXPECT_EQ(ids_of(c), expected);
                EXPECT_EQ(ic - c.begin(), where);

                auto d = make<T>(size);
                auto in = stream_of(ids);
                auto id = d.insert_range(d.begin() + where, stream_view(in));
                EXPECT_EQ(ids_of(d), expected);
                EXPECT_EQ(id - d.begin(), where);

                auto e = make<T>(size);
                e.insert_range(e.begin() + where, std::list<int>(ids.begin(), ids.end()));
                EXPECT_EQ(ids_of(e), expected);
            }
}

TYPED_TEST(InplaceVectorRanges, InsertRangeOverflow)
{
    using T = TypeParam;
    auto const ids = seq(4, 100);

    auto a = make<T>(4);
    EXPECT_THROW(a.insert_range(a.begin() + 1, ids), std::length_error);
    EXPECT_EQ(ids_of(a), seq(4)); // sized up front: nothing changes

    auto b = make<T>(4);
    auto in = stream_of(ids);
    EXPECT_THROW(b.insert_range(b.begin() + 1, stream_view(in)), std::length_error);
    EXPECT_EQ(ids_of(b), (std::vector<int>{1, 100, 101, 2, 3, 4})); // single-pass: the prefix that fit stays at the position

    auto c = make<T>(4);
    EXPECT_THROW(c.insert_range(c.begin() + 1, SizedInput{{}, 100, 4}), std::length_error);
    EXPECT_EQ(ids_of(c), (std::vector<int>{1, 100, 101, 2, 3, 4}));
}

TEST(InplaceVectorRangesMoveOnly, ElementsAreMovedFromRvalueRanges)
{
    std::vector<std::unique_ptr<int>> src;
    for (int i = 0; i < 3; ++i)
        src.push_back(std::make_unique<int>(i));

    qx::inplace_vector<6, std::unique_ptr<int>> v(std::from_range, src | std::views::as_rvalue);
    ASSERT_EQ(v.size(), 3u);
    EXPECT_EQ(*v[2], 2);
    EXPECT_THAT(src, ::testing::Each(::testing::IsNull()));

    std::vector<std::unique_ptr<int>> more;
    more.push_back(std::make_unique<int>(7));
    more.push_back(std::make_unique<int>(8));
    v.append_range(more | std::views::as_rvalue);
    v.insert_range(v.begin(), std::vector<std::unique_ptr<int>>{} | std::views::as_rvalue);
    ASSERT_EQ(v.size(), 5u);
    EXPECT_EQ(*v[3], 7);
    EXPECT_EQ(*v[4], 8);
}

TEST(InplaceVectorRangesContracts, InsertRangePositionMustBeInRange)
{
    using V = qx::inplace_vector<10, int>;
    EXPECT_DEATH(
        {
            V v(3, 1);
            v.insert_range(v.begin() + 4, std::vector<int>{1});
        },
        "contract violation");
}

#endif
