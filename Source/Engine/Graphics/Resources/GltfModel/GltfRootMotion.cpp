#include "GltfRootMotion.h"
#include "GltfModelData.h"

#include <windows.h>
#include <algorithm>

void GltfRootMotion::Initialize(const std::shared_ptr<const GltfModelData>& data)
{
	//参照するモデル情報が存在するか確認
	if (!data)
	{
		OutputDebugStringA("[GltfRootMotion Error] Initialize: data is null!\n");
		return;
	}
	model_data = data;

	//シーンとルートノード配列の存在確認
	int scene_index = model_data->default_scene;
	if (scene_index < 0 || static_cast<size_t>(scene_index) >= model_data->scenes.size())
	{
		scene_index = 0;
	}

	if (!model_data->scenes.empty() && !model_data->scenes.at(scene_index).nodes.empty())
	{
		base_root_node_index = model_data->scenes.at(scene_index).nodes.at(0);
	}

	AnalyzeAnimations();
	ResetDelta();
}

//アニメーションの進行に合わせてルートノードの差分を計算
void GltfRootMotion::Update(size_t animation_index, float current_time)
{
	current_target_node_index = INVALID_NODE_INDEX;

	auto it = animation_target_nodes.find(animation_index);	//現在のアニメーションに対応するターゲットノードを検索するためのイテレータ

	//検索結果の成否判定
	if (it != animation_target_nodes.end())
	{
		current_target_node_index = it->second;
	}

	//適用中ノードのIDと名前を可視化するデバッグ出力
	if (current_target_node_index != INVALID_NODE_INDEX && current_target_node_index < model_data->nodes.size())
	{
		char active_node_info[256];
		std::string node_name = model_data->nodes.at(current_target_node_index).name;

		sprintf_s(active_node_info, "[RootMotion Active Node] ID: %d | Name: %s\n", current_target_node_index, node_name.c_str());
	}

	//ターゲットインデックスの有効チェック
	if (current_target_node_index == INVALID_NODE_INDEX)
	{
		return;
	}

	//現在のモーションの0秒時点（初期位置）を算出してキャッシュ
	DirectX::XMFLOAT4 dummy_rotation;
	ComputeRootPose(animation_index, 0.0f, initial_local_position, dummy_rotation);

	DirectX::XMFLOAT3 current_position;	//現在の位置
	DirectX::XMFLOAT4 current_rotation;	//現在の角度

	ComputeRootPose(animation_index, current_time, current_position, current_rotation);

	//初回更新の判定
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
		DirectX::XMVECTOR prev_pos = DirectX::XMLoadFloat3(&previous_position);
		DirectX::XMVECTOR curr_pos = DirectX::XMLoadFloat3(&current_position);
		DirectX::XMVECTOR prev_rot = DirectX::XMLoadFloat4(&previous_rotation);
		DirectX::XMVECTOR curr_rot = DirectX::XMLoadFloat4(&current_rotation);

		//アニメーションの巻き戻り（ループ）判定
		if (current_time < previous_time)
		{
			DirectX::XMFLOAT3 start_position;
			DirectX::XMFLOAT4 start_rotation;
			ComputeRootPose(animation_index, 0.0f, start_position, start_rotation);
			DirectX::XMVECTOR begin_pos = DirectX::XMLoadFloat3(&start_position);
			DirectX::XMVECTOR begin_rot = DirectX::XMLoadFloat4(&start_rotation);

			float duration = model_data->animations.at(animation_index).duration;
			DirectX::XMFLOAT3 end_position;
			DirectX::XMFLOAT4 end_rotation;
			ComputeRootPose(animation_index, duration, end_position, end_rotation);
			DirectX::XMVECTOR end_pos = DirectX::XMLoadFloat3(&end_position);
			DirectX::XMVECTOR end_rot = DirectX::XMLoadFloat4(&end_rotation);

			DirectX::XMVECTOR old_to_end = DirectX::XMVectorSubtract(end_pos, prev_pos);
			DirectX::XMVECTOR begin_to_new = DirectX::XMVectorSubtract(curr_pos, begin_pos);

			DirectX::XMStoreFloat3(&delta_position, DirectX::XMVectorAdd(old_to_end, begin_to_new));

			DirectX::XMVECTOR old_to_end_rot = DirectX::XMQuaternionMultiply(end_rot, DirectX::XMQuaternionInverse(prev_rot));
			DirectX::XMVECTOR begin_to_new_rot = DirectX::XMQuaternionMultiply(curr_rot, DirectX::XMQuaternionInverse(begin_rot));

			DirectX::XMStoreFloat4(&delta_rotation, DirectX::XMQuaternionMultiply(begin_to_new_rot, old_to_end_rot));
		}
		else
		{
			DirectX::XMStoreFloat3(&delta_position, DirectX::XMVectorSubtract(curr_pos, prev_pos));
			DirectX::XMVECTOR prev_rot_inv = DirectX::XMQuaternionInverse(prev_rot);
			DirectX::XMStoreFloat4(&delta_rotation, DirectX::XMQuaternionMultiply(curr_rot, prev_rot_inv));
		}

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

//各アニメーションデータを走査して移動値のある子ノードを事前に自動検出
void GltfRootMotion::AnalyzeAnimations()
{
	//モデルデータの有効チェック
	if (!model_data)
	{
		return;
	}

	//全アニメーションデータの走査
	for (size_t anim_idx = 0; anim_idx < model_data->animations.size(); anim_idx++)
	{
		const GltfModelData::animation& animation = model_data->animations.at(anim_idx);
		int max_movement_node_index = INVALID_NODE_INDEX;
		float max_distance = 0.0f;

		//アニメーション内全チャンネルの走査
		for (const GltfModelData::animation::channel& channel : animation.channels)
		{
			//移動値（translation）チャンネルかつ最上位ルート以外のノードであるかの判定
			if (channel.target_path == "translation")
			{
				const GltfModelData::animation::sampler& sampler = animation.samplers.at(channel.sampler);
				auto trans_it = animation.translations.find(sampler.output);

				//位置キーフレームデータの存在確認
				if (trans_it != animation.translations.end())
				{
					const std::vector<DirectX::XMFLOAT3>& translations = trans_it->second;

					//キーフレーム数が有効であるかの判定
					if (translations.size() >= 2)
					{
						float current_node_total_distance = 0.0f;	//当該ボーンの全フレームにおける総走行距離を蓄積

						//全キーフレーム間の移動距離を累積するループ
						for (size_t k = 0; k < translations.size() - 1; k++)
						{
							//隣り合うキーフレームの座標ベクトルをロード
							DirectX::XMVECTOR p0 = DirectX::XMLoadFloat3(&translations.at(k));
							DirectX::XMVECTOR p1 = DirectX::XMLoadFloat3(&translations.at(k + 1));

							DirectX::XMVECTOR move_vec = DirectX::XMVectorSubtract(p1, p0);		//直線移動ベクトル

							DirectX::XMVECTOR length_vec = DirectX::XMVector3Length(move_vec);	//ベクトルの長さ

							float distance = 0.0f;	//直線距離
							DirectX::XMStoreFloat(&distance, length_vec);

							current_node_total_distance += distance;
						}
						//最大移動距離の更新判定
						if (current_node_total_distance > max_distance)
						{
							max_distance = current_node_total_distance;
							max_movement_node_index = channel.target_node;
						}
					}
				}
			}
		}

		//移動値を持つノードが検出できたかの判定
		if (max_movement_node_index != INVALID_NODE_INDEX)
		{
			animation_target_nodes[anim_idx] = max_movement_node_index;
		}
		else
		{
			animation_target_nodes[anim_idx] = base_root_node_index;
		}
	}
}

//任意の時間のルートノードの位置と回転を計算
void GltfRootMotion::ComputeRootPose(size_t animation_index, float time, DirectX::XMFLOAT3& out_position, DirectX::XMFLOAT4& out_rotation) const
{
	// 参照するアニメーションインデックスの範囲外チェック
	if (animation_index >= model_data->animations.size())
	{
		OutputDebugStringA("[GltfRootMotion エラー] ComputeRootPose: 指定されたアニメーションインデックスが範囲外です。\n");
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
		if (channel.target_node != current_target_node_index)
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
		if (timeline.empty())continue;

		//時間のクランプ処理
		float clamped_time = time;
		if (clamped_time <= timeline.front())clamped_time = timeline.front();
		else if (clamped_time >= timeline.back())clamped_time = timeline.back();

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

					if (clamped_time >= t0 && clamped_time <= t1)
					{
						float time_diff = t1 - t0;
						// 再生時間とキーフレームの時間から補間率を算出する
						float rate = (time_diff > 0.00001f) ? ((clamped_time - t0) / time_diff) : 0.0f;
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

						// 前のキーフレームと次のキーフレームのスケール姿勢をLerp補間
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
