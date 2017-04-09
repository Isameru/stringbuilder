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
#include <sstream>
#include <utility>
#include <deque>
#include <memory>
#include <numeric>

namespace detail
{
    template<typename CharTy,
        int inPlaceLength,
        typename Alloc>
    class StringBuilder : protected Alloc
    {
    private:
        struct Chunk;

        struct ChunkHeader
        {
            Chunk* next;
            int consumed;
            int reserved;
        };

        struct Chunk : public ChunkHeader
        {
            CharTy data[1];

            Chunk(int reserve) : ChunkHeader{nullptr, 0, reserve} { }
        };

        template<int DataLength>
        struct ChunkInPlace : public ChunkHeader
        {
            CharTy data[DataLength];

            ChunkInPlace() : ChunkHeader{nullptr, 0, DataLength} { }
        };
     
        template<>
        struct ChunkInPlace<0> : public ChunkHeader
        {
            ChunkInPlace() : ChunkHeader{nullptr, 0, 0} { }
        };

        ChunkInPlace<inPlaceLength> headChunk;
        ChunkHeader* tailChunk{&headChunk};

    public:
        StringBuilder(const Alloc& alloc = Alloc{}) : Alloc{alloc} {}
        StringBuilder(const StringBuilder&) = delete;
        StringBuilder(const StringBuilder&& sb) : chunks(std::move(sb)), alloc{sb.alloc} { }

        StringBuilder& append(CharTy ch)
        {
            assert(ch != '\0');

            const auto claimed = claim(1);
            assert(claimed.second == 1);
            *(claimed.first) = ch;

            return *this;
        }

        template<int StrSizeWith0>
        StringBuilder& append(const CharTy (&str)[StrSizeWith0])
        {
            constexpr int StrSize = StrSizeWith0 - 1;
            assert(str[StrSize] == '\0');

            for (int left = StrSize; left > 0;)
            {
                const auto claimed = claim(left);
                std::copy_n(&str[StrSize - left], claimed.second, claimed.first);
                left -= claimed.second;
            }

            return *this;
        }

        int size() const noexcept
        {
            int size = 0;
            const auto* chunk = reinterpret_cast<const Chunk*>(&headChunk);
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
            for (const Chunk* chunk = reinterpret_cast<const Chunk*>(&headChunk); chunk != nullptr; chunk = chunk->next)
            {
                std::copy_n(&chunk->data[0], chunk->consumed, std::begin(str) + consumed);
                consumed += chunk->consumed;
            }
            return str;
        }

        template<typename Any>
        StringBuilder& operator<<(Any&& a)
        {
            return append(a);
        }

    private:
        std::pair<CharTy*, int> claim(int length)
        {
            Chunk* lastChunk = reinterpret_cast<Chunk*>(tailChunk);
            auto lastChunkLeft = lastChunk->reserved - lastChunk->consumed;
            assert(lastChunkLeft >= 0);
            if (lastChunkLeft == 0)
            {
                const int newChunkLength = nextChunkSize(lastChunk->reserved);
                lastChunk->next = reinterpret_cast<Chunk*>(Alloc::allocate(sizeof(ChunkHeader) + sizeof(CharTy) * newChunkLength));
                tailChunk = lastChunk = new (lastChunk->next) Chunk{newChunkLength};
                lastChunkLeft = newChunkLength;
            }

            assert(lastChunkLeft > 0);
            const int claimed = std::min(lastChunkLeft, length);
            auto retval = std::make_pair(static_cast<CharTy*>(lastChunk->data) + lastChunk->consumed, claimed);
            lastChunk->consumed += claimed;
            return retval;
        }

        std::pair<CharTy*, int> claim_force(int length)
        {
            Chunk* lastChunk = reinterpret_cast<chunk*>(last_chunk);
            auto lastChunkLeft = lastChunk->reserved - lastChunk->consumed;
            assert(lastChunkLeft >= 0);
            if (lastChunkLeft < length)
            {
                const int newChunkLength = std::max(length, nextChunkSize(lastChunk->reserved));
                lastChunk->next = reinterpret_cast<chunk*>(Alloc::allocate(sizeof(ChunkHeader) + sizeof(CharTy) * newChunkLength));
                tailChunk = lastChunk = new (lastChunk->next) Chunk{newChunkLength};
                lastChunkLeft = newChunkLength;
            }

            assert(lastChunkLeft > 0);
            const int claimed = length;
            auto retval = std::make_pair(static_cast<CharTy*>(lastChunk->data) + lastChunk->consumed, claimed);
            lastChunk->consumed += claimed;
            return retval;
        }

        int nextChunkSize(int prevChunkSize)
        {
            return std::max(prevChunkSize * 2, 64);
        }
    };
}

namespace std
{
    template<typename... Any>
    inline string to_string(const detail::StringBuilder<Any...>& sb) noexcept
    {
        return sb.str();
    }
}

template<typename CharTy, int InPlaceSize, typename Alloc>
using basic_stringbuilder = detail::StringBuilder<CharTy, InPlaceSize, Alloc>;

template<int InPlaceSize, typename Alloc = std::allocator<char>>
using stringbuilder = basic_stringbuilder<char, InPlaceSize, Alloc>;

template<int InPlaceSize, typename Alloc = std::allocator<wchar_t>>
using wstringbuilder = basic_stringbuilder<wchar_t, InPlaceSize, Alloc>;
