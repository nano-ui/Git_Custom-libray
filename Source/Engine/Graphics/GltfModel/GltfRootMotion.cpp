#include "GltfRootMotion.h"
#include "GltfModelData.h"

#include <windows.h>
#include <algorithm>

//初期化
void GltfRootMotion::Initialize(const std::shared_ptr<const GltfModelData>& data, int root_node_index)
{
	//参照するモデル情報が存在するか確認
	if (!data)
	{
		OutputDebugStringA("[GltfRootMotion Error] Initialize: data is null!\n");
		return;
	}
	model_data = data;
	target_node_index = root_node_index;
	ResetDelta();
}

//アニメーションの進行に合わせてルートノードの差分を計算
void GltfRootMotion::Update(size_t animation_index, float current_time)
{
	DirectX::XMFLOAT3 current_position;	//現在のルートノードの位置
	DirectX::XMFLOAT4 current_rotation;	//現在のルートノードの回転

	ComputeRootPose(animation_index, current_time, current_position, current_rotation);

	//更新化の判定フラグの成否判定
	if (is_first_update)
	{
		previous_position = current_position;
		previous_rotation = current_rotation;
		delta_position = { 0.0f,0.0f,0.0f };
		delta_rotation = { 0.0f,0.0f,0.0f,1.0f };
		is_first_update = false;
	}
	else
	{
		DirectX::XMVECTOR prev_pos = DirectX::XMLoadFloat3(&previous_position);	//前回の位置ベクトル
		DirectX::XMVECTOR curr_pos = DirectX::XMLoadFloat3(&current_position);	//現在の位置ベクトル
		DirectX::XMStoreFloat3(&delta_position, DirectX::XMVectorSubtract(curr_pos, prev_pos));

		DirectX::XMVECTOR prev_rot = DirectX::XMLoadFloat4(&previous_rotation);	//前回の回転クォータニオン
		DirectX::XMVECTOR curr_rot = DirectX::XMLoadFloat4(&current_rotation);	//現在の回転クォータニオン
		DirectX::XMStoreFloat4(&delta_rotation, DirectX::XMQuaternionMultiply(prev_rot, curr_rot));

		previous_position = current_position;
		previous_rotation = current_rotation;
	}
}

//計算された位置の差分を取得
DirectX::XMFLOAT3 GltfRootMotion::GetDeltaPosition() const
{
	return delta_position;
}

//計算された回転の差分を取得
DirectX::XMFLOAT4 GltfRootMotion::GetDeltaRotation() const
{
	return delta_rotation;
}

//アニメーションの切り替えやループ時に、前回の状態を
void GltfRootMotion::ResetDelta()
{
	is_first_update = true;
	delta_position = { 0.0f,0.0f,0.0f };
	delta_rotation = { 0.0f,0.0f,0.0f,1.0f };
}

//任意の時間のルートノードの位置と回転を計算
void GltfRootMotion::ComputeRootPose(size_t animation_index, float time, DirectX::XMFLOAT3& out_position, DirectX::XMFLOAT4& out_rotation) const
{
	//アニメーションの番号が範囲外か判定
	if (animation_index >= model_data->animations.size())
	{
		OutputDebugStringA("[GltfRootMotion Error] ComputeRootPose: animation_index out of range!\n");
		return;
	}

	out_position = model_data->nodes.at(target_node_index).translation;
	out_rotation = model_data->nodes.at(target_node_index).rotation;

	constexpr size_t INDEX_OFFSET_NEXT = 1;			//次の番号への補正
	constexpr size_t INDEX_OFFSET_PREV = 2;			//２つ前の番号への補正
	constexpr float INTERPOLATION_MAX = 1.0f;		//補完係数の最大値
	constexpr float INTERPOLATION_MIN = 0.0f;		//補完係数の最小値
	constexpr size_t START_INDEX = 0;				//開始番号
	constexpr float MIN_TIME_DEFFERENCE = 0.00001f;	//時間差分の最小保証値

	const GltfModelData::animation& animation = model_data->animations.at(animation_index);	//参照するアニメーション情報

	//アニメーションのチャンネルを巡回
	for (const GltfModelData::animation::channel& channel : animation.channels)
	{
		//対象ノードがルートノードと一致するか判定
		if (channel.target_node == target_node_index)
		{
			//サンプラーの番号が範囲外か判定
			if (channel.sampler < START_INDEX || static_cast<size_t>(channel.sampler) >= animation.samplers.size())
			{
				OutputDebugStringA("[GltfRootMotion Error] ComputeRootPose: channel.sampler is out of range!\n");
				continue;
			}
			const GltfModelData::animation::sampler& sampler = animation.samplers.at(channel.sampler);	//参照するサンプラー情報
			auto timeline_it = animation.timelines.find(sampler.input);	//タイムラインのイテレーター

			//タイムラインが見つからない場合
			if (timeline_it == animation.timelines.end())
			{
				OutputDebugStringA("[GltfRootMotion Error] ComputeRootPose: sampler.input key not found!\n");
				continue;
			}
			const std::vector<float>& timeline = timeline_it->second;	//参照するタイムライン

			//タイムラインが空か判定
			if (timeline.empty())
			{
				continue;
			}
			float interpolation_factor = INTERPOLATION_MIN;	//補完用の係数
			size_t keyframe_index = START_INDEX;			//現在のキーフレーム番号
			const size_t keyframe_count = timeline.size();	//キーフレームの総数

			//アニメーション時間が最初のキーフレームより前か判定
			if (time <= timeline.at(START_INDEX))
			{
				keyframe_index = START_INDEX;
				interpolation_factor = INTERPOLATION_MIN;
			}
			//アニメーション時間が最後のキーフレームより後か判定
			else if (time >= timeline.at(keyframe_count - INDEX_OFFSET_NEXT))
			{
				keyframe_index = keyframe_count - INDEX_OFFSET_PREV;
				interpolation_factor = INTERPOLATION_MAX;
			}
			else
			{
				//タイムラインを巡回して現在の時間に該当するキーフレームを検索
				for (size_t i = START_INDEX; i < keyframe_count - INDEX_OFFSET_NEXT; i++)
				{
					//現在の時間がキーフレームの間にあるか判定
					if (time >= timeline.at(i) && time <= timeline.at(i + INDEX_OFFSET_NEXT))
					{
						keyframe_index = i;
						float time_difference = timeline.at(i + INDEX_OFFSET_NEXT) - timeline.at(i);	//時間の差分
						time_difference = std::max<float>(time_difference, MIN_TIME_DEFFERENCE);	
						interpolation_factor = (time - timeline.at(i)) / time_difference;
						break;
					}
				}
			}
			//ターゲットパスが移動か判定
			if (channel.target_path == "translation")
			{
				auto trans_it = animation.translations.find(sampler.output);	//移動情報のイテレーター

				//変換情報が見つからないか、番号が範囲外の場合はエラー
				if (trans_it == animation.translations.end() || (keyframe_index + INDEX_OFFSET_NEXT) >= trans_it->second.size())
				{
					OutputDebugStringA("[GltfRootMotion Error] ComputeRootPose: translations key or index error!\n");
					continue;
				}

				const std::vector<DirectX::XMFLOAT3>& translations = trans_it->second;	//移動情報
				DirectX::XMVECTOR trans_start = DirectX::XMLoadFloat3(&translations.at(keyframe_index));	//開始位置ベクトル
				DirectX::XMVECTOR trans_end = DirectX::XMLoadFloat3(&translations.at(keyframe_index + INDEX_OFFSET_NEXT));	//終了位置ベクトル
				DirectX::XMVECTOR lerped_trans = DirectX::XMVectorLerp(trans_start, trans_end, interpolation_factor);	//線形補完された位置ベクトル
				DirectX::XMStoreFloat3(&out_position, lerped_trans);
			}
			//ターゲットパスが回転か判定
			else if (channel.target_path == "rotation")
			{
				auto rot_it = animation.rotations.find(sampler.output);	//回転情報のイテレーター

				//回転情報が見つからないか、インデックスが範囲外の場合はエラー
				if (rot_it == animation.rotations.end() || (keyframe_index + INDEX_OFFSET_NEXT) >= rot_it->second.size())
				{
					OutputDebugStringA("[GltfRootMotion Error] ComputeRootPose: rotations key or index error!\n");
					continue;
				}

				const std::vector<DirectX::XMFLOAT4>& rotation = rot_it->second;	//回転情報
				DirectX::XMVECTOR rot_start = DirectX::XMLoadFloat4(&rotation.at(keyframe_index));	//開始回転クォータニオン
				DirectX::XMVECTOR rot_end = DirectX::XMLoadFloat4(&rotation.at(keyframe_index + INDEX_OFFSET_NEXT));	//回転終了クォータニオン
				DirectX::XMVECTOR slerped_rot = DirectX::XMQuaternionSlerp(rot_start, rot_end, interpolation_factor);	//球面線形補間された回転クォータニオン
				DirectX::XMStoreFloat4(&out_rotation, DirectX::XMQuaternionNormalize(slerped_rot));
			}
		}
	}
}
