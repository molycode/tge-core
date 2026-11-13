#pragma once

#include <tge/assets/asset_guid.hpp>
#include <tge/assets/image.hpp>
#include <tge/model/model_data.hpp>
#include <tge/non_copyable.hpp>
#include <expected>
#include <memory>
#include <string>
#include <string_view>

namespace Tge::Assets
{
// Callable from any thread: a scene loads models from its spawn jobs.
class IAssetProvider : public SNoCopyNoMove
{
public:

	// Shared while any copy lives, then evicted. Non-const because a renderer moves the pixels out during upload.
	virtual std::shared_ptr<Model::SModelData> LoadModel(std::string_view filePath) = 0;

	// Writes the asset's sidecar .meta the first time it is asked.
	virtual SAssetGuid GetAssetGuid(std::string_view filePath) = 0;

	// Empty when the GUID names no known asset.
	virtual std::string ResolveAssetPath(SAssetGuid const& guid) const = 0;

	// Decoded on the calling thread and not cached: an image is read once by whoever uploads it.
	virtual std::expected<SImage, EImageError> LoadImage(std::string_view filePath) = 0;

protected:

	~IAssetProvider() = default;
};

// Null outside the provider's lifetime: a module that calls it declares gModuleId Required.
extern IAssetProvider* gAssetProvider;
} // namespace Tge::Assets
