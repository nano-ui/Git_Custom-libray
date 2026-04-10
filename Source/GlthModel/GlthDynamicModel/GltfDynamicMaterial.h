#pragma once
#include <d3d11.h>
#include <wrl.h>
#include <DirectXMath.h>
#include <string>
#include "tiny_gltf.h"

class GltfDynamicMaterial
{
public:
	//コンストラクタ
	GltfDynamicMaterial();

	//GLTFデータからマテリアルを初期化
	void Initialize(
		ID3D11Device* device,
		const tinygltf::Model& model,
		const tinygltf::Material& gltf_mat,
		const std::string& directory);
	
	//シェーダーへのバインド
	void Bind(ID3D11DeviceContext* dc);

private:
	DirectX::XMFLOAT4 base_color;	//マテリアルの基本色
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> base_map;		//基本テクスチャ
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> normal_map;	//法線マップ
};
