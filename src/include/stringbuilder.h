// MIT License
//
// Copyright (c) 2017 Mariusz Łapiński
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#pragma once

#include <assert.h>
#include <array>
#include <sstream>
#include <utility>
#include <deque>
#include <memory>
#include <numeric>
#include <type_traits>
#include <string_view>

// Appender is an utility class for encoding various kinds of objects (integers) and their propagation to stringbuilder or inplace_stringbuilder.
//

// Unless there are suitable converters, make use of std::to_string() to stringify the object.
template<typename SB, typename T, typename Enable = void>
struct Appender {
    void operator()(SB& sb, const T& v) const {
        sb.append(std::to_string(v));
    }
};

namespace detail
{
    template<typename SB>
    struct Appender<SB, const typename SB::char_type*>
    {
        void operator()(SB& sb, const typename SB::char_type* str) const
        {
            sb.append_c_str(str);
        }
    };

    template<typename SB, typename IntegerT>
    struct Appender<SB, IntegerT, std::enable_if_t<
        std::is_integral_v<IntegerT> && !std::is_same_v<IntegerT, typename SB::char_type> >>
    {
        void operator()(SB& sb, IntegerT iv) const
        {
            // In this particular case, std::div() is x2 slower instead of / and %.
            if (iv >= 0) {
                if (iv >= 10) {
                    detail::basic_inplace_stringbuilder<typename SB::char_type, 20, false, SB::traits_type> bss;
                    do {
                        bss.append(static_cast<typename SB::char_type>('0' + iv % 10));
                        iv /= 10;
                    } while (iv > 0);
                    sb.append(bss);
                } else {
                    sb.append(static_cast<typename SB::char_type>('0' + static_cast<char>(iv)));
                }
            } else {
                if (iv <= -10) {
                    detail::basic_inplace_stringbuilder<typename SB::char_type, 20, false, SB::traits_type> bss;
                    do {
                        bss.append(static_cast<typename SB::char_type>('0' - iv % 10));
                        iv /= 10;
                    } while (iv < 0);
                    bss.append('-');
                    sb.append(bss);
                } else {
                    sb.append('-');
                    sb.append(static_cast<typename SB::char_type>('0' - static_cast<char>(iv)));
                }
            }
        }
    };

    // Provides means for building size-delimited strings in-place (without heap allocations).
    // Object of this class occupies fixed size (specified at compile time) and allows appending portions of strings unless there is space available.
    // In debug mode appending ensures that the built string does not exceed the available space.
    // In release mode no such checks are made thus dangerous memory corruption may occur if used incorrectly.
    //
    template<typename CharT,
        size_t MaxSize,
        bool Forward,
        typename Traits>
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

        size_type size() const noexcept { return consumed; }
        size_type length() const noexcept { return size(); }

        basic_inplace_stringbuilder& append(char_type ch) noexcept
        {
            assert(ch != '\0');
            assert(consumed < MaxSize);
            if /*constexpr*/ (Forward) {
                data_[consumed++] = ch;
            } else {
                data_[MaxSize - (++consumed)] = ch;
            }
            return *this;
        }

        template<size_type StrSizeWith0>
        basic_inplace_stringbuilder& append(const char_type(&str)[StrSizeWith0]) noexcept
        {
            constexpr size_t strSize = StrSizeWith0 - 1;
            assert(consumed + StrSizeWith0 <= MaxSize + 1);
            if (Forward) {
                Traits::copy(data_.data() + consumed, &str[0], strSize);
            } else {
                Traits::copy(data_.data() + MaxSize - strSize - consumed, &str[0], strSize);
            }
            consumed += strSize;
            return *this;
        }

        template<size_type N>
        basic_inplace_stringbuilder& append(const std::array<char_type, N>& arr) noexcept
        {
            return append(arr.data(), N);
        }

        basic_inplace_stringbuilder& append(const char_type* str, size_type size) noexcept
        {
            assert(consumed + size <= MaxSize);
            if (Forward) {
                Traits::copy(data_.data() + consumed, str, size);
            } else {
                Traits::copy(data_.data() + MaxSize - size - consumed, str, size);
            }
            consumed += size;
            return *this;
        }

        basic_inplace_stringbuilder& append_c_str(const char_type* str, size_type size) noexcept
        {
            return append(str, size);
        }

        basic_inplace_stringbuilder& append_c_str(const char_type* str) noexcept
        {
            return append(str, Traits::length(str));
        }

        template<typename OtherTraits, typename OtherAlloc>
        basic_inplace_stringbuilder& append(const std::basic_string<char_type, OtherTraits, OtherAlloc>& str) noexcept
        {
            return append(str.data(), str.size());
        }

        template<typename OtherTraits>
        basic_inplace_stringbuilder& append(const std::basic_string_view<char_type, OtherTraits>& sv) noexcept
        {
            return append(sv.data(), sv.size());
        }

        template<size_type OtherMaxSize, bool OtherForward, typename OtherTraits>
        basic_inplace_stringbuilder& append(const basic_inplace_stringbuilder<char_type, OtherMaxSize, OtherForward, OtherTraits>& ss) noexcept
        {
            return append(ss.str_view());
        }

        // Defining append(AnyT&& any) generates a plethora of class specialization issues.

        template<typename T>
        basic_inplace_stringbuilder& append(const T& v)
        {
            Appender<basic_inplace_stringbuilder, T>{}(*this, v);
            return *this;
        }

        template<typename AnyT>
        basic_inplace_stringbuilder& operator<<(AnyT&& any)
        {
            return append(std::forward<AnyT>(any));
        }

        template<typename AnyT>
        basic_inplace_stringbuilder& append_many(AnyT&& any)
        {
            return append(std::forward<AnyT1>(any));
        }

        template<typename AnyT1, typename... AnyTX>
        basic_inplace_stringbuilder& append_many(AnyT1&& any1, AnyTX&&... anyX)
        {
            return append(std::forward<AnyT1>(any1)).append_many(std::forward<AnyTX>(anyX)...);
        }

        char_type* data() noexcept
        {
            return data_.data() + (Forward ? 0 : MaxSize - consumed);
        }

        const char_type* data() const noexcept
        {
            return data_.data() + (Forward ? 0 : MaxSize - consumed);
        }

        const char_type* c_str() const noexcept
        {
            // Placing '\0' at the end of string is a kind of lazy evaluation and is acceptable also for const objects.
            const_cast<basic_inplace_stringbuilder*>(this)->placeNullTerminator();
            return data();
        }

        std::basic_string<char_type> str() const
        {
            assert(consumed <= MaxSize);
            const auto b = data_.cbegin();
            if /*constexpr*/ (Forward) {
                return {b, b + consumed};
            } else {
                return {b + MaxSize - consumed, b + MaxSize};
            }
        }

        std::basic_string_view<char_type, traits_type> str_view() const noexcept
        {
            return {data(), size()};
        }

    private:
        void placeNullTerminator() noexcept
        {
            assert(consumed <= MaxSize);
            const_cast<char_type&>(data_[Forward ? consumed : MaxSize]) = '\0';
        }

    private:
        size_type consumed = 0;
        std::array<char_type, MaxSize + 1> data_; // Last character is reserved for '\0'.
    };

    // The following code is based on ebo_helper explained in this talk:
    // https://youtu.be/hHQS-Q7aMzg?t=3039
    //
    template<typename OrigAlloc, bool UseEbo = !std::is_final_v<OrigAlloc> && std::is_empty_v<OrigAlloc>>
    struct raw_alloc_provider;

    template<typename OrigAlloc>
    struct raw_alloc_provider<OrigAlloc, true> : private std::allocator_traits<OrigAlloc>::rebind_alloc<uint8_t>
    {
        using AllocRebound = std::allocator_traits<OrigAlloc>::rebind_alloc<uint8_t>;

        template<typename OtherAlloc> constexpr explicit raw_alloc_provider(OtherAlloc&& otherAlloc) : AllocRebound{ std::forward<AllocRebound>(otherAlloc) } {}
        constexpr AllocRebound& get_rebound_allocator() { return *this; }
        constexpr OrigAlloc get_original_allocator() { return OrigAlloc{ *this }; }
    };

    template<typename OrigAlloc>
    struct raw_alloc_provider<OrigAlloc, false>
    {
        using AllocRebound = std::allocator_traits<OrigAlloc>::rebind_alloc<uint8_t>;

        template<typename OtherAlloc> constexpr explicit raw_alloc_provider(OtherAlloc&& otherAlloc) : alloc_rebound{ std::forward<AllocRebound>(otherAlloc) } {}
        constexpr AllocRebound& get_rebound_allocator() { return alloc_rebound; }
        constexpr OrigAlloc get_original_allocator() { return OrigAlloc{ alloc_rebound }; }
    private:
        AllocRebound alloc_rebound;
    };

    // Provides means for efficient construction of strings.
    // Object of this class occupies fixed size (specified at compile time) and allows appending portions of strings.
    // If the available space gets exhausted, new chunks of memory are allocated on the heap.
    //
    template<typename Char,
        size_t InPlaceSize,
        typename Traits,
        typename AllocOrig>
    class basic_stringbuilder : private raw_alloc_provider<AllocOrig>
    {
        using AllocProvider = raw_alloc_provider<AllocOrig>;
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

    private:
        struct Chunk;

        struct ChunkHeader
        {
            Chunk* next;
            size_type consumed;
            size_type reserved;
        };

        struct Chunk : public ChunkHeader
        {
            char_type data[1]; // In practice there are ChunkHeader::reserved of characters in this array.

            Chunk(size_type reserve) : ChunkHeader{nullptr, size_type{0}, reserve} { }
        };

        template<int DataLength>
        struct ChunkInPlace : public ChunkHeader
        {
            std::array<char_type, DataLength> data;

            ChunkInPlace() : ChunkHeader{nullptr, 0, DataLength} { }
        };

        template<>
        struct ChunkInPlace<0> : public ChunkHeader
        {
            ChunkInPlace() : ChunkHeader{nullptr, 0, 0} { }
        };

    public:
        constexpr AllocOrig& get_allocator() noexcept { return get_original_allocator(); }

        template<typename AllocOther = Alloc>
        basic_stringbuilder(AllocOther&& allocOther = AllocOther{}) noexcept : AllocProvider{std::forward<AllocOther>(allocOther)} {}

        basic_stringbuilder(const basic_stringbuilder&) = delete;

        basic_stringbuilder(basic_stringbuilder&& other) noexcept :
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
                AllocTraits::deallocate(get_rebound_allocator(), reinterpret_cast<AllocTraits::pointer>(chunk), sizeof(ChunkHeader) + chunk->reserved);
            }
        }

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

        size_type length() const noexcept { return size(); }

        basic_stringbuilder& append(char_type ch)
        {
            assert(ch != '\0');
            claimOne() = ch;
            return *this;
        }

        template<size_type StrSizeWith0>
        basic_stringbuilder& append(const char_type(&str)[StrSizeWith0])
        {
            constexpr size_type StrSize = StrSizeWith0 - 1;
            assert(str[StrSize] == '\0');
            for (auto left = StrSize; left > 0;)
            {
                const auto claimed = claim(left, 1);
                Traits::copy(claimed.first, &str[StrSize - left], claimed.second);
                left -= claimed.second;
            }
            return *this;
        }

        template<size_type N>
        basic_stringbuilder& append(const std::array<char_type, N>& arr) noexcept
        {
            return append(arr.data(), N);
        }

        basic_stringbuilder& append(const char_type* str, size_type size) noexcept
        {
            for (auto left = size; left > 0;)
            {
                const auto claimed = claim(left, 1);
                Traits::copy(claimed.first, &str[size - left], claimed.second);
                left -= claimed.second;
            }
            return *this;
        }

        basic_stringbuilder& append_c_str(const char_type* str, size_type size) noexcept
        {
            return append(str, size);
        }

        basic_stringbuilder& append_c_str(const char_type* str) noexcept
        {
            return append(str, Traits::length(str));
        }

        template<typename OtherTraits, typename OtherAlloc>
        basic_stringbuilder& append(const std::basic_string<char_type, OtherTraits, OtherAlloc>& str) noexcept
        {
            return append(str.data(), str.size());
        }

        template<typename OtherTraits>
        basic_stringbuilder& append(const std::basic_string_view<char_type, OtherTraits>& sv) noexcept
        {
            return append(sv.data(), sv.size());
        }

        template<size_type OtherMaxSize, bool OtherForward, typename OtherTraits>
        basic_stringbuilder& append(const basic_inplace_stringbuilder<char_type, OtherMaxSize, OtherForward, OtherTraits>& ss) noexcept
        {
            return append(ss.str_view());
        }

        // Defining append(AnyT&& any) generates a plethora of class specialization issues.

        template<typename T>
        basic_stringbuilder& append(const T& v)
        {
            Appender<basic_stringbuilder, T>{}(*this, v);
            return *this;
        }

        template<typename AnyT>
        basic_stringbuilder& operator<<(AnyT&& any)
        {
            return append(std::forward<AnyT>(any));
        }

        template<typename AnyT>
        basic_stringbuilder& append_many(AnyT&& any)
        {
            return append(std::forward<AnyT>(any));
        }

        template<typename AnyT1, typename... AnyTX>
        basic_stringbuilder& append_many(AnyT1&& any1, AnyTX&&... anyX)
        {
            return append(std::forward<AnyT1>(any1)).append_many(std::forward<AnyTX>(anyX)...);
        }

        auto str() const
        {
            auto str = std::basic_string<char_type>(static_cast<size_t>(size()), '\0');
            size_type consumed = 0;
            for (const Chunk* chunk = headChunk(); chunk != nullptr; chunk = chunk->next)
            {
                Traits::copy(str.data() + consumed, chunk->data,  chunk->consumed);
                consumed += chunk->consumed;
            }
            return str;
        }

    private:
        Chunk* allocChunk(size_type reserve)
        {
            const auto chunkTotalSize = (63 + sizeof(ChunkHeader) + sizeof(Char) * reserve) / 64 * 64;
            auto* rawChunk = AllocTraits::allocate(get_rebound_allocator(), chunkTotalSize, tailChunk);
            auto* chunk = reinterpret_cast<Chunk*>(rawChunk);
            AllocTraits::construct<Chunk>(get_rebound_allocator(), chunk, chunkTotalSize - sizeof(ChunkHeader));
            return chunk;
        }

        Chunk* headChunk() noexcept { return reinterpret_cast<Chunk*>(&headChunkInPlace); }
        const Chunk* headChunk() const noexcept { return reinterpret_cast<const Chunk*>(&headChunkInPlace); }

        std::pair<Char*, size_type> claim(size_type length, size_type minimum)
        {
            auto tailChunkLeft = tailChunk->reserved - tailChunk->consumed;
            assert(tailChunkLeft >= 0);
            if (tailChunkLeft < minimum)
            {
                const size_type newChunkLength = std::max(minimum, determineNextChunkSize());
                tailChunk = tailChunk->next = allocChunk(newChunkLength);
                assert(newChunkLength <= tailChunk->reserved);
                tailChunkLeft = newChunkLength;
            }

            assert(tailChunkLeft >= minimum);
            const size_type claimed = std::min(tailChunkLeft, length);
            auto retval = std::make_pair(static_cast<Char*>(tailChunk->data) + tailChunk->consumed, claimed);
            tailChunk->consumed += claimed;
            return retval;
        }

        Char& claimOne()
        {
            auto tailChunkLeft = tailChunk->reserved - tailChunk->consumed;
            assert(tailChunkLeft >= 0);
            if (tailChunkLeft < 1)
            {
                const size_type newChunkLength = determineNextChunkSize();
                tailChunk = tailChunk->next = allocChunk(newChunkLength);
                assert(newChunkLength <= tailChunk->reserved);
            }
            return *(static_cast<Char*>(tailChunk->data) + tailChunk->consumed++);
        }

        size_type determineNextChunkSize() const noexcept { return 2 * tailChunk->reserved; }

    private:
        ChunkInPlace<InPlaceSize> headChunkInPlace;
        Chunk* tailChunk = headChunk();
    };
}

namespace std
{
    template<typename CharT, int InPlaceSize, bool Forward, typename Traits>
    inline auto to_string(const detail::basic_inplace_stringbuilder<CharT, InPlaceSize, Forward, Traits>& sb)
    {
        return sb.str();
    }

    template<typename CharT, int InPlaceSize, typename Traits, typename Alloc>
    inline auto to_string(const detail::basic_stringbuilder<CharT, InPlaceSize, Traits, Alloc>& sb)
    {
        return sb.str();
    }
}

template<int MaxSize, bool Forward = true, typename Traits = std::char_traits<char>>
using inplace_stringbuilder = detail::basic_inplace_stringbuilder<char, MaxSize, Forward, Traits>;

template<int MaxSize, bool Forward = true, typename Traits = std::char_traits<wchar_t>>
using inplace_wstringbuilder = detail::basic_inplace_stringbuilder<wchar_t, MaxSize, Forward, Traits>;

template<int MaxSize, bool Forward = true, typename Traits = std::char_traits<char16_t>>
using inplace_u16stringbuilder = detail::basic_inplace_stringbuilder<char16_t, MaxSize, Forward, Traits>;

template<int MaxSize, bool Forward = true, typename Traits = std::char_traits<char32_t>>
using inplace_u32stringbuilder = detail::basic_inplace_stringbuilder<char32_t, MaxSize, Forward, Traits>;


template<int InPlaceSize = 0, typename Traits = std::char_traits<char>,typename Alloc = std::allocator<char>>
using stringbuilder = detail::basic_stringbuilder<char, InPlaceSize, Traits, Alloc>;

template<int InPlaceSize = 0, typename Traits = std::char_traits<wchar_t>, typename Alloc = std::allocator<wchar_t>>
using wstringbuilder = detail::basic_stringbuilder<wchar_t, InPlaceSize, Traits, Alloc>;

template<int InPlaceSize = 0, typename Traits = std::char_traits<char16_t>, typename Alloc = std::allocator<char16_t>>
using u16stringbuilder = detail::basic_stringbuilder<char16_t, InPlaceSize, Traits, Alloc>;

template<int InPlaceSize = 0, typename Traits = std::char_traits<char32_t>, typename Alloc = std::allocator<char32_t>>
using u32wstringbuilder = detail::basic_stringbuilder<char32_t, InPlaceSize, Traits, Alloc>;


template<int ExpectedSize, typename StringT>
auto sized_str(StringT&& str)
{
    return detail::sized_str_t<ExpectedSize, StringT>{ std::forward<StringT>(str) };
}

namespace detail
{
    template <typename T>
    struct type {};

    template<int ExpectedSize, typename StringT>
    struct sized_str_t
    {
        StringT str;
        sized_str_t(StringT s) : str(std::move(s)) {}
    };

    constexpr int estimateTypeSize(type<char>)
    {
        return 1;
    }

    template<int StrSizeWith0>
    constexpr int estimateTypeSize(type<const char(&)[StrSizeWith0]>)
    {
        return StrSizeWith0 - 1;
    }

    template<int ExpectedSize, typename StringT>
    constexpr int estimateTypeSize(type<sized_str_t<ExpectedSize, StringT>>)
    {
        return ExpectedSize;
    }

    template<typename T>
    constexpr int estimateTypeSeqSize(type<T> v)
    {
        return estimateTypeSize(v);
    }

    template<typename T1, typename... TX>
    constexpr int estimateTypeSeqSize(type<T1> v1, type<TX>... vx)
    {
        return estimateTypeSize(v1) + estimateTypeSeqSize(vx...);
    }

    template<size_t S1, size_t S2, std::size_t... I1, std::size_t... I2>
    constexpr auto concatenateArrayPair(const std::array<char, S1> arr1, const std::array<char, S2> arr2, std::index_sequence<I1...>, std::index_sequence<I2...>)
    {
        return std::array<char, S1 + S2>{ arr1[I1]..., arr2[I2]... };
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

    template<size_t S>
    constexpr auto concatenateArrays(const std::array<char, S> arr)
    {
        return arr;
    }

    /*
    template<size_t S, size_t... I>
    struct CharArrayHelper
    {
        char ch[S];

        constexpr CharArrayHelper(const std::array<char, S> arr) :
            ch{ arr[I]... }
        { }
    };

    template<size_t S, size_t... I>
    constexpr auto toCharArray(const std::array<char, S> arr, std::index_sequence<I...>)
    {
        return CharArrayHelper<S, I...> { arr };
    }

    template<size_t S>
    constexpr auto toCharArray(const std::array<char, S> arr)
    {
        return toCharArray(arr, std::make_index_sequence<S>());
    }
    */

    constexpr std::array<char, 1> stringify(char c)
    { return { c }; }

    template<typename T, typename = std::void_t<>>
    struct CanStringify : std::false_type {};

    template<typename T>
    struct CanStringify<T, std::void_t<decltype(stringify(std::declval<T>()))>> : std::true_type {};

    template<typename T>
    constexpr bool canStringify(type<T>)
    { return CanStringify<T>::value; }

    template<typename T1, typename... TX>
    constexpr bool canStringify(type<T1>, type<TX>... tx)
    { return CanStringify<T1>::value && canStringify(tx...); }

    template<bool Stringify>
    struct StringMaker
    {
        template<typename SB, typename T1, typename... TX>
        constexpr void append(SB& sb, T1&& v1, TX&&... vx) const
        {
            sb.append(std::forward<T1>(v1));
            append(sb, std::forward<TX>(vx)...);
        }

        template<typename SB, typename T>
        constexpr void append(SB& sb, T&& v) const
        {   sb.append(std::forward<T>(v)); }
        
        template<typename... TX>
        constexpr auto operator()(TX&&... vx) const
        {
            constexpr int estimatedSize = estimateTypeSeqSize(type<TX>{}...);
            stringbuilder<estimatedSize> ss;
            ss.append_many(vx...);
            return ss.str();
        }
    };

    template<>
    struct StringMaker<true>
    {
        template<typename... TX>
        constexpr auto operator()(TX&&... vx) const
        {
            //return toCharArray(concatenateArrays(stringify(vx)..., stringify('\0')));
            return concatenateArrays(stringify(vx)..., stringify('\0'));
        }
    };
}

template<typename... TX>
constexpr auto make_string(TX&&... vx)
{
    return detail::StringMaker<detail::canStringify(detail::type<TX>{}...)>{}(std::forward<TX>(vx)...);
}
