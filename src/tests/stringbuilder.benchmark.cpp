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
#include <strstream>
#include <iostream>
#include <chrono>

template<typename MethodT>
void Benchmark(const char* title, const size_t iterCount, MethodT method) {
    using Clock = std::chrono::high_resolution_clock;
    auto time0 = Clock::now();
    for (size_t iter = 0; iter < iterCount; ++iter)
    {
        if (iter == iterCount / 4) time0 = Clock::now();
        std::string str = method();
        volatile size_t size = str.size();
    }
    const auto elapsed = Clock::now() - time0;
    std::cout << "    " << title << ": " << std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count() << " ms" << std::endl;
};

void benchmarkIntegerSequence()
{
    std::cout << "Scenario: IntegerSequence" << std::endl;

    constexpr size_t iterCount = 3000;
    const int span = 1000;

    Benchmark("inplace_stringbuilder<big>", iterCount, [=]() {
        inplace_stringbuilder<8788> sb;
        for (int i = -span; i <= span; ++i) {
            sb << i;
            sb << ' ';
        }
        return sb.str();
    });

    Benchmark("stringbuilder<>", iterCount, [=]() {
        stringbuilder<> sb;
        for (int i = -span; i < span; ++i) {
            sb << i;
            sb << ' ';
        }
        return sb.str();
    });

    Benchmark("stringbuilder<big>", iterCount, [=]() {
        stringbuilder<8788> sb;
        for (int i = -span; i < span; ++i) {
            sb << i;
            sb << ' ';
        }
        return sb.str();
    });

    Benchmark("string.append(a).append(b)", iterCount, [=]() {
        std::string s;
        for (int i = -span; i <= span; ++i) {
            s.append(std::to_string(i)).append(1, ' ');
        }
        return s;
    });

    Benchmark("string.append(a+b)", iterCount, [=]() {
        std::string s;
        for (int i = -span; i <= span; ++i) {
            s.append(std::to_string(i) + ' ');
        }
        return s;
    });

    Benchmark("stringstream", iterCount, [=]() {
        std::stringstream ss;
        for (int i = -span; i <= span; ++i) {
            ss << i << ' ';
        }
        return ss.str();
    });

    Benchmark("static stringstream", iterCount, [=]() {
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
        Benchmark("strstream", iterCount, [=, &buf]() {
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

    constexpr size_t iterCount = 15;
    constexpr size_t wordCount = 400'000;
    constexpr const char* dictionary[] = { "Evolution", "brings", "human", "beings.", "Human", "beings,", "through", "a", "long", "and", "painful", "process,", "bring", "humanity." };
    constexpr size_t dictionarySize = sizeof(dictionary) / sizeof(dictionary[0]);

    Benchmark("stringbuilder<>", iterCount, [=]() {
        stringbuilder<> sb;
        for (size_t i = 0; i < wordCount; ++i) {
            sb << dictionary[i % dictionarySize] << ' ';
        }
        return sb.str();
    });

    Benchmark("stringbuilder<4kB>", iterCount, [=]() {
        stringbuilder<4 * 1024> sb;
        for (size_t i = 0; i < wordCount; ++i) {
            sb << dictionary[i % dictionarySize] << ' ';
        }
        return sb.str();
    });

    Benchmark("stringbuilder<64kB>", iterCount, [=]() {
        stringbuilder<64 * 1024> sb;
        for (size_t i = 0; i < wordCount; ++i) {
            sb << dictionary[i % dictionarySize] << ' ';
        }
        return sb.str();
    });

    Benchmark("stringbuilder<512kB>", iterCount, [=]() {
        stringbuilder<512 * 1024> sb;
        for (size_t i = 0; i < wordCount; ++i) {
            sb << dictionary[i % dictionarySize] << ' ';
        }
        return sb.str();
    });

    Benchmark("string.append(a).append(b)", iterCount, [=]() {
        std::string s;
        for (size_t i = 0; i < wordCount; ++i) {
            s.append(dictionary[i % dictionarySize]);
            s.append(1, ' ');
        }
        return s;
    });

    Benchmark("string.append(a+b)", iterCount, [=]() {
        std::string s;
        for (size_t i = 0; i < wordCount; ++i) {
            s.append(std::string(dictionary[i % dictionarySize]) + ' ');
        }
        return s;
    });

    Benchmark("stringstream", iterCount, [=]() {
        std::stringstream ss;
        for (size_t i = 0; i < wordCount; ++i) {
            ss << dictionary[i % dictionarySize] << ' ';
        }
        return ss.str();
    });

    Benchmark("static stringstream", iterCount, [=]() {
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
        Benchmark("strstream", iterCount, [=, &buf]() {
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

    Benchmark("one string", iterCount, [=]() {
        return std::string("There are only 10 people in the world: those who know binary and those who don't.");
    });

    Benchmark("constexpr make_string", iterCount, [=]() {
        constexpr auto constexpr_quote = make_string("There", ' ', "are", ' ', "only", ' ', "10", ' ', "people", ' ', "in", ' ', "the", ' ', "world", ':', ' ', "those", ' ', "who", ' ', "know", ' ', "binary", ' ', "and", ' ', "those", ' ', "who", ' ', "don't", '.');
        return constexpr_quote.str();
    });

    Benchmark("make_string", iterCount, [=]() {
        return make_string("There", ' ', "are", ' ', "only", ' ', 10, ' ', "people", ' ', "in", ' ', "the", ' ', "world", ':', ' ', "those", ' ', "who", ' ', "know", ' ', "binary", ' ', "and", ' ', "those", ' ', "who", ' ', "don't", '.');
    });

    Benchmark("string+string", iterCount, [=]() {
        return std::string("There") + ' ' + "are" + ' ' + "only" + ' ' + std::to_string(10) + ' ' + "people" + ' ' + "in" + ' ' + "the" + ' ' + "world" + ':' + ' ' + "those" + ' ' + "who" + ' ' + "know" + ' ' + "binary" + ' ' + "and" + ' ' + "those" + ' ' + "who" + ' ' + "don't" + '.';
    });

    Benchmark("string.append", iterCount, [=]() {
        return std::string("There").append(1, ' ').append("are").append(1, ' ').append("only").append(1, ' ').append(std::to_string(10)).append(1, ' ').append("people").append(1, ' ').append("in").append(1, ' ').append("the").append(1, ' ').append("world").append(1, ':').append(1, ' ').append("those").append(1, ' ').append("who").append(1, ' ').append("know").append(1, ' ').append("binary").append(1, ' ').append("and").append(1, ' ').append("those").append(1, ' ').append("who").append(1, ' ').append("don't").append(1, '.');
    });

    Benchmark("stringstream", iterCount, [=]() {
        std::stringstream ss;
        ss << "There" << ' ' << "are" << ' ' << "only" << ' ' << 10 << ' ' << "people" << ' ' << "in" << ' ' << "the" << ' ' << "world" << ':' << ' ' << "those" << ' ' << "who" << ' ' << "know" << ' ' << "binary" << ' ' << "and" << ' ' << "those" << ' ' << "who" << ' ' << "don't" << '.';
        return ss.str();
    });

    Benchmark("static stringstream", iterCount, [=]() {
        static std::stringstream ss;
        ss.str("");
        ss << "There" << ' ' << "are" << ' ' << "only" << ' ' << 10 << ' ' << "people" << ' ' << "in" << ' ' << "the" << ' ' << "world" << ':' << ' ' << "those" << ' ' << "who" << ' ' << "know" << ' ' << "binary" << ' ' << "and" << ' ' << "those" << ' ' << "who" << ' ' << "don't" << '.';
        return ss.str();
    });

    char buf[81 + 1];
    Benchmark("strstream", iterCount, [=, &buf]() {
        std::strstream ss{buf, sizeof(buf)/sizeof(buf[0])};
        ss << "There" << ' ' << "are" << ' ' << "only" << ' ' << 10 << ' ' << "people" << ' ' << "in" << ' ' << "the" << ' ' << "world" << ':' << ' ' << "those" << ' ' << "who" << ' ' << "know" << ' ' << "binary" << ' ' << "and" << ' ' << "those" << ' ' << "who" << ' ' << "don't" << '.' << std::ends;
        return ss.str();
    });
}

int main(const int argc, const char* const argv[])
{
    benchmarkQuote();
    benchmarkBook();
    benchmarkIntegerSequence();
}
