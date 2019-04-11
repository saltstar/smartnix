// Copyright 2018 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <cstdio>

#include <fbl/string.h>
#include <fbl/vector.h>
#include <zxtest/base/assertion.h>
#include <zxtest/base/event-broadcaster.h>
#include <zxtest/base/observer.h>
#include <zxtest/base/reporter.h>
#include <zxtest/base/test-case.h>
#include <zxtest/base/test-driver.h>
#include <zxtest/base/test-info.h>

namespace zxtest {

namespace internal {

// Test Driver implementation for the runner. Observes lifecycle events to
// reset the test state correctly.
class TestDriverImpl : public TestDriver, public LifecycleObserver {
public:
    TestDriverImpl();
    ~TestDriverImpl() final;

    // Called when a test is skipped.
    void Skip() final;

    // Return true if the is allowed to continue execution.
    bool Continue() const final;

    // Returns the status of the current test.
    TestStatus Status() const final { return status_; }

    // Reports before every test starts.
    void OnTestStart(const TestCase& test_case, const TestInfo& test) final;

    // Reports when current test assert condition fails.
    void OnAssertion(const Assertion& assertion) final;

    // Reports after a test execution was skipped.
    void OnTestSkip(const TestCase& test_case, const TestInfo& test) final;

    // Reports after test execution completed with failures.
    void OnTestFailure(const TestCase& test_case, const TestInfo& test) final;

    // Reports after test execution completed with no failures.
    void OnTestSuccess(const TestCase& test_case, const TestInfo& test) final;

    // Resets the states for running new tests.
    void Reset();

    // Returns whether any test driven by this instance had any test failure.
    // This is not cleared on |TestDriverImpl::Reset|.
    bool HadAnyFailures() const { return had_any_failures_; }

private:
    TestStatus status_ = TestStatus::kFailed;

    bool has_fatal_failures_ = false;

    bool had_any_failures_ = false;
};
} // namespace internal

// Struct used to safely reference a registered test. This is not affected
// by vector growth.
struct TestRef {
    size_t test_case_index = 0;
    size_t test_index = 0;
};

// Returns the amount of registered and active test and testcases.
struct RunnerSummary {
    // Number of iterations to run.
    size_t total_iterations = 1;
    // Number of registered tests that match a filter.
    size_t active_test_count = 0;
    // Number of registered test cases that match a filter.
    size_t active_test_case_count = 0;
    // Number of registered tests.
    size_t registered_test_count = 0;
    // Number of registered test cases.
    size_t registered_test_case_count = 0;
};

// Holds the pattern used for filtering.
struct FilterOp {
    // Returns true if the test_case and test matches |pattern|.
    bool operator()(const fbl::String& test_case, const fbl::String& test) const;

    fbl::String pattern;
};

// This class is the entry point for test and constructs registration.
class Runner {
public:
    struct Options {
        // Parses the contents of argv into |Options|.
        static Options FromArgs(int argc, char** argv, fbl::Vector<fbl::String>* errors);

        // Prints the usage message into the |stream|.
        static void Usage(char* bin, FILE* stream);

        // Pattern for filtering tests. Empty pattern matches all.
        fbl::String filter;

        // Seed used for random decisions.
        int seed = 0;

        // Number of iterations to run.
        int repeat = 1;

        // When set test order within a test case are randomized.
        bool shuffle = false;

        // When set prints the help message.
        bool help = false;

        // When set list all registered tests.
        bool list = false;
    };

    // Default Runner options.
    static const Options kDefaultOptions;

    // Returns process shared |Runner| instance.
    static Runner* GetInstance();

    Runner() = delete;
    explicit Runner(Reporter&& reporter);
    Runner(const Runner&) = delete;
    Runner(Runner&&) = delete;
    ~Runner();

    Runner& operator=(const Runner&) = delete;
    Runner& operator=(Runner&&) = delete;

    // Register a test for execution with the default factory.
    template <typename TestBase, typename TestImpl>
    TestRef RegisterTest(const fbl::String& test_case_name, const fbl::String& test_name,
                         const char* filename, int line) {
        return RegisterTest<TestBase, TestImpl>(test_case_name, test_name, filename, line,
                                                &Test::Create<TestImpl>);
    }

    // Register a test for execution with a customized factory.
    template <typename TestBase, typename TestImpl>
    TestRef RegisterTest(const fbl::String& test_case_name, const fbl::String& test_name,
                         const char* filename, int line, internal::TestFactory factory) {
        static_assert(std::is_base_of<Test, TestImpl>::value, "Must inherit from Test");
        SourceLocation location = {.filename = filename, .line_number = line};
        return RegisterTest(test_case_name, test_name, location, std::move(factory),
                            &TestBase::SetUpTestCase, &TestBase::TearDownTestCase);
    }

    // Runs the registered tests with the specified |options|.
    int Run(const Options& options);

    // List tests according to options.
    void List(const Options& options);

    const RunnerSummary& summary() const { return summary_; }

    const TestInfo& GetTestInfo(const TestRef& test_ref) {
        return test_cases_[test_ref.test_case_index].GetTestInfo(test_ref.test_index);
    }

    // Provides an entry point for asertions. The runner will propagate the assertion to the
    // interested parties. This is needed in a global scope, because helper methods do not have
    // access to a |Test| instance and legacy tests are not part of a Fixture, but wrapped by one.
    // If this is called without any test running, it will have no effect.
    void NotifyAssertion(const Assertion& assertion);

    // Returns true if the current test should be aborted. This happens as a result of a fatal
    // failure.
    bool ShouldAbortCurrentTest() { return !test_driver_.Continue(); }

private:
    TestRef RegisterTest(const fbl::String& test_case_name, const fbl::String& test_name,
                         const SourceLocation& location, internal::TestFactory factory,
                         internal::SetUpTestCaseFn set_up, internal::TearDownTestCaseFn tear_down);

    void Filter(const fbl::String& pattern);

    // List of registered test cases.
    fbl::Vector<TestCase> test_cases_;

    // Serves as a |LifecycleObserver| list where events are sent to all subscribed observers.
    internal::EventBroadcaster event_broadcaster_;

    // Driver owned by the |Runner| instance, which drives tests registered for execution
    // with the given instance. We need at the |Runner| level, to reduce the amount of piping
    // and exposure of the internal classes, so we can propagate errors in Helper methods
    // or those that are not within a Fixture scope.
    internal::TestDriverImpl test_driver_;

    // Provides human readable output.
    Reporter reporter_;

    // Runner information.
    RunnerSummary summary_;
};

// Entry point for C++
int RunAllTests(int argc, char** argv);

} // namespace zxtest
