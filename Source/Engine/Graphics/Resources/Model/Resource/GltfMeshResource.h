#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include <DirectXMath.h>

static constexpr int INVALID_MATERIAL_INDEX = -1;	//マテリアルの未指定
static constexpr int BONE_INFLUENCE_COUNT = 4;		//１頂点に影響を与える最大ボーン数

//頂点情報
struct GltfMeshVertex
{
	DirectX::XMFLOAT3 position = { 0.0f,0.0f,0.0f };	//頂点座標
	DirectX::XMFLOAT3 normal = { 0.0f,1.0f,0.0f };		//法線ベクトル
	DirectX::XMFLOAT4 tangent = { 1.0f,0.0f,0.0f,1.0f };//接線ベクトル
	DirectX::XMFLOAT2 texcoord = { 0.0f,0.0f };			//UV座標
	uint16_t joints[BONE_INFLUENCE_COUNT] = { 0,0,0,0 };//ボーンのインデックス番号配列
	DirectX::XMFLOAT4 weights = { 1.0f,0.0f,0.0f,0.0f };//影響度
};

//サブメッシュ情報
struct GltfSubMesh
{
	int material_index = INVALID_MATERIAL_INDEX;	//マテリアルインデックス
	uint32_t index_start = 0;						//インデックスの開始位置
	uint32_t index_count = 0;						//インデックス数
};

class GltfMeshResource
{
public:
	GltfMeshResource() = default;
	~GltfMeshResource() = default;

	//メッシュ名設定
	void SetName(const std::string& name) { mesh_name = name; }

	//メッシュ名の取得
	const std::string& GetName()const { return mesh_name; }

	//頂点情報配列の設定
	void SetVertices(std::vector<GltfMeshVertex>&& in_vertices) { vertices = std::move(in_vertices); }

	//頂点情報配列の取得
	const std::vector<GltfMeshVertex>& GetVertices()const { return vertices; }

	//頂点情報配列の取得
	std::vector<GltfMeshVertex>& GetVertices() { return vertices; }

	//インデックス配列の設定
	void SetIndices(std::vector<uint32_t>&& in_indices) { indices = std::move(in_indices); }

	//インデックス配列の取得
	const std::vector<uint32_t>& GetIndices()const { return indices; }

	//インデックス配列の取得
	std::vector<uint32_t>& GetIndices() { return indices; }

	//サブメッシュの追加
	void AddSubMesh(const GltfSubMesh& sub_mesh) { sub_meshes.push_back(sub_mesh); }

	//サブメッシュ配列の取得
	const std::vector<GltfSubMesh>& GetSubMeshs()const { return sub_meshes; }

	//サブメッシュ配列の取得
	std::vector<GltfSubMesh>& GetSubMeshs() { return sub_meshes; }

private:
	std::string mesh_name = "";				//メッシュ名
	std::vector<GltfMeshVertex> vertices;	//頂点情報配列
	std::vector<uint32_t> indices;			//インデックス配列
	std::vector<GltfSubMesh> sub_meshes;	//サブメッシュ配列
};