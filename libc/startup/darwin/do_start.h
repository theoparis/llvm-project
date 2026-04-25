//===-- Header file of do_start for darwin --------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIBC_STARTUP_DARWIN_DO_START_H
#define LLVM_LIBC_STARTUP_DARWIN_DO_START_H

#include "src/__support/macros/config.h"

namespace LIBC_NAMESPACE_DECL {
// argc/argv/envp/apple are passed in registers by dyld (x0–x3).
[[noreturn]] void do_start(int argc, char **argv, char **envp, char **apple);
} // namespace LIBC_NAMESPACE_DECL

#endif // LLVM_LIBC_STARTUP_DARWIN_DO_START_H
