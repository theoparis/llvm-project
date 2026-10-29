# REQUIRES: aarch64
# RUN: llvm-mc -filetype=obj -triple=arm64-apple-macos %s -o %t.o
# RUN: not %no-arg-lld -arch arm64 -platform_version macos 11.0 11.0 \
# RUN:   -pagezero_size 0 -segaddr __DATA 0x1000 -o %t %t.o 2>&1 | FileCheck %s --check-prefix=ALIGN
# ALIGN: error: -segaddr: address for segment __DATA is not aligned to the target page size
# RUN: %no-arg-lld -arch arm64 -platform_version macos 11.0 11.0 \
# RUN:   -pagezero_size 0 -segaddr __DATA 0x10000 -o %t %t.o
# RUN: llvm-objdump --macho --private-headers %t | FileCheck %s --check-prefix=ADDRESS
# ADDRESS: segname __DATA
# ADDRESS-NEXT: vmaddr 0x0000000000010000

.text
.globl _main
_main:
  ret
.data
.quad 0
