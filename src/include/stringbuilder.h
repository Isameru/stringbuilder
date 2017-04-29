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

namespace detail
{
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
            if (Forward) {
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
            assert(consumed + StrSizeWith0 <= InPlaceSize);
            if (Forward) {
                Traits::copy(data_.data() + consumed, &str[0], strSize);
            } else {
                Traits::copy(data_.data() + MaxSize + strSize - consumed, &str[0], strSize);
            }
            consumed += strSize;
            return *this;
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

        basic_inplace_stringbuilder& append(const char_type* str) noexcept
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

        template<typename Any>
        basic_inplace_stringbuilder& append(const Any& a) noexcept
        {
            return append(std::to_string(a));
        }

        template<typename IntegerT, typename = std::enable_if<Forward>>
        void encodeBackwards(IntegerT iv)
        {
            if (iv >= 0)
            {
                do {
                    auto divres = std::div(iv, IntegerT{10});
                    append(static_cast<char_type>('0' + divres.rem));
                    iv = divres.quot;
                } while (iv > 0);
            }
            else
            {
                do {
                    auto divres = std::div(iv, IntegerT{10});
                    append(static_cast<char_type>('0' - divres.rem));
                    iv = divres.quot;
                } while (iv < 0);
                append('-');
            }
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
            assureNullTermination();
            return data();
        }

        auto str() const
        {
            assert(consumed <= MaxSize);
            const auto b = data_.cbegin();
            if (Forward) {
                return std::basic_string<char_type>(b, b + consumed);
            } else {
                return std::basic_string<char_type>(b + MaxSize - consumed, b + MaxSize);
            }
        }

        std::basic_string_view<char_type, traits_type> str_view() const noexcept
        {
            return { data(), size() };
        }

        template<typename Any>
        basic_inplace_stringbuilder& operator<<(Any&& a)
        {
            return append(std::forward<Any>(a));
        }

    private:
        void assureNullTermination() const noexcept
        {
            assert(consumed <= MaxSize);
            const_cast<char_type&>(data_[Forward ? consumed : MaxSize]) = '\0';
        }

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

    // Provides means for building strings.
    // Object of this class occupies fixed size (specified at compile time) and allows appending portions of strings.
    // If the available space exhausts, new chunks of memory are allocated on heap.
    //
    template<typename Char,
        int InPlaceSize,
        typename AllocOrig>
    class basic_stringbuilder : private raw_alloc_provider<AllocOrig>
    {
        using AllocProvider = raw_alloc_provider<AllocOrig>;
        using Alloc = typename AllocProvider::AllocRebound;
        using AllocTraits = std::allocator_traits<Alloc>;

        struct Chunk;

        struct ChunkHeader
        {
            Chunk* next;
            int consumed;
            int reserved;
        };

        struct Chunk : public ChunkHeader
        {
            Char data[1]; // In practice there are (ChunkHeader::reserved) of characters in this array.

            Chunk(int reserve) : ChunkHeader{nullptr, 0, reserve} { }
        };

        template<int DataLength>
        struct ChunkInPlace : public ChunkHeader
        {
            std::array<Char, DataLength> data;

            ChunkInPlace() : ChunkHeader{nullptr, 0, DataLength} { }
        };

        template<>
        struct ChunkInPlace<0> : public ChunkHeader
        {
            ChunkInPlace() : ChunkHeader{nullptr, 0, 0} { }
        };

        ChunkInPlace<InPlaceSize> headChunkInPlace;
        Chunk* tailChunk = headChunk();

    public:
        using value_type = Char;
        using allocator_type = AllocOrig;
        using size_type = int;
        using difference_type = int;
        using reference = Char&;
        using const_reference = const Char&;
        using pointer = Char*;
        using const_pointer = const Char*;

        constexpr AllocOrig& get_allocator() noexcept
        {
            return get_original_allocator();
        }

        template<typename AllocOther = Alloc>
        basic_stringbuilder(AllocOther&& allocOther = AllocOther{}) noexcept : AllocProvider{std::forward<AllocOther>(allocOther)} {}
        basic_stringbuilder(const basic_stringbuilder&) = delete;
        basic_stringbuilder(basic_stringbuilder&& sb) noexcept = default;

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

        basic_stringbuilder& append(Char ch)
        {
            assert(ch != '\0');

            const auto claimed = claim(1, 1);
            assert(claimed.second == 1);
            *(claimed.first) = ch;

            return *this;
        }

        template<int StrSizeWith0>
        basic_stringbuilder& append(const Char (&str)[StrSizeWith0])
        {
            constexpr int StrSize = StrSizeWith0 - 1;
            assert(str[StrSize] == '\0');

            for (int left = StrSize; left > 0;)
            {
                const auto claimed = claim(left, 1);
                std::copy_n(&str[StrSize - left], claimed.second, claimed.first);
                left -= claimed.second;
            }

            return *this;
        }

        int size() const noexcept
        {
            int size = 0;
            const auto* chunk = headChunk();
            do {
                size += chunk->consumed;
                chunk = chunk->next;
            } while (chunk != nullptr);
            return size;
        }

        auto str() const
        {
            const auto size0 = size();
            auto str = std::basic_string<Char>(static_cast<const size_t>(size0), '\0');
            int consumed = 0;
            for (const Chunk* chunk = headChunk(); chunk != nullptr; chunk = chunk->next)
            {
                std::copy_n(&chunk->data[0], chunk->consumed, std::begin(str) + consumed);
                consumed += chunk->consumed;
            }
            return str;
        }

        template<typename Any>
        basic_stringbuilder& operator<<(Any&& a)
        {
            return append(std::forward<Any>(a));
        }

    private:
        Chunk* allocChunk(int reserve)
        {
            const auto chunkTotalSize = (63 + sizeof(ChunkHeader) + sizeof(Char) * reserve) / 64 * 64;
            auto* rawChunk = AllocTraits::allocate(get_rebound_allocator(), chunkTotalSize, tailChunk);
            auto* chunk = reinterpret_cast<Chunk*>(rawChunk);
            AllocTraits::construct<Chunk>(get_rebound_allocator(), chunk, chunkTotalSize - sizeof(ChunkHeader));
            return chunk;
        }

        Chunk* headChunk()
        { return reinterpret_cast<Chunk*>(&headChunkInPlace); }

        const Chunk* headChunk() const
        { return reinterpret_cast<const Chunk*>(&headChunkInPlace); }

        std::pair<Char*, int> claim(int length, int minimum)
        {
            auto tailChunkLeft = tailChunk->reserved - tailChunk->consumed;
            assert(tailChunkLeft >= 0);
            if (tailChunkLeft < minimum)
            {
                const int newChunkLength = std::max(minimum, determineNextChunkSize());
                tailChunk = tailChunk->next = allocChunk(newChunkLength);
                assert(newChunkLength <= tailChunk->reserved);
                tailChunkLeft = newChunkLength;
            }

            assert(tailChunkLeft >= minimum);
            const int claimed = std::min(tailChunkLeft, length);
            auto retval = std::make_pair(static_cast<Char*>(tailChunk->data) + tailChunk->consumed, claimed);
            tailChunk->consumed += claimed;
            return retval;
        }

        int determineNextChunkSize() const
        { return (2 * tailChunk->reserved + 63) / 64 * 64; }
    };
}

namespace std
{
    template<typename Char, int InPlaceSize, bool Forward, typename Traits>
    inline auto to_string(const detail::basic_inplace_stringbuilder<Char, InPlaceSize, Forward, Traits>& sb)
    {
        return sb.str();
    }

    template<typename Char, int InPlaceSize, typename Alloc>
    inline auto to_string(const detail::basic_stringbuilder<Char, InPlaceSize, Alloc>& sb)
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


template<int InPlaceSize, typename Alloc = std::allocator<char>>
using stringbuilder = detail::basic_stringbuilder<char, InPlaceSize, Alloc>;

template<int InPlaceSize, typename Alloc = std::allocator<wchar_t>>
using wstringbuilder = detail::basic_stringbuilder<wchar_t, InPlaceSize, Alloc>;


template<int Size>
struct sized_str
{
    std::string str;
    sized_str(std::string s) : str(std::move(s)) {}
};

namespace detail
{
    template <typename T>
    struct type {};

    constexpr int estimateTypeSize(type<char>)
    {
        return 1;
    }

    template<int StrSizeWith0>
    constexpr int estimateTypeSize(type<const char(&)[StrSizeWith0]>)
    {
        return StrSizeWith0 - 1;
    }

    template<int StrSize>
    constexpr int estimateTypeSize(type<sized_str<StrSize>>)
    {
        return StrSize;
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

    template<typename SS, typename T>
    SS& append(SS& ss, T&& t)
    {
        ss << t;
        return ss;
    }

    template<typename SS, typename T1, typename... TX>
    SS& append(SS& ss, T1&& t1, TX&&... tx)
    {
        append(ss, t1);
        append(ss, tx...);
        return ss;
    }

    template<bool Stringify>
    struct StringMaker
    {
        template<typename... TX>
        constexpr auto operator()(TX&&... vx) const
        {
            constexpr int estimatedSize = estimateTypeSeqSize(type<TX>{}...);
            stringbuilder<estimatedSize> ss;
            append(ss, vx...);
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
