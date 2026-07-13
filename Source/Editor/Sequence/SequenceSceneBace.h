#pragma once

class SequenceSceneBace
{
public:
	//仮想デストラクタ
	virtual ~SequenceSceneBace() = default;

	//初期化
	virtual void Initialize() = 0;

	//更新
	virtual void Update(float elapsed_time) = 0;

	//ImGui描画
	virtual void RenderGui() = 0;
};

