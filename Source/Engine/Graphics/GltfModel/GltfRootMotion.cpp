#include "GltfRootMotion.h"
#include "GltfModelData.h"

#include <windows.h>
#include <algorithm>

//初期化
void GltfRootMotion::Initialize(const std::shared_ptr<const GltfModelData>& data)
{
	//参照するモデル情報が存在するか確認
	if (!data)
	{
		OutputDebugStringA("[GltfRootMotion Error] Initialize: data is null!\n");
		return;
	}
	model_data = data;
	target_node_index = FindRootNodeIndex();
	printf("[DEBUG] RootMotion: Target Node Index is %d\n", target_node_index);
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
		DirectX::XMVECTOR prev_rot_inv = DirectX::XMQuaternionInverse(prev_rot); //前回回転の逆クォータニオン

		DirectX::XMStoreFloat4(&delta_rotation, DirectX::XMQuaternionMultiply(curr_rot, prev_rot_inv));

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

	out_position = { 0.0f, 0.0f, 0.0f };
	out_rotation = { 0.0f, 0.0f, 0.0f, 1.0f };

	constexpr size_t INDEX_OFFSET_NEXT = 1;			//次の番号への補正
	constexpr size_t INDEX_OFFSET_PREV = 2;			//２つ前の番号への補正
	constexpr float INTERPOLATION_MAX = 1.0f;		//補完係数の最大値
	constexpr float INTERPOLATION_MIN = 0.0f;		//補完係数の最小値
	constexpr size_t START_INDEX = 0;				//開始番号
	constexpr float MIN_TIME_DEFFERENCE = 0.00001f;	//時間差分の最小保証値

	const GltfModelData::animation& animation = model_data->animations.at(animation_index);	//参照するアニメーション情報

	bool found_translation = false;
	bool found_rotation = false;

	//アニメーションのチャンネルを巡回
	for (const GltfModelData::animation::channel& channel : animation.channels)
	{
		if (channel.target_node != target_node_index)
		{
			continue;
		}

		if (channel.target_path == "translation") {
			printf("DEBUG: Found target translation channel for Node: %d\n", channel.target_node);

			//対象ノードがルートノードと一致するか判定
			if (channel.target_path == "translation" || channel.target_path == "rotation")
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

				printf("[DEBUG] ComputeRootPose: Node %d, CurrentTime=%.3f, Timeline[0]=%.3f, Timeline[last]=%.3f\n",
					channel.target_node, time, timeline.front(), timeline.back());

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
					if (channel.target_node == target_node_index)
					{
						auto trans_it = animation.translations.find(sampler.output); // 移動情報のイテレーター

						// データが存在し、キーフレームが範囲内にあるか確認
						if (trans_it != animation.translations.end() && (keyframe_index + 1) < trans_it->second.size())
						{
							const std::vector<DirectX::XMFLOAT3>& translations = trans_it->second; // 移動情報

							// デバッグ出力：取得したキーフレームの中身を確認
							const DirectX::XMFLOAT3& start = translations.at(keyframe_index);
							const DirectX::XMFLOAT3& end = translations.at(keyframe_index + 1);
							printf("[DEBUG] ComputeRootPose: KeyIndex=%zu, Start=(%.4f, %.4f, %.4f), End=(%.4f, %.4f, %.4f)\n",
								keyframe_index, start.x, start.y, start.z, end.x, end.y, end.z);

							DirectX::XMVECTOR trans_start = DirectX::XMLoadFloat3(&start);
							DirectX::XMVECTOR trans_end = DirectX::XMLoadFloat3(&end);

							// 線形補間された位置を計算
							DirectX::XMVECTOR lerped_trans = DirectX::XMVectorLerp(trans_start, trans_end, interpolation_factor);
							DirectX::XMStoreFloat3(&out_position, lerped_trans);

							found_translation = true;
						}
						else
						{
							// データが見つからない、または範囲外の場合のデバッグ出力
							printf("[DEBUG] ComputeRootPose Warning: Translation data missing or index out of range for Node: %d\n", channel.target_node);
						}
					}
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
					found_rotation = true;
				}
			}
		}
	}
}

//階層構造からルートモーションの対象となるノードインデックスを検索
int GltfRootMotion::FindRootNodeIndex() const
{
	//モデル情報やスキン情報が有効か確認
	if (!model_data || model_data->skins.empty())
	{
		return 0;
	}

	std::string target_name = "Motion"; // ターゲットとするボーン名
	for (size_t i = 0; i < model_data->nodes.size(); i++)
	{
		if (model_data->nodes.at(i).name == target_name)
		{
			printf("[DEBUG] RootMotion: Found '%s' at Index: %zu\n", target_name.c_str(), i);
			return static_cast<int>(i);
		}
		else
		{
			printf("[DEBUG] RootMotion: '%s' not found. Falling back to 0.\n", target_name.c_str());
			return 0;
		}
	}

	const size_t target_skin_index = 0;	//対象のスキン番号
	const GltfModelData::skin& skin = model_data->skins.at(target_skin_index);	//モデルのスキン情報

	//スキンに含まれるすべてのボーンを走査
	for (size_t i = 0; i < skin.joints.size(); i++)
	{
		int current_joint = skin.joints.at(i);	//判定対象のボーン
		bool is_child_of_another = false;		//ボーンの子フラグ

		//他の全ボーンを走査して親子関係を確認
		for (size_t j = 0; j < skin.joints.size(); j++)
		{
			//自身との比較をスキップ
			if (i == j)
			{
				continue;
			}

			int other_joint = skin.joints.at(j);	//比較対象の親候補ボーン
			const GltfModelData::node& other_node = model_data->nodes.at(other_joint);	//親候補のノード情報

			//親候補の子供リストを走査
			for (int child_index : other_node.children)
			{
				//自身が子供リストに含まれているか判定
				if (child_index == current_joint)
				{
					is_child_of_another = true;
					break;
				}
			}
			//子である事が判明した場合は探索を抜ける
			if (is_child_of_another)
			{
				break;
			}
		}
		//どのボーンの子でもないか判定
		if (!is_child_of_another)
		{
			return current_joint;
		}
	}
	OutputDebugStringA("[GltfRootMotion Warning] Root node not found from skeleton hierarchy. Fallback to index 0.\n");
	return 0;
}
