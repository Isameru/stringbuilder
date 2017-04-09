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

#include <stringbuilder.h>
#include <gtest/gtest.h>

TEST(stringbuilder, Riddle_InPlace0)
{
    auto sb = stringbuilder<0>{};
    sb << "There" << ' ' << "are " << '8' << " bits in a " << "single byte" << '.';
    EXPECT_EQ(sb.str(), "There are 8 bits in a single byte.");
}

TEST(stringbuilder, Riddle_InPlace1)
{
    auto sb = stringbuilder<1>{};
    sb << "There" << ' ' << "are " << '8' << " bits in a " << "single byte" << '.';
    EXPECT_EQ(sb.str(), "There are 8 bits in a single byte.");
}

TEST(stringbuilder, Riddle_InPlace3)
{
    auto sb = stringbuilder<3>{};
    sb << "There" << ' ' << "are " << '8' << " bits in a " << "single byte" << '.';
    EXPECT_EQ(sb.str(), "There are 8 bits in a single byte.");
}

TEST(stringbuilder, Riddle_InPlace10)
{
    auto sb = stringbuilder<10>{};
    sb << "There" << ' ' << "are " << '8' << " bits in a " << "single byte" << '.';
    EXPECT_EQ(sb.str(), "There are 8 bits in a single byte.");
}
