# REQUIRES: x86
# RUN: rm -rf %t && mkdir -p %t
# RUN: llvm-mc -filetype=obj -triple=x86_64-apple-darwin %s -o %t/test.o
# RUN: %no-arg-lld -arch x86_64 -platform_version macos 11.0 11.0 \
# RUN:   -no_pie -pagezero_size 0 -image_base 100000 \
# RUN:   -segaddr __FIXED 0x400000 -o %t/out %t/test.o
# RUN: llvm-objdump --macho --private-headers %t/out | FileCheck %s
# RUN: not %no-arg-lld -arch x86_64 -platform_version macos 11.0 11.0 \
# RUN:   -image_base invalid -o /dev/null %t/test.o 2>&1 | \
# RUN:   FileCheck %s --check-prefix=BAD-BASE
# RUN: not %no-arg-lld -arch x86_64 -platform_version macos 11.0 11.0 \
# RUN:   -segaddr __FIXED 0x400001 -o /dev/null %t/test.o 2>&1 | \
# RUN:   FileCheck %s --check-prefix=BAD-SEGADDR

# CHECK:      segname __TEXT
# CHECK-NEXT: vmaddr 0x0000000000100000
# CHECK:      segname __DATA
# CHECK-NEXT: vmaddr 0x0000000000101000
# CHECK:      segname __FIXED
# CHECK-NEXT: vmaddr 0x0000000000400000
# CHECK:      segname __LINKEDIT
# CHECK-NEXT: vmaddr 0x0000000000401000

# BAD-BASE: error: -image_base: failed to parse 'invalid' as an address
# BAD-SEGADDR: error: -segaddr: address for segment __FIXED is not aligned to the target page size

# RUN: not %no-arg-lld -arch x86_64 -platform_version macos 11.0 11.0 \
# RUN:   -image_base 100000 -o /dev/null %t/test.o 2>&1 | FileCheck %s --check-prefix=PAGEZERO
# PAGEZERO: error: -image_base overlaps __PAGEZERO
# RUN: not %no-arg-lld -arch x86_64 -platform_version macos 11.0 11.0 \
# RUN:   -pagezero_size 0 -image_base 100000 -segaddr __FIXED 100000 \
# RUN:   -o /dev/null %t/test.o 2>&1 | FileCheck %s --check-prefix=OVERLAP
# OVERLAP: error: segment __FIXED overlaps or precedes an earlier segment
# RUN: %no-arg-lld -arch x86_64 -platform_version macos 11.0 11.0 \
# RUN:   -pagezero_size 0 -segaddr __LINKEDIT 800000 -o %t/linkedit %t/test.o
# RUN: llvm-objdump --macho --private-headers %t/linkedit | FileCheck %s --check-prefix=LINKEDIT
# LINKEDIT: segname __LINKEDIT
# LINKEDIT-NEXT: vmaddr 0x0000000000800000
# RUN: %no-arg-lld -arch x86_64 -platform_version macos 11.0 11.0 \
# RUN:   -dylib -image_base abc000 -o %t/dylib %t/test.o
# RUN: llvm-objdump --macho --private-headers %t/dylib | FileCheck %s --check-prefix=DYLIB
# DYLIB: segname __TEXT
# DYLIB-NEXT: vmaddr 0x0000000000abc000
# RUN: not %no-arg-lld -arch x86_64 -platform_version macos 11.0 11.0 \
# RUN:   -image_base ffffffffffffffff -o /dev/null %t/test.o 2>&1 | FileCheck %s --check-prefix=OVERFLOW
# OVERFLOW: error: -image_base: address is too large to align

.text
.globl _main
_main:
  retq

.data
.quad 0

.section __FIXED,__fixed
.quad 0
