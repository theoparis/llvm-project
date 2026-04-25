//===-- Shared cache ABI for darwin/aarch64 startup -------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIBC_STARTUP_DARWIN_SHARED_CACHE_H
#define LLVM_LIBC_STARTUP_DARWIN_SHARED_CACHE_H

#include "hdr/stdint_proxy.h"

// Syscall numbers (macOS arm64, BSD class = 0x2000000).
#define SYS_open 0x2000005
#define SYS_close 0x2000006
#define SYS_pread 0x2000099
#define SYS_exit 0x2000001
#define SYS_mmap 0x20000C5
#define SYS_shared_region_map_and_slide_2_np 0x2000218

// VM protection flags.
#define VM_PROT_READ 0x01
#define VM_PROT_WRITE 0x02
#define VM_PROT_EXECUTE 0x04
#define VM_PROT_SLIDE 0x20
#define VM_PROT_NOAUTH 0x40

// mmap flags.
#define MAP_ANON 0x1000
#define MAP_PRIVATE 0x0002
#define PROT_READ 0x01
#define PROT_WRITE 0x02

// Open flags.
#define O_RDONLY 0

// Shared region base for arm64.
#define SHARED_REGION_BASE_ARM64 0x180000000ULL

// dyld_cache_mapping_and_slide_info flags.
#define DYLD_CACHE_MAPPING_AUTH_DATA (1U << 0)

// Path to the shared cache on macOS (Cryptex location).
#define DYLD_SHARED_CACHE_PATH                                                 \
  "/System/Volumes/Preboot/Cryptexes/OS/System/Library/dyld/"                  \
  "dyld_shared_cache_arm64e"

// Upper bound on mappings we support at startup.
#define MAX_CACHE_MAPPINGS 16

// Minimal subset of dyld_cache_header. The real header is much larger but
// we only need enough fields to reach mappingWithSlideOffset/Count.
// Field offsets must match the Apple dyld_cache_format.h layout.
struct dyld_cache_header {
  char magic[16];
  uint32_t mappingOffset;
  uint32_t mappingCount;
  uint32_t imagesOffsetOld;
  uint32_t imagesCountOld;
  uint64_t dyldBaseAddress;
  uint64_t codeSignatureOffset;
  uint64_t codeSignatureSize;
  uint64_t slideInfoOffsetUnused;
  uint64_t slideInfoSizeUnused;
  uint64_t localSymbolsOffset;
  uint64_t localSymbolsSize;
  uint8_t uuid[16];
  uint64_t cacheType;
  uint32_t branchPoolsOffset;
  uint32_t branchPoolsCount;
  uint64_t dyldInCacheMH;
  uint64_t dyldInCacheEntry;
  uint64_t imagesTextOffset;
  uint64_t imagesTextCount;
  uint64_t patchInfoAddr;
  uint64_t patchInfoSize;
  uint64_t otherImageGroupAddrUnused;
  uint64_t otherImageGroupSizeUnused;
  uint64_t progClosuresAddr;
  uint64_t progClosuresSize;
  uint64_t progClosuresTrieAddr;
  uint64_t progClosuresTrieSize;
  uint32_t platform;
  uint32_t formatBits;
  uint64_t sharedRegionStart;
  uint64_t sharedRegionSize;
  uint64_t maxSlide;
  uint64_t dylibsImageArrayAddr;
  uint64_t dylibsImageArraySize;
  uint64_t dylibsTrieAddr;
  uint64_t dylibsTrieSize;
  uint64_t otherImageArrayAddr;
  uint64_t otherImageArraySize;
  uint64_t otherTrieAddr;
  uint64_t otherTrieSize;
  uint32_t mappingWithSlideOffset;
  uint32_t mappingWithSlideCount;
};

struct dyld_cache_mapping_and_slide_info {
  uint64_t address;
  uint64_t size;
  uint64_t fileOffset;
  uint64_t slideInfoFileOffset;
  uint64_t slideInfoFileSize;
  uint64_t flags;
  uint32_t maxProt;
  uint32_t initProt;
};

// Kernel ABI structs for shared_region_map_and_slide_2_np (syscall 536).
struct shared_file_np {
  int sf_fd;
  uint32_t sf_mappings_count;
  uint32_t sf_slide;
};

struct shared_file_mapping_slide_np {
  uint64_t sms_address;
  uint64_t sms_size;
  uint64_t sms_file_offset;
  uint64_t sms_slide_size;
  uint64_t sms_slide_start;
  uint32_t sms_max_prot;
  uint32_t sms_init_prot;
};

#endif // LLVM_LIBC_STARTUP_DARWIN_SHARED_CACHE_H
