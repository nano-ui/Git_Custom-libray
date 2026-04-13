#include "GltfDynamicModel.h"

//=================================================
//コンストラクタ
//=================================================
GltfDynamicModel::GltfDynamicModel(ID3D11Device* device, std::shared_ptr<GltfDynamicModelResource> resource_)
{
	//----------------------------
	//データの初期化
	//----------------------------
	resource = resource_;//共有のリソースを保持
	bones = resource->GetBones();	//ボーン階層をコピー

	//ワールド行列を単位行列で初期化
	DirectX::XMStoreFloat4x4(&wordl_matrix, DirectX::XMMatrixIdentity());

	//----------------------
	//再生時間の初期化
	//----------------------

	animation_time = 0.0f;
	is_loop = false;
	is_finished = false;
	current_animation_index = 0;

	//---------------------------
	//定数バッファの作成
	//---------------------------
	D3D11_BUFFER_DESC cb_desc = {};
	cb_desc.ByteWidth = sizeof(ObjectConstantBuffer);	//サイズ指定
	cb_desc.Usage = D3D11_USAGE_DYNAMIC;				//毎フレーム更新するため動的に設定
	cb_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;		//バッファとして作成
	cb_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;	//CPUからの書き込み許可

	//デバイスを使用して定数バッファを作成
	device->CreateBuffer(&cb_desc, nullptr, object_cb.GetAddressOf());
}

//=================================================
//アニメーション更新処理
//=================================================
void GltfDynamicModel::Update(float delta_time)
{
	//----------------------
	//共有リソースの取得
	//----------------------

	const auto& animations = resource->GetAnimation();	//アニメーションリスト取得

	//-----------------------------
	//再生時間の更新とループ判定
	//-----------------------------

	//インデックスが有効な場合のみ更新
	if (current_animation_index >= 0 && current_animation_index < static_cast<int>(animations.size()))
	{
		const auto& anim_data = animations[current_animation_index];	//共有データ参照
		float max_len = anim_data.GetMaxDuration();		//アニメーションの長さを取得

		//停止状態でなければ更新
		if (!is_finished && !is_loop)
		{
			//個別のタイマーを進める
			animation_time += delta_time;

			//終端に達した場合
			if (animation_time >= max_len)
			{
				if (is_loop)
				{
					//ループさせる
					animation_time = fmod(animation_time, max_len);
					is_finished = false;
				}
				else
				{
					//停止
					animation_time = max_len;
					is_finished = true;
				}
			}
		}

		//-------------------------
		//アニメーションの適用
		//-------------------------

		//指定されたアニメーションをボーンでローカル行列を更新
		anim_data.Update(animation_time, bones);
	}

	//-------------------------
	//階層構造の計算
	//-------------------------

	//全ボーンをループ
	for (auto& bone : bones)
	{
		//ローカル行列を取得
		DirectX::XMMATRIX global_matrix = DirectX::XMLoadFloat4x4(&bone.GetLocalMatrix());

		int parent_index = bone.GetParentIndex();//親のインデックスを取得
		//親が存在する場合
		if (parent_index != -1)
		{
			//親のグローバル行列をかけて自分のグローバル座標を計算
			global_matrix *= DirectX::XMLoadFloat4x4(&bones[parent_index].GetGlobalMatrix());
		}

		DirectX::XMFLOAT4X4 result;	//結果を格納
		DirectX::XMStoreFloat4x4(&result, global_matrix);
		bone.SetGlobalMatrix(result);
	}
}

//===============================================
//アニメーション切り替え処理
//===============================================
void GltfDynamicModel::ChangeAnimation(const std::string& name, bool loop)
{
	//-----------------------------------------------
	//共有リソースからアニメーションリストを取得
	//-----------------------------------------------

	const auto& animations = resource->GetAnimation();//リソース内の全アニメーションを参照

	//----------------------------
	//名前が一致するものを検索
	//----------------------------

	//全体をループ
	for (int i = 0; i < static_cast<int>(animations.size()); i++)
	{
		if (animations[i].GetAnimationName() == name)
		{
			//違うモーションの時のみリセット
			if (current_animation_index != i)
			{
				current_animation_index = i;	//インデックスの更新
				animation_time = 0.0f;			//時間のリセット
				is_finished = false;			//終了状態を解除
				is_loop = loop;					//ループするか否か
			}
			return;
		}
	}

	//------------------------------------
	//見つからなかった場合のエラー対策
	//------------------------------------
	current_animation_index = -1;
}

//=================================
//描画処理
//=================================
void GltfDynamicModel::Draw(ID3D11DeviceContext* dc)
{
	//-----------------------
	//定数バッファの更新
	//-----------------------

	//白色のマテリアルカラーで更新
	UpdateConstantBuffer(dc, DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));

	//----------------------
	//シェーダーの設定
	//----------------------
	dc->VSSetConstantBuffers(0, 1, object_cb.GetAddressOf());

	//--------------------
	//メッシュの描画
	//--------------------

	//全メッシュをループ
	for (const auto& mesh : resource->GetMeshes())
	{
		mesh->Render(dc);
	}
}

//=================================================
//定数バッファの更新
//=================================================
void GltfDynamicModel::UpdateConstantBuffer(ID3D11DeviceContext* dc, const DirectX::XMFLOAT4& color)
{
	//--------------------------
	//書き込み用データの準備
	//--------------------------

	D3D11_MAPPED_SUBRESOURCE mapped_res;//マップ用構造体
	//書き込みのためにバッファをロック
	if (SUCCEEDED(dc->Map(object_cb.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_res)))
	{
		//書き込み先のアドレスを取得
		ObjectConstantBuffer* cb_data = static_cast<ObjectConstantBuffer*>(mapped_res.pData);
		//ワールド行列のコピー
		cb_data->world = wordl_matrix;
		//マテリアル色の設定
		cb_data->material_color = color;

		//--------------------------
		//スキニング用行列の計算
		//--------------------------

		for (size_t i = 0; i < bones.size() && i < 256; i++)
		{
			DirectX::XMMATRIX skin_matrix = DirectX::XMLoadFloat4x4(&bones[i].GetOffsetMatrix()) * DirectX::XMLoadFloat4x4(&bones[i].GetGlobalMatrix());

			//定数バッファの配列に保存
			DirectX::XMStoreFloat4x4(&cb_data->bone_transformes[i], skin_matrix);
		}
		//ロックを解除してGPUへ反映
		dc->Unmap(object_cb.Get(), 0);
	}
}
