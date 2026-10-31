#include "GltfModelAnimation.h"

#include <algorithm>
#include <Engine\Core\MacroTracy.h>


//アニメーションクラスをモデルデータで初期化
void GltfModelAnimation::Initialize(const std::shared_ptr<const GltfModelData>& data)
{
	BS_PROFILE_FUNCTION();

	//データの保持と作業用ノード配列の初期化
	model_data = data;	//モデルデータを保存
	if (model_data)
	{
		animated_nodes = model_data->nodes;

		work_poses.resize(animated_nodes.size());
		blend_poses_a.resize(animated_nodes.size());
		blend_poses_b.resize(animated_nodes.size());

		target_root_node_indices.resize(model_data->animations.size(), -1);
		for (size_t i = 0; i < model_data->animations.size(); ++i)
		{
			target_root_node_indices[i] = ComputeTargetRootNodeIndex(i);
		}
		BuildAnimationCache();
	}
	is_animation_finished = false;
}

//親子関係をたどって各ノードのグローバル行列を計算
void GltfModelAnimation::CumulateTransforms()
{
	BS_PROFILE_FUNCTION();

	if (!model_data || model_data->flattened_node_indices.empty())
	{
		return;
	}

	using namespace DirectX;

	for (int node_index : model_data->flattened_node_indices)
	{
		if (node_index < 0 || static_cast<size_t>(node_index) >= animated_nodes.size())
		{
			continue;
		}

		GltfModelData::node& current_node = animated_nodes[node_index];

		XMMATRIX scale_matrix = XMMatrixScaling(current_node.scale.x, current_node.scale.y, current_node.scale.z);
		XMMATRIX rotation_matrix = XMMatrixRotationQuaternion(XMVectorSet(current_node.rotation.x, current_node.rotation.y, current_node.rotation.z, current_node.rotation.w));
		XMMATRIX translation_matrix = XMMatrixTranslation(current_node.translation.x, current_node.translation.y, current_node.translation.z);

		XMMATRIX local_matrix = scale_matrix * rotation_matrix * translation_matrix;

		int parent_index = current_node.parent_index;
		if (parent_index >= 0 && static_cast<size_t>(parent_index) < animated_nodes.size())
		{
			XMMATRIX parent_global = XMLoadFloat4x4(&animated_nodes[parent_index].global_transform);
			XMStoreFloat4x4(&current_node.global_transform, local_matrix * parent_global);
		}
		else
		{
			XMStoreFloat4x4(&current_node.global_transform, local_matrix);
		}
	}
}

//名前を指定してアニメーションの再生を開始
void GltfModelAnimation::PlayAniamtion(const std::string& animation_name, bool is_loop)
{
	BS_PROFILE_FUNCTION();

	if (!model_data) return;	//モデルデータが有効か確認
	//マップからアニメーション番号の検索
	auto iterator = model_data->animation_index_map.find(animation_name);	//指定されたアニメーションを検索
	if (iterator == model_data->animation_index_map.end())					//名前が見つからない場合
	{
		return;	//処理を終了
	}

	//再生状態の初期化
	current_animation_index = iterator->second;	//検索結果からインデックス番号を取得
	is_loop_enabled = is_loop;					//ループフラグを設定
	current_animation_time = 0.0f;				//再生時間をリセット
	current_animation_duration = CalculateAnimationDuration(current_animation_index);	//アニメーション終了時間を取得
	is_playing = true;							//再生フラグを起動
	is_animation_finished = false;
}

//アニメーション更新処理
void GltfModelAnimation::UpdateAnimation(float delta_time)
{
	BS_PROFILE_FUNCTION();

	if (!model_data || !is_playing || model_data->animations.empty())return;

	//時間の進行とループ判定
	current_animation_time += delta_time;						//経過時間を加算してアニメーションを進める
	if (current_animation_time > current_animation_duration)	//現在の時間がアニメーションの終了時間を超過した場合
	{
		is_animation_finished = true;
		if (is_loop_enabled)	//ループ再生が有効な場合
		{
			current_animation_time = fmod(current_animation_time, current_animation_duration);	//超過した余りの時間を計算し、最初からループ
		}
		else
		{
			current_animation_time = current_animation_duration;	//終了時間でピッタリ止めるため数値を固定
			is_playing = false;										//再生完了としてフラグをオフに設定
		}
	}
	Animate(current_animation_index, current_animation_time);	//実際の計算処理
}

//現在のアニメーション経過時間を取得
float GltfModelAnimation::GetCurrentAnimationTime() const
{
	BS_PROFILE_FUNCTION();

	//モデルデータが読み込まれていないなど、不正な状態での呼び出しを検知
	if (!model_data)
	{
		OutputDebugStringA("[GltfModelAnimation Error] GetCurrentAnimationTime: model_data is null!\n");
		return 0.0f;
	}

	return current_animation_time;
}

//アニメーションの再生時間を設定
void GltfModelAnimation::SetCurrentAnimationTime(float time)
{
	BS_PROFILE_FUNCTION();

	if (!model_data || model_data->animations.empty())return;

	//指定された時間に再生経過時間を設定
	current_animation_time = time;
	is_animation_finished = false;

	//ループ設定やアニメーションの総時間に合わせて、設定された時間を範囲内に丸め込む
	if (current_animation_time > current_animation_duration)
	{
		if (is_loop_enabled)
		{
			//ループ再生が有効な場合は、剰余をとってループ
			current_animation_time = fmodf(current_animation_time, current_animation_duration);
		}
		else
		{
			//ループ再生が無効な場合は、終端（総時間）で止める
			current_animation_time = current_animation_duration;
			is_animation_finished = true;
			is_playing = false;
		}
	}
	else if(current_animation_time < 0.0f)
	{
		current_animation_time = 0.0f;
	}
	Animate(current_animation_index, current_animation_time);
}

//アニメーションの総時間を取得
float GltfModelAnimation::GetAnimationDuration() const
{
	BS_PROFILE_FUNCTION();

	//モデルデータが読み込まれていない場合は安全のためゼロを返す
	if (!model_data)
	{
		OutputDebugStringA("[GltfModelAnimation Error] GetAnimationDuration: model_data is null!\n");
		return 0.0f;
	}

	return current_animation_duration;
}

//指定した時間のアニメーションからTRS成分のみをサンプリング
void GltfModelAnimation::SampleAnimationPose(size_t animation_index, float time, std::vector<NodePose>& out_poses)
{
	BS_PROFILE_FUNCTION();

	out_poses.resize(animated_nodes.size());

	for (size_t i = 0; i < animated_nodes.size(); ++i)
	{
		out_poses[i].scale = model_data->nodes[i].scale;
		out_poses[i].rotation = model_data->nodes[i].rotation;
		out_poses[i].translation = model_data->nodes[i].translation;
	}

	if (model_data->animations.empty() || animation_index >= animation_caches.size())
	{
		return;
	}

	using namespace DirectX;
	constexpr size_t INDEX_OFFSET_NEXT = 1;
	int target_root_node_index = GetTargetRootNodeIndex(animation_index);
	const std::vector<AnimationChannelCache>& cache_list = animation_caches[animation_index];

	for (const AnimationChannelCache& cache : cache_list)
	{
		if (static_cast<size_t>(cache.target_node) >= out_poses.size() || !cache.timeline)
		{
			continue;
		}

		float interpolation_factor = 0.0f;
		size_t keyframe_index = GetAnimationKeyframeIndex(*cache.timeline, time, interpolation_factor);

		if (cache.path_type == TargetPath::Scale && cache.scales)
		{
			if ((keyframe_index + INDEX_OFFSET_NEXT) < cache.scales->size())
			{
				const std::vector<XMFLOAT3>& scales = *cache.scales;
				XMVECTOR scale_start = XMLoadFloat3(&scales.at(keyframe_index));
				XMVECTOR scale_end = XMLoadFloat3(&scales.at(keyframe_index + INDEX_OFFSET_NEXT));
				XMVECTOR lerped_scale = XMVectorLerp(scale_start, scale_end, interpolation_factor);
				XMStoreFloat3(&out_poses[cache.target_node].scale, lerped_scale);
			}
		}
		else if (cache.path_type == TargetPath::Rotation && cache.rotations)
		{
			if ((keyframe_index + INDEX_OFFSET_NEXT) < cache.rotations->size())
			{
				const std::vector<XMFLOAT4>& rotations = *cache.rotations;
				XMVECTOR rot_start = XMQuaternionNormalize(XMLoadFloat4(&rotations.at(keyframe_index)));
				XMVECTOR rot_end = XMQuaternionNormalize(XMLoadFloat4(&rotations.at(keyframe_index + INDEX_OFFSET_NEXT)));
				XMVECTOR slerped_rot = XMQuaternionNormalize(XMQuaternionSlerp(rot_start, rot_end, interpolation_factor));
				XMStoreFloat4(&out_poses[cache.target_node].rotation, slerped_rot);
			}
		}
		else if (cache.path_type == TargetPath::Translation && cache.translations)
		{
			if ((keyframe_index + INDEX_OFFSET_NEXT) < cache.translations->size())
			{
				const std::vector<XMFLOAT3>& translations = *cache.translations;
				XMVECTOR trans_start = XMLoadFloat3(&translations.at(keyframe_index));
				XMVECTOR trans_end = XMLoadFloat3(&translations.at(keyframe_index + INDEX_OFFSET_NEXT));
				XMVECTOR lerped_trans = XMVectorLerp(trans_start, trans_end, interpolation_factor);

				if (IsRootNode(cache.target_node))
				{
					constexpr float ZERO_POSITION = 0.0f;
					out_poses[cache.target_node].translation = XMFLOAT3(ZERO_POSITION, ZERO_POSITION, ZERO_POSITION);
				}
				else if (cache.target_node == target_root_node_index)
				{
					XMFLOAT3 initial_pose_trans = translations.front();
					XMFLOAT3 final_trans;
					XMStoreFloat3(&final_trans, lerped_trans);
					final_trans.x = initial_pose_trans.x;
					final_trans.z = initial_pose_trans.z;
					out_poses[cache.target_node].translation = final_trans;
				}
				else
				{
					XMStoreFloat3(&out_poses[cache.target_node].translation, lerped_trans);
				}
			}
		}
	}
}

//指定した時間のアニメーションを適用しノード情報を更新
void GltfModelAnimation::Animate(size_t animation_index, float time)
{
	BS_PROFILE_FUNCTION();

	if (model_data->animations.empty())
	{
		return;
	}

	if (work_poses.size() != animated_nodes.size())
	{
		OutputDebugStringA("[GltfModelAnimation エラー] Animate: バッファサイズが不正です\n");
		return;
	}

	SampleAnimationPose(animation_index, time, work_poses);

	for (size_t i = 0; i < animated_nodes.size() && i < work_poses.size(); ++i)
	{
		animated_nodes[i].scale = work_poses[i].scale;
		animated_nodes[i].rotation = work_poses[i].rotation;
		animated_nodes[i].translation = work_poses[i].translation;
	}

	CumulateTransforms();
}

//対象のノードがシーンの最上位ルートノードが判定
bool GltfModelAnimation::IsRootNode(int node_index) const
{
	BS_PROFILE_FUNCTION();

	if (!model_data || model_data->scenes.empty())
	{
		OutputDebugStringA("[GltfModelAnimation 警告] IsRootNode: model_data または scenes が空です。\n");
		return false;
	}

	int scene_index = model_data->default_scene;
	if (scene_index < 0 || static_cast<size_t>(scene_index) >= model_data->scenes.size())scene_index = 0;
	const auto& root_nodes = model_data->scenes.at(scene_index).nodes;
	return std::find(root_nodes.begin(), root_nodes.end(), node_index) != root_nodes.end();
}

//指定アニメーション内で最も移動量の大きいルートモーション対象ノードのインデックスを取得
int GltfModelAnimation::GetTargetRootNodeIndex(size_t animation_index) const
{
	BS_PROFILE_FUNCTION();
	if (animation_index >= target_root_node_indices.size())
	{
		OutputDebugStringA("[GltfModelAnimation エラー] GetTargetRootNodeIndex: アニメーションインデックスが配列の範囲外です\n");
		return -1;
	}
	return target_root_node_indices[animation_index];
}

//アニメーションのキーフレームを取得し補間係数を計算
size_t GltfModelAnimation::GetAnimationKeyframeIndex(const std::vector<float>& timelines, float time, float& interpolation_factor)
{
	BS_PROFILE_FUNCTION();

	const size_t keyframe_count = timelines.size();
	const size_t MIN_KEYFRAME_COUNT = 2;
	const size_t INDEX_OFFSET_NEXT = 1;
	const size_t INDEX_OFFSET_PREV = 2;
	const float INTERPOLATION_MAX = 1.0f;
	const float INTERPOLATION_MIN = 0.0f;
	const size_t START_INDEX = 0;
	constexpr float EPSILON_TIME_DELTA = 0.00001f;

	if (keyframe_count < MIN_KEYFRAME_COUNT)
	{
		OutputDebugStringA("[GltfModelAnimation 警告] GetAnimationKeyframeIndex: タイムラインのキーフレーム数が2未満です。\n");
		interpolation_factor = INTERPOLATION_MIN;
		return START_INDEX;
	}

	const size_t last_keyframe_index = keyframe_count - INDEX_OFFSET_NEXT;
	const size_t max_valid_segment_index = keyframe_count - INDEX_OFFSET_PREV;

	if (time >= timelines.at(last_keyframe_index))
	{
		interpolation_factor = INTERPOLATION_MAX;
		return max_valid_segment_index;
	}
	else if (time <= timelines.at(START_INDEX))
	{
		interpolation_factor = INTERPOLATION_MIN;
		return START_INDEX;
	}

	size_t keyframe_index = START_INDEX;
	for (size_t time_index = INDEX_OFFSET_NEXT; time_index < keyframe_count; ++time_index)
	{
		if (time < timelines.at(time_index))
		{
			keyframe_index = time_index - INDEX_OFFSET_NEXT;
			break;
		}
	}

	if (keyframe_index > max_valid_segment_index)
	{
		char error_log[256];
		sprintf_s(error_log, "[GltfModelAnimation エラー] 存在しないキーフレーム区間が検出されました: index=%zu, max=%zu\n",
			keyframe_index, max_valid_segment_index);
		OutputDebugStringA(error_log);
		keyframe_index = max_valid_segment_index;
	}

	float time_difference = timelines.at(keyframe_index + INDEX_OFFSET_NEXT) - timelines.at(keyframe_index);
	time_difference = (std::max)(time_difference, EPSILON_TIME_DELTA);
	float raw_factor = (time - timelines.at(keyframe_index)) / time_difference;
	interpolation_factor = std::clamp(raw_factor, INTERPOLATION_MIN, INTERPOLATION_MAX);

	return keyframe_index;
}

//アニメーションの全体の長さを計算
float GltfModelAnimation::CalculateAnimationDuration(size_t animation_index)
{
	BS_PROFILE_FUNCTION();

	if (!model_data || model_data->animations.empty() || animation_index >= model_data->animations.size())
	{
		OutputDebugStringA("[GltfModelAnimation Error] CalculateAnimationDuration: index out of range!\n");
		return 0.0f;
	}

	float max_time = 0.0f;	//記録用の最大時間
	const GltfModelData::animation& animation = model_data->animations.at(animation_index);	//指定されたアニメーションデータを取得
	for (const GltfModelData::animation::channel& channel : animation.channels)			//アニメーションが持つ全チャンネルをループ
	{
		//サンプラーインデックスの安全確認
		if (channel.sampler < 0 || static_cast<size_t>(channel.sampler) >= animation.samplers.size())
		{
			OutputDebugStringA("[GltfModelAnimation Error] CalculateAnimationDuration : channel.sampler is out of range!\n");
			continue;
		}

		const GltfModelData::animation::sampler& sampler = animation.samplers.at(channel.sampler);	//サンプラーを取得
		
		//タイムラインマップのキーの安全な存在確認
		auto timeline_it = animation.timelines.find(sampler.input);
		if (timeline_it == animation.timelines.end())
		{
			OutputDebugStringA("[GltfModelAnimation Error] CalculateAnimationDuration: sampler.input key not found in timelines map!\n");
			continue;
		}
		
		const std::vector<float>& timeline = timeline_it->second;					//サンプラーからの時間軸の配列を取得
		if (!timeline.empty())	//タイムラインにデータが存在する場合
		{
			max_time = std::max<float>(max_time, timeline.back());	//これまでの最大時間と、現在のタイムラインの最後の時間を比較し、大きい方を記録
		}
	}
	return max_time;	//アニメーション終了時間を返す
}

//アニメーションキャッシュの構築
void GltfModelAnimation::BuildAnimationCache()
{
	BS_PROFILE_FUNCTION();
	if (!model_data) return;

	animation_caches.resize(model_data->animations.size());

	for (size_t i = 0; i < model_data->animations.size(); ++i)
	{
		const GltfModelData::animation& animation = model_data->animations[i];
		std::vector<AnimationChannelCache>& cache_list = animation_caches[i];
		cache_list.reserve(animation.channels.size());

		for (const GltfModelData::animation::channel& channel : animation.channels)
		{
			if (channel.sampler < 0 || static_cast<size_t>(channel.sampler) >= animation.samplers.size())
			{
				continue;
			}

			const GltfModelData::animation::sampler& sampler = animation.samplers.at(channel.sampler);
			AnimationChannelCache cache;
			cache.target_node = channel.target_node;

			auto timeline_it = animation.timelines.find(sampler.input);
			if (timeline_it != animation.timelines.end() && !timeline_it->second.empty())
			{
				cache.timeline = &timeline_it->second;
			}
			else
			{
				continue;
			}

			if (channel.target_path == "scale")
			{
				cache.path_type = TargetPath::Scale;
				auto data_it = animation.scales.find(sampler.output);
				if (data_it != animation.scales.end()) cache.scales = &data_it->second;
			}
			else if (channel.target_path == "rotation")
			{
				cache.path_type = TargetPath::Rotation;
				auto data_it = animation.rotations.find(sampler.output);
				if (data_it != animation.rotations.end()) cache.rotations = &data_it->second;
			}
			else if (channel.target_path == "translation")
			{
				cache.path_type = TargetPath::Translation;
				auto data_it = animation.translations.find(sampler.output);
				if (data_it != animation.translations.end()) cache.translations = &data_it->second;
			}

			if (cache.scales || cache.rotations || cache.translations)
			{
				cache_list.push_back(cache);
			}
		}
	}
}

//指定したノードの移動成分を外部から上書き
void GltfModelAnimation::SetNodeTranslation(int node_index, const DirectX::XMFLOAT3& translation)
{
	BS_PROFILE_FUNCTION();

	if (node_index >= 0 && static_cast<size_t>(node_index) < animated_nodes.size())
	{
		animated_nodes.at(node_index).translation = translation;
	}
}

//二つのアニメーション姿勢の合成
void GltfModelAnimation::AnimationBlend(size_t animation_index_a, float time_a, size_t animation_index_b, float time_b, float blend_factor)
{
	BS_PROFILE_FUNCTION();

	if (!model_data || model_data->animations.empty())
	{
		return;
	}

	constexpr float min_blend_factor = 0.0f;
	constexpr float max_blend_factor = 1.0f;
	float clamped_factor = std::clamp(blend_factor, min_blend_factor, max_blend_factor);

	constexpr float epsilon_threshold = 0.0001f;
	if (clamped_factor <= epsilon_threshold)
	{
		Animate(animation_index_a, time_a);
		return;
	}
	if (clamped_factor >= (max_blend_factor - epsilon_threshold))
	{
		Animate(animation_index_b, time_b);
		return;
	}

	if (blend_poses_a.size() != animated_nodes.size() || blend_poses_b.size() != animated_nodes.size())
	{
		OutputDebugStringA("[GltfModelAnimation エラー] AnimationBlend: バッファサイズが不正です\n");
		return;
	}

	SampleAnimationPose(animation_index_a, time_a, blend_poses_a);
	SampleAnimationPose(animation_index_b, time_b, blend_poses_b);

	using namespace DirectX;
	const size_t node_count = animated_nodes.size();

	for (size_t node_idx = 0; node_idx < node_count; ++node_idx)
	{
		XMVECTOR scale_a = XMLoadFloat3(&blend_poses_a[node_idx].scale);
		XMVECTOR scale_b = XMLoadFloat3(&blend_poses_b[node_idx].scale);
		XMVECTOR lerped_scale = XMVectorLerp(scale_a, scale_b, clamped_factor);
		XMStoreFloat3(&animated_nodes[node_idx].scale, lerped_scale);

		XMVECTOR rot_a = XMLoadFloat4(&blend_poses_a[node_idx].rotation);
		XMVECTOR rot_b = XMLoadFloat4(&blend_poses_b[node_idx].rotation);
		XMVECTOR slerped_rot = XMQuaternionNormalize(XMQuaternionSlerp(rot_a, rot_b, clamped_factor));
		XMStoreFloat4(&animated_nodes[node_idx].rotation, slerped_rot);

		XMVECTOR trans_a = XMLoadFloat3(&blend_poses_a[node_idx].translation);
		XMVECTOR trans_b = XMLoadFloat3(&blend_poses_b[node_idx].translation);
		XMVECTOR lerped_trans = XMVectorLerp(trans_a, trans_b, clamped_factor);
		XMStoreFloat3(&animated_nodes[node_idx].translation, lerped_trans);
	}

	// 合成された最終姿勢に対してのみ、1回だけ行列ツリーを計算する
	CumulateTransforms();
}

int GltfModelAnimation::ComputeTargetRootNodeIndex(size_t animation_index) const
{
	BS_PROFILE_FUNCTION();
	if (!model_data || animation_index >= model_data->animations.size())
	{
		OutputDebugStringA("[GltfModelAnimation ｴﾗｰ] ComputeTargetRootNodeIndex: model_dataが空、またはインデックスが範囲外です\n");
		return -1;
	}

	const GltfModelData::animation& animation = model_data->animations.at(animation_index);
	int max_movement_node_index = -1;
	float max_distance = 0.0f;

	for (const GltfModelData::animation::channel& channel : animation.channels)
	{
		if (channel.target_path == "translation")
		{
			if (channel.sampler < 0 || static_cast<size_t>(channel.sampler) >= animation.samplers.size())
			{
				continue;
			}
			const GltfModelData::animation::sampler& sampler = animation.samplers.at(channel.sampler);

			auto trans_it = animation.translations.find(sampler.output);
			if (trans_it != animation.translations.end())
			{
				const std::vector<DirectX::XMFLOAT3>& translations = trans_it->second;

				if (translations.size() >= 2)
				{
					float current_node_total_distance = 0.0f;
					for (size_t k = 0; k < translations.size() - 1; k++)
					{
						using namespace DirectX;
						XMVECTOR p0 = XMLoadFloat3(&translations.at(k));
						XMVECTOR p1 = XMLoadFloat3(&translations.at(k + 1));
						XMVECTOR move_vec = XMVectorSubtract(p1, p0);
						XMVECTOR length_vec = XMVector3Length(move_vec);

						float distance = 0.0f;
						XMStoreFloat(&distance, length_vec);
						current_node_total_distance += distance;
					}

					if (current_node_total_distance > max_distance)
					{
						max_distance = current_node_total_distance;
						max_movement_node_index = channel.target_node;
					}
				}
			}
		}
	}
	return max_movement_node_index;
}
