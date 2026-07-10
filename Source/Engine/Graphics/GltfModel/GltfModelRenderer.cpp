#include "GltfModelRenderer.h"
#include <../Engine/Core/misc.h>
#include "../Engine/Graphics/Shaders/shader.h"
#include "../Engine/Graphics/Graphics.h"
#include "../Engine/Graphics/GltfModel/GltfModel.h"

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

// GPUに命令を送りモデルを描画
void GltfModelRenderer::Render
(
	ID3D11DeviceContext* immediate_context,
	const GltfModel& model,
	const DirectX::XMFLOAT4X4& world,
	ID3D11SamplerState* const* sampler_states
)
{
	const GltfModelData& data = *model.GetData();	//モデルの実体情報
	const std::vector<GltfModelData::node>& nodes = model.GetAnimatedNodes();	//アニメーション計算済みのノード配列

	if (data.scenes.empty()) //シーン情報が空であるかの条件分岐
	{
		return;
	}

	std::vector<RenderCommand> render_commands; //描画コマンドを蓄積する配列

	for (size_t i = 0; i < nodes.size(); i++) //全ノードを走査するループ
	{
		const GltfModelData::node& current_node = nodes.at(i); //現在のノード情報

		if (current_node.mesh > -1) //ノードにメッシュが存在するかの条件分岐
		{
			const GltfModelData::mesh& mesh = data.meshes.at(current_node.mesh); //メッシュデータ

			for (const GltfModelData::mesh::primitive& primitive : mesh.primitives) //プリミティブを走査するループ
			{
				RenderCommand cmd = {}; //描画コマンドの一時格納用

				DirectX::XMStoreFloat4x4(&cmd.world_matrix,
					DirectX::XMLoadFloat4x4(&current_node.global_transform) *
					DirectX::XMLoadFloat4x4(&world));

				cmd.primitive = &primitive;
				cmd.skin_index = current_node.skin;
				cmd.node_global_matrix = current_node.global_transform;

				render_commands.push_back(cmd);
			}
		}
	}

	if (render_commands.empty()) //描画コマンドが空であるかの条件分岐
	{
		OutputDebugStringA("[GltfModelRenderer Error] Render: Completely failed to find any meshes!\n");
		return;
	}

	if (custom_shader) //カスタムシェーダーが有効であるかの条件分岐
	{
		custom_shader->Apply();
	}

	std::sort(render_commands.begin(), render_commands.end(), CompartMaterial());

	int last_material_id = -1; //前回のマテリアルIDを保持するキャッシュ

	for (const RenderCommand& cmd : render_commands) //ソート済みの描画コマンドを走査するループ
	{
		using namespace DirectX;
		const GltfModelData::mesh::primitive* primitive = cmd.primitive;

		if (cmd.skin_index > -1 && static_cast<size_t>(cmd.skin_index) < data.skins.size()) //スキニング処理を行うかの条件分岐
		{
			const GltfModelData::skin& skin = data.skins.at(cmd.skin_index);
			GltfModelData::primitive_joint_constants primitive_joint_data = {};

			for (size_t joint_index = 0; joint_index < skin.joints.size() && joint_index < 512; joint_index++) //ジョイントごとに行列を計算するループ
			{
				DirectX::XMMATRIX inv_bind = DirectX::XMLoadFloat4x4(&skin.inverse_bind_matrices.at(joint_index));
				DirectX::XMMATRIX joint_global = DirectX::XMLoadFloat4x4(&nodes.at(skin.joints.at(joint_index)).global_transform);
				DirectX::XMVECTOR determinant = {};
				DirectX::XMMATRIX mesh_inv = DirectX::XMMatrixInverse(&determinant, DirectX::XMLoadFloat4x4(&cmd.node_global_matrix));

				if (DirectX::XMVector3Less(DirectX::XMVectorAbs(determinant), DirectX::XMVectorReplicate(1e-6f))) //逆行列が計算不可能であるかの条件分岐
				{
					mesh_inv = DirectX::XMMatrixIdentity();
				}

				DirectX::XMMATRIX skin_matrix = inv_bind * joint_global * mesh_inv; // プロジェクトの累積ルールに合せて列優先の順序に復元

				DirectX::XMStoreFloat4x4(&primitive_joint_data.matrices[joint_index], skin_matrix);
			}

			immediate_context->UpdateSubresource(primitive_joint_cbuffer.Get(), OFFSET_ZERO, nullptr, &primitive_joint_data, OFFSET_ZERO, OFFSET_ZERO);
			immediate_context->VSSetConstantBuffers(SHADER_SLOT_2, RESOURCE_COUNT_1, primitive_joint_cbuffer.GetAddressOf());
		}

		{
			GltfModelData::primitive_constants primitive_data = {}; //GPU転送用のプリミティブ定数構造体
			primitive_data.material = primitive->material;
			primitive_data.has_tangent = primitive->has("TANGENT") ? 1 : 0;
			primitive_data.skin = cmd.skin_index;
			primitive_data.world = cmd.world_matrix; // 正しいワールド行列を設定（Identityによる上書き破壊コードを削除）

			immediate_context->UpdateSubresource(primitive_cbuffer.Get(), OFFSET_ZERO, nullptr, &primitive_data, OFFSET_ZERO, OFFSET_ZERO);
			immediate_context->VSSetConstantBuffers(SHADER_SLOT_0, RESOURCE_COUNT_1, primitive_cbuffer.GetAddressOf());
			immediate_context->PSSetConstantBuffers(SHADER_SLOT_0, RESOURCE_COUNT_1, primitive_cbuffer.GetAddressOf());
		}

		if (primitive->material != last_material_id) //マテリアルが切り替わったかの条件分岐
		{
			if (data.material_resource_view) //マテリアルリソースビューが有効であるかの条件分岐
			{
				immediate_context->PSSetShaderResources(0, RESOURCE_COUNT_1, data.material_resource_view.GetAddressOf());
			}

			ID3D11ShaderResourceView* shader_resource_views[TEXTURE_COUNT_5] = { nullptr }; //テクスチャビューの配列

			if (primitive->material >= 0 && static_cast<size_t>(primitive->material) < data.materials.size()) //有効なマテリアル番号であるかの条件分岐
			{
				const GltfModelData::material& material = data.materials.at(primitive->material); //マテリアルデータ
				const int texture_indices[] = //テクスチャインデックスの配列
				{
					material.data.pbr_metallic_roughness.basecolor_texture.index,
					material.data.pbr_metallic_roughness.metallic_roughness_texture.index,
					material.data.normal_texture.index,
					material.data.emissive_texture.index,
					material.data.occlusion_texture.index
				};

				for (int texture_index = 0; texture_index < TEXTURE_COUNT_5; texture_index++) //5つのテクスチャスロットを巡回するループ
				{
					int tex_idx = texture_indices[texture_index]; //テクスチャ番号
					if (tex_idx >= 0 && static_cast<size_t>(tex_idx) < data.textures.size()) //有効なテクスチャ番号であるかの条件分岐
					{
						int source_idx = data.textures.at(tex_idx).source; //画像ソース番号
						if (source_idx >= 0 && static_cast<size_t>(source_idx) < data.texture_resource_views.size()) //有効なリソースビューであるかの条件分岐
						{
							shader_resource_views[texture_index] = data.texture_resource_views.at(source_idx).Get();
						}
					}
				}
			}

			immediate_context->PSSetShaderResources(SHADER_SLOT_1, TEXTURE_COUNT_5, shader_resource_views);

			auto states = Graphics::Instance().GetPipelineStates(); //パイプラインステートマネージャーのポインタ
			if (states) //ステートが有効であるかの条件分岐
			{
				ID3D11SamplerState* samplers[SAMPLER_COUNT_3] = { //サンプラーステートの配列
					states->GetSamplerState(0).Get(),
					states->GetSamplerState(1).Get(),
					states->GetSamplerState(2).Get()
				};
				immediate_context->PSSetSamplers(OFFSET_ZERO, SAMPLER_COUNT_3, samplers);
			}
			last_material_id = primitive->material;
		}

		ID3D11Buffer* vertex_buffers[6] = { nullptr }; //頂点バッファのバインド用配列
		UINT strides[6] = { OFFSET_ZERO }; //ストライド値の配列
		UINT offsets[6] = { OFFSET_ZERO }; //オフセット値の配列

		const char* attribute_names[] = { "POSITION", "NORMAL", "TANGENT", "TEXCOORD_0", "JOINTS_0", "WEIGHTS_0" }; //属性名の配列

		for (int i = 0; i < 6; ++i) //6つの頂点属性を巡回するループ
		{
			const char* attr = attribute_names[i]; //対象 of 属性名
			if (primitive->has(attr)) //属性を保持しているかの条件分岐
			{
				auto it = primitive->vertex_buffer_views.find(attr); //バッファビューのイテレータ
				if (it != primitive->vertex_buffer_views.end()) //イテレータが有効であるかの条件分岐
				{
					int buffer_idx = it->second.buffer; //バッファ番号
					if (buffer_idx >= 0 && static_cast<size_t>(buffer_idx) < data.buffers.size()) //有効なバッファ番号であるかの条件分岐
					{
						vertex_buffers[i] = data.buffers.at(buffer_idx).Get();
						strides[i] = static_cast<UINT>(it->second.stride_in_bytes);
						offsets[i] = static_cast<UINT>(it->second.byte_offset);
					}
				}
			}
		}

		immediate_context->IASetVertexBuffers(SHADER_SLOT_0, _countof(vertex_buffers), vertex_buffers, strides, offsets);

		if (primitive->index_buffer_view.buffer > -1) //インデックスバッファが存在するかの条件分岐
		{
			immediate_context->IASetIndexBuffer(data.buffers.at(primitive->index_buffer_view.buffer).Get(),
				primitive->index_buffer_view.format, static_cast<UINT>(primitive->index_buffer_view.byte_offset));
			immediate_context->DrawIndexed(static_cast<UINT>(primitive->index_buffer_view.count), OFFSET_ZERO, OFFSET_ZERO);
		}
		else //インデックスバッファが存在しない場合の条件分岐
		{
			auto pos_it = primitive->vertex_buffer_views.find("POSITION"); //座標属性のイテレータ
			if (pos_it != primitive->vertex_buffer_views.end()) //座標属性が存在するかの条件分岐
			{
				immediate_context->Draw(static_cast<UINT>(pos_it->second.count), OFFSET_ZERO);
			}
		}
	}
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

			XMStoreFloat4x4(&primitive_joint_data.matrices[joint_index], skin_matrix);
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
