/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */
#pragma once

#include <expected>
#include <format>
#include <iostream>
#include <memory>
#include <span>
#include <string>
#include <system_error>
#include <type_traits>
#include <typeindex>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>

#include <concurrencpp/concurrencpp.h>
#include <crossguid/guid.hpp>
namespace ewok {
using xg::Guid;

class AssetDatabase;
class ComponentBase;
class GameObject;
class ObjectDatabase;

template <class TDerived>
class Component;

class Renderer;

struct IAsset;
struct IAssetLoader;
struct IComponentParser;
struct IFileProvider;
struct IRenderable;

template <typename T>
concept IsAssetType = std::is_base_of_v<IAsset, T> && std::is_final_v<T>;

template <IsAssetType T>
struct ITypedAssetLoader;

struct ComponentEditor;

using ComponentPtr = std::shared_ptr<ComponentBase>;
using GameObjectPtr = std::shared_ptr<GameObject>;
using GameObjectHandle = std::weak_ptr<GameObject>;

using IAssetPtr = std::shared_ptr<IAsset>;
using IAssetLoaderPtr = std::unique_ptr<IAssetLoader>;
using IComponentParserPtr = std::shared_ptr<IComponentParser>;
using IFileProviderPtr = std::shared_ptr<IFileProvider>;

std::shared_ptr<AssetDatabase> const& assetDatabase();
std::shared_ptr<concurrencpp::executor> const& globalExecutor();
std::shared_ptr<ObjectDatabase> const& objectDatabase();

void setAssetDatabase(std::shared_ptr<AssetDatabase> db);
void setGlobalExecutor(std::shared_ptr<concurrencpp::executor> executor);
void setObjectDatabase(std::shared_ptr<ObjectDatabase> db);
} // namespace ewok
