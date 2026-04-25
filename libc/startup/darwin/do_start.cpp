//===-- Implementation of do_start for darwin -----------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "startup/darwin/do_start.h"
#include "startup/darwin/shared_cache.h"
#include "config/app.h"
#include "src/__support/macros/config.h"

extern "C" int main(int argc, char **argv, char **envp);

// POSIX environ global. Defined here since this is the startup object.
extern "C" {
char **environ = nullptr;
}

namespace LIBC_NAMESPACE_DECL {

AppProperties app;

// Darwin arm64 syscall convention: x16 = syscall number, x0-x5 = args.
// On error the carry flag is set and x0 contains the errno value.
// Returns the raw x0 result; sets *err to 1 on error, 0 on success.
namespace {

[[gnu::always_inline]] inline long
darwin_syscall(long number, long a0, long a1, long a2, long a3, long a4,
               long a5, int *err) {
  register long x16 __asm__("x16") = number;
  register long x0 __asm__("x0") = a0;
  register long x1 __asm__("x1") = a1;
  register long x2 __asm__("x2") = a2;
  register long x3 __asm__("x3") = a3;
  register long x4 __asm__("x4") = a4;
  register long x5 __asm__("x5") = a5;
  int carry;
  __asm__ volatile("svc 0x80\n"
                   "cset %w[carry], cs"
                   : "=r"(x0), [carry] "=r"(carry)
                   : "r"(x16), "r"(x0), "r"(x1), "r"(x2), "r"(x3), "r"(x4),
                     "r"(x5)
                   : "memory", "cc");
  *err = carry;
  return x0;
}

template <typename... Args>
[[gnu::always_inline]] inline long checked_syscall(long number, Args... args) {
  static_assert(sizeof...(args) <= 6, "Too many arguments for syscall");
  // Pad remaining arguments with zeros up to 6.
  auto call = [&](long a0, long a1, long a2, long a3, long a4, long a5) {
    int err;
    long ret = darwin_syscall(number, a0, a1, a2, a3, a4, a5, &err);
    return err ? -ret : ret;
  };
  // Expand variadic args, padding with zeros.
  constexpr int N = sizeof...(args);
  long a[6] = {0, 0, 0, 0, 0, 0};
  long pack[] = {static_cast<long>(args)..., 0};
  for (int i = 0; i < N; ++i)
    a[i] = pack[i];
  return call(a[0], a[1], a[2], a[3], a[4], a[5]);
}

[[noreturn]] void die() {
  for (;;)
    checked_syscall(SYS_exit, 127);
}

// Read exactly |size| bytes at |offset| from |fd| into |buf|.
// Returns true on success.
bool pread_full(int fd, void *buf, uint64_t size, uint64_t offset) {
  auto *p = static_cast<char *>(buf);
  while (size > 0) {
    long n = checked_syscall(SYS_pread, fd, reinterpret_cast<long>(p),
                             static_cast<long>(size),
                             static_cast<long>(offset));
    if (n <= 0)
      return false;
    p += n;
    size -= static_cast<uint64_t>(n);
    offset += static_cast<uint64_t>(n);
  }
  return true;
}

bool str_eq(const char *a, const char *b) {
  while (*a && *b) {
    if (*a++ != *b++)
      return false;
  }
  return *a == *b;
}

void map_shared_cache() {
  // Open the shared cache file.
  int fd = static_cast<int>(checked_syscall(
      SYS_open, reinterpret_cast<long>(DYLD_SHARED_CACHE_PATH), O_RDONLY, 0));
  if (fd < 0)
    die();

  // Read the cache header.
  dyld_cache_header header;
  if (!pread_full(fd, &header, sizeof(header), 0))
    die();

  // Validate magic.
  if (!str_eq(header.magic, "dyld_v1  arm64e"))
    die();

  // Validate mapping count.
  uint32_t mapping_count = header.mappingWithSlideCount;
  if (mapping_count == 0 || mapping_count > MAX_CACHE_MAPPINGS)
    die();

  // Read the mapping-with-slide table.
  dyld_cache_mapping_and_slide_info
      cache_mappings[MAX_CACHE_MAPPINGS];
  if (!pread_full(fd, cache_mappings,
                  mapping_count * sizeof(dyld_cache_mapping_and_slide_info),
                  header.mappingWithSlideOffset))
    die();

  // Compute total slide info size and allocate a scratch buffer.
  uint64_t total_slide_size = 0;
  for (uint32_t i = 0; i < mapping_count; ++i)
    total_slide_size += cache_mappings[i].slideInfoFileSize;

  uint8_t *slide_buf = nullptr;
  if (total_slide_size > 0) {
    long buf = checked_syscall(SYS_mmap, 0, static_cast<long>(total_slide_size),
                               PROT_READ | PROT_WRITE,
                               MAP_ANON | MAP_PRIVATE, -1, 0);
    if (buf < 0)
      die();
    slide_buf = reinterpret_cast<uint8_t *>(buf);

    // Read each mapping's slide info into the buffer.
    uint64_t slide_offset = 0;
    for (uint32_t i = 0; i < mapping_count; ++i) {
      auto &m = cache_mappings[i];
      if (m.slideInfoFileSize > 0) {
        if (!pread_full(fd, slide_buf + slide_offset, m.slideInfoFileSize,
                        m.slideInfoFileOffset))
          die();
        slide_offset += m.slideInfoFileSize;
      }
    }
  }

  // Build the syscall argument arrays.
  shared_file_np files[1];
  files[0].sf_fd = fd;
  files[0].sf_mappings_count = mapping_count;
  files[0].sf_slide = 0;

  shared_file_mapping_slide_np mappings[MAX_CACHE_MAPPINGS];
  uint64_t slide_cursor = 0;
  for (uint32_t i = 0; i < mapping_count; ++i) {
    auto &cm = cache_mappings[i];
    auto &sm = mappings[i];

    sm.sms_address = cm.address;
    sm.sms_size = cm.size;
    sm.sms_file_offset = cm.fileOffset;

    if (cm.slideInfoFileSize > 0 && slide_buf) {
      sm.sms_slide_start =
          reinterpret_cast<uint64_t>(slide_buf + slide_cursor);
      sm.sms_slide_size = cm.slideInfoFileSize;
      slide_cursor += cm.slideInfoFileSize;
    } else {
      sm.sms_slide_start = 0;
      sm.sms_slide_size = 0;
    }

    // Build protection flags: start with the cache file's prot values.
    uint32_t max_prot = cm.maxProt;
    // If slide info is present, add VM_PROT_SLIDE.
    if (cm.slideInfoFileSize > 0)
      max_prot |= VM_PROT_SLIDE;
    // If this is not auth data, add VM_PROT_NOAUTH.
    if (!(cm.flags & DYLD_CACHE_MAPPING_AUTH_DATA))
      max_prot |= VM_PROT_NOAUTH;

    sm.sms_max_prot = max_prot;
    sm.sms_init_prot = cm.initProt;
  }

  // Map the shared cache into the shared region.
  long ret = checked_syscall(
      SYS_shared_region_map_and_slide_2_np, 1,
      reinterpret_cast<long>(files), static_cast<long>(mapping_count),
      reinterpret_cast<long>(mappings));

  // A return of 0 means success. Non-zero is an error but we continue
  // anyway as the shared region may already be mapped by a prior process.
  (void)ret;

  checked_syscall(SYS_close, fd);
}

} // namespace

// C-linkage entry for the standalone (kernel-direct) path.
// start_standalone.S parses argc/argv/envp/apple from the kernel stack and
// tail-calls here.  We own shared-cache setup before handing off to main.
extern "C" [[noreturn]] void libc_standalone_start(int argc, char **argv,
                                                    char **envp, char **apple) {
  map_shared_cache();
  do_start(argc, argv, envp, apple);
}

[[noreturn]] void do_start(int argc, char **argv, char **envp, char **apple) {
  // When linked with -lSystem, dyld calls _start with argc/argv/envp/apple
  // in registers x0-x3 and has already mapped the shared cache.
  app.env_ptr = reinterpret_cast<uintptr_t *>(envp);
  app.apple_ptr = reinterpret_cast<uintptr_t *>(apple);
  environ = envp;

  int retval = main(argc, argv, envp);
  for (;;)
    checked_syscall(SYS_exit, retval);
}

} // namespace LIBC_NAMESPACE_DECL
