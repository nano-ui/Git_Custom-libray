#pragma once

#include "d3d11.h"
#include <wrl.h>
#include <vector>
#include <string>
#include <memory>
#include <unordered_map>

#include "../Common/ModelData.h"

//前方宣言
class FbxLoader;
class SkinnedMeshSerializer;

struct NodeData
{
	uint64_t unique_id = 0;		//一意なID
	std::string name;			//ノード名
	int64_t parent_index = -1;	//親ノードのインデックス
	DirectX::XMFLOAT4X4 local_transform;	//ローカル変換行列
	DirectX::XMFLOAT4X4 global_transform;	//グローバル変換行列
};

class FbxSkinnedResource :public std::enable_shared_from_this<FbxSkinnedResource>
{
public:
	//FbxLoaderがprivateメンバにデータを書き込めるように許可する
	friend class FbxLoader;
	friend class SkinnedMeshSerializer;

public:
	//コンストラクタ
	FbxSkinnedResource(ID3D11Device* device);
	~FbxSkinnedResource() = default;

	//FBXファイルの読み込み
	bool Load(const std::string& fbx_filename);

	//ゲッター
	const std::vector<MeshData>& GetMeshes()const { return meshes; }
	const std::vector<BoneData>& GetBones()const { return bones; }
	const std::unordered_map<uint64_t, MaterialData>& GetMaterials() const { return materials; }
	const std::unordered_map<std::string, AnimationClip>& GetAnimations()const { return animations; }
	ID3D11Device* GetDevice()const { return device.Get(); }

public:
	std::vector<NodeData> nodes;//ノードリスト（シーン全体の階層構造）
	std::unordered_map<std::string, int64_t> node_name_map;	//ノード名からインデックスを引くためのマップ
	
	int64_t IndexOf(uint64_t unique_id)const
	{
		for (size_t i = 0; i < nodes.size(); ++i)
		{
			if (nodes[i].unique_id == unique_id) return static_cast<int64_t>(i);
		}
		return -1;
	}

private:
	Microsoft::WRL::ComPtr<ID3D11Device> device;

	//モデルデータ
	std::vector<MeshData> meshes;								//読み込んだメッシュデータを格納するリスト
	std::vector<BoneData> bones;								//読み込んだボーンデータを格納するリスト
	std::unordered_map<std::string, AnimationClip> animations;	//読み込んだアニメーションデータを格納するリスト
	std::unordered_map<uint64_t, MaterialData> materials;		//読み込んだマテリアルデータを格納するリスト
};

