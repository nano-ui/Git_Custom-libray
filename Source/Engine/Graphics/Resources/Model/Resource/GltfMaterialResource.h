#pragma once

#include <string>
#include <DirectXMath.h>

static constexpr int INVALID_TEXTURE_INDEX = -1;	//テクスチャの無効値
static constexpr int DEFAULT_TEXCOORD_INDEX = 0;	//標準UVスロット番号
static constexpr float DEFAULT_ALPHA_CUTOFF = 0.5f;	//アルファテスト時の標準閾値
static constexpr float DEFAULT_FACTOR_ONE = 1.0f;	//最大要素値
static constexpr float DEFAULT_FACTOR_ZERO = 0.0f;	//最小要素値
static constexpr int DEFAULT_ALPHA_MODE_OPAQE = 0;	//不透明度
static constexpr int DEFAULT_DOUBLE_SIDED_FALSE = 0;//両面表示値

//Gltfテクスチャ情報
struct GltfTextureInfo
{
	int index = INVALID_TEXTURE_INDEX;		//インデックス番号
	int texcoord = DEFAULT_TEXCOORD_INDEX;	//UVセット番号
};

//ノーマルマップテクスチャ情報
struct GltfNormalTextureInfo
{
	GltfTextureInfo texture_info;			//テクスチャ情報
	float scale = DEFAULT_FACTOR_ONE;		//法線の凹凸の深さ
};

//オクルージョンテクスチャ情報
struct GltfOcclusionTextureInfo
{
	GltfTextureInfo texture_info;			//テクスチャ情報
	float strength = DEFAULT_FACTOR_ONE;	//陰影の強弱
};

//PBRテクスチャ情報
struct GltfPbrMetellicRoughness
{
	DirectX::XMFLOAT4 basecolor_factor = { DEFAULT_FACTOR_ONE,DEFAULT_FACTOR_ONE,DEFAULT_FACTOR_ONE,DEFAULT_FACTOR_ONE };	//モデルの色
	GltfTextureInfo basecolor_texture;			//ベースカラーのテクスチャ
	float metallic_factor = DEFAULT_FACTOR_ONE;	//金属間の強さ
	float roughness_factor = DEFAULT_FACTOR_ONE;//表面の粗さ
	GltfTextureInfo metallic_roughness_texture;	//メタリック・ラフネスマップ
};

//Gltfマテリアル情報
struct GltfMaterialConstants
{
	DirectX::XMFLOAT3 emmissive_factor = { DEFAULT_FACTOR_ZERO,DEFAULT_FACTOR_ZERO,DEFAULT_FACTOR_ZERO };	//自己発光色
	int alpha_mode = DEFAULT_ALPHA_CUTOFF;			//描画状態
	float alpha_cutoff = DEFAULT_ALPHA_CUTOFF;		//マスク描画時のアルファ打ち切り値
	int double_sided = DEFAULT_DOUBLE_SIDED_FALSE;	//両面描画フラグ
	GltfPbrMetellicRoughness pbr_metallic_roughness;//PBRパラメータ群
	GltfNormalTextureInfo normal_texture;			//法線マップ
	GltfOcclusionTextureInfo occlusion_texture;		//オクルージョンマップ
	GltfTextureInfo emissive_texture;				//発光マップ
};

class GltfMaterialResource
{
public:
	GltfMaterialResource() = default;
	~GltfMaterialResource() = default;

	//マテリアル名の設定
	void SetName(const std::string& name) { material_name = name; }

	//マテリアル名の取得
	const std::string& GetName()const { return material_name; }

	//マテリアル情報の設定
	void SetData(const GltfMaterialConstants& constants) { data = constants; }

	//マテリアル情報の取得
	const GltfMaterialConstants& GetData()const { return data; }

	//マテリアル情報の取得
	GltfMaterialConstants& GetData() { return data; }

private:
	std::string material_name = "";	//マテリアル名
	GltfMaterialConstants data;		//マテリアル情報
};

