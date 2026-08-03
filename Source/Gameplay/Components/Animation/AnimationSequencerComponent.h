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

	//アニメーション名取得
	std::string GetCurrentAnimationName()const { return current_animaiton_name; }

	//アニメーション再生時間の取得
	float GetCurrentSequenceTime()const { return current_sequence_time; }

	//シーケンサ基準のアニメーション終了判定を取得
	bool IsAnimationFinished()const { return ia_animation_finished; }

	//現在読み取っているシーケンサのファイルパスを取得
	const std::string& GetSequenceFilePath()const { return sequence_file_path; }

private:
	//指定時刻上の速度倍率を取得
	float GetSpeedMultiplierAt(float seq_time)const;

	//モデル再生時間を算出
	float GetIntegratedModelTime(float seq_time)const;

	//シーケンサ調整後の実効総時間を取得
	float GetEffectiveDuration()const;

private:
	std::weak_ptr<Model> target_model;	//対象のモデル
	std::unique_ptr<AnimationBlender> animation_blender;					//補完計算クラス
	std::unordered_map <std::string, AnimationSequenceData> sequence_map;	//全アニメーションのシーケンス情報
	std::string current_animaiton_name;										//現在のアニメーション名
	float current_sequence_time = 0.0f;										//シーケンサ上の経過時間
	bool is_active = false;													//シーケンサフラグ
	bool ia_animation_finished = false;										//アニメーション終了フラグ
	std::string sequence_file_path = "";									//シーケンサパス
};

