// MIT License
//
// Copyright (c) 2017 Isameru
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

namespace detail
{
    // Provides means for building size-delimited strings in-place.
    // Object of this class occupies fixed size (specified at compile time) and allows appending portions of strings unless there is space available.
    // In debug mode appending ensures that the built string does not exceed the available space.
    // In release mode no such checks are made thus dangerous memory corruption may occur if used incorrectly.
    //
    template<typename CharTy,
        int InPlaceSize,
        bool Forward>
    class basic_inplace_stringbuilder
    {
        int consumed = 0;
        std::array<CharTy, InPlaceSize> data; // Last character is reserved for '\0'.

    public:
        basic_inplace_stringbuilder& append(CharTy ch)
        {
            assert(ch != '\0');
            assert(consumed + 1 < InPlaceSize);
            if (Forward) {
                data[consumed++] = ch;
            } else {
                data[InPlaceSize - 1 - (++consumed)] = ch;
            }
            return *this;
        }

        template<int StrSizeWith0>
        basic_inplace_stringbuilder& append(const CharTy (&str)[StrSizeWith0])
        {
            assert(consumed + StrSizeWith0 <= InPlaceSize);
            if (Forward) {
                std::copy_n(&str[0], StrSizeWith0 - 1, std::begin(data) + consumed);
            } else {
                std::copy_n(&str[0], StrSizeWith0 - 1, std::begin(data) + InPlaceSize - StrSizeWith0 - consumed);
            }
            consumed += StrSizeWith0 - 1;
            return *this;
        }

        auto str() const
        {
            assert(consumed < InPlaceSize);
            const auto b = data.cbegin();
            if (Forward) {
                return std::basic_string<CharTy>(b, b + consumed);
            } else {
                return std::basic_string<CharTy>(b + InPlaceSize - 1 - consumed, b + InPlaceSize - 1);
            }
        }

        template<typename Any>
        basic_inplace_stringbuilder& operator<<(Any&& a)
        {
            return append(std::forward<Any>(a));
        }
    };

    // ...
    //
    template<typename CharTy,
        int InPlaceSize,
        typename Alloc>
    class basic_stringbuilder : protected Alloc
    {
        struct Chunk;

        struct ChunkHeader
        {
            Chunk* next;
            int consumed;
            int reserved;
        };

        struct Chunk : public ChunkHeader
        {
            CharTy data[1]; // In practice there are (ChunkHeader::reserved) of characters in this array.

            Chunk(int reserve) : ChunkHeader{nullptr, 0, reserve} { }
        };

        template<int DataLength>
        struct ChunkInPlace : public ChunkHeader
        {
            std::array<CharTy, DataLength> data;

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
        basic_stringbuilder(const Alloc& alloc = Alloc{}) : Alloc{alloc} {}
        basic_stringbuilder(const basic_stringbuilder&) = delete;
        basic_stringbuilder(basic_stringbuilder&& sb) = default;

        basic_stringbuilder& append(CharTy ch)
        {
            assert(ch != '\0');

            const auto claimed = claim(1, 1);
            assert(claimed.second == 1);
            *(claimed.first) = ch;

            return *this;
        }

        template<int StrSizeWith0>
        basic_stringbuilder& append(const CharTy (&str)[StrSizeWith0])
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
            auto str = std::basic_string<CharTy>(static_cast<const size_t>(size0), '\0');
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
        Chunk* headChunk()
        { return reinterpret_cast<Chunk*>(&headChunkInPlace); }

        const Chunk* headChunk() const
        { return reinterpret_cast<const Chunk*>(&headChunkInPlace); }

        std::pair<CharTy*, int> claim(int length, int minimum)
        {
            auto tailChunkLeft = tailChunk->reserved - tailChunk->consumed;
            assert(tailChunkLeft >= 0);
            if (tailChunkLeft < minimum)
            {
                const int newChunkLength = std::max(minimum, determineNextChunkSize());
                tailChunk->next = reinterpret_cast<Chunk*>(Alloc::allocate(sizeof(ChunkHeader) + sizeof(CharTy) * newChunkLength));
                tailChunk = new (tailChunk->next) Chunk{newChunkLength};
                tailChunkLeft = newChunkLength;
            }

            assert(tailChunkLeft >= minimum);
            const int claimed = std::min(tailChunkLeft, length);
            auto retval = std::make_pair(static_cast<CharTy*>(tailChunk->data) + tailChunk->consumed, claimed);
            tailChunk->consumed += claimed;
            return retval;
        }

        int determineNextChunkSize() const
        { return (2 * tailChunk->reserved + 63) / 64 * 64; }
    };
}

namespace std
{
    template<typename CharTy, int InPlaceSize, bool Forward>
    inline auto to_string(const detail::basic_inplace_stringbuilder<CharTy, InPlaceSize, Forward>& sb)
    {
        return sb.str();
    }

    template<typename CharTy, int InPlaceSize, typename Alloc>
    inline auto to_string(const detail::basic_stringbuilder<CharTy, InPlaceSize, Alloc>& sb)
    {
        return sb.str();
    }
}

template<int InPlaceSize, bool Forward = true>
using inplace_stringbuilder = detail::basic_inplace_stringbuilder<char, InPlaceSize, Forward>;

template<int InPlaceSize, bool Forward = true>
using inplace_wstringbuilder = detail::basic_inplace_stringbuilder<wchar_t, InPlaceSize, Forward>;

template<int InPlaceSize, typename Alloc = std::allocator<char>>
using stringbuilder = detail::basic_stringbuilder<char, InPlaceSize, Alloc>;

template<int InPlaceSize, typename Alloc = std::allocator<wchar_t>>
using wstringbuilder = detail::basic_stringbuilder<wchar_t, InPlaceSize, Alloc>;
