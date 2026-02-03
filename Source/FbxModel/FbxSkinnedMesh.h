
#include<vector>
#include <d3d11.h>

#include "../Common/ModelData.h"

namespace fbxsdk
{
	class FbxScene;
	class FbxMesh;
}

class FbxAnimation
{
public:
	//メッシュデータの抽出
	static void Feach(ID3D11Device* device, fbxsdk::FbxScene* scene, const std::vector<BoneData>& bones, std::vector<MeshData>& out_meshes);

private:
	//制御点ごとのボーン影響度
	using BoneInfluencePerControlPoint = std::vector<BoneInfluence>;

	//ウェイト情報の保持
	static void FetchBoneInfuences(const fbxsdk::FbxMesh* fbx_mesh, const std::vector<BoneData>& bones, std::vector<BoneInfluencePerControlPoint>& influences);

	//GPUバッファ作成
	static void CreateComBuffers(ID3D11Device* device, MeshData& mesh);

};

