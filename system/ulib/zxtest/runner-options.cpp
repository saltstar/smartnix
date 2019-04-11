// Copyright 2019 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <getopt.h>

#include <fbl/auto_call.h>
#include <fbl/function.h>
#include <fbl/string_printf.h>
#include <zxtest/base/runner.h>

namespace zxtest {

namespace {
using Options = Runner::Options;

bool GetBoolFlag(const char* arg) {
    return arg == nullptr || strlen(arg) == 0 || strcmp("true", arg);
}

// getopt_long ignores -f value and --flag value opt args, and requires either = or -fvalue for
// optional arguments. This will return an opt arg if available.
char* GetOptArg(int opt, int argc, char** argv) {
    // We might need to check we ar enot skipping any argument.
    int index = opt;

    if (index >= argc) {
        return nullptr;
    }

    if (argv[index] == nullptr) {
        return nullptr;
    }

    if (argv[index][0] == '\0') {
        return nullptr;
    }

    if (argv[index][0] == '-') {
        return nullptr;
    }

    return argv[index];
}

} // namespace

void Options::Usage(char* bin, FILE* stream) {
    fprintf(stream, "\nUsage:\n");
    fprintf(stream, "  %s  [OPTIONS]\n", bin);
    fprintf(stream, "\n");
    fprintf(stream, "  [OPTIONS]\n");
    fprintf(stream, "  --help[-h]                       Prints this message.\n");
    fprintf(stream, "  --gtest_filter[-f] PATTERN       Runner will consider only registered\n");
    fprintf(stream, "                                   tests that match PATTERN.\n");
    fprintf(stream, "  --gtest_list_tests[-l] BOOL      Runner will list all registed tests\n");
    fprintf(stream, "                                   that would be executed.\n");
    fprintf(stream, "  --gtest_shuffle[-s] BOOL         Runner will shuffle test and test case\n");
    fprintf(stream, "                                   execution order.\n");
    fprintf(stream, "  --gtest_repeat[-i] REPEAT        Runner will run REPEAT iterations of\n");
    fprintf(stream, "                                   each test case.\n");
    fprintf(stream,
            "  --gtest_random_seed[-r] SEED     Runner will use SEED for random decisions.\n");
}

Options Options::FromArgs(int argc, char** argv, fbl::Vector<fbl::String>* errors) {
    // Reset index of parsed arguments.
    optind = 0;
    static const struct option opts[] = {
        {"help", optional_argument, nullptr, 'h'},
        {"gtest_filter", optional_argument, nullptr, 'f'},
        {"gtest_list_tests", optional_argument, nullptr, 'l'},
        {"gtest_shuffle", optional_argument, nullptr, 's'},
        {"gtest_repeat", required_argument, nullptr, 'i'},
        {"gtest_random_seed", required_argument, nullptr, 'r'},
        {0, 0, 0, 0},
    };
    Runner::Options options;

    auto reset = fbl::MakeAutoCall([]() { optind = 0; });

    int c = -1;
    int option_index = -1;
    char* val = nullptr;
    while ((c = getopt_long(argc, argv, "f::l::s::i:r:h::", opts, &option_index)) >= 0) {

        val = optarg;
        if (val == nullptr) {
            // Verifies that the flag value could be in the form -f value not just -fValue.
            val = GetOptArg(optind, argc, argv);
            if (val != nullptr) {
                ++optind;
            }
        }

        switch (c) {
        case 'h':
            options.help = GetBoolFlag(val);
            return options;
        case 'f':
            // -f with no args resets the filter.

            options.filter = (val == nullptr ? "" : val);
            break;
        case 'l':
            options.list = GetBoolFlag(val);
            break;
        case 's':
            options.shuffle = GetBoolFlag(val);
            break;
        case 'i': {
            int iters = atoi(val);
            if (iters <= 0) {
                options.help = true;
                errors->push_back(fbl::StringPrintf(
                    "--gtest_repeat(-i) must take a positive value. (value was %d)", iters));
                return options;
            }
            options.repeat = iters;
            break;
        }
        case 'r':
            options.seed = atoi(val);
            break;
        }
    }

    return options;
}

} // namespace zxtest
