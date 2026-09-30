#include <gmock/gmock.h>

#include <qx/inplace_vector.h>

#include <algorithm>
#include <functional>
#include <list>
#include <string>
#include <vector>

// With a throwing T every operation must keep the vector valid: size() equals the number of live elements, nothing
// leaks and nothing is destroyed twice. Each operation is run once per possible throw point: a countdown makes the
// k-th special-member call of Throwy throw.

namespace
{

constexpr std::size_t capacity = 8;

struct Boom
{};

struct Fuse
{
    static inline int budget = -1; // < 0: disarmed
    static inline long live = 0;
    static void tick()
    {
        if (budget >= 0 && budget-- == 0)
            throw Boom{};
    }
};

struct Throwy
{
    int id;
    std::string heap;

    Throwy()
        : id(0)
        , heap(40, 'd')
    {
        Fuse::tick();
        ++Fuse::live;
    }
    explicit Throwy(int i) // never throws: used to build fixtures and arguments
        : id(i)
        , heap(40, 'x')
    {
        ++Fuse::live;
    }
    Throwy(Throwy const& o)
        : id(o.id)
        , heap(o.heap)
    {
        Fuse::tick();
        ++Fuse::live;
    }
    Throwy(Throwy&& o)
        : id(o.id)
        , heap(std::move(o.heap))
    {
        Fuse::tick();
        ++Fuse::live;
    }
    Throwy& operator=(Throwy const& o)
    {
        Fuse::tick();
        id = o.id;
        heap = o.heap;
        return *this;
    }
    Throwy& operator=(Throwy&& o)
    {
        Fuse::tick();
        id = o.id;
        heap = std::move(o.heap);
        return *this;
    }
    ~Throwy() { --Fuse::live; }
};

using V = qx::inplace_vector<capacity, Throwy>;

V make(int n, int first)
{
    Fuse::budget = -1;
    V v;
    for (int i = 0; i < n; ++i)
        v.emplace_back(first + i);
    return v;
}

std::vector<Throwy> make_std(int n)
{
    std::vector<Throwy> v;
    for (int i = 0; i < n; ++i)
        v.emplace_back(i);
    return v;
}

V::iterator at(V& v, std::size_t i)
{
    return v.begin() + std::min(i, v.size());
}

struct Op
{
    using BinaryOp = void (*)(V&, V&);
    char const* name;
    BinaryOp run;
};

std::vector<Op> const& operations()
{
    // clang-format off
    static std::vector<Op> const all = {
        {"push_back", [](V& a, V&) { Throwy x(1); a.push_back(x); }},
        {"push_back&&", [](V& a, V&) { a.push_back(Throwy(1)); }},
        {"emplace_back", [](V& a, V&) { a.emplace_back(1); }},
        {"try_push_back", [](V& a, V&) { Throwy x(1); a.try_push_back(x); }},
        {"try_push_back&&", [](V& a, V&) { a.try_push_back(Throwy(1)); }},
        {"try_emplace_back", [](V& a, V&) { a.try_emplace_back(1); }},
        {"unchecked_push_back", [](V& a, V&) { Throwy x(1); if (a.size() < capacity) a.unchecked_push_back(x); }},
        {"unchecked_push_back&&", [](V& a, V&) { Throwy x(1); if (a.size() < capacity) a.unchecked_push_back(std::move(x)); }},
        {"unchecked_emplace_back", [](V& a, V&) { Throwy x(1); if (a.size() < capacity) a.unchecked_emplace_back(x); }},
        {"insert", [](V& a, V&) { Throwy x(1); a.insert(at(a, 1), x); }},
        {"insert&&", [](V& a, V&) { a.insert(at(a, 1), Throwy(1)); }},
        {"insert count", [](V& a, V&) { Throwy x(1); a.insert(at(a, 1), 3, x); }},
        {"insert count (past the tail)", [](V& a, V&) { Throwy x(1); a.insert(at(a, 1), 6, x); }},
        {"insert range", [](V& a, V&) { auto s = make_std(3); a.insert(at(a, 1), s.begin(), s.end()); }},
        {"emplace", [](V& a, V&) { a.emplace(at(a, 1), 5); }},
        {"erase", [](V& a, V&) { if (a.size() > 1) a.erase(a.begin() + 1); }},
        {"erase range", [](V& a, V&) { a.erase(at(a, 1), at(a, 3)); }},
        {"assign count", [](V& a, V&) { Throwy x(1); a.assign(6, x); }},
        {"assign range", [](V& a, V&) { auto s = make_std(6); a.assign(s.begin(), s.end()); }},
        {"resize", [](V& a, V&) { a.resize(7); }},
        {"resize value", [](V& a, V&) { Throwy x(1); a.resize(7, x); }},
        {"copy constructor", [](V& a, V&) { V c(a); }},
        {"move constructor", [](V& a, V&) { V c(std::move(a)); }},
        {"copy assignment", [](V& a, V& b) { a = b; }},
        {"move assignment", [](V& a, V& b) { a = std::move(b); }},
        {"swap", [](V& a, V& b) { a.swap(b); }},
        {"count constructor", [](V&, V&) { V c(5); }},
        {"count value constructor", [](V&, V&) { Throwy x(1); V c(5, x); }},
        {"range constructor", [](V&, V&) { auto s = make_std(5); V c(s.begin(), s.end()); }},
        {"initializer list constructor", [](V&, V&) { V c{Throwy(1), Throwy(2), Throwy(3)}; }},
    };
    // clang-format on
    return all;
}

} // namespace

TEST(InplaceVectorExceptionSafety, EveryOperationKeepsTheVectorValidAtEveryThrowPoint)
{
    ASSERT_EQ(Fuse::live, 0);
    for (Op const& op : operations())
        for (int na : {0, 2, 5, 8})
            for (int nb : {0, 3, 8})
                for (int k = 0; k < 40; ++k)
                {
                    {
                        V a = make(na, 0);
                        V b = make(nb, 10);
                        Fuse::budget = k;
                        try
                        {
                            op.run(a, b);
                        }
                        catch (Boom const&)
                        {}
                        catch (std::length_error const&)
                        {}
                        Fuse::budget = -1;

                        EXPECT_EQ(Fuse::live, static_cast<long>(a.size() + b.size()))
                            << op.name << " na=" << na << " nb=" << nb << " k=" << k;
                        EXPECT_LE(a.size(), capacity);
                        EXPECT_LE(b.size(), capacity);
                    }
                    ASSERT_EQ(Fuse::live, 0) << "leaked or destroyed twice: " << op.name << " na=" << na << " nb=" << nb << " k=" << k;
                }
}

TEST(InplaceVectorExceptionSafety, PushBackIsStrong)
{
    for (int n : {0, 3, 7})
        for (int k = 0; k < 4; ++k)
        {
            V v = make(n, 0);
            Fuse::budget = k;
            try
            {
                Throwy x(1);
                v.push_back(x);
            }
            catch (Boom const&)
            {
                Fuse::budget = -1;
                ASSERT_EQ(v.size(), static_cast<std::size_t>(n));
                for (int i = 0; i < n; ++i)
                    EXPECT_EQ(v[static_cast<std::size_t>(i)].id, i);
            }
            Fuse::budget = -1;
        }
}

TEST(InplaceVectorExceptionSafety, CapacityOverflowIsStrongForSizedInputs)
{
    V v = make(5, 0);
    Throwy x(1);
    auto big = make_std(5); // 5 new elements > 3 free slots
    std::list<Throwy> big_list(big.begin(), big.end());

    auto unchanged = [&]
    {
        ASSERT_EQ(v.size(), 5u);
        for (int i = 0; i < 5; ++i)
            EXPECT_EQ(v[static_cast<std::size_t>(i)].id, i);
    };
    EXPECT_THROW(v.insert(v.begin() + 2, 4, x), std::length_error);
    unchanged();
    EXPECT_THROW(v.insert(v.begin() + 2, big.begin(), big.end()), std::length_error);
    unchanged();
    EXPECT_THROW(v.insert(v.begin() + 2, big_list.begin(), big_list.end()), std::length_error);
    unchanged();
    EXPECT_THROW(v.assign(capacity + 1, x), std::length_error);
    unchanged();
    EXPECT_THROW(v.resize(capacity + 1), std::length_error);
    unchanged();
}