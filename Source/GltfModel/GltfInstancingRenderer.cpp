#include "GltfInstancingRenderer.h"
#include "GltfModelData.h"
#include "../Graphics/shader.h"
#include "../Shaders/CustomShader.h"
#include <d3d11.h>
#include <misc.h>

//コンストラクタ
GltfInstancingRenderer::GltfInstancingRenderer(ID3D11Device* device)
{
	//入力レイアウトの定義
	D3D11_INPUT_ELEMENT_DESC input_element_desc[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA,   0 },
		{ "NORMAL"  , 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA,   0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA,   0 },
		// 4x4行列はfloat4が4つ分として定義する
		{ "INSTANCE_TRANSFORM", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, INSTANCE_SLOT, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
		{ "INSTANCE_TRANSFORM", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, INSTANCE_SLOT, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
		{ "INSTANCE_TRANSFORM", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, INSTANCE_SLOT, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
		{ "INSTANCE_TRANSFORM", 3, DXGI_FORMAT_R32G32B32A32_FLOAT, INSTANCE_SLOT, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
	};

	//シェーダーの初期化
	custom_shader = std::make_unique<CustomShader>();
	bool is_success = custom_shader->Initialize("gltf_instancing_vs.cso", "gltf_model_ps.cso", input_element_desc, _countof(input_element_desc));
	_ASSERT_EXPR(is_success, L"Failed to initialize instancing shader");

	//インスタンスバッファの作成
	D3D11_BUFFER_DESC buffer_desc = {};
	buffer_desc.ByteWidth = sizeof(instance_data) * MAX_INSTANCE_COUNT;
	buffer_desc.Usage = D3D11_USAGE_DYNAMIC;
	buffer_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	buffer_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	HRESULT hr = device->CreateBuffer(&buffer_desc, nullptr, instance_buffer.ReleaseAndGetAddressOf());
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));
}

//複数のインスタンスを一括で描画
void GltfInstancingRenderer::RenderInstanced(ID3D11DeviceContext* immediate_context, const GltfModelData& data, const std::vector<instance_data>& instances)
{
	if (instances.empty()) return;

	//インスタンスバッファの更新
	D3D11_MAPPED_SUBRESOURCE mapped_resource;
	HRESULT hr = immediate_context->Map(instance_buffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_resource);
	if (SUCCEEDED(hr))
	{
		memcpy(mapped_resource.pData, instances.data(), sizeof(instance_data) * instances.size());
		immediate_context->Unmap(instance_buffer.Get(), 0);
	}

	//パイプラインへのバッファのセット
	UINT stride = sizeof(instance_data);
	UINT offset = 0;
	immediate_context->IASetVertexBuffers(INSTANCE_SLOT, 1, instance_buffer.GetAddressOf(), &stride, &offset);
	custom_shader->Apply();

	//メッシュの巡回とワンドローコールの実行
	const GltfModelData::node& root_node = data.nodes.at(data.scenes.at(data.default_scene).nodes.at(0));
	if (root_node.mesh > -1)
	{
		const GltfModelData::mesh& mesh = data.meshes.at(root_node.mesh);
		for (const GltfModelData::mesh::primitive& primitive : mesh.primitives)
		{
			ID3D11Buffer* vertex_buffers[] = { primitive.has("POSITION") ? data.buffers.at(primitive.vertex_buffer_views.at("POSITION").buffer).Get() : nullptr };
			UINT strides[] = { primitive.has("POSITION") ? static_cast<UINT>(primitive.vertex_buffer_views.at("POSITION").stride_in_bytes) : 0 };
			UINT offsets[] = { primitive.has("POSITION") ? static_cast<UINT>(primitive.vertex_buffer_views.at("POSITION").byte_offset) : 0 };
			immediate_context->IASetVertexBuffers(0, 1, vertex_buffers, strides, offsets);

			if (primitive.index_buffer_view.buffer > -1)
			{
				immediate_context->IASetIndexBuffer(data.buffers.at(primitive.index_buffer_view.buffer).Get(), primitive.index_buffer_view.format, static_cast<UINT>(primitive.index_buffer_view.byte_offset));
				immediate_context->DrawIndexedInstanced(static_cast<UINT>(primitive.index_buffer_view.count), static_cast<UINT>(instances.size()), 0, 0, 0);
			}
		}
	}
}
