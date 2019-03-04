
#include "benchmarks.h"

#include <stdio.h>

#define NTRACE
#include <trace/event.h>

#include "runner.h"

static void NullSetup() {}
static void NullTeardown() {}

void RunNoTraceBenchmarks() {
    RunAndMeasure(
        "TRACE_ENABLED", "NTRACE",
        [] {
             ZX_DEBUG_ASSERT(!TRACE_ENABLED());
        },
        NullSetup, NullTeardown);

    RunAndMeasure(
        "TRACE_CATEGORY_ENABLED", "NTRACE",
        [] {
             ZX_DEBUG_ASSERT(!TRACE_CATEGORY_ENABLED("+enabled"));
        },
        NullSetup, NullTeardown);

    RunAndMeasure(
        "TRACE_DURATION_BEGIN macro with 0 arguments", "NTRACE",
        [] {
             TRACE_DURATION_BEGIN("+enabled", "name");
        },
        NullSetup, NullTeardown);

    RunAndMeasure(
        "TRACE_DURATION_BEGIN macro with 1 int32 argument", "NTRACE",
        [] {
             TRACE_DURATION_BEGIN("+enabled", "name",
                                  "k1", 1);
        },
        NullSetup, NullTeardown);

    RunAndMeasure(
        "TRACE_DURATION_BEGIN macro with 4 int32 arguments", "NTRACE",
        [] {
             TRACE_DURATION_BEGIN("+enabled", "name",
                                  "k1", 1, "k2", 2, "k3", 3, "k4", 4);
        },
        NullSetup, NullTeardown);

    RunAndMeasure(
        "TRACE_DURATION_BEGIN macro with 8 int32 arguments", "NTRACE",
        [] {
             TRACE_DURATION_BEGIN("+enabled", "name",
                                  "k1", 1, "k2", 2, "k3", 3, "k4", 4,
                                  "k5", 5, "k6", 6, "k7", 7, "k8", 8);
        },
        NullSetup, NullTeardown);

    RunAndMeasure(
        "TRACE_VTHREAD_DURATION_BEGIN macro with 0 arguments", "NTRACE",
        [] {
             TRACE_VTHREAD_DURATION_BEGIN("+enabled", "name", "vthread", 1);
        },
        NullSetup, NullTeardown);
}
