
#include <stringbuilder.h>
#include <algorithm>
//#define _SILENCE_CXX17_STRSTREAM_DEPRECATION_WARNING
//#include <strstream>
#include <iostream>
#include <cstddef>
#include <sstream>
#include <chrono>
#include <vector>
#ifdef WIN32
#include <intrin.h>
#endif

using namespace sbldr;
using namespace sbldr::detail;

//static void escape(void* p) { asm volatile("" : : "g"(p) : "memory"); }
volatile size_t vsize;
volatile const char* vcstr;

void ProvideResult(std::string&& str)
{
    vsize = str.size();
    vcstr = str.c_str();
}

#if STRINGBUILDER_USES_STRING_VIEW
void ProvideResult(std::basic_string_view<char>&& str_view)
{
    vsize = str_view.size();
    vcstr = str_view.data();
}
#endif

enum class BenchmarkTiming { Mean, Best };

template<typename MethodT>
void Benchmark(const std::string& title, BenchmarkTiming timing, const size_t iterCount, const size_t microIterCount, MethodT method)
{
    using Clock = std::chrono::high_resolution_clock;

    if (timing == BenchmarkTiming::Mean)
    {
        for (size_t iter = 0; iter < iterCount * microIterCount / 10; ++iter)
        {
            ProvideResult(method());
        }
    }

    std::vector<Clock::duration> durations;
    durations.reserve(iterCount);

    for (size_t iter = 0; iter < iterCount; ++iter)
    {
        const auto time0 = Clock::now();

        for (size_t i = 0; i < microIterCount; ++i)
        {
            ProvideResult(method());
        }

        const auto elapsed = Clock::now() - time0;
        durations.push_back(elapsed / microIterCount);
    }

    std::sort(std::begin(durations), std::end(durations));

    const auto meanDuration = timing == BenchmarkTiming::Mean ? durations[durations.size() / 2] : durations[0];
    const char* const timingText = timing == BenchmarkTiming::Mean ? "[mean]" : "[best]";

    if (meanDuration < std::chrono::microseconds{ 10 })
    {
        std::cout << "    " << title << ": " << std::chrono::duration_cast<std::chrono::nanoseconds>(meanDuration).count() << " ns " << timingText << std::endl;
    }
    else
    {
        std::cout << "    " << title << ": " << std::chrono::duration_cast<std::chrono::microseconds>(meanDuration).count() << " us " << timingText << std::endl;
    }
};


void benchmarkIntegerSequence()
{
    std::cout << "Scenario: IntegerSequence" << std::endl;

    constexpr size_t iterCount = 3000;
    constexpr int span = 1000;

    Benchmark("inplace_stringbuilder<big>", BenchmarkTiming::Best, iterCount, 1, [=]() {
        inplace_stringbuilder<8788> sb;
        for (int i = -span; i <= span; ++i) {
            sb << i;
            sb << ' ';
        }
        return sb.str();
    });

    Benchmark("stringbuilder<>", BenchmarkTiming::Best, iterCount, 1, [=]() {
        stringbuilder<> sb;
        for (int i = -span; i < span; ++i) {
            sb << i;
            sb << ' ';
        }
        return sb.str();
    });

    Benchmark("stringbuilder<> with reserve", BenchmarkTiming::Best, iterCount, 1, [=]() {
        stringbuilder<> sb;
        sb.reserve(8788);
        for (int i = -span; i < span; ++i) {
            sb << i;
            sb << ' ';
        }
        return sb.str();
    });

    Benchmark("stringbuilder<big>", BenchmarkTiming::Best, iterCount, 1, [=]() {
        stringbuilder<8788> sb;
        for (int i = -span; i < span; ++i) {
            sb << i;
            sb << ' ';
        }
        return sb.str();
    });

    Benchmark("string.append(a).append(b)", BenchmarkTiming::Best, iterCount, 1, [=]() {
        std::string s;
        for (int i = -span; i <= span; ++i) {
            s.append(std::to_string(i)).append(1, ' ');
        }
        return s;
    });

    Benchmark("string.append(a+b)", BenchmarkTiming::Best, iterCount, 1, [=]() {
        std::string s;
        for (int i = -span; i <= span; ++i) {
            s.append(std::to_string(i) + ' ');
        }
        return s;
    });

    Benchmark("stringstream", BenchmarkTiming::Best, iterCount, 1, [=]() {
        std::stringstream ss;
        for (int i = -span; i <= span; ++i) {
            ss << i << ' ';
        }
        return ss.str();
    });

    Benchmark("static stringstream", BenchmarkTiming::Best, iterCount, 1, [=]() {
        static std::stringstream ss;
        ss.str("");
        for (int i = -span; i <= span; ++i) {
            ss << i << ' ';
        }
        return ss.str();
    });

    // {
    //     constexpr int bufSize = 8788 + 1;
    //     char buf[bufSize];
    //     Benchmark("strstream", BenchmarkTiming::Best, iterCount, 1, [=, &buf]() {
    //         std::strstream ss{buf, bufSize};
    //         for (int i = -span; i <= span; ++i) {
    //             ss << i << ' ';
    //         }
    //         ss << std::ends;
    //         return std::string{ ss.str() };
    //     });
    // }
}


std::array<const char*, 33> words{ "There", " ", "are", " ", "only", " ", "10", " ", "people", " ", "in", " ", "the", " ", "world", ":", " ", "those", " ", "who", " ", "know", " ", "binary", " ", "and", " ", "those", " ", "who", " ", "don't", "." };
const char* g_joke = nullptr;

void benchmarkBook()
{
    std::cout << "Scenario: Book" << std::endl;

    constexpr size_t iterCount = 25;
    constexpr size_t miniIterCount = 5;
    constexpr size_t wordCount = 400000;

    // Beware of % of 64-bit ints!

    //Benchmark("view of stringbuilder<> with reserve << *", BenchmarkTiming::Best, iterCount, miniIterCount, [=]() {
    //    stringbuilder<> sb;
    //    sb.reserve(1000000);
    //    for (size_t i = 0; i < wordCount; ++i) {
    //        sb.append_c_str(dictionary[i % dictionarySize]);
    //    }
    //    return sb.str_view();
    //});

    //Benchmark("view of string.append(*) with reserve", BenchmarkTiming::Best, iterCount, miniIterCount, [=]() {
    //    std::string s;
    //    s.reserve(1000000);
    //    for (size_t i = 0; i < wordCount; ++i) {
    //        s.append(dictionary[i % dictionarySize]);
    //    }
    //    return std::string_view{ s };
    //});

    Benchmark("stringbuilder<> << *", BenchmarkTiming::Best, iterCount, miniIterCount, [=]() {
        stringbuilder<> sb;
        for (int s = 0; s < 25000; ++s) {
            for (int w = 0; w < (int)words.size(); ++w)
                sb << words[w];
        }
        sb << " ";
        return sb.str();
    });

    Benchmark("stringbuilder<> << [N]", BenchmarkTiming::Best, iterCount, miniIterCount, [=]() {
        stringbuilder<> sb;
        for (size_t i = 0; i < wordCount; i += words.size()) {
            sb << "There" << " " << "are" << " " << "only" << " " << "10" << " " << "people" << " " << "in" << " " << "the" << " " << "world" << ":" << " " << "those" << " " << "who" << " " << "know" << " " << "binary" << " " << "and" << " " << "those" << " " << "who" << " " << "don't" << "." << " ";
        }
        return sb.str();
    });

    Benchmark("stringbuilder<> with reserve << [N]", BenchmarkTiming::Best, iterCount, miniIterCount, [=]() {
        stringbuilder<> sb;
        sb.reserve(1000000);
        for (size_t i = 0; i < wordCount; i += words.size()) {
            sb << "There" << " " << "are" << " " << "only" << " " << "10" << " " << "people" << " " << "in" << " " << "the" << " " << "world" << ":" << " " << "those" << " " << "who" << " " << "know" << " " << "binary" << " " << "and" << " " << "those" << " " << "who" << " " << "don't" << "." << " ";
        }
        return sb.str();
    });

    //Benchmark("stringbuilder<4kB> << *", BenchmarkTiming::Best, iterCount, miniIterCount, [=]() {
    //    stringbuilder<4 * 1024> sb;
    //    for (size_t i = 0; i < wordCount; ++i) {
    //        sb << dictionary[i % dictionarySize];
    //    }
    //    return sb.str();
    //});

    Benchmark("stringbuilder<4kB> << [N]", BenchmarkTiming::Best, iterCount, miniIterCount, [=]() {
        stringbuilder<4 * 1024> sb;
        for (size_t i = 0; i < wordCount; i += words.size()) {
            sb << "There" << " " << "are" << " " << "only" << " " << "10" << " " << "people" << " " << "in" << " " << "the" << " " << "world" << ":" << " " << "those" << " " << "who" << " " << "know" << " " << "binary" << " " << "and" << " " << "those" << " " << "who" << " " << "don't" << "." << " ";
        }
        return sb.str();
    });

    //Benchmark("stringbuilder<64kB> << *", BenchmarkTiming::Best, iterCount, miniIterCount, [=]() {
    //    stringbuilder<64 * 1024> sb;
    //    for (size_t i = 0; i < wordCount; ++i) {
    //        sb << dictionary[i % dictionarySize];
    //    }
    //    return sb.str();
    //});

    Benchmark("stringbuilder<64kB> << [N]", BenchmarkTiming::Best, iterCount, miniIterCount, [=]() {
        stringbuilder<64 * 1024> sb;
        for (size_t i = 0; i < wordCount; i += words.size()) {
            sb << "There" << " " << "are" << " " << "only" << " " << "10" << " " << "people" << " " << "in" << " " << "the" << " " << "world" << ":" << " " << "those" << " " << "who" << " " << "know" << " " << "binary" << " " << "and" << " " << "those" << " " << "who" << " " << "don't" << "." << " ";
        }
        return sb.str();
    });

    //Benchmark("stringbuilder<512kB> << *", BenchmarkTiming::Best, iterCount, miniIterCount, [=]() {
    //    stringbuilder<512 * 1024> sb;
    //    for (size_t i = 0; i < wordCount; ++i) {
    //        sb << dictionary[i % dictionarySize];
    //    }
    //    return sb.str();
    //});

    Benchmark("stringbuilder<512kB> << [N]", BenchmarkTiming::Best, iterCount, miniIterCount, [=]() {
        stringbuilder<512 * 1024> sb;
        for (size_t i = 0; i < wordCount; i += words.size()) {
            sb << "There" << " " << "are" << " " << "only" << " " << "10" << " " << "people" << " " << "in" << " " << "the" << " " << "world" << ":" << " " << "those" << " " << "who" << " " << "know" << " " << "binary" << " " << "and" << " " << "those" << " " << "who" << " " << "don't" << "." << " ";
        }
        return sb.str();
    });

    //Benchmark("string.append(*)", BenchmarkTiming::Best, iterCount, miniIterCount, [=]() {
    //    std::string s;
    //    for (size_t i = 0; i < wordCount; ++i) {
    //        s.append(dictionary[i % dictionarySize]);
    //    }
    //    return s;
    //});

    Benchmark("string.append([N])", BenchmarkTiming::Best, iterCount, miniIterCount, [=]() {
        std::string s;
        for (size_t i = 0; i < wordCount; i += words.size()) {
            s.append("There").append(" ").append("are").append(" ").append("only").append(" ").append("10").append(" ").append("people").append(" ").append("in").append(" ").append("the").append(" ").append("world").append(":").append(" ").append("those").append(" ").append("who").append(" ").append("know").append(" ").append("binary").append(" ").append("and").append(" ").append("those").append(" ").append("who").append(" ").append("don't").append(".").append(" ");
        }
        return s;
    });

    Benchmark("string.append([N]) with reserve", BenchmarkTiming::Best, iterCount, miniIterCount, [=]() {
        std::string s;
        s.reserve(1000000);
        for (size_t i = 0; i < wordCount; i += words.size()) {
            s.append("There").append(" ").append("are").append(" ").append("only").append(" ").append("10").append(" ").append("people").append(" ").append("in").append(" ").append("the").append(" ").append("world").append(":").append(" ").append("those").append(" ").append("who").append(" ").append("know").append(" ").append("binary").append(" ").append("and").append(" ").append("those").append(" ").append("who").append(" ").append("don't").append(".").append(" ");
        }
        return s;
    });

    //Benchmark("stringstream << *", BenchmarkTiming::Best, iterCount, miniIterCount, [=]() {
    //    std::stringstream ss;
    //    for (size_t i = 0; i < wordCount; ++i) {
    //        ss << dictionary[i % dictionarySize];
    //    }
    //    return ss.str();
    //});

    Benchmark("stringstream << [N]", BenchmarkTiming::Best, iterCount, miniIterCount, [=]() {
        std::stringstream ss;
        for (size_t i = 0; i < wordCount; i += words.size()) {
            ss << "There" << " " << "are" << " " << "only" << " " << "10" << " " << "people" << " " << "in" << " " << "the" << " " << "world" << ":" << " " << "those" << " " << "who" << " " << "know" << " " << "binary" << " " << "and" << " " << "those" << " " << "who" << " " << "don't" << "." << " ";
        }
        return ss.str();
    });

    //Benchmark("static stringstream << *", BenchmarkTiming::Best, iterCount, miniIterCount, [=]() {
    //    static std::stringstream ss;
    //    ss.str("");
    //    ss.clear();
    //    for (size_t i = 0; i < wordCount; ++i) {
    //        ss << dictionary[i % dictionarySize];
    //    }
    //    return ss.str();
    //});

    Benchmark("static stringstream << [N]", BenchmarkTiming::Best, iterCount, miniIterCount, [=]() {
        static std::stringstream ss;
        ss.str("");
        ss.clear();
        for (size_t i = 0; i < wordCount; i += words.size()) {
            ss << "There" << " " << "are" << " " << "only" << " " << "10" << " " << "people" << " " << "in" << " " << "the" << " " << "world" << ":" << " " << "those" << " " << "who" << " " << "know" << " " << "binary" << " " << "and" << " " << "those" << " " << "who" << " " << "don't" << "." << " ";
        }
        return ss.str();
    });

    // {
    //     //constexpr int bufSize = 2771432 + 1;
    //     constexpr int bufSize = 139400 + 1;
    //     auto buf = std::make_unique<char[]>(bufSize);
    //     BenchmarkMean("strstream << *", iterCount, 1, [=, &buf]() {
    //         std::strstream ss{buf.get(), bufSize};
    //         for (size_t i = 0; i < wordCount; ++i) {
    //             ss << dictionary[i % dictionarySize];// << ' ';
    //         }
    //         ss << std::ends;
    //         return ss.str();
    //     });
    // }
}

void benchmarkQuote()
{
    std::cout << "Scenario: Quote" << std::endl;

    constexpr size_t iterCount = 500;
    constexpr size_t miniIterCount = 500;

    Benchmark("empty", BenchmarkTiming::Mean, iterCount, miniIterCount, [=]() {
        return std::string{};
    });

    Benchmark("empty + computing length of a string literal", BenchmarkTiming::Mean, iterCount, miniIterCount, [=]() {
        vsize = std::char_traits<char>::length("There are only 10 people in the world: those who know binary and those who don't.");
        return std::string{};
    });

    Benchmark("empty + computing length", BenchmarkTiming::Mean, iterCount, miniIterCount, [=]() {
        vsize = std::char_traits<char>::length(g_joke);
        return std::string{};
    });

    Benchmark("just string from literal", BenchmarkTiming::Mean, iterCount, miniIterCount, [=]() {
        return std::string("There are only 10 people in the world: those who know binary and those who don't.");
    });

    Benchmark("just string from literal with known size", BenchmarkTiming::Mean, iterCount, miniIterCount, [=]() {
        return std::string("There are only 10 people in the world: those who know binary and those who don't.", 81);
    });

    Benchmark("just string", BenchmarkTiming::Mean, iterCount, miniIterCount, [=]() {
        return std::string(g_joke);
    });

    Benchmark("just string with known size", BenchmarkTiming::Mean, iterCount, miniIterCount, [=]() {
        return std::string(g_joke, 81);
    });

#ifdef STRINGBUILDER_SUPPORTS_MAKE
    Benchmark("constexpr = make_string", BenchmarkTiming::Mean, iterCount, miniIterCount, [=]() {
        constexpr auto constexpr_quote = make_string("There", " ", "are", " ", "only", " ", "10", " ", "people", " ", "in", " ", "the", " ", "world", ":", " ", "those", " ", "who", " ", "know", " ", "binary", " ", "and", " ", "those", " ", "who", " ", "don't", ".");
        return constexpr_quote.str();
    });

    Benchmark("make_string(constexpr)", BenchmarkTiming::Mean, iterCount, miniIterCount, [=]() {
        return make_string("There", " ", "are", " ", "only", " ", "10", " ", "people", " ", "in", " ", "the", " ", "world", ":", " ", "those", " ", "who", " ", "know", " ", "binary", " ", "and", " ", "those", " ", "who", " ", "don't", ".").str();
    });

    Benchmark("make_string", BenchmarkTiming::Mean, iterCount, miniIterCount, [=]() {
        return make_string("There", " ", "are", " ", "only", " ", sized_str<2>("10"), " ", "people", " ", "in", " ", "the", " ", "world", ":", " ", "those", " ", "who", " ", "know", " ", "binary", " ", "and", " ", "those", " ", "who", " ", "don't", ".");
    });
#endif

    Benchmark("stringbuilder<>([N])", BenchmarkTiming::Mean, iterCount, miniIterCount, [=]() {
        stringbuilder<> sb;
        sb << "There" << " " << "are" << " " << "only" << " " << "10" << " " << "people" << " " << "in" << " " << "the" << " " << "world" << ":" << " " << "those" << " " << "who" << " " << "know" << " " << "binary" << " " << "and" << " " << "those" << " " << "who" << " " << "don't" << ".";
        return sb.str();
    });

    Benchmark("stringbuilder<>(*)", BenchmarkTiming::Mean, iterCount, miniIterCount, [=]() {
        stringbuilder<> sb;
        for (const char* word : words)
        {
            sb << word;
        }
        return sb.str();
    });

    Benchmark("stringbuilder<81>([N])", BenchmarkTiming::Mean, iterCount, miniIterCount, [=]() {
        stringbuilder<81> sb;
        sb << "There" << " " << "are" << " " << "only" << " " << "10" << " " << "people" << " " << "in" << " " << "the" << " " << "world" << ":" << " " << "those" << " " << "who" << " " << "know" << " " << "binary" << " " << "and" << " " << "those" << " " << "who" << " " << "don't" << ".";
        return sb.str();
    });

    Benchmark("stringbuilder<81>(*)", BenchmarkTiming::Mean, iterCount, miniIterCount, [=]() {
        stringbuilder<81> sb;
        for (const char* word : words)
        {
            sb << word;
        }
        return sb.str();
    });

    Benchmark("inplace_stringbuilder<81>([N])", BenchmarkTiming::Mean, iterCount, miniIterCount, [=]() {
        inplace_stringbuilder<81> sb;
        sb << "There" << " " << "are" << " " << "only" << " " << "10" << " " << "people" << " " << "in" << " " << "the" << " " << "world" << ":" << " " << "those" << " " << "who" << " " << "know" << " " << "binary" << " " << "and" << " " << "those" << " " << "who" << " " << "don't" << ".";
        return sb.str();
    });

    Benchmark("inplace_stringbuilder<81>(*)", BenchmarkTiming::Mean, iterCount, miniIterCount, [=]() {
        inplace_stringbuilder<81> sb;
        for (const char* word : words)
        {
            sb << word;
        }
        return sb.str();
    });

    Benchmark("string + string (loop)", BenchmarkTiming::Mean, iterCount, miniIterCount, [=]() {
        std::string text;
        for (const char* word : words)
        {
            text = text + word;
        }
        return text;
    });

    Benchmark("string + string (expression)", BenchmarkTiming::Mean, iterCount, miniIterCount, [=]() {
        return std::string("There") + " " + "are" + " " + "only" + " " + "10" + " " + "people" + " " + "in" + " " + "the" + " " + "world" + ":" + " " + "those" + " " + "who" + " " + "know" + " " + "binary" + " " + "and" + " " + "those" + " " + "who" + " " + "don't" + ".";
    });

    Benchmark("string.append (loop)", BenchmarkTiming::Mean, iterCount, miniIterCount, [=]() {
        std::string text;
        for (const char* const word : words)
        {
            text.append(word);
        }
        return text;
    });

    Benchmark("string.append (flat)", BenchmarkTiming::Mean, iterCount, miniIterCount, [=]() {
        return std::string("There").append(" ").append("are").append(" ").append("only").append(" ").append("10").append(" ").append("people").append(" ").append("in").append(" ").append("the").append(" ").append("world").append(":").append(" ").append("those").append(" ").append("who").append(" ").append("know").append(" ").append("binary").append(" ").append("and").append(" ").append("those").append(" ").append("who").append(" ").append("don't").append(".");
    });

    Benchmark("string.append with reserve (loop)", BenchmarkTiming::Mean, iterCount, miniIterCount, [=]() {
        std::string text;
        text.reserve(81);
        for (const char* const word : words)
        {
            text.append(word);
        }
        return text;
    });

    Benchmark("string.append with reserve (flat)", BenchmarkTiming::Mean, iterCount, miniIterCount, [=]() {
        std::string text;
        text.reserve(81);
        return text.append("There").append(" ").append("are").append(" ").append("only").append(" ").append("10").append(" ").append("people").append(" ").append("in").append(" ").append("the").append(" ").append("world").append(":").append(" ").append("those").append(" ").append("who").append(" ").append("know").append(" ").append("binary").append(" ").append("and").append(" ").append("those").append(" ").append("who").append(" ").append("don't").append(".");
    });

    Benchmark("static string.append with reserve (loop)", BenchmarkTiming::Mean, iterCount, miniIterCount, [=]() {
        static std::string text;
        text.clear();
        text.reserve(81);
        for (const char* const word : words)
        {
            text.append(word);
        }
        return text;
    });

    Benchmark("static string.append with reserve (flat)", BenchmarkTiming::Mean, iterCount, miniIterCount, [=]() {
        static std::string text;
        text.clear();
        text.reserve(81);
        return text.append("There").append(" ").append("are").append(" ").append("only").append(" ").append("10").append(" ").append("people").append(" ").append("in").append(" ").append("the").append(" ").append("world").append(":").append(" ").append("those").append(" ").append("who").append(" ").append("know").append(" ").append("binary").append(" ").append("and").append(" ").append("those").append(" ").append("who").append(" ").append("don't").append(".");
    });

    Benchmark("thread_local string.append with reserve (loop)", BenchmarkTiming::Mean, iterCount, miniIterCount, [=]() {
        thread_local std::string text;
        text.clear();
        text.reserve(81);
        for (const char* const word : words)
        {
            text.append(word);
        }
        return text;
    });

    Benchmark("thread_local string.append with reserve (flat)", BenchmarkTiming::Mean, iterCount, miniIterCount, [=]() {
        thread_local std::string text;
        text.clear();
        text.reserve(81);
        return text.append("There").append(" ").append("are").append(" ").append("only").append(" ").append("10").append(" ").append("people").append(" ").append("in").append(" ").append("the").append(" ").append("world").append(":").append(" ").append("those").append(" ").append("who").append(" ").append("know").append(" ").append("binary").append(" ").append("and").append(" ").append("those").append(" ").append("who").append(" ").append("don't").append(".");
    });

    Benchmark("stringstream << *", BenchmarkTiming::Mean, iterCount, miniIterCount, [=]() {
        std::stringstream ss;
        for (const char* const word : words)
        {
            ss << word;
        }
        return ss.str();
    });

    Benchmark("stringstream << [N]", BenchmarkTiming::Mean, iterCount, miniIterCount, [=]() {
        std::stringstream ss;
        ss << "There" << " " << "are" << " " << "only" << " " << "10" << " " << "people" << " " << "in" << " " << "the" << " " << "world" << ":" << " " << "those" << " " << "who" << " " << "know" << " " << "binary" << " " << "and" << " " << "those" << " " << "who" << " " << "don't" << ".";
        return ss.str();
    });

    Benchmark("static stringstream << *", BenchmarkTiming::Mean, iterCount, miniIterCount, [=]() {
        static std::stringstream ss;
        ss.str("");
        ss.clear();
        for (const char* word : words)
        {
            ss << word;
        }
        return ss.str();
    });

    Benchmark("static stringstream << [N]", BenchmarkTiming::Mean, iterCount, miniIterCount, [=]() {
        static std::stringstream ss;
        ss.str("");
        ss.clear();
        ss << "There" << " " << "are" << " " << "only" << " " << "10" << " " << "people" << " " << "in" << " " << "the" << " " << "world" << ":" << " " << "those" << " " << "who" << " " << "know" << " " << "binary" << " " << "and" << " " << "those" << " " << "who" << " " << "don't" << ".";
        return ss.str();
    });

    Benchmark("thread_local stringstream << *", BenchmarkTiming::Mean, iterCount, miniIterCount, [=]() {
        thread_local std::stringstream ss;
        ss.str("");
        ss.clear();
        for (const char* word : words)
        {
            ss << word;
        }
        return ss.str();
    });

    Benchmark("thread_local stringstream << [N]", BenchmarkTiming::Mean, iterCount, miniIterCount, [=]() {
        thread_local std::stringstream ss;
        ss.str("");
        ss.clear();
        ss << "There" << " " << "are" << " " << "only" << " " << "10" << " " << "people" << " " << "in" << " " << "the" << " " << "world" << ":" << " " << "those" << " " << "who" << " " << "know" << " " << "binary" << " " << "and" << " " << "those" << " " << "who" << " " << "don't" << ".";
        return ss.str();
    });

    // char buf[81 + 1];
    // Benchmark("strstream", BenchmarkTiming::Mean, iterCount, miniIterCount, [=, &buf]() {
    //     std::strstream ss{buf, sizeof(buf)/sizeof(buf[0])};
    //     for (const char* word : words)
    //     {
    //         ss << word;
    //     }
    //     return std::string{ ss.str() };
    // });
}


enum class SbI_Mode
{
    Simple,
    Prefetch,
    Progressive
};

template<size_t InPlaceSize, SbI_Mode mode>
struct SbI
{
    char data[InPlaceSize + 1];
    size_t consumed = 0;

    SbI(size_t)
    { }

    SbI& append_c_str_progressive(const char* str) {
        while (*str != 0) {
            data[consumed++] = *(str++);
        }
        return *this;
    }

    SbI& append_c_str(const char* const str) {
        if (mode == SbI_Mode::Prefetch) prefetchWrite(data + consumed);
        const auto size = std::char_traits<char>::length(str);
        std::char_traits<char>::copy(data + consumed, str, size);
        consumed += size;
        return *this;
    }

    template<size_t N>
    SbI& append_c_array(const char(&str)[N]) {
        assert(str[N - 1] == '\0');
        if (mode == SbI_Mode::Prefetch) prefetchWrite(data + consumed);
        std::char_traits<char>::copy(data + consumed, str, N - 1);
        consumed += N - 1;
        return *this;
    }

    template<typename T>
    SbI& operator<<(const T str) {
        if (mode == SbI_Mode::Progressive)
            return append_c_str_progressive(str);
        else
            return append_c_str(str);
    }

    template<size_t N>
    SbI& operator<<(const char (&str)[N]) {
        if (mode == SbI_Mode::Progressive)
            return append_c_str_progressive(str);
        else
            return append_c_array(str);
    }

    std::string str() const {
        return { data, data + consumed };
    }

#if STRINGBUILDER_USES_STRING_VIEW
    std::basic_string_view<char> str_view() const noexcept {
        return { data, consumed };
    }
#endif
};


template<bool Prefetch>
struct SbS
{
    std::unique_ptr<char[]> data;
    size_t consumed = 0;

    SbS(size_t maxSize) :
        data{new char[maxSize]}
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

    SbSR<Prefetch, Likely>& operator<<(const char* const str) {
        if (Prefetch) prefetchWrite(data.get() + consumed);
        const auto size = std::char_traits<char>::length(str);
        if (Likely)
        {
            if (STRINGBUILDER_UNLIKELY(consumed + size > reserved)) {
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
        chunk{reinterpret_cast<Chunk*>(new uint8_t[sizeof(chunk->consumed) +  maxSize])}
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
        chunk{reinterpret_cast<Chunk*>(new uint8_t[sizeof(Chunk) - sizeof(chunk->data) +  maxSize])}
    {
        chunk->consumed = 0;
        chunk->reserved = maxSize;
    }

    SbCR& operator<<(const char* str) {
        if (Prefetch) prefetchWrite(chunk->data + chunk->consumed);
        const auto size = std::char_traits<char>::length(str);
        if (Likely)
        {
            if (STRINGBUILDER_UNLIKELY(chunk->consumed + size > chunk->reserved)) {
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
    //int64_t spaceLeft;
    const char* const dataEnd;

    SbTR(size_t maxSize) :
        data{new char[maxSize]},
        tail{data.get()},
        //spaceLeft{ (int)maxSize }
        dataEnd{tail + maxSize}
    { }

    SbTR& operator<<(const char* str) {
        if (Prefetch) prefetchWrite(tail);
        const auto size = std::char_traits<char>::length(str);
        if (Likely)
        {
            if (STRINGBUILDER_UNLIKELY(dataEnd - tail < static_cast<std::ptrdiff_t>(size))) {
                // Will not happen, but don't tell it to the compiler.
                //spaceLeft += tail - data.get();
                tail = data.get();
                std::char_traits<char>::assign(tail, 9999, '\0');
            }
        }
        else
        {
            if (dataEnd - tail < static_cast<std::ptrdiff_t>(size)) {
                // Will not happen, but don't tell it to the compiler.
                //spaceLeft += tail - data.get();
                tail = data.get();
                std::char_traits<char>::assign(tail, 9999, '\0');
            }
        }
        std::char_traits<char>::copy(tail, str, size);
        tail += size;
        //spaceLeft -= size;
        return *this;
    }

    std::string str() const {
        return {data.get(), tail};
    }
};

template<bool Prefetch, bool Likely>
struct SbTR2
{
    std::unique_ptr<char[]> data;
    char* tail;
    size_t spaceLeft;

    SbTR2(size_t maxSize) :
        data{ new char[maxSize] },
        tail{ data.get() },
        spaceLeft{ maxSize }
    { }

    SbTR2& operator<<(const char* str) {
        if (Prefetch) prefetchWrite(tail);
        const auto size = std::char_traits<char>::length(str);
        if (Likely)
        {
            if (STRINGBUILDER_UNLIKELY(spaceLeft < size)) {
                // Will not happen, but don't tell it to the compiler.
                //spaceLeft += tail - data.get();
                tail = data.get();
                std::char_traits<char>::assign(tail, 9999, '\0');
            }
        }
        else
        {
            if (spaceLeft < size) {
                // Will not happen, but don't tell it to the compiler.
                //spaceLeft += tail - data.get();
                tail = data.get();
                std::char_traits<char>::assign(tail, 9999, '\0');
            }
        }
        std::char_traits<char>::copy(tail, str, size);
        tail += size;
        spaceLeft -= size;
        return *this;
    }

    std::string str() const {
        return { data.get(), tail };
    }
};


template<typename SbT>
void benchmarkAppend(const std::string& title)
{
    constexpr size_t iterCount = 1000;
    constexpr size_t miniIterCount = 1000;

    std::array<const char*, 33> words { "There", " ", "are", " ", "only", " ", "10", " ", "people", " ", "in", " ", "the", " ", "world", ":", " ", "those", " ", "who", " ", "know", " ", "binary", " ", "and", " ", "those", " ", "who", " ", "don't", "." };
    const size_t maxSize = 81;

    Benchmark(title + " << *", BenchmarkTiming::Mean, iterCount, miniIterCount, [=]() {
        SbT sb{maxSize};
        for (const char* const word : words) {
            sb << word;
        }
        return sb.str();
        //return sb.str_view();
    });

    Benchmark(title + " << [N]", BenchmarkTiming::Mean, iterCount, miniIterCount, [=]() {
        SbT sb{maxSize};
        sb << "There" << " " << "are" << " " << "only" << " " << "10" << " " << "people" << " " << "in" << " " << "the" << " " << "world" << ":" << " " << "those" << " " << "who" << " " << "know" << " " << "binary" << " " << "and" << " " << "those" << " " << "who" << " " << "don't" << ".";
        return sb.str();
        //return sb.str_view();
    });
}

void benchmarkAppend()
{
    std::cout << "Scenario: Append" << std::endl;

    benchmarkAppend<SbI<81, SbI_Mode::Simple>>("inplace data[consumed++]");
    benchmarkAppend<SbI<81, SbI_Mode::Prefetch>>("inplace prefetch data[consumed++]");
    benchmarkAppend<SbI<81, SbI_Mode::Progressive>>("inplace progressive data[consumed++]");
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
    benchmarkAppend<SbTR2<false, false>>("if(spaceLeft) tail++");
    benchmarkAppend<SbTR<true, false>>("prefetch if(spaceLeft) tail++");
#ifdef __GNUC__
    benchmarkAppend<SbTR<false, true>>("if(likely spaceLeft) tail++");
    benchmarkAppend<SbTR<true, true>>("prefetch if(likely spaceLeft) tail++");
#endif
}


#ifdef STRINGBUILDER_SUPPORTS_MAKE_STRING
namespace detail
{
    template<typename Method, size_t... IX>
    struct foreach_integral_constant_helper;

    template<typename Method, size_t I>
    struct foreach_integral_constant_helper<Method, I>
    {
        void operator()(Method method)
        {
            method(std::integral_constant<size_t, I>{});
        }
    };

    template<typename Method, size_t I0, size_t... IX>
    struct foreach_integral_constant_helper<Method, I0, IX...>
    {
        void operator()(Method method)
        {
            method(std::integral_constant<size_t, I0>{});
            foreach_integral_constant_helper<Method, IX...>{}(method);
        }
    };
}

template<typename Method, size_t... IX>
void foreach_integral_constant(std::integer_sequence<size_t, IX...>, Method method)
{
    ::detail::foreach_integral_constant_helper<Method, IX...>{}(method);
}

void benchmarkProgressiveAppend()
{
    std::cout << "Scenario: ProgressiveAppend (words)" << std::endl;
    const std::array<const char* const, 33> words{ "There", " ", "are", " ", "only", " ", "10", " ", "people", " ", "in", " ", "the", " ", "world", ":", " ", "those", " ", "who", " ", "know", " ", "binary", " ", "and", " ", "those", " ", "who", " ", "don't", ". " };

    foreach_integral_constant(
        std::integer_sequence<size_t, 64, 512, 4098, 32 * 1024, 128 * 1024, 512 * 1024>{},
        [=](auto inPlaceSizeIC) {
            constexpr auto inPlaceSize = inPlaceSizeIC.value;

            Benchmark(make_string("inplace_stringbuilder<", inPlaceSize, ">.append_c_str(*)"), BenchmarkTiming::Best, 100, 10, [=]() {
                constexpr auto inPlaceSize = inPlaceSizeIC.value;
                inplace_stringbuilder<inPlaceSize> sb;

                for (size_t i = 0; sb.length() + 8 < inPlaceSize; ++i)
                {
                    sb.append_c_str(words[i % words.size()]);
                }

                return sb.str();
            });

            Benchmark(make_string("inplace_stringbuilder<", inPlaceSize, ">.append_c_str_progressive(*)"), BenchmarkTiming::Best, 100, 10, [=]() {
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

            Benchmark(make_string("inplace_stringbuilder<", inPlaceSize, ">.append_c_str(*)"), BenchmarkTiming::Best, 100, 10, [=]() {
                constexpr auto inPlaceSize = inPlaceSizeIC.value;
                inplace_stringbuilder<inPlaceSize> sb;

                for (size_t i = 0; sb.length() + 81 < inPlaceSize; ++i)
                {
                    sb.append_c_str(g_joke);
                }

                return sb.str();
            });

            Benchmark(make_string("inplace_stringbuilder<", inPlaceSize, ">.append_c_str(*,81)"), BenchmarkTiming::Best, 100, 10, [=]() {
                constexpr auto inPlaceSize = inPlaceSizeIC.value;
                inplace_stringbuilder<inPlaceSize> sb;

                for (size_t i = 0; sb.length() + 81 < inPlaceSize; ++i)
                {
                    sb.append_c_str(g_joke, 81);
                }

                return sb.str();
            });

            Benchmark(make_string("inplace_stringbuilder<", inPlaceSize, ">.append_c_str_progressive(*)"), BenchmarkTiming::Best, 100, 10, [=]() {
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
#endif // STRINGBUILDER_SUPPORTS_MAKE_STRING

void benchmarkProgressiveThreshold()
{
    std::cout << "Scenario: ProgressiveThreshold()" << std::endl;

    constexpr size_t iterCount = 10;
    constexpr size_t miniIterCount = 100;

    stringbuilder<> jokess;
    jokess << "There are only " << "10 people in the world: those " << "who know binary and those who don't.";
    std::string joke_string{ g_joke };

    for (size_t wordLength = 20; wordLength > 1; --wordLength)
    {
        joke_string[wordLength] = '\0';
        const char* const joke = joke_string.c_str();

        Benchmark("Basic - words of length " + std::to_string(wordLength), BenchmarkTiming::Best, iterCount, miniIterCount, [=]() {
            stringbuilder<> sb;
            sb.reserve(1000000);
            for (size_t charCount = 0; charCount < 1000000 - 100; charCount += wordLength)
            {
                sb.append_c_str(joke);
            }
#if STRINGBUILDER_USES_STRING_VIEW
            return sb.str_view();
#else
            return sb.str();
#endif
        });

        Benchmark("Progressive - words of length " + std::to_string(wordLength), BenchmarkTiming::Best, iterCount, miniIterCount, [=]() {
            stringbuilder<> sb;
            sb.reserve(1000000);
            for (size_t charCount = 0; charCount < 1000000 - 100; charCount += wordLength)
            {
                sb.append_c_str_progressive(joke);
            }
#if STRINGBUILDER_USES_STRING_VIEW
            return sb.str_view();
#else
            return sb.str();
#endif
            });
    }
}

int main(const int argc, const char* const argv[])
{
    stringbuilder<> joke_ss;
    joke_ss << "There are only " << "10 people in the world: those " << "who know binary and those who don't.";
    std::string joke_string = joke_ss.str();
    g_joke = joke_string.c_str();

    do {
        benchmarkIntegerSequence();
        benchmarkBook();
        benchmarkQuote();
        benchmarkAppend();
#ifdef STRINGBUILDER_SUPPORTS_MAKE_STRING
        benchmarkProgressiveAppend();
#endif
        benchmarkProgressiveThreshold();

        //if (vsize == 0 || vcstr == nullptr) std::cout << "vsize == 0 || vcstr == nullptr" << std::endl;
    } while (false);

    return 0;
}
