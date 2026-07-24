#include "AnimationSequencerComponent.h"
#include "Engine\Graphics\Resources\Model.h"

#include <windows.h>
#include <cstdio>

static constexpr float DEFAULT_SPEED_MULTIPLIER = 1.0f;	//標準速度倍率

//コンストラクタ
AnimationSequencerComponent::AnimationSequencerComponent(std::weak_ptr<Model> target_model)
	:target_model(target_model)
	, current_sequence_time(0.0f)
	, is_active(false)
{
}

//デストラクタ
AnimationSequencerComponent::~AnimationSequencerComponent() = default;

//初期化処理
bool AnimationSequencerComponent::Initialize(const std::string& model_name)
{
	sequence_map.clear();
	current_sequence_time = 0.0f;
	is_active = false;

	if (model_name.empty())
	{
		OutputDebugStringA("[SequencerComponent Warning] Initialize: model_name is empty!\n");
		return false;
	}

	//JSONファイルから指定モデルの全シーケンス設定を一括ロード
	if (AnimationSequenceSerializer::LoadFromFile(model_name, sequence_map))
	{
		is_active = true;
		char log_buf[256];
		sprintf_s(log_buf, "[SequencerComponent] Successfully initialized for model: %s\n", model_name.c_str());
		OutputDebugStringA(log_buf);
		return true;
	}

	OutputDebugStringA("[SequencerComponent Warning] Failed to load sequence data or file not found.\n");
	return false;
}

//更新処理
void AnimationSequencerComponent::Update(float elapsed_time)
{
	if (!is_active || current_animaiton_name.empty())return;

	//スマートポインタの昇格確認
	std::shared_ptr<Model> shared_model = target_model.lock();
	if (!shared_model)
	{
		OutputDebugStringA("[SequencerComponent Error] Update: target_model has been expired!\n");
		return;
	}

	//経過時間を加算
	current_sequence_time += elapsed_time;

	//現在のアニメーションに対応する速度カーブからモデル再生時間を算出
	float integrated_time = GetIntegratedModelTime(current_sequence_time);

	//モーションの終了時にシーケンサ時間をリセット
	auto it = sequence_map.find(current_animaiton_name);
	if (it != sequence_map.end())
	{
		float anim_duration = it->second.animation_duration;
		float eff_duration = it->second.effective_duration;

		if ((anim_duration > 0.0f && integrated_time >= anim_duration) ||
			(eff_duration > 0.0f && current_sequence_time >= eff_duration))
		{
			current_sequence_time = 0.0f;
			integrated_time = 0.0f;
		}
	}
	else
	{
		OutputDebugStringA(u8"[SequencerComponent Warning] Update: sequence_map に該当アニメーションデータが存在しません。\n");
	}

	//計算した時間をモデルへ設定
	shared_model->SetAnimationTime(integrated_time);
}

//アニメーション切り替え
void AnimationSequencerComponent::ChangeAnimation(const std::string& anim_name)
{
	if (current_animaiton_name == anim_name)return;
	current_animaiton_name = anim_name;
	current_sequence_time = 0.0f;
}

//指定時刻上の速度倍率を取得
float AnimationSequencerComponent::GetSpeedMultiplierAt(float seq_time) const
{
	auto it = sequence_map.find(current_animaiton_name);
	if (it == sequence_map.end() || it->second.keyframes.empty())return DEFAULT_SPEED_MULTIPLIER;
	const std::vector<SequenceKeyframe>& keyframes = it->second.keyframes;
	if (seq_time <= keyframes.front().sequencer_time)return keyframes.front().speed_multiplier;
	if (seq_time >= keyframes.back().sequencer_time)return keyframes.back().speed_multiplier;

	for (size_t i = 0; i < keyframes.size() - 1; i++)
	{
		const SequenceKeyframe& kf0 = keyframes[i];
		const SequenceKeyframe& kf1 = keyframes[i + 1];

		if (seq_time >= kf0.sequencer_time && seq_time <= kf1.sequencer_time)
		{
			float duration = kf1.sequencer_time - kf0.sequencer_time;
			if (duration <= 0.00001f)return kf0.speed_multiplier;
			float rate = (seq_time - kf0.sequencer_time) / duration;
			return kf0.speed_multiplier + rate * (kf1.speed_multiplier - kf0.speed_multiplier);
		}
	}
	return DEFAULT_SPEED_MULTIPLIER;
}

//モデル再生時間を算出
float AnimationSequencerComponent::GetIntegratedModelTime(float seq_time) const
{
	auto it = sequence_map.find(current_animaiton_name);
	if (it == sequence_map.end() || it->second.keyframes.empty())return seq_time;
	const std::vector<SequenceKeyframe>& keyframes = it->second.keyframes;
	float total_model_time = 0.0f;

	//指定時間までの区間を巡回して台形公式で面積を加算
	for (size_t i = 0; i < keyframes.size() - 1; i++)
	{
		const SequenceKeyframe& kf0 = keyframes[i];
		const SequenceKeyframe& kf1 = keyframes[i + 1];
		
		if (seq_time <= kf0.sequencer_time)break;

		float t_start = kf0.sequencer_time;
		float t_end = (seq_time < kf1.sequencer_time) ? seq_time : kf1.sequencer_time;
		float dt = t_end - t_start;

		if (dt > 0.0f)
		{
			float s_start = GetSpeedMultiplierAt(t_start);
			float s_end = GetSpeedMultiplierAt(t_end);
			float segment_area = ((s_start + s_end) * 0.5f) * dt;
			total_model_time += segment_area;
		}
	}
	return total_model_time;
}