/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include "engine/system_file_provider.h"

#include <filesystem>

namespace ewok {
SystemFileProvider::SystemFileProvider(
    std::shared_ptr<concurrencpp::executor> ioExecutor, std::string path)
    : ioExecutor_(std::move(ioExecutor)) {
  std::filesystem::path p(path);
  path_ =
      std::filesystem::absolute(std::filesystem::path(path)).generic_string();
}

auto SystemFileProvider::readFileAsync(
    std::string const& fn,
    std::shared_ptr<concurrencpp::executor> const& resumeOnExecutor)
    -> concurrencpp::result<std::vector<char>> {
  auto result = co_await ioReadFileAsync(fn);

  // Make sure we always get back to the expected executor
  co_await concurrencpp::resume_on(resumeOnExecutor);

  if (result.has_value()) {
    co_return result.value();
  } else {
    throw std::system_error(result.error());
  }
}

auto SystemFileProvider::ioReadFileAsync(std::string const& fn)
    -> concurrencpp::result<std::expected<std::vector<char>, std::error_code>> {
  co_await concurrencpp::resume_on(ioExecutor_);

  auto path = std::filesystem::path(path_) / fn;

  FILE* f = fopen(path.generic_string().c_str(), "rb");
  if (!f) {
    co_return std::unexpected{
        std::make_error_code(std::errc::no_such_file_or_directory)};
  }

  if (fseek(f, 0, SEEK_END)) {
    fclose(f);
    co_return std::unexpected{
        std::make_error_code(std::errc::bad_file_descriptor)};
  }

  auto len = ftell(f);

  std::vector<char> data;
  data.resize(len + 1);
  data[data.size() - 1] = 0;

  fseek(f, 0, SEEK_SET);
  auto readLen = fread(data.data(), 1, len, f);
  if (readLen != len) {
    fclose(f);
    co_return std::unexpected{
        std::make_error_code(std::errc::bad_file_descriptor)};
  }
  assert(data[data.size() - 1] == 0);

  co_return data;
}
} // namespace ewok
