#pragma once

#include "Serialization\AnimationSequenceSerializer.h"
#include "AnimationBlender.h"

#include <memory>
#include <string>
#include <unordered_map>

class Model;

class AnimationSequencerComponent
{
public:
	//デフォルトの経過時間
	static constexpr float DEFAULT_BLEND_TIME = 0.2f;

	//コンストラクタ
	explicit AnimationSequencerComponent(std::weak_ptr<Model> target_model);

	//デストラクタ
	~AnimationSequencerComponent();

	//初期化処理
	bool Initialize(const std::string& model_name);

	//更新処理
	void Update(float elapsed_time);

	//アニメーション切り替え
	void ChangeAnimation(const std::string& anim_name, float blend_time = DEFAULT_BLEND_TIME);

private:
	//指定時刻上の速度倍率を取得
	float GetSpeedMultiplierAt(float seq_time)const;

	//モデル再生時間を算出
	float GetIntegratedModelTime(float seq_time)const;

private:
	std::weak_ptr<Model> target_model;	//対象のモデル
	std::unique_ptr<AnimationBlender> animation_blender;					//補完計算クラス
	std::unordered_map <std::string, AnimationSequenceData> sequence_map;	//全アニメーションのシーケンス情報
	std::string current_animaiton_name;										//現在のアニメーション名
	float current_sequence_time = 0.0f;										//シーケンサ上の経過時間
	bool is_active = false;													//シーケンサフラグ
};

