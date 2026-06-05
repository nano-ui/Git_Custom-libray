#include "SceneManager.h"
#include "Scene.h"
#include "../Input/Input.h"

//シングルトンの取得
SceneManager& SceneManager::Instance()
{
	static SceneManager instance;
	return instance;
}

//コンストラクタ
SceneManager::SceneManager()
{

}

//デストラクタ
SceneManager::~SceneManager()
{
	Finalize();
}

//シーン切り替え
void SceneManager::ChangeScene(std::unique_ptr<Scene> next_scene)
{
	reserved_scene = std::move(next_scene);
}

//更新処理
void SceneManager::Update(float elapsed_time)
{
	//F5キーによる一時停止のトグル切り替え判定
	if (Input::Instance().IsKeyTrigger(VK_F5))
	{
		is_paused = !is_paused;
	}

	//シーン遷移の予約がある場合は入れ替え
	if (reserved_scene)
	{
		if (current_scene)
		{
			current_scene->Finalize();
		}
		current_scene = std::move(reserved_scene);
		current_scene->Initialize();
	}

	//実行中のシーンを更新
	if (current_scene)
	{
		current_scene->Update(elapsed_time);
	}
}

//描画処理
void SceneManager::Render(float elapsed_time)
{
	if (current_scene)
	{
		current_scene->Render(elapsed_time);
	}
}

//マネージャーの終了処理
void SceneManager::Finalize()
{
	//保持している全シーンの明示的破棄
	if (current_scene)
	{
		current_scene->Finalize();
		current_scene.reset();
	}
	if (reserved_scene)
	{
		reserved_scene.reset();
	}
}

