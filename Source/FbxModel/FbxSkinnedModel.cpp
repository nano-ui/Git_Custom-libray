#include "FbxSkinnedModel.h"

#include <algorithm>
#include <cmath>

#include "../Graphics/shader.h"
#include "../Common/misc.h"
#include "../Graphics/shader.h"
#include "../Graphics/Graphics.h"

namespace
{
	//--------------------------------------------------------------------
	// ヘルパー関数: 2つのキーフレームを補間して「ローカル行列」を計算する
	//--------------------------------------------------------------------
	// curr: 現在のフレームの姿勢データ
	// next: 次のフレームの姿勢データ
	// t:    補間係数 (0.0 ～ 1.0)
	//--------------------------------------------------------------------
	DirectX::XMMATRIX ComputeInterpolatedLocalMatrix(
		const AnimationKeyframeNode& curr,
		const AnimationKeyframeNode& next,
		float t)
	{
		// 1. スケールの線形補間 (Lerp)
		DirectX::XMVECTOR S_curr = XMLoadFloat3(&curr.scaling);
		DirectX::XMVECTOR S_next = XMLoadFloat3(&next.scaling);
		DirectX::XMVECTOR S = DirectX::XMVectorLerp(S_curr, S_next, t);

		// 2. 回転の球面線形補間 (Slerp)
		// クォータニオンを使うことで滑らかな回転を実現します
		DirectX::XMVECTOR R_curr = XMLoadFloat4(&curr.rotation);
		DirectX::XMVECTOR R_next = XMLoadFloat4(&next.rotation);
		DirectX::XMVECTOR R = DirectX::XMQuaternionSlerp(R_curr, R_next, t);

		// 3. 位置の線形補間 (Lerp)
		DirectX::XMVECTOR T_curr = XMLoadFloat3(&curr.translation);
		DirectX::XMVECTOR T_next = XMLoadFloat3(&next.translation);
		DirectX::XMVECTOR T = DirectX::XMVectorLerp(T_curr, T_next, t);

		// 4. アフィン変換行列の作成 ( Scale * Rotation * Translation )
		return DirectX::XMMatrixAffineTransformation(S, DirectX::XMVectorZero(), R, T);
	}
}


//===================
//コンストラクタ
//===================
FbxSkinnedModel::FbxSkinnedModel(std::shared_ptr<FbxSkinnedResource> resource)
	:resource(resource)
{
	//インプットレイアウトの定義
	D3D11_INPUT_ELEMENT_DESC input_element_desc[] =
	{
		// 名前      インデックス  フォーマット                  スロット  オフセット                    クラス                       インスタンスステップ
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }, // 位置
		{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }, // 法線
		{ "TANGENT",  0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }, // 接線
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }, // UV座標
		{ "WEIGHTS",  0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }, // ボーンウェイト
		{ "BONES",    0, DXGI_FORMAT_R32G32B32A32_UINT,  0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }, // ボーンインデックス(Uint)
	};

	//頂点シェーダーの作成
	create_vs_from_cso(
		resource->GetDevice(),
		"skinned_mesh_vs.cso",
		vertex_shader.ReleaseAndGetAddressOf(),
		input_layout.ReleaseAndGetAddressOf(),
		input_element_desc,
		ARRAYSIZE(input_element_desc)
	);

	//ピクセルシェーダーの作成
	create_ps_from_cso(
		resource->GetDevice(),
		"skinned_mesh_ps.cso",
		pixel_shader.ReleaseAndGetAddressOf()
	);

	//定数バッファ作成
	D3D11_BUFFER_DESC buffer_desc{};
	buffer_desc.ByteWidth = sizeof(Constants);
	buffer_desc.Usage = D3D11_USAGE_DEFAULT;
	buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	buffer_desc.CPUAccessFlags = 0;
	resource->GetDevice()->CreateBuffer(&buffer_desc, nullptr, constant_buffer.GetAddressOf());
}

//================
//更新処理
//================
void FbxSkinnedModel::AnimationUpdate(float delta_time)
{
	if (!resource)return;

	//ノードリストへの参照
	const auto& nodes = resource->nodes;
	const auto& animations = resource->GetAnimations();
	const auto& bones = resource->GetBones();

	//行列配列のサイズ確保
	if (node_global_transforms.size() != nodes.size())
	{
		node_global_transforms.resize(nodes.size());
	}

	//ボーン用行列（スキニング用）のサイズ確保
	if (bone_transforms.size() != bones.size())
	{
		bone_transforms.resize(bones.size());
	}

	//-----------------------------------
	//アニメーション時間の進行
	//-----------------------------------

	auto it = animations.find(current_clip_name);
	bool has_animation = (it != animations.end());
	const AnimationClip* clip = nullptr;

	//キーフレーム計算用変数
	size_t frame_curr = 0;
	size_t frame_next = 0;
	float t = 0.0f;

	if (has_animation)
	{
		clip = &it->second;

		//時間を進める
		current_time += delta_time * clip->sampling_rate;
		float duration = static_cast<float>(clip->sequence.size() - 1);

		//ループ処理
		if (is_loop) current_time = fmod(current_time, duration);
		else         current_time = (std::min)(current_time, duration);

		//フレーム番号の算出
		frame_curr = static_cast<size_t>(current_time);
		frame_next = frame_curr + 1;
		if (frame_next >= clip->sequence.size())
		{
			frame_next = is_loop ? 0 : clip->sequence.size() - 1;
		}

		//補完係数
		t = current_time - static_cast<float>(frame_curr);
	}

	//-------------------------------
	//ノード階層の更新
	//-------------------------------

	for (size_t i = 0; i < nodes.size(); i++)
	{
		const auto& node = nodes[i];
		DirectX::XMMATRIX local_matrix;

		if (has_animation)
		{
			//アニメーションがある場合：キーフレームから補間してローカル行列を作る
			const auto& node_anim_curr = clip->sequence[frame_curr].nodes[i];
			const auto& node_anim_next = clip->sequence[frame_next].nodes[i];

			local_matrix = ComputeInterpolatedLocalMatrix(node_anim_curr, node_anim_next, t);
		}
		else
		{
			//アニメーションがない場合：初期姿勢（Bind Pose）を使用
			local_matrix = XMLoadFloat4x4(&node.local_transform);
		}

		//親の行列を適用
		DirectX::XMMATRIX global_matrix = local_matrix;
		if (node.parent_index >= 0)
		{
			//親は必ず自分より前のインデックスにいるので、計算済みの行列を取得できる
			DirectX::XMMATRIX parent_global = DirectX::XMLoadFloat4x4(&node_global_transforms[node.parent_index]);
			global_matrix = global_matrix * parent_global;
		}

		//計算結果を保存
		DirectX::XMStoreFloat4x4(&node_global_transforms[i], global_matrix);
	}

	//----------------------
	//ボーン行列の更新
	//----------------------

	//計算済みのノード行列を使って、スキニングに必要な行列を作成
	for (size_t i = 0; i < bones.size(); i++)
	{
		//ボーンが参照しているノードのインデックスを取得
		int64_t node_index = bones[i].node_index;

		if (node_index >= 0 && node_index < node_global_transforms.size())
		{
			DirectX::XMMATRIX global = XMLoadFloat4x4(&node_global_transforms[node_index]);
			DirectX::XMMATRIX offset = XMLoadFloat4x4(&bones[i].offset_transform);

			//スキニング行列 = Offset * Global
			XMStoreFloat4x4(&bone_transforms[i], offset * global);
		}
	}
}

//================
//描画処理
//================
void FbxSkinnedModel::Render(
	ID3D11DeviceContext* context,
	const DirectX::XMFLOAT4X4& world, 
	const DirectX::XMFLOAT4& color)
{
	if (!resource)return;

	//基本となるワールド行列
	DirectX::XMMATRIX base_world = DirectX::XMLoadFloat4x4(&world);

	// -------------------------------------------------
	// 定数バッファデータの準備
	// -------------------------------------------------
	Constants data;
	data.material_color = color;

	//計算済みのボーン行列をコピー
	size_t bone_count = (std::min)(bone_transforms.size(), static_cast<size_t>(256));
	for (size_t i = 0; i < bone_count; i++)
	{
		data.bone_transforms[i] = bone_transforms[i];
	}

	// ------------------
	// パイプライン設定
	// ------------------
	context->VSSetShader(vertex_shader.Get(), nullptr, 0);
	context->PSSetShader(pixel_shader.Get(), nullptr, 0);

	PipelineStates* pipeline_states = Graphics::Instance().GetPipelineStates();
	context->OMSetDepthStencilState(pipeline_states->GetDepthStenceilState(1).Get(), 0);
	float blend_factor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	context->OMSetBlendState(pipeline_states->GetBlendState(0).Get(), blend_factor, 0xFFFFFFFF);
	context->RSSetState(pipeline_states->GetRasterizerState(2).Get());

	context->VSSetConstantBuffers(0, 1, constant_buffer.GetAddressOf());
	context->PSSetConstantBuffers(0, 1, constant_buffer.GetAddressOf());
	context->IASetInputLayout(input_layout.Get());
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// ----------------------
	// メッシュごとの描画
	// ----------------------
	const auto& meshes = resource->GetMeshes();
	const auto& materials = resource->GetMaterials();

	for (const auto& mesh : meshes)
	{
		//メッシュごとの行列計算
		DirectX::XMMATRIX mesh_local_transform = DirectX::XMMatrixIdentity();

		if (mesh.node_index >= 0 && mesh.node_index < node_global_transforms.size())
		{
			mesh_local_transform = DirectX::XMLoadFloat4x4(&node_global_transforms[mesh.node_index]);
		}

		//最終的なワールド行列 = メッシュのノード行列 × モデル全体の行列
		DirectX::XMStoreFloat4x4(&data.world, mesh_local_transform * base_world);

		//定数バッファの更新
		context->UpdateSubresource(constant_buffer.Get(), 0, nullptr, &data, 0, 0);

		//頂点バッファとインデックスバッファのセット
		UINT stride = sizeof(MeshVertex);
		UINT offset = 0;
		context->IASetVertexBuffers(0, 1, mesh.vertex_buffer.GetAddressOf(), &stride, &offset);
		context->IASetIndexBuffer(mesh.index_buffer.Get(), DXGI_FORMAT_R32_UINT, 0);

		//サブセット(マテリアル)ごとの描画
		for (const auto& subset : mesh.subsets)
		{
			//マテリアルIDからテクスチャを検索して適用
			auto it = materials.find(subset.material_unique_id);
			if (it != materials.end())
			{
				const auto& mat = it->second;

				//テクスチャ配列を作成
				ID3D11ShaderResourceView* srvs[] = {
					mat.shader_resource_views[0].Get(),
					mat.shader_resource_views[1].Get() 
				};

				//シェーダーリソースとしてセット
				context->PSSetShaderResources(0, 2, srvs);
			}

			//インデックス描画
			context->DrawIndexed(subset.index_count, subset.start_index_location, 0);
		}
	}
}

//===========================
//アニメーション更新処理
//===========================
void FbxSkinnedModel::PlayAnimation(
	const std::string& clip_name,
	bool loop)
{
	if (current_clip_name != clip_name)
	{
		current_clip_name = clip_name;
		current_time = 0.0f;
		is_loop = loop;
	}
}
