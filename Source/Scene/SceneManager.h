#pragma once

#include <memory>

class Scene;

class SceneManager
{
public:
	//シングルトンを取得
	static SceneManager& Instance();

	//シーン遷移
	void ChangeScene(std::unique_ptr<Scene> next_scene);

	//更新処理
	void Update(float elapsed_time);

	//描画処理
	void Render(float elapsed_time);

	//マネージャーの終了処理
	void Finalize();

private:
	//コンストラクタ
	SceneManager();

	//デストラクタ
	~SceneManager();

	//コピーと代入を禁止
	SceneManager(const SceneManager&) = delete;
	SceneManager& operator = (const SceneManager&) = delete;

private:
	std::unique_ptr<Scene> current_scene;	//実行中のシーン
	std::unique_ptr<Scene> reserved_scene;	//予約シーン
};

