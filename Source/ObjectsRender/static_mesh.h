#pragma once

#include <d3d11.h>
#include <DirectXMath.h>
#include <wrl.h>
#include<string>
#include<vector>

#include "texture.h"

class static_mesh
{

public:
	struct vertex
	{
		DirectX::XMFLOAT3 position;
		DirectX::XMFLOAT3 normal;
		DirectX::XMFLOAT2 texcord;
	};

	struct constants
	{
		DirectX::XMFLOAT4X4 world;
		DirectX::XMFLOAT4 material_color;
	};

	struct subset
	{
		std::wstring usimtl;
		uint32_t index_start{ 0 };
		uint32_t index_count{ 0 };
	};
	std::vector<subset>subsets;

private:
	Microsoft::WRL::ComPtr<ID3D11Buffer>vertex_buffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer>index_buffer;

	Microsoft::WRL::ComPtr<ID3D11VertexShader>vertex_shader;
	Microsoft::WRL::ComPtr<ID3D11PixelShader>pixel_shader;
	Microsoft::WRL::ComPtr<ID3D11InputLayout>input_layout;
	Microsoft::WRL::ComPtr<ID3D11Buffer>constant_buffer;


	//std::wstring texture_filename;
	//Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> shader_resource_view;
	struct material
	{
		std::wstring name;
		DirectX::XMFLOAT4 ka{ 0.2f,0.2f,0.2f,0.2f };
		DirectX::XMFLOAT4 kd{ 0.8f,0.8f,0.8f,0.8f };
		DirectX::XMFLOAT4 ks{ 1.0f,1.0f,1.0f,1.0f };
		std::wstring texture_filename[2];
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> shader_resource_view[2];
	};

	std::vector<material> materials;

	DirectX::XMFLOAT3 min_pos;
	DirectX::XMFLOAT3 max_pos;

public:
	static_mesh(ID3D11Device* device, const wchar_t* obj_fileneme);
	virtual ~static_mesh() = default;

	void render(
		ID3D11DeviceContext* immediate_context,
		const DirectX::XMFLOAT4X4& world,
		const DirectX::XMFLOAT4& material_color
	);


	DirectX::XMFLOAT3 get_min_pos() const { return min_pos; }
	DirectX::XMFLOAT3 get_max_pos() const { return max_pos; }

	//バウンディングボックス描画用の頂点バッファとインデックスバッファ
	Microsoft::WRL::ComPtr<ID3D11Buffer> Bbox_vertex_buffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> Bbox_index_buf;


protected:
	void create_com_buffers(
		ID3D11Device* device,
		vertex* vertices,
		size_t vertex_count,
		uint32_t* indices,
		size_t index_count
	);

};

