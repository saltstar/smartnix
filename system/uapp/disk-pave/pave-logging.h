
#pragma once

#include <stdio.h>

#define PAVER_PREFIX "paver:"
#define ERROR(fmt, ...) fprintf(stderr, PAVER_PREFIX "[%s] " fmt, __FUNCTION__, ##__VA_ARGS__);
#define LOG(fmt, ...) fprintf(stderr, PAVER_PREFIX "[%s] " fmt, __FUNCTION__, ##__VA_ARGS__);

