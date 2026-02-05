#pragma once

#include<vector>
#include <d3d11.h>
#include <unordered_map>
#include <string>

#include "../Common/ModelData.h"

namespace fbxsdk
{
	class FbxScene;
	class FbxMesh;
}

class FbxSkinnedMesh
{
public:
	//メッシュデータの抽出
	static void Fetch(
		ID3D11Device* device,
		fbxsdk::FbxScene* scene,
		const std::string& fbx_filename,
		const std::vector<BoneData>& bones,
		std::vector<MeshData>& out_meshes,
		std::unordered_map<uint64_t, MaterialData>& out_materials);

	//GPUバッファ作成
	static void CreateComBuffers(ID3D11Device* device, MeshData& mesh);

private:
	//制御点ごとのボーン影響度
	using BoneInfluencePerControlPoint = std::vector<BoneInfluence>;

	//ウェイト情報の保持
	static void FetchBoneInfuences(const fbxsdk::FbxMesh* fbx_mesh, const std::vector<BoneData>& bones, std::vector<BoneInfluencePerControlPoint>& influences);
};

