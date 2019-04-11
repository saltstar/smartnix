// Copyright 2018 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <fbl/string_piece.h>
#include <zxtest/base/runner.h>

namespace zxtest {

const Runner::Options Runner::kDefaultOptions;

namespace internal {

TestDriverImpl::TestDriverImpl() = default;
TestDriverImpl::~TestDriverImpl() = default;

void TestDriverImpl::Skip() {
    status_ = TestStatus::kSkipped;
}

bool TestDriverImpl::Continue() const {
    return !has_fatal_failures_;
}

void TestDriverImpl::OnTestStart(const TestCase& test_case, const TestInfo& test_info) {
    status_ = TestStatus::kPassed;
}

void TestDriverImpl::OnTestSkip(const TestCase& test_case, const TestInfo& test_info) {
    Reset();
}

void TestDriverImpl::OnTestSuccess(const TestCase& test_case, const TestInfo& test_info) {
    Reset();
}

void TestDriverImpl::OnTestFailure(const TestCase& test_case, const TestInfo& test_info) {
    Reset();
}

void TestDriverImpl::OnAssertion(const Assertion& assertion) {
    status_ = TestStatus::kFailed;
    has_fatal_failures_ = assertion.is_fatal();
    had_any_failures_ = true;
}

void TestDriverImpl::Reset() {
    has_fatal_failures_ = false;
    status_ = TestStatus::kPassed;
}

} // namespace internal

Runner::Runner(Reporter&& reporter) : reporter_(std::move(reporter)) {
    event_broadcaster_.Subscribe(&test_driver_);
    event_broadcaster_.Subscribe(&reporter_);
}
Runner::~Runner() = default;

TestRef Runner::RegisterTest(const fbl::String& test_case_name, const fbl::String& test_name,
                             const SourceLocation& location, internal::TestFactory factory,
                             internal::SetUpTestCaseFn set_up,
                             internal::TearDownTestCaseFn tear_down) {
    ZX_ASSERT_MSG(!test_case_name.empty(), "test_case_name cannot be an empty string.");
    ZX_ASSERT_MSG(!test_name.empty(), "test_name cannot be an empty string.");

    // TODO(gevalentino): replace by std::find.
    TestCase* target_test_case = nullptr;
    TestRef test_ref;

    for (auto& test_case : test_cases_) {
        if (test_case.name() == test_case_name) {
            target_test_case = &test_case;
            break;
        }
        test_ref.test_case_index++;
    }

    // If there is no existing test case with |test_case_name|, create a new one.
    if (target_test_case == nullptr) {
        test_cases_.push_back(TestCase(test_case_name, std::move(set_up), std::move(tear_down)));
        target_test_case = &test_cases_[test_cases_.size() - 1];
    }

    test_ref.test_index = target_test_case->TestCount();
    ZX_ASSERT_MSG(target_test_case->RegisterTest(test_name, location, std::move(factory)),
                  "Test Registration failed.");
    summary_.registered_test_count++;
    summary_.registered_test_case_count = test_cases_.size();

    return test_ref;
}

int Runner::Run(const Runner::Options& options) {
    summary_.total_iterations = options.repeat;
    Filter(options.filter);

    event_broadcaster_.OnProgramStart(*this);
    for (int i = 0; i < options.repeat; ++i) {
        event_broadcaster_.OnIterationStart(*this, i);
        event_broadcaster_.OnEnvironmentSetUp(*this);
        for (auto& test_case : test_cases_) {
            if (options.shuffle) {
                test_case.Shuffle(options.seed);
            }

            test_case.Run(&event_broadcaster_, &test_driver_);

            if (options.shuffle) {
                test_case.UnShuffle();
            }
        }
        event_broadcaster_.OnEnvironmentTearDown(*this);
        event_broadcaster_.OnIterationEnd(*this, i);
    }
    event_broadcaster_.OnProgramEnd(*this);

    return test_driver_.HadAnyFailures() ? -1 : 0;
}

void Runner::List(const Runner::Options& options) {
    summary_.total_iterations = options.repeat;
    Filter(options.filter);
    FILE* output = reporter_.stream();

    if (output == nullptr) {
        return;
    }

    for (const auto& test_case : test_cases_) {
        if (test_case.MatchingTestCount() == 0) {
            continue;
        }

        fprintf(output, "%s\n", test_case.name().c_str());
        for (size_t i = 0; i < test_case.MatchingTestCount(); ++i) {
            fprintf(output, "  .%s\n", test_case.GetMatchingTestInfo(i).name().c_str());
        }
    }
}

void Runner::Filter(const fbl::String& pattern) {
    summary_.active_test_count = 0;
    summary_.active_test_case_count = 0;
    const FilterOp filter_op = {.pattern = pattern};
    for (auto& test_case : test_cases_) {
        // TODO(gevalentino): replace with filter function.
        test_case.Filter(filter_op);
        if (test_case.MatchingTestCount() > 0) {
            summary_.active_test_case_count++;
            summary_.active_test_count += test_case.MatchingTestCount();
        }
    }
}

void Runner::NotifyAssertion(const Assertion& assertion) {
    event_broadcaster_.OnAssertion(assertion);
}

Runner* Runner::GetInstance() {
    static Runner runner = Runner(Reporter(stdout));
    return &runner;
}

int RunAllTests(int argc, char** argv) {
    fbl::Vector<fbl::String> errors;
    Runner::Options options = Runner::Options::FromArgs(argc, argv, &errors);

    if (!errors.is_empty()) {
        for (const auto& error : errors) {
            fprintf(stderr, "%s\n", error.c_str());
        }
        options.help = true;
    }

    // Errors will always set help to true.
    if (options.help) {
        Runner::Options::Usage(argv[0], stdout);
        return errors.is_empty();
    }

    if (options.list) {
        Runner::GetInstance()->List(options);
        return 0;
    }

    return Runner::GetInstance()->Run(options);
}

namespace {
bool MatchPatterns(fbl::StringPiece pattern, fbl::StringPiece str) {
    static constexpr auto match_pattern = [](const char* pattern, const char* str) -> bool {
        auto advance = [](const char* pattern, const char* str, auto& self) -> bool {
            switch (*pattern) {
            // Single characeter matching for gTest
            case '?':
                return *str != '\0' && self(pattern + 1, str + 1, self);
            // Wild card matches anything.
            case '*':
                return (*str != '\0' && self(pattern, str + 1, self)) ||
                       self(pattern + 1, str, self);
            // Pattern completed or another pattern in the list.
            case '\0':
            case ':':
                return *str == '\0';
            // 1:1 match
            default:
                return *str == *pattern && self(pattern + 1, str + 1, self);
            };
        };
        return advance(pattern, str, advance);
    };

    bool has_next = true;
    const char* curr_pattern = pattern.data();
    while (has_next) {
        if (match_pattern(curr_pattern, str.data())) {
            return true;
        }
        curr_pattern = strchr(curr_pattern, ':');
        has_next = (curr_pattern != nullptr);
        // Skip ':'
        curr_pattern++;
    }

    return false;
}

} // namespace

bool FilterOp::operator()(const fbl::String& test_case, const fbl::String& test) const {
    fbl::String full_test_name = fbl::StringPrintf("%s.%s", test_case.c_str(), test.c_str());

    const char* p = pattern.c_str();
    const char* d = strchr(p, '-');
    fbl::StringPiece positive, negative;

    // No negative string.
    if (d == nullptr) {
        positive = fbl::StringPiece(p, pattern.size());
    } else {
        size_t delta = d - p;
        positive = fbl::StringPiece(p, delta);
        negative = fbl::StringPiece(d + 1, pattern.size() - delta - 1);
    }
    return (positive.empty() || MatchPatterns(positive, full_test_name)) &&
           (negative.empty() || !MatchPatterns(negative, full_test_name));
}

} // namespace zxtest
