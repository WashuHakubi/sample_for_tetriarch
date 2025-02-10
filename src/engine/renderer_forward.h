/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */
#pragma once

namespace ewok {
class Material;
class Mesh;
class Renderer;

struct IRenderable;

using MaterialPtr = std::shared_ptr<Material>;
using RendererPtr = std::shared_ptr<Renderer>;

template <typename Tag>
struct Handle {
  void* data{nullptr};
};

namespace detail {
struct BufferTag;
struct ShaderTag;
} // namespace detail

using BufferHandle = Handle<detail::BufferTag>;
using ShaderHandle = Handle<detail::ShaderTag>;
} // namespace ewok
