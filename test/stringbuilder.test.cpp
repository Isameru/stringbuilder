
#include <stringbuilder.h>
#define CATCH_CONFIG_MAIN
#include "catch.hpp"

TEST_CASE("Riddle", "[InPlaceStringBuilder]")
{
    auto sb = inplace_stringbuilder<34>{};
    sb << "There" << ' ' << "are " << 8 << " bits in a " << "single byte" << '.';
    REQUIRE(sb.size() == 34);
    REQUIRE(std::to_string(sb) == "There are 8 bits in a single byte.");
    REQUIRE(sb.c_str() == std::string{"There are 8 bits in a single byte."});
}

TEST_CASE("RiddleReversed", "[InPlaceStringBuilder]")
{
    auto sb = inplace_stringbuilder<34, false>{};
    sb << "There" << ' ' << "are " << 8 << " bits in a " << "single byte" << '.';
    REQUIRE(std::to_string(sb) == ".single byte bits in a 8are  There");
    REQUIRE(sb.c_str() == std::string{".single byte bits in a 8are  There"});
}

TEST_CASE("EncodeInteger", "[InPlaceStringBuilder]")
{
    {   auto sb = inplace_stringbuilder<1>{};
        sb << 0;
        REQUIRE(std::to_string(sb) == "0");
    }
    {   auto sb = inplace_stringbuilder<1>{};
        sb << 7;
        REQUIRE(std::to_string(sb) == "7");
    }
    {   auto sb = inplace_stringbuilder<2>{};
        sb << -7;
        REQUIRE(std::to_string(sb) == "-7");
    }
    {   auto sb = inplace_stringbuilder<19>{};
        sb << std::numeric_limits<int64_t>::max();
        REQUIRE(std::to_string(sb) == "9223372036854775807");
    }
    {   auto sb = inplace_stringbuilder<20>{};
        sb << std::numeric_limits<int64_t>::min();
        REQUIRE(std::to_string(sb) == "-9223372036854775808");
    }
    {   auto sb = inplace_stringbuilder<20>{};
        sb << std::numeric_limits<uint64_t>::max();
        REQUIRE(std::to_string(sb) == "18446744073709551615");
    }
}

TEST_CASE("EncodeOther", "[InPlaceStringBuilder]")
{
    {   auto sb = inplace_stringbuilder<11, false>{};
        sb << -123.4567;
        REQUIRE(std::to_string(sb) == "-123.456700");
    }
}

TEST_CASE("Riddle_InPlace0", "[StringBuilder]")
{
    auto sb = stringbuilder<0>{};
    sb << "There" << ' ' << "are " << 8 << " bits in a " << "single byte" << '.';
    REQUIRE(std::to_string(sb) == "There are 8 bits in a single byte.");
}

TEST_CASE("Riddle_InPlace1", "[StringBuilder]")
{
    auto sb = stringbuilder<1>{};
    sb << "There" << ' ' << "are " << 8 << " bits in a " << "single byte" << '.';
    REQUIRE(std::to_string(sb) == "There are 8 bits in a single byte.");
}

TEST_CASE("Riddle_InPlace10", "[StringBuilder]")
{
    auto sb = stringbuilder<10>{};
    sb << "There" << ' ' << "are " << 8 << " bits in a " << "single byte" << '.';
    REQUIRE(std::to_string(sb) == "There are 8 bits in a single byte.");
}

TEST_CASE("Riddle_InPlace100", "[StringBuilder]")
{
    stringbuilder<100> sb;
    sb << "There" << ' ' << "are " << 8 << " bits in a " << "single byte" << '.';
    REQUIRE(std::to_string(sb) == "There are 8 bits in a single byte.");
}

TEST_CASE("Reserve", "[StringBuilder]")
{
    auto sb = stringbuilder<5>{};
    sb << "abcd";
    sb.reserve(2);
    sb << "xyzw";
    REQUIRE(std::to_string(sb) == "abcdxyzw");
}

TEST_CASE("AppendCharMulti", "[StringBuilder]")
{
    auto sb = stringbuilder<5>{};
    sb.append(10, '.');
    auto ipsb = inplace_stringbuilder<10>{};
    ipsb.append(10, '.');
    REQUIRE(std::to_string(sb) == std::to_string(ipsb));
}

TEST_CASE("AppendStringBuilder", "[StringBuilder]")
{
    auto sb = stringbuilder<5>{};
    sb << "123 ";
    sb << sb;
    sb << sb;
    sb << sb;
    REQUIRE(std::to_string(sb) == "123 123 123 123 123 123 123 123 ");
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

TEST_CASE("CustomAppender", "[StringBuilder]")
{
    stringbuilder<> sb;
    sb << make_vec3('x', 'y', 'z') << " :: " << make_vec3(-12, 23, -34);
    REQUIRE(std::to_string(sb) == "[x y z] :: [-12 23 -34]");
}

#ifdef STRINGBUILDER_SUPPORTS_MAKE_STRING
TEST_CASE("Simple", "[MakeString]")
{
    REQUIRE(make_string("There", ' ', "are ", 8, " bits in a ", "single byte", '.') == std::string{"There are 8 bits in a single byte."});
}

TEST_CASE("SizedStr", "[MakeString]")
{
    REQUIRE(make_string("There", ' ', "are ", 8, " bits in a ", "single ", sized_str<4>(std::string{ "byte" }), '.') == std::string{"There are 8 bits in a single byte."});
}

TEST_CASE("Constexpr_Simple", "[MakeString]")
{
    {   constexpr auto s = make_string('a', "bcd", 'x');
        REQUIRE(s.c_str() == std::string{"abcdx"});
    }
    {   constexpr auto s = make_string("There", ' ', "are ", '8', " bits in a ", "single byte", '.');
        REQUIRE(s.c_str() == std::string{"There are 8 bits in a single byte."});
    }
}
#endif // STRINGBUILDER_SUPPORTS_MAKE_STRING
