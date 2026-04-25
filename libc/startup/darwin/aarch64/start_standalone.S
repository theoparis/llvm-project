//===-- start_standalone.S - Kernel-direct entry for Darwin/AArch64 ------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Entry point for the standalone (no system dyld) path.
//
// When XNU starts a process without LC_LOAD_DYLINKER the kernel jumps directly
// to the entry point with the stack arranged as:
//
//   SP ->  argc           (8 bytes, uintptr_t)
//          argv[0]        (8 bytes each)
//          ...
//          argv[argc-1]
//          NULL           (argv terminator)
//          envp[0]
//          ...
//          envp[n-1]
//          NULL           (envp terminator)
//          apple[0]
//          ...
//          NULL           (apple terminator)
//
// General-purpose registers are undefined on kernel entry.
// SP is 16-byte aligned by the kernel.
//
// This stub parses the stack into (argc, argv, envp, apple) and tail-calls
// libc_standalone_start(), which maps the dyld shared cache and calls main().
//
//===----------------------------------------------------------------------===//

    .text
    .p2align 2
    .globl  __start
__start:
    // x0 = argc (kernel stores it as uintptr_t but it always fits in int)
    ldr     x0, [sp]

    // x1 = argv = SP + 8
    add     x1, sp, #8

    // x2 = envp = argv + argc + 1  (skip argc pointers + NULL terminator)
    add     x2, x1, x0, lsl #3
    add     x2, x2, #8

    // x3 = apple: scan past envp entries until the NULL terminator
    mov     x3, x2
Lscan_envp:
    ldr     x4, [x3], #8
    cbnz    x4, Lscan_envp
    // x3 now points to apple[0]

    // Tail-call into C; libc_standalone_start is [[noreturn]]
    // SP is still kernel-provided and 16-byte aligned — we have not touched it.
    bl      _libc_standalone_start
    // Should never return; trap defensively
    brk     #1
