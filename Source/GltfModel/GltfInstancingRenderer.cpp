#include "GltfInstancingRenderer.h"
#include "GltfModelData.h"
#include "../Graphics/shader.h"
#include "../Graphics/Graphics.h"
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
		{ "TANGENT" , 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA,   0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA,   0 },
		{ "JOINTS"  , 0, DXGI_FORMAT_R16G16B16A16_UINT,  0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA,   0 },
		{ "WEIGHTS" , 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA,   0 },

		{ "INSTANCE_TRANSFORM", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, INSTANCE_SLOT, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
		{ "INSTANCE_TRANSFORM", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, INSTANCE_SLOT, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
		{ "INSTANCE_TRANSFORM", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, INSTANCE_SLOT, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
		{ "INSTANCE_TRANSFORM", 3, DXGI_FORMAT_R32G32B32A32_FLOAT, INSTANCE_SLOT, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
		{ "BONE_OFFSET",        0, DXGI_FORMAT_R32_UINT,           INSTANCE_SLOT, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
	};

	//シェーダーの初期化
	custom_shader = std::make_unique<CustomShader>();
	bool is_success = custom_shader->Initialize("gltf_instancing_skin_vs.cso", "gltf_model_ps.cso", input_element_desc, _countof(input_element_desc));
	_ASSERT_EXPR(is_success, L"Failed to initialize instancing shader");

	//インスタンスバッファの作成
	D3D11_BUFFER_DESC buffer_desc = {};
	buffer_desc.ByteWidth = sizeof(instance_data) * MAX_INSTANCE_COUNT;
	buffer_desc.Usage = D3D11_USAGE_DYNAMIC;
	buffer_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	buffer_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	HRESULT hr = device->CreateBuffer(&buffer_desc, nullptr, instance_buffer.ReleaseAndGetAddressOf());
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

	//GPUへ送るための定数バッファの生成
	buffer_desc.ByteWidth = sizeof(GltfModelData::primitive_constants);																	
	buffer_desc.Usage = D3D11_USAGE_DEFAULT;																							
	buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;																					
	buffer_desc.CPUAccessFlags = OFFSET_ZERO;																							
	hr = device->CreateBuffer(&buffer_desc, nullptr, primitive_cbuffer.ReleaseAndGetAddressOf());										
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));	

	InitializeBoneBuffer(device);
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

	//パイプラインステートと共通リソースの設定
	UINT stride = sizeof(instance_data);
	UINT offset = 0;
	immediate_context->IASetVertexBuffers(INSTANCE_SLOT, 1, instance_buffer.GetAddressOf(), &stride, &offset);
	custom_shader->Apply();

	auto states = Graphics::Instance().GetPipelineStates();																				//グラフィックス管理クラスからステートを取得
	ID3D11SamplerState* sampler_states[] = {
		states->GetSamplerState(0).Get(),																								//ポイントサンプラー
		states->GetSamplerState(1).Get(),																								//リニアサンプラー
		states->GetSamplerState(2).Get()																								//異方性サンプラー
	};

	//メッシュの巡回とワンドローコールの実行
	const GltfModelData::node& root_node = data.nodes.at(data.scenes.at(data.default_scene).nodes.at(0));
	if (root_node.mesh > -1)
	{
		const GltfModelData::mesh& mesh = data.meshes.at(root_node.mesh);
		for (const GltfModelData::mesh::primitive& primitive : mesh.primitives)
		{
			//頂点情報の各バッファポインタを属性ごとに配列化
			ID3D11Buffer* vertex_buffers[]
			{
				primitive.has("POSITION") ? data.buffers.at(primitive.vertex_buffer_views.at("POSITION").buffer).Get() : nullptr,
				primitive.has("NORMAL") ? data.buffers.at(primitive.vertex_buffer_views.at("NORMAL").buffer).Get() : nullptr,
				primitive.has("TANGENT") ? data.buffers.at(primitive.vertex_buffer_views.at("TANGENT").buffer).Get() : nullptr,
				primitive.has("TEXCOORD_0") ? data.buffers.at(primitive.vertex_buffer_views.at("TEXCOORD_0").buffer).Get() : nullptr,
				primitive.has("JOINTS_0") ? data.buffers.at(primitive.vertex_buffer_views.at("JOINTS_0").buffer).Get() : nullptr,
				primitive.has("WEIGHTS_0") ? data.buffers.at(primitive.vertex_buffer_views.at("WEIGHTS_0").buffer).Get() : nullptr,
			};

			//各頂点バッファの1要素あたりのバイトサイズを配列化
			UINT strides[]
			{
				primitive.has("POSITION") ? static_cast<UINT>(primitive.vertex_buffer_views.at("POSITION").stride_in_bytes) : OFFSET_ZERO,
				primitive.has("NORMAL") ? static_cast<UINT>(primitive.vertex_buffer_views.at("NORMAL").stride_in_bytes) : OFFSET_ZERO,
				primitive.has("TANGENT") ? static_cast<UINT>(primitive.vertex_buffer_views.at("TANGENT").stride_in_bytes) : OFFSET_ZERO,
				primitive.has("TEXCOORD_0") ? static_cast<UINT>(primitive.vertex_buffer_views.at("TEXCOORD_0").stride_in_bytes) : OFFSET_ZERO,
				primitive.has("JOINTS_0") ? static_cast<UINT>(primitive.vertex_buffer_views.at("JOINTS_0").stride_in_bytes) : OFFSET_ZERO,
				primitive.has("WEIGHTS_0") ? static_cast<UINT>(primitive.vertex_buffer_views.at("WEIGHTS_0").stride_in_bytes) : OFFSET_ZERO,
			};

			//各頂点バッファの読み込み開始位置を配列化
			UINT offsets[]
			{
				primitive.has("POSITION") ? static_cast<UINT>(primitive.vertex_buffer_views.at("POSITION").byte_offset) : OFFSET_ZERO,
				primitive.has("NORMAL") ? static_cast<UINT>(primitive.vertex_buffer_views.at("NORMAL").byte_offset) : OFFSET_ZERO,
				primitive.has("TANGENT") ? static_cast<UINT>(primitive.vertex_buffer_views.at("TANGENT").byte_offset) : OFFSET_ZERO,
				primitive.has("TEXCOORD_0") ? static_cast<UINT>(primitive.vertex_buffer_views.at("TEXCOORD_0").byte_offset) : OFFSET_ZERO,
				primitive.has("JOINTS_0") ? static_cast<UINT>(primitive.vertex_buffer_views.at("JOINTS_0").byte_offset) : OFFSET_ZERO,
				primitive.has("WEIGHTS_0") ? static_cast<UINT>(primitive.vertex_buffer_views.at("WEIGHTS_0").byte_offset) : OFFSET_ZERO,
			};

			immediate_context->IASetVertexBuffers(SHADER_SLOT_0, _countof(vertex_buffers), vertex_buffers, strides, offsets); immediate_context->IASetVertexBuffers(SHADER_SLOT_0, _countof(vertex_buffers), vertex_buffers, strides, offsets);

			//プリミティブごとの定数データ更新
			GltfModelData::primitive_constants primitive_data = {};															
			primitive_data.material = primitive.material;																	
			primitive_data.has_tangent = primitive.has("TANGENT") ? 1 : 0;													
			primitive_data.skin = root_node.skin;																			

			immediate_context->UpdateSubresource(primitive_cbuffer.Get(), OFFSET_ZERO, nullptr, &primitive_data, OFFSET_ZERO, OFFSET_ZERO);	
			immediate_context->VSSetConstantBuffers(SHADER_SLOT_0, RESOURCE_COUNT_1, primitive_cbuffer.GetAddressOf());						
			immediate_context->PSSetConstantBuffers(SHADER_SLOT_0, RESOURCE_COUNT_1, primitive_cbuffer.GetAddressOf());						

			//テクスチャリソースのバインド
			const GltfModelData::material& material = data.materials.at(primitive.material);								
			const int texture_indices[]																						
			{
				material.data.pbr_metallic_roughness.basecolor_texture.index,												
				material.data.pbr_metallic_roughness.metallic_roughness_texture.index,										
				material.data.normal_texture.index,																			
				material.data.emissive_texture.index,																		
				material.data.occlusion_texture.index,																		
			};

			std::vector<ID3D11ShaderResourceView*> shader_resource_views(_countof(texture_indices));						
			for (int texture_index = 0; texture_index < shader_resource_views.size(); texture_index++)						
			{
				shader_resource_views.at(texture_index) = texture_indices[texture_index] > -1 ?								
					data.texture_resource_views.at(data.textures.at(texture_indices[texture_index]).source).Get() : nullptr;
			}
			immediate_context->PSSetShaderResources(SHADER_SLOT_1, static_cast<UINT>(shader_resource_views.size()), shader_resource_views.data());

			ID3D11SamplerState* sampler_p0 = sampler_states[0];																
			ID3D11SamplerState* sampler_p1 = sampler_states[1];																
			ID3D11SamplerState* sampler_p2 = sampler_states[2];																

			immediate_context->PSSetSamplers(0, 1, &sampler_p0);															
			immediate_context->PSSetSamplers(1, 1, &sampler_p1);															
			immediate_context->PSSetSamplers(2, 1, &sampler_p2);

			//実際の描画命令の発行
			if (primitive.index_buffer_view.buffer > -1)																	
			{
				immediate_context->IASetIndexBuffer(data.buffers.at(primitive.index_buffer_view.buffer).Get(), primitive.index_buffer_view.format, static_cast<UINT>(primitive.index_buffer_view.byte_offset));	
				immediate_context->DrawIndexedInstanced(static_cast<UINT>(primitive.index_buffer_view.count), static_cast<UINT>(instances.size()), OFFSET_ZERO, OFFSET_ZERO, OFFSET_ZERO);						
			}
			else
			{
				immediate_context->DrawInstanced(static_cast<UINT>(primitive.vertex_buffer_views.at("POSITION").count), static_cast<UINT>(instances.size()), OFFSET_ZERO, OFFSET_ZERO);							
			}
		}
	}
}

//ボーン情報を格納する大容量バッファの作成
void GltfInstancingRenderer::InitializeBoneBuffer(ID3D11Device* device)
{
	//構造化バッファの設定と作成
	D3D11_BUFFER_DESC buffer_desc = {};										
	buffer_desc.ByteWidth = sizeof(DirectX::XMFLOAT4X4) * MAX_TOTAL_BONES;	
	buffer_desc.Usage = D3D11_USAGE_DYNAMIC;								
	buffer_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;						
	buffer_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;					
	buffer_desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;			
	buffer_desc.StructureByteStride = sizeof(DirectX::XMFLOAT4X4);

	HRESULT hr = device->CreateBuffer(&buffer_desc, nullptr, bone_matrix_buffer.ReleaseAndGetAddressOf());
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

	//シェーダーリソースビュー(SRV)の作成
	D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc = {};							
	srv_desc.Format = DXGI_FORMAT_UNKNOWN;									
	srv_desc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;					
	srv_desc.Buffer.NumElements = MAX_TOTAL_BONES;

	hr = device->CreateShaderResourceView(bone_matrix_buffer.Get(), &srv_desc, bone_matrix_srv.ReleaseAndGetAddressOf());
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));
}

//ボーン配列を一括更新してGPUにバインド
void GltfInstancingRenderer::UpdateAndBindBoneBuffer(ID3D11DeviceContext* immediate_context, const std::vector<DirectX::XMFLOAT4X4>& all_bone_matrices)
{
	//ボーン行列のGPUメモリへの転送とセット
	if (all_bone_matrices.empty()) return;

	D3D11_MAPPED_SUBRESOURCE mapped_resource;
	HRESULT hr = immediate_context->Map(bone_matrix_buffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_resource);

	if (SUCCEEDED(hr))
	{
		memcpy(mapped_resource.pData, all_bone_matrices.data(), sizeof(DirectX::XMFLOAT4X4) * all_bone_matrices.size());
		immediate_context->Unmap(bone_matrix_buffer.Get(), 0);
	}
	immediate_context->VSSetShaderResources(BONE_BUFFER_SLOT, 1, bone_matrix_srv.GetAddressOf());
}
