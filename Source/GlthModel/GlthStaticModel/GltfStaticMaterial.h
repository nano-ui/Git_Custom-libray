#pragma once

#include <d3d11.h>
#include <wrl.h>
#include <DirectXMath.h>

class GltfStaticMaterial
{
public:
	// PBRマテリアル定数バッファデータ (HLSLのMATERIAL_CONSTANT_BUFFERに対応)
	struct MaterialConstants
	{
		DirectX::XMFLOAT4 color_factor     = { 1.0f, 1.0f, 1.0f, 1.0f };
		DirectX::XMFLOAT3 emissive_factor  = { 0.0f, 0.0f, 0.0f };
		float metallic_factor              = 1.0f;
		float roughness_factor             = 1.0f;
		float normal_scale                 = 1.0f;
		float occlusion_strength           = 1.0f;
		float _padding                     = 0.0f;
	};

	GltfStaticMaterial() = default;
	~GltfStaticMaterial() = default;

	// PBRマテリアルデータ
	MaterialConstants data;

	// テクスチャ (ピクセルシェーダーの t0〜t4 に対応)
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> color_texture;            // t0
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> normal_texture;           // t1
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> metallic_roughness_texture; // t2
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> occlusion_texture;        // t3
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> emissive_texture;         // t4

	// 定数バッファ作成
	bool CreateConstantBuffer(ID3D11Device* device);

	// マテリアルデータとテクスチャをピクセルシェーダーへ適用
	void ApplyToShader(ID3D11DeviceContext* context) const;

private:
	Microsoft::WRL::ComPtr<ID3D11Buffer> constant_buffer;
};
