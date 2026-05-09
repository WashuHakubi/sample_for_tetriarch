/*
 * ------
 * Copyright (c) 2026 Sean Kent
 * All rights reserved.
 */

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <coro/task.hpp>

#include <glm/ext.hpp>
#include <glm/glm.hpp>

#include <nlohmann/json.hpp>

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <entt/entt.hpp>

#include <coreclr_delegates.h>
#include <hostfxr.h>
#include <nethost.h>

#include <cassert>

#ifdef WINDOWS
#include <Windows.h>

#define STR(s) L##s
#define CH(c) L##c
#define DIR_SEPARATOR L'\\'

#define string_compare wcscmp

static void* load_library(const char_t* path) {
  HMODULE h = ::LoadLibraryW(path);
  assert(h != nullptr);
  return (void*)h;
}

static void* get_export(void* h, const char* name) {
  void* f = ::GetProcAddress((HMODULE)h, name);
  assert(f != nullptr);
  return f;
}
#else //! defined(WINDOWS)
#include <dlfcn.h>
#include <limits.h>

#define STR(s) s
#define CH(c) c
#define DIR_SEPARATOR '/'
#define MAX_PATH PATH_MAX

#define string_compare strcmp

static void* load_library(const char_t* path) {
  void* h = dlopen(path, RTLD_LAZY | RTLD_LOCAL);
  assert(h != nullptr);
  return h;
}

static void* get_export(void* h, const char* name) {
  void* f = dlsym(h, name);
  assert(f != nullptr);
  return f;
}
#endif

using string_t = std::basic_string<char_t>;

namespace {
hostfxr_initialize_for_runtime_config_fn init_for_config_fptr;
hostfxr_get_runtime_delegate_fn get_delegate_fptr;
hostfxr_close_fn close_fptr;
load_assembly_and_get_function_pointer_fn load_assembly_and_get_function_pointer;

/**
 * Loads the hostfxr library, used for .net core hosting
 */
void load_hostfxr(char_t const* assembly_path, char_t const* dotnet_root) {
  get_hostfxr_parameters params{sizeof(get_hostfxr_parameters), assembly_path, dotnet_root};

  // Get the path to hostfxr library.
  char_t buffer[MAX_PATH];
  size_t buffer_size = sizeof(buffer) / sizeof(char_t);
  auto rc = get_hostfxr_path(buffer, &buffer_size, &params);
  if (rc) {
    SPDLOG_CRITICAL("Failed while trying to get hostfx path: {}", rc);
    return;
  }

  // Attempt to load the hostfxr library.
  auto lib = load_library(buffer);
  assert(lib);

  // Configure error log printing from hostfxr
  auto set_error_writer = (hostfxr_set_error_writer_fn)get_export(lib, "hostfxr_set_error_writer");
  set_error_writer([](char_t const* message) { spdlog::error(message); });

  // Get function pointers for the methods we need to load our game assembly.
  init_for_config_fptr =
      (hostfxr_initialize_for_runtime_config_fn)get_export(lib, "hostfxr_initialize_for_runtime_config");
  get_delegate_fptr = (hostfxr_get_runtime_delegate_fn)get_export(lib, "hostfxr_get_runtime_delegate");
  close_fptr = (hostfxr_close_fn)get_export(lib, "hostfxr_close");

  assert(init_for_config_fptr && get_delegate_fptr && close_fptr);
}

bool get_dotnet_load_assembly(char_t const* configFilePath) {
  // Start .net core, this uses the runtime config we pass to figure out which .net version to load.
  hostfxr_handle ctx;
  auto rc = init_for_config_fptr(configFilePath, nullptr, &ctx);
  if (rc || ctx == nullptr) {
    SPDLOG_CRITICAL("Failed to initialize .net: {:x}", rc);
    return false;
  }

  // Get a method for loading assemblies and retrieving a function pointer from it.
  rc = get_delegate_fptr(
      ctx,
      hdt_load_assembly_and_get_function_pointer,
      (void**)&load_assembly_and_get_function_pointer);
  if (rc || load_assembly_and_get_function_pointer == nullptr) {
    SPDLOG_CRITICAL("Failed to get hdt_load_assembly_and_get_function_pointer: {:x}", rc);
    close_fptr(ctx);
    return false;
  }

  close_fptr(ctx);
  return true;
}

template <class Fn>
Fn get_managed_function(string_t const& assembly_path, char_t const* type, char_t const* method) {
  Fn ptr;
  load_assembly_and_get_function_pointer(
      assembly_path.c_str(),
      type,
      method,
      UNMANAGEDCALLERSONLY_METHOD,
      nullptr,
      (void**)&ptr);
  assert(ptr);
  return ptr;
}
} // namespace

static constexpr uint64_t ticks_per_update = static_cast<uint64_t>(1 / 60.0 * 1000000000.0);

namespace wutcs {
// Must match SPDLOG levels
enum class log_level : uint32_t {
  trace = 0,
  debug = 1,
  info = 2,
  warn = 3,
  error = 4,
  critical = 5,
};

void log_message(log_level level, char const* msg) {
  spdlog::log((spdlog::level::level_enum)level, msg);
}

entt::entity entt_create_entity(entt::registry* reg) {
  return reg->create();
}

void entt_destroy_entity(entt::registry* reg, entt::entity ent) {
  reg->destroy(ent);
}

struct game_initialize_options {
  uint64_t ticks_per_update{::ticks_per_update};
  entt::registry* registry;

  decltype(wutcs::log_message)* log_message{&wutcs::log_message};

  decltype(entt_create_entity)* create_entity{&entt_create_entity};

  decltype(entt_destroy_entity)* destroy_entity{&entt_destroy_entity};
};

void(CORECLR_DELEGATE_CALLTYPE* gamemanager_on_entity_constructed)(entt::entity);

void on_entity_constructed(entt::registry& r, entt::entity e) {
  gamemanager_on_entity_constructed(e);
}

void(CORECLR_DELEGATE_CALLTYPE* gamemanager_on_entity_destroyed)(entt::entity);

void on_entity_destroyed(entt::registry& r, entt::entity e) {
  gamemanager_on_entity_destroyed(e);
}
} // namespace wutcs

int main(int argc, char** argv) {
  spdlog::log(spdlog::level::info, "");
  SPDLOG_INFO("Starting...");

  char_t host_path[MAX_PATH];
#if WINDOWS
  auto size = ::GetFullPathNameW(argv[0], sizeof(host_path) / sizeof(char_t), host_path, nullptr);
  assert(size != 0);
#else
  auto resolved = realpath(argv[0], host_path);
  assert(resolved != nullptr);
#endif

  string_t root_path = host_path;
  auto pos = root_path.find_last_of(DIR_SEPARATOR);
  assert(pos != string_t::npos);
  root_path = root_path.substr(0, pos + 1);

  load_hostfxr(nullptr, root_path.c_str());
  SPDLOG_INFO("Loaded hostfxr");

  auto const config_path = root_path + STR("net10.0/WutGame.runtimeconfig.json");
  auto const assembly_path = root_path + STR("net10.0/WutGame.dll");

  SPDLOG_INFO("Runtime config: {}", config_path);

  auto fns = get_dotnet_load_assembly(config_path.c_str());
  assert(fns);

  auto gamemanager_initialize =
      get_managed_function<int(CORECLR_DELEGATE_CALLTYPE*)(wutcs::game_initialize_options const*, int32_t)>(
          assembly_path,
          STR("WutGame.GameManager, WutGame"),
          STR("Initialize"));

  auto gamemanager_update = get_managed_function<void(CORECLR_DELEGATE_CALLTYPE*)(uint64_t)>(
      assembly_path,
      STR("WutGame.GameManager, WutGame"),
      STR("Update"));

  auto gamemanager_render = get_managed_function<void(CORECLR_DELEGATE_CALLTYPE*)(uint64_t)>(
      assembly_path,
      STR("WutGame.GameManager, WutGame"),
      STR("Render"));

  auto gamemanager_shutdown = get_managed_function<void(CORECLR_DELEGATE_CALLTYPE*)()>(
      assembly_path,
      STR("WutGame.GameManager, WutGame"),
      STR("Shutdown"));

  wutcs::gamemanager_on_entity_constructed = get_managed_function<void(CORECLR_DELEGATE_CALLTYPE*)(entt::entity)>(
      assembly_path,
      STR("WutGame.GameManager, WutGame"),
      STR("OnEntityConstructed"));

  wutcs::gamemanager_on_entity_destroyed = get_managed_function<void(CORECLR_DELEGATE_CALLTYPE*)(entt::entity)>(
      assembly_path,
      STR("WutGame.GameManager, WutGame"),
      STR("OnEntityDestroyed"));

  SDL_InitSubSystem(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_AUDIO);

  SDL_Window* window;
  SDL_Renderer* renderer;
  if (!SDL_CreateWindowAndRenderer(
          "wut game",
          800,
          600,
          SDL_WINDOW_HIGH_PIXEL_DENSITY | SDL_WINDOW_RESIZABLE,
          &window,
          &renderer)) {
    SPDLOG_CRITICAL("Failed to create window and renderer: {}", SDL_GetError());
    return -1;
  }

  entt::registry registry;

  wutcs::game_initialize_options opts{
      .registry = &registry,
  };

  int rc = gamemanager_initialize(&opts, sizeof(opts));
  if (rc) {
    SPDLOG_CRITICAL("GameManager::Initialize failed.");
    return -1;
  }

  registry.on_construct<entt::entity>().connect<&wutcs::on_entity_constructed>();
  registry.on_destroy<entt::entity>().connect<&wutcs::on_entity_destroyed>();

  auto e = registry.create();
  registry.destroy(e);

  auto last_time = SDL_GetTicksNS();
  auto delta_time = 0ull;

  bool run = true;
  while (run) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      switch (event.type) {
        case SDL_EVENT_QUIT:
          run = false;
          break;
        case SDL_EVENT_WINDOW_RESIZED:
          break;
      }
    }

    auto cur_time = SDL_GetTicksNS();
    auto cur_delta_time = cur_time - last_time;
    delta_time += cur_delta_time;

    while (delta_time > ticks_per_update) {
      delta_time -= ticks_per_update;

      // Game Update
      gamemanager_update(ticks_per_update);
    }

    SDL_SetRenderDrawColor(renderer, 0x80, 0x80, 0x80, 0xFF);
    SDL_RenderClear(renderer);

    // Render
    gamemanager_render(cur_delta_time);

    SDL_RenderPresent(renderer);
  }

  gamemanager_shutdown();

  SDL_DestroyWindow(window);
  SDL_Quit();
}
