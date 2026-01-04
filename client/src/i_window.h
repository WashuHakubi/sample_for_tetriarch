/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#pragma once
#include <memory>
#include <string_view>
#include <utility>

#include <glm/glm.hpp>

#include <shared/enum_flags.h>

namespace ew {
enum class WindowFlags {
  FullScreen = 0x01,
  Resizable = 0x02,
};
template <>
struct is_enum_flags_type<WindowFlags> : std::true_type {};

struct IWindow {
  virtual ~IWindow() = default;

  virtual void captureMouse(bool capture) = 0;

  /// @brief Gets the video driver name, native device descriptor (or null if none), and window handle.
  virtual auto getWindowDescriptors() const -> std::tuple<std::string_view, void*, void*> = 0;

  virtual auto getWindowSize() const -> std::pair<int, int> = 0;

  virtual void setWindowSize(int w, int h) = 0;

  virtual void setFullscreen(bool fs) = 0;
};
using WindowPtr = std::unique_ptr<IWindow>;

WindowPtr createWindow(std::string const& name, int width, int height, WindowFlags flags);
} // namespace ew
