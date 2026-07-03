#include "GltfRootMotion.h"
#include "GltfModelData.h"

#include <windows.h>
#include <algorithm>

void GltfRootMotion::Initialize(const std::shared_ptr<const GltfModelData>& data, const std::string& root_node_name)
{
	//参照するモデル情報が存在するか確認
	if (!data)
	{
		OutputDebugStringA("[GltfRootMotion Error] Initialize: data is null!\n");
		return;
	}
	model_data = data;
	target_node_index = FindRootNodeIndex(root_node_name);
	printf("[DEBUG] RootMotion: Target Node Index is %d\n", target_node_index);
	ResetDelta();
}

//アニメーションの進行に合わせてルートノードの差分を計算
void GltfRootMotion::Update(size_t animation_index, float current_time)
{
	DirectX::XMFLOAT3 current_position;	//現在のルートノードの位置
	DirectX::XMFLOAT4 current_rotation;	//現在のルートノードの回転

	//指定時間におけるルートノードの生の姿勢を計算
	ComputeRootPose(animation_index, current_time, current_position, current_rotation);

	//更新化の判定フラグの成否判定
	if (is_first_update)
	{
		previous_position = current_position;
		previous_rotation = current_rotation;
		previous_time = current_time;
		delta_position = { 0.0f,0.0f,0.0f };
		delta_rotation = { 0.0f,0.0f,0.0f,1.0f };
		is_first_update = false;
	}
	else
	{
		DirectX::XMVECTOR prev_pos = DirectX::XMLoadFloat3(&previous_position); // 前回の位置ベクトル
		DirectX::XMVECTOR curr_pos = DirectX::XMLoadFloat3(&current_position); // 現在の位置ベクトル

		//アニメーションループ（時間が巻き戻った瞬間）の検知と補正
		if (current_time < previous_time)
		{
			DirectX::XMFLOAT3 start_position;
			DirectX::XMFLOAT4 dummy_start_rotation;
			ComputeRootPose(animation_index, 0.0f, start_position, dummy_start_rotation);
			DirectX::XMVECTOR begin_pos = DirectX::XMLoadFloat3(&start_position);


			// アニメーション全体の終端（最後のキーフレーム）における座標を取得するため
			// そのアニメーションの最大継続時間（duration）を使って姿勢を再計算
			float duration = model_data->animations.at(animation_index).duration;
			DirectX::XMFLOAT3 end_position;
			DirectX::XMFLOAT4 dummy_rotation;
			ComputeRootPose(animation_index, duration, end_position, dummy_rotation);
			DirectX::XMVECTOR end_pos = DirectX::XMLoadFloat3(&end_position);

			// 外部プロジェクトの思想に基づき、「前回の位置から終端までの移動量」と
			// 「開始点（0）から現在の位置までの移動量」を合計して連続した移動量（Delta）を算出します
			DirectX::XMVECTOR old_to_end = DirectX::XMVectorSubtract(end_pos, prev_pos);
			DirectX::XMVECTOR begin_to_new = DirectX::XMVectorSubtract(curr_pos, begin_pos);

			DirectX::XMStoreFloat3(&delta_position, DirectX::XMVectorAdd(old_to_end, begin_to_new));
		}
		else
		{
			// 通常進行時は、純粋に現在の位置と前回の位置の差分を移動量とする
			DirectX::XMStoreFloat3(&delta_position, DirectX::XMVectorSubtract(curr_pos, prev_pos)); //
		}

		//回転の差分計算とトランスフォームの保存
		DirectX::XMVECTOR prev_rot = DirectX::XMLoadFloat4(&previous_rotation); // 前回の回転クォータニオン
		DirectX::XMVECTOR curr_rot = DirectX::XMLoadFloat4(&current_rotation); // 現在の回転クォータニオン
		DirectX::XMVECTOR prev_rot_inv = DirectX::XMQuaternionInverse(prev_rot); // 前回回転の逆クォータニオン

		DirectX::XMStoreFloat4(&delta_rotation, DirectX::XMQuaternionMultiply(curr_rot, prev_rot_inv)); 

		DirectX::XMVECTOR local_translation = DirectX::XMVectorSubtract(curr_pos, prev_pos);
		const GltfModelData::node& root_motion_node = model_data->nodes.at(target_node_index);
		DirectX::XMMATRIX parentGlobalTransform = DirectX::XMMatrixIdentity();


		//次のフレームのために現在の状態を保存
		previous_position = current_position; 
		previous_rotation = current_rotation;
		previous_time = current_time;
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
	previous_time = 0.0f;
}

//任意の時間のルートノードの位置と回転を計算
void GltfRootMotion::ComputeRootPose(size_t animation_index, float time, DirectX::XMFLOAT3& out_position, DirectX::XMFLOAT4& out_rotation) const
{
	// 参照するアニメーションインデックスの範囲外チェック
	if (animation_index >= model_data->animations.size())
	{
		OutputDebugStringA("[GltfRootMotion Error] ComputeRootPose: animation_index out of range!\n");
		return;
	}

	// 出力パラメータを初期化
	out_position = { 0.0f, 0.0f, 0.0f };
	out_rotation = { 0.0f, 0.0f, 0.0f, 1.0f };
	DirectX::XMVECTOR final_scale = DirectX::XMVectorSet(1.0f, 1.0f, 1.0f, 0.0f);

	const GltfModelData::animation& animation = model_data->animations.at(animation_index);

	// アニメーション内の各チャンネルを走査
	for (const GltfModelData::animation::channel& channel : animation.channels)
	{
		// 対象がルートモーションのターゲットノードではない場合はスキップ
		if (channel.target_node != target_node_index)
		{
			continue;
		}

		// サンプラーのインデックス範囲外チェック
		if (channel.sampler < 0 || static_cast<size_t>(channel.sampler) >= animation.samplers.size())
		{
			OutputDebugStringA("[GltfRootMotion Error] ComputeRootPose: channel.sampler is out of range!\n");
			continue;
		}

		const GltfModelData::animation::sampler& sampler = animation.samplers.at(channel.sampler);
		auto timeline_it = animation.timelines.find(sampler.input);

		// タイムラインデータの存在チェック
		if (timeline_it == animation.timelines.end())
		{
			OutputDebugStringA("[GltfRootMotion Error] ComputeRootPose: sampler.input key not found!\n");
			continue;
		}

		const std::vector<float>& timeline = timeline_it->second;

		// タイムラインが空の場合はスキップ
		if (timeline.empty())
		{
			continue;
		}

		// 位置（translation）の補間計算
		if (channel.target_path == "translation")
		{
			auto trans_it = animation.translations.find(sampler.output);
			if (trans_it != animation.translations.end() && trans_it->second.size() == timeline.size())
			{
				const std::vector<DirectX::XMFLOAT3>& translations = trans_it->second;

				// タイムラインの配列をループで走査し、現在の時間がどのキーフレームの間にあるか判定する
				for (size_t index = 0; index < timeline.size() - 1; ++index)
				{
					float t0 = timeline.at(index);
					float t1 = timeline.at(index + 1);

					if (time >= t0 && time <= t1)
					{
						// 再生時間とキーフレームの時間から補間率を算出する
						float rate = (time - t0) / (t1 - t0);

						// 前のキーフレームと次のキーフレームの姿勢を補間
						DirectX::XMVECTOR V0 = DirectX::XMLoadFloat3(&translations.at(index));
						DirectX::XMVECTOR V1 = DirectX::XMLoadFloat3(&translations.at(index + 1));
						DirectX::XMVECTOR V = DirectX::XMVectorLerp(V0, V1, rate);

						// 計算結果を格納
						DirectX::XMStoreFloat3(&out_position, V);
						break;
					}
				}
			}
		}
		//回転（rotation）の補間計算
		if (channel.target_path == "rotation")
		{
			auto rot_it = animation.rotations.find(sampler.output);
			if (rot_it != animation.rotations.end() && rot_it->second.size() == timeline.size())
			{
				const std::vector<DirectX::XMFLOAT4>& rotations = rot_it->second;

				// タイムラインの配列をループで走査し、現在の時間がどのキーフレームの間にあるか判定する
				for (size_t index = 0; index < timeline.size() - 1; ++index)
				{
					float t0 = timeline.at(index);
					float t1 = timeline.at(index + 1);

					if (time >= t0 && time <= t1)
					{
						// 再生時間とキーフレームの時間から補間率を算出する
						float rate = (time - t0) / (t1 - t0);

						// 前のキーフレームと次のキーフレームの姿勢を補間
						DirectX::XMVECTOR Q0 = DirectX::XMLoadFloat4(&rotations.at(index));
						DirectX::XMVECTOR Q1 = DirectX::XMLoadFloat4(&rotations.at(index + 1));
						DirectX::XMVECTOR Q = DirectX::XMQuaternionSlerp(Q0, Q1, rate);

						// 計算結果を格納
						DirectX::XMStoreFloat4(&out_rotation, DirectX::XMQuaternionNormalize(Q));
						break;
					}
				}
			}
		}
		//スケール（scale）の補間計算
		if (channel.target_path == "scale")
		{
			auto scale_it = animation.scales.find(sampler.output);
			if (scale_it != animation.scales.end() && scale_it->second.size() == timeline.size())
			{
				const std::vector<DirectX::XMFLOAT3>& scales = scale_it->second;

				// タイムラインの配列をループで走査し、現在の時間がどのキーフレームの間にあるか判定する
				for (size_t index = 0; index < timeline.size() - 1; ++index)
				{
					float t0 = timeline.at(index);
					float t1 = timeline.at(index + 1);

					if (time >= t0 && time <= t1)
					{
						// 再生時間とキーフレームの時間から補間率を算出する
						float rate = (time - t0) / (t1 - t0);

						// 前のキーフレームと次のキーフレームのスケール姿勢を提示コードと同じ作りでLerp補間
						DirectX::XMVECTOR S0 = DirectX::XMLoadFloat3(&scales.at(index));
						DirectX::XMVECTOR S1 = DirectX::XMLoadFloat3(&scales.at(index + 1));
						final_scale = DirectX::XMVectorLerp(S0, S1, rate);
						break;
					}
				}
			}
		}
	}
	DirectX::XMVECTOR calculated_position = DirectX::XMLoadFloat3(&out_position);
	calculated_position = DirectX::XMVectorMultiply(calculated_position, final_scale);
	DirectX::XMStoreFloat3(&out_position, calculated_position);
}

//階層構造からルートモーションの対象となるノードインデックスを検索
int GltfRootMotion::FindRootNodeIndex(const std::string& root_node_name) const
{
	// モデルデータが正しく読み込まれているかチェック
	if (!model_data)
	{
		return 0;
	}

	// モデルが持つすべてのノードから、引数で指定された名前が一致するものを走査
	for (size_t i = 0; i < model_data->nodes.size(); i++)
	{
		// ノード名が指定されたルートノード名と一致したか判定
		if (model_data->nodes.at(i).name == root_node_name)
		{
			// デバッグ出力：見つかったノード名とインデックス番号を出力
			printf("[DEBUG] RootMotion: Found target node '%s' at Index: %zu\n", root_node_name.c_str(), i);
			return static_cast<int>(i);
		}
	}

	// 指定された名前のノードが一切見つからなかった場合の警告とフォールバック
	printf("[GltfRootMotion Warning] '%s' node was not found! Fallback to index 0.\n", root_node_name.c_str());
	return 0;
}
