#pragma once

#include <vector>
#include <string>
#include <Windows.h>
#include <DirectXMath.h>

#include "Engine\Core\Log\Log.h"

static constexpr int INVALED_NODE_INDEX = -1;	//ノードの無効値

//ボーンノード情報
struct GltfBoneNode
{
	std::string name = "";									//ボーン名
	int parent_index = INVALED_NODE_INDEX;					//親ボーンのインデックス番号
	std::vector<int> children;								//子ボーンのインデックス番号配列
	DirectX::XMFLOAT3 translation = { 0.0f,0.0f,0.0f };		//ローカル平行移動
	DirectX::XMFLOAT4 rotation = { 0.0f,0.0f,0.0f,1.0f };	//ローカル回転クォータニオン
	DirectX::XMFLOAT3 scale = { 1.0f,1.0f,1.0f };			//ローカル拡大率
	DirectX::XMFLOAT4X4 local_transform =					//ローカル姿勢行列
	{					
		1.0f,0.0f,0.0f,0.0f,
		0.0f,1.0f,0.0f,0.0f,
		0.0f,0.0f,1.0f,0.0f,
		0.0f,0.0f,0.0f,1.0f
	};
};

//スキニング情報
struct GltfSkin
{
	std::vector<DirectX::XMFLOAT4X4> inverse_bind_matrices;	//逆バインド行列配列
	std::vector<int> joints;								//ボーンノードのインデックス配列
};

class GltfSkeletonResource
{
public:
	GltfSkeletonResource() = default;
	~GltfSkeletonResource() = default;

public:
	//ボーンノードを設定
	void SetNodes(std::vector<GltfBoneNode>&& in_nodes) { nodes = std::move(in_nodes); }

	//ボーンノードを取得
	const std::vector<GltfBoneNode>& GetNodes()const { return nodes; }

	//ボーンノードを取得
	std::vector<GltfBoneNode>& GetNodes() { return nodes; }

public:
	//スキニング情報を設定
	void SetSkins(std::vector<GltfSkin>&& in_skins) { skins = std::move(in_skins); }

	//スキニング情報を取得
	const std::vector<GltfSkin>& GetSkins()const { return skins; }

	//スキニング情報を取得
	std::vector<GltfSkin>& GetSkins() { return skins; }

public:
	//階層順の探索用インデックス配列の設定
	void SetHierachyNodeIndices(std::vector<int>&& indices) { hierachy_node_indices = std::move(indices); }

	//階層順の探索用インデックス配列の取得
	const std::vector<int>& GetHierachyNodeIndices()const { return hierachy_node_indices; }

public:
	//ノードインデックス番号を検索
	int FindNodeIndex(const std::string& node_name)const
	{
		if (node_name.empty())
		{
			LOG_WARNING({ u8"設定" }, u8"検索対象の名前が設定されていません");
			return INVALED_NODE_INDEX;
		}

		for (size_t i = 0; i < nodes.size(); i++)
		{
			if (nodes[i].name == node_name)return static_cast<int>(i);
		}
		LOG_WARNING({ u8"検索" }, u8"検索対象のノードが見つかりませんでした", node_name);
		return INVALED_NODE_INDEX;
	}

private:
	std::vector<GltfBoneNode> nodes;		//ボーンノード配列
	std::vector<GltfSkin> skins;			//スキニング配列
	std::vector<int> hierachy_node_indices;	//階層順の探索用インデックス配列
};