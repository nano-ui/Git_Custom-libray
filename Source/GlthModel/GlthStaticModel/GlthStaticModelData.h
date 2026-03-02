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

struct GlthMesh
{
	std::vector<GlthVertex> vertices;
	std::vector<uint32_t> indices;
	Microsoft::WRL::ComPtr<ID3D11Buffer> vertex_buffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> index_buffer;
};

struct GltfModel
{
	std::vector<GlthMesh> meshes;
	DirectX::XMFLOAT4X4 transform;
};
