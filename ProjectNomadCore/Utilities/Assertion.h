#pragma once

// uncomment to disable assert()
// #define NDEBUG

#include <cassert>

// TODO: Fairly certain these asserts don't even run in Unreal
// Need to test and create an alternative (eg, singleton + log in Unreal or perhaps something else)

// Use (void) to silent unused warnings.
// From https://en.cppreference.com/w/cpp/error/assert
#define assertm(exp, msg) assert(((void)msg, exp))

// I... have no idea where the below came from. Should work now given using C++17... methinks?
//template<typename T1, typename T2>
//constexpr auto assertm(T1 exp, T2  msg) { return assert(((void)msg, exp)); }

// Alternatively, I really want one that does a "__debugbreak()" or such
// Hazel (Cherno) example:
// https://youtu.be/88dmtleVywk?t=1297
// SO example:
// https://stackoverflow.com/questions/3314314/ways-to-show-your-co-programmers-that-some-methods-are-not-yet-implemented-in-a-c/3316954#3316954