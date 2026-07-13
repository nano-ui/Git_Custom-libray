#pragma once

struct ID3D11DeviceContext;

class SequenceSceneBace
{
public:
	//仮想デストラクタ
	virtual ~SequenceSceneBace() = default;

	//初期化
	virtual void Initialize() = 0;

	//更新
	virtual void Update(float elapsed_time) = 0;

	//描画処理
	virtual void Render(ID3D11DeviceContext* immediate_context) = 0;

	//ImGui描画
	virtual void RenderGui() = 0;
};

