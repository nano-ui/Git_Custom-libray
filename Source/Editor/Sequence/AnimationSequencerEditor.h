#pragma once
#include "Serialization\AnimationSequenceSerializer.h"

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

class SequenceSceneBace;
struct ID3D11DeviceContext;

class AnimationSequencerEditor
{
public:
	//タイムリマップキーフレーム
	struct TimeMapKeyframe
	{
		float sequencer_time;		//シーケンサ上の再生経過時間
		float speed_multiplier;		//再生速度倍率
	};

	//コライダー有効区間トラック
	struct ColliderActiveTrack
	{
		std::string collider_name = "";	//コライダー名
		float start_time = 0.0f;		//開始時間
		float end_time = 0.0f;			//終了時間
	};

public:
	//コンストラクタ
	AnimationSequencerEditor();

	//デストラクタ
	~AnimationSequencerEditor();

	//初期化
	void Initialize();

	//更新
	void Update(float elapsed_time);

	//描画
	void Render(ID3D11DeviceContext* immediate_context);

	//ImGui描画
	void RenderGui();

	//現在の再生時刻に基づいてコライダー有効状態を同期
	void UpdateColliderActiveStates(float current_seq_time);

private:
	//キーフレームの初期化
	void InitializerTimeMap();

	//モデル時間を線形補間して算出
	float GetRemappedTime(float seq_time)const;

	//指定時刻における速度倍率を取得
	float GetSpeedMultiplierAt(float seq_time) const;

	//台形公式を用いて0秒から指定時刻までの速度倍率を積算し、モデル再生時間を算出
	float GetIntegratedModelTime(float seq_time) const;

	//速度カーブからモデルが完走するのに必要な実際の合計時間（実効総時間）を算出
	float GetEffectiveDuration() const;

	//現在編集中のタイムラインキーフレームをマップ構造体へ退避・同期
	void SaveCurrentSequenceDataToMap();

	//マップ構造体から現在選択中のアニメーションキーフレームへ復元
	void LoadCurrentSequenceDataFromMap();

	//タイムライン詳細トラックを描画し、ドラッグなどのマウス操作を行う
	void DrawTimelineTracks();

	//キーフレームをシーケンサ時間の昇順でソート
	static bool CompareKeyframes(const TimeMapKeyframe& a, const TimeMapKeyframe& b);

	//コライダー有効区間設定用UI描画
	void DrawColliderTracksGui();

private:
	//バーのドラッグ操作種別
	enum class DragMode
	{
		None,		//操作なし
		Move,		//バー全体の水平移動
		StartEdge,	//開始時間の伸縮
		EndEdge		//終了時間の伸縮
	};
	DragMode drag_mode = DragMode::None;	//現在のドラッグ状態
	float drag_start_mouse_x = 0.0f;		//ドラッグ開始時のマウスX座標
	float drag_initial_start_time = 0.0f;	//ドラッグ開始時の始点時間
	float drag_initial_end_time = 0.0f;		//ドラッグ開始時の終点時間

private:
	std::shared_ptr<SequenceSceneBace> active_scene;	//現在の画面

	float playback_speed = 1.0f;						//アニメーション再生速度
	bool is_playing = true;								//再生/一時停止フラグ
	bool is_loop = true;								//ループ再生フラグ
	float current_time = 0.0f;							//現在の再生時刻
	float animation_duration = 0.0f;					//アニメーションの総時間
	
	std::vector<TimeMapKeyframe> time_map_keyframes;	//タイムリマップキーフレーム配列
	int selected_keyframe_index = -1;					//ドラッグ移動中のキーフレーム番号

	std::unordered_map<std::string, AnimationSequenceData> all_sequences_map;	//モデルの全アニメーションシーケンス設定を保持
	std::string current_model_name = "DefaultModel";		//現在ロード中のモデル
	std::string current_animation_name = "DefaultAnim";		//現在再生中のアニメーション

	std::vector<ColliderActiveTrack> collider_tracks;		//コライダー有効区間トラック配列
	int selected_collider_trank_index = -1;					//選択中のコライダートラック番号
};

