/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#pragma once

#include <RmlUi_Platform_SDL.h>
#include <RmlUi_Renderer_SDL.h>
#include <SDL3/SDL.h>

#include <RmlUi/Core.h>

namespace ewok {
    class LocalSystemInterface : public SystemInterface_SDL {
    public:
        bool LogMessage(Rml::Log::Type type, Rml::String const &message) override;
    };

    class RmlContext {
    public:
        explicit RmlContext(SDL_Window *mainWindow, SDL_Renderer *renderer, std::pair<int, int> size);

        ~RmlContext();

        void loadDocument(std::string const &filename);

        Rml::DataModelHandle
        bind(std::string const &name, const std::function<void(Rml::DataModelConstructor &constructor)> &fn);

        void render();

        bool processEvent(SDL_Event &ev) const;

    private:
        bool
        onKeyDown(Rml::Input::KeyIdentifier key, int keyModifier, float nativeDisplayScale, bool highPriority) const;

    private:
        SDL_Window *window_{nullptr};
        LocalSystemInterface rmlSystemInterface_;
        RenderInterface_SDL rmlRenderInterface_;
        std::unique_ptr<Rml::FileInterface> fileInterface_;
        Rml::Context *context_;
        Rml::DataModelHandle dataModel_;
    };
} // namespace ewok
