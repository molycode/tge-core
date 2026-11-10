#pragma once

#include <tge/math/types.hpp>
#include <tge/math/constants.hpp>
#include <tge/math/intersection.hpp>
#include <tge/index.hpp>
#include <tge/material/material_properties.hpp>
#include <tge/material/texture_transform.hpp>
#include <tge/model/animation_data.hpp>
#include <tge/geometry/vertex.hpp>
#include <tge/geometry/primitive_topology.hpp>
#include <tge/light/light.hpp>
#include <vector>
#include <string>
#include <cstddef>
#include <cstdint>

namespace Tge::Model
{

enum class ETextureUsage
{
	Color,  // sRGB format: baseColor, emissive (color/light data)
	Data    // Linear format: normal, metallicRoughness, occlusion (physical/geometric data)
};

enum class ETextureCompression
{
	None,  // Uncompressed RGBA8
	BC7    // BC7 block-compressed
};

enum class ETextureFilter
{
	Nearest,
	Linear,
	NearestMipmapNearest,
	LinearMipmapNearest,
	NearestMipmapLinear,
	LinearMipmapLinear
};

enum class ETextureWrap
{
	Repeat,
	ClampToEdge,
	MirroredRepeat
};

struct SSamplerSettings final
{
	ETextureFilter minFilter{ ETextureFilter::Linear };
	ETextureFilter magFilter{ ETextureFilter::Linear };
	ETextureWrap wrapS{ ETextureWrap::Repeat };
	ETextureWrap wrapT{ ETextureWrap::Repeat };

	bool operator==(SSamplerSettings const&) const = default;
};

struct STextureData final
{
	// mipLevels[0] = mip firstResidentMip; mipLevels[1] = mip firstResidentMip+1; etc.
	// PNG/JPG: firstResidentMip=0, mipLevels[0] is the full-resolution image.
	// KTX2:    firstResidentMip>=0, only the low-res mip tail is loaded initially.
	std::vector<std::vector<unsigned char>> mipLevels;
	uint32_t firstResidentMip{ 0 };  // Mip number that mipLevels[0] corresponds to
	uint32_t totalMipCount{ 0 };     // Full mip chain length (including not-yet-loaded mips)
	uint32_t width{ 0 };             // Full-resolution (mip 0) width
	uint32_t height{ 0 };            // Full-resolution (mip 0) height
	uint32_t channels{ 0 };
	std::string sourcePath;
	ETextureUsage usage{ ETextureUsage::Color };
	// Compression is the only stored pixel-format axis; color space (sRGB vs linear)
	// is derived from usage at upload time so the two can never desync.
	ETextureCompression compression{ ETextureCompression::None };
	SSamplerSettings samplerSettings;
	// Raw compressed KTX2 bytes retained for on-demand high-res mip loading.
	// Non-empty only when firstResidentMip > 0 (mip streaming is active for this texture).
	// Required for embedded textures (GLB) that cannot be re-read from a path.
	std::vector<uint8_t> ktx2RawBytes;
};

struct SMaterialData final
{
	// All material property values (single source of truth)
	Material::SMaterialProperties properties;

	// Unconditional by design: gating this by build flag gave a public struct two layouts.
	std::string materialName;

	// Texture indices (InvalidIndex = not present)
	uint32_t baseColorTextureIndex{ InvalidIndex };
	uint32_t metallicRoughnessTextureIndex{ InvalidIndex };
	uint32_t normalTextureIndex{ InvalidIndex };
	uint32_t emissiveTextureIndex{ InvalidIndex };
	uint32_t occlusionTextureIndex{ InvalidIndex };
	uint32_t specularTextureIndex{ InvalidIndex };           // KHR_materials_specular
	uint32_t specularColorTextureIndex{ InvalidIndex };      // KHR_materials_specular
	uint32_t anisotropyTextureIndex{ InvalidIndex };         // KHR_materials_anisotropy
	uint32_t transmissionTextureIndex{ InvalidIndex };       // KHR_materials_transmission
	uint32_t diffuseTransmissionTextureIndex{ InvalidIndex };      // KHR_materials_diffuse_transmission (A channel)
	uint32_t diffuseTransmissionColorTextureIndex{ InvalidIndex }; // KHR_materials_diffuse_transmission (RGB channels)
	uint32_t iridescenceTextureIndex{ InvalidIndex };        // KHR_materials_iridescence (R channel)
	uint32_t iridescenceThicknessTextureIndex{ InvalidIndex }; // KHR_materials_iridescence (G channel)
	uint32_t clearcoatTextureIndex{ InvalidIndex };          // KHR_materials_clearcoat (R channel)
	uint32_t clearcoatRoughnessTextureIndex{ InvalidIndex }; // KHR_materials_clearcoat (G channel)
	uint32_t clearcoatNormalTextureIndex{ InvalidIndex };    // KHR_materials_clearcoat
	uint32_t sheenColorTextureIndex{ InvalidIndex };         // KHR_materials_sheen (RGB channels)
	uint32_t sheenRoughnessTextureIndex{ InvalidIndex };     // KHR_materials_sheen (A channel)
	uint32_t thicknessTextureIndex{ InvalidIndex };          // KHR_materials_volume (G channel)

	// Per-texture transforms (glTF KHR_texture_transform)
	Material::STextureTransform baseColorTransform;
	Material::STextureTransform normalTransform;
	Material::STextureTransform emissiveTransform;
	Material::STextureTransform metallicRoughnessTransform;
	Material::STextureTransform occlusionTransform;
	Material::STextureTransform specularTransform;
	Material::STextureTransform specularColorTransform;
	Material::STextureTransform anisotropyTransform;
	Material::STextureTransform transmissionTransform;
	Material::STextureTransform diffuseTransmissionTransform;
	Material::STextureTransform diffuseTransmissionColorTransform;
	Material::STextureTransform iridescenceTransform;
	Material::STextureTransform iridescenceThicknessTransform;
	Material::STextureTransform clearcoatTransform;
	Material::STextureTransform clearcoatRoughnessTransform;
	Material::STextureTransform clearcoatNormalTransform;
	Material::STextureTransform sheenColorTransform;
	Material::STextureTransform sheenRoughnessTransform;
	Material::STextureTransform thicknessTransform;
};

// One cluster (meshlet) of a mesh's index range, produced by meshlet_builder. firstIndex is LOCAL
// to the index list it belongs to (LOD0 or a reduced LOD); the renderer rebases it to the merged
// index buffer at upload. AABB is local model space — used for per-cluster shadow caster culling.
struct SMeshletDesc final
{
	Math::Vec3 aabbMin{ Math::Vec3Zero };
	Math::Vec3 aabbMax{ Math::Vec3Zero };
	uint32_t   firstIndex{ 0 };
	uint32_t   indexCount{ 0 };
};

// The cooked .tgemodel format hashes this layout, so changing it invalidates every cooked model.
static_assert(sizeof(SMeshletDesc) == 32 && alignof(SMeshletDesc) == 4);
static_assert(offsetof(SMeshletDesc, aabbMax) == 12 && offsetof(SMeshletDesc, firstIndex) == 24
              && offsetof(SMeshletDesc, indexCount) == 28);

struct SMeshLODLevel final
{
	std::vector<uint32_t> indices; // indexes into SMeshData::vertices (LOD0 vertices are shared)
	std::vector<SMeshletDesc> clusters; // meshlets of this LOD; firstIndex local to `indices`
};

// KHR_materials_variants: the material a primitive draws with while `variantIndex` is selected.
struct SVariantMaterial final
{
	uint32_t variantIndex{ InvalidIndex }; // index into SModelData::variantNames
	SMaterialData material;
};

struct SMeshData final
{
	// Unconditional by design: gating this by build flag gave a public struct two layouts.
	std::string name;

	std::vector<Geometry::SVertex> vertices;
	std::vector<uint32_t> indices;

	// LOD0 positions in local space, set at load: a renderer releases `vertices` once it has uploaded them.
	Math::CAABB bounds;

	SMaterialData material;

	// Sparse: a variant that maps nothing here keeps `material` in force, as the spec requires.
	std::vector<SVariantMaterial> variantMaterials;

	// Source glTF material index (into the glTF `materials` array); InvalidIndex if the primitive has no
	// material. Retained so KHR_animation_pointer `/materials/{i}/...` channels can resolve to
	// the runtime mesh(es) derived from glTF material i.
	uint32_t sourceMaterialIndex{ InvalidIndex };

	Geometry::EPrimitiveTopology topology{ Geometry::EPrimitiveTopology::TriangleList };
	uint32_t vertexCount{ 0 };  // Vertex count for non-indexed rendering

	std::vector<SMeshLODLevel> reducedLods; // [0]=LOD1 (~50% triangles), [1]=LOD2 (~12.5%)
	std::vector<SMeshletDesc>  clusters;    // LOD0 meshlets; firstIndex local to `indices`

	// Vertex-deformation data, parallel to `vertices` (same order/count). Both empty for static meshes.
	// Morph deltas are laid out [vertexLocal * numMorphTargets + target]; morphWeights holds the default
	// (glTF mesh.weights / node.weights), size == numMorphTargets. skinVertices is one entry per vertex.
	std::vector<Geometry::SMorphDelta> morphDeltas;
	std::vector<float>                 morphWeights;
	uint32_t                           numMorphTargets{ 0 };
	std::vector<Geometry::SSkinVertex> skinVertices;
};

struct SNodeData final
{
	Math::Mat4 localMatrix{ Math::IdentityMat4 };  // Local transform (GLTF matrix or TRS converted to matrix)
	std::vector<uint32_t> meshIndices;  // Support multiple primitives per GLTF mesh
	// EXT_mesh_gpu_instancing per-instance transforms, applied in node space
	// (instanceWorld = nodeWorld * instanceTransform). Empty = not instanced.
	std::vector<Math::Mat4> instanceTransforms;
	std::vector<uint32_t> childIndices;
	uint32_t lightIndex{ InvalidIndex };  // Index into SModelData::lights
	// Source glTF node index (into the glTF `nodes` array). Retained so animation channels
	// (classic target_node and KHR_animation_pointer `/nodes/{i}/...`) resolve to the spawned
	// entity. ProcessNode otherwise remaps nodes to dense local indices, losing this.
	uint32_t sourceNodeIndex{ InvalidIndex };
	// Index into SModelData::skins (== glTF node.skin); InvalidIndex = no skin. The meshes at this node
	// are linear-blend skinned by that skin's joints.
	uint32_t skinIndex{ InvalidIndex };
	// KHR_node_visibility; false hides the whole subtree but the node still transforms + skins.
	bool visible{ true };
	std::string name;
};

// One glTF skin: its joint node list and matching inverse-bind matrices (skin.joints /
// skin.inverseBindMatrices). The animation system builds the per-frame joint palette from the joints'
// world matrices times these. jointNodeIndices are source glTF node indices (resolve via the spawned
// entities); inverseBindMatrices is identity-filled when the accessor is absent (glTF default).
struct SSkinData final
{
	std::vector<uint32_t>   jointNodeIndices;
	std::vector<Math::Mat4> inverseBindMatrices;
};

struct SModelData final
{
	std::vector<SMeshData> meshes;
	std::vector<STextureData> textures;
	std::vector<SNodeData> nodes;
	std::vector<Light::SLight> lights;
	std::vector<SAnimation> animations;
	std::vector<SSkinData> skins;
	std::vector<uint32_t> rootNodeIndices; // the default scene's roots

	// glTF order: a primitive's mappings and a spawned model's selection both key into this by position.
	std::vector<std::string> variantNames;

	// Every declared scene, in glTF order; names are optional and may repeat, so scenes are addressed by index.
	std::vector<std::string>           sceneNames;
	std::vector<std::vector<uint32_t>> sceneNodeIndices;
	uint32_t                           defaultSceneIndex{ 0u };

	std::string sourcePath;
};
} // namespace Tge::Model
