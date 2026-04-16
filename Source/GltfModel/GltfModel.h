#pragma once

#define NOMINMAX
#include <d3d11.h>
#include <wrl.h>
#include <DirectXMath.h>
#include <stack>
#define TINYGLTF_NO_EXTERNAL_IMAGE
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_STB_IMAGE_WRITE
#include "tinygltf-release/tiny_gltf.h"
 
class GltfModel
{
public:
	//Gltfシーン情報格納構造体
	struct scene
	{
		std::string name;		//シーンの名前
		std::vector<int> nodes;	//シーンのルートノードインデックス一覧
	};

public:
	//Gltfノード情報格納構造体
	struct node
	{
		std::string name;	//名前
		int skin = -1;		//スキンのインデックス
		int mesh = -1;		//マッシュのイデックス
		std::vector<int> children;	//子ノードのインデックス一覧
		DirectX::XMFLOAT4 rotation = { 0.0f,0.0f,0.0f,1.0f };	//回転情報
		DirectX::XMFLOAT3 scale = { 1.0f,1.0f,1.0f };			//スケール情報
		DirectX::XMFLOAT3 translation = { 0.0f,0.0f,0.0f };		//位置情報
		DirectX::XMFLOAT4X4 global_transform =					//最終的な行列結果
		{
			1.0f,0.0f,0.0f,0.0f,
			0.0f,1.0f,0.0f,0.0f,
			0.0f,0.0f,1.0f,0.0f,
			0.0f,0.0f,0.0f,1.0f
		};
	};

public:
	//GPUバッファの特定範囲を表す構造体
	struct buffer_view
	{
		DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;	//データの形式
		int buffer = -1;							//参照先のバッファインデックス
		size_t stride_in_bytes = 0;					//データ１つ当たりのバイトサイズ
		size_t byte_offset = 0;						//バッファ内の開始オフセット位置
		size_t count = 0;							//含まれる要素の数
	};

public:
	//プリミティブ描画用の定数バッファ構造体
	struct primitive_constants
	{
		DirectX::XMFLOAT4X4 world;	//ワールド変換行列
		int material = -1;			//マテリアルのインデックス
		int has_tangent = 0;		//接線情報の有無を判定するフラグ
		int skin = -1;				//スキン情報のインデックス
		int pad;					//アライメント調整用のパラメータ
	};

public:
	//ポリゴンメッシュの集合を表す構造体
	struct mesh
	{
		std::string name;	//メッシュの名前

		//メッシュを構成する最小単位を表す構造体
		struct primitive
		{
			int material;	//マテリアルのインデックス
			std::map<std::string, buffer_view> vertex_buffer_views;	//属性名ごとの頂点バッファ情報
			buffer_view index_buffer_view;	//インデックスバッファの情報

			//特定の属性を保持しているかの確認
			bool has(const char* attribute)const
			{
				//マップ内に存在し、かつバッファが有効か判定
				return vertex_buffer_views.find(attribute) != vertex_buffer_views.end()
					&& vertex_buffer_views.at(attribute).buffer > -1;
			}
		};
		std::vector<primitive> primitives;	//メッシュに含まれるプリミティブのリスト
	};

public:
	//デバイスとファイルを受け取ってモデルを初期化
	GltfModel(ID3D11Device* device, const std::string& filename);

	//モデル描画
	void Render(ID3D11DeviceContext* immediate_context, const DirectX::XMFLOAT4X4& world);

	//親子関係をたどって各ノードのグローバル行列を計算
	void CumulateTransforms(std::vector<node>& nodes);

	//tinygltfの型情報をDirectXのフォーマットに変換
	DXGI_FORMAT ConvertFormat(const tinygltf::Accessor& accessor);

	//メッシュ情報とGPUバッファを生成
	void FetchMeshes(ID3D11Device* device, const tinygltf::Model& gltf_model);

	//tinygltfのモデルからノード情報を抽出
	void FetchNodes(const tinygltf::Model& gltf_model);

public:
	std::string filename;	//ファイルの名前

	std::vector<scene> scenes;	//解析された全シーンのリスト
	std::vector<node> nodes;	//解析された全ノードのリスト
	std::vector<mesh> meshes;	//解析された全メッシュのリスト
	std::vector<Microsoft::WRL::ComPtr<ID3D11Buffer>> buffers;	//生成された全GPUバッファのリスト
	Microsoft::WRL::ComPtr<ID3D11Buffer> primitive_cbuffer;		//プリミティブの情報を送るための定数バッファ

	Microsoft::WRL::ComPtr<ID3D11VertexShader> vertex_shader;	//頂点シェーダーオブジェクト
	Microsoft::WRL::ComPtr<ID3D11PixelShader> pixel_shader;		//ピクセルシェーダーオブジェクト
	Microsoft::WRL::ComPtr<ID3D11InputLayout> input_layout;		//シェーダーへの入力データ形式を定義するレイアウト

	int default_scene = 0;	//デフォルトで使用されるシーンのインデックス
};

