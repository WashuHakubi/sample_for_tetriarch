/*
 * Copyright (c) 2026 Sean Kent. All rights reserved.
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include "shader_program_loader.h"

#include <nlohmann/json.hpp>

auto ShaderProgramLoader::load(ew::IAssetProviderPtr const& provider, const std::string& fn, std::vector<uint8_t> data)
    -> ew::IAssetPtr {
  nlohmann::json j = nlohmann::json::parse(data);
  auto vsFileName = j["vs"].get<std::string>() + ".bin";
  auto fsFileName = j["fs"].get<std::string>() + ".bin";

  std::string pathPrefix;
  auto renderType = bgfx::getRendererType();
  switch (renderType) {
    case bgfx::RendererType::Direct3D11:
      pathPrefix += "shaders/dx11/";
      break;
    case bgfx::RendererType::Direct3D12:
      pathPrefix += "shaders/dx12/";
      break;
    case bgfx::RendererType::Metal:
      pathPrefix += "shaders/metal/";
      break;
    case bgfx::RendererType::OpenGLES:
    case bgfx::RendererType::OpenGL:
      pathPrefix += "shaders/glsl/";
      break;
    case bgfx::RendererType::Vulkan:
      pathPrefix += "shaders/spirv/";
      break;
    default:
      throw std::runtime_error{"Unsupported renderer type"};
  }

  auto vsData = provider->loadRawAsset(pathPrefix + vsFileName);
  auto fsData = provider->loadRawAsset(pathPrefix + fsFileName);

  auto vs = bgfx::createShader(bgfx::copy(vsData.data(), vsData.size()));
  bgfx::setName(vs, vsFileName.c_str());

  auto fs = bgfx::createShader(bgfx::copy(fsData.data(), fsData.size()));
  bgfx::setName(fs, fsFileName.c_str());

  auto program = bgfx::createProgram(vs, fs, true);
  return std::make_shared<ShaderProgramAsset>(program);
}

auto ShaderProgramLoader::loadAsync(
    ew::IAssetProviderPtr const& provider,
    const std::string& fn,
    std::vector<uint8_t> data) -> coro::task<ew::IAssetPtr> {
  nlohmann::json j = nlohmann::json::parse(data);
  auto vsFileName = j["vs"].get<std::string>() + ".bin";
  auto fsFileName = j["fs"].get<std::string>() + ".bin";

  std::string pathPrefix;
  auto renderType = bgfx::getRendererType();
  switch (renderType) {
    case bgfx::RendererType::Direct3D11:
      pathPrefix += "shaders/dx11/";
      break;
    case bgfx::RendererType::Direct3D12:
      pathPrefix += "shaders/dx12/";
      break;
    case bgfx::RendererType::Metal:
      pathPrefix += "shaders/metal/";
      break;
    case bgfx::RendererType::OpenGLES:
    case bgfx::RendererType::OpenGL:
      pathPrefix += "shaders/glsl/";
      break;
    case bgfx::RendererType::Vulkan:
      pathPrefix += "shaders/spirv/";
      break;
    default:
      throw std::runtime_error{"Unsupported renderer type"};
  }

  auto vsDataResult = co_await provider->loadRawAssetAsync(pathPrefix + vsFileName);
  auto fsDataResult = co_await provider->loadRawAssetAsync(pathPrefix + fsFileName);

  if (!vsDataResult.has_value() || !fsDataResult.has_value()) {
    co_return nullptr;
  }

  auto vsData = std::move(vsDataResult.value());
  auto fsData = std::move(fsDataResult.value());

  auto vs = bgfx::createShader(bgfx::copy(vsData.data(), vsData.size()));
  bgfx::setName(vs, vsFileName.c_str());

  auto fs = bgfx::createShader(bgfx::copy(fsData.data(), fsData.size()));
  bgfx::setName(fs, fsFileName.c_str());

  auto program = bgfx::createProgram(vs, fs, true);
  co_return std::make_shared<ShaderProgramAsset>(program);
}
