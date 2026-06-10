#include "GltfModelRenderer.h"
#include <misc.h>
#include "../Graphics/shader.h"
#include "../Graphics/Graphics.h"

//=========================================
//デバイスを受け取り描画リソースを初期化
//=========================================
GltfModelRenderer::GltfModelRenderer(ID3D11Device* device)
{
	//--------------------------------------------------
	// シェーダーへの入力レイアウトの定義
	//--------------------------------------------------
	D3D11_INPUT_ELEMENT_DESC input_element_desc[]
	{
		{ "POSITION", OFFSET_ZERO, DXGI_FORMAT_R32G32B32_FLOAT,    SHADER_SLOT_0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, OFFSET_ZERO },	// 頂点座標の定義（スロット0）
		{ "NORMAL"  , OFFSET_ZERO, DXGI_FORMAT_R32G32B32_FLOAT,    SHADER_SLOT_1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, OFFSET_ZERO },	// 法線ベクトルの定義（スロット1）
		{ "TANGENT" , OFFSET_ZERO, DXGI_FORMAT_R32G32B32A32_FLOAT, SHADER_SLOT_2, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, OFFSET_ZERO },	// 接線ベクトルの定義（スロット2）
		{ "TEXCOORD", OFFSET_ZERO, DXGI_FORMAT_R32G32_FLOAT,       3,             D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, OFFSET_ZERO },	// テクスチャ座標の定義（スロット3）
		{ "JOINTS"  , OFFSET_ZERO, DXGI_FORMAT_R16G16B16A16_UINT,  4,             D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, OFFSET_ZERO },	// ボーンインデックスの定義（16ビット指定、スロット4）
		{ "WEIGHTS" , OFFSET_ZERO, DXGI_FORMAT_R32G32B32A32_FLOAT, 5,             D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, OFFSET_ZERO }	// スキンウェイトの定義（スロット5）
	};

	//--------------------------------------------------------
	//シェーダーオブジェクトの生成と手動入力レイアウトの適用
	//--------------------------------------------------------
	custom_shader = std::make_unique<CustomShader>();
	bool is_success = custom_shader->Initialize("gltf_model_vs.cso", "gltf_model_ps.cso", input_element_desc, _countof(input_element_desc));
	_ASSERT_EXPR(is_success, L"Failed to initialize custom shader");

	//--------------------------------------------------
	// GPUへ送るための定数バッファの生成
	//--------------------------------------------------
	D3D11_BUFFER_DESC buffer_desc = {};												//バッファ作成用の設定構造体
	HRESULT hr;

	buffer_desc.ByteWidth = sizeof(GltfModelData::primitive_constants);				//プリミティブ用構造体のサイズを設定
	buffer_desc.Usage = D3D11_USAGE_DEFAULT;										//GPUから読み書き可能なデフォルト設定
	buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;								//定数バッファとしてGPUにバインドすることを指定
	hr = device->CreateBuffer(&buffer_desc, nullptr, primitive_cbuffer.ReleaseAndGetAddressOf());
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));										//作成に失敗した場合はプログラムを停止し原因を出力

	buffer_desc.ByteWidth = sizeof(GltfModelData::primitive_joint_constants);		//スキニング用の関節行列構造体のサイズを設定
	hr = device->CreateBuffer(&buffer_desc, nullptr, primitive_joint_cbuffer.ReleaseAndGetAddressOf());
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));
}

//===============================
//GPUに命令を送りモデルを描画
//===============================
void GltfModelRenderer::Render(ID3D11DeviceContext* immediate_context, const GltfModelData& data, const DirectX::XMFLOAT4X4& world, const std::vector<GltfModelData::node>& animated_nodes)
{
	//描画対象のノード配列の設定
	const std::vector<GltfModelData::node>& nodes_to_render = animated_nodes.empty() ? data.nodes : animated_nodes;

	//パイプラインステートと共通リソースの設定
	immediate_context->PSSetShaderResources(SHADER_SLOT_0, RESOURCE_COUNT_1, data.material_resource_view.GetAddressOf());	//全マテリアル情報を格納した構造化バッファをピクセルシェーダーにセット
	custom_shader->Apply();
	immediate_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);	//頂点を3つずつ結んで三角形を描画するモード（トポロジー）に設定

	auto states = Graphics::Instance().GetPipelineStates();
	ID3D11SamplerState* sampler_states[SAMPLER_COUNT_3] = {
		states->GetSamplerState(0).Get(),
		states->GetSamplerState(1).Get(),
		states->GetSamplerState(2).Get()
	};

	immediate_context->PSSetSamplers(OFFSET_ZERO, SAMPLER_COUNT_3, sampler_states);

	RenderFlatLoop(immediate_context, data, nodes_to_render, world);
}

//1次元配列によるフラットループ描画
void GltfModelRenderer::RenderFlatLoop(ID3D11DeviceContext* immediate_context, const GltfModelData& data, const std::vector<GltfModelData::node>& nodes_to_render, const DirectX::XMFLOAT4X4& world)
{
	using namespace DirectX;

	//メッシュを保持するフラットなインデックス配列でループ
	for (int node_index : data.node_update_order)
	{
		const GltfModelData::node& current_node = nodes_to_render.at(node_index);

		if (current_node.skin <= -1 && current_node.mesh <= -1)
		{
			continue;
		}

		//メッシュの描画ブロック
		if (current_node.mesh > -1)
		{
			DirectX::XMFLOAT4X4 final_world_matrix;
			XMStoreFloat4x4(&final_world_matrix, XMLoadFloat4x4(&current_node.global_transform) * XMLoadFloat4x4(&world));

			if (current_node.skin > -1)
			{
				const GltfModelData::skin& skin = data.skins.at(current_node.skin);
				GltfModelData::primitive_joint_constants primitive_joint_data = {};

				for (size_t joint_index = OFFSET_ZERO; joint_index < skin.joints.size(); ++joint_index)
				{
					XMStoreFloat4x4(&primitive_joint_data.matrices[joint_index],
						XMLoadFloat4x4(&skin.inverse_bind_matrices.at(joint_index)) *
						XMLoadFloat4x4(&nodes_to_render.at(skin.joints.at(joint_index)).global_transform) *
						XMMatrixInverse(nullptr, XMLoadFloat4x4(&current_node.global_transform))
					);
				}

				immediate_context->UpdateSubresource(primitive_joint_cbuffer.Get(), OFFSET_ZERO, nullptr, &primitive_joint_data, OFFSET_ZERO, OFFSET_ZERO);
				immediate_context->VSSetConstantBuffers(SHADER_SLOT_2, RESOURCE_COUNT_1, primitive_joint_cbuffer.GetAddressOf());
			}
			else
			{

			}

			//各プリミティブの実際の描画ループ
			const GltfModelData::mesh& mesh = data.meshes.at(current_node.mesh);
			for (const GltfModelData::mesh::primitive& primitive : mesh.primitives)
			{
				ID3D11Buffer* vertex_buffers[] =
				{
					primitive.has("POSITION") ? data.buffers.at(primitive.vertex_buffer_views.at("POSITION").buffer).Get() : nullptr,
					primitive.has("NORMAL") ? data.buffers.at(primitive.vertex_buffer_views.at("NORMAL").buffer).Get() : nullptr,
					primitive.has("TANGENT") ? data.buffers.at(primitive.vertex_buffer_views.at("TANGENT").buffer).Get() : nullptr,
					primitive.has("TEXCOORD_0") ? data.buffers.at(primitive.vertex_buffer_views.at("TEXCOORD_0").buffer).Get() : nullptr,
					primitive.has("JOINTS_0") ? data.buffers.at(primitive.vertex_buffer_views.at("JOINTS_0").buffer).Get() : nullptr,
					primitive.has("WEIGHTS_0") ? data.buffers.at(primitive.vertex_buffer_views.at("WEIGHTS_0").buffer).Get() : nullptr,
				};

				UINT strides[] =
				{
					primitive.has("POSITION") ? static_cast<UINT>(primitive.vertex_buffer_views.at("POSITION").stride_in_bytes) : OFFSET_ZERO,
					primitive.has("NORMAL") ? static_cast<UINT>(primitive.vertex_buffer_views.at("NORMAL").stride_in_bytes) : OFFSET_ZERO,
					primitive.has("TANGENT") ? static_cast<UINT>(primitive.vertex_buffer_views.at("TANGENT").stride_in_bytes) : OFFSET_ZERO,
					primitive.has("TEXCOORD_0") ? static_cast<UINT>(primitive.vertex_buffer_views.at("TEXCOORD_0").stride_in_bytes) : OFFSET_ZERO,
					primitive.has("JOINTS_0") ? static_cast<UINT>(primitive.vertex_buffer_views.at("JOINTS_0").stride_in_bytes) : OFFSET_ZERO,
					primitive.has("WEIGHTS_0") ? static_cast<UINT>(primitive.vertex_buffer_views.at("WEIGHTS_0").stride_in_bytes) : OFFSET_ZERO,
				};

				UINT offsets[] =
				{
					primitive.has("POSITION") ? static_cast<UINT>(primitive.vertex_buffer_views.at("POSITION").byte_offset) : OFFSET_ZERO,
					primitive.has("NORMAL") ? static_cast<UINT>(primitive.vertex_buffer_views.at("NORMAL").byte_offset) : OFFSET_ZERO,
					primitive.has("TANGENT") ? static_cast<UINT>(primitive.vertex_buffer_views.at("TANGENT").byte_offset) : OFFSET_ZERO,
					primitive.has("TEXCOORD_0") ? static_cast<UINT>(primitive.vertex_buffer_views.at("TEXCOORD_0").byte_offset) : OFFSET_ZERO,
					primitive.has("JOINTS_0") ? static_cast<UINT>(primitive.vertex_buffer_views.at("JOINTS_0").byte_offset) : OFFSET_ZERO,
					primitive.has("WEIGHTS_0") ? static_cast<UINT>(primitive.vertex_buffer_views.at("WEIGHTS_0").byte_offset) : OFFSET_ZERO,
				};

				immediate_context->IASetVertexBuffers(SHADER_SLOT_0, _countof(vertex_buffers), vertex_buffers, strides, offsets);

				GltfModelData::primitive_constants primitive_data = {};
				primitive_data.material = primitive.material;
				primitive_data.has_tangent = primitive.has("TANGENT") ? 1 : 0;
				primitive_data.skin = current_node.skin; // スキンを持たないパーツの時は、シェーダー側に正確に -1 が渡ります
				primitive_data.world = final_world_matrix;

				immediate_context->UpdateSubresource(primitive_cbuffer.Get(), OFFSET_ZERO, nullptr, &primitive_data, OFFSET_ZERO, OFFSET_ZERO);
				immediate_context->VSSetConstantBuffers(SHADER_SLOT_0, RESOURCE_COUNT_1, primitive_cbuffer.GetAddressOf());
				immediate_context->PSSetConstantBuffers(SHADER_SLOT_0, RESOURCE_COUNT_1, primitive_cbuffer.GetAddressOf());

				const GltfModelData::material& material = data.materials.at(primitive.material);
				const int texture_indices[] =
				{
					material.data.pbr_metallic_roughness.basecolor_texture.index,
					material.data.pbr_metallic_roughness.metallic_roughness_texture.index,
					material.data.normal_texture.index,
					material.data.emissive_texture.index,
					material.data.occlusion_texture.index,
				};

				ID3D11ShaderResourceView* shader_resource_views[TEXTURE_COUNT_5];
				for (int texture_index = OFFSET_ZERO; texture_index < TEXTURE_COUNT_5; ++texture_index)
				{
					shader_resource_views[texture_index] = texture_indices[texture_index] > -1 ?
						data.texture_resource_views.at(data.textures.at(texture_indices[texture_index]).source).Get() : nullptr;
				}
				immediate_context->PSSetShaderResources(SHADER_SLOT_1, TEXTURE_COUNT_5, shader_resource_views);

				if (primitive.index_buffer_view.buffer > -1)
				{
					immediate_context->IASetIndexBuffer(data.buffers.at(primitive.index_buffer_view.buffer).Get(),
						primitive.index_buffer_view.format, static_cast<UINT>(primitive.index_buffer_view.byte_offset));
					immediate_context->DrawIndexed(static_cast<UINT>(primitive.index_buffer_view.count), OFFSET_ZERO, OFFSET_ZERO);
				}
				else
				{
					immediate_context->Draw(static_cast<UINT>(primitive.vertex_buffer_views.at("POSITION").count), OFFSET_ZERO);
				}
			}
		}
	}
}