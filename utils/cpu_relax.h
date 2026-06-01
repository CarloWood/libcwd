// SPDX-FileCopyrightText: 2018, 2026 Carlo Wood
// SPDX-License-Identifier: MIT

#pragma once

[[gnu::always_inline]] inline static void cpu_relax()
{
#if defined(__x86_64__) || defined(__i386__)
  asm volatile("pause" ::: "memory");
#elif defined(__aarch64__)
  asm volatile("yield" ::: "memory");
#else
#error "Please extent utils/cpu_relax.h for your architecture."
#endif
}
