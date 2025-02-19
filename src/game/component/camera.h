/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#pragma once

#include "engine/component.h"
#include "engine/game_object.h"
#include "engine/reflection/reflection.h"

namespace ewok {
enum class CameraType {
  FirstPerson,
  ThirdPerson,
  Free,
};

// Fake camera, not a real one, just showing off some component stuff.
class Camera : public Component<Camera> {
 public:
  static auto typeName() -> std::string { return "ewok::Camera"; }

  void attach() override;

  constexpr auto name() -> std::string const& { return name_; }
  void setName(std::string name) { name_ = std::move(name); }

  auto target() const -> GameObjectPtr { return target_.lock(); }
  void setTarget(GameObjectPtr target) { target_ = std::move(target); }

 private:
  EWOK_REFLECTION_DECL

  friend class CameraEditor;
  std::string name_;

  // Weak handle to a game object, this prevents circular references that can
  // cause memory leaks.
  GameObjectHandle target_;

  int32_t s32_;
  float f_;
  uint64_t u64_;

  std::vector<float> floats_;

  std::vector<Vec3> vecs_;

  Transform t_;

  CameraType cameraType_;
};
} // namespace ewok
