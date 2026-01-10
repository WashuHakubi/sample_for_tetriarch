/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include "simple_file_provider.h"

#include <cstdio>

#include <ng-log/logging.h>

SimpleFileProvider::SimpleFileProvider(std::shared_ptr<coro::io_scheduler> scheduler, std::string basePath)
    : basePath_(std::move(basePath))
    , scheduler_(std::move(scheduler)) {
  if (basePath_.back() != '/') {
    basePath_ += '/';
  }
}

auto SimpleFileProvider::load(std::string const& fn) -> std::vector<uint8_t> {
  auto path = basePath_ + fn;

  auto file = fopen(path.c_str(), "rb");
  if (!file) {
    LOG(FATAL) << "Failed to open file: " << path;
  }

  fseek(file, 0, SEEK_END);
  auto size = ftell(file);
  fseek(file, 0, SEEK_SET);
  auto buffer = std::vector<uint8_t>();
  buffer.resize(size);
  fread(buffer.data(), 1, size, file);
  fclose(file);

  return buffer;
}

auto SimpleFileProvider::loadAsync(std::string const& fn)
    -> coro::task<std::expected<std::vector<uint8_t>, ew::FileError>> {
  co_await scheduler_->schedule();

  auto path = basePath_ + fn;

  auto file = fopen(path.c_str(), "rb");
  if (!file) {
    co_return std::unexpected{ew::FileError::FileNotFound};
  }

  fseek(file, 0, SEEK_END);
  size_t size = ftell(file);
  fseek(file, 0, SEEK_SET);
  auto buffer = std::vector<uint8_t>();
  buffer.resize(size);
  size_t sizeRead = fread(buffer.data(), 1, size, file);
  fclose(file);

  if (sizeRead != size) {
    co_return std::unexpected{ew::FileError::ReadFailed};
  }

  co_return buffer;
}
