#pragma once
#include <vector>
#include <memory>
#include <string>
#include "GltfDynamicMesh.h"

class GltfDynamicModelResource
{
public:
	GltfDynamicModelResource(ID3D11Device* device, const char* filename);
	const std::vector<std::shared_ptr<GltfDynamicMesh>>& GetMeshes() const { return meshes; }

private:
	std::vector<std::shared_ptr<GltfDynamicMesh>> meshes;	//メッシュリスト
};

