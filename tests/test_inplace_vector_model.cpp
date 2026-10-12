#include <gmock/gmock.h>

#include <qx/inplace_vector.h>

#include "inplace_vector_test_utils.h"

#include <algorithm>
#include <array>
#include <forward_list>
#include <iterator>
#include <list>
#include <memory>
#include <new>
#include <numeric>
#include <sstream>
#include <vector>

// Differential tests: every modifier runs on every (size, position, count) combination and is compared with std::vector<int>
// as the model, for element types that select different storage layers (trivial, non-trivial, trivially destructible with a
// user-provided copy, trivially copyable with a user-provided move).

namespace
{

constexpr std::size_t cap = 6;

template <class T>
using IV = qx::inplace_vector<cap, T>;

std::vector<int> seq(std::size_t n, int first = 1)
{
    std::vector<int> r(n);
    std::iota(r.begin(), r.end(), first);
    return r;
}

template <class T>
IV<T> make(std::vector<int> const& ids)
{
    IV<T> v;
    for (int i : ids)
        v.emplace_back(i);
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

std::istringstream stream_of(std::vector<int> const& ids)
{
    std::ostringstream o;
    for (int i : ids)
        o << i << ' ';
    return std::istringstream(o.str());
}

std::ptrdiff_t as_diff(std::size_t n)
{
    return static_cast<std::ptrdiff_t>(n);
}

#define TRACE(...) SCOPED_TRACE(::testing::Message() << __VA_ARGS__)

} // namespace

template <class T>
struct InplaceVectorModel : ::testing::Test
{};

using ModelTypes = ::testing::Types<int, Tracked, CopyCounted, MoveCounted>;
TYPED_TEST_SUITE(InplaceVectorModel, ModelTypes);

TYPED_TEST(InplaceVectorModel, InsertSingle)
{
    using T = TypeParam;
    for (std::size_t size = 0; size <= cap; ++size)
        for (std::size_t pos = 0; pos <= size; ++pos)
        {
            TRACE("size=" << size << " pos=" << pos);
            auto const model = seq(size);
            auto expected = model;
            if (size < cap)
                expected.insert(expected.begin() + as_diff(pos), 100);

            auto v = make<T>(model);
            if (size == cap)
            {
                T const x(100);
                EXPECT_THROW(v.insert(v.begin() + as_diff(pos), x), std::length_error);
                EXPECT_THROW(v.insert(v.begin() + as_diff(pos), T(100)), std::length_error);
                EXPECT_THROW(v.emplace(v.begin() + as_diff(pos), 100), std::length_error);
                EXPECT_EQ(ids_of(v), model);
                continue;
            }

            {
                auto a = v;
                T const x(100);
                auto it = a.insert(a.begin() + as_diff(pos), x);
                EXPECT_EQ(ids_of(a), expected);
                EXPECT_EQ(it - a.begin(), as_diff(pos));
            }
            {
                auto a = v;
                auto it = a.insert(a.begin() + as_diff(pos), T(100));
                EXPECT_EQ(ids_of(a), expected);
                EXPECT_EQ(it - a.begin(), as_diff(pos));
            }
            {
                auto a = v;
                auto it = a.emplace(a.begin() + as_diff(pos), 100);
                EXPECT_EQ(ids_of(a), expected);
                EXPECT_EQ(it - a.begin(), as_diff(pos));
            }
            for (std::size_t j = 0; j < size; ++j) // the source is an element that the insertion shifts
            {
                auto a = v;
                auto m = model;
                m.insert(m.begin() + as_diff(pos), model[j]);
                a.insert(a.begin() + as_diff(pos), a[j]);
                EXPECT_EQ(ids_of(a), m) << "aliased element " << j;
            }
        }
}

TYPED_TEST(InplaceVectorModel, InsertCount)
{
    using T = TypeParam;
    for (std::size_t size = 0; size <= cap; ++size)
        for (std::size_t pos = 0; pos <= size; ++pos)
            for (std::size_t n = 0; n <= cap + 1; ++n)
            {
                TRACE("size=" << size << " pos=" << pos << " n=" << n);
                auto const model = seq(size);
                auto v = make<T>(model);
                T const x(100);

                if (n > cap - size)
                {
                    EXPECT_THROW(v.insert(v.begin() + as_diff(pos), n, x), std::length_error);
                    EXPECT_EQ(ids_of(v), model);
                    continue;
                }

                auto expected = model;
                expected.insert(expected.begin() + as_diff(pos), n, 100);
                auto it = v.insert(v.begin() + as_diff(pos), n, x);
                EXPECT_EQ(ids_of(v), expected);
                EXPECT_EQ(it - v.begin(), as_diff(pos));

                for (std::size_t j = 0; j < size; ++j) // the source is an element that the insertion shifts
                {
                    auto a = make<T>(model);
                    auto m = model;
                    m.insert(m.begin() + as_diff(pos), n, model[j]);
                    a.insert(a.begin() + as_diff(pos), n, a[j]);
                    EXPECT_EQ(ids_of(a), m) << "aliased element " << j;
                }
            }
}

// The value to insert is detected as "one of our elements" by its address. Values that live elsewhere must not be mistaken
// for elements wherever they are: above the vector (a stack variable next to a heap vector) or below it (a heap value next
// to a stack vector).
TYPED_TEST(InplaceVectorModel, InsertExternalValueAtAnyAddress)
{
    using T = TypeParam;
    T const stack_x(100);
    auto const heap_x = std::make_unique<T>(100);

    for (std::size_t size = 0; size < cap; ++size)
        for (std::size_t pos = 0; pos <= size; ++pos)
            for (std::size_t n = 0; n <= cap - size; ++n)
            {
                TRACE("size=" << size << " pos=" << pos << " n=" << n);
                auto const model = seq(size);
                auto expected = model;
                expected.insert(expected.begin() + as_diff(pos), n, 100);
                auto expected_single = model;
                expected_single.insert(expected_single.begin() + as_diff(pos), 100);

                for (T const* x : std::array<T const*, 2>{&stack_x, heap_x.get()})
                {
                    auto on_stack = make<T>(model);
                    on_stack.insert(on_stack.begin() + as_diff(pos), n, *x);
                    EXPECT_EQ(ids_of(on_stack), expected);

                    auto on_heap = std::make_unique<IV<T>>(make<T>(model)); // stack variables are above a heap vector
                    on_heap->insert(on_heap->begin() + as_diff(pos), n, *x);
                    EXPECT_EQ(ids_of(*on_heap), expected);

                    on_heap = std::make_unique<IV<T>>(make<T>(model));
                    on_heap->insert(on_heap->begin() + as_diff(pos), *x);
                    EXPECT_EQ(ids_of(*on_heap), expected_single);

                    on_stack = make<T>(model);
                    on_stack.insert(on_stack.begin() + as_diff(pos), *x);
                    EXPECT_EQ(ids_of(on_stack), expected_single);
                }
            }
}

TYPED_TEST(InplaceVectorModel, InsertForwardRange)
{
    using T = TypeParam;
    auto check = [](auto const& source, std::size_t n, std::size_t size, std::size_t pos)
    {
        TRACE("size=" << size << " pos=" << pos << " n=" << n);
        auto const model = seq(size);
        auto v = make<T>(model);
        auto const first = source.begin();
        auto const last = source.end();
        if (n > cap - size)
        {
            EXPECT_THROW(v.insert(v.begin() + as_diff(pos), first, last), std::length_error);
            EXPECT_EQ(ids_of(v), model);
            return;
        }
        auto expected = model;
        auto const ids = seq(n, 100);
        expected.insert(expected.begin() + as_diff(pos), ids.begin(), ids.end());
        auto it = v.insert(v.begin() + as_diff(pos), first, last);
        EXPECT_EQ(ids_of(v), expected);
        EXPECT_EQ(it - v.begin(), as_diff(pos));
    };

    for (std::size_t size = 0; size <= cap; ++size)
        for (std::size_t pos = 0; pos <= size; ++pos)
            for (std::size_t n = 0; n <= cap + 1; ++n)
            {
                auto const ids = seq(n, 100);
                check(ids, n, size, pos);                                           // contiguous
                check(std::list<int>(ids.begin(), ids.end()), n, size, pos);         // bidirectional
                check(std::forward_list<int>(ids.begin(), ids.end()), n, size, pos); // forward
            }
}

TYPED_TEST(InplaceVectorModel, InsertSinglePassRangeKeepsThePrefixThatFits)
{
    using T = TypeParam;
    for (std::size_t size = 0; size <= cap; ++size)
        for (std::size_t pos = 0; pos <= size; ++pos)
            for (std::size_t n = 0; n <= cap + 1; ++n)
            {
                TRACE("size=" << size << " pos=" << pos << " n=" << n);
                auto const model = seq(size);
                auto v = make<T>(model);
                auto in = stream_of(seq(n, 100));

                bool const overflow = n > cap - size;
                auto const ids = seq(std::min(n, cap - size), 100);
                auto expected = model;
                expected.insert(expected.begin() + as_diff(pos), ids.begin(), ids.end());

                if (overflow)
                {
                    EXPECT_THROW(v.insert(v.begin() + as_diff(pos), std::istream_iterator<int>(in), std::istream_iterator<int>()),
                        std::length_error);
                }
                else
                {
                    auto it = v.insert(v.begin() + as_diff(pos), std::istream_iterator<int>(in), std::istream_iterator<int>());
                    EXPECT_EQ(it - v.begin(), as_diff(pos));
                }
                EXPECT_EQ(ids_of(v), expected);
            }
}

TYPED_TEST(InplaceVectorModel, InsertInitializerList)
{
    using T = TypeParam;
    for (std::size_t size = 0; size + 2 <= cap; ++size)
        for (std::size_t pos = 0; pos <= size; ++pos)
        {
            auto const model = seq(size);
            auto v = make<T>(model);
            auto expected = model;
            expected.insert(expected.begin() + as_diff(pos), {100, 101});
            v.insert(v.begin() + as_diff(pos), {100, 101});
            EXPECT_EQ(ids_of(v), expected) << "size=" << size << " pos=" << pos;
        }
}

TYPED_TEST(InplaceVectorModel, Erase)
{
    using T = TypeParam;
    for (std::size_t size = 0; size <= cap; ++size)
    {
        auto const model = seq(size);
        for (std::size_t first = 0; first < size; ++first)
        {
            auto v = make<T>(model);
            auto expected = model;
            expected.erase(expected.begin() + as_diff(first));
            auto it = v.erase(v.begin() + as_diff(first));
            EXPECT_EQ(ids_of(v), expected) << "size=" << size << " first=" << first;
            EXPECT_EQ(it - v.begin(), as_diff(first));
        }
        for (std::size_t first = 0; first <= size; ++first)
            for (std::size_t last = first; last <= size; ++last)
            {
                auto v = make<T>(model);
                auto expected = model;
                expected.erase(expected.begin() + as_diff(first), expected.begin() + as_diff(last));
                auto it = v.erase(v.begin() + as_diff(first), v.begin() + as_diff(last));
                EXPECT_EQ(ids_of(v), expected) << "size=" << size << " range=[" << first << ", " << last << ")";
                EXPECT_EQ(it - v.begin(), as_diff(first));
            }
    }
}

TYPED_TEST(InplaceVectorModel, Assign)
{
    using T = TypeParam;
    for (std::size_t size = 0; size <= cap; ++size)
        for (std::size_t n = 0; n <= cap + 1; ++n)
        {
            TRACE("size=" << size << " n=" << n);
            auto const model = seq(size);
            auto const ids = seq(n, 100);
            bool const overflow = n > cap;
            T const x(100);

            {
                auto v = make<T>(model);
                if (overflow)
                {
                    EXPECT_THROW(v.assign(n, x), std::length_error);
                    EXPECT_EQ(ids_of(v), model);
                }
                else
                {
                    v.assign(n, x);
                    EXPECT_EQ(ids_of(v), std::vector<int>(n, 100));
                }
            }
            {
                auto v = make<T>(model);
                if (overflow)
                {
                    EXPECT_THROW(v.assign(ids.begin(), ids.end()), std::length_error);
                    EXPECT_EQ(ids_of(v), model);
                }
                else
                {
                    v.assign(ids.begin(), ids.end());
                    EXPECT_EQ(ids_of(v), ids);
                }
            }
            {
                std::list<int> const l(ids.begin(), ids.end());
                auto v = make<T>(model);
                if (overflow)
                {
                    EXPECT_THROW(v.assign(l.begin(), l.end()), std::length_error);
                    EXPECT_EQ(ids_of(v), model);
                }
                else
                {
                    v.assign(l.begin(), l.end());
                    EXPECT_EQ(ids_of(v), ids);
                }
            }
            {
                auto v = make<T>(model);
                auto in = stream_of(ids);
                auto const kept = seq(std::min(n, cap), 100);
                if (overflow)
                    EXPECT_THROW(v.assign(std::istream_iterator<int>(in), std::istream_iterator<int>()), std::length_error);
                else
                    v.assign(std::istream_iterator<int>(in), std::istream_iterator<int>());
                EXPECT_EQ(ids_of(v), kept); // single-pass input keeps what fit
            }
        }
}

TYPED_TEST(InplaceVectorModel, Resize)
{
    using T = TypeParam;
    for (std::size_t size = 0; size <= cap; ++size)
        for (std::size_t n = 0; n <= cap + 1; ++n)
        {
            TRACE("size=" << size << " n=" << n);
            auto const model = seq(size);
            T const x(100);
            auto v = make<T>(model);
            auto w = make<T>(model);
            if (n > cap)
            {
                EXPECT_THROW(v.resize(n), std::length_error);
                EXPECT_THROW(w.resize(n, x), std::length_error);
                EXPECT_EQ(ids_of(v), model);
                EXPECT_EQ(ids_of(w), model);
                continue;
            }
            auto expected = model;
            expected.resize(n);
            v.resize(n);
            EXPECT_EQ(ids_of(v), expected);

            expected = model;
            expected.resize(n, 100);
            w.resize(n, x);
            EXPECT_EQ(ids_of(w), expected);
        }
}

TYPED_TEST(InplaceVectorModel, CopyMoveSwap)
{
    using T = TypeParam;
    for (std::size_t sa = 0; sa <= cap; ++sa)
        for (std::size_t sb = 0; sb <= cap; ++sb)
        {
            TRACE("sa=" << sa << " sb=" << sb);
            auto const ma = seq(sa, 1);
            auto const mb = seq(sb, 50);
            auto const a = make<T>(ma);
            auto const b = make<T>(mb);

            {
                IV<T> c(a);
                EXPECT_EQ(ids_of(c), ma);
            }
            {
                IV<T> c(b);
                c = a;
                EXPECT_EQ(ids_of(c), ma);
                EXPECT_EQ(ids_of(a), ma);
            }
            {
                auto src = a;
                IV<T> c(std::move(src));
                EXPECT_EQ(ids_of(c), ma);
            }
            {
                auto src = a;
                IV<T> c(b);
                c = std::move(src);
                EXPECT_EQ(ids_of(c), ma);
            }
            {
                auto c = a;
                auto d = b;
                c.swap(d);
                EXPECT_EQ(ids_of(c), mb);
                EXPECT_EQ(ids_of(d), ma);
                swap(c, d);
                EXPECT_EQ(ids_of(c), ma);
                EXPECT_EQ(ids_of(d), mb);
            }
        }
}

TYPED_TEST(InplaceVectorModel, Compare)
{
    using T = TypeParam;
    std::vector<std::vector<int>> sequences = {{}};
    for (std::size_t len = 1; len <= 3; ++len)
        for (int code = 0; code < (1 << len); ++code)
        {
            std::vector<int> s;
            for (std::size_t i = 0; i < len; ++i)
                s.push_back(1 + ((code >> i) & 1));
            sequences.push_back(s);
        }

    for (auto const& x : sequences)
        for (auto const& y : sequences)
        {
            auto const a = make<T>(x);
            auto const b = make<T>(y);
            EXPECT_EQ(a == b, x == y);
            EXPECT_EQ(a != b, x != y);
            EXPECT_EQ(a < b, x < y);
            EXPECT_EQ(a > b, x > y);
            EXPECT_EQ(a <= b, x <= y);
            EXPECT_EQ(a >= b, x >= y);
        }
}
