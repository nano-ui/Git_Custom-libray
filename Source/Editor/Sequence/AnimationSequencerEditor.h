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
};

