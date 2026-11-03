#ifndef TESTS_H
#define TESTS_H

#include <string>
#include <vector>

struct TestResult{
    std::string name;
    bool passed;
    std::string detail;         // why it failed (empty when it passed)
    double ms;                  // time the test took
};

// Runs every automated test and returns one result per test. Uses its own temporary
// chains/tables; the only shared state it touches is a throw-away ballot file and
// a few temporary users, which are removed again before it returns.
std::vector<TestResult> runAutomatedTests();

#endif
