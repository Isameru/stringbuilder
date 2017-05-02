# `stringbuilder` Utility Library

`stringbuilder` is a one-file header-only library providing classes and functions for building strings from smaller parts.
This utility is a fast, efficient and convenient alternative to std::stringstream and std::string concatenation.
It was inspired by `System.Text.StringBuilder` class available in .NET Framework Class Library, where as instance of `StringBuilder` class is created and its content built by successive calls to `Append()` method.

## Dependencies

The library takes advantage of C++17 features so a fairly modern compiler is needed (Visual Studio 2017 will do).

## Installation

```cpp
#include <stringbuilder.h>
```

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
`operator<<` appends portions of a final string: first using the in-place storage - then: dynamically allocating new chunks on a heap for additional storage.
To avoid dynamic memory allocation we can increase the in-place size:

```cpp
auto sb = stringbuilder<52>{};
...
```

As we know that the resulting string has exactly 52 characters, we can speed up the code:

```cpp
auto sb = inplace_stringbuilder<52>{};
...
```

`inplace_stringbuilder<MaxSize>` is a pure in-place character storage which can hold up to `MaxSize` characters and no more.
Exceeding the capacity of this container leads to an assertion failure or memory corruption so it must be used with caution.
