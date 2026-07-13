#pragma once

#include "SequenceSceneBace.h"

#include <memory>

class framebuffer;

class ModelPreviewScene : public SequenceSceneBace
{
public:
	//コンストラクタ
	ModelPreviewScene();

	//デストラクタ
	~ModelPreviewScene()override = default;

	//初期化
	void Initialize()override;

	//更新
	void Update(float elapsed_time)override;

	//ImGui描画
	void RenderGui()override;

private:
	std::unique_ptr<framebuffer> frame_buffer;	//描画バッファ
};

