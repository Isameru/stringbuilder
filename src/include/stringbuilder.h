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

template<typename CharTy = char,
    typename Alloc = std::allocator<CharTy>>
class stringbuilder
{
private:
    struct chunk
    {
        int reserved;
        int consumed;
        std::unique_ptr<CharTy[]> data;
    };

    std::deque<chunk> chunks;
    Alloc alloc;

public:
    stringbuilder(const Alloc& alloc_ = Alloc{}) : chunks{}, alloc{alloc_} {}
    stringbuilder(const stringbuilder&) = delete;
    stringbuilder(const stringbuilder&& sb) : chunks(std::move(sb)), alloc{sb.alloc} { }

    stringbuilder& append(CharTy ch)
    {
        assert(ch != '\0');
        auto lastChunkIter{chunks.rbegin()};
        if (lastChunkIter == std::rend(chunks))
        {
            const auto newChunkLength = 64;
            chunks.push_back(chunk{newChunkLength, 0, std::make_unique<CharTy[]>(newChunkLength)});
            lastChunkIter = chunks.rbegin();
        }
        else if (lastChunkIter->consumed == lastChunkIter->reserved)
        {
            const auto newChunkLength = 2 * lastChunkIter->reserved;
            chunks.push_back(chunk{4, 0, std::make_unique<CharTy[]>(4)});
            lastChunkIter = chunks.rbegin();
        }

        lastChunkIter->data.get()[lastChunkIter->consumed] = ch;
        ++lastChunkIter->consumed;

        return *this;
    }

    template<int StrSizeWith0>
    stringbuilder& append(const CharTy (&str)[StrSizeWith0])
    {
        constexpr int StrSize = StrSizeWith0 - 1;
        assert(str[StrSize] == '\0');

        for (int left = StrSize; left > 0;)
        {
            auto lastChunkIter{chunks.rbegin()};
            if (lastChunkIter == std::rend(chunks))
            {
                const auto newChunkLength = 64;
                chunks.push_back(chunk{newChunkLength, 0, std::make_unique<CharTy[]>(newChunkLength)});
                lastChunkIter = chunks.rbegin();
            }
            else if (lastChunkIter->consumed == lastChunkIter->reserved)
            {
                const auto newChunkLength = 2 * lastChunkIter->reserved;
                chunks.push_back(chunk{4, 0, std::make_unique<CharTy[]>(4)});
                lastChunkIter = chunks.rbegin();
            }

            const auto chunkLeft{lastChunkIter->reserved - lastChunkIter->consumed};
            const auto toCopy{std::min(left, chunkLeft)};

            std::copy_n(&str[StrSize - left], toCopy, lastChunkIter->data.get() + lastChunkIter->consumed);

            left -= toCopy;
            lastChunkIter->consumed += toCopy;
        }

        return *this;
    }

    auto str() const
    {
        auto length = std::accumulate(std::begin(chunks), std::end(chunks), int{0}, [](const int& count, const chunk& chunk) { return count + chunk.consumed; });
        auto str = std::basic_string<CharTy>(static_cast<const size_t>(length), '\0');
        int consumed = 0;
        for (const chunk& chunk : chunks)
        {
            std::copy_n(chunk.data.get(), chunk.consumed, std::begin(str) + consumed);
            consumed += chunk.consumed;
        }
        return str;
    }

    template<typename Any>
    stringbuilder& operator<<(Any&& a)
    {
        return append(a);
    }
};

namespace std
{
    template<typename... Any>
    inline string to_string(const stringbuilder<Any...>& sb)
    {
        return sb.str();
    }
}
