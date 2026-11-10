#include "GltfModelRenderer.h"
#include <Engine/Core/misc.h>
#include "Engine/Graphics/Shaders/shader.h"
#include "Engine\Graphics\Renderers\Graphics.h"
#include "Engine\Graphics\Resources\GltfModel\GltfModel.h"
#include "Engine/Graphics/Resources/GltfModel/ModelRenderQueue.h"
#include <Engine\Core\MacroTracy.h>

//=========================================
//デバイスを受け取り描画リソースを初期化
//=========================================
GltfModelRenderer::GltfModelRenderer(ID3D11Device* device)
{
	BS_PROFILE_FUNCTION();

	D3D11_INPUT_ELEMENT_DESC input_element_desc[]
	{
		{ "POSITION",     OFFSET_ZERO, DXGI_FORMAT_R32G32B32_FLOAT,    SHADER_SLOT_0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA,   OFFSET_ZERO },
		{ "NORMAL",       OFFSET_ZERO, DXGI_FORMAT_R32G32B32_FLOAT,    SHADER_SLOT_0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA,   OFFSET_ZERO },
		{ "TANGENT",      OFFSET_ZERO, DXGI_FORMAT_R32G32B32A32_FLOAT, SHADER_SLOT_0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA,   OFFSET_ZERO },
		{ "TEXCOORD",     OFFSET_ZERO, DXGI_FORMAT_R32G32_FLOAT,       SHADER_SLOT_0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA,   OFFSET_ZERO },
		{ "JOINTS",       OFFSET_ZERO, DXGI_FORMAT_R16G16B16A16_UINT,  SHADER_SLOT_0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA,   OFFSET_ZERO },
		{ "WEIGHTS",      OFFSET_ZERO, DXGI_FORMAT_R32G32B32A32_FLOAT, SHADER_SLOT_0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA,   OFFSET_ZERO },

		{ "INSTANCE_WORLD", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, INSTANCE_BUFFER_SLOT, 0,  D3D11_INPUT_PER_INSTANCE_DATA, 1 },
		{ "INSTANCE_WORLD", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, INSTANCE_BUFFER_SLOT, 16, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
		{ "INSTANCE_WORLD", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, INSTANCE_BUFFER_SLOT, 32, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
		{ "INSTANCE_WORLD", 3, DXGI_FORMAT_R32G32B32A32_FLOAT, INSTANCE_BUFFER_SLOT, 48, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
		{ "INSTANCE_ANIM",  0, DXGI_FORMAT_R32G32B32A32_FLOAT, INSTANCE_BUFFER_SLOT, 64, D3D11_INPUT_PER_INSTANCE_DATA, 1 }
	};

	custom_shader = std::make_unique<CustomShader>();
	bool is_success = custom_shader->Initialize("gltf_model_vs.cso", "gltf_model_ps.cso", input_element_desc, _countof(input_element_desc));
	_ASSERT_EXPR(is_success, L"Failed to initialize custom shader");

	D3D11_BUFFER_DESC buffer_desc = {};
	HRESULT hr;

	buffer_desc.ByteWidth = sizeof(GltfModelData::primitive_constants);
	buffer_desc.Usage = D3D11_USAGE_DEFAULT;
	buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	hr = device->CreateBuffer(&buffer_desc, nullptr, primitive_cbuffer.ReleaseAndGetAddressOf());
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

	buffer_desc.ByteWidth = sizeof(GltfModelData::primitive_joint_constants);
	hr = device->CreateBuffer(&buffer_desc, nullptr, primitive_joint_cbuffer.ReleaseAndGetAddressOf());
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

	D3D11_BUFFER_DESC inst_desc = {};
	inst_desc.ByteWidth = static_cast<UINT>(sizeof(ModelInstanceData) * max_instance_count);
	inst_desc.Usage = D3D11_USAGE_DYNAMIC;
	inst_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	inst_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	hr = device->CreateBuffer(&inst_desc, nullptr, instance_buffer.ReleaseAndGetAddressOf());
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));
}

// GPUに命令を送りモデルを描画
void GltfModelRenderer::Render
(
	ID3D11DeviceContext* immediate_context,
	const GltfModel& model,
	const DirectX::XMFLOAT4X4& world,
	ID3D11SamplerState* const* sampler_states
)
{
	BS_PROFILE_FUNCTION();

	const GltfModelData& data = *model.GetData();
	const std::vector<GltfModelData::node>& nodes = model.GetAnimatedNodes();

	if (data.scenes.empty())
	{
		return;
	}

	if (data.cached_render_commands.empty())
	{
		OutputDebugStringA("[GltfModelRenderer エラー] Render: 事前構築された描画キャッシュが空です。\n");
		return;
	}

	if (custom_shader)
	{
		custom_shader->Apply();
	}

	int last_skin_id = -1;
	int last_material_id = -1;

	for (const GltfModelData::cached_command& cmd : data.cached_render_commands)
	{
		using namespace DirectX;
		const GltfModelData::node& current_node = nodes.at(cmd.node_index);
		const GltfModelData::mesh::primitive* primitive = &data.meshes.at(cmd.mesh_index).primitives.at(cmd.primitive_index);

		DirectX::XMFLOAT4X4 world_matrix;
		DirectX::XMStoreFloat4x4(&world_matrix, DirectX::XMLoadFloat4x4(&current_node.global_transform) * DirectX::XMLoadFloat4x4(&world));

		int skin_index = current_node.skin;
		if (skin_index > -1 && skin_index != last_skin_id)
		{
			UpdateSkinMatrices(immediate_context, data, nodes, skin_index, current_node.global_transform);
			last_skin_id = skin_index;
		}

		GltfModelData::primitive_constants primitive_data = {};
		primitive_data.material = primitive->material;
		primitive_data.has_tangent = primitive->has("TANGENT") ? 1 : 0;
		primitive_data.skin = skin_index;
		primitive_data.world = world_matrix;

		immediate_context->UpdateSubresource(primitive_cbuffer.Get(), OFFSET_ZERO, nullptr, &primitive_data, OFFSET_ZERO, OFFSET_ZERO);
		immediate_context->VSSetConstantBuffers(SHADER_SLOT_0, RESOURCE_COUNT_1, primitive_cbuffer.GetAddressOf());
		immediate_context->PSSetConstantBuffers(SHADER_SLOT_0, RESOURCE_COUNT_1, primitive_cbuffer.GetAddressOf());

		if (primitive->material != last_material_id)
		{
			if (data.material_resource_view)
			{
				immediate_context->PSSetShaderResources(0, RESOURCE_COUNT_1, data.material_resource_view.GetAddressOf());
			}

			ID3D11ShaderResourceView* shader_resource_views[TEXTURE_COUNT_5] = { nullptr };
			if (primitive->material >= 0 && static_cast<size_t>(primitive->material) < data.materials.size())
			{
				const GltfModelData::material& material = data.materials.at(primitive->material);
				const int texture_indices[] = {
					material.data.pbr_metallic_roughness.basecolor_texture.index,
					material.data.pbr_metallic_roughness.metallic_roughness_texture.index,
					material.data.normal_texture.index,
					material.data.emissive_texture.index,
					material.data.occlusion_texture.index
				};

				for (int texture_index = 0; texture_index < TEXTURE_COUNT_5; texture_index++)
				{
					int tex_idx = texture_indices[texture_index];
					if (tex_idx >= 0 && static_cast<size_t>(tex_idx) < data.textures.size())
					{
						int source_idx = data.textures.at(tex_idx).source;
						if (source_idx >= 0 && static_cast<size_t>(source_idx) < data.texture_resource_views.size())
						{
							shader_resource_views[texture_index] = data.texture_resource_views.at(source_idx).Get();
						}
					}
				}
			}

			immediate_context->PSSetShaderResources(SHADER_SLOT_1, TEXTURE_COUNT_5, shader_resource_views);

			auto states = Graphics::Instance().GetPipelineStates();
			if (states)
			{
				ID3D11SamplerState* samplers[SAMPLER_COUNT_3] = {
					states->GetSamplerState(0).Get(),
					states->GetSamplerState(1).Get(),
					states->GetSamplerState(2).Get()
				};
				immediate_context->PSSetSamplers(OFFSET_ZERO, SAMPLER_COUNT_3, samplers);
			}

			last_material_id = primitive->material;
		}

		if (primitive->vertex_buffer_view.buffer >= 0 && static_cast<size_t>(primitive->vertex_buffer_view.buffer) < data.buffers.size())
		{
			ID3D11Buffer* vbs[2] = {
				data.buffers.at(primitive->vertex_buffer_view.buffer).Get(),
				instance_buffer.Get()
			};
			UINT strides[2] = {
				static_cast<UINT>(primitive->vertex_buffer_view.stride_in_bytes),
				static_cast<UINT>(sizeof(ModelInstanceData))
			};
			UINT offsets[2] = {
				static_cast<UINT>(primitive->vertex_buffer_view.byte_offset),
				0
			};

			ModelInstanceData single_instance = {};
			single_instance.world_matrix = world_matrix;
			if (data.HasBoneTexture())
			{
				const auto& bake_infos = data.GetAnimationBakeInfos();
				size_t anim_idx = model.GetCurrentAnimationIndex();
				if (anim_idx < bake_infos.size())
				{
					constexpr float BAKE_FRAME_RATE = 30.0f;
					float anim_time = model.GetAnimationCurrentTime();
					float duration = bake_infos[anim_idx].duration;
					float clamped_time = (std::max)(0.0f, (std::min)(anim_time, duration));
					float current_frame = std::floor(clamped_time * BAKE_FRAME_RATE);
					single_instance.anim_row_offset = static_cast<float>(bake_infos[anim_idx].start_frame) + current_frame;
					single_instance.current_anim_time = anim_time;
					single_instance.anim_duration = duration;
				}
			}

			D3D11_MAPPED_SUBRESOURCE mapped = {};
			HRESULT hr = immediate_context->Map(instance_buffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
			if (SUCCEEDED(hr))
			{
				std::memcpy(mapped.pData, &single_instance, sizeof(ModelInstanceData));
				immediate_context->Unmap(instance_buffer.Get(), 0);
			}

			immediate_context->IASetVertexBuffers(0, 2, vbs, strides, offsets);

			if (data.HasBoneTexture())
			{
				ID3D11ShaderResourceView* bone_srv = data.GetBoneTextureSRV();
				immediate_context->VSSetShaderResources(SHADER_SLOT_3, RESOURCE_COUNT_1, &bone_srv);
			}
		}
		else
		{
			OutputDebugStringA("[GltfModelRenderer エラー] Render: 頂点バッファのインデックスが無効です。\n");
		}

		if (primitive->index_buffer_view.buffer > -1)
		{
			immediate_context->IASetIndexBuffer(
				data.buffers.at(primitive->index_buffer_view.buffer).Get(),
				primitive->index_buffer_view.format,
				static_cast<UINT>(primitive->index_buffer_view.byte_offset));

			immediate_context->DrawIndexed(static_cast<UINT>(primitive->index_buffer_view.count), OFFSET_ZERO, OFFSET_ZERO);
		}
		else
		{
			immediate_context->Draw(static_cast<UINT>(primitive->vertex_buffer_view.count), OFFSET_ZERO);
		}
	}
}

//レンダーキューに蓄積されたすべての描画コマンドを一括走査
void GltfModelRenderer::RenderQueue(ID3D11DeviceContext* immediate_context, const ModelRenderQueue& queue)
{
	BS_PROFILE_FUNCTION();

	if (!immediate_context)
	{
		OutputDebugStringA("[GltfModelRenderer エラー] RenderQueue: immediate_context が nullptr です。\n");
		return;
	}

	const auto& commands = queue.GetCommands();
	if (commands.empty())
	{
		return;
	}

	if (!custom_shader)
	{
		OutputDebugStringA("[GltfModelRenderer エラー] RenderQueue: custom_shader が nullptr です。\n");
		return;
	}

	custom_shader->Apply();

	immediate_context->VSSetConstantBuffers(SHADER_SLOT_0, RESOURCE_COUNT_1, primitive_cbuffer.GetAddressOf());
	immediate_context->PSSetConstantBuffers(SHADER_SLOT_0, RESOURCE_COUNT_1, primitive_cbuffer.GetAddressOf());

	std::vector<ModelInstanceData> batched_instances;
	batched_instances.reserve(commands.size());

	const GltfModelData* current_model_data = nullptr;
	const GltfModelData::mesh::primitive* current_primitive = nullptr;
	int current_material_id = -1;

	auto FlushBatch = [&]()
		{
			if (batched_instances.empty() || !current_model_data || !current_primitive)
			{
				batched_instances.clear();
				return;
			}

			D3D11_MAPPED_SUBRESOURCE mapped = {};
			HRESULT hr = immediate_context->Map(instance_buffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
			if (SUCCEEDED(hr))
			{
				size_t copy_count = (std::min)(batched_instances.size(), max_instance_count);
				std::memcpy(mapped.pData, batched_instances.data(), sizeof(ModelInstanceData) * copy_count);
				immediate_context->Unmap(instance_buffer.Get(), 0);

				ID3D11Buffer* vbs[2] = {
					current_model_data->buffers.at(current_primitive->vertex_buffer_view.buffer).Get(),
					instance_buffer.Get()
				};
				UINT strides[2] = {
					static_cast<UINT>(current_primitive->vertex_buffer_view.stride_in_bytes),
					static_cast<UINT>(sizeof(ModelInstanceData))
				};
				UINT offsets[2] = {
					static_cast<UINT>(current_primitive->vertex_buffer_view.byte_offset),
					0
				};
				immediate_context->IASetVertexBuffers(0, 2, vbs, strides, offsets);

				if (current_model_data->HasBoneTexture())
				{
					ID3D11ShaderResourceView* bone_srv = current_model_data->GetBoneTextureSRV();
					immediate_context->VSSetShaderResources(SHADER_SLOT_3, RESOURCE_COUNT_1, &bone_srv);
				}

				if (current_primitive->index_buffer_view.buffer > -1)
				{
					immediate_context->IASetIndexBuffer(
						current_model_data->buffers.at(current_primitive->index_buffer_view.buffer).Get(),
						current_primitive->index_buffer_view.format,
						static_cast<UINT>(current_primitive->index_buffer_view.byte_offset));

					immediate_context->DrawIndexedInstanced(
						static_cast<UINT>(current_primitive->index_buffer_view.count),
						static_cast<UINT>(copy_count),
						0, 0, 0);
				}
				else
				{
					immediate_context->DrawInstanced(
						static_cast<UINT>(current_primitive->vertex_buffer_view.count),
						static_cast<UINT>(copy_count),
						0, 0);
				}
			}
			batched_instances.clear();
		};

	for (const ModelDrawCommand& cmd : commands)
	{
		if (!cmd.data || !cmd.primitive)
		{
			continue;
		}

		if (cmd.data != current_model_data || cmd.primitive != current_primitive || cmd.material_index != current_material_id)
		{
			FlushBatch();

			current_model_data = cmd.data;
			current_primitive = cmd.primitive;
			current_material_id = cmd.material_index;

			GltfModelData::primitive_constants primitive_data = {};
			primitive_data.material = cmd.primitive->material;
			primitive_data.has_tangent = cmd.primitive->has("TANGENT") ? 1 : 0;
			primitive_data.skin = cmd.skin_index;
			DirectX::XMStoreFloat4x4(&primitive_data.world, DirectX::XMMatrixIdentity());
			immediate_context->UpdateSubresource(primitive_cbuffer.Get(), OFFSET_ZERO, nullptr, &primitive_data, OFFSET_ZERO, OFFSET_ZERO);

			if (cmd.data->material_resource_view)
			{
				immediate_context->PSSetShaderResources(OFFSET_ZERO, RESOURCE_COUNT_1, cmd.data->material_resource_view.GetAddressOf());
			}

			ID3D11ShaderResourceView* srvs[TEXTURE_COUNT_5] = { nullptr };
			if (cmd.material_index >= 0 && static_cast<size_t>(cmd.material_index) < cmd.data->materials.size())
			{
				const GltfModelData::material& material = cmd.data->materials.at(cmd.material_index);
				const int tex_indices[] = {
					material.data.pbr_metallic_roughness.basecolor_texture.index,
					material.data.pbr_metallic_roughness.metallic_roughness_texture.index,
					material.data.normal_texture.index,
					material.data.emissive_texture.index,
					material.data.occlusion_texture.index
				};

				for (int t = 0; t < static_cast<int>(TEXTURE_COUNT_5); ++t)
				{
					int idx = tex_indices[t];
					if (idx >= 0 && static_cast<size_t>(idx) < cmd.data->textures.size())
					{
						int src_idx = cmd.data->textures.at(idx).source;
						if (src_idx >= 0 && static_cast<size_t>(src_idx) < cmd.data->texture_resource_views.size())
						{
							srvs[t] = cmd.data->texture_resource_views.at(src_idx).Get();
						}
					}
				}
			}
			immediate_context->PSSetShaderResources(SHADER_SLOT_1, TEXTURE_COUNT_5, srvs);

			auto states = Graphics::Instance().GetPipelineStates();
			if (states)
			{
				ID3D11SamplerState* samplers[SAMPLER_COUNT_3] = {
					states->GetSamplerState(0).Get(),
					states->GetSamplerState(1).Get(),
					states->GetSamplerState(2).Get()
				};
				immediate_context->PSSetSamplers(OFFSET_ZERO, SAMPLER_COUNT_3, samplers);
			}
		}

		ModelInstanceData inst = {};
		inst.world_matrix = cmd.world_matrix;

		constexpr float BAKE_FRAME_RATE = 30.0f;
		float current_frame = 0.0f;

		if (cmd.anim_duration > 0.0f)
		{
			float clamped_time = (std::max)(0.0f, (std::min)(cmd.current_anim_time, cmd.anim_duration));
			current_frame = std::floor(clamped_time * BAKE_FRAME_RATE);

			float max_frames = std::ceil(cmd.anim_duration * BAKE_FRAME_RATE);
			current_frame = (std::min)(current_frame, max_frames);
		}

		inst.anim_row_offset = static_cast<float>(cmd.anim_row_offset) + current_frame;
		inst.current_anim_time = cmd.current_anim_time;
		inst.anim_duration = cmd.anim_duration;

		batched_instances.push_back(inst);
	}

	FlushBatch(); 
}

//==================================
//描画対象のノードを階層順に巡回
//==================================
void GltfModelRenderer::TraverseNodeForRender
(
	int node_index,
	ID3D11DeviceContext* immediate_context,
	const GltfModelData& data,
	const std::vector<GltfModelData::node>& nodes,
	const DirectX::XMFLOAT4X4& world,
	ID3D11SamplerState** sampler_states
)
{
	BS_PROFILE_FUNCTION();

	using namespace DirectX;														// DirectXMathの名前空間を使用
	const GltfModelData::node& current_node = nodes.at(node_index);					// 描画対象のノード情報を参照で取得

	//--------------------------------------------------
	// スキン（ボーン）のアニメーション行列のGPU転送
	//--------------------------------------------------
	if (current_node.skin > -1)														// ノードにスキン情報が関連付けられている場合
	{
		const GltfModelData::skin& skin = data.skins.at(current_node.skin);			// 該当するスキンデータを取得
		GltfModelData::primitive_joint_constants primitive_joint_data = {};			// GPUに送るための行列配列を初期化

		for (size_t joint_index = 0; joint_index < skin.joints.size(); joint_index++)	// スキンに含まれる全ての関節（ジョイント）をループ
		{
			DirectX::XMMATRIX inv_bind = DirectX::XMLoadFloat4x4(&skin.inverse_bind_matrices.at(joint_index));
			DirectX::XMMATRIX joint_global = DirectX::XMLoadFloat4x4(&nodes.at(skin.joints.at(joint_index)).global_transform);
			DirectX::XMMATRIX current_inv = DirectX::XMMatrixInverse(nullptr, DirectX::XMLoadFloat4x4(&current_node.global_transform));

			DirectX::XMMATRIX skin_matrix = inv_bind * joint_global * current_inv;
			DirectX::XMStoreFloat4x4(&primitive_joint_data.matrices[joint_index], skin_matrix);
		}

		immediate_context->UpdateSubresource(primitive_joint_cbuffer.Get(), OFFSET_ZERO, nullptr, &primitive_joint_data, OFFSET_ZERO, OFFSET_ZERO);	// 計算したスキニング行列をGPUバッファに更新
		immediate_context->VSSetConstantBuffers(SHADER_SLOT_2, RESOURCE_COUNT_1, primitive_joint_cbuffer.GetAddressOf());							// 更新した定数バッファを頂点シェーダーのスロット2番にセット
	}

	//--------------------------------------------------
	//メッシュの描画処理
	//--------------------------------------------------
	if (current_node.mesh > -1)														//ノードに描画対象のメッシュが存在する場合
	{
		const GltfModelData::mesh& mesh = data.meshes.at(current_node.mesh);		//該当するメッシュデータを取得

		for (const GltfModelData::mesh::primitive& primitive : mesh.primitives)		//メッシュを構成する最小単位（プリミティブ）を全てループ
		{
			ID3D11Buffer* vertex_buffers[6] = { nullptr };
			UINT strides[6] = { OFFSET_ZERO };
			UINT offsets[6] = { OFFSET_ZERO };

			const char* attribute_names[] = { "POSITION", "NORMAL", "TANGENT", "TEXCOORD_0", "JOINTS_0", "WEIGHTS_0" };

			for (int i = 0; i < 6; ++i)
			{
				const char* attr = attribute_names[i];
				if (primitive.has(attr))
				{
					auto it = primitive.vertex_buffer_views.find(attr);
					if (it != primitive.vertex_buffer_views.end())
					{
						int buffer_idx = it->second.buffer;
						if (buffer_idx >= 0 && static_cast<size_t>(buffer_idx) < data.buffers.size())
						{
							vertex_buffers[i] = data.buffers.at(buffer_idx).Get();
							strides[i] = static_cast<UINT>(it->second.stride_in_bytes);
							offsets[i] = static_cast<UINT>(it->second.byte_offset);
						}
					}
				}
			}

			immediate_context->IASetVertexBuffers(SHADER_SLOT_0, _countof(vertex_buffers), vertex_buffers, strides, offsets);

			//--------------------------------------------------
			// プリミティブごとの定数データ更新
			//--------------------------------------------------
			GltfModelData::primitive_constants primitive_data = {};															//プリミティブ定数用の構造体を初期化
			primitive_data.material = primitive.material;																	//使用するマテリアルのIDを設定
			primitive_data.has_tangent = primitive.has("TANGENT") ? 1 : 0;													//法線マッピング用の接線が存在するかフラグを設定
			primitive_data.skin = current_node.skin;																		//スキニングの有無を設定
			XMStoreFloat4x4(&primitive_data.world, XMLoadFloat4x4(&current_node.global_transform) * XMLoadFloat4x4(&world));//ノードの行列にシーン全体のワールド行列を掛けて最終位置を算出

			immediate_context->UpdateSubresource(primitive_cbuffer.Get(), OFFSET_ZERO, nullptr, &primitive_data, OFFSET_ZERO, OFFSET_ZERO);	//計算した定数データをGPUに転送
			immediate_context->VSSetConstantBuffers(SHADER_SLOT_0, RESOURCE_COUNT_1, primitive_cbuffer.GetAddressOf());						//頂点シェーダーの0番スロットにセット
			immediate_context->PSSetConstantBuffers(SHADER_SLOT_0, RESOURCE_COUNT_1, primitive_cbuffer.GetAddressOf());						//ピクセルシェーダーの0番スロットにセット

			//--------------------------------------------------
			//テクスチャリソースのバインド
			//--------------------------------------------------
			ID3D11ShaderResourceView* shader_resource_views[TEXTURE_COUNT_5] = { nullptr }; //GPUにセットするためのビュー配列を用意

			//マテリアルインデックスが有効な範囲内にあるかチェック
			if (primitive.material >= 0 && static_cast<size_t>(primitive.material) < data.materials.size())
			{
				const GltfModelData::material& material = data.materials.at(primitive.material);								//描画に使用するマテリアル情報を取得
				const int texture_indices[]																						//各種テクスチャのインデックスを配列化
				{
					material.data.pbr_metallic_roughness.basecolor_texture.index,												//ベースカラー（基本色）
					material.data.pbr_metallic_roughness.metallic_roughness_texture.index,										//メタリック・ラフネス（金属感と粗さ）
					material.data.normal_texture.index,																			//ノーマル（法線マップ）
					material.data.emissive_texture.index,																		//エミッシブ（自己発光）
					material.data.occlusion_texture.index,																		//オクルージョン（環境遮蔽）
				};

				for (int texture_index = 0; texture_index < TEXTURE_COUNT_5; texture_index++)						//用意した全テクスチャに対してループ
				{
					int tex_idx = texture_indices[texture_index];
					if (tex_idx >= 0 && static_cast<size_t>(tex_idx) < data.textures.size())
					{
						int source_idx = data.textures.at(tex_idx).source;
						if (source_idx >= 0 && static_cast<size_t>(source_idx) < data.texture_resource_views.size())
						{
							shader_resource_views[texture_index] = data.texture_resource_views.at(source_idx).Get();
						}
					}

					//エラー対策セーフガード：テクスチャが空、あるいはロード失敗時はダミー割り当て等を行う
					if (shader_resource_views[texture_index] == nullptr)
					{
						if (texture_index == 0)
						{
							OutputDebugStringA("[GltfModelRenderer Warning] BaseColor texture bind failed or missing.\n");
						}
					}
				}
			}
			else
			{
				OutputDebugStringA("[GltfModelRenderer Warning] primitive.material index is invalid!\n");
			}

			// 実体のリソースビューを取得、無効ならnullptr			}
			immediate_context->PSSetShaderResources(SHADER_SLOT_1, TEXTURE_COUNT_5, shader_resource_views);

			//--------------------------------------------------
			//実際の描画命令の発行
			//--------------------------------------------------
			if (primitive.index_buffer_view.buffer > -1)																	//インデックスバッファが存在する場合
			{
				immediate_context->IASetIndexBuffer(data.buffers.at(primitive.index_buffer_view.buffer).Get(),
					primitive.index_buffer_view.format, static_cast<UINT>(primitive.index_buffer_view.byte_offset));		//インデックスバッファをパイプラインにセット
				immediate_context->DrawIndexed(static_cast<UINT>(primitive.index_buffer_view.count), OFFSET_ZERO, OFFSET_ZERO);	//インデックス（頂点の結びつき）に基づく描画を実行
			}
			else
			{
				auto pos_it = primitive.vertex_buffer_views.find("POSITION");
				if (pos_it != primitive.vertex_buffer_views.end())
				{
					immediate_context->Draw(static_cast<UINT>(pos_it->second.count), OFFSET_ZERO);	//インデックスが無い場合は頂点数に基づいて直接描画を実行
				}
			}
		}
	}

	//--------------------------------------------------
	// 子ノードに対する再帰呼び出し
	//--------------------------------------------------
	for (int child_index : current_node.children)																			// 描画が完了したノードの全ての子ノードをループ
	{
		TraverseNodeForRender(child_index, immediate_context, data, nodes, world, sampler_states);											// 子ノードのインデックスを渡し自身を再帰呼び出し
	}
}

//描画対象のノードを巡回し、プリミティブ情報をフラットな配列に集める
void GltfModelRenderer::GetherRenderCommands
(
	int node_index,									//現在のノード番号
	const GltfModelData& data,						//実体情報
	const std::vector<GltfModelData::node>& nodes,	//モデル情報
	const DirectX::XMFLOAT4X4& world,				//ワールド行列
	std::vector<RenderCommand>& commands			//描画情報
)
{
	BS_PROFILE_FUNCTION();

	const GltfModelData::node& current_node = nodes.at(node_index);	//現在のモデル情報

	//メッシュが存在する場合、プリミティブ情報を配列に格納
	if (current_node.mesh > -1)
	{
		const GltfModelData::mesh& mesh = data.meshes.at(current_node.mesh);	//現在のメッシュ情報

		for (const GltfModelData::mesh::primitive& primitive : mesh.primitives)
		{
			RenderCommand cmd = {};	//現在のメッシュ描画に必要な情報格納先

			//プリミティブごとの最終的なワールド行列を計算して格納
			DirectX::XMStoreFloat4x4(&cmd.world_matrix,
				DirectX::XMLoadFloat4x4(&current_node.global_transform) *
				DirectX::XMLoadFloat4x4(&world));

			cmd.primitive = &primitive;
			cmd.skin_index = current_node.skin;
			cmd.node_global_matrix = current_node.global_transform;

			commands.push_back(cmd);
		}
	}

	//子ノードに対する再帰呼び出し
	for (int child_index : current_node.children)
	{
		GetherRenderCommands(child_index, data, nodes, world, commands);
	}
}

//スキニング用定数バッファの更新処理
void GltfModelRenderer::UpdateSkinMatrices(
	ID3D11DeviceContext* immediate_context,
	const GltfModelData& data,
	const std::vector<GltfModelData::node>& nodes,
	int skin_index,
	const DirectX::XMFLOAT4X4& node_global_matrix)
{
	BS_PROFILE_FUNCTION();

	if (skin_index < 0 || static_cast<size_t>(skin_index) >= data.skins.size())
	{
		return;
	}

	const GltfModelData::skin& skin = data.skins.at(skin_index);
	GltfModelData::primitive_joint_constants primitive_joint_data = {};

	DirectX::XMVECTOR determinant = {};
	DirectX::XMMATRIX mesh_inv = DirectX::XMMatrixInverse(&determinant, DirectX::XMLoadFloat4x4(&node_global_matrix));

	if (DirectX::XMVector3Less(DirectX::XMVectorAbs(determinant), DirectX::XMVectorReplicate(1e-6f)))
	{
		mesh_inv = DirectX::XMMatrixIdentity();
	}

	constexpr size_t max_joints_limit = 512;
	const size_t joint_count = (std::min)(skin.joints.size(), max_joints_limit);

	for (size_t joint_index = 0; joint_index < joint_count; ++joint_index)
	{
		DirectX::XMMATRIX inv_bind = DirectX::XMLoadFloat4x4(&skin.inverse_bind_matrices.at(joint_index));
		DirectX::XMMATRIX joint_global = DirectX::XMLoadFloat4x4(&nodes.at(skin.joints.at(joint_index)).global_transform);

		DirectX::XMMATRIX skin_matrix = inv_bind * joint_global * mesh_inv;
		DirectX::XMStoreFloat4x4(&primitive_joint_data.matrices[joint_index], skin_matrix);
	}

	immediate_context->UpdateSubresource(primitive_joint_cbuffer.Get(), OFFSET_ZERO, nullptr, &primitive_joint_data, OFFSET_ZERO, OFFSET_ZERO);
	immediate_context->VSSetConstantBuffers(SHADER_SLOT_2, RESOURCE_COUNT_1, primitive_joint_cbuffer.GetAddressOf());
}
