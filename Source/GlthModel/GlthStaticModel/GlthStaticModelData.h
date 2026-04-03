#pragma once

#include <memory>
#include <string>
#include <vector>
#include <d3d11.h>
#include <wrl.h>
#include <DirectXMath.h>

class GltfStaticMaterial;

//Glth系列のモデルデータ構造
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
	int material_index = -1;
};

struct GltfModel
{
	std::vector<GlthMesh> meshes;
	std::vector<std::shared_ptr<GltfStaticMaterial>> materials;
	DirectX::XMFLOAT4X4 transform;

	// gltf_static_mesh シェーダーオブジェクト
	Microsoft::WRL::ComPtr<ID3D11VertexShader>  vertex_shader;
	Microsoft::WRL::ComPtr<ID3D11PixelShader>   pixel_shader;
	Microsoft::WRL::ComPtr<ID3D11InputLayout>   input_layout;
	Microsoft::WRL::ComPtr<ID3D11Buffer>        object_constant_buffer;
};
