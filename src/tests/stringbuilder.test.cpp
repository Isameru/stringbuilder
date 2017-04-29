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
#include <string_view>
#include <gtest/gtest.h>

TEST(stringbuilder, InPlaceStringBuilder_Riddle)
{
    auto sb = inplace_stringbuilder<34>{};
    sb << "There" << ' ' << "are " << '8' << " bits in a " << "single byte" << '.';
    EXPECT_EQ(sb.size(), 34);
    EXPECT_EQ(std::to_string(sb), "There are 8 bits in a single byte.");
    ASSERT_STREQ(sb.c_str(), "There are 8 bits in a single byte.");
}

TEST(stringbuilder, InPlaceStringBuilder_Riddle_Reversed)
{
    auto sb = inplace_stringbuilder<34, false>{};
    sb << "There" << ' ' << "are " << '8' << " bits in a " << "single byte" << '.';
    EXPECT_EQ(std::to_string(sb), ".single byte bits in a 8are  There");
    ASSERT_STREQ(sb.c_str(), ".single byte bits in a 8are  There");
}

TEST(stringbuilder, InPlaceStringBuilder_EncodeInteger)
{
    {   auto sb = inplace_stringbuilder<1, false>{};
        sb << 0;
        EXPECT_EQ(std::to_string(sb), "0");
    }
    {   auto sb = inplace_stringbuilder<19, false>{};
        sb << std::numeric_limits<int64_t>::max();
        EXPECT_EQ(std::to_string(sb), "9223372036854775807");
        auto sb2 = inplace_stringbuilder<19>{};
        sb2 << sb;
        EXPECT_EQ(std::to_string(sb2), "9223372036854775807");
    }
    {   auto sb = inplace_stringbuilder<20, false>{};
        sb << std::numeric_limits<int64_t>::min();
        EXPECT_EQ(std::to_string(sb), "-9223372036854775808");
        auto sb2 = inplace_stringbuilder<20>{};
        sb2 << sb;
        EXPECT_EQ(std::to_string(sb2), "-9223372036854775808");
    }
}

TEST(stringbuilder, InPlaceStringBuilder_EncodeOther)
{
    {   auto sb = inplace_stringbuilder<11, false>{};
        sb << -123.4567;
        EXPECT_EQ(std::to_string(sb), "-123.456700");
    }
}


TEST(stringbuilder, Riddle_InPlace0)
{
    auto sb = stringbuilder<0>{};
    sb << "There" << ' ' << "are " << '8' << " bits in a " << "single byte" << '.';
    EXPECT_EQ(std::to_string(sb), "There are 8 bits in a single byte.");
}

TEST(stringbuilder, Riddle_InPlace1)
{
    auto sb = stringbuilder<1>{};
    sb << "There" << ' ' << "are " << '8' << " bits in a " << "single byte" << '.';
    EXPECT_EQ(std::to_string(sb), "There are 8 bits in a single byte.");
}

TEST(stringbuilder, Riddle_InPlace10)
{
    auto sb = stringbuilder<10>{};
    sb << "There" << ' ' << "are " << '8' << " bits in a " << "single byte" << '.';
    EXPECT_EQ(std::to_string(sb), "There are 8 bits in a single byte.");
}

TEST(stringbuilder, Riddle_InPlace100)
{
    stringbuilder<100> sb;
    sb << "There" << ' ' << "are " << '8' << " bits in a " << "single byte" << '.';
    EXPECT_EQ(std::to_string(sb), "There are 8 bits in a single byte.");
}

TEST(stringbuilder, MakeString_Simple)
{
    EXPECT_EQ(make_string('a', "bcd", 'x'), "abcdx");
    EXPECT_EQ(make_string("There", ' ', "are ", '8', " bits in a ", "single byte", '.'), "There are 8 bits in a single byte.");
}

TEST(stringbuilder, MakeStringConstexpr_Simple)
{
    constexpr auto a = make_string('a', 'b', 'c');
    constexpr auto e = std::array<char, 4>{ 'a', 'b', 'c', '\0' };
    EXPECT_EQ(a, e);
}
