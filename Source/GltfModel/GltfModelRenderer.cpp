#include "GltfModelRenderer.h"
#include <misc.h>
#include "../Graphics/shader.h"

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
		{ "POSITION", OFFSET_ZERO, DXGI_FORMAT_R32G32B32_FLOAT,    SHADER_SLOT_0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, OFFSET_ZERO },	// 頂点座標の定義
		{ "NORMAL"  , OFFSET_ZERO, DXGI_FORMAT_R32G32B32_FLOAT,    SHADER_SLOT_1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, OFFSET_ZERO },	// 法線ベクトルの定義
		{ "TANGENT" , OFFSET_ZERO, DXGI_FORMAT_R32G32B32A32_FLOAT, SHADER_SLOT_2, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, OFFSET_ZERO },	// 接線ベクトルの定義
		{ "TEXCOORD", OFFSET_ZERO, DXGI_FORMAT_R32G32_FLOAT,       3,             D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, OFFSET_ZERO },	// テクスチャ座標の定義
		{ "JOINTS"  , OFFSET_ZERO, DXGI_FORMAT_R16G16B16A16_UINT,  4,             D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, OFFSET_ZERO },	// ボーンインデックスの定義
		{ "WEIGHTS" , OFFSET_ZERO, DXGI_FORMAT_R32G32B32A32_FLOAT, 5,             D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, OFFSET_ZERO }	// スキンウェイトの定義
	};

	//--------------------------------------------------
	// シェーダーオブジェクトの生成
	//--------------------------------------------------
	create_vs_from_cso(																
		device,
		"gltf_model_vs.cso",							
		vertex_shader.ReleaseAndGetAddressOf(),
		input_layout.ReleaseAndGetAddressOf(),
		input_element_desc,
		_countof(input_element_desc)
	);

	create_ps_from_cso(																
		device,
		"gltf_model_ps.cso",														
		pixel_shader.ReleaseAndGetAddressOf()
	);

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
	//-----------------------------
	//描画対象のノード配列の設定
	//-----------------------------
	//アニメーションによって更新されたノードがある場合はそちらを使用し、無い場合は初期データを使用
	const std::vector<GltfModelData::node>& nodes_to_render = animated_nodes.empty() ? data.nodes : animated_nodes;

	//------------------------------------------
	//パイプラインステートと共通リソースの設定
	//------------------------------------------
	immediate_context->PSSetShaderResources(SHADER_SLOT_0, RESOURCE_COUNT_1, data.material_resource_view.GetAddressOf());	//全マテリアル情報を格納した構造化バッファをピクセルシェーダーにセット
	immediate_context->VSSetShader(vertex_shader.Get(), nullptr, OFFSET_ZERO);			//初期化時に読み込んだ頂点シェーダーを有効化
	immediate_context->PSSetShader(pixel_shader.Get(), nullptr, OFFSET_ZERO);			//初期化時に読み込んだピクセルシェーダーを有効化
	immediate_context->IASetInputLayout(input_layout.Get());							//頂点データのメモリレイアウトを有効化
	immediate_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);	//頂点を3つずつ結んで三角形を描画するモード（トポロジー）に設定

	//-----------------------------------
	//ルートノードからの描画巡回処理
	//-----------------------------------
	for (int node_index : data.scenes.at(data.default_scene).nodes)
	{
		TraverseNodeForRender(node_index, immediate_context, data, nodes_to_render, world);
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
	const DirectX::XMFLOAT4X4& world
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
			// 逆バインド行列 × ジョイントのグローバル行列 × 自身の逆行列を計算して最終的なスキニング行列を算出
			XMStoreFloat4x4(&primitive_joint_data.matrices[joint_index],
				XMLoadFloat4x4(&skin.inverse_bind_matrices.at(joint_index)) *
				XMLoadFloat4x4(&nodes.at(skin.joints.at(joint_index)).global_transform) *
				XMMatrixInverse(nullptr, XMLoadFloat4x4(&current_node.global_transform))
			);
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

			immediate_context->IASetVertexBuffers(SHADER_SLOT_0, _countof(vertex_buffers), vertex_buffers, strides, offsets); //用意した頂点バッファ群をパイプラインにセット

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
			const GltfModelData::material& material = data.materials.at(primitive.material);								//描画に使用するマテリアル情報を取得
			const int texture_indices[]																						//各種テクスチャのインデックスを配列化
			{
				material.data.pbr_metallic_roughness.basecolor_texture.index,												//ベースカラー（基本色）
				material.data.pbr_metallic_roughness.metallic_roughness_texture.index,										//メタリック・ラフネス（金属感と粗さ）
				material.data.normal_texture.index,																			//ノーマル（法線マップ）
				material.data.emissive_texture.index,																		//エミッシブ（自己発光）
				material.data.occlusion_texture.index,																		//オクルージョン（環境遮蔽）
			};

			std::vector<ID3D11ShaderResourceView*> shader_resource_views(_countof(texture_indices));						//GPUにセットするためのビュー配列を用意
			for (int texture_index = 0; texture_index < shader_resource_views.size(); texture_index++)						//用意した全テクスチャに対してループ
			{
				shader_resource_views.at(texture_index) = texture_indices[texture_index] > -1 ?								//インデックスが有効な場合
					data.texture_resource_views.at(data.textures.at(texture_indices[texture_index]).source).Get() : nullptr;//実体のリソースビューを取得、無効ならnullptr
			}
			immediate_context->PSSetShaderResources(SHADER_SLOT_1, static_cast<UINT>(shader_resource_views.size()), shader_resource_views.data()); //全テクスチャを一括でピクセルシェーダーの1番スロットからセット

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
				immediate_context->Draw(static_cast<UINT>(primitive.vertex_buffer_views.at("POSITION").count), OFFSET_ZERO);	//インデックスが無い場合は頂点数に基づいて直接描画を実行
			}
		}
	}

	//--------------------------------------------------
	// 子ノードに対する再帰呼び出し
	//--------------------------------------------------
	for (int child_index : current_node.children)																			// 描画が完了したノードの全ての子ノードをループ
	{
		TraverseNodeForRender(child_index, immediate_context, data, nodes, world);											// 子ノードのインデックスを渡し自身を再帰呼び出し
	}
}
