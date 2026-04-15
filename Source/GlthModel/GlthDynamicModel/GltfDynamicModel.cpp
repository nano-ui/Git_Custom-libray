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
	for (auto& bone : bones)
	{
		bone.SetLocalMatrix(bone.GetIntialLocalMatrix());
	}

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
		if (!is_finished)
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

	DirectX::XMMATRIX root_fix = DirectX::XMMatrixIdentity();

	//ルートボーン（親がいない骨）から順番に計算を開始
	for (int i = 0; i < static_cast<int>(bones.size()); i++)
	{
		//：親がいないボーン（ルート）のみを起点として計算を開始
		if (bones[i].GetParentIndex() == -1)
		{
			//ルートボーンとして、単位行列を親の行列として渡して開始
			UpdateArmature(i, root_fix);
		}
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

//=====================
//再帰的に骨格を更新
//=====================
void GltfDynamicModel::UpdateArmature(int bone_index, const DirectX::XMMATRIX& parent_matrix)
{
	//-----------------------
	//ワールド行列を確定
	//-----------------------

	auto& bone = bones[bone_index];	//処理対象のボーンを参照
	DirectX::XMMATRIX local = DirectX::XMLoadFloat4x4(&bone.GetLocalMatrix());	//自身のローカルポーズ取得
	DirectX::XMMATRIX global = local * parent_matrix;	//自身の行列　= ローカル行列 * 親のグローバル行列

	DirectX::XMFLOAT4X4 stored_matrix;	//結果保存変数
	DirectX::XMStoreFloat4x4(&stored_matrix, global);	//計算結果を格納
	bone.SetGlobalMatrix(stored_matrix);	//グローバル行列を保存

	//------------------------------------------------------
	//自分を親に持っている「子供たち」を順番に計算させる
	//------------------------------------------------------

	//全リストから探す
	for (int i = 0; i < static_cast<int>(bones.size()); i++)
	{
		//自身の番号を親に持つボーンの確認
		if (bones[i].GetParentIndex() == bone_index)
		{
			UpdateArmature(i, global);
		}
	}
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
	dc->PSSetConstantBuffers(0, 1, object_cb.GetAddressOf());

	//--------------------
	//メッシュの描画
	//--------------------

	//全メッシュをループ
	for (const auto& mesh : resource->GetMeshes())
	{
		mesh->Render(dc);
	}
}

//==========================
//モデルにシェーダーを設定
//==========================
void GltfDynamicModel::SetModelShader(ID3D11VertexShader* vs, ID3D11PixelShader* ps, ID3D11InputLayout* layout)
{
	//----------------------------------------------------
	//モデル内の全メッシュをループしてマテリアルを取得
	//----------------------------------------------------

	for (const auto& mesh : resource->GetMeshes())
	{
		//メッシュが持つマテリアルへのスマートポインタを取得
		std::shared_ptr<GltfDynamicMaterial> material = mesh->GetMaterial();

		//----------------------------------------------------
		//マテリアルに対してシェーダーオブジェクトをセット
		//----------------------------------------------------
		
		//マテリアルが存在するか判定
		if (material)
		{
			//マテリアル内部の変数にシェーダーを保存
			material->SetShader(vs, ps, layout);
		}
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

		//---------------------
		//行列の転置とコピー
		//---------------------
		
		//ワールド行列を転置してコピー
		DirectX::XMStoreFloat4x4(&cb_data->world, DirectX::XMMatrixTranspose(DirectX::XMLoadFloat4x4(&wordl_matrix)));

		//マテリアル色の設定
		cb_data->material_color = color;

		//--------------------------
		//スキニング用行列の計算
		//--------------------------

		const std::vector<int>& joints = resource->GetJoints();
		for (size_t i = 0; i < joints.size() && i < 256; i++)
		{
			int node_index = joints[i];

			DirectX::XMMATRIX offset = DirectX::XMLoadFloat4x4(&bones[node_index].GetOffsetMatrix());
			DirectX::XMMATRIX global = DirectX::XMLoadFloat4x4(&bones[node_index].GetGlobalMatrix());

			//定数バッファの配列に保存
			DirectX::XMStoreFloat4x4(&cb_data->bone_transformes[i], DirectX::XMMatrixTranspose(offset * global));
		}
		//ロックを解除してGPUへ反映
		dc->Unmap(object_cb.Get(), 0);
	}
}
