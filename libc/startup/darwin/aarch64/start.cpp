//===-- Implementation of _start for darwin/aarch64 -----------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "startup/darwin/do_start.h"

// When linked with -lSystem, dyld is the loader and calls _start with
// argc/argv/envp/apple already in registers x0-x3.
extern "C" [[noreturn]] void _start(int argc, char **argv, char **envp,
                                    char **apple) {
  LIBC_NAMESPACE::do_start(argc, argv, envp, apple);
}
