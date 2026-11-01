#pragma once

#include <memory>
#include <vector>
#include <cstdint>

class AnimationSequencerComponent;
class ModelComponent;

//管理対象情報
struct AnimationTarget
{
	uint32_t id;		//自身のID
	uint32_t parent_id;	//親のID
	std::weak_ptr<AnimationSequencerComponent> sequencer_component;	//状態を持つシーケンサ
	std::weak_ptr<ModelComponent> model_component;					//対象のモデル

	AnimationTarget()
	{
		id = UINT32_MAX;
		parent_id = UINT32_MAX;
		sequencer_component.reset();
		model_component.reset();
	}
};

class AnimationManager
{
public:
	//シングルトン
	static AnimationManager& Instance()
	{
		static AnimationManager instance;
		return instance;
	}

	//計算対象の登録
	void Register(
		uint32_t id,
		std::weak_ptr<AnimationSequencerComponent> sequencer,
		std::weak_ptr<ModelComponent> model,
		uint32_t parent_id = 0);

	//アニメーション行列の計算
	void Update();

	//登録リストのクリア
	void Clear();

private:
	AnimationManager() = default;
	~AnimationManager() = default;
	AnimationManager(const AnimationManager&) = delete;
	AnimationManager& operator=(const AnimationManager&) = delete;

	//親子関係に基づくソート
	void SortTargets();

	//単一ターゲットのアニメーション行列計算
	void ExecuteAnimationCalculation(const AnimationTarget& target);

private:
	std::vector<AnimationTarget> animation_targets;	//計算対象のリスト
	bool is_dirty = false;;							//リストの変更フラグ
};

