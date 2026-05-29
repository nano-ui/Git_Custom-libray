#pragma once
#include "Scene.h"

class SceneGame : public Scene
{
public:
	//コンストラクタ
	SceneGame();

	//デストラクタ
	~SceneGame();

	//初期化
	void Initialize()override;

	//終了化
	void Finalize()override;

	//更新処理
	void Update(float elapsed_time)override;

	//描画処理
	void Render(float elapsed_time)override;

	//ImGuiデバッグ描画
	void RenderGui()override;

private:

};

