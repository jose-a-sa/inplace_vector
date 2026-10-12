#include <gmock/gmock.h>

#include <qx/inplace_vector.h>

#include "inplace_vector_test_utils.h"

#if __cplusplus >= 202002L

#include <array>
#include <iterator>
#include <string>
#include <type_traits>
#include <utility>

#if QX_WITH_CXX23_RANGES
#include <ranges>
#endif

// Constexpr
//
// Everything that needs no element to be constructed is constexpr from C++20: the observers, iterators, comparison, clear,
// erase, pop_back and the (destructor of the) vector itself, using only std facilities that are constexpr in C++20.
// Everything that constructs elements goes through the std::uninitialized_* algorithms or placement new, which are
// constexpr from C++26 (P3508R0, P2747R2; the union storage also needs P3074R7), and is constexpr when they are.
//
// Every check runs twice: in a static_assert (constant evaluation: no undefined behavior, no exception), and as an
// ordinary test, so the same code is also checked at run time under the sanitizers.

// Set when the standard library can construct elements in a constant expression.
#if __cplusplus > 202302L && defined(__cpp_lib_raw_memory_algorithms) && __cpp_lib_raw_memory_algorithms >= 202411L && __cpp_constexpr >= 202406L
#define QX_TEST_CONSTEXPR_MUTATORS 1
#else
#define QX_TEST_CONSTEXPR_MUTATORS 0
#endif

namespace
{

#define CHECK(...)                                                                                                                         \
    if (!(__VA_ARGS__))                                                                                                                    \
    return false

using V = qx::inplace_vector<8, int>;

template <class Vec>
constexpr bool is(Vec const& v, std::initializer_list<int> expected)
{
    if (v.size() != expected.size())
        return false;
    std::size_t i = 0;
    for (int e : expected)
    {
        if (!(v[i] == e))
            return false;
        ++i;
    }
    return true;
}

// Input/forward iterators over 0, 1, 2, ... (prvalue reference): the single-pass and the multi-pass constructors
template <class Category>
struct Counting
{
    using iterator_category = Category;
    using value_type = int;
    using difference_type = std::ptrdiff_t;
    using pointer = int const*;
    using reference = int;

    int cur = 0;

    constexpr Counting() = default;
    constexpr explicit Counting(int c)
        : cur(c)
    {}

    constexpr int operator*() const { return cur; }
    constexpr Counting& operator++()
    {
        ++cur;
        return *this;
    }
    constexpr Counting operator++(int)
    {
        Counting t = *this;
        ++cur;
        return t;
    }
    friend constexpr bool operator==(Counting a, Counting b) { return a.cur == b.cur; }
    friend constexpr bool operator!=(Counting a, Counting b) { return a.cur != b.cur; }
};

using InputIt = Counting<std::input_iterator_tag>;
using ForwardIt = Counting<std::forward_iterator_tag>;

struct Pod
{
    int a;
    int b;
    friend constexpr bool operator==(Pod const& x, Pod const& y) { return x.a == y.a && x.b == y.b; }
    friend constexpr bool operator<(Pod const& x, Pod const& y) { return x.a != y.a ? x.a < y.a : x.b < y.b; }
};
static_assert(std::is_trivially_default_constructible_v<Pod> && std::is_trivially_destructible_v<Pod>);

// ---- C++20: what needs no element construction

constexpr bool empty_observers()
{
    qx::inplace_vector<4, int> v;
    qx::inplace_vector<4, int> const& c = v;

    CHECK(v.empty() && v.size() == 0 && v.capacity() == 4 && v.max_size() == 4);
    CHECK(v.begin() == v.end() && c.begin() == c.end() && c.cbegin() == c.cend());
    CHECK(v.rbegin() == v.rend() && c.rbegin() == c.rend() && c.crbegin() == c.crend());
    CHECK(v.data() == c.data());
    v.reserve(0);
    v.reserve(4);
    v.shrink_to_fit();
    v.clear();
    CHECK(v.erase(v.begin(), v.end()) == v.end());
    CHECK(erase(v, 1) == 0 && erase_if(v, [](int) { return true; }) == 0);

    auto copy = v;
    auto moved = std::move(copy);
    copy = moved;
    moved = std::move(copy);
    CHECK(moved.empty());
    return true;
}

constexpr bool empty_comparison()
{
    qx::inplace_vector<4, int> const a, b;
    qx::inplace_vector<4, Pod> const p, q;
    CHECK(a == b && !(a != b) && !(a < b) && !(a > b) && a <= b && a >= b);
    CHECK(p == q && !(p < q) && p <= q);
#if QX_WITH_THREE_WAY
    CHECK((a <=> b) == 0 && (p <=> q) == 0);
#endif
    return true;
}

constexpr bool empty_specialization()
{
    qx::inplace_vector<0, int> v;
    qx::inplace_vector<0, int> const& c = v;
    CHECK(v.empty() && v.size() == 0 && v.capacity() == 0 && v.max_size() == 0);
    CHECK(v.begin() == v.end() && c.cbegin() == c.cend() && v.rbegin() == v.rend() && v.data() == nullptr);
    v.clear();
    v.reserve(0);
    CHECK(v.erase(v.begin(), v.end()) == v.end());
    CHECK(erase(v, 1) == 0 && erase_if(v, [](int) { return true; }) == 0);
    auto w = v;
    w = std::move(v);
    CHECK(w == v && !(w < v) && w.empty());
    return true;
}

// the (constexpr) destructor of a vector whose element type has a non-trivial one
#if defined(__cpp_lib_constexpr_string) && __cpp_lib_constexpr_string >= 201907L
constexpr bool non_trivial_destructor()
{
    qx::inplace_vector<4, std::string> v;
    CHECK(v.empty() && v.capacity() == 4);
    v.clear();
    return true;
}
#endif

#if QX_TEST_CONSTEXPR_MUTATORS
// ---- construction

constexpr bool construction()
{
    V d;
    CHECK(d.empty() && d.size() == 0);

    V counted(3);
    CHECK(is(counted, {0, 0, 0}));

    V filled(3, 7);
    CHECK(is(filled, {7, 7, 7}));

    int const src[] = {1, 2, 3};
    V from_pointers(src, src + 3);
    CHECK(is(from_pointers, {1, 2, 3}));

    V from_input(InputIt(4), InputIt(7));
    CHECK(is(from_input, {4, 5, 6}));

    V from_forward(ForwardIt(4), ForwardIt(7));
    CHECK(is(from_forward, {4, 5, 6}));

    V il = {4, 5};
    CHECK(is(il, {4, 5}));

    V copy(il);
    CHECK(is(copy, {4, 5}) && is(il, {4, 5}));

    V moved(std::move(il));
    CHECK(is(moved, {4, 5}));

    V full = {1, 2, 3, 4, 5, 6, 7, 8};
    CHECK(full.size() == full.capacity());

    constexpr qx::inplace_vector<0, int> zero{};
    CHECK(zero.empty() && zero.capacity() == 0);
    return true;
}

// ---- observers and element access

constexpr bool observers()
{
    V v = {1, 2, 3};
    V const& c = v;

    CHECK(v.size() == 3 && v.capacity() == 8 && v.max_size() == 8 && !v.empty());
    CHECK(v[0] == 1 && c[2] == 3 && v.at(1) == 2 && c.at(1) == 2);
    CHECK(v.front() == 1 && v.back() == 3 && c.front() == 1 && c.back() == 3);
    CHECK(v.data() == &v[0] && c.data() == &c[0]);

    v.front() = 10;
    v.back() = 30;
    v.at(1) = 20;
    v[1] += 1;
    v.data()[0] += 1;
    CHECK(is(v, {11, 21, 30}));

    CHECK(v.end() - v.begin() == 3 && c.end() - c.begin() == 3 && c.cend() - c.cbegin() == 3);
    CHECK(*v.rbegin() == 30 && *c.rbegin() == 30 && *c.crbegin() == 30 && v.rend() - v.rbegin() == 3 && c.crend() - c.crbegin() == 3);
    int sum = 0;
    for (int x : v)
        sum += x;
    CHECK(sum == 62);

    v.reserve(8);
    v.shrink_to_fit();
    CHECK(v.size() == 3);
    return true;
}

// ---- appending

constexpr bool appending()
{
    V v;
    int const lvalue = 1;
    CHECK(&v.push_back(lvalue) == &v[0]);
    CHECK(&v.push_back(2) == &v[1]);
    CHECK(v.emplace_back(3) == 3);
    CHECK(is(v, {1, 2, 3}));

    CHECK(v.try_push_back(lvalue) == &v[3]);
    CHECK(v.try_push_back(5) == &v[4]);
    CHECK(*v.try_emplace_back(6) == 6);
    CHECK(v.unchecked_push_back(lvalue) == 1);
    CHECK(v.unchecked_push_back(8) == 8);
    CHECK(is(v, {1, 2, 3, 1, 5, 6, 1, 8}));

    CHECK(v.try_push_back(9) == nullptr && v.try_push_back(lvalue) == nullptr && v.try_emplace_back(9) == nullptr);
    CHECK(v.size() == 8);

    v.pop_back();
    v.pop_back();
    CHECK(is(v, {1, 2, 3, 1, 5, 6}));
    CHECK(v.unchecked_emplace_back(7) == 7);

    qx::inplace_vector<2, Pod> p;
    p.emplace_back(Pod{1, 2});
    p.push_back(Pod{3, 4});
    CHECK(p[0] == (Pod{1, 2}) && p[1] == (Pod{3, 4}) && p.back().b == 4);
    return true;
}

// ---- insertion

constexpr bool insertion()
{
    V v = {1, 2, 3, 4};
    int const x = 9;

    auto it = v.insert(v.begin() + 1, x);
    CHECK(is(v, {1, 9, 2, 3, 4}) && it == v.begin() + 1);
    it = v.insert(v.begin(), 8);
    CHECK(is(v, {8, 1, 9, 2, 3, 4}) && it == v.begin());
    it = v.insert(v.end(), 7);
    CHECK(is(v, {8, 1, 9, 2, 3, 4, 7}) && it == v.end() - 1);

    v = {1, 2, 3, 4};
    v.insert(v.begin() + 1, v[2]); // the source is shifted by the insertion
    CHECK(is(v, {1, 3, 2, 3, 4}));
    v.insert(v.end(), v[0]);
    CHECK(is(v, {1, 3, 2, 3, 4, 1}));

    v = {1, 2, 3, 4};
    v.emplace(v.begin() + 2, 9);
    CHECK(is(v, {1, 2, 9, 3, 4}));
    v.emplace(v.end(), 5);
    CHECK(is(v, {1, 2, 9, 3, 4, 5}));

    v = {1, 2, 3, 4}; // count: fewer than the tail, more than the tail, at the end, none
    v.insert(v.begin() + 1, 2, x);
    CHECK(is(v, {1, 9, 9, 2, 3, 4}));
    v = {1, 2, 3, 4};
    v.insert(v.begin() + 3, 4, x);
    CHECK(is(v, {1, 2, 3, 9, 9, 9, 9, 4}));
    v = {1, 2, 3, 4};
    v.insert(v.end(), 3, x);
    CHECK(is(v, {1, 2, 3, 4, 9, 9, 9}));
    v.insert(v.begin(), 0, x);
    CHECK(is(v, {1, 2, 3, 4, 9, 9, 9}));

    v = {1, 2, 3, 4};
    v.insert(v.begin() + 1, 2, v[2]); // the source is shifted by the insertion
    CHECK(is(v, {1, 3, 3, 2, 3, 4}));
    v = {1, 2, 3, 4};
    v.insert(v.begin() + 2, 4, v[3]);
    CHECK(is(v, {1, 2, 4, 4, 4, 4, 3, 4}));

    int const src[] = {7, 8, 9};
    v = {1, 2, 3, 4};
    it = v.insert(v.begin() + 1, src, src + 3);
    CHECK(is(v, {1, 7, 8, 9, 2, 3, 4}) && it == v.begin() + 1);
    v = {1, 2, 3, 4};
    v.insert(v.begin() + 3, src, src + 3);
    CHECK(is(v, {1, 2, 3, 7, 8, 9, 4}));
    v = {1, 2, 3};
    v.insert(v.end(), src, src + 2);
    CHECK(is(v, {1, 2, 3, 7, 8}));

    v = {1, 2, 3};
    v.insert(v.begin() + 1, ForwardIt(10), ForwardIt(12)); // multi-pass
    CHECK(is(v, {1, 10, 11, 2, 3}));
    v = {1, 2, 3};
    v.insert(v.begin() + 1, InputIt(10), InputIt(13)); // single-pass
    CHECK(is(v, {1, 10, 11, 12, 2, 3}));
    v.insert(v.begin(), InputIt(0), InputIt(0));
    CHECK(is(v, {1, 10, 11, 12, 2, 3}));

    v = {1, 2, 3};
    it = v.insert(v.begin() + 1, {50, 51});
    CHECK(is(v, {1, 50, 51, 2, 3}) && it == v.begin() + 1);

    qx::inplace_vector<4, Pod> p = {Pod{1, 1}, Pod{3, 3}};
    p.insert(p.begin() + 1, Pod{2, 2});
    CHECK(p[0].a == 1 && p[1].a == 2 && p[2].a == 3);
    return true;
}

// ---- removal

constexpr bool removal()
{
    V v = {1, 2, 3, 4, 5, 6};

    auto it = v.erase(v.begin() + 2);
    CHECK(is(v, {1, 2, 4, 5, 6}) && *it == 4);
    it = v.erase(v.begin());
    CHECK(is(v, {2, 4, 5, 6}) && *it == 2);
    it = v.erase(v.end() - 1);
    CHECK(is(v, {2, 4, 5}) && it == v.end());

    v = {1, 2, 3, 4, 5, 6};
    it = v.erase(v.begin() + 1, v.begin() + 3);
    CHECK(is(v, {1, 4, 5, 6}) && *it == 4);
    it = v.erase(v.begin() + 1, v.begin() + 1);
    CHECK(is(v, {1, 4, 5, 6}) && *it == 4);
    it = v.erase(v.begin() + 2, v.end());
    CHECK(is(v, {1, 4}) && it == v.end());

    v.clear();
    CHECK(v.empty() && v.begin() == v.end());
    v.push_back(4);
    CHECK(is(v, {4}));

    v = {1, 2, 1, 3, 1};
    CHECK(erase(v, 1) == 3 && is(v, {2, 3}));
    v = {1, 2, 3, 4, 5, 6};
    CHECK(erase_if(v, [](int e) { return e % 2 == 0; }) == 3 && is(v, {1, 3, 5}));
    CHECK(erase_if(v, [](int) { return false; }) == 0);
    return true;
}

// ---- assignment, resizing, swap

constexpr bool assignment()
{
    V v = {9, 9, 9, 9, 9};
    v = {1, 2};
    CHECK(is(v, {1, 2}));
    v = {1, 2, 3, 4, 5, 6};
    CHECK(is(v, {1, 2, 3, 4, 5, 6}));
    v = {};
    CHECK(v.empty());

    v.assign(5, 7);
    CHECK(is(v, {7, 7, 7, 7, 7}));
    v.assign(2, 4);
    CHECK(is(v, {4, 4}));
    v.assign(0, 1);
    CHECK(v.empty());

    int const src[] = {1, 2, 3, 4};
    v = {9, 9, 9, 9, 9, 9};
    v.assign(src, src + 4); // shrinks
    CHECK(is(v, {1, 2, 3, 4}));
    v = {9};
    v.assign(src, src + 4); // grows
    CHECK(is(v, {1, 2, 3, 4}));
    v.assign(ForwardIt(5), ForwardIt(7));
    CHECK(is(v, {5, 6}));
    v.assign(ForwardIt(5), ForwardIt(10));
    CHECK(is(v, {5, 6, 7, 8, 9}));
    v.assign(InputIt(1), InputIt(3)); // single-pass: shrinks
    CHECK(is(v, {1, 2}));
    v.assign(InputIt(1), InputIt(6)); // single-pass: grows
    CHECK(is(v, {1, 2, 3, 4, 5}));
    v.assign({7, 8});
    CHECK(is(v, {7, 8}));

    V a = {1, 2, 3};
    V b = {4, 5};
    b = a;
    CHECK(is(b, {1, 2, 3}) && is(a, {1, 2, 3}));
    V c = {9, 9, 9, 9};
    c = std::move(a);
    CHECK(is(c, {1, 2, 3}));
    return true;
}

constexpr bool resizing()
{
    V v = {1, 2, 3};
    v.resize(5, 9);
    CHECK(is(v, {1, 2, 3, 9, 9}));
    v.resize(2);
    CHECK(is(v, {1, 2}));
    v.resize(5);
    CHECK(is(v, {1, 2, 0, 0, 0})); // value-initialized, not stale
    v.resize(0);
    v.resize(3, 4);
    CHECK(is(v, {4, 4, 4}));
    int const x = 6;
    v.resize(5, x);
    CHECK(is(v, {4, 4, 4, 6, 6}));
    v.resize(8);
    CHECK(v.size() == 8);
    return true;
}

constexpr bool swapping()
{
    V a = {1, 2, 3};
    V b = {4, 5, 6, 7, 8};

    a.swap(b); // shorter.swap(longer)
    CHECK(is(a, {4, 5, 6, 7, 8}) && is(b, {1, 2, 3}));
    a.swap(b); // longer.swap(shorter)
    CHECK(is(a, {1, 2, 3}) && is(b, {4, 5, 6, 7, 8}));
    swap(a, b);
    CHECK(is(a, {4, 5, 6, 7, 8}) && is(b, {1, 2, 3}));
    a.swap(a);
    CHECK(is(a, {4, 5, 6, 7, 8}));

    V empty;
    a.swap(empty);
    CHECK(a.empty() && is(empty, {4, 5, 6, 7, 8}));

    qx::inplace_vector<0, int> z1, z2;
    z1.swap(z2);
    return true;
}

// ---- comparison

constexpr bool comparison()
{
    V const a = {1, 2, 3};
    V const b = {1, 2, 3};
    V const bigger = {1, 3};
    V const shorter = {1, 2};

    CHECK(a == b && !(a != b) && a != bigger && a != shorter);
    CHECK(a < bigger && bigger > a && shorter < a && a > shorter && a <= b && a >= b && !(a < b) && !(bigger <= a));
    CHECK(V{} == V{} && V{} < a);
#if QX_WITH_THREE_WAY
    CHECK((a <=> b) == 0 && (a <=> bigger) < 0 && (bigger <=> a) > 0 && (a <=> shorter) > 0);
#endif

    qx::inplace_vector<3, Pod> p = {Pod{1, 2}};
    qx::inplace_vector<3, Pod> q = {Pod{1, 3}};
    CHECK(p < q && p != q && p == p);
    return true;
}

#if QX_WITH_CXX23_RANGES

constexpr bool ranges()
{
    std::array<int, 3> const a = {1, 2, 3};

    V from(std::from_range, a);
    CHECK(is(from, {1, 2, 3}));
    V from_view(std::from_range, std::views::iota(5, 8));
    CHECK(is(from_view, {5, 6, 7}));
    V from_single_pass(std::from_range, std::ranges::subrange(InputIt(1), InputIt(4)));
    CHECK(is(from_single_pass, {1, 2, 3}));

    V v;
    v.assign_range(a);
    CHECK(is(v, {1, 2, 3}));
    v.assign_range(std::ranges::subrange(InputIt(7), InputIt(9)));
    CHECK(is(v, {7, 8}));

    v.append_range(a);
    CHECK(is(v, {7, 8, 1, 2, 3}));
    v.append_range(std::ranges::subrange(InputIt(0), InputIt(2)));
    CHECK(is(v, {7, 8, 1, 2, 3, 0, 1}));

    V w = {1, 2};
    auto pos = w.insert_range(w.begin() + 1, a);
    CHECK(is(w, {1, 1, 2, 3, 2}) && pos == w.begin() + 1);
    w.insert_range(w.end(), std::ranges::subrange(InputIt(8), InputIt(10)));
    CHECK(is(w, {1, 1, 2, 3, 2, 8, 9}));
    return true;
}

#endif

// ---- an element type with several members, and the empty vector

constexpr bool pod_elements()
{
    qx::inplace_vector<4, Pod> v;
    v.push_back(Pod{1, 2});
    v.emplace_back(Pod{3, 4});
    v.insert(v.begin(), Pod{0, 0});
    auto w = v;
    w.erase(w.begin());
    v.swap(w);
    return v.size() == 2 && v[0] == (Pod{1, 2}) && w.size() == 3 && w[0] == (Pod{0, 0});
}

constexpr bool empty_specialization_mutators()
{
    qx::inplace_vector<0, int> v;
    CHECK(v.empty() && v.size() == 0 && v.begin() == v.end() && v.rbegin() == v.rend());
    CHECK(v.try_push_back(1) == nullptr && v.try_emplace_back(1) == nullptr);
    v.clear();
    v.resize(0);
    v.assign(0, 1);
    v.insert(v.begin(), 0, 1);
    CHECK(v.erase(v.begin(), v.end()) == v.end());
    auto w = v;
    w = std::move(v);
    CHECK(w.empty() && w == v && !(w < v));
    return true;
}

#endif

#undef CHECK

#define CONSTEXPR_AND_RUNTIME(fn, TestName)                                                                                                \
    static_assert(fn(), #fn " is not a constant expression");                                                                              \
    TEST(InplaceVectorConstexpr, TestName)                                                                                                 \
    {                                                                                                                                      \
        EXPECT_TRUE(fn());                                                                                                                 \
    }

} // namespace

CONSTEXPR_AND_RUNTIME(empty_observers, EmptyObservers)
CONSTEXPR_AND_RUNTIME(empty_comparison, EmptyComparison)
CONSTEXPR_AND_RUNTIME(empty_specialization, EmptySpecialization)
#if defined(__cpp_lib_constexpr_string) && __cpp_lib_constexpr_string >= 201907L
CONSTEXPR_AND_RUNTIME(non_trivial_destructor, NonTrivialDestructor)
#endif

#if QX_TEST_CONSTEXPR_MUTATORS
CONSTEXPR_AND_RUNTIME(construction, Construction)
CONSTEXPR_AND_RUNTIME(observers, Observers)
CONSTEXPR_AND_RUNTIME(appending, Appending)
CONSTEXPR_AND_RUNTIME(insertion, Insertion)
CONSTEXPR_AND_RUNTIME(removal, Removal)
CONSTEXPR_AND_RUNTIME(assignment, Assignment)
CONSTEXPR_AND_RUNTIME(resizing, Resizing)
CONSTEXPR_AND_RUNTIME(swapping, Swapping)
CONSTEXPR_AND_RUNTIME(comparison, Comparison)
CONSTEXPR_AND_RUNTIME(pod_elements, PodElements)
CONSTEXPR_AND_RUNTIME(empty_specialization_mutators, EmptySpecializationMutators)
#if QX_WITH_CXX23_RANGES
CONSTEXPR_AND_RUNTIME(ranges, Ranges)
#endif
#endif

namespace
{

// constexpr variables
constexpr qx::inplace_vector<4, int> constexpr_empty;
static_assert(constexpr_empty.empty() && constexpr_empty.capacity() == 4);
constexpr auto constexpr_copy = constexpr_empty;
static_assert(constexpr_copy == constexpr_empty && constexpr_copy.begin() == constexpr_copy.end());

constexpr qx::inplace_vector<4, Pod> pod_empty;
static_assert(pod_empty.empty() && pod_empty.max_size() == 4);

// the union storage (T not trivially default constructible) is usable as far as no element is constructed
constexpr qx::inplace_vector<4, NoDefault> union_empty;
static_assert(union_empty.empty() && union_empty.begin() == union_empty.end() && union_empty.capacity() == 4);
constexpr qx::inplace_vector<4, Noisy> noisy_empty;
static_assert(noisy_empty.empty());
constexpr qx::inplace_vector<4, qx::inplace_vector<2, int>> nested_empty;
static_assert(nested_empty.size() == 0);
static_assert(qx::inplace_vector<0, Tracked>{}.capacity() == 0);

// Violations of a precondition are not constant expressions: they never silently "work" at compile time.
template <auto F, class = std::enable_if_t<(F(), true)>>
constexpr bool is_constant_expression(int)
{
    return true;
}
template <auto F>
constexpr bool is_constant_expression(...)
{
    return false;
}

using Small = qx::inplace_vector<2, int>;

static_assert(is_constant_expression<[] { Small v; return v.size() + v.capacity(); }>(0));
static_assert(is_constant_expression<[] { Small v; v.clear(); return v.empty(); }>(0));

static_assert(!is_constant_expression<[] { Small v; v.pop_back(); return 0; }>(0));                  // contract
static_assert(!is_constant_expression<[] { Small v; return v[0]; }>(0));                             // contract
static_assert(!is_constant_expression<[] { Small const v; return v[0]; }>(0));                       // contract
static_assert(!is_constant_expression<[] { Small v; return v.front(); }>(0));                        // contract
static_assert(!is_constant_expression<[] { Small v; return v.back(); }>(0));                         // contract
static_assert(!is_constant_expression<[] { Small v; v.erase(v.end()); return 0; }>(0));              // contract
static_assert(!is_constant_expression<[] { Small v; v.erase(v.begin() + 1, v.begin()); return 0; }>(0));
static_assert(!is_constant_expression<[] { Small v; return v.at(0); }>(0));                           // throws out_of_range
static_assert(!is_constant_expression<[] { Small::reserve(3); return 0; }>(0));                       // throws bad_alloc
static_assert(!is_constant_expression<[] { Small v(3); return 0; }>(0));                              // throws bad_alloc

#if !QX_TEST_CONSTEXPR_MUTATORS
// Without a library that constructs elements in constant expressions, the mutators that need to are not constant
// expressions (they are callable at run time, as the other tests check), and fail to compile rather than misbehave.
static_assert(!is_constant_expression<[] { Small v; v.push_back(1); return 0; }>(0));
static_assert(!is_constant_expression<[] { Small v; v.emplace_back(1); return 0; }>(0));
static_assert(!is_constant_expression<[] { Small v{1}; return 0; }>(0));
static_assert(!is_constant_expression<[] { Small v; v.resize(1); return 0; }>(0));
static_assert(!is_constant_expression<[] { Small v; v.insert(v.begin(), 1); return 0; }>(0));
#endif

} // namespace

#endif
