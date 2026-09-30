#pragma once

#include <gmock/gmock.h>

#include <ostream>
#include <string>

// Non-trivially-copyable element that compares equal to plain ints, so `EXPECT_THAT(v, ElementsAre(1, 2, 3))` reads the
// same for inplace_vector<N, int> (trivial storage) and inplace_vector<N, Tracked> (non-trivial storage).
// The payload is longer than any small-string buffer, so a leaked or double-destroyed element is visible to ASan, and
// `live` catches unbalanced construction/destruction (checked after every test by the listener below).
struct Tracked
{
    static constexpr int moved_from = -1;
    static inline long live = 0;

    int id;
    std::string heap;

    Tracked(int i = 0)
        : id(i)
        , heap(40, 'x')
    {
        ++live;
    }
    Tracked(Tracked const& o)
        : id(o.id)
        , heap(o.heap)
    {
        ++live;
    }
    Tracked(Tracked&& o) noexcept
        : id(o.id)
        , heap(std::move(o.heap))
    {
        o.id = moved_from;
        ++live;
    }
    Tracked& operator=(Tracked const& o)
    {
        id = o.id;
        heap = o.heap;
        return *this;
    }
    Tracked& operator=(Tracked&& o) noexcept
    {
        if (this != &o)
        {
            id = o.id;
            heap = std::move(o.heap);
            o.id = moved_from;
        }
        return *this;
    }
    ~Tracked() { --live; }

    friend bool operator==(Tracked const& a, Tracked const& b) { return a.id == b.id; }
    friend bool operator==(Tracked const& a, int b) { return a.id == b; }
    friend bool operator<(Tracked const& a, Tracked const& b) { return a.id < b.id; }
    friend std::ostream& operator<<(std::ostream& os, Tracked const& t) { return os << t.id; }
};

// Trivially copyable, not default constructible.
struct NoDefault
{
    int x;
    explicit NoDefault(int v)
        : x(v)
    {}
};

// Trivially copyable, but every default construction is observable.
struct Noisy
{
    static inline int constructed = 0;
    int v = 7;
    Noisy() { ++constructed; }
    explicit Noisy(int x)
        : v(x)
    {
        ++constructed;
    }
};

namespace intl_test
{

struct LiveCheck : ::testing::EmptyTestEventListener
{
    void OnTestEnd(::testing::TestInfo const&) override
    {
        if (Tracked::live != 0)
            ADD_FAILURE() << Tracked::live << " Tracked element(s) leaked or destroyed twice";
        Tracked::live = 0;
    }
};

inline bool const live_check_registered = (::testing::UnitTest::GetInstance()->listeners().Append(new LiveCheck), true);

} // namespace intl_test
