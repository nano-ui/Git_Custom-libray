#pragma once

#include <memory>
#include <string>

class SequenceSceneBace;
class ModelPreviewScene;
struct ID3D11DeviceContext;

class AnimationSequencerEditor
{
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

private:
	std::shared_ptr<SequenceSceneBace> active_scene;	//現在の画面
	std::shared_ptr<ModelPreviewScene> preview_scene;	//モデル描画画面

	float playback_speed = 1.0f;						//アニメーション再生速度
	bool is_playing = true;								//再生/一時停止フラグ
	bool is_loop = true;								//ループ再生フラグ
	float current_time = 0.0f;							//現在の再生時刻
	float animation_duration = 0.0f;					//アニメーションの総時間
};

