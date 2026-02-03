#pragma once

#include <d3d11.h>
#include <DirectXMath.h>
#include <vector>
#include <string>
#include <wrl.h>

//メッシュの部品
struct MeshSubset
{
	uint64_t material_unique_id{ 0 };	//各マテリアルのID
	std::string material_name;			//各マテリアルの名前
	uint64_t start_index_location{ 0 };	//全体のインデックスバッファの描画開始位置
	uint32_t index_count{ 0 };			//描画する頂点数
};

//メッシュの頂点データ
struct MeshVertex
{
	DirectX::XMFLOAT3 position;							//頂点の座標
	DirectX::XMFLOAT3 normal;							//法線ベクトル
	DirectX::XMFLOAT4 tangent{ 1.0f,0.0f,0.0f,1.0f };	//接線ベクトル
	DirectX::XMFLOAT2 texcoord;							//テクスチャ座標
	float bone_weights[4]{ 1.0f,0.0f,0.0f,0.0f };		//頂点のボーンの影響度
	uint32_t bone_indices[4]{ 0,0,0,0 };				//影響を受けるボーンの番号
};

//モデルの形状データ（頂点の集合体）
struct MeshData
{
	uint64_t unique_id{ 0 };							//メッシュの固有ID
	std::string name;									//メッシュの名前
	int64_t node_index{ 0 };							//シーン内のどのノード(ボーン)に紐づいているか
	DirectX::XMFLOAT4X4 default_global_transform;		//初期の配置行列
	std::vector<MeshSubset> subsets;					//マテリアル分けされたサブセットのリスト

	//シリアライズ用にCPU側にデータを一時保持するコンテナ
	std::vector<MeshVertex> vertices;
	std::vector<uint32_t> indices;

	Microsoft::WRL::ComPtr<ID3D11Buffer> vertex_buffer;	//頂点データをGPUに送るバッファ
	Microsoft::WRL::ComPtr<ID3D11Buffer> index_buffer;	//頂点の順番(インデックス)をGPUに送るバッファ
	DirectX::XMFLOAT3 bounding_box[2]					//バウンディングボックス
	{
		{+D3D11_FLOAT32_MAX,+D3D11_FLOAT32_MAX,+D3D11_FLOAT32_MAX},	//Min
		{-D3D11_FLOAT32_MAX,-D3D11_FLOAT32_MAX,-D3D11_FLOAT32_MAX}	//Max
	};
};

//アニメーションするための骨の情報
struct BoneData
{
	std::string name;						//ボーンの名前
	int64_t parent_index{ -1 };				//親のボーン番号
	DirectX::XMFLOAT4X4 offset_transform;	//逆バインドポーズ行列
};

//内部計算用のヘルパー構造体（頂点ごとのボーン影響情報）
struct BoneInfluence
{
	uint32_t bone_index;
	float weight;
};

//アニメーションの1フレームにおけるボーンの状態
struct AnimationKeyframeNode
{
	DirectX::XMFLOAT4X4 global_transform;	//計算済みの最終的な行列
	DirectX::XMFLOAT3 scaling;				//拡大縮小
	DirectX::XMFLOAT4 rotation;				//回転
	DirectX::XMFLOAT3 translation;			//位置移動
};

//アニメーション全体
struct AnimationClip
{
	std::string name;											//クリップ名
	float sampling_rate{ 0 };									//1秒間のフレーム数
	std::vector<std::vector<AnimationKeyframeNode>> sequence;	//キーフレームの配列(時間経過ごとの全ボーンの姿勢リスト)
};

//モデルの色やテクスチャの情報
struct MaterialData
{
	uint64_t unique_id{ 0 };													//マテリアル固有のID
	std::string name;															//マテリアル名
	DirectX::XMFLOAT4 color{ 1.0f,1.0f,1.0f,1.0f };								//基本色
	std::string texture_filenames[2];											//テクスチャのファイル名
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> shader_resource_views[2];	//GPUでテクスチャを読むためのビュー
};
