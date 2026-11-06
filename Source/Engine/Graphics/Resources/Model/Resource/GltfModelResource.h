#pragma once

#include <vector>
#include <string>
#include <memory>

#include "GltfMeshResource.h"
#include "GltfMaterialResource.h"
#include "GltfSkeletonResource.h"
#include "GltfAnimationResource.h"

//メッシュとノードの紐づけ情報
struct GltfMeshNodeBinding
{
	int node_index;	//ボーンノードインデックス番号
	int mesh_index;	//メッシュ情報インデックス番号
	int skin_index;	//スキニング情報インデックス番号

	GltfMeshNodeBinding()
	{
		node_index = INVALED_NODE_INDEX;
		mesh_index = -1;
		skin_index = INVALED_NODE_INDEX;
	}
};

class GltfModelResource
{
public:
	//コンストラクタ
	GltfModelResource()
	{
		model_name = "";
		file_path = "";
		skeleton_resource = nullptr;
		animation_resource = nullptr;
	}

	~GltfModelResource() = default;

	//モデル名の設定
	void SetModelName(const std::string& name) { model_name = name; }

	//モデル名の取得
	const std::string& GetModelName()const { return model_name; }

	//ファイルパスの設定
	void SetFilePath(const std::string& path) { file_path = path; }

	//ファイルパスの取得
	const std::string& GetFilePath()const { return file_path; }

	//スケルトン情報の設定
	void SetSkeletonResource(const std::shared_ptr<GltfSkeletonResource>& skeleton) { skeleton_resource = skeleton; }

	//スケルトン情報の取得
	std::shared_ptr<GltfSkeletonResource> GetSkeletonResource()const { return skeleton_resource; }

	//アニメーション情報の設定
	void SetAnimationResource(const std::shared_ptr<GltfAnimationResource>& animation) { animation_resource = animation; }

	//アニメーション情報の取得
	std::shared_ptr<GltfAnimationResource> GetAnimationResource()const { return animation_resource; }

	//メッシュ情報の追加
	void AddMeshResource(const std::shared_ptr<GltfMeshResource>& mesh) { mesh_resources.push_back(mesh); }

	//メッシュ情報の取得
	std::vector<std::shared_ptr<GltfMeshResource>> GetMeshResource()const { return mesh_resources; }

	//マテリアル情報の追加
	void AddMaterialResource(const std::shared_ptr<GltfMaterialResource>& material) { material_resources.push_back(material); }

	//マテリアル情報の取得
	std::vector<std::shared_ptr<GltfMaterialResource>> GetMaterialResource()const { return material_resources; }

	//バインド情報の追加
	void AddMeshBinding(const GltfMeshNodeBinding& binding) { mesh_bindings.push_back(binding); }

	//バインド情報の取得
	const std::vector<GltfMeshNodeBinding>& GetMeshBinding()const { return mesh_bindings; }

private:
	std::string model_name;													//モデル名
	std::string file_path;													//モデルパス
	std::shared_ptr<GltfSkeletonResource> skeleton_resource;				//スケルトン情報
	std::shared_ptr<GltfAnimationResource> animation_resource;				//アニメーション情報
	std::vector<std::shared_ptr<GltfMeshResource>> mesh_resources;			//メッシュ情報配列
	std::vector<std::shared_ptr<GltfMaterialResource>> material_resources;	//マテリアル情報配列
	std::vector<GltfMeshNodeBinding> mesh_bindings;							//ノードとメッシュの対応関係配列
};