/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include "rml_context.h"

#include <cstdio>
#include <filesystem>
#include <utility>

#include <RmlUi/Debugger.h>
#include <ng-log/logging.h>

#include "engine/asset_database.h"
#include "engine/forward.h"

namespace ewok {
bool LocalSystemInterface::LogMessage(Rml::Log::Type type, const Rml::String& message) {
  switch (type) {
    case Rml::Log::LT_ALWAYS:
      LOG(FATAL) << message;
      break;
    case Rml::Log::LT_ERROR:
      LOG(ERROR) << message;
      break;
    case Rml::Log::LT_ASSERT:
      LOG(ERROR) << message;
      break;
    case Rml::Log::LT_WARNING:
      LOG(WARNING) << message;
      break;
    case Rml::Log::LT_INFO:
      LOG(INFO) << message;
      break;
    case Rml::Log::LT_DEBUG:
#if !defined(NDEBUG)
      LOG(INFO) << message;
#endif
      break;
    default:
      abort();
  }
  return true;
}

class AssetFileInterface : public Rml::FileInterface {
 public:
  explicit AssetFileInterface(std::string basePath) {
    basePath_ = std::filesystem::absolute(std::filesystem::path(basePath)).generic_string();
    if (basePath_.ends_with('/')) {
      return;
    }

    basePath_ += '/';
  }

  Rml::FileHandle Open(const Rml::String& path) override {
    if (path.starts_with("/")) {
      return reinterpret_cast<Rml::FileHandle>(fopen(path.c_str(), "rb"));
    }

    auto rootedPath = basePath_ + path;
    return reinterpret_cast<Rml::FileHandle>(fopen(rootedPath.c_str(), "rb"));
  }

  void Close(Rml::FileHandle file) override { fclose(reinterpret_cast<FILE*>(file)); }

  size_t Read(void* buffer, size_t size, Rml::FileHandle file) override {
    return fread(buffer, 1, size, reinterpret_cast<FILE*>(file));
  }

  bool Seek(Rml::FileHandle file, long offset, int origin) override {
    return fseek(reinterpret_cast<FILE*>(file), offset, origin) == 0;
  }

  size_t Tell(Rml::FileHandle file) override { return ftell(reinterpret_cast<FILE*>(file)); }

 private:
  std::string basePath_;
};

RmlContext::RmlContext(SDL_Window* mainWindow, SDL_Renderer* renderer, std::pair<int, int> size)
    : window_{mainWindow},
      rmlRenderInterface_{renderer},
      // This should really use IFileProvider
      fileInterface_(std::make_unique<AssetFileInterface>("assets/")) {
  rmlSystemInterface_.SetWindow(mainWindow);

  Rml::SetFileInterface(fileInterface_.get());
  Rml::SetSystemInterface(&rmlSystemInterface_);
  Rml::SetRenderInterface(&rmlRenderInterface_);

  if (!Rml::Initialise()) {
    LOG(ERROR) << "Failed to initialize RML";
    abort();
  }

  const Rml::String directory = "fonts/";

  struct FontFace {
    const char* filename;
    bool fallbackFace;
  };
  FontFace fontFaces[] = {
      {"LatoLatin-Regular.ttf", false},
      {"LatoLatin-Italic.ttf", false},
      {"LatoLatin-Bold.ttf", false},
      {"LatoLatin-BoldItalic.ttf", false},
      {"NotoEmoji-Regular.ttf", true},
  };

  for (const FontFace& face : fontFaces) {
    Rml::LoadFontFace(directory + face.filename, face.fallbackFace);
  }

  auto [width, height] = size;
  context_ = Rml::CreateContext("main", Rml::Vector2i(width, height));
  if (!context_) {
    LOG(ERROR) << "Failed to create RML context";
    abort();
  }

  Rml::Debugger::Initialise(context_);
}

RmlContext::~RmlContext() {
  Rml::Shutdown();
}

void RmlContext::loadDocument(std::string const& filename) {
  auto doc = context_->LoadDocument(filename);
  doc->Show();
}

void RmlContext::render() {
  context_->Update();

  rmlRenderInterface_.BeginFrame();
  context_->Render();
  rmlRenderInterface_.EndFrame();
}

bool RmlContext::processEvent(SDL_Event& ev) const {
  bool handled = false;
  switch (ev.type) {
    case SDL_EVENT_QUIT: {
      return false;
    }
    case SDL_EVENT_KEY_DOWN: {
      const Rml::Input::KeyIdentifier key = RmlSDL::ConvertKey(ev.key.key);
      const int keyModifier = RmlSDL::GetKeyModifierState();
      const float nativeDisplayScale = SDL_GetWindowDisplayScale(window_);

      if (!onKeyDown(key, keyModifier, nativeDisplayScale, true)) {
        handled = true;
        break;
      }

      if (!RmlSDL::InputEventHandler(context_, window_, ev)) {
        handled = true;
        break;
      }

      if (!onKeyDown(key, keyModifier, nativeDisplayScale, false)) {
        handled = true;
        break;
      }
      break;
    }
    default:
      handled = RmlSDL::InputEventHandler(context_, window_, ev);
      break;
  }

  return handled;
}

bool RmlContext::onKeyDown(
    const Rml::Input::KeyIdentifier key, const int keyModifier, const float nativeDisplayScale, const bool highPriority)
    const {
  bool result = false;
  if (highPriority) {
    if (key == Rml::Input::KI_F12) {
      Rml::Debugger::SetVisible(!Rml::Debugger::IsVisible());
    } else {
      result = true;
    }
  } else {
    if (key == Rml::Input::KI_F5) {
      for (int i = 0; i < context_->GetNumDocuments(); i++) {
        Rml::ElementDocument* document = context_->GetDocument(i);
        if (Rml::String const& src = document->GetSourceURL(); src.size() > 4 && src.substr(src.size() - 4) == ".rml") {
          document->ReloadStyleSheet();
        }
      }
    } else {
      result = true;
    }
  }
  return result;
}
} // namespace ewok