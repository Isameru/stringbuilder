![Logo](logo.png) `stringbuilder` Utility Library
===========

[![License](https://img.shields.io/badge/license-MIT-blue.svg?maxAge=3600)](https://raw.githubusercontent.com/isameru/stringbuilder/master/LICENSE)
[![Travis](https://img.shields.io/travis/Isameru/stringbuilder/master.svg?label=Travis)](https://travis-ci.org/Isameru/stringbuilder)
[![AppVeyor](https://img.shields.io/appveyor/ci/Isameru/stringbuilder/master.svg?label=AppVeyor)](https://ci.appveyor.com/project/Isameru/stringbuilder)
[![Codecov](https://img.shields.io/codecov/c/github/Isameru/stringbuilder/master.svg)](https://codecov.io/github/Isameru/stringbuilder?branch=master)

`stringbuilder` is a one-file header-only library providing classes and functions for building strings from smaller parts.
This utility is a fast, efficient and convenient alternative to std::stringstream and std::string concatenation.
It was inspired by `System.Text.StringBuilder` class available in .NET Framework Class Library, where an instance of `StringBuilder` class is created and its content built by successive calls to `Append()` method.

The library takes advantage of C++17 features (most notably `std::string_view`), but will also work with C++14 and C++11 in limited form.

You may find this library not sufficient to your needs, as it does not implement `std::ostream`, nor honor the stream formatters (e.g. `std::hex`). A lot of features need to be added to this project to make it decent.

## Installation

```cpp
#include <stringbuilder.h>
using namespace sbldr;  // Worry not - it will not pollute your scope with unnecessary symbols (check for yourself!)
```

Place `stringbuilder.h` somewhere in your source tree or include the project directory with *CMake* and link with the target `stringbuilder`.

## Basic Usage

Let's print several numbers of a Fibonacci sequence:

```cpp
auto sb = stringbuilder<10>{};
int fn0 = 1, fn1 = 0, fn2;
sb << "Fibonacci numbers: " << fn1 << ' ' << fn0;
while (fn0 < 100) {
    fn2 = fn1;   fn1 = fn0;   fn0 = fn1 + fn2;
    sb << ' ' << fn0;
}
sb << '\n';
std::cout << sb.str();
std::cout << "String size: " << sb.size();
```

The output of this program is:

```
Fibonacci numbers: 0 1 1 2 3 5 8 13 21 34 55 89 144
String size: 52
```

`stringbuilder<InPlaceSize>` is a storage of characters (of type `char`) which holds up to `InPlaceSize` character on itself (in-place, so effectively on a thread's stack).
`operator<<` appends portions of a final string: first using the in-place storage - then: dynamically allocating new chunks on the heap for additional storage.
To avoid dynamic memory allocation we can increase the in-place size:

```cpp
auto sb = stringbuilder<52>{};
// ...
```

As we know that the resulting string has exactly 52 characters, we can speed up the code:

```cpp
auto sb = inplace_stringbuilder<52>{};
// ...
```

`inplace_stringbuilder<MaxSize>` is a pure in-place character storage which can hold up to `MaxSize` characters and no more.
Exceeding the capacity of this container leads to an assertion failure or memory corruption so it must be used with caution.

## `make_string` *(C++17)*

Suppose we need to build an error message - we can do it this way:

```cpp
const auto fileName = std::string{"settings.config"};
const int errorCode = 3075;
// ...
auto errorMessage = make_string("error", ": ", "Cannot access file ", '"', sized_str<32>(fileName), '"', " (", "errorCode:", errorCode, ')');
```

This code produces `std::string` with the following message:

`error: Cannot access file "settings.config" (errorCode:3075)`

`make_string` first estimates the resulting string size in compile-time. It is easy for constexpr objects like characters, string literals and integral numbers, but for run-time objects (like the examplar `std::string fileName`) a hint in a form of `sized_str<ExpectedSize>(str)` is necessary.
In this particular case if `str.size() <= 32` then `make_string` guarantees not to perform a single dynamic memory allocation on the heap, but rather build the string entirely on the current thread's stack.

Now suppose we have a simplier case where all of the string components are constexpr:

```cpp
constexpr const char fileName[] = "settings.config";
constexpr auto errorMessage = make_string("error", ": ", "Cannot access file ", '"', fileName, '"');
constexpr auto errorMessage_c_str = errorMessage.c_str();
// errorMessage_c_str is a compile-time string literal: "error: Cannot access file "settings.config""
```

In this case `make_string` returns a compile-time object of type `constexpr_str` which provides `c_str()` member function to `constexpr const char*`.
This means that the run-time overhead of this code is exactly none.
