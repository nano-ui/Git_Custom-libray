#include "GltfAnimation.h"
#include "GltfBone.h"

#include <tiny_gltf.h>
#include <algorithm>

//=========================================
//コンストラクタ
//=========================================
GltfAnimation::GltfAnimation()
{
	max_duration = 0.0f;
}

//=========================================
//アニメーションの初期化
//=========================================
void GltfAnimation::Initialize(const tinygltf::Model& model, const tinygltf::Animation& anim)
{
	//---------------------------
	//基本情報の読み込み
	//---------------------------

	//アニメーション名をコピー
	animation_name = anim.name;
	//メモリを確保
	channels.reserve(anim.channels.size());

	//---------------------------
	//全チャンネルをループして解析
	//---------------------------

	for (const auto& gltf_channel : anim.channels)
	{
		Channel channel = {};
		//対象のノード番号をセット
		channel.target_bone_index = gltf_channel.target_node;
		channel.last_key_index = 0;	//検索ヒントを初期化

		//ターゲット項目の判別
		if (gltf_channel.target_path == "translation")
		{
			//位置移動
			channel.target_type = TargetType::Translation;
		}
		else if (gltf_channel.target_path == "rotation")
		{
			//回転
			channel.target_type = TargetType::Rotation;
		}
		else if (gltf_channel.target_path == "scale")
		{
			//拡大・縮小
			channel.target_type = TargetType::Scale;
		}

		//サンプラーの取得
		const auto& sampler = anim.samplers[gltf_channel.sampler];

		//時間軸データのポインタ取得
		const auto& input_acc = model.accessors[sampler.input];		//アクセッサ情報取得
		const float* time_ptr = reinterpret_cast<const float*>(&model.buffers[model.bufferViews[input_acc.bufferView].buffer].data[model.bufferViews[input_acc.bufferView].byteOffset + input_acc.byteOffset]);//時間の先頭
		//値データのポインタ取得
		const auto& output_acc = model.accessors[sampler.output];	//アクセッサ情報取得
		const float* val_ptr = reinterpret_cast<const float*>(&model.buffers[model.bufferViews[output_acc.bufferView].buffer].data[model.bufferViews[output_acc.bufferView].byteOffset + output_acc.byteOffset]); //値データ先頭

		//キーフレームの構築
		channel.keyframes.reserve(input_acc.count);	//メモリ確保

		for (size_t i = 0; i < input_acc.count; i++)
		{
			//キーフレーム
			Keyframe kf = {};
			kf.time = time_ptr[i];	//時間セット
			if (max_duration < kf.time)
			{
				//最大時間更新
				max_duration = kf.time;
			}
			//回転の場合
			if (channel.target_type == TargetType::Rotation)
			{
				kf.value = { val_ptr[i * 4], val_ptr[i * 4 + 1], val_ptr[i * 4 + 2], val_ptr[i * 4 + 3] };
			}
			//位置・スケールの場合
			else
			{
				kf.value = { val_ptr[i * 3], val_ptr[i * 3 + 1], val_ptr[i * 3 + 2], 0.0f };
			}
			channel.keyframes.push_back(kf);//リストに追加
		}
		//チャンネル登録
		channels.push_back(channel);
	}
}

//=========================================
//アニメーションの更新
//=========================================
void GltfAnimation::Update(float absolute_time, std::vector<GltfBone>& bones)const
{
	//---------------------------
	//全チャンネルの変化をボーンに適用
	//---------------------------

	//登録された全チャンネルをループ
	for (const auto& channel : channels)
	{
		//対象ボーンが無ければスキップ
		if (channel.target_bone_index < 0) continue;

		//--------------------------------------
		//キーフレームが1つしかない場合の処理
		//--------------------------------------

		if (channel.keyframes.size() == 1)
		{
			continue;
		}

		//---------------------------
		//現在の時間に該当するキーフレームを探す
		//---------------------------

		size_t next_index = 0;//次のキーフレームのインデックス

		//キーフレームを二分探索で検索
		auto it = std::upper_bound(channel.keyframes.begin(), channel.keyframes.end(), absolute_time,
			[](float t, const Keyframe& k) { return t < k.time; });

		//次のインデックス
		next_index = std::distance(channel.keyframes.begin(), it);

		//--------------------------
		//インデックスの範囲制限
		//--------------------------

		if (next_index == 0)
		{
			next_index = 1;//最小インデックスを保証
		}
		else if (next_index >= channel.keyframes.size())
		{
			next_index = channel.keyframes.size() - 1;
		}

		size_t prev_index = next_index - 1;	//1つ前を現在のフレームにする

		//---------------------------
		//補完係数を求める
		//---------------------------

		float prev_time = channel.keyframes[prev_index].time;	//前の時間
		float next_time = channel.keyframes[next_index].time;	//次の時間
		float time = (prev_time == next_time) ? 0.0f : (absolute_time - prev_time) / (next_time - prev_time);//0.0～1.0の割合を計算

		//---------------------------
		//値の補完と反映
		//---------------------------

		auto& bone = bones[channel.target_bone_index];	//書き換え対象のボーン参照
		DirectX::XMMATRIX bone_local = DirectX::XMLoadFloat4x4(&bone.GetLocalMatrix());//現在の行列を取得
		DirectX::XMVECTOR v_pos, v_rot, v_scl;//TRS分解用のベクトル
		DirectX::XMMatrixDecompose(&v_scl, &v_rot, &v_pos, bone_local);//行列をTRSに分解

		//---------------------------
		//チャンネルの種類に応じて、分解した成分を書き換える
		//---------------------------

		if (channel.target_type == TargetType::Translation)
		{
			//位置
			v_pos = DirectX::XMVectorLerp(DirectX::XMLoadFloat4(&channel.keyframes[prev_index].value), DirectX::XMLoadFloat4(&channel.keyframes[next_index].value), time);
		}
		else if (channel.target_type == TargetType::Rotation)
		{
			//回転
			v_rot = DirectX::XMQuaternionSlerp(DirectX::XMLoadFloat4(&channel.keyframes[prev_index].value), DirectX::XMLoadFloat4(&channel.keyframes[next_index].value), time);
		}
		else if (channel.target_type == TargetType::Scale)
		{
			//スケール
			v_scl = DirectX::XMVectorLerp(DirectX::XMLoadFloat4(&channel.keyframes[prev_index].value), DirectX::XMLoadFloat4(&channel.keyframes[next_index].value), time);
		}

		//---------------------------
		//行列の再合成と適用
		//---------------------------

		DirectX::XMMATRIX new_local = DirectX::XMMatrixAffineTransformation(v_scl, DirectX::XMQuaternionIdentity(), v_rot, v_pos);//合成
		DirectX::XMFLOAT4X4 stored_matrix;	//保存用
		DirectX::XMStoreFloat4x4(&stored_matrix, new_local);//XMFLOAT4X4に変換
		bone.SetLocalMatrix(stored_matrix);	//ボーンに書き戻す
	}
}
