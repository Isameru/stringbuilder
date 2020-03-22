/*
  MIT License

  Copyright (c) 2017-2020 Mariusz Łapiński <gmail:isameru>

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.


    ███████╗████████╗██████╗ ██╗███╗   ██╗ ██████╗  .██████╗ ██╗   ██╗██╗██╗     ██████╗ ███████╗██████╗
    ██╔════╝╚══██╔══╝██╔══██╗██║████╗  ██║██╔════╝   ██╔══██╗██║   ██║██║██║     ██╔══██╗██╔════╝██╔══██╗
    ███████╗   ██║   ██████╔╝██║██╔██╗ ██║██║  ███╗-:██████╔╝██║   ██║██║██║     ██║  ██║█████╗  ██████╔╝
    ╚════██║   ██║   ██╔══██╗██║██║╚██╗██║██║   ██║  ██╔══██╗██║   ██║██║██║     ██║  ██║██╔══╝  ██╔══██╗
    ███████║   ██║   ██║  ██║██║██║ ╚████║╚██████╔╝ .██████╔╝╚██████╔╝██║███████╗██████╔╝███████╗██║  ██║
    ╚══════╝   ╚═╝   ╚═╝  ╚═╝╚═╝╚═╝  ╚═══╝ ╚═════╝   ╚═════╝  ╚═════╝ ╚═╝╚══════╝╚═════╝ ╚══════╝╚═╝  ╚═╝

        GitHub  : https://github.com/Isameru/stringbuilder
        Version : 1.1


    Release Notes
  :----------------
    Version 1.1 (2020-03)
        * C++11/C++14 support
        * Fresh CMake files
        * Doxygen, comments

    Version 1.0 (2017)
        * inplace_stringbuilder, stringbuilder, make_string, tests, benchmark
        * Performance optimizations
        * CMake, Travis, Appveyor


    TODO
  :-------
    * Formatters (aligned column, float)
    * More tests!
*/

#pragma once

#include <new>
#include <array>
#include <memory>
#include <string>
#include <numeric>
#include <utility>
#include <assert.h>
#include <type_traits>
#if defined(__has_include) && __has_include(<string_view>) && __cpp_lib_string_view
#include <string_view>
#define STRINGBUILDER_USES_STRING_VIEW      true
#else
#define STRINGBUILDER_USES_STRING_VIEW      false
#endif

#ifndef STRINGBUILDER_NAMESPACE
#define STRINGBUILDER_NAMESPACE             sbldr
#endif

//#ifndef STRINGBUILDER_SIZE_T
//#define STRINGBUILDER_SIZE_T                size_t
//#endif

#ifdef __GNUC__
#define STRINGBUILDER_LIKELY(x)             __builtin_expect(!!(x), 1)
#define STRINGBUILDER_UNLIKELY(x)           __builtin_expect(!!(x), 0)
#define STRINGBUILDER_NOINLINE              __attribute__((noinline))
#else
#define STRINGBUILDER_LIKELY(x)             (x)
#define STRINGBUILDER_UNLIKELY(x)           (x)
#define STRINGBUILDER_NOINLINE              __declspec(noinline)
#endif

#if defined(__has_include) && __has_include(<intrin.h>)
#include <intrin.h>
#endif

namespace STRINGBUILDER_NAMESPACE
{
    namespace detail
    {
        void prefetchWrite(const void* p)
        {
#if defined(__GNUC__)
            __builtin_prefetch(p, 1);
#elif defined(_WIN32)
            _mm_prefetch((const char*)p, _MM_HINT_T0);
#endif
        }
    }

    template<size_t ExpectedSize, typename StringT>
    struct sized_str_t
    {
        StringT str;
    };

#ifdef __cpp_decltype_auto
    template<size_t ExpectedSize, typename StringT>
    auto sized_str(StringT&& str)
    {
        return sized_str_t<ExpectedSize, StringT>{ std::forward<StringT>(str) };
    }
#endif


    // Appender is an utility class for encoding various kinds of objects (integers) and their propagation to stringbuilder or inplace_stringbuilder.
    //

    // Unless there are suitable converters, make use of to_string() to stringify the object.
    template<typename SB, typename T, typename Enable = void>
    struct sb_appender {
        void operator()(SB& sb, const T& v) const {
            using namespace std;
            sb.append(to_string(v));
        }
    };

    template<typename SB, size_t ExpectedSize, typename StringT>
    struct sb_appender<SB, sized_str_t<ExpectedSize, StringT>>
    {
        void operator()(SB& sb, const sized_str_t<ExpectedSize, StringT>& sizedStr) const
        {
            sb.append(sizedStr.str);
        }
    };

    template<typename SB>
    struct sb_appender<SB, const typename SB::char_type*>
    {
        void operator()(SB& sb, const typename SB::char_type* str) const
        {
            sb.append_c_str(str);
        }
    };


    /// Compile-time switch which controls the behaiovr of inplace_stringbuilder upon buffer overflow.
    ///
    enum class inplace_stringbuilder_overflow_polcy {
        assert,             // Debug: assertion failure | Release: like corrupt_memory
        corrupt_memory,     // Overwrite the memory beyond the buffer (not recommended)
        early_exception,    // Throw inplace_stringbuilder_early_overflow_error exception while the content remains unaltered
        late_exception,     // Throw inplace_stringbuilder_late_overflow_error expection after the content is filled up to the maximal capacity
        protect             // Fill up the content up to the maximal capacity, while the rest is ignored (clipped)
    };

    /// Exception raised whenever overflow occurs in an inplace_stringbuilder with overflow policy set to early_exception or later_exception.
    ///
    struct inplace_stringbuilder_overflow_error : public std::overflow_error {
        inplace_stringbuilder_overflow_error() :
            std::overflow_error{ "inplace_stringbuilder overflow" }
        { }
    };

    /// Exception raised whenever overflow occurs in an inplace_stringbuilder with overflow policy set to early_exception.
    ///
    struct inplace_stringbuilder_early_overflow_error : public inplace_stringbuilder_overflow_error {};

    /// Exception raised whenever overflow occurs in an inplace_stringbuilder with overflow policy set to later_exception.
    ///
    struct inplace_stringbuilder_late_overflow_error : public inplace_stringbuilder_overflow_error {};

    /// Provides means for building size-delimited strings in-situ (without heap allocations).
    /// Object of this class occupies fixed size (specified at compile-time) and allows appending portions of strings unless there is space available.
    /// In debug mode appending ensures that the built string does not exceed the available space.
    /// In release mode no such checks are made thus dangerous memory corruption may occur if used incorrectly.
    ///
    template<
        typename CharT,
        size_t MaxSize,
        bool Forward = true,
        typename Traits = std::char_traits<CharT>,
        inplace_stringbuilder_overflow_polcy OverflowPolicy = inplace_stringbuilder_overflow_polcy::assert
    >
        class basic_inplace_stringbuilder
    {
        static_assert(MaxSize > 0, "MaxSize must be greater than zero");

    public:
        using traits_type = Traits;
        using char_type = CharT;
        using value_type = char_type;
        using size_type = size_t;
        using difference_type = std::ptrdiff_t;
        using reference = char_type&;
        using const_reference = const char_type&;
        using pointer = char_type*;
        using const_pointer = const char_type*;
        static constexpr bool forward = Forward;
        static constexpr inplace_stringbuilder_overflow_polcy overflow_policy = OverflowPolicy;
        /// Indicates whether the append*() function calls may throw.
        /// This may happen upon character buffer overflow when the overflow policy is set to either early_exception or late_exception.
        static constexpr bool append_may_not_throw = overflow_policy != inplace_stringbuilder_overflow_polcy::early_exception && overflow_policy != inplace_stringbuilder_overflow_polcy::late_exception;

        /// Gets the number of characters appended to the buffer.
        size_type size() const noexcept { return consumed; }
        /// Gets the number of characters appended to the buffer.
        size_type length() const noexcept { return size(); }
        /// Gets the number of characters which can be safely appended to the buffer (before overflow happens).
        size_type space_left() const noexcept { return MaxSize - consumed; }

        /// Appends a single character to the buffer.
        basic_inplace_stringbuilder& append(char_type ch)
            //noexcept(append_may_not_throw)
        {
            assert(ch != '\0' && "Cannot append a null-termination character, as it may lead to undesirable effects");

            switch (overflow_policy) {
                case inplace_stringbuilder_overflow_polcy::assert:
                    assert(consumed < MaxSize);
                    break;
                case inplace_stringbuilder_overflow_polcy::corrupt_memory:
                    break;
                case inplace_stringbuilder_overflow_polcy::early_exception:
                    if (consumed >= MaxSize)
                        throw inplace_stringbuilder_early_overflow_error{};
                    break;
                case inplace_stringbuilder_overflow_polcy::late_exception:
                    if (consumed >= MaxSize)
                        throw inplace_stringbuilder_late_overflow_error{};
                    break;
                case inplace_stringbuilder_overflow_polcy::protect:
                    if (consumed >= MaxSize)
                        return *this;
                    break;
            }

            if (Forward) {
                data_[consumed++] = ch;
            }
            else {
                data_[MaxSize - (++consumed)] = ch;
            }
            return *this;
        }

        /// Appends the same character specified number of times.
        basic_inplace_stringbuilder& append(size_type count, char_type ch)
            //noexcept(append_may_not_throw)
        {
            assert(ch != '\0' && "Cannot append a null-termination character (\\0), as it may lead to undesirable effects");

            bool throw_late_exception;
            switch (overflow_policy) {
                case inplace_stringbuilder_overflow_polcy::assert:
                    assert(consumed + count <= MaxSize);
                    break;
                case inplace_stringbuilder_overflow_polcy::corrupt_memory:
                    break;
                case inplace_stringbuilder_overflow_polcy::early_exception:
                    if (consumed + count > MaxSize)
                        throw inplace_stringbuilder_early_overflow_error{};
                    break;
                case inplace_stringbuilder_overflow_polcy::late_exception: {
                    auto original_count = count;
                    count = std::min(space_left(), count);
                    throw_late_exception = (original_count != count);
                    break;
                }
                case inplace_stringbuilder_overflow_polcy::protect:
                    count = std::min(space_left(), count);
                    break;
            }

            if (Forward) {
                while (count-- > 0) data_[consumed++] = ch;
            }
            else {
                while (count-- > 0) data_[MaxSize - (++consumed)] = ch;
            }

            if (overflow_policy == inplace_stringbuilder_overflow_polcy::late_exception) {
                if (throw_late_exception)
                    throw inplace_stringbuilder_late_overflow_error{};
            }

            return *this;
        }

        /// Appends a string literal, i.e. an array of characters without the last element, which is expected to be a trailing null-termination character.
        template<size_type StrSizeWith0>
        basic_inplace_stringbuilder& append(const char_type(&str)[StrSizeWith0])
            //noexcept(append_may_not_throw)
        {
            assert(str[StrSizeWith0-1] == 0);
            return append(str, StrSizeWith0-1);
        }

        /// Appends an array of characters.
        template<size_type N>
        basic_inplace_stringbuilder& append(const std::array<char_type, N>& arr)
            //noexcept(append_may_not_throw)
        {
            return append(arr.data(), N);
        }

        /// Appends the specified number of characters of a given C-style string.
        /// It is up to the user to ensure that the specified sized string is valid.
        basic_inplace_stringbuilder& append(const char_type* str, size_type size)
            //noexcept(append_may_not_throw)
        {
            bool throw_late_exception;
            switch (overflow_policy) {
                case inplace_stringbuilder_overflow_polcy::assert:
                    assert(consumed + size <= MaxSize);
                    break;
                case inplace_stringbuilder_overflow_polcy::corrupt_memory:
                    break;
                case inplace_stringbuilder_overflow_polcy::early_exception:
                    if (consumed + size > MaxSize)
                        throw inplace_stringbuilder_early_overflow_error{};
                    break;
                case inplace_stringbuilder_overflow_polcy::late_exception: {
                    auto original_size = size;
                    size = std::min(space_left(), size);
                    throw_late_exception = (original_size != size);
                    break;
                }
                case inplace_stringbuilder_overflow_polcy::protect:
                    size = std::min(space_left(), size);
                    break;
            }

            if (Forward) {
                Traits::copy(data_.data() + consumed, str, size);
            }
            else {
                Traits::copy(data_.data() + MaxSize - size - consumed, str, size);
            }
            consumed += size;

            if (overflow_policy == inplace_stringbuilder_overflow_polcy::late_exception) {
                if (throw_late_exception)
                    throw inplace_stringbuilder_late_overflow_error{};
            }

            return *this;
        }

        /// Appends the specified number of characters of a given C-style string.
        /// It is up to the user to ensure that the specified sized string is valid.
        basic_inplace_stringbuilder& append_c_str(const char_type* str, size_type size)
            //noexcept(append_may_not_throw)
        {
            return append(str, size);
        }

        /// Appends the null-terminated C-style string.
        basic_inplace_stringbuilder& append_c_str(const char_type* str)
            noexcept(append_may_not_throw)
        {
            return append(str, Traits::length(str));
        }

        /// Appends the null-terminated C-style string, copying the characters one-by-one.
        /// This method may be slightly faster than append_c_str() for small strings (up to 4 characters), whose size is unknown a priori.
        /// This is not a recommended approach, unless it proves to give a visible performance gain in your benchmark.
        basic_inplace_stringbuilder& append_c_str_progressive(const char_type* str)
            //noexcept(append_may_not_throw)
        {
            assert(Forward && "Progressive append is supported only in Forward mode");
            while (*str != 0)
            {
                switch (overflow_policy) {
                    case inplace_stringbuilder_overflow_polcy::assert:
                        assert(consumed < MaxSize);
                        break;
                    case inplace_stringbuilder_overflow_polcy::corrupt_memory:
                        break;
                    case inplace_stringbuilder_overflow_polcy::early_exception:
                        // Progressive append cannot detect the overflow beforehand, so it works like late_exception.
                        if (consumed >= MaxSize)
                            throw inplace_stringbuilder_early_overflow_error{};
                        break;
                    case inplace_stringbuilder_overflow_polcy::late_exception:
                        if (consumed >= MaxSize)
                            throw inplace_stringbuilder_early_overflow_error{};
                        break;
                    case inplace_stringbuilder_overflow_polcy::protect:
                        if (consumed >= MaxSize)
                            return *this;
                        break;
                }

                data_[consumed++] = *(str++);
            }
            return *this;
        }

        /// Appends a string.
        template<typename OtherTraits, typename OtherAlloc>
        basic_inplace_stringbuilder& append(const std::basic_string<char_type, OtherTraits, OtherAlloc>& str)
            //noexcept(append_may_not_throw)
        {
            return append(str.data(), str.size());
        }

#if STRINGBUILDER_USES_STRING_VIEW
        /// Appends a string view.
        template<typename OtherTraits>
        basic_inplace_stringbuilder& append(const std::basic_string_view<char_type, OtherTraits>& sv)
            //noexcept(append_may_not_throw)
        {
            return append(sv.data(), sv.size());
        }
#endif

        /// Appends an in-place string builder.
        template<size_type OtherMaxSize, bool OtherForward, typename OtherTraits>
        basic_inplace_stringbuilder& append(const basic_inplace_stringbuilder<char_type, OtherMaxSize, OtherForward, OtherTraits>& sb)
            //noexcept(append_may_not_throw)
        {
            return append_c_str(sb.data(), sb.size());
        }

        /// Appends an "any" object using a type-deduced formatter.
        template<typename T>
        basic_inplace_stringbuilder& append(const T& v)
        {
            sb_appender<basic_inplace_stringbuilder, T>{}(*this, v);
            return *this;
        }

        /// Appends an "any" object using a type-deduced formatter.
        template<typename AnyT>
        basic_inplace_stringbuilder& operator<<(AnyT&& any)
        {
            return append(std::forward<AnyT>(any));
        }

        /// Appends multiple "any" objects using type-deduced formatters.
        template<typename AnyT>
        basic_inplace_stringbuilder& append_many(AnyT&& any)
        {
            return append(std::forward<AnyT>(any));
        }

        /// Appends multiple "any" objects using type-deduced formatters.
        template<typename AnyT1, typename... AnyTX>
        basic_inplace_stringbuilder& append_many(AnyT1&& any1, AnyTX&&... anyX)
        {
            return append(std::forward<AnyT1>(any1)).append_many(std::forward<AnyTX>(anyX)...);
        }

        /// Gets the pointer to the first character in the internal buffer.
        /// The retrieved string is not null-terminated.
        char_type* data() noexcept
        {
            return data_.data() + (Forward ? 0 : MaxSize - consumed);
        }

        /// Gets the const-pointer to the first character in the internal buffer.
        /// The retrieved string is not null-terminated.
        const char_type* data() const noexcept
        {
            return data_.data() + (Forward ? 0 : MaxSize - consumed);
        }

        /// Gets the pointer to the beginning of a null-terminated C-style string.
        const char_type* c_str() const noexcept
        {
            // Placing '\0' at the end of string is a kind of lazy evaluation and is acceptable also for const objects.
            const_cast<basic_inplace_stringbuilder*>(this)->placeNullTerminator();
            return data();
        }

        /// Creates and returns a string object containing a copy of all appended characters.
        std::basic_string<char_type> str() const
        {
            assert(consumed <= MaxSize);
            const auto b = data_.cbegin();
            if /*constexpr*/ (Forward) {
                return { b, b + consumed };
            }
            else {
                return { b + MaxSize - consumed, b + MaxSize };
            }
        }

#if STRINGBUILDER_USES_STRING_VIEW
        /// Returns a string_view spanning over all appended characters.
        /// The retrieved string is not null-terminated.
        std::basic_string_view<char_type, traits_type> str_view() const noexcept
        {
            return { data(), size() };
        }
#endif

        /// Prints the content to the output stream.
        template<typename OtherCharTraitsT>
        friend std::basic_ostream<char_type, OtherCharTraitsT>& operator<<(
            std::basic_ostream<char_type, OtherCharTraitsT>& out,
            const basic_inplace_stringbuilder<char_type, MaxSize, forward, traits_type, overflow_policy>& sb)
        {
            return out.write(sb.data(), static_cast<std::streamsize>(sb.size()));
        }

    private:
        /// Puts a null-termination character ('\0') after the last appended character.
        void placeNullTerminator() const noexcept
        {
            assert(consumed <= MaxSize);
            const_cast<char_type&>(data_[Forward ? consumed : MaxSize]) = '\0';
        }

    private:
        /// Number of characters appended to the buffer.
        size_type consumed = 0;
        /// In-situ internal character buffer.
        std::array<char_type, MaxSize + 1> data_;  // Last character is reserved for '\0'.
    };


    namespace detail
    {
        // The following code is based on Empty Base Optimization Helper (ebo_helper) explained in this talk:
        // https://youtu.be/hHQS-Q7aMzg?t=3039 [code::dive 2016 conference – Mateusz Pusz – std::shared_ptr/T/]
        // This technique achieves what [[no_unique_address]] attribute (available since C++20) does.
        //
        template<typename OrigAlloc, bool UseEbo =
#if __cpp_lib_is_final
            !std::is_final<OrigAlloc>::value&& std::is_empty<OrigAlloc>::value
#else
            false
#endif
        >
            struct raw_alloc_provider;

        template<typename OrigAlloc>
        struct raw_alloc_provider<OrigAlloc, true> :
            private std::allocator_traits<OrigAlloc>::template rebind_alloc<uint8_t>
        {
            using AllocRebound = typename std::allocator_traits<OrigAlloc>::template rebind_alloc<uint8_t>;

            template<typename OtherAlloc> constexpr explicit raw_alloc_provider(OtherAlloc&& otherAlloc) : AllocRebound{ std::forward<AllocRebound>(otherAlloc) } {}
            AllocRebound& get_rebound_allocator() { return *this; }
            OrigAlloc get_original_allocator() const { return OrigAlloc{ *this }; }
        };

        template<typename OrigAlloc>
        struct raw_alloc_provider<OrigAlloc, false>
        {
            using AllocRebound = typename std::allocator_traits<OrigAlloc>::template rebind_alloc<uint8_t>;

            template<typename OtherAlloc> constexpr explicit raw_alloc_provider(OtherAlloc&& otherAlloc) : alloc_rebound{ std::forward<AllocRebound>(otherAlloc) } {}
            AllocRebound& get_rebound_allocator() { return alloc_rebound; }
            OrigAlloc get_original_allocator() const { return OrigAlloc{ alloc_rebound }; }

        private:
            mutable AllocRebound alloc_rebound;
        };


        template<typename CharT>
        struct Chunk;

        template<typename CharT>
        struct ChunkHeader
        {
            Chunk<CharT>* next;
            size_t consumed;
            size_t reserved;
        };

        template<typename CharT>
        struct Chunk : public ChunkHeader<CharT>
        {
            CharT data[1]; // In practice there are ChunkHeader::reserved of characters in this array.

            Chunk(size_t reserve) : ChunkHeader<CharT>{nullptr, size_t{0}, reserve} { }
        };

        template<typename CharT, int DataLength>
        struct ChunkInPlace : public ChunkHeader<CharT>
        {
            std::array<CharT, DataLength> data;

            ChunkInPlace() : ChunkHeader<CharT>{nullptr, 0, DataLength} { }
        };

        template<typename CharT>
        struct ChunkInPlace<CharT, 0> : public ChunkHeader<CharT>
        {
            ChunkInPlace() : ChunkHeader<CharT>{nullptr, 0, 0} { }
        };
    }


    /// Provides means for efficient construction of strings.
    /// Object of this class occupies fixed size (specified at compile-time) and allows appending portions of strings.
    /// If the available space gets exhausted, new chunks of memory are allocated on the heap.
    ///
    template<typename Char,
        size_t InPlaceSize,
        typename Traits,
        typename AllocOrig>
        class basic_stringbuilder : private detail::raw_alloc_provider<AllocOrig>
    {
        using AllocProvider = detail::raw_alloc_provider<AllocOrig>;
        using Alloc = typename AllocProvider::AllocRebound;
        using AllocTraits = std::allocator_traits<Alloc>;

    public:
        using traits_type = Traits;
        using char_type = Char;
        using value_type = char_type;
        using allocator_type = AllocOrig;
        using size_type = size_t;
        using difference_type = std::ptrdiff_t;
        using reference = char_type&;
        using const_reference = const char_type&;
        using pointer = char_type*;
        using const_pointer = const char_type*;
        static constexpr size_t inplace_size = InPlaceSize;

    private:
        using Chunk = detail::Chunk<char_type>;
        using ChunkHeader = detail::ChunkHeader<char_type>;
        template<int N> using ChunkInPlace = detail::ChunkInPlace<char_type, N>;

    public:
        /// Recreates and returns the allocator originally passed to stringbuilder object during construction (it is kept internally in a rebound form).
        AllocOrig get_allocator() const noexcept { return AllocProvider::get_original_allocator(); }

        template<typename AllocOther = Alloc>
        basic_stringbuilder(AllocOther&& allocOther = AllocOther{}) noexcept : AllocProvider{std::forward<AllocOther>(allocOther)} {}

        basic_stringbuilder(const basic_stringbuilder&) = delete;

        basic_stringbuilder(basic_stringbuilder&& other) noexcept :
            AllocProvider{other.get_allocator()},
            headChunkInPlace{other.headChunkInPlace},
            tailChunk{other.tailChunk}
        {
            other.headChunkInPlace.next = nullptr;
        }

        ~basic_stringbuilder()
        {
            Chunk* nextChunk = headChunk()->next;
            for (auto chunk = nextChunk; chunk != nullptr; chunk = nextChunk)
            {
                nextChunk = chunk->next;
                //AllocTraits::destroy...?
                AllocTraits::deallocate(AllocProvider::get_rebound_allocator(), reinterpret_cast<typename AllocTraits::pointer>(chunk), sizeof(ChunkHeader) + chunk->reserved);
            }
        }

        /// Gets the number of characters appended to the buffer.
        size_type size() const noexcept
        {
            size_type size = 0;
            const auto* chunk = headChunk();
            do {
                size += chunk->consumed;
                chunk = chunk->next;
            } while (chunk != nullptr);
            return size;
        }

        /// Gets the number of characters appended to the buffer.
        size_type length() const noexcept { return size(); }

        void reserve(size_type size)
        {
            for (Chunk* chunk = tailChunk; size > chunk->reserved - chunk->consumed; chunk = chunk->next)
            {
                size -= chunk->reserved - chunk->consumed;
                assert(size > 0);
                if (chunk->next == nullptr) {
                    chunk->next = allocChunk(size);
                }
            }
        }

        /// Appends a single character to the buffer.
        basic_stringbuilder& append(char_type ch)
        {
            assert(ch != '\0');
            claimOne() = ch;
            return *this;
        }

        /// Appends the same character specified number of times.
        basic_stringbuilder& append(size_type count, char_type ch)
        {
            assert(ch != '\0');
            for (auto left = count; left > 0;) {
                const auto claimed = claim(1, left);
                for (size_type i = 0; i < claimed.second; ++i) {
                    claimed.first[i] = ch;
                }
                left -= claimed.second;
            }
            return *this;
        }

        /// Appends a string literal, i.e. an array of characters without the last element, which is expected to be a trailing null-termination character.
        template<size_type StrSizeWith0>
        basic_stringbuilder& append(const char_type(&str)[StrSizeWith0])
        {
            assert(str[StrSizeWith0-1] == 0);
            return append(str, StrSizeWith0-1);
        }

        /// Appends an array of characters.
        template<size_type N>
        basic_stringbuilder& append(const std::array<char_type, N>& arr)
        {
            return append(arr.data(), N);
        }

        /// Appends the specified number of characters of a given C-style string.
        /// It is up to the user to ensure that the specified sized string is valid.
        basic_stringbuilder& append(const char_type* str, size_type size)
        {
            Traits::copy(claim(size), str, size);
            return *this;
        }

        /// Appends the specified number of characters of a given C-style string.
        /// It is up to the user to ensure that the specified sized string is valid.
        basic_stringbuilder& append_c_str(const char_type* str, size_type size)
        {
            return append(str, size);
        }

        /// Appends the null-terminated C-style string.
        template<bool Prefetch = false>
        basic_stringbuilder& append_c_str(const char_type* str)
        {
            if (Prefetch) detail::prefetchWrite(&tailChunk->data[tailChunk->consumed]);
            return append(str, Traits::length(str));
        }

        /// Appends the null-terminated C-style string, copying the characters one-by-one.
        /// This method may be slightly faster than append_c_str() for small strings (up to 4 characters), whose size is unknown a priori.
        /// This is not a recommended approach, unless it proves to give a visible performance gain in your benchmark.
        basic_stringbuilder& append_c_str_progressive(const char_type* str)
        {
            while (true) {
                auto claimed = claim(1, 64);
                auto dst = claimed.first;
                const auto dstEnd = dst + claimed.second;
                while (dst < dstEnd) {
                    if (*str == 0) {
                        reclaim(dstEnd - dst);
                        return *this;
                    }
                    *(dst++) = *(str++);
                }
            }
            assert(false);
            return *this;
        }

        /// Appends a string.
        template<typename OtherTraits, typename OtherAlloc>
        basic_stringbuilder& append(const std::basic_string<char_type, OtherTraits, OtherAlloc>& str)
        {
            return append(str.data(), str.size());
        }

#if STRINGBUILDER_USES_STRING_VIEW
        /// Appends a string view.
        template<typename OtherTraits>
        basic_stringbuilder& append(const std::basic_string_view<char_type, OtherTraits>& sv)
        {
            return append(sv.data(), sv.size());
        }
#endif

        /// Appends an in-place string builder.
        template<size_type OtherMaxSize, bool OtherForward, typename OtherTraits>
        basic_stringbuilder& append(const basic_inplace_stringbuilder<char_type, OtherMaxSize, OtherForward, OtherTraits>& sb)
        {
            return append(sb.data(), sb.size());
        }

        /// Appends a string builder.
        template<size_type OtherInPlaceSize, typename OtherTraits, typename OtherAlloc>
        basic_stringbuilder& append(const basic_stringbuilder<char_type, OtherInPlaceSize, OtherTraits, OtherAlloc>& sb)
        {
            size_type size = sb.size();
            reserve(size);

            const Chunk* chunk = sb.headChunk();
            while (size > 0) {
                assert(chunk != nullptr);
                const size_type toCopy = std::min(size, chunk->consumed);
                append(chunk->data, toCopy);
                size -= toCopy;
                chunk = chunk->next;
            }
            return *this;
        }

        /// Appends an "any" object using a type-deduced formatter.
        template<typename T>
        basic_stringbuilder& append(const T& v)
        {
            sb_appender<basic_stringbuilder, T>{}(*this, v);
            return *this;
        }

        /// Appends an "any" object using a type-deduced formatter.
        template<typename AnyT>
        basic_stringbuilder& operator<<(AnyT&& any)
        {
            return append(std::forward<AnyT>(any));
        }

        /// Appends multiple "any" objects using type-deduced formatters.
        template<typename AnyT>
        basic_stringbuilder& append_many(AnyT&& any)
        {
            return append(std::forward<AnyT>(any));
        }

        /// Appends multiple "any" objects using type-deduced formatters.
        template<typename AnyT1, typename... AnyTX>
        basic_stringbuilder& append_many(AnyT1&& any1, AnyTX&&... anyX)
        {
            return append(std::forward<AnyT1>(any1)).append_many(std::forward<AnyTX>(anyX)...);
        }

        /// Creates and returns a string object containing a copy of all appended characters.
        std::basic_string<char_type> str() const
        {
            const auto size0 = size();
            auto str = std::basic_string<char_type>{};
            str.reserve(size0);
            for (const Chunk* chunk = headChunk(); chunk != nullptr; chunk = chunk->next)
            {
                str.append(chunk->data, chunk->consumed);
            }
            return str;
        }

        /// Checks whether the contained (valid) characters form a linear buffer in memory.
        bool is_linear() const
        {
            const auto* chunk = headChunk();
            bool has_data = chunk->consumed > 0;
            while (chunk->next) {
                chunk = chunk->next;
                if (chunk->consumed > 0) {
                    if (has_data)
                        return false;
                    has_data = true;
                }
            }
            return true;
        }

#if STRINGBUILDER_USES_STRING_VIEW
        /// Returns a string_view spanning over all appended characters.
        /// The retrieved string is not null-terminated.
        std::basic_string_view<char_type> str_view() const
        {
            assert(is_linear());
            const auto* chunk = headChunk();
            while (chunk->consumed == 0) {
                chunk = chunk->next;
                assert(chunk != nullptr);
            }
            return { chunk->data, chunk->consumed };
        }
#endif

        /// Prints the content to the output stream.
        /// There may be multiple writes to ostream.
        template<typename OtherCharTraitsT>
        friend std::basic_ostream<char_type, OtherCharTraitsT>& operator<<(
            std::basic_ostream<char_type, OtherCharTraitsT>& out,
            const basic_stringbuilder<char_type, inplace_size, traits_type, allocator_type>& sb)
        {
            for (const Chunk* chunk = sb.headChunk(); chunk != nullptr; chunk = chunk->next)
            {
                out.write(chunk->data, static_cast<std::streamsize>(chunk->consumed));
            }
            return out;
        }

    private:
        Chunk* headChunk() noexcept { return reinterpret_cast<Chunk*>(&headChunkInPlace); }
        const Chunk* headChunk() const noexcept { return reinterpret_cast<const Chunk*>(&headChunkInPlace); }

        char_type* claim(size_type exact)
        {
            if (STRINGBUILDER_UNLIKELY(tailChunk->reserved - tailChunk->consumed < exact))
                prepareSpace(exact);

            char_type* const claimedChars = &tailChunk->data[tailChunk->consumed];
            tailChunk->consumed += exact;
            return claimedChars;
        }

        std::pair<char_type*, size_type> claim(size_type minimum, size_type maximum)
        {
            assert(maximum >= minimum);
            assert(tailChunk->reserved >= tailChunk->consumed);

            if (STRINGBUILDER_UNLIKELY(tailChunk->reserved - tailChunk->consumed < minimum))
                prepareSpace(minimum, maximum);

            assert(tailChunk->reserved >= tailChunk->consumed);
            assert(minimum <= tailChunk->reserved - tailChunk->consumed);

            const size_type claimedSize = std::min(maximum, tailChunk->reserved - tailChunk->consumed);
            const auto claimed = std::make_pair(&tailChunk->data[tailChunk->consumed], claimedSize);
            tailChunk->consumed += claimedSize;
            return claimed;
        }

        Char& claimOne()
        {
            if (STRINGBUILDER_UNLIKELY(tailChunk->reserved - tailChunk->consumed < 1))
                prepareSpace(1);
            return tailChunk->data[tailChunk->consumed++];
        }

        void reclaim(size_t exact)
        {
            assert(tailChunk->consumed >= exact);
            tailChunk->consumed -= exact;
        }

        STRINGBUILDER_NOINLINE void prepareSpace(size_type minimum)
        {
            if (tailChunk->next == nullptr) {
                tailChunk->next = allocChunk(minimum);
                tailChunk = tailChunk->next;
            }
            else {
                while (true) {
                    tailChunk = tailChunk->next;
                    assert(tailChunk->consumed == 0);
                    if (tailChunk->reserved >= minimum)
                        return;

                    if (tailChunk->next == nullptr)
                        tailChunk->next = allocChunk(minimum);
                }
            }
        }

        STRINGBUILDER_NOINLINE void prepareSpace(size_type minimum, size_type maximum)
        {
            if (tailChunk->next == nullptr) {
                tailChunk->next = allocChunk(maximum);
                tailChunk = tailChunk->next;
            }
            else
            {
                while (true) {
                    tailChunk = tailChunk->next;
                    assert(tailChunk->consumed == 0);

                    if (tailChunk->reserved >= minimum)
                        return;

                    if (tailChunk->next == nullptr)
                        tailChunk->next = allocChunk(maximum);
                }
            }
        }

        size_type determineNextChunkSize(size_type minimum) const noexcept { return std::max(2 * tailChunk->reserved, minimum); }

        constexpr static size_type l1DataCacheLineSize = 64; //std::hardware_destructive_interference_size;

        constexpr static size_type roundToL1DataCacheLine(size_type size) noexcept
        {
            return ((l1DataCacheLineSize - 1) + size) / l1DataCacheLineSize * l1DataCacheLineSize;
        }

        Chunk* allocChunk(size_type minimum)
        {
            assert(minimum > 0);
            const auto chunkTotalSize = roundToL1DataCacheLine(determineNextChunkSize(minimum) + sizeof(ChunkHeader));
            auto* rawChunk = AllocTraits::allocate(AllocProvider::get_rebound_allocator(), chunkTotalSize, tailChunk);
            auto* chunk = reinterpret_cast<Chunk*>(rawChunk);
            AllocTraits::construct(AllocProvider::get_rebound_allocator(), chunk, chunkTotalSize - sizeof(ChunkHeader));
            return chunk;
        }

    private:
        ChunkInPlace<InPlaceSize> headChunkInPlace;
        Chunk* tailChunk = headChunk();
    };

} // namespace STRINGBUILDER_NAMESPACE

namespace std
{
    template<typename CharT, size_t InPlaceSize, bool Forward, typename Traits>
    inline std::basic_string<CharT> to_string(const STRINGBUILDER_NAMESPACE::basic_inplace_stringbuilder<CharT, InPlaceSize, Forward, Traits>& sb)
    {
        return sb.str();
    }

    template<typename CharT, size_t InPlaceSize, typename Traits, typename Alloc>
    inline std::basic_string<CharT> to_string(const STRINGBUILDER_NAMESPACE::basic_stringbuilder<CharT, InPlaceSize, Traits, Alloc>& sb)
    {
        return sb.str();
    }
}


namespace STRINGBUILDER_NAMESPACE
{
    template<int MaxSize, bool Forward = true, typename Traits = std::char_traits<char>, inplace_stringbuilder_overflow_polcy OverflowPolicy = inplace_stringbuilder_overflow_polcy::assert>
    using inplace_stringbuilder = basic_inplace_stringbuilder<char, MaxSize, Forward, Traits, OverflowPolicy>;

    template<int MaxSize, bool Forward = true, typename Traits = std::char_traits<wchar_t>, inplace_stringbuilder_overflow_polcy OverflowPolicy = inplace_stringbuilder_overflow_polcy::assert>
    using inplace_wstringbuilder = basic_inplace_stringbuilder<wchar_t, MaxSize, Forward, Traits, OverflowPolicy>;

    template<int MaxSize, bool Forward = true, typename Traits = std::char_traits<char16_t>, inplace_stringbuilder_overflow_polcy OverflowPolicy = inplace_stringbuilder_overflow_polcy::assert>
    using inplace_u16stringbuilder = basic_inplace_stringbuilder<char16_t, MaxSize, Forward, Traits, OverflowPolicy>;

    template<int MaxSize, bool Forward = true, typename Traits = std::char_traits<char32_t>, inplace_stringbuilder_overflow_polcy OverflowPolicy = inplace_stringbuilder_overflow_polcy::assert>
    using inplace_u32stringbuilder = basic_inplace_stringbuilder<char32_t, MaxSize, Forward, Traits, OverflowPolicy>;


    template<int InPlaceSize = 0, typename Traits = std::char_traits<char>, typename Alloc = std::allocator<char>>
    using stringbuilder = basic_stringbuilder<char, InPlaceSize, Traits, Alloc>;

    template<int InPlaceSize = 0, typename Traits = std::char_traits<wchar_t>, typename Alloc = std::allocator<wchar_t>>
    using wstringbuilder = basic_stringbuilder<wchar_t, InPlaceSize, Traits, Alloc>;

    template<int InPlaceSize = 0, typename Traits = std::char_traits<char16_t>, typename Alloc = std::allocator<char16_t>>
    using u16stringbuilder = basic_stringbuilder<char16_t, InPlaceSize, Traits, Alloc>;

    template<int InPlaceSize = 0, typename Traits = std::char_traits<char32_t>, typename Alloc = std::allocator<char32_t>>
    using u32wstringbuilder = basic_stringbuilder<char32_t, InPlaceSize, Traits, Alloc>;


    template<typename SB, typename IntegerT>
    struct sb_appender<SB, IntegerT, typename std::enable_if<
        std::is_integral<IntegerT>::value && !::std::is_same<IntegerT, typename SB::char_type>::value >::type>
    {
        void operator()(SB& sb, IntegerT iv) const
        {
            // In this particular case, std::div() is x2 slower instead of / and %.
            if (iv >= 0) {
                if (iv >= 10) {
                    basic_inplace_stringbuilder<typename SB::char_type, 20, false, typename SB::traits_type> bss;
                    do {
                        bss.append(static_cast<typename SB::char_type>('0' + iv % 10));
                        iv /= 10;
                    } while (iv > 0);
                    sb.append(bss);
                }
                else {
                    sb.append(static_cast<typename SB::char_type>('0' + static_cast<char>(iv)));
                }
            }
            else {
                if (iv <= -10) {
                    basic_inplace_stringbuilder<typename SB::char_type, 20, false, typename SB::traits_type> bss;
                    do {
                        bss.append(static_cast<typename SB::char_type>('0' - iv % 10));
                        iv /= 10;
                    } while (iv < 0);
                    bss.append('-');
                    sb.append(bss);
                }
                else {
                    sb.append('-');
                    sb.append(static_cast<typename SB::char_type>('0' - static_cast<char>(iv)));
                }
            }
        }
    };


#if __cpp_lib_integer_sequence && __cpp_lib_void_t
#define STRINGBUILDER_SUPPORTS_MAKE_STRING

    template<typename CharT, size_t N, size_t... IX>
    struct constexpr_str
    {
    private:
        const std::array<CharT, N> arr;
        const CharT c_str_[N];

    public:
        constexpr constexpr_str(const std::array<CharT, N> arr_, const std::index_sequence<IX...>) :
            arr{arr_},
            c_str_{ arr_[IX]... }
        { }

        constexpr size_t size() const { return N - 1; }
        constexpr const CharT* c_str() const { return c_str_; }
        constexpr auto str() const { return std::string(c_str_, size()); }
    };


    namespace detail
    {
        template<typename T>
        struct type { using value_type = T; };

        template<typename CharT>
        constexpr int estimateTypeSize(type<CharT>) {
            return 1;
        }

        template<typename CharT, typename IntegralT>
        constexpr int estimateTypeSize(type<IntegralT>, std::enable_if_t<std::is_integral<IntegralT>::value && !std::is_same<CharT, IntegralT>::value>* = 0) {
            return 20;
        }

        template<typename CharT, size_t StrSizeWith0>
        constexpr int estimateTypeSize(type<const CharT[StrSizeWith0]>) {
            return StrSizeWith0 - 1;
        }

        template<typename CharT, size_t ExpectedSize, typename StringT>
        constexpr int estimateTypeSize(type<sized_str_t<ExpectedSize, StringT>>) {
            return ExpectedSize;
        }

        template<typename CharT, typename T>
        constexpr int estimateTypeSeqSize(type<T> t) {
            return estimateTypeSize<CharT>(t);
        }

        template<typename CharT, typename T1, typename... TX>
        constexpr int estimateTypeSeqSize(type<T1> t1, type<TX>... tx) {
            return estimateTypeSize<CharT>(t1) + estimateTypeSeqSize<CharT>(tx...);
        }


        template<size_t S1, size_t S2, std::size_t... I1, std::size_t... I2>
        constexpr auto concatenateArrayPair(const std::array<char, S1> arr1, const std::array<char, S2> arr2, std::index_sequence<I1...>, std::index_sequence<I2...>)
        {
            return std::array<char, S1 + S2>{ arr1[I1]..., arr2[I2]... };
        }

        template<size_t S>
        constexpr auto concatenateArrays(const std::array<char, S> arr)
        {
            return arr;
        }

        template<size_t S1, size_t S2, size_t... SX>
        constexpr auto concatenateArrays(const std::array<char, S1> arr1, const std::array<char, S2> arr2, const std::array<char, SX>... arrX)
        {
            return concatenateArrays(concatenateArrayPair(arr1, arr2, std::make_index_sequence<arr1.size()>(), std::make_index_sequence<arr2.size()>()), arrX...);
        }

        template<size_t S1, size_t S2>
        constexpr auto concatenateArrays(const std::array<char, S1> arr1, const std::array<char, S2> arr2)
        {
            return concatenateArrayPair(arr1, arr2, std::make_index_sequence<arr1.size()>(), std::make_index_sequence<arr2.size()>());
        }


        template<typename CharT>
        constexpr std::array<char, 1> stringify(CharT c) {
            return { c };
        }

        template<typename CharT, typename IntegralT>
        constexpr std::enable_if_t<std::is_integral<IntegralT>::value && !std::is_same<CharT, IntegralT>::value> stringify(IntegralT) = delete;

        template<typename CharT, size_t N, size_t... IX>
        constexpr std::array<CharT, sizeof...(IX)> stringify(const CharT(&c)[N], std::index_sequence<IX...>) {
            return { c[IX]... };
        }

        template<typename CharT, size_t N>
        constexpr std::array<char, N - 1> stringify(const CharT(&c)[N]) {
            return stringify<CharT>(c, std::make_index_sequence<N - 1>());
        }


        template<typename CharT, typename T, typename = std::void_t<>>
        struct CanStringify : std::false_type {};

        template<typename CharT, typename T>
        struct CanStringify<CharT, T, std::void_t<decltype(stringify<CharT>(std::declval<T>()))>> : std::true_type {};

        template<typename CharT, typename T>
        constexpr bool canStringify(type<T>) {
            return CanStringify<CharT, T>::value;
        }

        template<typename CharT, typename T1, typename... TX>
        constexpr bool canStringify(type<T1>, type<TX>... tx) {
            return CanStringify<CharT, T1>::value && canStringify<CharT>(tx...);
        }


        template<typename CharT, bool StringifyConstexpr>
        struct StringMaker
        {
            template<typename... TX>
            auto operator()(TX&&... vx) const
            {
                constexpr size_t estimatedSize = estimateTypeSeqSize<CharT>(type<std::remove_reference_t<std::remove_cv_t<TX>>>{}...);
                basic_stringbuilder<CharT, estimatedSize, std::char_traits<CharT>, std::allocator<uint8_t>> sb;
                sb.append_many(std::forward<TX>(vx)...);
                return sb.str();
            }
        };

        template<typename CharT>
        struct StringMaker<CharT, true>
        {
            template<size_t N, size_t... IX>
            constexpr auto data_of(const std::array<CharT, N> arr, const std::index_sequence<IX...> ix) const
            {
                return constexpr_str<CharT, N, IX...>{arr, ix};
            }

            template<size_t N>
            constexpr auto data_of(const std::array<CharT, N> arr) const
            {
                return data_of(arr, std::make_index_sequence<N>());
            }

            template<typename... TX>
            constexpr auto operator()(TX&&... vx) const
            {
                return data_of(concatenateArrays(stringify<CharT>(vx)..., stringify<CharT>('\0')));
            }
        };
    }

    template<typename... TX>
    constexpr auto make_stringbuilder(TX&&... vx)
    {
        constexpr size_t estimatedSize = detail::estimateTypeSeqSize<char>(detail::type<TX>{}...);
        stringbuilder<estimatedSize> sb;
        sb.append_many(std::forward<TX>(vx)...);
        return sb;
    }

    template<typename... TX>
    constexpr auto make_string(TX&&... vx)
    {
        constexpr bool stringifyConstexpr = detail::canStringify<char>(detail::type<TX>{}...);
        return detail::StringMaker<char, stringifyConstexpr>{}(std::forward<TX>(vx)...);
    }

#endif // __cpp_lib_integer_sequence

} // namespace STRINGBUILDER_NAMESPACE
