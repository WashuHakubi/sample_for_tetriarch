/*
 * Copyright (c) 2026 Sean Kent. All rights reserved.
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#pragma once

#include "i_asset_provider.h"
#include "loaders/i_file_provider.h"

namespace ew {
// Creates an asset provider and registers all known loaders.
IAssetProviderPtr createAssetProvider(IFileProviderPtr fileProvider);
} // namespace ew