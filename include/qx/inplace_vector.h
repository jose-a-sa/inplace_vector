/*
 * Copyright (c) 2026 Jose Sa
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <initializer_list>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <utility>

// contract hardening level (0: none, 1: all, default: none)
#ifndef QX_HARDENING_MODE_NONE
#define QX_HARDENING_MODE_NONE 0
#define QX_HARDENING_MODE_ALL 1
#endif

#ifndef QX_HARDENING_MODE
#define QX_HARDENING_MODE QX_HARDENING_MODE_NONE
#endif

// contract assert behavior (default: IO-logging and trap)
#ifndef QX_ASSERT_MODE_NONE
#define QX_ASSERT_MODE_NONE 0
#define QX_ASSERT_MODE_TRAP 1
#define QX_ASSERT_MODE_LOG_TRAP 2
#define QX_ASSERT_MODE_ABORT 3
#define QX_ASSERT_MODE_LOG_ABORT 4
#endif

#ifndef QX_ASSERT_MODE
#define QX_ASSERT_MODE QX_ASSERT_MODE_LOG_TRAP // default assertion mode
#else
#if (QX_ASSERT_MODE > QX_ASSERT_MODE_LOG_ABORT)
#undef QX_ASSERT_MODE
#define QX_ASSERT_MODE QX_ASSERT_MODE_NONE
#endif
#endif

#if !defined(QX_STL_LIBCPP) && !defined(QX_STL_LIBSTDCXX) && !defined(QX_STL_MSVC)
#ifdef _LIBCPP_VERSION
#define QX_STL_LIBCPP
#elif defined(__GLIBCXX__)
#define QX_STL_LIBSTDCXX
#elif defined(_MSVC_STL_VERSION) || defined(_CPPLIB_VER)
#define QX_STL_MSVC
#else
#if __cplusplus < 202002L
#error "qx::inplace_vector not supported: unknown STL in CXX17"
#endif
#endif
#endif

#ifndef QX_STRINGIFY
#define QX_STRINGIFY_IMPL(x) #x
#define QX_STRINGIFY(x) QX_STRINGIFY_IMPL(x)
#endif

// set __has_builtin when not defined
#ifndef __has_builtin
#define __has_builtin(x) 0
#endif

#ifndef __has_include
#define __has_include(x) 0
#endif

// __builtin_constant_p is a GCC + Clang builtin
#ifndef QX_IS_CONSTANT
#if __has_builtin(__builtin_constant_p) || defined(__GNUC__)
#define QX_IS_CONSTANT(x) __builtin_constant_p(x)
#else
#define QX_IS_CONSTANT(x) false
#endif
#endif

// __builtin_expect is a GCC + Clang builtin
#ifndef QX_LIKELY
#if __has_builtin(__builtin_expect) || defined(__GNUC__)
#define QX_LIKELY(x) __builtin_expect(!!(x), 1)
#define QX_UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
#define QX_LIKELY(x) (x)
#define QX_UNLIKELY(x) (x)
#endif
#endif

#ifndef QX_CONSTEXPR_CXX20
#if __cplusplus >= 202002L
#define QX_CONSTEXPR_CXX20 constexpr
#else
#define QX_CONSTEXPR_CXX20
#endif
#endif

#ifndef QX_CONSTEXPR_CXX23
#if __cplusplus >= 202302L
#define QX_CONSTEXPR_CXX23 constexpr
#else
#define QX_CONSTEXPR_CXX23
#endif
#endif

#ifndef QX_CONSTEXPR_CXX26
#if __cplusplus >= 202400L
#define QX_CONSTEXPR_CXX26 constexpr
#else
#define QX_CONSTEXPR_CXX26
#endif
#endif

#ifndef QX_FORCE_INLINE
#if defined(__GNUC__) || defined(__clang__)
#define QX_FORCE_INLINE inline __attribute__((always_inline))
#elif defined(_MSC_VER)
#define QX_FORCE_INLINE __forceinline
#else
#define QX_FORCE_INLINE inline
#endif
#endif

#ifndef QX_COLD_NOINLINE
#if defined(__GNUC__) || defined(__clang__)
#define QX_COLD_NOINLINE __attribute__((cold, noinline))
#elif defined(_MSC_VER)
#define QX_COLD_NOINLINE __declspec(noinline)
#else
#define QX_COLD_NOINLINE
#endif
#endif

// __builtin_trap is a GCC + Clang builtin
#ifndef QX_TRAP
#if __has_builtin(__builtin_trap) || defined(__GNUC__)
#define QX_TRAP() __builtin_trap()
#elif defined(_MSC_VER)
// __debugbreak() alone doesn't mark the site as noreturn to the
// optimizer; __assume(false) tells MSVC control never continues.
#define QX_TRAP() (__debugbreak(), __assume(false))
#else
#define QX_TRAP() std::abort()
#endif
#endif

// __builtin_verbose_trap is Clang-only.
// AppleClang < 17 shipped a 1-arg version; upstream Clang 18+ is 2-arg.
#ifndef QX_TRAP_WITH_MSG
#if defined(__clang__) && __has_builtin(__builtin_verbose_trap)
#if defined(__apple_build_version__) && __apple_build_version__ < 17000000
#define QX_TRAP_WITH_MSG(tag, msg) __builtin_verbose_trap(tag ": " msg)
#else
#define QX_TRAP_WITH_MSG(tag, msg) __builtin_verbose_trap(tag, msg)
#endif
#else
#define QX_TRAP_WITH_MSG(tag, msg) QX_TRAP()
#endif
#endif

#if __cplusplus >= 202002L && __has_include(<compare>)
#include <compare>
#endif

#ifndef QX_WITH_THREE_WAY
#if defined(__cpp_lib_three_way_comparison) && __cpp_lib_three_way_comparison >= 201907L && defined(__cpp_lib_concepts)
#define QX_WITH_THREE_WAY 1
#else
#define QX_WITH_THREE_WAY 0
#endif
#endif

#ifndef QX_WITH_CXX23_RANGES
#if defined(__cpp_lib_ranges) && __cpp_lib_ranges >= 202207L && defined(__cpp_lib_ranges_to_container) &&                                  \
    __cpp_lib_ranges_to_container >= 202202L
#define QX_WITH_CXX23_RANGES 1
#include <ranges>
#else
#define QX_WITH_CXX23_RANGES 0
#endif
#endif

namespace qx
{

namespace intl
{

// std::type_identity (C++20)

#if __cplusplus >= 202002L
using std::type_identity;
using std::type_identity_t;
#else
template <class T>
struct type_identity
{
    using type = T;
};
template <class T>
using type_identity_t = typename type_identity<T>::type;
#endif // __cplusplus >= 202002L

// std::remove_cvref (C++20)

#if __cplusplus >= 202002L
using std::remove_cvref;
using std::remove_cvref_t;
#else
template <class T>
struct remove_cvref : std::remove_cv<std::remove_reference_t<T>>
{};
template <class T>
using remove_cvref_t = std::remove_cv_t<std::remove_reference_t<T>>;
#endif // __cplusplus >= 202002L

// std::is_constant_evaluated() (C++20)

constexpr bool is_constant_evaluated() noexcept
{
#if defined(__cpp_lib_is_constant_evaluated) && __cpp_lib_is_constant_evaluated >= 201811L
    return std::is_constant_evaluated();
#elif __has_builtin(__builtin_is_constant_evaluated)
    return __builtin_is_constant_evaluated();
#else
    return false; // Fallback
#endif
}

inline QX_COLD_NOINLINE void v_contract_fail_handler(char const* msg)
{
    std::fputs(msg, stderr);
    std::fputc('\n', stderr);
    std::fflush(stderr);
}

#ifndef QX_CONTRACT_FAIL_HANDLER
#if QX_ASSERT_MODE == QX_ASSERT_MODE_NONE
#define QX_CONTRACT_FAIL_HANDLER(tag, loc, msg) ((void)0)
#elif QX_ASSERT_MODE == QX_ASSERT_MODE_TRAP
#define QX_CONTRACT_FAIL_HANDLER(tag, loc, msg) (::qx::intl::is_constant_evaluated() ? QX_TRAP() : QX_TRAP_WITH_MSG(tag, msg))
#elif QX_ASSERT_MODE == QX_ASSERT_MODE_LOG_TRAP
#define QX_CONTRACT_FAIL_HANDLER(tag, loc, msg)                                                                                            \
    (::qx::intl::is_constant_evaluated() ? QX_TRAP() : (::qx::intl::v_contract_fail_handler(loc ": " msg), QX_TRAP_WITH_MSG(tag, msg)))
#elif QX_ASSERT_MODE == QX_ASSERT_MODE_ABORT
#define QX_CONTRACT_FAIL_HANDLER(tag, loc, msg) (::qx::intl::is_constant_evaluated() ? QX_TRAP() : std::abort())
#elif QX_ASSERT_MODE == QX_ASSERT_MODE_LOG_ABORT
#define QX_CONTRACT_FAIL_HANDLER(tag, loc, msg)                                                                                            \
    (::qx::intl::is_constant_evaluated() ? QX_TRAP() : (::qx::intl::v_contract_fail_handler(loc ": " msg), std::abort()))
#else
#define QX_CONTRACT_FAIL_HANDLER(tag, loc, msg) ((void)0)
#endif
#endif

#ifndef QX_ASSERT_CONTRACT
#if (QX_HARDENING_MODE > QX_HARDENING_MODE_NONE) && (QX_ASSERT_MODE > QX_ASSERT_MODE_NONE)
#define QX_ASSERT_CONTRACT(cond, msg)                                                                                                      \
    (QX_LIKELY(cond) ? ((void)0)                                                                                                           \
                     : QX_CONTRACT_FAIL_HANDLER("qxlib", __FILE__ ":" QX_STRINGIFY(__LINE__), "contract violation '" #cond "': " msg))
#else
#define QX_ASSERT_CONTRACT(cond, msg) ((void)0)
#endif
#endif

// iterator_value

template <class Iter>
using iter_value_t = typename std::iterator_traits<Iter>::value_type;

// iterator_category (C++17 and earlier)

template <class Iter>
using iter_category_t = typename std::iterator_traits<Iter>::iterator_category;

template <class Iter, class Cat, class = void>
struct is_iter_with_category : std::false_type
{};
template <class Iter, class Cat>
struct is_iter_with_category<Iter, Cat, std::void_t<iter_category_t<Iter>>> : std::is_convertible<iter_category_t<Iter>, Cat>
{};
template <class Iter, class Cat>
inline constexpr bool is_iter_with_category_v = is_iter_with_category<Iter, Cat>::value;

// is_valid_iter_with_category: Iter has (at least) category Tag and T is constructible from its reference type.
// Lazy on purpose: iterator_traits<Iter>::reference is only named for real iterators, so `vec(3, 4)` picks (count, value).

template <class T, class Iter, class Tag, class = void>
struct is_constructible_with_iter : std::false_type
{};
template <class T, class Iter, class Tag>
struct is_constructible_with_iter<T, Iter, Tag, std::enable_if_t<is_iter_with_category_v<Iter, Tag>>>
    : std::is_constructible<T, typename std::iterator_traits<Iter>::reference>
{};

// is_contiguous_iterator

#if __cplusplus >= 202002L

template <class T>
struct is_contiguous_iterator : std::bool_constant<std::contiguous_iterator<T>>
{};

#else // C++17 implementation

template <class T, class = void>
struct is_contiguous_iterator : std::false_type
{};
template <class T>
struct is_contiguous_iterator<T*, void> : std::is_object<T>
{};

#if defined(QX_STL_LIBCPP) // libc++ (LLVM)

#if __has_include(<__iterator/wrap_iter.h>)
#include <__iterator/wrap_iter.h>
template <class Iter>
struct is_contiguous_iterator<std::__wrap_iter<Iter>, void> : is_contiguous_iterator<Iter>
{};
#endif // __has_include(<__iterator/wrap_iter.h>)

#if __has_include(<__iterator/bounded_iter.h>)
#include <__iterator/bounded_iter.h>
template <class Iter>
struct is_contiguous_iterator<std::__bounded_iter<Iter>, void> : is_contiguous_iterator<Iter>
{};
#endif // __has_include(<__iterator/bounded_iter.h>)

#elif defined(QX_STL_LIBSTDCXX) // libstdc++ (GNU)

template <class T>
struct is_gnu_wrapped_iterator : std::false_type
{};
template <class T>
inline constexpr bool is_gnu_wrapped_iterator_v = is_gnu_wrapped_iterator<T>::value;

#if __has_include(<bits/stl_iterator.h>)
#include <bits/stl_iterator.h>
template <class Iter, class Cont>
struct is_gnu_wrapped_iterator<::__gnu_cxx::__normal_iterator<Iter, Cont>> : std::true_type
{};
template <class Iter, class Cont>
struct is_contiguous_iterator<::__gnu_cxx::__normal_iterator<Iter, Cont>, void> : is_contiguous_iterator<Iter>
{};
template <class Iter, class Cont, class Tag>
struct is_gnu_wrapped_iterator<::__gnu_debug::_Safe_iterator<Iter, Cont, Tag>> : std::true_type
{};
template <class Iter, class Cont, class Tag>
struct is_contiguous_iterator<::__gnu_debug::_Safe_iterator<Iter, Cont, Tag>, void> : is_contiguous_iterator<Iter>
{};
#endif // __has_include(<bits/stl_iterator.h>)

#elif defined(QX_STL_MSVC) // MSVC STL

template <class T, class = void>
struct is_msvc_wrapped_iterator : std::false_type
{};
template <class T>
inline constexpr bool is_msvc_wrapped_iterator_v = is_msvc_wrapped_iterator<T>::value;

#if __has_include(<xutility>)
#include <xutility>
template <class T>
struct is_msvc_wrapped_iterator<T, std::void_t<decltype(std::_Get_unwrapped(std::declval<T&>()))>> : std::true_type
{};
template <class T>
struct is_contiguous_iterator<T, std::enable_if_t<!std::is_pointer_v<T> && is_msvc_wrapped_iterator_v<T>>>
    : std::is_pointer<decltype(std::_Get_unwrapped(std::declval<T&>()))>
{};
#endif // __has_include(<xutility>)

#endif
#endif // __cplusplus >= 202002L

template <class T>
inline constexpr bool is_contiguous_iterator_v = is_contiguous_iterator<T>::value;

// has_pointer_traits_to_address

template <class Ptr, class = void>
struct has_pointer_traits_to_address : std::false_type
{};
template <class Ptr>
struct has_pointer_traits_to_address<Ptr, std::void_t<decltype(std::pointer_traits<Ptr>::to_address(std::declval<Ptr&>()))>>
    : std::true_type
{};
template <class Ptr>
inline constexpr bool has_pointer_traits_to_address_v = has_pointer_traits_to_address<Ptr>::value;

// has_arrow_operator

template <class Ptr, class = void>
struct has_arrow_operator : std::false_type
{};
template <class Ptr>
struct has_arrow_operator<Ptr, std::void_t<decltype(std::declval<Ptr&>().operator->())>> : std::true_type
{};
template <class Ptr>
inline constexpr bool has_arrow_operator_v = has_arrow_operator<Ptr>::value;

// std::to_address (C++20)

#if __cplusplus >= 202002L
using std::to_address;
#else // __cplusplus < 202002L

template <class T>
constexpr T* to_address(T* p) noexcept
{
    static_assert(!std::is_function_v<T>, "T must not be a function type");
    return p;
}

#ifdef QX_STL_LIBSTDCXX
template <class T>
inline constexpr auto is_fancy_pointer_v =
    std::is_class_v<T> && (is_gnu_wrapped_iterator_v<T> || has_pointer_traits_to_address_v<T> || has_arrow_operator_v<T>);
#elif defined(QX_STL_MSVC)
template <class T>
inline constexpr auto is_fancy_pointer_v =
    std::is_class_v<T> && (is_msvc_wrapped_iterator_v<T> || has_arrow_operator_v<T> || has_pointer_traits_to_address_v<T>);
#else
template <class T>
inline constexpr auto is_fancy_pointer_v = std::is_class_v<T> && (has_arrow_operator_v<T> || has_pointer_traits_to_address_v<T>);
#endif

template <class Ptr, std::enable_if_t<is_fancy_pointer_v<std::remove_reference_t<Ptr>>, int> = 0>
constexpr auto to_address(Ptr&& ptr) noexcept -> decltype(auto)
{
    using pointer = std::remove_reference_t<Ptr>;
    if constexpr (has_pointer_traits_to_address_v<pointer>) // handlers the LLVM  unwrapping
        return std::pointer_traits<pointer>::to_address(std::forward<Ptr>(ptr));
#ifdef QX_STL_LIBSTDCXX
    if constexpr (is_gnu_wrapped_iterator_v<pointer>)
        return to_address(std::forward<Ptr>(ptr).base());
#elif defined(QX_STL_MSVC)
    if constexpr (is_msvc_wrapped_iterator_v<pointer>)
        return to_address(std::_Get_unwrapped(std::forward<Ptr>(ptr)));
#endif
    return to_address(std::forward<Ptr>(ptr).operator->());
}

#endif // __cplusplus >= 202002L

template <class T, class U, class = void>
struct is_less_than_comparable : std::false_type
{};
template <class T, class U>
struct is_less_than_comparable<T, U, std::void_t<decltype(std::declval<T>() < std::declval<U>())>> : std::true_type
{};
template <class T, class U>
inline constexpr bool is_less_than_comparable_v = is_less_than_comparable<T, U>::value;

#if QX_WITH_THREE_WAY
// synth-three-way: <=> if T has it, otherwise a weak ordering from <
struct synth_three_way
{
    template <class U>
        requires requires(U const& l, U const& r) {
            { l < r } -> std::convertible_to<bool>;
            { r < l } -> std::convertible_to<bool>;
        }
    constexpr auto operator()(U const& l, U const& r) const
    {
        if constexpr (std::three_way_comparable<U>)
        {
            return l <=> r;
        }
        else
        {
            if (l < r)
                return std::weak_ordering::less;
            if (r < l)
                return std::weak_ordering::greater;
            return std::weak_ordering::equivalent;
        }
    }
};
#endif

// is_pointer_in_range: checks if a pointer of type U* points to an address in the range [begin, end) of type T*, even
// if T and U are different types (e.g. char and unsigned char). This is used to check for overlapping ranges when
// appending or inserting from iterators that may point into the string's own buffer.
template <class T, class U>
constexpr bool is_pointer_in_range(T const* begin, T const* end, U const* ptr)
{
    if (is_constant_evaluated())
    {
        if (QX_IS_CONSTANT(begin <= ptr && ptr < end))
            return begin <= ptr && ptr < end;

        return false;
    }

    if constexpr (is_less_than_comparable_v<T const*, U const*>)
    {
        return !std::less<>{}(ptr, begin) && std::less<>{}(ptr, end);
    }
    else
    {
        auto const b = reinterpret_cast<std::uintptr_t>(begin);
        auto const e = reinterpret_cast<std::uintptr_t>(end);
        auto const p = reinterpret_cast<std::uintptr_t>(ptr);
        return p >= b && p < e;
    }
}

[[noreturn]] QX_COLD_NOINLINE inline void throw_out_of_range(char const* msg)
{
    throw std::out_of_range{msg};
}

[[noreturn]] QX_COLD_NOINLINE inline void throw_out_of_capacity(char const* msg = "")
{
    throw std::length_error{msg};
}

template <class T>
struct inplace_vector_empty_base
{
    using size_type = std::size_t;
    using pointer = T*;
    using const_pointer = T const*;

protected:
    constexpr size_type get_size() const noexcept { return 0; }
    constexpr pointer begin_ptr() noexcept { return nullptr; }
    constexpr const_pointer begin_ptr() const noexcept { return nullptr; }
    constexpr pointer end_ptr() noexcept { return nullptr; }
    constexpr const_pointer end_ptr() const noexcept { return nullptr; }
    constexpr void set_end(const_pointer /* end */) noexcept {}
    QX_CONSTEXPR_CXX20 void destroy_from_end(pointer /* new_last */) noexcept {}
    template <class Iterator>
    QX_CONSTEXPR_CXX26 void unchecked_init_with_size(Iterator /* first */, size_type /* n */)
    {}
    template <class Iterator>
    QX_CONSTEXPR_CXX26 void unchecked_assign_with_size(Iterator /* first */, size_type /* n */)
    {}
};

template <std::size_t N, class T, bool = std::is_trivially_destructible_v<T>>
union inplace_vector_array
{
    constexpr inplace_vector_array() noexcept
        : dummy{}
    {}

    char dummy;
    T data[N];
};

template <std::size_t N, class T>

union inplace_vector_array<N, T, /*is_trivially_destructible=*/false>
{
    constexpr inplace_vector_array() noexcept
        : dummy{}
    {}
    QX_CONSTEXPR_CXX20 ~inplace_vector_array() noexcept {}

    char dummy;
    T data[N];
};

template <std::size_t N, class T>
struct inplace_vector_storage
{
    using size_type = std::size_t;
    using pointer = T*;
    using const_pointer = T const*;

    constexpr inplace_vector_storage() noexcept = default;

protected:
    constexpr size_type get_size() const noexcept { return sz_; }
    constexpr pointer begin_ptr() noexcept { return arr_.data; }
    constexpr const_pointer begin_ptr() const noexcept { return arr_.data; }
    constexpr pointer end_ptr() noexcept { return arr_.data + sz_; }
    constexpr const_pointer end_ptr() const noexcept { return arr_.data + sz_; }
    constexpr void set_end(const_pointer end) noexcept { sz_ = static_cast<size_type>(end - begin_ptr()); }

    QX_CONSTEXPR_CXX20 void destroy_from_end(pointer new_last) noexcept
    {
        if constexpr (!std::is_trivially_destructible_v<T>)
        {
            pointer soon_to_be_end = end_ptr();
            while (new_last != soon_to_be_end)
                std::destroy_at(--soon_to_be_end);
        }
        set_end(new_last);
    }

    template <class Iterator>
    QX_CONSTEXPR_CXX26 void unchecked_init_with_size(Iterator first, size_type n)
    {
        auto const new_end = std::uninitialized_copy_n(std::move(first), n, begin_ptr());
        set_end(new_end);
    }

    template <class Iterator>
    QX_CONSTEXPR_CXX26 void unchecked_assign_with_size(Iterator first, size_type n)
    {
        if (n > get_size())
        {
#if QX_WITH_CXX23_RANGES
            auto [mid_in, mid_out] = std::ranges::copy_n(std::move(first), get_size(), begin_ptr());
#else
            Iterator mid_in = std::next(first, get_size());
            auto mid_out = std::copy(first, mid_in, begin_ptr());
#endif
            pointer new_end = std::uninitialized_copy_n(std::move(mid_in), n - get_size(), mid_out);
            set_end(new_end);
        }
        else
        {
            pointer m = std::copy_n(std::move(first), n, begin_ptr());
            destroy_from_end(m);
        }
    }

private:
    size_type sz_{};
    inplace_vector_array<N, T> arr_;
};

// destructor: trivial if T is trivially destructible
template <std::size_t N, class T, bool = std::is_trivially_destructible_v<T>>
struct inplace_vector_dtor_base : inplace_vector_storage<N, T>
{
    inplace_vector_dtor_base() = default;
};

template <std::size_t N, class T>
struct inplace_vector_dtor_base<N, T, /*is_trivially_destructible=*/false> : inplace_vector_storage<N, T>
{
    inplace_vector_dtor_base() = default;
    inplace_vector_dtor_base(inplace_vector_dtor_base const&) = default;
    inplace_vector_dtor_base(inplace_vector_dtor_base&&) = default;
    inplace_vector_dtor_base& operator=(inplace_vector_dtor_base const&) = default;
    inplace_vector_dtor_base& operator=(inplace_vector_dtor_base&&) = default;
    QX_CONSTEXPR_CXX20 ~inplace_vector_dtor_base() noexcept { this->destroy_from_end(this->begin_ptr()); }
};

// copy constructor: trivial if T is trivially copy constructible
template <std::size_t N, class T, bool = std::is_trivially_copy_constructible_v<T> || !std::is_copy_constructible_v<T>>
struct inplace_vector_copy_ctor_base : inplace_vector_dtor_base<N, T>
{
    inplace_vector_copy_ctor_base() = default;
};

template <std::size_t N, class T>
struct inplace_vector_copy_ctor_base<N, T, /*is_trivially_destructible=*/false> : inplace_vector_dtor_base<N, T>
{
    inplace_vector_copy_ctor_base() = default;
    QX_CONSTEXPR_CXX26 inplace_vector_copy_ctor_base(inplace_vector_copy_ctor_base const& other)
        : inplace_vector_dtor_base<N, T>()
    {
        this->unchecked_init_with_size(other.begin_ptr(), other.get_size());
    }
    inplace_vector_copy_ctor_base(inplace_vector_copy_ctor_base&&) = default;
    inplace_vector_copy_ctor_base& operator=(inplace_vector_copy_ctor_base const&) = default;
    inplace_vector_copy_ctor_base& operator=(inplace_vector_copy_ctor_base&&) = default;
    QX_CONSTEXPR_CXX20 ~inplace_vector_copy_ctor_base() = default;
};

// move constructor: trivial if T is trivially move constructible
template <std::size_t N, class T, bool = std::is_trivially_move_constructible_v<T> || !std::is_move_constructible_v<T>>
struct inplace_vector_move_ctor_base : inplace_vector_copy_ctor_base<N, T>
{
    inplace_vector_move_ctor_base() = default;
};

template <std::size_t N, class T>
struct inplace_vector_move_ctor_base<N, T, /*is_trivially_destructible=*/false> : inplace_vector_copy_ctor_base<N, T>
{
    inplace_vector_move_ctor_base() = default;
    inplace_vector_move_ctor_base(inplace_vector_move_ctor_base const&) = default;
    QX_CONSTEXPR_CXX26 inplace_vector_move_ctor_base(inplace_vector_move_ctor_base&& other) noexcept(
        std::is_nothrow_move_constructible_v<T>)
        : inplace_vector_copy_ctor_base<N, T>()
    {
        this->unchecked_init_with_size(std::make_move_iterator(other.begin_ptr()), other.get_size());
    }
    inplace_vector_move_ctor_base& operator=(inplace_vector_move_ctor_base const&) = default;
    inplace_vector_move_ctor_base& operator=(inplace_vector_move_ctor_base&&) = default;
    QX_CONSTEXPR_CXX20 ~inplace_vector_move_ctor_base() = default;
};

// copy assignment: trivial if T is trivially destructible, copy constructible and copy assignable
template <std::size_t N,
    class T,
    bool = (std::is_trivially_destructible_v<T> && std::is_trivially_copy_constructible_v<T> && std::is_trivially_copy_assignable_v<T>) ||
        !std::is_copy_assignable_v<T>>
struct inplace_vector_copy_assign_base : inplace_vector_move_ctor_base<N, T>
{
    inplace_vector_copy_assign_base() = default;
};

template <std::size_t N, class T>
struct inplace_vector_copy_assign_base<N, T, /*is_trivially_destructible=*/false> : inplace_vector_move_ctor_base<N, T>
{
    using size_type = std::size_t;
    using pointer = T*;
    using const_pointer = T const*;

    inplace_vector_copy_assign_base() = default;
    inplace_vector_copy_assign_base(inplace_vector_copy_assign_base const&) = default;
    inplace_vector_copy_assign_base(inplace_vector_copy_assign_base&&) = default;
    QX_CONSTEXPR_CXX26 inplace_vector_copy_assign_base& operator=(inplace_vector_copy_assign_base const& other)
    {
        if (this != &other)
            this->unchecked_assign_with_size(other.begin_ptr(), other.get_size());
        return *this;
    }
    inplace_vector_copy_assign_base& operator=(inplace_vector_copy_assign_base&&) = default;
    QX_CONSTEXPR_CXX20 ~inplace_vector_copy_assign_base() = default;
};

// move assignment: trivial if T is trivially destructible, move constructible and move assignable
template <std::size_t N,
    class T,
    bool = (std::is_trivially_destructible_v<T> && std::is_trivially_move_constructible_v<T> && std::is_trivially_move_assignable_v<T>) ||
        !std::is_move_assignable_v<T>>
struct inplace_vector_move_assign_base : inplace_vector_copy_assign_base<N, T>
{
    inplace_vector_move_assign_base() = default;
};

template <std::size_t N, class T>
struct inplace_vector_move_assign_base<N, T, /*is_trivially_destructible=*/false> : inplace_vector_copy_assign_base<N, T>
{
    inplace_vector_move_assign_base() = default;
    inplace_vector_move_assign_base(inplace_vector_move_assign_base const&) = default;
    inplace_vector_move_assign_base(inplace_vector_move_assign_base&&) = default;
    inplace_vector_move_assign_base& operator=(inplace_vector_move_assign_base const&) = default;
    QX_CONSTEXPR_CXX26 inplace_vector_move_assign_base& operator=(inplace_vector_move_assign_base&& other) noexcept(
        std::is_nothrow_move_assignable_v<T> && std::is_nothrow_move_constructible_v<T>)
    {
        if (this != &other)
            this->unchecked_assign_with_size(std::make_move_iterator(other.begin_ptr()), other.get_size());
        return *this;
    }
    QX_CONSTEXPR_CXX20 ~inplace_vector_move_assign_base() = default;
};

template <std::size_t N, class T>
using inplace_vector_base = std::conditional_t<N == 0, inplace_vector_empty_base<T>, inplace_vector_move_assign_base<N, T>>;

} // namespace intl

template <std::size_t N, class T>
class inplace_vector : private intl::inplace_vector_base<N, T>
{
    using base = intl::inplace_vector_base<N, T>;

    template <class Iter, class Tag>
    static constexpr bool is_valid_iter_with_category_v = intl::is_constructible_with_iter<T, Iter, Tag>::value;

public:
    using value_type = T;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using reference = value_type&;
    using const_reference = value_type const&;
    using pointer = value_type*;
    using const_pointer = value_type const*;

    using iterator = pointer;
    using const_iterator = const_pointer;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    inplace_vector() noexcept = default;

    QX_CONSTEXPR_CXX26 explicit inplace_vector(size_type n)
    {
        if (n > 0)
        {
            if (n <= max_size())
                this->construct_at_end(n);
            else
                intl::throw_out_of_capacity();
        }
    }

    QX_CONSTEXPR_CXX26 inplace_vector(size_type n, value_type const& value)
    {
        if (n > 0)
        {
            if (n <= max_size())
                this->construct_at_end(n, value);
            else
                intl::throw_out_of_capacity();
        }
    }

    template <class InputIterator, std::enable_if_t<is_valid_iter_with_category_v<InputIterator, std::input_iterator_tag>, int> = 0>
    QX_CONSTEXPR_CXX26 inplace_vector(InputIterator first, InputIterator last)
    {
        if constexpr (is_valid_iter_with_category_v<InputIterator, std::forward_iterator_tag>)
        {
            auto n = static_cast<size_type>(std::distance(first, last));
            init_with_size(std::move(first), std::move(last), n);
        }
        else
        {
            init_with_sentinel(std::move(first), std::move(last));
        }
    }

#if QX_WITH_CXX23_RANGES
    template <class Range>
        requires std::ranges::input_range<Range> && std::constructible_from<T, std::ranges::range_reference_t<Range>>
    constexpr inplace_vector(std::from_range_t /* std::from_range */, Range&& range)
    {
        if constexpr (std::ranges::forward_range<Range>)
        {
            auto const n = static_cast<size_type>(std::ranges::distance(range));
            init_with_size(std::ranges::begin(range), std::ranges::end(range), n);
        }
        else
        {
            init_with_sentinel(std::ranges::begin(range), std::ranges::end(range));
        }
    }
#endif

    QX_CONSTEXPR_CXX26 inplace_vector(std::initializer_list<value_type> il) { init_with_size(il.begin(), il.end(), il.size()); }

    QX_CONSTEXPR_CXX26 inplace_vector& operator=(std::initializer_list<value_type> il)
    {
        assign(il.begin(), il.end());
        return *this;
    }

    template <class InputIterator, std::enable_if_t<is_valid_iter_with_category_v<InputIterator, std::input_iterator_tag>, int> = 0>
    QX_CONSTEXPR_CXX26 void assign(InputIterator first, InputIterator last)
    {
        if constexpr (is_valid_iter_with_category_v<InputIterator, std::forward_iterator_tag>)
        {
            auto n = static_cast<size_type>(std::distance(first, last));
            return assign_with_size(std::move(first), std::move(last), n);
        }
        else
        {
            return assign_with_sentinel(std::move(first), std::move(last));
        }
    }

#if QX_WITH_CXX23_RANGES
    template <class Range>
        requires std::ranges::input_range<Range> && std::constructible_from<T, std::ranges::range_reference_t<Range>>
    constexpr void assign_range(Range&& range)
    {
        if constexpr (std::ranges::forward_range<Range>)
        {
            auto const n = static_cast<size_type>(std::ranges::distance(range));
            assign_with_size(std::ranges::begin(range), std::ranges::end(range), n);
        }
        else
        {
            assign_with_sentinel(std::ranges::begin(range), std::ranges::end(range));
        }
    }
#endif

    QX_CONSTEXPR_CXX26 void assign(size_type n, value_type const& u)
    {
        if (n <= capacity())
        {
            size_type sz = size();
            std::fill_n(this->begin_ptr(), std::min(n, sz), u);
            if (n > sz)
                this->construct_at_end(n - sz, u);
            else
                this->destroy_from_end(this->begin_ptr() + n);
        }
        else
        {
            intl::throw_out_of_capacity();
        }
    }

    QX_CONSTEXPR_CXX26 void assign(std::initializer_list<value_type> il) { return assign(il.begin(), il.end()); }

    constexpr iterator begin() noexcept { return iterator(this->begin_ptr()); }
    constexpr const_iterator begin() const noexcept { return const_iterator(this->begin_ptr()); }
    constexpr iterator end() noexcept { return iterator(this->end_ptr()); }
    constexpr const_iterator end() const noexcept { return const_iterator(this->end_ptr()); }

    constexpr reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }
    constexpr const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator(end()); }
    constexpr reverse_iterator rend() noexcept { return reverse_iterator(begin()); }
    constexpr const_reverse_iterator rend() const noexcept { return const_reverse_iterator(begin()); }

    constexpr const_iterator cbegin() const noexcept { return begin(); }
    constexpr const_iterator cend() const noexcept { return end(); }
    constexpr const_reverse_iterator crbegin() const noexcept { return rbegin(); }
    constexpr const_reverse_iterator crend() const noexcept { return rend(); }

    constexpr size_type size() const noexcept { return this->get_size(); }
    static constexpr size_type max_size() noexcept { return N; }
    static constexpr size_type capacity() noexcept { return N; }
    constexpr bool empty() const noexcept { return size() == 0; }

    static constexpr void reserve(size_type n)
    {
        if (n > capacity())
            intl::throw_out_of_capacity();
    }
    static constexpr void shrink_to_fit() noexcept { /*nop*/ }

    constexpr reference operator[](size_type n) noexcept
    {
        QX_ASSERT_CONTRACT(n < size(), "inplace_vector[] index out of bounds");
        return this->begin_ptr()[n];
    }
    constexpr const_reference operator[](size_type n) const noexcept
    {
        QX_ASSERT_CONTRACT(n < size(), "inplace_vector[] index out of bounds");
        return this->begin_ptr()[n];
    }
    constexpr reference at(size_type n)
    {
        if (n >= size())
            intl::throw_out_of_range("inplace_vector::at");
        return this->begin_ptr()[n];
    }
    constexpr const_reference at(size_type n) const
    {
        if (n >= size())
            intl::throw_out_of_range("inplace_vector::at");
        return this->begin_ptr()[n];
    }

    constexpr reference front() noexcept
    {
        QX_ASSERT_CONTRACT(!empty(), "front() called on an empty inplace_vector");
        return *this->begin_ptr();
    }
    constexpr const_reference front() const noexcept
    {
        QX_ASSERT_CONTRACT(!empty(), "front() called on an empty inplace_vector");
        return *this->begin_ptr();
    }
    constexpr reference back() noexcept
    {
        QX_ASSERT_CONTRACT(!empty(), "back() called on an empty inplace_vector");
        return *(this->end_ptr() - 1);
    }
    constexpr const_reference back() const noexcept
    {
        QX_ASSERT_CONTRACT(!empty(), "back() called on an empty inplace_vector");
        return *(this->end_ptr() - 1);
    }

    constexpr pointer data() noexcept { return this->begin_ptr(); }
    constexpr const_pointer data() const noexcept { return this->begin_ptr(); }

    QX_CONSTEXPR_CXX26 reference push_back(value_type const& x) { return emplace_back(x); }
    QX_CONSTEXPR_CXX26 reference push_back(value_type&& x) { return emplace_back(std::move(x)); }

    QX_CONSTEXPR_CXX26 pointer try_push_back(value_type const& x) noexcept(std::is_nothrow_copy_constructible_v<T>)
    {
        return try_emplace_back(x);
    }
    QX_CONSTEXPR_CXX26 pointer try_push_back(value_type&& x) noexcept(std::is_nothrow_move_constructible_v<T>)
    {
        return try_emplace_back(std::move(x));
    }

    QX_CONSTEXPR_CXX26 reference unchecked_push_back(value_type const& x) noexcept(std::is_nothrow_copy_constructible_v<T>)
    {
        return unchecked_emplace_back(x);
    }
    QX_CONSTEXPR_CXX26 reference unchecked_push_back(value_type&& x) noexcept(std::is_nothrow_move_constructible_v<T>)
    {
        return unchecked_emplace_back(std::move(x));
    }

    template <class... Args>
    QX_CONSTEXPR_CXX26 reference emplace_back(Args&&... args)
    {
        if (size() < capacity())
            return unchecked_emplace_back(std::forward<Args>(args)...);
        else
            intl::throw_out_of_capacity();
    }

    template <class... Args>
    QX_CONSTEXPR_CXX26 pointer try_emplace_back(Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args...>)
    {
        if (size() < capacity())
        {
            this->construct_one_at_end(std::forward<Args>(args)...);
            return this->end_ptr() - 1;
        }
        return nullptr;
    }

    template <class... Args>
    QX_CONSTEXPR_CXX26 reference unchecked_emplace_back(Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args...>)
    {
        QX_ASSERT_CONTRACT(size() != capacity(), "inplace_vector::unchecked_emplace_back called on a full inplace_vector");
        this->construct_one_at_end(std::forward<Args>(args)...);
        return *(this->end_ptr() - 1);
    }

#if QX_WITH_CXX23_RANGES
    template <class Range>
        requires std::ranges::input_range<Range> && std::constructible_from<T, std::ranges::range_reference_t<Range>>
    constexpr void append_range(Range&& range)
    {
        if constexpr (std::ranges::forward_range<Range>)
        {
            auto const len = std::ranges::distance(range);
            auto const rem_cap = static_cast<decltype(len)>(capacity() - size());
            if (len <= rem_cap)
                this->construct_at_end(std::ranges::begin(range), std::ranges::end(range), static_cast<size_type>(len));
            else
                intl::throw_out_of_capacity();
        }
        else
        {
            append_with_sentinel(std::ranges::begin(range), std::ranges::end(range));
        }
    }
#endif

    QX_CONSTEXPR_CXX20 void pop_back() noexcept
    {
        QX_ASSERT_CONTRACT(!empty(), "inplace_vector::pop_back called on an empty inplace_vector");
        this->destroy_from_end(this->end_ptr() - 1);
    }

    template <class... Args>
    QX_CONSTEXPR_CXX26 iterator emplace(const_iterator pos, Args&&... args)
    {
        QX_ASSERT_CONTRACT(begin() <= pos && pos <= end(), "inplace_vector::emplace(iterator, ...) called with an invalid iterator");
        pointer p = pointer_at_position(pos);
        if (size() < capacity())
        {
            if (p == this->end_ptr())
            {
                this->construct_one_at_end(std::forward<Args>(args)...);
            }
            else
            {
                value_type tmp(std::forward<Args>(args)...);
                this->move_range(p, this->end_ptr(), p + 1);
                *p = std::move(tmp);
            }
        }
        else
        {
            intl::throw_out_of_capacity();
        }
        return iterator(p);
    }

    QX_CONSTEXPR_CXX26 iterator insert(const_iterator pos, const_reference x)
    {
        QX_ASSERT_CONTRACT(begin() <= pos && pos <= end(), "inplace_vector::insert(iterator, x) called with an invalid iterator");
        pointer p = pointer_at_position(pos);
        if (size() < capacity())
        {
            if (p == this->end_ptr())
            {
                this->construct_one_at_end(x);
            }
            else
            {
                this->move_range(p, this->end_ptr(), p + 1);
                const_pointer xr = std::pointer_traits<const_pointer>::pointer_to(x);
                if (intl::is_pointer_in_range(p, this->end_ptr(), std::addressof(x)))
                    ++xr;
                *p = *xr;
            }
        }
        else
        {
            intl::throw_out_of_capacity();
        }
        return iterator(p);
    }

    QX_CONSTEXPR_CXX26 iterator insert(const_iterator pos, value_type&& x)
    {
        QX_ASSERT_CONTRACT(begin() <= pos && pos <= end(), "inplace_vector::insert(iterator, x) called with an invalid iterator");
        pointer p = pointer_at_position(pos);
        if (size() < capacity())
        {
            if (p == this->end_ptr())
            {
                this->construct_one_at_end(std::move(x));
            }
            else
            {
                this->move_range(p, this->end_ptr(), p + 1);
                *p = std::move(x);
            }
        }
        else
        {
            intl::throw_out_of_capacity();
        }
        return iterator(p);
    }

    QX_CONSTEXPR_CXX26 iterator insert(const_iterator pos, size_type n, value_type const& x)
    {
        QX_ASSERT_CONTRACT(begin() <= pos && pos <= end(), "inplace_vector::insert(iterator, n, x) called with an invalid iterator");
        pointer p = pointer_at_position(pos);
        if (n > 0)
        {
            if (n <= capacity() - size())
            {
                size_type old_n = n;
                pointer old_last = this->end_ptr();
                auto off = static_cast<size_type>(old_last - p);
                if (n > off)
                {
                    size_type cx = n - off;
                    this->construct_at_end(cx, x);
                    n -= cx;
                }
                if (n > 0)
                {
                    this->move_range(p, old_last, p + old_n);
                    const_pointer xr = std::pointer_traits<const_pointer>::pointer_to(x);
                    if (intl::is_pointer_in_range(p, this->end_ptr(), xr))
                        xr += old_n;
                    std::fill_n(p, n, *xr);
                }
            }
            else
            {
                intl::throw_out_of_capacity();
            }
        }
        return iterator(p);
    }

    template <class InputIterator, std::enable_if_t<is_valid_iter_with_category_v<InputIterator, std::input_iterator_tag>, int> = 0>
    QX_CONSTEXPR_CXX26 iterator insert(const_iterator pos, InputIterator first, InputIterator last)
    {
        if constexpr (is_valid_iter_with_category_v<InputIterator, std::forward_iterator_tag>)
        {
            auto n = static_cast<size_type>(std::distance(first, last));
            return insert_with_size(pos, std::move(first), std::move(last), n);
        }
        else
        {
            return insert_with_sentinel(pos, std::move(first), std::move(last));
        }
    }

#if QX_WITH_CXX23_RANGES
    template <class Range>
        requires std::ranges::input_range<Range> && std::constructible_from<T, std::ranges::range_reference_t<Range>>
    constexpr iterator insert_range(const_iterator pos, Range&& range)
    {
        if constexpr (std::ranges::forward_range<Range>)
        {
            auto const n = static_cast<size_type>(std::ranges::distance(range));
            return insert_with_size(pos, std::ranges::begin(range), std::ranges::end(range), n);
        }
        else
        {
            return insert_with_sentinel(pos, std::ranges::begin(range), std::ranges::end(range));
        }
    }
#endif

    QX_CONSTEXPR_CXX26 iterator insert(const_iterator pos, std::initializer_list<value_type> il)
    {
        return insert(pos, il.begin(), il.end());
    }

    QX_CONSTEXPR_CXX20 iterator erase(const_iterator pos)
    {
        QX_ASSERT_CONTRACT(begin() <= pos && pos < end(), "inplace_vector::erase(iterator) called with a non-dereferenceable iterator");
        pointer p = pointer_at_position(pos);
        this->destroy_from_end(std::move(p + 1, this->end_ptr(), p));
        return iterator(p);
    }

    QX_CONSTEXPR_CXX20 iterator erase(const_iterator first, const_iterator last)
    {
        QX_ASSERT_CONTRACT(
            begin() <= first && first <= last && last <= end(), "inplace_vector::erase(first, last) called with invalid range");
        pointer p = pointer_at_position(first);
        if (first != last)
        {
            this->destroy_from_end(std::move(p + (last - first), this->end_ptr(), p));
        }
        return iterator(p);
    }

    QX_CONSTEXPR_CXX20 void clear() noexcept { this->destroy_from_end(this->begin_ptr()); }

    QX_CONSTEXPR_CXX26 void resize(size_type sz)
    {
        size_type const cs = size();
        if (cs < sz)
        {
            if (capacity() >= sz)
                this->construct_at_end(sz - cs);
            else
                intl::throw_out_of_capacity();
        }
        else if (cs > sz)
        {
            this->destroy_from_end(this->begin_ptr() + sz);
        }
    }

    QX_CONSTEXPR_CXX26 void resize(size_type sz, value_type const& c)
    {
        size_type const cs = size();
        if (cs < sz)
        {
            if (capacity() >= sz)
                this->construct_at_end(sz - cs, c);
            else
                intl::throw_out_of_capacity();
        }
        else if (cs > sz)
        {
            this->destroy_from_end(this->begin_ptr() + sz);
        }
    }

    QX_CONSTEXPR_CXX26 void swap(inplace_vector& other) noexcept(
        N == 0 || (std::is_nothrow_swappable_v<T> && std::is_nothrow_move_constructible_v<T>))
    {
        if constexpr (N == 0)
        {
            return;
        }
        else
        {
            if (this == &other)
                return;

            size_type const common = std::min(size(), other.size());
            pointer const mid = this->begin_ptr() + common;
            std::swap_ranges(this->begin_ptr(), mid, other.begin_ptr());

            if (size() > other.size())
            {
                pointer const new_end = std::uninitialized_move_n(mid, size() - common, other.end_ptr()).second;
                other.set_end(new_end);
                this->destroy_from_end(mid);
            }
            else if (size() < other.size())
            {
                pointer const other_mid = other.begin_ptr() + common;
                pointer const new_end = std::uninitialized_move_n(other_mid, other.size() - common, this->end_ptr()).second;
                this->set_end(new_end);
                other.destroy_from_end(other_mid);
            }
        }
    }

    friend QX_CONSTEXPR_CXX26 void swap(inplace_vector& a, inplace_vector& b) noexcept(noexcept(a.swap(b))) { a.swap(b); }

    friend QX_CONSTEXPR_CXX20 bool operator==(inplace_vector const& a, inplace_vector const& b)
    {
        return a.size() == b.size() && std::equal(a.begin(), a.end(), b.begin());
    }

#if QX_WITH_THREE_WAY
    friend constexpr auto operator<=>(inplace_vector const& a, inplace_vector const& b)
        requires std::is_invocable_v<intl::synth_three_way, T const&, T const&>
    {
        return std::lexicographical_compare_three_way(a.begin(), a.end(), b.begin(), b.end(), intl::synth_three_way{});
    }
#else
    friend QX_CONSTEXPR_CXX20 bool operator!=(inplace_vector const& a, inplace_vector const& b) { return !(a == b); }

    friend QX_CONSTEXPR_CXX20 bool operator<(inplace_vector const& a, inplace_vector const& b)
    {
        return std::lexicographical_compare(a.begin(), a.end(), b.begin(), b.end());
    }
    friend QX_CONSTEXPR_CXX20 bool operator>(inplace_vector const& a, inplace_vector const& b) { return b < a; }
    friend QX_CONSTEXPR_CXX20 bool operator<=(inplace_vector const& a, inplace_vector const& b) { return !(b < a); }
    friend QX_CONSTEXPR_CXX20 bool operator>=(inplace_vector const& a, inplace_vector const& b) { return !(a < b); }
#endif

private:
    constexpr pointer pointer_at_position(const_iterator pos) noexcept { return this->begin_ptr() + (pos - begin()); }

    template <class... Args>
    QX_CONSTEXPR_CXX26 void construct_one_at_end(Args&&... args)
    {
        ::new (this->end_ptr()) value_type(std::forward<Args>(args)...);
        this->set_end(this->end_ptr() + 1);
    }

    QX_CONSTEXPR_CXX26 void construct_at_end(size_type n)
    {
        pointer const new_end = std::uninitialized_value_construct_n(this->end_ptr(), n);
        this->set_end(new_end);
    }

    QX_CONSTEXPR_CXX26 void construct_at_end(size_type n, value_type const& x)
    {
        pointer const new_end = std::uninitialized_fill_n(this->end_ptr(), n, x);
        this->set_end(new_end);
    }

    template <class InputIterator, class Sentinel>
    QX_CONSTEXPR_CXX26 void construct_at_end(InputIterator first, Sentinel /* last */, size_type n)
    {
        pointer const new_end = std::uninitialized_copy_n(std::move(first), n, this->end_ptr());
        this->set_end(new_end);
    }

    QX_CONSTEXPR_CXX26 pointer move_range(pointer in_first, pointer in_last, pointer o_first)
    {
        pointer const old_last = this->end_ptr();
        difference_type const n = old_last - o_first;
        pointer const i = in_first + n;

        pointer const new_end = std::uninitialized_move(i, in_last, old_last);
        this->set_end(new_end);
        (void)std::move_backward(in_first, i, old_last);
        return new_end;
    }

    template <class InputIterator, class Sentinel>
    QX_CONSTEXPR_CXX26 void init_with_size(InputIterator first, Sentinel /*last*/, size_type n)
    {
        if (n > 0)
        {
            if (n <= max_size())
                this->unchecked_init_with_size(std::move(first), n);
            else
                intl::throw_out_of_capacity();
        }
    }

    template <class InputIterator, class Sentinel>
    QX_CONSTEXPR_CXX26 void init_with_sentinel(InputIterator first, Sentinel last)
    {
        try
        {
            for (; first != last; ++first)
                emplace_back(*first);
        }
        catch (...)
        {
            clear();
            throw;
        }
    }

    template <class InputIterator, class Sentinel>
    QX_CONSTEXPR_CXX26 void append_with_sentinel(InputIterator first, Sentinel last)
    {
        pointer const old_end = this->end_ptr();
        try
        {
            for (; first != last; ++first)
                emplace_back(*first);
        }
        catch (...)
        {
            this->destroy_from_end(old_end);
            throw;
        }
    }

    template <class InputIterator, class Sentinel>
    QX_CONSTEXPR_CXX20 void assign_with_size(InputIterator first, Sentinel /*last*/, difference_type n)
    {
        auto const new_size = static_cast<size_type>(n);
        if (new_size <= capacity())
        {
            this->unchecked_assign_with_size(std::move(first), n);
        }
        else
        {
            intl::throw_out_of_capacity();
        }
    }

    template <class InputIterator, class Sentinel>
    QX_CONSTEXPR_CXX26 void assign_with_sentinel(InputIterator first, Sentinel last)
    {
        pointer cur = this->begin_ptr();
        for (; first != last && cur != this->end_ptr(); ++first, (void)++cur)
            *cur = *first;

        if (cur != this->end_ptr())
        {
            this->destroy_from_end(cur);
        }
        else
        {
            for (; first != last; ++first)
                emplace_back(*first);
        }
    }

    template <class InputIterator, class Sentinel>
    QX_CONSTEXPR_CXX26 iterator insert_with_sentinel(const_iterator pos, InputIterator first, Sentinel last)
    {
        QX_ASSERT_CONTRACT(begin() <= pos && pos <= end(), "inplace_vector::insert(pos, first, last) called with invalid pos");
        pointer p = pointer_at_position(pos);
        pointer old_last = this->end_ptr();
        for (; size() != capacity() && first != last; ++first)
            this->construct_one_at_end(*first);

        (void)std::rotate(p, old_last, this->end_ptr());
        if (first != last)
            intl::throw_out_of_capacity();

        return iterator(p);
    }

    template <class Iterator, class Sentinel>
    QX_CONSTEXPR_CXX26 iterator insert_with_size(const_iterator pos, Iterator first, Sentinel last, difference_type n)
    {
        QX_ASSERT_CONTRACT(begin() <= pos && pos <= end(), "inplace_vector::insert(pos, first, last) called with invalid pos");
        pointer p = pointer_at_position(pos);
        if (n > 0)
        {
            if (static_cast<size_type>(n) <= capacity() - size())
            {
                pointer old_last = this->end_ptr();
                difference_type dx = this->end_ptr() - p;
                if (n > dx)
                {
                    Iterator m = std::next(first, dx);
                    this->construct_at_end(m, last, n - dx);
                    if (dx > 0)
                    {
                        this->move_range(p, old_last, p + n);
                        std::copy(std::move(first), std::move(m), p);
                    }
                }
                else
                {
                    this->move_range(p, old_last, p + n);
                    std::copy_n(std::move(first), n, p);
                }
            }
            else
            {
                intl::throw_out_of_capacity();
            }
        }
        return iterator(p);
    }
};

template <std::size_t N, class T, class U = T>
QX_CONSTEXPR_CXX20 typename inplace_vector<N, T>::size_type erase(inplace_vector<N, T>& c, U const& value)
{
    auto const it = std::remove(c.begin(), c.end(), value);
    auto const removed = static_cast<typename inplace_vector<N, T>::size_type>(c.end() - it);
    c.erase(it, c.end());
    return removed;
}

template <std::size_t N, class T, class Predicate>
QX_CONSTEXPR_CXX20 typename inplace_vector<N, T>::size_type erase_if(inplace_vector<N, T>& c, Predicate pred)
{
    auto const it = std::remove_if(c.begin(), c.end(), pred);
    auto const removed = static_cast<typename inplace_vector<N, T>::size_type>(c.end() - it);
    c.erase(it, c.end());
    return removed;
}

} // namespace qx
