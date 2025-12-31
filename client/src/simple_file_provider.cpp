/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include "simple_file_provider.h"

#include <cstdio>

#include <ng-log/logging.h>

SimpleFileProvider::SimpleFileProvider(std::string basePath) : basePath_(std::move(basePath)) {
  if (basePath_.back() != '/') {
    basePath_ += '/';
  }
}

auto SimpleFileProvider::load(std::string const& fn) -> std::string {
  auto path = basePath_ + fn;

  auto file = fopen(path.c_str(), "rb");
  if (!file) {
    LOG(FATAL) << "Failed to open file: " << path;
  }

  fseek(file, 0, SEEK_END);
  auto size = ftell(file);
  fseek(file, 0, SEEK_SET);
  auto buffer = std::string(size, '\0');
  fread(buffer.data(), 1, size, file);
  fclose(file);

  return buffer;
}
