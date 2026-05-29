#pragma once
#include "Scene.h"

#include <memory>

class ObjectManager;
class Camera;
class Light;

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
	std::unique_ptr<ObjectManager> object_manager;	//全ゲームオブジェクトを一括管理
	std::unique_ptr<Camera> camera;	//カメラ管理
	std::unique_ptr<Light> light;	//ライト管理
};

