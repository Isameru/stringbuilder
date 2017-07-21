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

volatile size_t vsize;
volatile const char* vcstr;

template<typename MethodT>
void BenchmarkMean(const std::string& title, const size_t iterCount, const size_t microIterCount, MethodT method) {
    using Clock = std::chrono::high_resolution_clock;

    for (size_t iter = 0; iter < iterCount * microIterCount; ++iter)
    {
        std::string str = method();
        vsize = str.size();
        vcstr = str.c_str();
    }

    std::vector<Clock::duration> durations;
    durations.reserve(iterCount);

    for (size_t iter = 0; iter < iterCount; ++iter)
    {
        const auto time0 = Clock::now();

        for (int i = 0; i < microIterCount; ++i)
        {
            std::string str = method();
            vsize = str.size();
            vcstr = str.c_str();
        }

        const auto elapsed = Clock::now() - time0;
        durations.push_back(elapsed / microIterCount);
    }

    std::sort(std::begin(durations), std::end(durations));
    const auto meanDuration = durations[durations.size() / 2];

    if (meanDuration < std::chrono::microseconds{10})
    {
        std::cout << "    " << title << ": " << std::chrono::duration_cast<std::chrono::nanoseconds>(meanDuration).count() << " ns" << std::endl;
    }
    else
    {
        std::cout << "    " << title << ": " << std::chrono::duration_cast<std::chrono::microseconds>(meanDuration).count() << " us" << std::endl;
    }
};

template<typename MethodT>
void BenchmarkAverage(const char* title, const size_t iterCount, MethodT method) {
    using Clock = std::chrono::high_resolution_clock;

    for (size_t iter = 0; iter < iterCount; ++iter)
    {
        std::string str = method();
        vsize = str.size();
        vcstr = str.c_str();
    }

    const auto time0 = Clock::now();

    for (size_t iter = 0; iter < iterCount; ++iter)
    {
        std::string str = method();
        vsize = str.size();
        vcstr = str.c_str();
    }

    const auto elapsed = Clock::now() - time0;

    std::cout << "    " << title << ": " << std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed).count() / iterCount << " ns" << std::endl;
};

void benchmarkIntegerSequence()
{
    std::cout << "Scenario: IntegerSequence" << std::endl;

    constexpr size_t iterCount = 3000;
    constexpr int span = 1000;

    BenchmarkMean("inplace_stringbuilder<big>", iterCount, 1, [=]() {
        inplace_stringbuilder<8788> sb;
        for (int i = -span; i <= span; ++i) {
            sb << i;
            sb << ' ';
        }
        return sb.str();
    });

    BenchmarkMean("stringbuilder<>", iterCount, 1, [=]() {
        stringbuilder<> sb;
        for (int i = -span; i < span; ++i) {
            sb << i;
            sb << ' ';
        }
        return sb.str();
    });

    BenchmarkMean("stringbuilder<> with reserve", iterCount, 1, [=]() {
        stringbuilder<> sb;
        sb.reserve(8788);
        for (int i = -span; i < span; ++i) {
            sb << i;
            sb << ' ';
        }
        return sb.str();
    });

    BenchmarkMean("stringbuilder<big>", iterCount, 1, [=]() {
        stringbuilder<8788> sb;
        for (int i = -span; i < span; ++i) {
            sb << i;
            sb << ' ';
        }
        return sb.str();
    });

    BenchmarkMean("string.append(a).append(b)", iterCount, 1, [=]() {
        std::string s;
        for (int i = -span; i <= span; ++i) {
            s.append(std::to_string(i)).append(1, ' ');
        }
        return s;
    });

    BenchmarkMean("string.append(a+b)", iterCount, 1, [=]() {
        std::string s;
        for (int i = -span; i <= span; ++i) {
            s.append(std::to_string(i) + ' ');
        }
        return s;
    });

    BenchmarkMean("stringstream", iterCount, 1, [=]() {
        std::stringstream ss;
        for (int i = -span; i <= span; ++i) {
            ss << i << ' ';
        }
        return ss.str();
    });

    BenchmarkMean("static stringstream", iterCount, 1, [=]() {
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
        BenchmarkMean("strstream", iterCount, 1, [=, &buf]() {
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

    BenchmarkMean("stringbuilder<> << *", iterCount, 1, [=]() {
        stringbuilder<> sb;
        for (size_t i = 0; i < wordCount; ++i) {
            sb << dictionary[i % dictionarySize] << ' ';
        }
        return sb.str();
    });

    BenchmarkMean("stringbuilder<> << [N]", iterCount, 1, [=]() {
        stringbuilder<> sb;
        for (size_t i = 0; i < wordCount; i += dictionarySize) {
            sb << "Evolution" << ' ' << "brings" << ' ' << "human" << ' ' << "beings." << ' ' << "Human" << ' ' << "beings," << ' ' << "through" << ' ' << "a" << ' ' << "long" << ' ' << "and" << ' ' << "painful" << ' ' << "process," << ' ' << "bring" << ' ' << "humanity.";
        }
        return sb.str();
    });

    BenchmarkMean("stringbuilder<> with reserve << *", iterCount, 1, [=]() {
        stringbuilder<> sb;
        sb.reserve(2771432);
        for (size_t i = 0; i < wordCount; ++i) {
            sb << dictionary[i % dictionarySize] << ' ';
        }
        return sb.str();
    });

    BenchmarkMean("stringbuilder<> with reserve << [N]", iterCount, 1, [=]() {
        stringbuilder<> sb;
        sb.reserve(2771432);
        for (size_t i = 0; i < wordCount; i += dictionarySize) {
            sb << "Evolution" << ' ' << "brings" << ' ' << "human" << ' ' << "beings." << ' ' << "Human" << ' ' << "beings," << ' ' << "through" << ' ' << "a" << ' ' << "long" << ' ' << "and" << ' ' << "painful" << ' ' << "process," << ' ' << "bring" << ' ' << "humanity.";
        }
        return sb.str();
    });

    BenchmarkMean("stringbuilder<4kB> << *", iterCount, 1, [=]() {
        stringbuilder<4 * 1024> sb;
        for (size_t i = 0; i < wordCount; ++i) {
            sb << dictionary[i % dictionarySize] << ' ';
        }
        return sb.str();
    });

    BenchmarkMean("stringbuilder<4kB> << [N]", iterCount, 1, [=]() {
        stringbuilder<4 * 1024> sb;
        for (size_t i = 0; i < wordCount; i += dictionarySize) {
            sb << "Evolution" << ' ' << "brings" << ' ' << "human" << ' ' << "beings." << ' ' << "Human" << ' ' << "beings," << ' ' << "through" << ' ' << "a" << ' ' << "long" << ' ' << "and" << ' ' << "painful" << ' ' << "process," << ' ' << "bring" << ' ' << "humanity.";
        }
        return sb.str();
    });

    BenchmarkMean("stringbuilder<64kB> << *", iterCount, 1, [=]() {
        stringbuilder<64 * 1024> sb;
        for (size_t i = 0; i < wordCount; ++i) {
            sb << dictionary[i % dictionarySize] << ' ';
        }
        return sb.str();
    });

    BenchmarkMean("stringbuilder<64kB> << [N]", iterCount, 1, [=]() {
        stringbuilder<64 * 1024> sb;
        for (size_t i = 0; i < wordCount; i += dictionarySize) {
            sb << "Evolution" << ' ' << "brings" << ' ' << "human" << ' ' << "beings." << ' ' << "Human" << ' ' << "beings," << ' ' << "through" << ' ' << "a" << ' ' << "long" << ' ' << "and" << ' ' << "painful" << ' ' << "process," << ' ' << "bring" << ' ' << "humanity.";
        }
        return sb.str();
    });

    BenchmarkMean("stringbuilder<512kB> << *", iterCount, 1, [=]() {
        stringbuilder<512 * 1024> sb;
        for (size_t i = 0; i < wordCount; ++i) {
            sb << dictionary[i % dictionarySize] << ' ';
        }
        return sb.str();
    });

    BenchmarkMean("stringbuilder<512kB> << [N]", iterCount, 1, [=]() {
        stringbuilder<512 * 1024> sb;
        for (size_t i = 0; i < wordCount; i += dictionarySize) {
            sb << "Evolution" << ' ' << "brings" << ' ' << "human" << ' ' << "beings." << ' ' << "Human" << ' ' << "beings," << ' ' << "through" << ' ' << "a" << ' ' << "long" << ' ' << "and" << ' ' << "painful" << ' ' << "process," << ' ' << "bring" << ' ' << "humanity.";
        }
        return sb.str();
    });

    BenchmarkMean("string.append(*).append(char)", iterCount, 1, [=]() {
        std::string s;
        for (size_t i = 0; i < wordCount; ++i) {
            s.append(dictionary[i % dictionarySize]);
            s.append(1, ' ');
        }
        return s;
    });

    BenchmarkMean("string.append(str+char)", iterCount, 1, [=]() {
        std::string s;
        for (size_t i = 0; i < wordCount; ++i) {
            s.append(std::string(dictionary[i % dictionarySize]) + ' ');
        }
        return s;
    });

    BenchmarkMean("stringstream << *", iterCount, 1, [=]() {
        std::stringstream ss;
        for (size_t i = 0; i < wordCount; ++i) {
            ss << dictionary[i % dictionarySize] << ' ';
        }
        return ss.str();
    });

    BenchmarkMean("stringstream << [N]", iterCount, 1, [=]() {
        std::stringstream ss;
        for (size_t i = 0; i < wordCount; i += dictionarySize) {
            ss << "Evolution" << ' ' << "brings" << ' ' << "human" << ' ' << "beings." << ' ' << "Human" << ' ' << "beings," << ' ' << "through" << ' ' << "a" << ' ' << "long" << ' ' << "and" << ' ' << "painful" << ' ' << "process," << ' ' << "bring" << ' ' << "humanity.";
        }
        return ss.str();
    });

    BenchmarkMean("static stringstream << *", iterCount, 1, [=]() {
        static std::stringstream ss;
        ss.str("");
        for (size_t i = 0; i < wordCount; ++i) {
            ss << dictionary[i % dictionarySize] << ' ';
        }
        return ss.str();
    });

    BenchmarkMean("static stringstream << [N]", iterCount, 1, [=]() {
        static std::stringstream ss;
        ss.str("");
        for (size_t i = 0; i < wordCount; i += dictionarySize) {
            ss << "Evolution" << ' ' << "brings" << ' ' << "human" << ' ' << "beings." << ' ' << "Human" << ' ' << "beings," << ' ' << "through" << ' ' << "a" << ' ' << "long" << ' ' << "and" << ' ' << "painful" << ' ' << "process," << ' ' << "bring" << ' ' << "humanity.";
        }
        return ss.str();
    });

    {
        constexpr int bufSize = 2771432 + 1;
        auto buf = std::make_unique<char[]>(bufSize);
        BenchmarkMean("strstream << *", iterCount, 1, [=, &buf]() {
            std::strstream ss{buf.get(), bufSize};
            for (size_t i = 0; i < wordCount; ++i) {
                ss << dictionary[i % dictionarySize] << ' ';
            }
            ss << std::ends;
            return ss.str();
        });
    }
}

const char* g_joke = nullptr;

void benchmarkQuote()
{
    //const char* const words[33] = { "There", " ", "are", " ", "only", " ", "10", " ", "people", " ", "in", " ", "the", " ", "world", ":", " ", "those", " ", "who", " ", "know", " ", "binary", " ", "and", " ", "those", " ", "who", " ", "don't", "." };
    std::array<const char*, 33> words { "There", " ", "are", " ", "only", " ", "10", " ", "people", " ", "in", " ", "the", " ", "world", ":", " ", "those", " ", "who", " ", "know", " ", "binary", " ", "and", " ", "those", " ", "who", " ", "don't", "." };

    std::cout << "Scenario: Quote" << std::endl;

    constexpr size_t iterCount = 500'000;

    BenchmarkAverage("empty", iterCount, [=]() {
        return std::string{};
    });

    BenchmarkAverage("empty + computing length", iterCount, [=]() {
        vsize = std::char_traits<char>::length(g_joke);
        return std::string{};
    });

    BenchmarkAverage("just string", iterCount, [=]() {
        //return std::string("There are only 10 people in the world: those who know binary and those who don't.");
        return std::string(g_joke);
    });

    BenchmarkAverage("just string with known size", iterCount, [=]() {
        //return std::string("There are only 10 people in the world: those who know binary and those who don't.", 81);
        return std::string(g_joke, 81);
    });

    BenchmarkAverage("constexpr = make_string", iterCount, [=]() {
        constexpr auto constexpr_quote = make_string("There", " ", "are", " ", "only", " ", "10", " ", "people", " ", "in", " ", "the", " ", "world", ":", " ", "those", " ", "who", " ", "know", " ", "binary", " ", "and", " ", "those", " ", "who", " ", "don't", ".");
        return constexpr_quote.str();
    });

    BenchmarkAverage("make_string(constexpr)", iterCount, [=]() {
        return make_string("There", " ", "are", " ", "only", " ", "10", " ", "people", " ", "in", " ", "the", " ", "world", ":", " ", "those", " ", "who", " ", "know", " ", "binary", " ", "and", " ", "those", " ", "who", " ", "don't", ".").str();
    });

    BenchmarkAverage("make_string", iterCount, [=]() {
        return make_string("There", " ", "are", " ", "only", " ", sized_str<2>("10"), " ", "people", " ", "in", " ", "the", " ", "world", ":", " ", "those", " ", "who", " ", "know", " ", "binary", " ", "and", " ", "those", " ", "who", " ", "don't", ".");
    });

    BenchmarkAverage("stringbuilder<>([N])", iterCount, [=]() {
        stringbuilder<> sb;
        sb << "There" << " " << "are" << " " << "only" << " " << "10" << " " << "people" << " " << "in" << " " << "the" << " " << "world" << ":" << " " << "those" << " " << "who" << " " << "know" << " " << "binary" << " " << "and" << " " << "those" << " " << "who" << " " << "don't" << ".";
        return sb.str();
    });

    BenchmarkAverage("stringbuilder<>(*)", iterCount, [=]() {
        stringbuilder<> sb;
        for (const char* word : words)
        {
            sb << word;
        }
        return sb.str();
    });

    BenchmarkAverage("stringbuilder<81>([N])", iterCount, [=]() {
        stringbuilder<81> sb;
        sb << "There" << " " << "are" << " " << "only" << " " << "10" << " " << "people" << " " << "in" << " " << "the" << " " << "world" << ":" << " " << "those" << " " << "who" << " " << "know" << " " << "binary" << " " << "and" << " " << "those" << " " << "who" << " " << "don't" << ".";
        return sb.str();
    });

    BenchmarkAverage("stringbuilder<81>(*)", iterCount, [=]() {
        stringbuilder<81> sb;
        for (const char* word : words)
        {
            sb << word;
        }
        return sb.str();
    });

    BenchmarkAverage("inplace_stringbuilder<81>([N])", iterCount, [=]() {
        inplace_stringbuilder<81> sb;
        sb << "There" << " " << "are" << " " << "only" << " " << "10" << " " << "people" << " " << "in" << " " << "the" << " " << "world" << ":" << " " << "those" << " " << "who" << " " << "know" << " " << "binary" << " " << "and" << " " << "those" << " " << "who" << " " << "don't" << ".";
        return sb.str();
    });

    BenchmarkAverage("inplace_stringbuilder<81>(*)", iterCount, [=]() {
        inplace_stringbuilder<81> sb;
        for (const char* word : words)
        {
            sb << word;
        }
        return sb.str();
    });

    BenchmarkAverage("string + string (loop)", iterCount, [=]() {
        std::string text;
        for (const char* word : words)
        {
            text = text + word;
        }
        return text;
    });

    BenchmarkAverage("string + string (expression)", iterCount, [=]() {
        return std::string("There") + " " + "are" + " " + "only" + " " + "10" + " " + "people" + " " + "in" + " " + "the" + " " + "world" + ":" + " " + "those" + " " + "who" + " " + "know" + " " + "binary" + " " + "and" + " " + "those" + " " + "who" + " " + "don't" + ".";
    });

    BenchmarkAverage("string.append (loop)", iterCount, [=]() {
        std::string text;
        for (const char* const word : words)
        {
            text.append(word);
        }
        return text;
    });

    BenchmarkAverage("string.append with reserve (loop)", iterCount, [=]() {
        std::string text;
        text.reserve(81);
        for (const char* const word : words)
        {
            text.append(word);
        }
        return text;
    });

    BenchmarkAverage("string.append (chain)", iterCount, [=]() {
        return std::string("There").append(" ").append("are").append(" ").append("only").append(" ").append("10").append(" ").append("people").append(" ").append("in").append(" ").append("the").append(" ").append("world").append(":").append(" ").append("those").append(" ").append("who").append(" ").append("know").append(" ").append("binary").append(" ").append("and").append(" ").append("those").append(" ").append("who").append(" ").append("don't").append(".");
    });

    BenchmarkAverage("string.append with reserve (chain)", iterCount, [=]() {
        std::string text;
        text.reserve(81);
        return text.append("There").append(" ").append("are").append(" ").append("only").append(" ").append("10").append(" ").append("people").append(" ").append("in").append(" ").append("the").append(" ").append("world").append(":").append(" ").append("those").append(" ").append("who").append(" ").append("know").append(" ").append("binary").append(" ").append("and").append(" ").append("those").append(" ").append("who").append(" ").append("don't").append(".");
    });

    BenchmarkAverage("stringstream << *", iterCount, [=]() {
        std::stringstream ss;
        for (const char* const word : words)
        {
            ss << word;
        }
        return ss.str();
    });

    BenchmarkAverage("stringstream << [N]", iterCount, [=]() {
        std::stringstream ss;
        ss << "There" << " " << "are" << " " << "only" << " " << "10" << " " << "people" << " " << "in" << " " << "the" << " " << "world" << ":" << " " << "those" << " " << "who" << " " << "know" << " " << "binary" << " " << "and" << " " << "those" << " " << "who" << " " << "don't" << ".";
        return ss.str();
    });

    BenchmarkAverage("static stringstream << *", iterCount, [=]() {
        static std::stringstream ss;
        ss.str("");
        for (const char* word : words)
        {
            ss << word;
        }
        return ss.str();
    });

    BenchmarkAverage("static stringstream << [N]", iterCount, [=]() {
        static std::stringstream ss;
        ss.str("");
        ss << "There" << " " << "are" << " " << "only" << " " << "10" << " " << "people" << " " << "in" << " " << "the" << " " << "world" << ":" << " " << "those" << " " << "who" << " " << "know" << " " << "binary" << " " << "and" << " " << "those" << " " << "who" << " " << "don't" << ".";
        return ss.str();
    });

    char buf[81 + 1];
    BenchmarkAverage("strstream", iterCount, [=, &buf]() {
        std::strstream ss{buf, sizeof(buf)/sizeof(buf[0])};
        for (const char* word : words)
        {
            ss << word;
        }
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

template<size_t InPlaceSize, bool Prefetch>
struct SbI
{
    char data[InPlaceSize + 1];
    size_t consumed = 0;

    SbI(size_t)
    { }

    template<typename T>
    SbI& operator<<(const T& str) {
        if (Prefetch) prefetchWrite(data + consumed);
        const auto size = std::char_traits<char>::length(str);
        std::char_traits<char>::copy(data + consumed, str, size);
        consumed += size;
        return *this;
    }

    template<size_t N>
    SbI& operator<<(const char (&str)[N]) {
        std::char_traits<char>::copy(data + consumed, str, N-1);
        consumed += N-1;
        return *this;
    }

    std::string str() const {
        return { data, data + consumed };
    }
};

template<bool Prefetch>
struct SbS
{
    std::unique_ptr<char[]> data;
    size_t consumed = 0;

    SbS(size_t maxSize) :
        data{std::make_unique<char[]>(maxSize)}
        //data{new char[maxSize]}
    { }

    SbS& operator<<(const char* str) {
        if (Prefetch) prefetchWrite(data.get() + consumed);
        const auto size = std::char_traits<char>::length(str);
        std::char_traits<char>::copy(data.get() + consumed, str, size);
        consumed += size;
        return *this;
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

    SbSR& operator<<(const char* str) {
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
        return *this;
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

    SbC& operator<<(const char* str) {
        if (Prefetch) prefetchWrite(chunk->data + chunk->consumed);
        const auto size = std::char_traits<char>::length(str);
        std::char_traits<char>::copy(chunk->data + chunk->consumed, str, size);
        chunk->consumed += size;
        return *this;
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

    SbCR& operator<<(const char* str) {
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
        return *this;
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

    SbT& operator<<(const char* str) {
        if (Prefetch) prefetchWrite(tail);
        const auto size = std::char_traits<char>::length(str);
        std::char_traits<char>::copy(tail, str, size);
        tail += size;

        return *this;
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

    SbTR& operator<<(const char* str) {
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
        return *this;
    }

    std::string str() const {
        return {data.get(), tail};
    }
};


template<typename SbT>
void benchmarkAppend(const std::string& title)
{
    const size_t iterCount = 1000;

    //const char* const words[33] = { "There", " ", "are", " ", "only", " ", "10", " ", "people", " ", "in", " ", "the", " ", "world", ":", " ", "those", " ", "who", " ", "know", " ", "binary", " ", "and", " ", "those", " ", "who", " ", "don't", "." };
    std::array<const char*, 33> words { "There", " ", "are", " ", "only", " ", "10", " ", "people", " ", "in", " ", "the", " ", "world", ":", " ", "those", " ", "who", " ", "know", " ", "binary", " ", "and", " ", "those", " ", "who", " ", "don't", "." };
    const size_t maxSize = 81;

    BenchmarkMean(title + " << *", iterCount, 1000, [=]() {
        SbT sb{maxSize};
        for (const char* const word : words) {
            sb << word;
        }
        return sb.str();
    });

    BenchmarkMean(title + " << [N]", iterCount, 1000, [=]() {
        SbT sb{maxSize};
        sb << "There" << " " << "are" << " " << "only" << " " << "10" << " " << "people" << " " << "in" << " " << "the" << " " << "world" << ":" << " " << "those" << " " << "who" << " " << "know" << " " << "binary" << " " << "and" << " " << "those" << " " << "who" << " " << "don't" << ".";
        return sb.str();
    });
}

void benchmarkAppend()
{
    std::cout << "Scenario: Append" << std::endl;

    benchmarkAppend<SbI<81, false>>("inplace data[consumed++]");
    benchmarkAppend<SbI<81, true>>("inplace prefetch data[consumed++]");
    std::cout << std::endl;

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

template<typename Method, size_t I>
void foreach_integral_constant(std::integer_sequence<size_t, I>, Method method)
{
    method(std::integral_constant<size_t, I>{});
}

template<typename Method, size_t I0, size_t... IX>
void foreach_integral_constant(std::integer_sequence<size_t, I0, IX...>, Method method)
{
    method(std::integral_constant<size_t, I0>{});
    foreach_integral_constant(std::integer_sequence<size_t, IX...>{}, method);
}

void benchmarkProgressiveAppend()
{
    std::cout << "Scenario: ProgressiveAppend (words)" << std::endl;
    const std::array<const char* const, 33> words{ "There", " ", "are", " ", "only", " ", "10", " ", "people", " ", "in", " ", "the", " ", "world", ":", " ", "those", " ", "who", " ", "know", " ", "binary", " ", "and", " ", "those", " ", "who", " ", "don't", ". " };

    foreach_integral_constant(
        std::integer_sequence<size_t, 64, 512, 4098, 32 * 1024, 128 * 1024, 512 * 1024>{},
        [=](auto inPlaceSizeIC) {
            constexpr auto inPlaceSize = inPlaceSizeIC.value;

            BenchmarkMean(make_string("inplace_stringbuilder<", inPlaceSize, ">.append_c_str(*)"), 100, 10, [=]() {
                constexpr auto inPlaceSize = inPlaceSizeIC.value;
                inplace_stringbuilder<inPlaceSize> sb;

                for (size_t i = 0; sb.length() + 8 < inPlaceSize; ++i)
                {
                    sb.append_c_str(words[i % words.size()]);
                }

                return sb.str();
            });

            BenchmarkMean(make_string("inplace_stringbuilder<", inPlaceSize, ">.append_c_str_progressive(*)"), 100, 10, [=]() {
                constexpr auto inPlaceSize = inPlaceSizeIC.value;
                inplace_stringbuilder<inPlaceSize> sb;

                for (size_t i = 0; sb.length() + 8 < inPlaceSize; ++i)
                {
                    sb.append_c_str_progressive(words[i % words.size()]);
                }

                return sb.str();
            });
        }
    );

    std::cout << "Scenario: ProgressiveAppend (sentences)" << std::endl;

    foreach_integral_constant(
        std::integer_sequence<size_t, 512, 2048, 2*4098, 64*1024, 128*1024, 256*1024, 512*1024>{},
        [=](auto inPlaceSizeIC) {
            constexpr auto inPlaceSize = inPlaceSizeIC.value;

            BenchmarkMean(make_string("inplace_stringbuilder<", inPlaceSize, ">.append_c_str(*)"), 100, 10, [=]() {
                constexpr auto inPlaceSize = inPlaceSizeIC.value;
                inplace_stringbuilder<inPlaceSize> sb;

                for (size_t i = 0; sb.length() + 81 < inPlaceSize; ++i)
                {
                    sb.append_c_str(g_joke);
                }

                return sb.str();
            });

            BenchmarkMean(make_string("inplace_stringbuilder<", inPlaceSize, ">.append_c_str(*,81)"), 100, 10, [=]() {
                constexpr auto inPlaceSize = inPlaceSizeIC.value;
                inplace_stringbuilder<inPlaceSize> sb;

                for (size_t i = 0; sb.length() + 81 < inPlaceSize; ++i)
                {
                    sb.append_c_str(g_joke, 81);
                }

                return sb.str();
            });

            BenchmarkMean(make_string("inplace_stringbuilder<", inPlaceSize, ">.append_c_str_progressive(*)"), 100, 10, [=]() {
                constexpr auto inPlaceSize = inPlaceSizeIC.value;
                inplace_stringbuilder<inPlaceSize> sb;

                for (size_t i = 0; sb.length() + 81 < inPlaceSize; ++i)
                {
                    sb.append_c_str_progressive(g_joke);
                }

                return sb.str();
            });
        }
    );
}

//#include <iomanip>
//template<typename T>
//void dumpHex(const T& v)
//{
//    int s = sizeof(v);
//    for (int i = 0; i < s; ++i)
//    {
//        std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)(reinterpret_cast<const uint8_t*>(&v)[i]) << ' ';
//    }
//}

int main(const int argc, const char* const argv[])
{
    {   inplace_stringbuilder<81> jokess;
        jokess << "There are only 10 people in the world: those who know binary and those who don't.";
        g_joke = jokess.c_str();
    }

    do {
        benchmarkIntegerSequence();
        benchmarkBook();
        benchmarkQuote();
        benchmarkAppend();
        benchmarkProgressiveAppend();

        if (vsize == 0 || vcstr == nullptr) std::cout << "vsize == 0 || vcstr == nullptr" << std::endl;
    } while (false);

    return 0;
}
