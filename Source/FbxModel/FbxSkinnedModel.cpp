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

	const auto& animations = resource->GetAnimations();
	const auto& bones = resource->GetBones();

	//ボーン行列配列のサイズ確保
	if (bone_transforms.size() != bones.size())
	{
		bone_transforms.resize(bones.size());
		global_transforms.resize(bones.size());

		for (auto& m : bone_transforms)
		{
			DirectX::XMStoreFloat4x4(&m, DirectX::XMMatrixIdentity());
		}
	}

	//指定したアニメーションがあるか検索
	auto it = animations.find(current_clip_name);
	if (it == animations.end())
	{
		//アニメーションが無い場合は更新しない
		return;
	}

	const AnimationClip& clip = it->second;

	//時間を進める
	current_time += delta_time * clip.sampling_rate;

	//総フレーム数
	float duration = static_cast<float>(clip.sequence.size() - 1);

	//ループ制御
	if (is_loop)
	{
		current_time = fmod(current_time, duration);
	}
	else
	{
		current_time = (std::min)(current_time, duration);
	}

	//現在と次のキーフレーム番号を計算
	size_t frame_curr = static_cast<size_t>(current_time);
	size_t frame_next = static_cast<size_t>(current_time);

	//次のフレームが範囲外ならループ、または停止
	if (frame_next >= clip.sequence.size())
	{
		frame_next = is_loop ? 0 : clip.sequence.size() - 1;
	}

	//補完係数
	float t = current_time - static_cast<float>(frame_curr);

	const auto& nodes_curr = clip.sequence[frame_curr];
	const auto& nodes_next = clip.sequence[frame_next];

	//ボーン行列の計算ループ
	for (size_t i = 0; i < bones.size(); i++)
	{
		//ヘルパー関数を使ってローカル行列を計算
		DirectX::XMMATRIX local = ComputeInterpolatedLocalMatrix(nodes_curr[i], nodes_next[i], t);

		//親子関係を解決してグローバル行列にする
		int64_t parent_index = bones[i].parent_index;
		DirectX::XMMATRIX global;

		if (parent_index >= 0)
		{
			//親のグローバル行列を取得して掛ける
			DirectX::XMMATRIX parent_global = DirectX::XMLoadFloat4x4(&global_transforms[parent_index]);
			global = local * parent_global;
		}
		else
		{
			//親がいなければローカルがそのままグローバル
			global = local;
		}

		//次の子ボーン計算のために純粋なグローバル行列を保存
		DirectX::XMStoreFloat4x4(&global_transforms[i], global);

		//「逆バインドポーズ行列（Offset）」を掛けて「スキニング行列」を完成させる
		DirectX::XMMATRIX offset = DirectX::XMLoadFloat4x4(&bones[i].offset_transform);

		//頂点を「ボーン空間」に戻してから(Offset)、現在の姿勢(Global)へ動かす
		DirectX::XMStoreFloat4x4(&bone_transforms[i], offset * global);
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

	//----------------------
	//定数バッファの更新
	//----------------------
	Constants data;
	data.world = world;
	data.material_color = color;

	//計算済みのボーン行列をコピー
	size_t bone_count = (std::min)(bone_transforms.size(), static_cast<size_t>(256));
	for (size_t i = 0; i < bone_count; i++)
	{
		data.bone_transforms[i] = bone_transforms[i];
	}
	context->UpdateSubresource(constant_buffer.Get(), 0, nullptr, &data, 0, 0);

	//------------------
	//パイプライン設定
	//------------------
	context->VSSetShader(vertex_shader.Get(), nullptr, 0);
	context->PSSetShader(pixel_shader.Get(), nullptr, 0);

	// Graphicsクラスからパイプラインステートを取得
	PipelineStates* pipeline_states = Graphics::Instance().GetPipelineStates();

	// 深度ステンシルステートの設定
	// Index 1: 深度テスト有効、深度書き込み有効 (通常描画用)
	context->OMSetDepthStencilState(pipeline_states->GetDepthStenceilState(1).Get(), 0);

	// ブレンドステートの設定
	// Index 0: ブレンド無効 (不透明描画用)
	float blend_factor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	context->OMSetBlendState(pipeline_states->GetBlendState(0).Get(), blend_factor, 0xFFFFFFFF);

	//カリングなし
	context->RSSetState(pipeline_states->GetRasterizerState(2).Get());

	context->VSSetConstantBuffers(0, 1, constant_buffer.GetAddressOf());
	context->PSSetConstantBuffers(0, 1, constant_buffer.GetAddressOf());
	context->IASetInputLayout(input_layout.Get());
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	//----------------------
	//メッシュごとの描画
	//----------------------
	const auto& meshes = resource->GetMeshes();
	const auto& materials = resource->GetMaterials();

	for (const auto& mesh : meshes)
	{
		//頂点バッファとインデックスバッファのセット
		UINT stride = sizeof(MeshVertex);
		UINT offset = 0;
		context->IASetVertexBuffers(0, 1, mesh.vertex_buffer.GetAddressOf(), &stride, &offset);
		context->IASetIndexBuffer(mesh.index_buffer.Get(), DXGI_FORMAT_R32_UINT, 0);

		//サブセット(マテリアル)ごとの描画
		for (const auto& subset : mesh.subsets)
		{
			//描画に使用するマテリアルへのポインタ
			const MaterialData* use_material = nullptr;

			//マテリアルIDから検索
			auto it = materials.find(subset.material_unique_id);
			if (it != materials.end())
			{
				use_material = &it->second;
			}
			else
			{
				//見つからない場合はデフォルトマテリアルを使用
				if (!materials.empty())
				{
					use_material = &materials.begin()->second;
				}
			}

			//マテリアル（またはダミー）が確定した場合
			if (use_material)
			{
				//テクスチャ配列を作成
				ID3D11ShaderResourceView* srvs[] = {
					use_material->shader_resource_views[0].Get(),	//デフォルト
					use_material->shader_resource_views[1].Get()	//ノーマル
				};

				//シェーダーリソースとしてセット
				context->PSSetShaderResources(0, 2, srvs);
			}
			else
			{
				//マテリアルが1つも読み込まれていない場合の保険
				ID3D11ShaderResourceView* null_srvs[] = { nullptr, nullptr };
				context->PSSetShaderResources(0, 2, null_srvs);
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
