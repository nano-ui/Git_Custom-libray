#pragma once
#include <vector>
#include <memory>
#include <string>
#include "GltfDynamicMesh.h"
#include "GltfDynamicMaterial.h"
#include "GltfBone.h"
#include "GltfAnimation.h"

class GltfDynamicModelResource
{
public:
	GltfDynamicModelResource(ID3D11Device* device, const char* filename);

	const std::vector<std::shared_ptr<GltfDynamicMesh>>& GetMeshes() const { return meshes; }
	const std::vector<std::shared_ptr<GltfDynamicMaterial>>& GetMaterials() const { return materials; }
	const std::vector<GltfBone>& GetBones() const { return bones; }
	const std::vector<GltfAnimation>& GetAnimation()const { return animations; }

private:
	//アクセッサからポインタを取得
	template<typename T>
	const T* GetElementPointer(const tinygltf::Model& model, int accessor_index);

private:
	std::vector<std::shared_ptr<GltfDynamicMesh>> meshes;			//メッシュリスト
	std::vector<std::shared_ptr<GltfDynamicMaterial>> materials;	//マテリアルリスト
	std::vector<GltfBone> bones;									//ボーンリスト
	std::vector<GltfAnimation> animations;							//アニメーションリスト
};

