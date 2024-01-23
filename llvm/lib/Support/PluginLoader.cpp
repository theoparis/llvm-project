//===-- PluginLoader.cpp - Implement -load command line option ------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file implements the -load <plugin> command line option handler.
//
//===----------------------------------------------------------------------===//

#define DONT_GET_PLUGIN_LOADER_OPTION
#include "llvm/Support/PluginLoader.h"
#include "llvm/Support/DynamicLibrary.h"
#ifndef _REENTRANT
#include "llvm/Support/Mutex.h"
#endif
#include "llvm/Support/raw_ostream.h"
#include <vector>
using namespace llvm;

namespace {

struct Plugins {
#ifndef _REENTRANT
  sys::SmartMutex<true> Lock;
#endif
  std::vector<std::string> List;
};

Plugins &getPlugins() {
  static Plugins P;
  return P;
}

} // anonymous namespace

void PluginLoader::operator=(const std::string &Filename) {
  auto &P = getPlugins();
#ifndef _REENTRANT
  sys::SmartScopedLock<true> Lock(P.Lock);
#endif
  std::string Error;
  if (sys::DynamicLibrary::LoadLibraryPermanently(Filename.c_str(), &Error)) {
    errs() << "Error opening '" << Filename << "': " << Error
           << "\n  -load request ignored.\n";
  } else {
    P.List.push_back(Filename);
  }
}

unsigned PluginLoader::getNumPlugins() {
  auto &P = getPlugins();
#ifndef _REENTRANT
  sys::SmartScopedLock<true> Lock(P.Lock);
#endif
  return P.List.size();
}

std::string &PluginLoader::getPlugin(unsigned num) {
  auto &P = getPlugins();
#ifndef _REENTRANT
  sys::SmartScopedLock<true> Lock(P.Lock);
#endif
  assert(num < P.List.size() && "Asking for an out of bounds plugin");
  return P.List[num];
}
