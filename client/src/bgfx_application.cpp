/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include <functional>
#include <iostream>
#include <typeindex>

#include "i_application.h"
#include "i_window.h"

#include "ecs_systems.h"
#include "sim_time.h"

#include <utility>
#include <bgfx/bgfx.h>
#include <bgfx/platform.h>
#include <bx/bx.h>
#include <bx/math.h>
#include <bx/spscqueue.h>
#include <entt/entt.hpp>
#include <glm/ext.hpp>
#include <ng-log/logging.h>
#include <nlohmann/json.hpp>

struct IFileProvider {
  virtual ~IFileProvider() = default;

  [[nodiscard]] virtual auto load(std::string const& fn) -> std::string = 0;
};
using IFileProviderPtr = std::shared_ptr<IFileProvider>;

struct SimpleFileProvider : IFileProvider {
  explicit SimpleFileProvider(std::string basePath);

  [[nodiscard]] auto load(std::string const& fn) -> std::string override;

 private:
  std::string basePath_;
};

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

struct AssetProvider;
using AssetProviderPtr = std::shared_ptr<AssetProvider>;

struct IAsset {
  virtual ~IAsset() = default;
};
using IAssetPtr = std::shared_ptr<IAsset>;
using IWeakAssetPtr = std::weak_ptr<IAsset>;

template <class T>
struct Asset : IAsset {
  using Ptr = std::shared_ptr<T>;

  Asset() { static_assert(std::is_base_of_v<Asset, T>); }
};

struct IAssetLoader {
  virtual ~IAssetLoader() = default;

  [[nodiscard]] virtual auto loadedType() const -> std::type_index = 0;

  [[nodiscard]] virtual auto load(AssetProviderPtr const& provider, const std::string& fn, std::string const& data)
      -> IAssetPtr = 0;
};
using IAssetLoaderPtr = std::shared_ptr<IAssetLoader>;

struct AssetProvider : std::enable_shared_from_this<AssetProvider> {
  explicit AssetProvider(IFileProviderPtr fileProvider) : fileProvider_(std::move(fileProvider)) {}

  void registerAssetLoader(IAssetLoaderPtr loader) { assetLoaders_[loader->loadedType()] = std::move(loader); }

  template <class T>
    requires(std::is_base_of_v<IAsset, T>)
  auto load(std::string const& fn) -> std::shared_ptr<T> {
    auto const asset = load(fn, std::type_index(typeid(T)));
    return std::static_pointer_cast<T>(asset);
  }

  auto load(std::string const& fn, std::type_index typeId) -> IAssetPtr;

  auto loadRawAsset(std::string const& fn) -> std::string { return fileProvider_->load(fn); }

 private:
  std::shared_ptr<IFileProvider> fileProvider_;
  std::unordered_map<std::type_index, IAssetLoaderPtr> assetLoaders_;
  std::unordered_map<std::string, IWeakAssetPtr> assetsCache_;
};

struct ShaderProgram final : Asset<ShaderProgram> {
  ShaderProgram(bgfx::ProgramHandle handle) : programHandle{handle} {}

  ~ShaderProgram() override { bgfx::destroy(programHandle); }

  bgfx::ProgramHandle programHandle{bgfx::kInvalidHandle};
};

struct ShaderProgramLoader final : IAssetLoader {
  [[nodiscard]] auto loadedType() const -> std::type_index override { return typeid(ShaderProgram); }

  [[nodiscard]] auto load(AssetProviderPtr const& provider, const std::string& fn, std::string const& data)
      -> IAssetPtr override {
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
    return std::make_shared<ShaderProgram>(program);
  }
};

auto AssetProvider::load(std::string const& fn, std::type_index typeId) -> IAssetPtr {
  if (auto const it = assetsCache_.find(fn); it != assetsCache_.end()) {
    if (auto asset = it->second.lock()) {
      return asset;
    }

    // Cleanup dead entry in the asset cache
    assetsCache_.erase(it);
  }

  auto const data = fileProvider_->load(fn);
  auto const asset = assetLoaders_.at(typeId)->load(shared_from_this(), fn, data);
  assetsCache_.emplace(fn, asset);
  return asset;
}

struct SimpleFrameRateSystem {
  void render(float dt) {
    accum_ += dt;
    ++count_;

    if (accum_ >= 0.5f) {
      // recomputes the frame rate every half-second
      rate_ = count_ / accum_;
      count_ = 0;
      accum_ = 0;
    }

    bgfx::dbgTextPrintf(0, 1, 0x0b, "Frame rate: %.1f", rate_);
  }

  float rate_{0};
  float count_{0};
  float accum_{0};
};

struct SetFullScreenMsg {
  bool fullscreen;
};

using MainMsg = std::variant<SetFullScreenMsg>;

struct PosColorVertex {
  float x, y, z;
  uint32_t color;

  static auto& layout() {
    static bool s_init{false};
    static bgfx::VertexLayout s_layout;

    if (!s_init) {
      s_layout.begin()
          .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
          .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
          .end();
      s_init = true;
    }

    return s_layout;
  }
};

struct DebugCubesRenderingSystem {
  DebugCubesRenderingSystem(AssetProviderPtr provider) : assetProvider_(std::move(provider)) {}

  ~DebugCubesRenderingSystem() {
    if (bgfx::isValid(vbh_)) {
      bgfx::destroy(vbh_);
    }
    if (bgfx::isValid(ibh_)) {
      bgfx::destroy(ibh_);
    }
  }

  void render(float dt) {
    static constexpr PosColorVertex cubeVertices[] = {
        {-1.0f, 1.0f, 1.0f, 0xff000000},
        {1.0f, 1.0f, 1.0f, 0xff0000ff},
        {-1.0f, -1.0f, 1.0f, 0xff00ff00},
        {1.0f, -1.0f, 1.0f, 0xff00ffff},
        {-1.0f, 1.0f, -1.0f, 0xffff0000},
        {1.0f, 1.0f, -1.0f, 0xffff00ff},
        {-1.0f, -1.0f, -1.0f, 0xffffff00},
        {1.0f, -1.0f, -1.0f, 0xffffffff},
    };

    static constexpr uint16_t cubeTriList[] = {
        0, 1, 2, // 0
        1, 3, 2, //
        4, 6, 5, // 2
        5, 6, 7, //
        0, 2, 4, // 4
        4, 2, 6, //
        1, 5, 3, // 6
        5, 7, 3, //
        0, 4, 1, // 8
        4, 5, 1, //
        2, 3, 6, // 10
        6, 3, 7,
    };

    constexpr uint64_t state = 0 | BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_WRITE_Z |
        BGFX_STATE_DEPTH_TEST_LESS | BGFX_STATE_MSAA | BGFX_STATE_CULL_CW;

    if (!program_) {
      program_ = assetProvider_->load<ShaderProgram>("cube.json");
      return;
    }

    if (!bgfx::isValid(vbh_)) {
      vbh_ = bgfx::createVertexBuffer(
          // Static data can be passed with bgfx::makeRef
          bgfx::makeRef(cubeVertices, sizeof(cubeVertices)),
          PosColorVertex::layout());
    }

    if (!bgfx::isValid(ibh_)) {
      // Create a static index buffer for triangle list rendering.
      ibh_ = bgfx::createIndexBuffer(
          // Static data can be passed with bgfx::makeRef
          bgfx::makeRef(cubeTriList, sizeof(cubeTriList)));
    }

    accum_ += dt;

    // Submit 11x11 cubes.
    for (uint32_t yy = 0; yy < 11; ++yy) {
      for (uint32_t xx = 0; xx < 11; ++xx) {
        float mtx[16];
        bx::mtxRotateXY(mtx, accum_ + xx * 0.21f, accum_ + yy * 0.37f);
        mtx[12] = -15.0f + static_cast<float>(xx) * 3.0f;
        mtx[13] = -15.0f + static_cast<float>(yy) * 3.0f;
        mtx[14] = 0.0f;

        // Set model matrix for rendering.
        bgfx::setTransform(mtx);

        // Set vertex and index buffer.
        bgfx::setVertexBuffer(0, vbh_);
        bgfx::setIndexBuffer(ibh_);

        // Set render states.
        bgfx::setState(state);

        // Submit primitive for rendering to view 0.
        bgfx::submit(0, program_->programHandle);
      }
    }
  }

  float accum_{0};
  bgfx::VertexBufferHandle vbh_{bgfx::kInvalidHandle};
  bgfx::IndexBufferHandle ibh_{bgfx::kInvalidHandle};
  ShaderProgram::Ptr program_;
  AssetProviderPtr assetProvider_;
};

struct CameraSystem {
  CameraSystem() { homogeneousDepth_ = bgfx::getCaps()->homogeneousDepth; }

  void render(float dt) {
    angle_ += rotate_ * dt;
    zoomed_ = std::max(std::min(zoomed_ + zoom_ * zoomScale_ * dt, 20.0f), -20.0f);

    constexpr bx::Vec3 at = {0.0f, 0.0f, 0.0f};
    bx::Vec3 eye = {0.0f, 0.0f, -35.0f + zoomed_};

    // Real simple orbit camera around the origin.
    // Rotate `eye` around `at`.
    eye = rotateAround(eye, at, bx::Vec3{0.0f, 1.0f, 0.0f}, angle_);
    float view[16];

    // Update our view to look at `at`
    bx::mtxLookAt(view, eye, at);

    float proj[16];
    bx::mtxProj(proj, 60.0f, aspectRatio_, 0.1f, 100.0f, homogeneousDepth_);
    bgfx::setViewTransform(0, view, proj);
  }

  bx::Vec3 rotateAround(bx::Vec3 const& pos, bx::Vec3 const& center, bx::Vec3 const& axis, float angle) {
    auto rot = bx::fromAxisAngle(axis, angle);
    auto dir = bx::sub(pos, center);
    dir = bx::mul(dir, rot);

    return bx::add(center, dir);
  }

  void handleMessage(ew::Msg const& msg) {
    if (auto resize = std::get_if<ew::ResizeMsg>(&msg)) {
      width_ = resize->width;
      height_ = resize->height;
      aspectRatio_ = width_ / static_cast<float>(height_);
    }

    if (auto key = std::get_if<ew::KeyMsg>(&msg)) {
      if (key->scancode == ew::Scancode::SCANCODE_Q || key->scancode == ew::Scancode::SCANCODE_E) {
        rotate_ = key->down ? key->scancode == ew::Scancode::SCANCODE_Q ? 1.0f : -1.0f : 0.0f;
      }
    }

    if (auto wheel = std::get_if<ew::MouseWheelMsg>(&msg)) {
      zoom_ = wheel->delta;
    }
  }

  int width_{0};
  int height_{0};
  float aspectRatio_;
  float rotate_;
  float angle_{0.0f};
  float zoom_{0.0f};
  float zoomScale_{1.5f};
  float zoomed_{0.0f};
  bool homogeneousDepth_{false};
};

struct BgfxApplication : ew::IApplication {
  const unsigned kDefaultClearColor = 0x303030ff;
  const unsigned kAltClearColor = 0x334433ff;

  BgfxApplication();

  void handle(ew::Msg const& msg) override;

  bool init(int argc, char** argv) override;

  bool update() override;

 private:
  void processMessages();

  void run(std::pair<void*, void*> descriptors);

  ew::WindowPtr window_;
  std::thread gameThread_;
  bx::DefaultAllocator alloc_;
  bx::SpScBlockingUnboundedQueueT<ew::Msg> gameMsgs_;
  bx::SpScBlockingUnboundedQueueT<MainMsg> mainMsgs_;
  ew::EcsSystems systems_;
  std::atomic_bool exit_{false};
  bool fullscreen_{false};
  std::pair<int, int> windowSize_{800, 600};
};

BgfxApplication::BgfxApplication() : gameMsgs_(&alloc_), mainMsgs_(&alloc_) {}

void BgfxApplication::handle(ew::Msg const& msg) {
  // Forward message to the game thread
  gameMsgs_.push(new ew::Msg(msg));
}

bool BgfxApplication::init(int argc, char** argv) {
  // Window is created on the main thread, which is also where rendering will happen
  window_ = ew::createWindow("game", windowSize_.first, windowSize_.second, ew::WindowFlags::Resizable);
  if (!window_) {
    LOG(FATAL) << "Failed to create window";
  }

  // Calling this before bgfx::init() ensures we don't create a rendering thread.
  bgfx::renderFrame();

  // These cannot be safely accessed on the game thread, so capture them here.
  auto descriptors = window_->getWindowDescriptors();

  // Send a message to the game thread containing the window dimensions.
  windowSize_ = window_->getWindowSize();
  auto [width, height] = windowSize_;
  handle(ew::ResizeMsg{width, height});

  // Create the game thread.
  gameThread_ = std::thread([this, descriptors]() { this->run(descriptors); });

  return true;
}

bool BgfxApplication::update() {
  // Render all the queued commands
  bgfx::renderFrame();

  // Process any requests from the game thread.
  while (auto const ptr = mainMsgs_.pop(0)) {
    std::unique_ptr<MainMsg> msg(ptr);

    if (auto const fs = std::get_if<SetFullScreenMsg>(msg.get())) {
      window_->setFullscreen(fs->fullscreen);
    }
  }

  if (exit_) {
    // Wait for the game thread to finish before we terminate.
    while (bgfx::renderFrame() != bgfx::RenderFrame::NoContext) {
    }
    gameThread_.join();
  }

  return !exit_;
}

void BgfxApplication::processMessages() {
  while (auto const ptr = gameMsgs_.pop(0)) {
    // Ensure the message always gets deleted.
    std::unique_ptr<ew::Msg> msg(ptr);

    systems_.handleMessage(*msg);

    if (auto const resize = std::get_if<ew::ResizeMsg>(msg.get())) {
      windowSize_ = {resize->width, resize->height};
      bgfx::reset(resize->width, resize->height, BGFX_RESET_VSYNC);
      bgfx::setViewRect(0, 0, 0, resize->width, resize->height);
    } else if (auto const key = std::get_if<ew::KeyMsg>(msg.get())) {
      if (key->scancode == ew::Scancode::SCANCODE_LSHIFT) {
        bgfx::setViewClear(
            0,
            BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH,
            key->down ? kAltClearColor : kDefaultClearColor,
            1.0f,
            0);
      }

      if (key->scancode == ew::Scancode::SCANCODE_F1 && key->down) {
        fullscreen_ = !fullscreen_;

        // Request the render thread to switch our full-screen status.
        mainMsgs_.push(new MainMsg{SetFullScreenMsg{fullscreen_}});
      }
    } else if (std::get_if<ew::ShutdownMsg>(msg.get())) {
      exit_ = true;
    }
  }
}

void BgfxApplication::run(std::pair<void*, void*> descriptors) {
  bgfx::Init init;

  auto [ndt, nwh] = descriptors;

  init.platformData.ndt = ndt;
  init.platformData.nwh = nwh;

  // This must be called on the "game" thread.
  bgfx::init(init);

  bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, kDefaultClearColor, 1.0f, 0);

  constexpr auto kSimTickRate = 1.0 / 60.0;
  constexpr auto kMaxSimTime = 5 * kSimTickRate;

  bgfx::setDebug(BGFX_DEBUG_TEXT);

  ew::SimTime time;
  double timeAccumulator{0};
  entt::registry registry;

  auto assetProvider = std::make_shared<AssetProvider>(std::make_shared<SimpleFileProvider>("assets"));
  assetProvider->registerAssetLoader(std::make_shared<ShaderProgramLoader>());

  systems_.addSystem(std::make_shared<SimpleFrameRateSystem>());
  systems_.addSystem(std::make_shared<CameraSystem>());
  systems_.addSystem(std::make_shared<DebugCubesRenderingSystem>(assetProvider));

  // Game loop
  while (!exit_) {
    processMessages();

    time.update();

    // Cap the number of times we'll update the sim per frame to ensure rendering and other events are handled.
    timeAccumulator = std::max(timeAccumulator + time.simDeltaTime(), kMaxSimTime);

    while (timeAccumulator >= kSimTickRate) {
      // update sim
      systems_.update(kSimTickRate);

      // Remove sim tick time from the accumulator
      timeAccumulator -= kSimTickRate;
    }

    // Ensure view 0 is touched with at least a dummy event
    bgfx::touch(0);

    bgfx::dbgTextClear();
    bgfx::dbgTextPrintf(0, 0, 0x0b, "Dimensions: %d x %d", windowSize_.first, windowSize_.second);

    systems_.render(static_cast<float>(time.simDeltaTime()));

    // present the frame, dispatches to the rendering thread.
    bgfx::frame();
  }

  systems_.clear();

  bgfx::shutdown();
}

ew::ApplicationPtr ew::createApplication() {
  return std::make_shared<BgfxApplication>();
}
