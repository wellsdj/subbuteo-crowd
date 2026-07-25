// A deliberately tiny test harness.
//
// The core library has no dependencies, and this keeps it that way: the tests
// build and run anywhere with a C++17 compiler, with nothing to download.

#pragma once

#include <cmath>
#include <functional>
#include <iostream>
#include <string>
#include <vector>

namespace testing
{

struct TestCase
{
    std::string           name;
    std::function<void()> body;
};

inline std::vector<TestCase>& registry()
{
    static std::vector<TestCase> tests;
    return tests;
}

inline int& failureCount()
{
    static int failures = 0;
    return failures;
}

inline std::string& currentTest()
{
    static std::string name;
    return name;
}

struct Registrar
{
    Registrar (const std::string& name, std::function<void()> body)
    {
        registry().push_back ({ name, std::move (body) });
    }
};

inline void reportFailure (const std::string& detail, int line)
{
    ++failureCount();
    std::cout << "    FAIL (line " << line << "): " << detail << '\n';
}

inline void check (bool condition, const std::string& expression, int line)
{
    if (! condition)
        reportFailure (expression, line);
}

inline void checkNear (double actual, double expected, double tolerance,
                       const std::string& expression, int line)
{
    if (std::abs (actual - expected) > tolerance)
        reportFailure (expression + "  (expected " + std::to_string (expected)
                       + ", got " + std::to_string (actual) + ")", line);
}

inline void checkEqualInt (long long actual, long long expected,
                           const std::string& expression, int line)
{
    if (actual != expected)
        reportFailure (expression + "  (expected " + std::to_string (expected)
                       + ", got " + std::to_string (actual) + ")", line);
}

inline int runAll()
{
    int passed = 0;

    for (auto& test : registry())
    {
        currentTest() = test.name;

        const int before = failureCount();
        std::cout << "  " << test.name << '\n';
        test.body();

        if (failureCount() == before)
            ++passed;
    }

    std::cout << "\n" << passed << " / " << registry().size() << " tests passed";
    std::cout << ", " << failureCount() << " assertion failure(s)\n";

    return failureCount() == 0 ? 0 : 1;
}

} // namespace testing

#define TEST(name)                                                             \
    static void name();                                                        \
    static ::testing::Registrar registrar_##name (#name, name);                \
    static void name()

#define CHECK(cond)          ::testing::check ((cond), #cond, __LINE__)
#define CHECK_NEAR(a, b, t)  ::testing::checkNear ((a), (b), (t), #a " ~= " #b, __LINE__)
#define CHECK_EQ(a, b)       ::testing::checkEqualInt ((a), (b), #a " == " #b, __LINE__)
