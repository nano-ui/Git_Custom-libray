#pragma once

#include <string>
#include <vector>
#include <d3d11.h>
#include <wrl.h>
#include <DirectXMath.h>

//Glth形式のモデルデータ構造体
struct GlthVertex
{
	DirectX::XMFLOAT3 position;
	DirectX::XMFLOAT3 normal;
	DirectX::XMFLOAT2 texcoord;
};

struct GlthMaterial
{
	std::string name;
	DirectX::XMFLOAT4 base_color = { 1.0f, 1.0f, 1.0f, 1.0f };
	float metalness = 0.0f;
	float roughness = 1.0f;

	// テクスチャリソース
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> base_color_texture;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> normal_texture;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> metallic_roughness_texture;
};

struct GlthMesh
{
	std::vector<GlthVertex> vertices;
	std::vector<uint32_t> indices;
	Microsoft::WRL::ComPtr<ID3D11Buffer> vertex_buffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> index_buffer;

	//マテリアル情報
	int materialIndex = -1;
	GlthMaterial* material = nullptr;
};

struct GltfModel
{
	std::vector<GlthMesh> meshes;
	std::vector<GlthMaterial> materials;
	DirectX::XMFLOAT4X4 transform;
};
