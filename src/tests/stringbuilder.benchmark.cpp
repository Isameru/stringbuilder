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

#include <stringbuilder.h>
#include <algorithm>
#include <strstream>
#include <iostream>
#include <chrono>
#include <vector>
#ifdef WIN32
#include <intrin.h>
#endif

template<typename MethodT>
void BenchmarkMean(const char* title, const size_t iterCount, MethodT method) {
    using Clock = std::chrono::high_resolution_clock;

    for (size_t iter = 0; iter < iterCount / 10; ++iter)
    {
        std::string str = method();
        volatile size_t size = str.size();
    }

    std::vector<Clock::duration> durations;
    durations.reserve(iterCount);

    for (size_t iter = 0; iter < iterCount; ++iter)
    {
        const auto time0 = Clock::now();

        std::string str = method();
        volatile size_t size = str.size();

        const auto elapsed = Clock::now() - time0;
        durations.push_back(elapsed);
    }

    std::sort(std::begin(durations), std::end(durations));
    const auto meanDuration = durations[durations.size() / 2];

    std::cout << "    " << title << ": " << std::chrono::duration_cast<std::chrono::nanoseconds>(meanDuration).count() << " ns" << std::endl;
};

template<typename MethodT>
void BenchmarkAverage(const char* title, const size_t iterCount, MethodT method) {
    using Clock = std::chrono::high_resolution_clock;

    for (size_t iter = 0; iter < iterCount / 10; ++iter)
    {
        std::string str = method();
        volatile size_t size = str.size();
    }

    const auto time0 = Clock::now();

    for (size_t iter = 0; iter < iterCount; ++iter)
    {
        std::string str = method();
        volatile size_t size = str.size();
    }

    const auto elapsed = Clock::now() - time0;

    std::cout << "    " << title << ": " << std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed).count() / iterCount << " ns" << std::endl;
};

void benchmarkIntegerSequence()
{
    std::cout << "Scenario: IntegerSequence" << std::endl;

    constexpr size_t iterCount = 3000;
    constexpr int span = 1000;

    BenchmarkMean("inplace_stringbuilder<big>", iterCount, [=]() {
        inplace_stringbuilder<8788> sb;
        for (int i = -span; i <= span; ++i) {
            sb << i;
            sb << ' ';
        }
        return sb.str();
    });

    BenchmarkMean("stringbuilder<>", iterCount, [=]() {
        stringbuilder<> sb;
        for (int i = -span; i < span; ++i) {
            sb << i;
            sb << ' ';
        }
        return sb.str();
    });

    BenchmarkMean("stringbuilder<> with reserve", iterCount, [=]() {
        stringbuilder<> sb;
        sb.reserve(8788);
        for (int i = -span; i < span; ++i) {
            sb << i;
            sb << ' ';
        }
        return sb.str();
    });

    BenchmarkMean("stringbuilder<big>", iterCount, [=]() {
        stringbuilder<8788> sb;
        for (int i = -span; i < span; ++i) {
            sb << i;
            sb << ' ';
        }
        return sb.str();
    });

    BenchmarkMean("string.append(a).append(b)", iterCount, [=]() {
        std::string s;
        for (int i = -span; i <= span; ++i) {
            s.append(std::to_string(i)).append(1, ' ');
        }
        return s;
    });

    BenchmarkMean("string.append(a+b)", iterCount, [=]() {
        std::string s;
        for (int i = -span; i <= span; ++i) {
            s.append(std::to_string(i) + ' ');
        }
        return s;
    });

    BenchmarkMean("stringstream", iterCount, [=]() {
        std::stringstream ss;
        for (int i = -span; i <= span; ++i) {
            ss << i << ' ';
        }
        return ss.str();
    });

    BenchmarkMean("static stringstream", iterCount, [=]() {
        static std::stringstream ss;
        ss.str("");
        for (int i = -span; i <= span; ++i) {
            ss << i << ' ';
        }
        return ss.str();
    });

    {
        constexpr int bufSize = 8788 + 1;
        char buf[bufSize];
        BenchmarkMean("strstream", iterCount, [=, &buf]() {
            std::strstream ss{buf, bufSize};
            for (int i = -span; i <= span; ++i) {
                ss << i << ' ';
            }
            ss << std::ends;
            return ss.str();
        });
    }
}

void benchmarkBook()
{
    std::cout << "Scenario: Book" << std::endl;

    constexpr size_t iterCount = 32;
    constexpr size_t wordCount = 400'000;
    constexpr const char* dictionary[] = { "Evolution", "brings", "human", "beings.", "Human", "beings,", "through", "a", "long", "and", "painful", "process,", "bring", "humanity." };
    constexpr size_t dictionarySize = sizeof(dictionary) / sizeof(dictionary[0]);

    BenchmarkMean("stringbuilder<>", iterCount, [=]() {
        stringbuilder<> sb;
        for (size_t i = 0; i < wordCount; ++i) {
            sb << dictionary[i % dictionarySize] << ' ';
        }
        return sb.str();
    });

    BenchmarkMean("stringbuilder<> with reserve", iterCount, [=]() {
        stringbuilder<> sb;
        sb.reserve(2771432);
        for (size_t i = 0; i < wordCount; ++i) {
            sb << dictionary[i % dictionarySize] << ' ';
        }
        return sb.str();
    });

    BenchmarkMean("stringbuilder<4kB>", iterCount, [=]() {
        stringbuilder<4 * 1024> sb;
        for (size_t i = 0; i < wordCount; ++i) {
            sb << dictionary[i % dictionarySize] << ' ';
        }
        return sb.str();
    });

    BenchmarkMean("stringbuilder<64kB>", iterCount, [=]() {
        stringbuilder<64 * 1024> sb;
        for (size_t i = 0; i < wordCount; ++i) {
            sb << dictionary[i % dictionarySize] << ' ';
        }
        return sb.str();
    });

    BenchmarkMean("stringbuilder<512kB>", iterCount, [=]() {
        stringbuilder<512 * 1024> sb;
        for (size_t i = 0; i < wordCount; ++i) {
            sb << dictionary[i % dictionarySize] << ' ';
        }
        return sb.str();
    });

    BenchmarkMean("string.append(a).append(b)", iterCount, [=]() {
        std::string s;
        for (size_t i = 0; i < wordCount; ++i) {
            s.append(dictionary[i % dictionarySize]);
            s.append(1, ' ');
        }
        return s;
    });

    BenchmarkMean("string.append(a+b)", iterCount, [=]() {
        std::string s;
        for (size_t i = 0; i < wordCount; ++i) {
            s.append(std::string(dictionary[i % dictionarySize]) + ' ');
        }
        return s;
    });

    BenchmarkMean("stringstream", iterCount, [=]() {
        std::stringstream ss;
        for (size_t i = 0; i < wordCount; ++i) {
            ss << dictionary[i % dictionarySize] << ' ';
        }
        return ss.str();
    });

    BenchmarkMean("static stringstream", iterCount, [=]() {
        static std::stringstream ss;
        ss.str("");
        for (size_t i = 0; i < wordCount; ++i) {
            ss << dictionary[i % dictionarySize] << ' ';
        }
        return ss.str();
    });

    {
        constexpr int bufSize = 2771432 + 1;
        auto buf = std::make_unique<char[]>(bufSize);
        BenchmarkMean("strstream", iterCount, [=, &buf]() {
            std::strstream ss{buf.get(), bufSize};
            for (size_t i = 0; i < wordCount; ++i) {
                ss << dictionary[i % dictionarySize] << ' ';
            }
            ss << std::ends;
            return ss.str();
        });
    }
}

void benchmarkQuote()
{
    std::cout << "Scenario: Quote" << std::endl;

    constexpr size_t iterCount = 200'000;

    BenchmarkAverage("one string", iterCount, [=]() {
        return std::string("There are only 10 people in the world: those who know binary and those who don't.");
    });

    BenchmarkAverage("constexpr make_string", iterCount, [=]() {
        constexpr auto constexpr_quote = make_string("There", ' ', "are", ' ', "only", ' ', "10", ' ', "people", ' ', "in", ' ', "the", ' ', "world", ':', ' ', "those", ' ', "who", ' ', "know", ' ', "binary", ' ', "and", ' ', "those", ' ', "who", ' ', "don't", '.');
        return constexpr_quote.str();
    });

    BenchmarkAverage("make_string", iterCount, [=]() {
        return make_string("There", ' ', "are", ' ', "only", ' ', 10, ' ', "people", ' ', "in", ' ', "the", ' ', "world", ':', ' ', "those", ' ', "who", ' ', "know", ' ', "binary", ' ', "and", ' ', "those", ' ', "who", ' ', "don't", '.');
    });

    BenchmarkAverage("string+string", iterCount, [=]() {
        return std::string("There") + ' ' + "are" + ' ' + "only" + ' ' + std::to_string(10) + ' ' + "people" + ' ' + "in" + ' ' + "the" + ' ' + "world" + ':' + ' ' + "those" + ' ' + "who" + ' ' + "know" + ' ' + "binary" + ' ' + "and" + ' ' + "those" + ' ' + "who" + ' ' + "don't" + '.';
    });

    BenchmarkAverage("string.append", iterCount, [=]() {
        return std::string("There").append(1, ' ').append("are").append(1, ' ').append("only").append(1, ' ').append(std::to_string(10)).append(1, ' ').append("people").append(1, ' ').append("in").append(1, ' ').append("the").append(1, ' ').append("world").append(1, ':').append(1, ' ').append("those").append(1, ' ').append("who").append(1, ' ').append("know").append(1, ' ').append("binary").append(1, ' ').append("and").append(1, ' ').append("those").append(1, ' ').append("who").append(1, ' ').append("don't").append(1, '.');
    });

    BenchmarkAverage("stringstream", iterCount, [=]() {
        std::stringstream ss;
        ss << "There" << ' ' << "are" << ' ' << "only" << ' ' << 10 << ' ' << "people" << ' ' << "in" << ' ' << "the" << ' ' << "world" << ':' << ' ' << "those" << ' ' << "who" << ' ' << "know" << ' ' << "binary" << ' ' << "and" << ' ' << "those" << ' ' << "who" << ' ' << "don't" << '.';
        return ss.str();
    });

    BenchmarkAverage("static stringstream", iterCount, [=]() {
        static std::stringstream ss;
        ss.str("");
        ss << "There" << ' ' << "are" << ' ' << "only" << ' ' << 10 << ' ' << "people" << ' ' << "in" << ' ' << "the" << ' ' << "world" << ':' << ' ' << "those" << ' ' << "who" << ' ' << "know" << ' ' << "binary" << ' ' << "and" << ' ' << "those" << ' ' << "who" << ' ' << "don't" << '.';
        return ss.str();
    });

    char buf[81 + 1];
    BenchmarkAverage("strstream", iterCount, [=, &buf]() {
        std::strstream ss{buf, sizeof(buf)/sizeof(buf[0])};
        ss << "There" << ' ' << "are" << ' ' << "only" << ' ' << 10 << ' ' << "people" << ' ' << "in" << ' ' << "the" << ' ' << "world" << ':' << ' ' << "those" << ' ' << "who" << ' ' << "know" << ' ' << "binary" << ' ' << "and" << ' ' << "those" << ' ' << "who" << ' ' << "don't" << '.' << std::ends;
        return ss.str();
    });
}

//static void escape(void* p) { asm volatile("" : : "g"(p) : "memory"); }

#ifdef __GNUC__
#define likely(x)       __builtin_expect(!!(x), 1)
#define unlikely(x)     __builtin_expect(!!(x), 0)
#else
#define likely(x)       (x)
#define unlikely(x)     (x)
#endif

void prefetchWrite(const void* p)
{
#ifdef __GNUC__
    __builtin_prefetch(p, 1);
#else
    _mm_prefetch((const char*)p, _MM_HINT_T0);
#endif
}

template<bool Prefetch>
struct SbS
{
    std::unique_ptr<char[]> data;
    size_t consumed = 0;

    SbS(size_t maxSize) :
        //data{std::make_unique<char[]>(maxSize)}
        data{new char[maxSize]}
    { }

    void operator<<(const char* str) {
        if (Prefetch) prefetchWrite(data.get() + consumed);
        const auto size = std::char_traits<char>::length(str);
        std::char_traits<char>::copy(data.get() + consumed, str, size);
        consumed += size;
    }

    std::string str() const {
        return {data.get(), data.get() + consumed};
    }
};

template<bool Prefetch, bool Likely>
struct SbSR
{
    std::unique_ptr<char[]> data;
    size_t consumed = 0;
    size_t reserved;

    SbSR(size_t maxSize) :
        data{new char[maxSize]},
        reserved{maxSize}
    { }

    void operator<<(const char* str) {
        if (Prefetch) prefetchWrite(data.get() + consumed);
        const auto size = std::char_traits<char>::length(str);
        if (Likely)
        {
            if (unlikely(consumed + size > reserved)) {
                // Will not happen, but don't tell it to the compiler.
                std::char_traits<char>::assign(data.get(), reserved, '\0');
                consumed = 0;
            }
        }
        else
        {
            if (consumed + size > reserved) {
                // Will not happen, but don't tell it to the compiler.
                std::char_traits<char>::assign(data.get(), reserved, '\0');
                consumed = 0;
            }
        }
        std::char_traits<char>::copy(data.get() + consumed, str, size);
        consumed += size;
    }

    std::string str() const {
        return {data.get(), data.get() + consumed};
    }
};

template<bool Prefetch>
struct SbC
{
    struct Chunk
    {
        size_t consumed;
        char data[1];
    };

    std::unique_ptr<Chunk> chunk;

    SbC(size_t maxSize) :
        chunk{reinterpret_cast<Chunk*>(new uint8_t[sizeof(Chunk::consumed) +  maxSize])}
    {
        chunk->consumed = 0;
    }

    void operator<<(const char* str) {
        if (Prefetch) prefetchWrite(chunk->data + chunk->consumed);
        const auto size = std::char_traits<char>::length(str);
        std::char_traits<char>::copy(chunk->data + chunk->consumed, str, size);
        chunk->consumed += size;
    }

    std::string str() const {
        return {chunk->data, chunk->data + chunk->consumed};
    }
};

template<bool Prefetch, bool Likely>
struct SbCR
{
    struct Chunk
    {
        size_t consumed;
        size_t reserved;
        char data[1];
    };

    std::unique_ptr<Chunk> chunk;

    SbCR(size_t maxSize) :
        chunk{reinterpret_cast<Chunk*>(new uint8_t[sizeof(Chunk) - sizeof(Chunk::data) +  maxSize])}
    {
        chunk->consumed = 0;
        chunk->reserved = maxSize;
    }

    void operator<<(const char* str) {
        if (Prefetch) prefetchWrite(chunk->data + chunk->consumed);
        const auto size = std::char_traits<char>::length(str);
        if (Likely)
        {
            if (unlikely(chunk->consumed + size > chunk->reserved)) {
                // Will not happen, but don't tell it to the compiler.
                std::char_traits<char>::assign(chunk->data, chunk->reserved, '\0');
                chunk->consumed = 0;
            }
        }
        else
        {
            if (chunk->consumed + size > chunk->reserved) {
                // Will not happen, but don't tell it to the compiler.
                std::char_traits<char>::assign(chunk->data, chunk->reserved, '\0');
                chunk->consumed = 0;
            }
        }
        std::char_traits<char>::copy(chunk->data + chunk->consumed, str, size);
        chunk->consumed += size;
    }

    std::string str() const {
        return {chunk->data, chunk->data + chunk->consumed};
    }
};

template<bool Prefetch>
struct SbT
{
    std::unique_ptr<char[]> data;
    char* tail;

    SbT(size_t maxSize) :
        data{new char[maxSize]},
        tail{data.get()}
    { }

    void operator<<(const char* str) {
        if (Prefetch) prefetchWrite(tail);
        const auto size = std::char_traits<char>::length(str);
        std::char_traits<char>::copy(tail, str, size);
        tail += size;
    }

    std::string str() const {
        return {data.get(), tail};
    }
};

template<bool Prefetch, bool Likely>
struct SbTR
{
    std::unique_ptr<char[]> data;
    char* tail;
    size_t spaceLeft;

    SbTR(size_t maxSize) :
        data{new char[maxSize]},
        tail{data.get()},
        spaceLeft{maxSize}
    { }

    void operator<<(const char* str) {
        if (Prefetch) prefetchWrite(tail);
        const auto size = std::char_traits<char>::length(str);
        if (Likely)
        {
            if (unlikely(spaceLeft < size)) {
                // Will not happen, but don't tell it to the compiler.
                spaceLeft += tail - data.get();
                tail = data.get();
                std::char_traits<char>::assign(tail, spaceLeft, '\0');
            }
        }
        else
        {
            if (spaceLeft < size) {
                // Will not happen, but don't tell it to the compiler.
                spaceLeft += tail - data.get();
                tail = data.get();
                std::char_traits<char>::assign(tail, spaceLeft, '\0');
            }
        }
        std::char_traits<char>::copy(tail, str, size);
        tail += size;
        spaceLeft -= size;
    }

    std::string str() const {
        return {data.get(), tail};
    }
};


template<typename SbT>
void benchmarkAppend(const char* title)
{
    const size_t iterCount = 200;
    const size_t wordCount = 10000;
    const char* word = "Batman! ";
    const size_t maxSize = std::char_traits<char>::length(word) * wordCount;

    BenchmarkMean(title, iterCount, [=]() {
        SbT sb{maxSize};
        for (size_t i = 0; i < wordCount; ++i) {
            sb << word;
        }
        return sb.str();
    });
}

void benchmarkAppend()
{
    std::cout << "Scenario: Append" << std::endl;

    benchmarkAppend<SbS<false>>("data[consumed++]");
    benchmarkAppend<SbS<true>>("prefetch data[consumed++]");
    benchmarkAppend<SbSR<false, false>>("if(spaceLeft) data[consumed++]");
    benchmarkAppend<SbSR<true, false>>("prefetch if(spaceLeft) data[consumed++]");
#ifdef __GNUC__
    benchmarkAppend<SbSR<false, true>>("if(likely spaceLeft) data[consumed++]");
    benchmarkAppend<SbSR<true, true>>("prefetch if(likely spaceLeft) data[consumed++]");
#endif
    std::cout << std::endl;

    benchmarkAppend<SbC<false>>("chunk->data[chunk->consumed++]");
    benchmarkAppend<SbC<true>>("prefetch chunk->data[chunk->consumed++]");
    benchmarkAppend<SbCR<false, false>>("if(spaceLeft) chunk->data[chunk->consumed++]");
    benchmarkAppend<SbCR<true, false>>("prefetch if(spaceLeft) chunk->data[chunk->consumed++]");
#ifdef __GNUC__
    benchmarkAppend<SbCR<false, true>>("if(likely spaceLeft) chunk->data[chunk->consumed++]");
    benchmarkAppend<SbCR<true, true>>("prefetch if(likely spaceLeft) chunk->data[chunk->consumed++]");
#endif
    std::cout << std::endl;

    benchmarkAppend<SbT<false>>("tail++");
    benchmarkAppend<SbT<true>>("prefetch tail++");
    benchmarkAppend<SbTR<false, false>>("if(spaceLeft) tail++");
    benchmarkAppend<SbTR<true, false>>("prefetch if(spaceLeft) tail++");
#ifdef __GNUC__
    benchmarkAppend<SbTR<false, true>>("if(likely spaceLeft) tail++");
    benchmarkAppend<SbTR<true, true>>("prefetch if(likely spaceLeft) tail++");
#endif
}

int main(const int argc, const char* const argv[])
{
    benchmarkIntegerSequence();
    benchmarkBook();
    benchmarkQuote();
    benchmarkAppend();
}
