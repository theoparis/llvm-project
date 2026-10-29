# REQUIRES: x86
# RUN: rm -rf %t && mkdir -p %t
# RUN: llvm-mc -filetype=obj -triple=x86_64-apple-darwin %s -o %t/test.o
# RUN: %no-arg-lld -arch x86_64 -platform_version macos 11.0 11.0 \
# RUN:   -no_pie -pagezero_size 0 \
# RUN:   -segment_order __TEXT:__THIRD:__FIRST:__SECOND:__LINKEDIT \
# RUN:   -o %t/out %t/test.o
# RUN: llvm-objdump --macho --private-headers %t/out | FileCheck %s
# RUN: not %no-arg-lld -arch x86_64 -platform_version macos 11.0 11.0 \
# RUN:   -segment_order __TEXT:__TEXT -o /dev/null %t/test.o 2>&1 | \
# RUN:   FileCheck %s --check-prefix=DUPLICATE

# CHECK: segname __TEXT
# CHECK: segname __THIRD
# CHECK: segname __FIRST
# CHECK: segname __SECOND
# CHECK: segname __UNLISTED
# CHECK: segname __LINKEDIT

# RUN: %no-arg-lld -arch x86_64 -platform_version macos 11.0 11.0 \
# RUN:   -pagezero_size 0 -segment_order __THIRD -o %t/omitted %t/test.o
# RUN: llvm-objdump --macho --private-headers %t/omitted | FileCheck %s --check-prefix=OMITTED
# OMITTED: segname __TEXT
# OMITTED: segname __THIRD
# RUN: %no-arg-lld -arch x86_64 -platform_version macos 11.0 11.0 \
# RUN:   -pagezero_size 0 -segment_order __THIRD:__TEXT -o %t/reversed %t/test.o
# RUN: llvm-objdump --macho --private-headers %t/reversed | FileCheck %s --check-prefix=OMITTED

# DUPLICATE: error: -segment_order: duplicate segment __TEXT

.text
.globl _main
_main:
  retq

.section __FIRST,__first
.quad 0

.section __SECOND,__second
.quad 0

.section __THIRD,__third
.quad 0

.section __UNLISTED,__unlisted
.quad 0
