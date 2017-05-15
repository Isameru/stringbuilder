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
#include <gtest/gtest.h>
#include <chrono>

TEST(InPlaceStringBuilder, Riddle)
{
    auto sb = inplace_stringbuilder<34>{};
    sb << "There" << ' ' << "are " << 8 << " bits in a " << "single byte" << '.';
    EXPECT_EQ(sb.size(), 34);
    EXPECT_EQ(std::to_string(sb), "There are 8 bits in a single byte.");
    ASSERT_STREQ(sb.c_str(), "There are 8 bits in a single byte.");
}

TEST(InPlaceStringBuilder, RiddleReversed)
{
    auto sb = inplace_stringbuilder<34, false>{};
    sb << "There" << ' ' << "are " << 8 << " bits in a " << "single byte" << '.';
    EXPECT_EQ(std::to_string(sb), ".single byte bits in a 8are  There");
    ASSERT_STREQ(sb.c_str(), ".single byte bits in a 8are  There");
}

TEST(InPlaceStringBuilder, EncodeInteger)
{
    {   auto sb = inplace_stringbuilder<1>{};
        sb << 0;
        EXPECT_EQ(std::to_string(sb), "0");
    }
    {   auto sb = inplace_stringbuilder<19>{};
        sb << std::numeric_limits<int64_t>::max();
        EXPECT_EQ(std::to_string(sb), "9223372036854775807");
    }
    {   auto sb = inplace_stringbuilder<20>{};
        sb << std::numeric_limits<int64_t>::min();
        EXPECT_EQ(std::to_string(sb), "-9223372036854775808");
    }
    {   auto sb = inplace_stringbuilder<20>{};
        sb << std::numeric_limits<uint64_t>::max();
        EXPECT_EQ(std::to_string(sb), "18446744073709551615");
    }
}

TEST(InPlaceStringBuilder, EncodeOther)
{
    {   auto sb = inplace_stringbuilder<11, false>{};
        sb << -123.4567;
        EXPECT_EQ(std::to_string(sb), "-123.456700");
    }
}

TEST(StringBuilder, Riddle_InPlace0)
{
    auto sb = stringbuilder<0>{};
    sb << "There" << ' ' << "are " << 8 << " bits in a " << "single byte" << '.';
    EXPECT_EQ(std::to_string(sb), "There are 8 bits in a single byte.");
}

TEST(StringBuilder, Riddle_InPlace1)
{
    auto sb = stringbuilder<1>{};
    sb << "There" << ' ' << "are " << 8 << " bits in a " << "single byte" << '.';
    EXPECT_EQ(std::to_string(sb), "There are 8 bits in a single byte.");
}

TEST(StringBuilder, Riddle_InPlace10)
{
    auto sb = stringbuilder<10>{};
    sb << "There" << ' ' << "are " << 8 << " bits in a " << "single byte" << '.';
    EXPECT_EQ(std::to_string(sb), "There are 8 bits in a single byte.");
}

TEST(StringBuilder, Riddle_InPlace100)
{
    stringbuilder<100> sb;
    sb << "There" << ' ' << "are " << 8 << " bits in a " << "single byte" << '.';
    EXPECT_EQ(std::to_string(sb), "There are 8 bits in a single byte.");
}

TEST(StringBuilder, Reserve)
{
    auto sb = stringbuilder<5>{};
    sb << "abcd";
    sb.reserve(2);
    sb << "xyzw";
    EXPECT_EQ(std::to_string(sb), "abcdxyzw");
}

TEST(StringBuilder, AppendCharMulti)
{
    auto sb = stringbuilder<5>{};
    sb.append('.', 10);
    auto ipsb = inplace_stringbuilder<10>{};
    ipsb.append('.', 10);
    EXPECT_EQ(std::to_string(sb), std::to_string(ipsb));
}

TEST(StringBuilder, AppendStringBuilder)
{
    auto sb = stringbuilder<5>{};
    sb << "123 ";
    sb << sb;
    sb << sb;
    sb << sb;
    EXPECT_EQ(std::to_string(sb), "123 123 123 123 123 123 123 123 ");
}

template<typename T>
struct vec3 {
    T x, y, z;
};

template<typename SB, typename T>
struct sb_appender<SB, vec3<T>> {
    void operator()(SB& sb, const vec3<T>& v) {
        sb << '[' << v.x << ' ' << v.y << ' ' << v.z << ']';
    }
};

template<typename T>
vec3<T> make_vec3(T&& x, T&& y, T&& z) { return vec3<T>{std::forward<T>(x), std::forward<T>(y), std::forward<T>(z)}; }

TEST(StringBuilder, CustomAppender)
{
    stringbuilder<> sb;
    sb << make_vec3('x', 'y', 'z') << " :: " << make_vec3(-12, 23, -34);
    EXPECT_EQ(std::to_string(sb), "[x y z] :: [-12 23 -34]");
}

TEST(MakeString, Simple)
{
    EXPECT_EQ(make_string("There", ' ', "are ", 8, " bits in a ", "single byte", '.'), "There are 8 bits in a single byte.");
}

TEST(MakeString, SizedStr)
{
    EXPECT_EQ(make_string("There", ' ', "are ", 8, " bits in a ", "single ", sized_str<4>(std::string{ "byte" }), '.'), "There are 8 bits in a single byte.");
}

TEST(MakeString, Constexpr_Simple)
{
    {   constexpr auto s = make_string('a', "bcd", 'x');
        ASSERT_STREQ(s.c_str(), "abcdx");
    }
    {   constexpr auto s = make_string("There", ' ', "are ", '8', " bits in a ", "single byte", '.');
        ASSERT_STREQ(s.c_str(), "There are 8 bits in a single byte.");
    }
}

template<typename MethodT>
void Benchmark(const char* title, const size_t iterCount, MethodT method) {
    using Clock = std::chrono::high_resolution_clock;
    auto time0 = Clock::now();
    for (int iter = 0; iter < iterCount; ++iter)
    {
        if (iter == iterCount / 4) time0 = Clock::now();
        std::string str = method();
        volatile size_t size = str.size();
    }
    const auto elapsed = Clock::now() - time0;
    std::cerr << "[  PERFORM ] " << title << ": " << std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count() << " ms" << std::endl;
};

TEST(Perf, IntegerSequence)
{
    constexpr int iterCount = 3000;
    const int span = 1000;

    using Clock = std::chrono::high_resolution_clock;

    Benchmark("std::string concatenation", iterCount, [=]() {
        std::string s;
        for (int i = -span; i <= span; ++i) {
            s += std::to_string(i) + ' ';
        }
        return s;
    });

    Benchmark("std::stringstream", iterCount, [=]() {
        std::stringstream ss;
        for (int i = -span; i <= span; ++i) {
            ss << i << ' ';
        }
        return ss.str();
    });

    Benchmark("static std::stringstream", iterCount, [=]() {
        static std::stringstream ss;
        ss.str("");
        for (int i = -span; i <= span; ++i) {
            ss << i << ' ';
        }
        return ss.str();
    });

    Benchmark("inplace_stringbuilder<SufficientMaxSize>", iterCount, [=]() {
        inplace_stringbuilder<8832> sb;
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

    Benchmark("stringbuilder<SufficientMaxSize>", iterCount, [=]() {
        stringbuilder<8832> sb;
        for (int i = -span; i < span; ++i) {
            sb << i;
            sb << ' ';
        }
        return sb.str();
    });
}

TEST(Perf, Book)
{
    constexpr int iterCount = 15;
    constexpr int wordCount = 400'000;
    constexpr const char* dictionary[] = { "Evolution", "brings", "human", "beings.", "Human", "beings,", "through", "a", "long", "and", "painful", "process,", "bring", "humanity." };
    constexpr size_t dictionarySize = sizeof(dictionary) / sizeof(dictionary[0]);

    using Clock = std::chrono::high_resolution_clock;

    Benchmark("std::string concatenation", iterCount, [=]() {
        std::string s;
        for (size_t i = 0; i < wordCount; ++i) {
            s.append(dictionary[i % dictionarySize]);
            s.append(1, ' ');
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

    Benchmark("stringbuilder<>", iterCount, [=]() {
        stringbuilder<> sb;
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
}
