#pragma once

#include <string>
#include <vector>
#include <d3d11.h>
#include <wrl.h>
#include <DirectXMath.h>

//PBRマテリアルのパラメータ構造体
struct GltfStaticMaterialData
{
	DirectX::XMFLOAT4 color;		//基本色
	DirectX::XMFLOAT3 emissive;		//放射色
	float metallic;					//メタリック値
	float roughness;				//ラフネス値
	float normal_scale;				//法線スケール	
	float occlusion_strength;		//オクルージョン強度
	int padding[2];					//パディング
};

//マテリアルテクスチャ参照
struct GltfStaticMaterialTextures
{
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> color_texture;		//基本色テクスチャ
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> normal_texture;	//法線テクスチャ
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> metallic_roughness_texture;	//メタリック・ラフネステクスチャ
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> occlusion_texture;	//オクルージョンテクスチャ
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> emissive_texture;	//放射テクスチャ
};


class GltfStaticMaterial
{
public:
	GltfStaticMaterial();
	~GltfStaticMaterial() = default;

	//マテリアルの設定・取得
	void SetMaterialData(const GltfStaticMaterialData& material_data_in);
	const GltfStaticMaterialData& GetMaterialData() const;

	//テクスチャの設定・取得
	void SetTextures(const GltfStaticMaterialTextures& textures_in);
	const GltfStaticMaterialTextures& GetTextures() const;

	//マテリアル名の設定・取得
	void SetMaterialName(const std::string& name);
	const std::string& GetMaterialName() const;

	//コンスタントバッファの作成
	Microsoft::WRL::ComPtr<ID3D11Buffer> CreateConstantBuffer(ID3D11Device* device);

	//マテリアルをシェーダーに適用
	void ApplyToShader(
		ID3D11DeviceContext* context,
		Microsoft::WRL::ComPtr<ID3D11Buffer> constnt_buffer,
		unsigned int slot
	);

private:
	std::string material_name;					//マテリアル名
	GltfStaticMaterialData material_data;		//マテリアルのパラメータ
	GltfStaticMaterialTextures textures;		//マテリアルのテクスチャ参照
};

