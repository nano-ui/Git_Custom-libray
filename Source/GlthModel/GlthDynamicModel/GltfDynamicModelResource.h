#pragma once
#include <vector>
#include <memory>
#include <string>
#include "GltfDynamicMesh.h"
#include "GltfDynamicMaterial.h"

class GltfDynamicModelResource
{
public:
	GltfDynamicModelResource(ID3D11Device* device, const char* filename);

	const std::vector<std::shared_ptr<GltfDynamicMesh>>& GetMeshes() const { return meshes; }
	const std::vector<std::shared_ptr<GltfDynamicMaterial>>& GetMaterials() const { return materials; }

private:
	std::vector<std::shared_ptr<GltfDynamicMesh>> meshes;			//メッシュリスト
	std::vector<std::shared_ptr<GltfDynamicMaterial>> materials;	//マテリアルリスト
};

