
#pragma once

#include <unittest/unittest.h>

void watchdog_set_base_timeout(int seconds);

bool watchdog_is_enabled();

void watchdog_initialize();

void watchdog_terminate();

void watchdog_start(test_type_t type, const char* name);

void watchdog_cancel();
