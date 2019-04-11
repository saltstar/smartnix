// Copyright 2018 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdio.h>

#include <utility>

#ifdef __Fuchsia__
#include <lib/zx/time.h>
#else
#include <sys/time.h>
#endif
#include <zxtest/base/reporter.h>
#include <zxtest/base/runner.h>

namespace zxtest {
namespace {

uint64_t now() {
#ifdef __Fuchsia__
    return (zx::clock::get_monotonic() - zx::time(0)).to_nsecs();
#else
    struct timeval tv;
    if (gettimeofday(&tv, nullptr) < 0)
        return 0u;
    return tv.tv_sec * 1000000000ull + tv.tv_usec * 1000ull;
#endif
}

template <typename T>
const char* Pluralize(T value, bool capitalize = false) {
    if (value > 1) {
        return capitalize ? "S" : "s";
    }
    return "";
}

} // namespace

namespace internal {

Timer::Timer()
    : start_(now()) {}

void Timer::Reset() {
    start_ = now();
}

int64_t Timer::GetElapsedTime() const {
    return (now() - start_) / 1000000;
}

void IterationSummary::Reset() {
    failed = 0;
    passed = 0;
    skipped = 0;
    failed_tests.reset();
}
} // namespace internal

Reporter::Reporter(FILE* stream)
    : stream_(stream) {}

void Reporter::OnProgramStart(const Runner& runner) {
    timers_.program.Reset();
}

void Reporter::OnIterationStart(const Runner& runner, int iteration) {
    timers_.iteration.Reset();
    iteration_summary_.Reset();

    if (stream_ == nullptr) {
        return;
    }

    if (runner.summary().total_iterations > 1) {
        fprintf(stream_, "\nRepeating all tests (iteration %d) . . .\n\n", iteration);
    }

    fprintf(stream_, "[==========] Running %lu test%s from %lu test case%s.\n",
            runner.summary().active_test_count, Pluralize(runner.summary().active_test_count),
            runner.summary().active_test_case_count,
            Pluralize(runner.summary().active_test_case_count));
}

void Reporter::OnEnvironmentSetUp(const Runner& runner) {

    if (stream_ == nullptr) {
        return;
    }

    fprintf(stream_, "[----------] Global test environment set-up.\n");
}

void Reporter::OnTestCaseStart(const TestCase& test_case) {
    timers_.test_case.Reset();

    if (stream_ == nullptr) {
        return;
    }

    fprintf(stream_, "[----------] %lu test%s from %s\n", test_case.MatchingTestCount(),
            Pluralize(test_case.MatchingTestCount()), test_case.name().c_str());
}

void Reporter::OnTestStart(const TestCase& test_case, const TestInfo& test) {
    timers_.test.Reset();

    if (stream_ == nullptr) {
        return;
    }

    fprintf(stream_, "[ RUN      ] %s.%s\n", test_case.name().c_str(), test.name().c_str());
}

void Reporter::OnAssertion(const Assertion& assertion) {
    fprintf(stream_, "%s:%ld: error: Failure:\n%s\n    Expected: %s\n",
            assertion.location().filename, assertion.location().line_number,
            assertion.description().c_str(), assertion.expected().c_str());
    // When it is not a literal.
    if (assertion.expected() != assertion.expected_eval()) {
        fprintf(stream_, "    Which is: %s\n", assertion.expected_eval().c_str());
    }

    fprintf(stream_, "    Actual  : %s\n", assertion.actual().c_str());
    // When it is not a literal.
    if (assertion.actual() != assertion.actual_eval()) {
        fprintf(stream_, "    Which is: %s\n", assertion.actual_eval().c_str());
    }
}

void Reporter::OnTestSkip(const TestCase& test_case, const TestInfo& test) {

    if (stream_ == nullptr) {
        return;
    }

    int64_t elapsed_time = timers_.test.GetElapsedTime();
    iteration_summary_.skipped++;
    fprintf(stream_, "[  SKIPPED ] %s.%s  (%ld ms)\n", test_case.name().c_str(),
            test.name().c_str(), elapsed_time);
}

void Reporter::OnTestFailure(const TestCase& test_case, const TestInfo& test) {

    if (stream_ == nullptr) {
        return;
    }

    int64_t elapsed_time = timers_.test.GetElapsedTime();
    char buffer[test_case.name().size() + test.name().size() + 2];
    sprintf(buffer, "%s.%s", test_case.name().c_str(), test.name().c_str());
    iteration_summary_.failed++;
    iteration_summary_.failed_tests.push_back(buffer);
    fprintf(stream_, "[  FAILED  ] %s.%s (%ld ms)\n", test_case.name().c_str(),
            test_case.name().c_str(), elapsed_time);
}

void Reporter::OnTestSuccess(const TestCase& test_case, const TestInfo& test) {

    if (stream_ == nullptr) {
        return;
    }

    int64_t elapsed_time = timers_.test.GetElapsedTime();
    iteration_summary_.passed++;
    fprintf(stream_, "[       OK ] %s.%s (%ld ms)\n", test_case.name().c_str(), test.name().c_str(),
            elapsed_time);
}

void Reporter::OnTestCaseEnd(const TestCase& test_case) {

    if (stream_ == nullptr) {
        return;
    }

    int64_t elapsed_time = timers_.test_case.GetElapsedTime();
    fprintf(stream_, "[----------] %lu test%s from %s (%ld ms total)\n\n",
            test_case.MatchingTestCount(), Pluralize(test_case.MatchingTestCount()),
            test_case.name().c_str(), elapsed_time);
}

void Reporter::OnEnvironmentTearDown(const Runner& runner) {

    if (stream_ == nullptr) {
        return;
    }

    fprintf(stream_, "[----------] Global test environment tear-down.\n");
}

void Reporter::OnIterationEnd(const Runner& runner, int iteration) {

    if (stream_ == nullptr) {
        return;
    }

    int64_t elapsed_time = timers_.iteration.GetElapsedTime();
    fprintf(stream_, "[==========] %ld test%s from %lu test case%s ran (%ld ms total).\n",
            runner.summary().active_test_count, Pluralize(runner.summary().active_test_count),
            runner.summary().active_test_case_count,
            Pluralize(runner.summary().active_test_case_count), elapsed_time);
    if (iteration_summary_.passed > 0) {
        fprintf(stream_, "[  PASSED  ] %lu test%s\n", iteration_summary_.passed,
                Pluralize(iteration_summary_.passed));
    }
    if (iteration_summary_.skipped > 0) {
        fprintf(stream_, "[  SKIPPED ] %lu test%s\n", iteration_summary_.skipped,
                Pluralize(iteration_summary_.skipped));
    }
    if (iteration_summary_.failed > 0) {
        fprintf(stream_, "[  FAILED  ] %lu test%s, listed below:\n", iteration_summary_.failed,
                Pluralize(iteration_summary_.failed));
        if (iteration_summary_.failed_tests.size() > 0) {
            for (auto& failed_test : iteration_summary_.failed_tests) {
                fprintf(stream_, "[  FAILED  ] %s\n", failed_test.c_str());
            }
            fprintf(stream_, "%ld FAILED TEST%s\n", iteration_summary_.failed,
                    Pluralize(iteration_summary_.failed, /*capitalize*/ true));
        }
    }
}

void Reporter::OnProgramEnd(const Runner& runner) {

    if (stream_ == nullptr) {
        return;
    }

    timers_.program.Reset();
}

} // namespace zxtest
