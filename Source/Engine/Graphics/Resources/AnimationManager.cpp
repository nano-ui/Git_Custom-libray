#include "AnimationManager.h"

#include "Gameplay\Components\Model\ModelComponent.h"
#include "Gameplay\Components\Animation\AnimationSequencerComponent.h"
#include "Engine\Graphics\Resources\GltfModel\GltfModel.h"
#include "Engine\Graphics\Resources\Model.h"
#include "Engine\Graphics\Device\Frustum.h"
#include "Engine\Core\Log\Log.h"

#include <windows.h>
#include <string>
#include <unordered_map>
#include <algorithm>
#include <Engine\Core\MacroTracy.h>

//計算対象の登録
void AnimationManager::Register(
	uint32_t id,
	std::weak_ptr<AnimationSequencerComponent> sequencer,
	std::weak_ptr<ModelComponent> model,
	uint32_t parent_id)
{
	BS_PROFILE_FUNCTION();

	//ポインタの存在確認
	if (sequencer.expired() || model.expired())
	{
		LOG_WARNING({ u8"設定" }, u8"無効なシーケンサ、またはモデルが渡されたため登録をスキップ");
		return;
	}

	std::shared_ptr<ModelComponent> current_model = model.lock();

	//同一の実体が既に登録されているか確認
	for (auto& target : animation_targets)
	{
		std::shared_ptr<ModelComponent> existing_model = target.model_component.lock();
		if (existing_model && existing_model == current_model)
		{
			target.id = id;
			target.parent_id = parent_id;
			target.sequencer_component = sequencer;
			target.model_component = model;
			is_dirty = true;
			LOG_NORMAL({ u8"アニメーション", u8"登録" }, u8"同一モデルの実体を検出したため、登録情報を更新しました。ID: %u", id);
			return;
		}
	}

	//同一IDの衝突確認
	for (const auto& target : animation_targets)
	{
		if (target.id == id)
		{
			LOG_WARNING({ u8"アニメーション", u8"登録" }, u8"ID: %u が重複しています。GameObject側の object_id の採番を確認してください。", id);
			return;
		}
	}

	//リストに登録
	AnimationTarget new_target;
	new_target.id = id;
	new_target.parent_id = parent_id;
	new_target.sequencer_component = sequencer;
	new_target.model_component = model;

	animation_targets.push_back(new_target);
	is_dirty = true;

	LOG_SUCCESS({ u8"アニメーション", u8"登録" }, u8"アニメーション対象を登録しました。ID: %u, 現在の登録総数: %zu", id, animation_targets.size());
}

//アニメーション行列の計算
void AnimationManager::Update()
{
	BS_PROFILE_FUNCTION();

	const size_t prev_count = animation_targets.size();

	//削除されたオブジェクトをリストから削除
	animation_targets.erase(
		std::remove_if(animation_targets.begin(), animation_targets.end(),
			[](const AnimationTarget& target) {
				return target.sequencer_component.expired() || target.model_component.expired();
			}),
		animation_targets.end()
	);

	if (animation_targets.size() != prev_count)is_dirty = true;
	if (is_dirty)
	{
		SortTargets();
	}

	//ソート済みの順序で全オブジェクトの行列計算を一括実行
	for (const auto& target : animation_targets)
	{
		ExecuteAnimationCalculation(target);
	}
}

//登録リストのクリア
void AnimationManager::Clear()
{
	BS_PROFILE_FUNCTION();

	animation_targets.clear();
	is_dirty = false;
}

//親子関係に基づくソート
void AnimationManager::SortTargets()
{
	BS_PROFILE_FUNCTION();

	if (animation_targets.empty())return;

	//IDと親IDの紐づいたマップを作成
	std::unordered_map<uint32_t, uint32_t> parent_map;
	for (const auto& target : animation_targets)
	{
		parent_map[target.id] = target.parent_id;
	}

	constexpr int MAX_DEPENDENCY_DEPTH = 100; // 循環参照検知用の最大階層数

	//特定IDの階層の深さを算出するラムダ式
	auto GetDepth = [&](uint32_t id)->int
		{
			int depth = 0;
			uint32_t current_id = id;

			while (true)
			{
				if (depth > MAX_DEPENDENCY_DEPTH)
				{
					LOG_WARNING({ u8"検索" }, u8"親子関係の循環参照を検出");
					return 0;
				}

				auto it = parent_map.find(current_id);
				if (it == parent_map.end() || it->second == 0)break;
				current_id = it->second;
				depth++;
			}
			return depth;
		};

	//深さを事前登録してペアにする
	std::vector<std::pair<int, AnimationTarget>> depth_target_pairs;
	depth_target_pairs.reserve(animation_targets.size());
	for (const auto& target : animation_targets)
	{
		depth_target_pairs.emplace_back(GetDepth(target.id), target);
	}

	//階層が浅い順にソート
	std::sort(depth_target_pairs.begin(), depth_target_pairs.end(),
		[](const auto& a, const auto& b) {
			return a.first < b.first;
		});

	//ソートされた順序で元のリストを上書き
	animation_targets.clear();
	for (const auto& pair : depth_target_pairs)
	{
		animation_targets.push_back(pair.second);
	}
	is_dirty = false;
}

//単一ターゲットのアニメーション行列計算
void AnimationManager::ExecuteAnimationCalculation(const AnimationTarget& target)
{
	BS_PROFILE_FUNCTION();

	std::shared_ptr<AnimationSequencerComponent> sequencer = target.sequencer_component.lock();
	std::shared_ptr<ModelComponent> model_comp = target.model_component.lock();

	if (!sequencer || !model_comp)
	{
		LOG_WARNING({ u8"アニメーション", u8"計算" }, u8"ターゲットのコンポーネントが無効です。ID: %u", target.id);
		return;
	}

	GltfModel* model = model_comp->GetModel();

	if (!model)
	{
		LOG_WARNING({ u8"アニメーション", u8"計算" }, u8"GltfModel が存在しません。ID: %u", target.id);
		return;
	}

	if (!model->IsInFrustum())return;

	const AnimationPlaybackState& state = sequencer->GetPlaybackState();

	if (state.current_anim_name.empty())
	{
		return;
	}

	if (!std::isfinite(state.current_model_time))
	{
		LOG_WARNING({ u8"アニメーション", u8"計算" }, u8"再生時間(current_model_time)が不正な数値(NaN)です。モデル名: %s", state.current_anim_name.c_str());
		return;
	}

	std::shared_ptr<GltfModelData> data = model->GetData();
	if (data && data->HasBoneTexture() && !model->IsCpuAnimationEnabled())
	{
		model->SetPlaybackTimeOnly(state.current_model_time);
		return;
	}

	if (state.is_blending && !state.prev_anim_name.empty())
	{
		model->AnimateBlend(
			state.prev_anim_name,
			state.prev_model_time,
			state.current_anim_name,
			state.current_model_time,
			state.blend_factor
		);
	}
	else
	{
		model->SetAnimationTime(state.current_model_time);
	}
}
