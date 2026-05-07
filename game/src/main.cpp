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

#else
#include <dlfcn.h>
#include <limits.h>

#define STR(s) s
#define CH(c) c
#define DIR_SEPARATOR '/'
#define MAX_PATH PATH_MAX

#define string_compare strcmp

#endif

using string_t = std::basic_string<char_t>;

namespace {
hostfxr_initialize_for_runtime_config_fn init_for_config_fptr;
hostfxr_get_runtime_delegate_fn get_delegate_fptr;
hostfxr_close_fn close_fptr;

#ifdef WINDOWS
void* load_library(const char_t* path) {
  HMODULE h = ::LoadLibraryW(path);
  assert(h != nullptr);
  return (void*)h;
}

void* get_export(void* h, const char* name) {
  void* f = ::GetProcAddress((HMODULE)h, name);
  assert(f != nullptr);
  return f;
}
#else
void* load_library(const char_t* path) {
  void* h = dlopen(path, RTLD_LAZY | RTLD_LOCAL);
  assert(h != nullptr);
  return h;
}

void* get_export(void* h, const char* name) {
  void* f = dlsym(h, name);
  assert(f != nullptr);
  return f;
}
#endif

void load_hostfxr(string_t const& assembly_path) {
  get_hostfxr_parameters params{
      sizeof(get_hostfxr_parameters),
      assembly_path.empty() ? nullptr : assembly_path.c_str(),
      nullptr};

  char_t buffer[1024];
  size_t buffer_size = sizeof(buffer) / sizeof(char_t);
  auto rc = get_hostfxr_path(buffer, &buffer_size, &params);
  if (rc) {
    SPDLOG_CRITICAL("Failed while trying to get hostfx path: {}", rc);
  }

  auto lib = load_library(buffer);
  assert(lib);
  init_for_config_fptr =
      (hostfxr_initialize_for_runtime_config_fn)get_export(lib, "hostfxr_initialize_for_runtime_config");
  get_delegate_fptr = (hostfxr_get_runtime_delegate_fn)get_export(lib, "hostfxr_get_runtime_delegate");
  close_fptr = (hostfxr_close_fn)get_export(lib, "hostfxr_close");

  assert(init_for_config_fptr && get_delegate_fptr && close_fptr);
}

load_assembly_and_get_function_pointer_fn get_dotnet_load_assembly(char_t const* configFilePath) {
  hostfxr_handle ctx;
  auto rc = init_for_config_fptr(configFilePath, nullptr, &ctx);
  if (rc || ctx == nullptr) {
    SPDLOG_CRITICAL("Failed to initialize .net: {:x}", rc);
    return nullptr;
  }

  void* load_assembly_and_get_function_pointer;

  rc = get_delegate_fptr(ctx, hdt_load_assembly_and_get_function_pointer, &load_assembly_and_get_function_pointer);
  if (rc || load_assembly_and_get_function_pointer == nullptr) {
    SPDLOG_CRITICAL("Failed to get function to load assembly: {:x}", rc);
    close_fptr(ctx);
    return nullptr;
  }

  close_fptr(ctx);
  return (load_assembly_and_get_function_pointer_fn)load_assembly_and_get_function_pointer;
}
} // namespace

int main(int argc, char** argv) {
  spdlog::info("Starting...");

  load_hostfxr("");
  SPDLOG_INFO("Loaded hostfxr");
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

  auto const config_path = root_path + STR("net10.0/WutGame.runtimeconfig.json");
  auto const assembly_path = root_path + STR("net10.0/WutGame.dll");

  SPDLOG_INFO("Runtime config: {}", config_path);

  auto load_assembly_and_get_function_pointer = get_dotnet_load_assembly(config_path.c_str());
  assert(load_assembly_and_get_function_pointer);

  typedef void (*gamemanager_initialize_fptr)();
  gamemanager_initialize_fptr gamemanager_initialize;

  load_assembly_and_get_function_pointer(
      assembly_path.c_str(),
      STR("WutGame.GameManager, WutGame"),
      STR("Initialize"),
      UNMANAGEDCALLERSONLY_METHOD,
      nullptr,
      (void**)&gamemanager_initialize);
  assert(gamemanager_initialize);

  gamemanager_initialize();
}
