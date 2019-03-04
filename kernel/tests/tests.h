
#pragma once

#include <lib/console.h>
#include <zircon/compiler.h>

__BEGIN_CDECLS

console_cmd uart_tests, thread_tests, sleep_tests, port_tests;
console_cmd clock_tests, timer_diag, timer_stress, benchmarks, fibo;
console_cmd spinner, ref_counted_tests, ref_ptr_tests;
console_cmd unique_ptr_tests, forward_tests, list_tests;
console_cmd hash_tests, vm_tests, auto_call_tests;
console_cmd arena_tests, fifo_tests, alloc_checker_tests;
console_cmd cpu_resume_tests;
void unittests(void);

__END_CDECLS
