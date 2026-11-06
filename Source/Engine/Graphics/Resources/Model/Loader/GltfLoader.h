#pragma once

#include <memory>
#include <string>
#include <vector>

class GltfModelResource;
class GltfSkeletonResource;
class GltfMeshResource;
class GltfMaterialResource;
class GltfAnimationResource;
struct GltfMeshNodeBinding;

namespace tinygltf
{
	class Model;
}

class GltfLoader
{
public:
	GltfLoader() = default;
	~GltfLoader() = default;

	//読み込み
	static std::shared_ptr<GltfModelResource> Load(const std::string& file_path);

private:
	//ファイル読み込み
	static bool LoadFile(const std::string& file_path, tinygltf::Model& out_model);

	//ボーン情報解析
	static std::shared_ptr<GltfSkeletonResource> ExtractSkelton(const tinygltf::Model& out_model);

	//メッシュ情報解析
	static std::vector<std::shared_ptr<GltfMeshResource>> ExtractMeshes(const tinygltf::Model& out_model);

	//マテリアル情報解析
	static std::vector<std::shared_ptr<GltfMaterialResource>> ExtractMaterial(const tinygltf::Model& out_model);

	//アニメーション情報解析
	static std::shared_ptr<GltfAnimationResource> ExtractAnimation(const tinygltf::Model& out_model);

	//ノードとメッシュの紐づけ
	static std::vector<GltfMeshNodeBinding> ExtractMeshBinding(const tinygltf::Model& out_model);
};

