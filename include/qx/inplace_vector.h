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

// contract hardening level (0: none, 1: all, default: debug-only)
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

[[noreturn]] QX_COLD_NOINLINE inline void throw_out_of_capacity(char const* msg)
{
    throw std::length_error{msg};
}

template <class T>
struct inplace_vector_storage_empty
{
    using size_type = std::size_t;
    using pointer = T*;
    using const_pointer = T const*;

protected:
    constexpr size_type size() const noexcept { return 0; }
    constexpr pointer begin() noexcept { return nullptr; }
    constexpr const_pointer begin() const noexcept { return nullptr; }
    constexpr pointer end() noexcept { return nullptr; }
    constexpr const_pointer end() const noexcept { return nullptr; }
    constexpr void set_size(size_type /* n */) noexcept {}
    constexpr void set_end(const_pointer /* end */) noexcept {}

    QX_CONSTEXPR_CXX20 void destroy_from_end(pointer /* new_last */) noexcept {}
};

template <std::size_t N, class T>
struct inplace_vector_storage_trivial
{
    using size_type = std::size_t;
    using pointer = T*;
    using const_pointer = T const*;

    constexpr inplace_vector_storage_trivial() noexcept
        : sz_{}
        , dummy_{}
    {}

protected:
    constexpr size_type size() const noexcept { return sz_; }
    constexpr pointer begin() noexcept { return data_; }
    constexpr const_pointer begin() const noexcept { return data_; }
    constexpr pointer end() noexcept { return data_ + sz_; }
    constexpr const_pointer end() const noexcept { return data_ + sz_; }
    constexpr void set_size(size_type n) noexcept { sz_ = n; }
    constexpr void set_end(const_pointer end) noexcept { sz_ = static_cast<size_type>(end - begin()); }

    QX_CONSTEXPR_CXX20 void destroy_from_end(pointer new_last) noexcept { set_end(new_last); }

private:
    size_type sz_;
    union
    {
        char dummy_;
        T data_[N];
    };
};

template <std::size_t N, class T>
struct inplace_vector_storage_nontrivial
{
    using size_type = std::size_t;
    using pointer = T*;
    using const_pointer = T const*;

    constexpr inplace_vector_storage_nontrivial() noexcept
        : sz_{}
        , dummy_{}
    {}

    QX_CONSTEXPR_CXX20 ~inplace_vector_storage_nontrivial() noexcept { destroy_from_end(begin()); }

    inplace_vector_storage_nontrivial(inplace_vector_storage_nontrivial const& other)
        : inplace_vector_storage_nontrivial()
    {
        pointer new_end = std::uninitialized_copy_n(other.begin(), other.size(), begin());
        set_end(new_end);
    }

    inplace_vector_storage_nontrivial(inplace_vector_storage_nontrivial&& other) noexcept(std::is_nothrow_move_constructible_v<T>)
        : inplace_vector_storage_nontrivial()
    {
        set_end(std::uninitialized_move_n(other.begin(), other.size(), begin()).second);
    }

    inplace_vector_storage_nontrivial& operator=(inplace_vector_storage_nontrivial const& other)
    {
        if (this != &other)
            assign(other.begin(), other.size());
        return *this;
    }

    inplace_vector_storage_nontrivial& operator=(inplace_vector_storage_nontrivial&& other) noexcept(
        std::is_nothrow_move_assignable_v<T> && std::is_nothrow_move_constructible_v<T>)
    {
        if (this != &other)
            assign(std::make_move_iterator(other.begin()), other.size());
        return *this;
    }

protected:
    constexpr size_type size() const noexcept { return sz_; }
    constexpr pointer begin() noexcept { return data_; }
    constexpr const_pointer begin() const noexcept { return data_; }
    constexpr pointer end() noexcept { return data_ + sz_; }
    constexpr const_pointer end() const noexcept { return data_ + sz_; }
    constexpr void set_size(size_type n) noexcept { sz_ = n; }
    constexpr void set_end(const_pointer end) noexcept { sz_ = static_cast<size_type>(end - begin()); }

    QX_CONSTEXPR_CXX20 void destroy_from_end(pointer new_last) noexcept
    {
        pointer soon_to_be_end = end();
        while (new_last != soon_to_be_end)
            std::destroy_at(intl::to_address(--soon_to_be_end));
        set_end(new_last);
    }

private:
    size_type sz_;
    union
    {
        char dummy_;
        T data_[N];
    };

    template <class Iterator>
    void assign(Iterator first, size_type n)
    {
        size_type const sz = size();
        size_type const common = std::min(n, sz);
        (void)std::copy_n(first, common, begin()); // may throw: size unchanged
        if (n < sz)
        {
            destroy_from_end(begin() + n);
        }
        else
        {
            size_type const dx = n - common;
            pointer new_end = std::uninitialized_copy_n(std::next(first, common), dx, end());
            set_end(new_end);
        }
    }
};

template <std::size_t N, class T>
using inplace_vector_storage = std::conditional_t<N == 0,
    intl::inplace_vector_storage_empty<T>,
    std::conditional_t<std::is_trivially_copyable_v<T>,
        intl::inplace_vector_storage_trivial<N, T>,
        intl::inplace_vector_storage_nontrivial<N, T>>>;

} // namespace intl

template <std::size_t N, class T>
class inplace_vector : private intl::inplace_vector_storage<N, T>
{
    using base = intl::inplace_vector_storage<N, T>;

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

    explicit inplace_vector(size_type n)
    {
        if (n > max_size())
            intl::throw_out_of_capacity("inplace_vector::inplace_vector");
        construct_at_end(n);
    }

    inplace_vector(size_type n, value_type const& value)
    {
        if (n > max_size())
            intl::throw_out_of_capacity("inplace_vector::inplace_vector");
        construct_at_end(n, value);
    }

    template <class InputIterator, std::enable_if_t<is_valid_iter_with_category_v<InputIterator, std::input_iterator_tag>, int> = 0>
    inplace_vector(InputIterator first, InputIterator last)
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

    // template<container-compatible-range<T> R>
    // constexpr inplace_vector(from_range_t, R&& rg); // C++23

    inplace_vector(std::initializer_list<value_type> il) { init_with_size(il.begin(), il.end(), il.size()); }

    inplace_vector& operator=(std::initializer_list<value_type> il)
    {
        assign(il.begin(), il.end());
        return *this;
    }

    template <class InputIterator, std::enable_if_t<is_valid_iter_with_category_v<InputIterator, std::input_iterator_tag>, int> = 0>
    void assign(InputIterator first, InputIterator last)
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

    // template<container-compatible-range<T> R>
    // constexpr void assign_range(R&& rg); // C++23

    void assign(size_type n, value_type const& u)
    {
        if (n <= capacity())
        {
            size_type sz = size();
            std::fill_n(base::begin(), std::min(n, sz), u);
            if (n > sz)
                this->construct_at_end(n - sz, u);
            else
                base::destroy_from_end(base::begin() + n);
        }
        else
        {
            intl::throw_out_of_capacity("inplace_vector::assign");
        }
    }

    void assign(std::initializer_list<value_type> il) { return assign(il.begin(), il.end()); }

    constexpr iterator begin() noexcept { return iterator(base::begin()); }
    constexpr const_iterator begin() const noexcept { return const_iterator(base::begin()); }
    constexpr iterator end() noexcept { return iterator(base::end()); }
    constexpr const_iterator end() const noexcept { return const_iterator(base::end()); }

    constexpr reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }
    constexpr const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator(end()); }
    constexpr reverse_iterator rend() noexcept { return reverse_iterator(begin()); }
    constexpr const_reverse_iterator rend() const noexcept { return const_reverse_iterator(begin()); }

    constexpr const_iterator cbegin() const noexcept { return begin(); }
    constexpr const_iterator cend() const noexcept { return end(); }
    constexpr const_reverse_iterator crbegin() const noexcept { return rbegin(); }
    constexpr const_reverse_iterator crend() const noexcept { return rend(); }

    constexpr size_type size() const noexcept { return base::size(); }
    static constexpr size_type max_size() noexcept { return N; }
    static constexpr size_type capacity() noexcept { return N; }
    constexpr bool empty() const noexcept { return size() == 0; }

    static constexpr void reserve(size_type n)
    {
        if (n > capacity())
            intl::throw_out_of_capacity("inplace_vector::reserve");
    }
    static constexpr void shrink_to_fit() noexcept { /*nop*/ }

    constexpr reference operator[](size_type n) noexcept
    {
        QX_ASSERT_CONTRACT(n < size(), "inplace_vector[] index out of bounds");
        return base::begin()[n];
    }
    constexpr const_reference operator[](size_type n) const noexcept
    {
        QX_ASSERT_CONTRACT(n < size(), "inplace_vector[] index out of bounds");
        return base::begin()[n];
    }
    constexpr reference at(size_type n)
    {
        if (n >= size())
            intl::throw_out_of_range("inplace_vector::at");
        return base::begin()[n];
    }
    constexpr const_reference at(size_type n) const
    {
        if (n >= size())
            intl::throw_out_of_range("inplace_vector::at");
        return base::begin()[n];
    }

    constexpr reference front() noexcept
    {
        QX_ASSERT_CONTRACT(!empty(), "front() called on an empty inplace_vector");
        return *base::begin();
    }
    constexpr const_reference front() const noexcept
    {
        QX_ASSERT_CONTRACT(!empty(), "front() called on an empty inplace_vector");
        return *base::begin();
    }
    constexpr reference back() noexcept
    {
        QX_ASSERT_CONTRACT(!empty(), "back() called on an empty inplace_vector");
        return *(base::end() - 1);
    }
    constexpr const_reference back() const noexcept
    {
        QX_ASSERT_CONTRACT(!empty(), "back() called on an empty inplace_vector");
        return *(base::end() - 1);
    }

    constexpr pointer data() noexcept { return base::begin(); }
    constexpr const_pointer data() const noexcept { return base::begin(); }

    reference push_back(value_type const& x) { return emplace_back(x); }
    reference push_back(value_type&& x) { return emplace_back(std::move(x)); }

    pointer try_push_back(value_type const& x) noexcept(std::is_nothrow_copy_constructible_v<T>) { return try_emplace_back(x); }
    pointer try_push_back(value_type&& x) noexcept(std::is_nothrow_move_constructible_v<T>) { return try_emplace_back(std::move(x)); }

    reference unchecked_push_back(value_type const& x) noexcept(std::is_nothrow_copy_constructible_v<T>)
    {
        return unchecked_emplace_back(x);
    }
    reference unchecked_push_back(value_type&& x) noexcept(std::is_nothrow_move_constructible_v<T>)
    {
        return unchecked_emplace_back(std::move(x));
    }

    template <class... Args>
    reference emplace_back(Args&&... args)
    {
        if (size() < capacity())
            return unchecked_emplace_back(std::forward<Args>(args)...);
        else
            intl::throw_out_of_capacity("inplace_vector::emplace_back");
    }

    template <class... Args>
    pointer try_emplace_back(Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args...>)
    {
        if (size() < capacity())
        {
            this->construct_one_at_end(std::forward<Args>(args)...);
            return base::end() - 1;
        }
        return nullptr;
    }

    template <class... Args>
    reference unchecked_emplace_back(Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args...>)
    {
        QX_ASSERT_CONTRACT(size() < capacity(), "inplace_vector::unchecked_emplace_back called on a full inplace_vector");
        this->construct_one_at_end(std::forward<Args>(args)...);
        return *(base::end() - 1);
    }

    // template <container - compatible - range<T> R>
    // constexpr void append_range(R&& rg); // C++23

    void pop_back() noexcept
    {
        QX_ASSERT_CONTRACT(!empty(), "inplace_vector::pop_back called on an empty inplace_vector");
        base::destroy_from_end(base::end() - 1);
    }

    template <class... Args>
    iterator emplace(const_iterator position, Args&&... args)
    {
        pointer p = base::begin() + (position - begin());
        if (size() < capacity())
        {
            if (p == base::end())
            {
                this->construct_one_at_end(std::forward<Args>(args)...);
            }
            else
            {
                value_type tmp(std::forward<Args>(args)...);
                this->move_range(p, base::end(), p + 1);
                *p = std::move(tmp);
            }
        }
        else
        {
            intl::throw_out_of_capacity("inplace_vector::emplace");
        }
        return iterator(p);
    }

    iterator insert(const_iterator position, const_reference x)
    {
        pointer p = base::begin() + (position - begin());
        if (size() < capacity())
        {
            if (p == base::end())
            {
                this->construct_one_at_end(x);
            }
            else
            {
                this->move_range(p, base::end(), p + 1);
                const_pointer xr = std::pointer_traits<const_pointer>::pointer_to(x);
                if (intl::is_pointer_in_range(p, base::end(), std::addressof(x)))
                    ++xr;
                *p = *xr;
            }
        }
        else
        {
            intl::throw_out_of_capacity("inplace_vector::insert");
        }
        return iterator(p);
    }

    iterator insert(const_iterator position, value_type&& x)
    {
        pointer p = base::begin() + (position - begin());
        if (size() < capacity())
        {
            if (p == base::end())
            {
                this->construct_one_at_end(std::move(x));
            }
            else
            {
                this->move_range(p, base::end(), p + 1);
                *p = std::move(x);
            }
        }
        else
        {
            intl::throw_out_of_capacity("inplace_vector::insert");
        }
        return iterator(p);
    }

    iterator insert(const_iterator position, size_type n, value_type const& x)
    {
        pointer p = base::begin() + (position - begin());
        if (n > 0)
        {
            if (n <= capacity() - size())
            {
                size_type old_n = n;
                pointer old_last = base::end();
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
                    if (p <= xr && xr < base::end())
                        xr += old_n;
                    std::fill_n(p, n, *xr);
                }
            }
            else
            {
                intl::throw_out_of_capacity("inplace_vector::insert");
            }
        }
        return iterator(p);
    }

    template <class InputIterator, std::enable_if_t<is_valid_iter_with_category_v<InputIterator, std::input_iterator_tag>, int> = 0>
    iterator insert(const_iterator position, InputIterator first, InputIterator last)
    {
        if constexpr (is_valid_iter_with_category_v<InputIterator, std::forward_iterator_tag>)
        {
            auto n = static_cast<size_type>(std::distance(first, last));
            return insert_with_size(position, std::move(first), std::move(last), n);
        }
        else
        {
            return insert_with_sentinel(position, std::move(first), std::move(last));
        }
    }

    iterator insert(const_iterator position, std::initializer_list<value_type> il) { return insert(position, il.begin(), il.end()); }

    iterator erase(const_iterator position)
    {
        QX_ASSERT_CONTRACT(position != end(), "inplace_vector::erase(iterator) called with a non-dereferenceable iterator");
        difference_type ps = position - cbegin();
        pointer p = base::begin() + ps;
        base::destroy_from_end(std::move(p + 1, base::end(), p));
        return iterator(p);
    }

    iterator erase(const_iterator first, const_iterator last)
    {
        QX_ASSERT_CONTRACT(first <= last, "inplace_vector::erase(first, last) called with invalid range");
        pointer p = base::begin() + (first - begin());
        if (first != last)
        {
            base::destroy_from_end(std::move(p + (last - first), base::end(), p));
        }
        return iterator(p);
    }

    void clear() noexcept { base::destroy_from_end(base::begin()); }

    void resize(size_type sz)
    {
        size_type const cs = size();
        if (cs < sz)
        {
            if (capacity() >= sz)
                this->construct_at_end(sz - cs);
            else
                intl::throw_out_of_capacity("inplace_vector::resize");
        }
        else if (cs > sz)
        {
            base::destroy_from_end(base::begin() + sz);
        }
    }

    void resize(size_type sz, value_type const& c)
    {
        size_type const cs = size();
        if (cs < sz)
        {
            if (capacity() >= sz)
                this->construct_at_end(sz - cs, c);
            else
                intl::throw_out_of_capacity("inplace_vector::resize");
        }
        else if (cs > sz)
        {
            base::destroy_from_end(base::begin() + sz);
        }
    }

    void swap(inplace_vector& other) noexcept(std::is_nothrow_swappable_v<T> && std::is_nothrow_move_constructible_v<T>)
    {
        if constexpr (N == 0)
            return;

        if (this != &other)
        {
            size_type const size1 = size();
            size_type const size2 = other.size();
            size_type const common = std::min(size1, size2);

            std::swap_ranges(begin(), begin() + common, other.begin());
            if (size1 > size2)
            {
                pointer new_end = std::uninitialized_move(begin() + common, begin() + size1, other.end());
                other.base::set_end(new_end);
                base::destroy_from_end(base::begin() + common);
            }
            else if (size2 > size1)
            {
                pointer new_end = std::uninitialized_move(other.base::begin() + common, other.base::begin() + size2, end());
                base::set_end(new_end);
                other.base::destroy_from_end(other.base::begin() + common);
            }
        }
    }

    friend void swap(inplace_vector& a, inplace_vector& b) noexcept(noexcept(a.swap(b))) { a.swap(b); }

    friend bool operator==(inplace_vector const& a, inplace_vector const& b)
    {
        return a.size() == b.size() && std::equal(a.begin(), a.end(), b.begin());
    }
    friend bool operator!=(inplace_vector const& a, inplace_vector const& b) { return !(a == b); }

    friend bool operator<(inplace_vector const& a, inplace_vector const& b)
    {
        return std::lexicographical_compare(a.begin(), a.end(), b.begin(), b.end());
    }
    friend bool operator>(inplace_vector const& a, inplace_vector const& b) { return b < a; }
    friend bool operator<=(inplace_vector const& a, inplace_vector const& b) { return !(b < a); }
    friend bool operator>=(inplace_vector const& a, inplace_vector const& b) { return !(a < b); }

private:
    template <class... Args>
    void construct_one_at_end(Args&&... args)
    {
        ::new (base::end()) value_type(std::forward<Args>(args)...);
        base::set_end(base::end() + 1);
    }

    void construct_at_end(size_type n)
    {
        pointer new_end = std::uninitialized_value_construct_n(base::end(), n);
        base::set_end(new_end);
    }

    void construct_at_end(size_type n, value_type const& x)
    {
        pointer new_end = std::uninitialized_fill_n(base::end(), n, x);
        base::set_end(new_end);
    }

    template <class InputIterator, class Sentinel>
    void construct_at_end(InputIterator first, Sentinel /* last */, size_type n)
    {
        pointer new_end = std::uninitialized_copy_n(std::move(first), n, base::end());
        base::set_end(new_end);
    }

    pointer move_range(pointer in_first, pointer in_last, pointer o_first)
    {
        if constexpr (std::is_trivially_copyable_v<T>)
        {
            pointer new_end = o_first + (in_last - in_first);
            (void)std::move_backward(in_first, in_last, new_end);
            base::set_end(new_end);
            return new_end;
        }

        pointer old_last = base::end();
        difference_type n = old_last - o_first;
        pointer i = in_first + n;

        auto new_end = std::uninitialized_move(i, in_last, old_last);
        base::set_end(new_end);

        (void)std::move_backward(in_first, i, old_last);
        return new_end;
    }

    template <class InputIterator, class Sentinel>
    void init_with_size(InputIterator first, Sentinel last, size_type n)
    {
        if (n > max_size())
            intl::throw_out_of_capacity("inplace_vector::inplace_vector");
        this->construct_at_end(std::move(first), std::move(last), n);
    }

    template <class InputIterator, class Sentinel>
    void init_with_sentinel(InputIterator first, Sentinel last)
    {
        for (; first != last; ++first)
            emplace_back(*first);
    }

    template <class InputIterator, class Sentinel>
    QX_CONSTEXPR_CXX20 void assign_with_size(InputIterator first, Sentinel last, difference_type n)
    {
        auto const new_size = static_cast<size_type>(n);
        if (new_size <= capacity())
        {
            if (new_size > size())
            {
                InputIterator mid = std::next(first, size());
                std::copy(first, mid, base::begin());
                this->construct_at_end(mid, last, new_size - size());
            }
            else
            {
                pointer m = std::copy(std::move(first), last, base::begin());
                base::destroy_from_end(m);
            }
        }
        else
        {
            intl::throw_out_of_capacity("inplace_vector::assign");
        }
    }

    template <class InputIterator, class Sentinel>
    void assign_with_sentinel(InputIterator first, Sentinel last)
    {
        pointer cur = base::begin();
        for (; first != last && cur != base::end(); ++first, (void)++cur)
            *cur = *first;

        if (cur != base::end())
        {
            base::destroy_from_end(cur);
        }
        else
        {
            for (; first != last; ++first)
                emplace_back(*first);
        }
    }

    template <class InputIterator, class Sentinel>
    iterator insert_with_sentinel(const_iterator position, InputIterator first, Sentinel last)
    {
        pointer p = base::begin() + (position - begin());
        pointer old_last = base::end();
        for (; size() != capacity() && first != last; ++first)
            this->construct_one_at_end(*first);

        (void)std::rotate(p, old_last, base::end());
        if (first != last)
            intl::throw_out_of_capacity("inplace_vector::insert");

        return iterator(p);
    }

    template <class Iterator, class Sentinel>
    iterator insert_with_size(const_iterator position, Iterator first, Sentinel last, difference_type n)
    {
        pointer p = base::begin() + (position - begin());
        if (n > 0)
        {
            if (static_cast<size_type>(n) <= capacity() - size())
            {
                pointer old_last = base::end();
                difference_type dx = base::end() - p;
                if (n > dx)
                {
                    Iterator m = std::next(first, dx);
                    this->construct_at_end(m, last, n - dx);
                    if (dx > 0)
                    {
                        this->move_range(p, old_last, p + n);
                        std::copy(first, m, p);
                    }
                }
                else
                {
                    this->move_range(p, old_last, p + n);
                    std::copy_n(first, n, p);
                }
            }
            else
            {
                intl::throw_out_of_capacity("inplace_vector::insert");
            }
        }
        return iterator(p);
    }
};

} // namespace qx