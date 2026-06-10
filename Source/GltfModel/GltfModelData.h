#pragma once

#define NOMINMAX
#include <d3d11.h>
#include <wrl.h>
#include <DirectXMath.h>
#include <vector>
#include <string>
#include <map>
#include <unordered_map>
#include <filesystem>
#include <unordered_map>

#define TINYGLTF_NO_EXTERNAL_IMAGE
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_STB_IMAGE_WRITE
#include "tinygltf-release/tiny_gltf.h"

class GltfModelData
{
public:
private:
	static const size_t PRIMITIVE_MAX_JOINTS = 512;	//1つのプリミティブが持てる最大ジョイント数
	static constexpr size_t MATRIX_DIMENSION = 4;

public:
	//Gltfシーン情報
	struct scene
	{
		std::string name;		//シーンの名前
		std::vector<int> nodes;	//シーンのルートノードインデックス一覧
	};

public:
	//Gltfノード情報
	struct node
	{
		std::string name;	//名前
		int parent = -1;	//親ノードのインデックス
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
	//GPUバッファの特定範囲を表す情報
	struct buffer_view
	{
		DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;	//データの形式
		int buffer = -1;							//参照先のバッファインデックス
		size_t stride_in_bytes = 0;					//データ１つ当たりのバイトサイズ
		size_t byte_offset = 0;						//バッファ内の開始オフセット位置
		size_t count = 0;							//含まれる要素の数
	};

public:
	//プリミティブ描画用の定数バッファ情報
	struct primitive_constants
	{
		DirectX::XMFLOAT4X4 world;	//ワールド変換行列
		int material = -1;			//マテリアルのインデックス
		int has_tangent = 0;		//接線情報の有無を判定するフラグ
		int skin = -1;				//スキン情報のインデックス
		int pad;					//アライメント調整用のパラメータ
	};

public:
	//ポリゴンメッシュの集合体情報
	struct mesh
	{
		std::string name;	//メッシュの名前

		//メッシュを構成する最小単位情報
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
	//テクスチャの基本情報
	struct texture_info
	{
		int index = -1;		//テクスチャ番号
		int texcoord = 0;	//UV座標の設定
	};

	//法線マップの情報
	struct normal_texture_info
	{
		int index = -1;		//法線テクスチャの番号
		int texcoord = 0;	//UV座標の設定
		float scale = 1;	//法線の強度を調整するスケール値
	};

	//オクルージョンマップの情報
	struct occlusion_texture_info
	{
		int index = -1;		//オクルージョンテクスチャの番号
		int texcoord = 0;	//UV座標の設定
		float strength = 1;	//遮蔽の強さ
	};

	//PBR(物理ベースレンダリング)の金属・粗さ情報
	struct pbr_metallic_roughness
	{
		float basecolor_factor[4] = { 1.0f,1.0f,1.0f,1.0f };//基本色
		texture_info basecolor_texture;						//基礎テクスチャ情報
		float metallic_factor = 1;							//金属の強さ
		float roughness_factor = 1;							//表面の粗さ
		texture_info metallic_roughness_texture;			//金属感・粗さマップの情報
	};

public:

	//最終的なマテリアル情報
	struct material
	{
		std::string name;	//マテリアル名
		struct cbuffer
		{
			float emissive_factor[3] = { 0.0f,0.0f,0.0f };	//自己発光の強度
			int alpha_mode = 0;								//アルファ値の描画モード
			float alpha_cutoff = 0.5f;						//アルファマスクの閾値
			int double_sided = 0;							//画面描画を行うかどうかのフラグ
			pbr_metallic_roughness pbr_metallic_roughness;	//PBRパラメータ構造体
			normal_texture_info normal_texture;				//法線マップ構造体
			occlusion_texture_info occlusion_texture;		//オクルージョンマップ構造体
			texture_info emissive_texture;					//基本マップ構造体
		};
		cbuffer data;	//CPU転送用の実データ
	};

public:
	//テクスチャ情報
	struct texture
	{
		std::string name;	//テクスチャ名
		int source = -1;	//imageのインデックス
	};

	//画像リソースの情報
	struct image
	{
		std::string name;		//画像名
		int width = -1;			//横幅
		int height = -1;		//立幅
		int component = -1;		//コンポーネント数
		int bits = -1;			//ビット数
		int pixel_type = -1;	//ピクセルタイプ
		int buffer_view;		//バッファビューインデックス
		std::string mime_type;	//MIMEタイプ
		std::string uri;		//UPIパス
		bool as_is = false;		//生データフラグ
		std::vector<unsigned char> raw_image_data;	//シリアライズ用に画像の生バイナリデータを直接保持する配列
	};

public:
	//スキニング情報
	struct skin
	{
		std::vector<DirectX::XMFLOAT4X4> inverse_bind_matrices;	//逆バインド行列
		std::vector<int> joints;	//ジョイントして機能するノード群
	};

public:
	//アニメーション情報
	struct animation
	{
		std::string name;	//アニメーション名
		float duration = 0.0f;	//再生時間
		struct channel
		{
			int sampler = -1;	//サンプラー番号
			int target_node = -1;	//対象ノード
			std::string target_path;	//対象パラメータ
		};
		std::vector<channel> channels;	//チャンネルリスト
		//アニメーションサンプラー情報
		struct sampler
		{
			int input = -1;	//時間軸(timeline)の番号
			int output = -1;	//値(keyframe)の番号
			std::string interpolation;	//補完方法
		};
		std::vector<sampler> samplers;		//全アニメーションサンプラーのリスト

		std::unordered_map<int /*sampler.input*/, std::vector<float>> timelines;					//時間軸
		std::unordered_map<int /*sampler output*/, std::vector<DirectX::XMFLOAT3>> scales;			//スケール軸
		std::unordered_map<int /*sampler.output*/, std::vector<DirectX::XMFLOAT4>> rotations;		//回転軸
		std::unordered_map<int /*sampler.output*/, std::vector<DirectX::XMFLOAT3>> translations;	//位置軸
	};

public:
	struct primitive_joint_constants
	{
		DirectX::XMFLOAT4X4 matrices[PRIMITIVE_MAX_JOINTS];
	};

public:
	//コンストラクタ
	GltfModelData() = default;

	//デバイスとファイルを受け取ってモデルデータを初期化
	GltfModelData(const Microsoft::WRL::ComPtr<ID3D11Device>& device, const std::string& filename);

	//復元した生データからGPUリソースを構築
	void CreateGpuResources(ID3D11Device* device);

	//バイナリキャッシュを利用してモデルを読み込む
	static std::shared_ptr<GltfModelData> Load(ID3D11Device* device, const std::string& filename);

private:
	DXGI_FORMAT ConvertFormat(const tinygltf::Accessor& accessor);
	void FetchMeshes(const tinygltf::Model& gltf_model);
	void FetchNodes(const tinygltf::Model& gltf_model);
	void FetchMaterials(const tinygltf::Model& gltf_model);
	void FetchTextures(const tinygltf::Model& gltf_model);
	void FetchAnimations(const tinygltf::Model& gltf_model);

	//アニメーション名をマップに登録
	void MapAnimationNames(const tinygltf::Model& gltf_model);

	//親から子の順にノードを並べ直す
	void ReorderNodes();

	//親から子の順に行列を計算するための更新順序リストを構築
	void ComputeUpdateOrder();

public:
	std::vector<std::vector<unsigned char>> raw_buffers;	//シリアライズ用の生データ保持配列
	std::string filename;	//ファイルの名前

	std::vector<scene> scenes;			//全シーンのリスト
	std::vector<node> nodes;			//全ノードのリスト
	std::vector<mesh> meshes;			//全メッシュのリスト
	std::vector<material> materials;	//全マテリアルのリスト
	std::vector<texture> textures;		//全テクスチャのリスト
	std::vector<image> images;			//全画像のリスト
	std::vector<skin> skins;			//全スキンのリスト
	std::vector<animation> animations;	//全アニメーションのリスト

	std::vector<int> node_update_order;	//親から子の順に並んだノードインデックスのリスト

	std::vector<Microsoft::WRL::ComPtr<ID3D11Buffer>> buffers;								//生成された全GPUバッファのリスト
	std::vector<Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>> texture_resource_views;	//全テクスチャビューのリスト

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> material_resource_view;	//全マテリアル情報を格納したSRV

	int default_scene = 0;	//開始シーン番号

	std::unordered_map<std::string, size_t> animation_index_map;	//アニメーション名からインデックスを検索するためのマップ
};

