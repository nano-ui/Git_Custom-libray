#pragma once

#include <memory>
#include <thread>
#include <atomic>
#include <DirectXMath.h>
#include "Scene.h"

class sprite_batch;
class Camera;

// ローディング画面の描画と次シーンの非同期読み込みを管理するクラス
class SceneLoading : public Scene
{
public:
	// コンストラクタ（遷移先のシーンの所有権を受け取る）
	explicit SceneLoading(std::unique_ptr<Scene> next_scene);

	// デストラクタ
	~SceneLoading() override;

	// 初期化処理（スプライト生成と読み込みスレッドの開始）
	void Initialize() override;

	// 終了処理（スレッドの完了待機とリソース解放）
	void Finalize() override;

	// 更新処理（アイコンの回転と読み込み完了の監視）
	void Update(float elapsed_time) override;

	// 描画処理（ローディングアイコンとデバッグUIの描画）
	void Render(float elapsed_time) override;

	// ImGuiデバッグ描画
	void RenderGui() override;

private:
	// 別スレッドで次シーンのLoad()を実行する関数
	void LoadingThread();

	// スレッドが安全に終了するまで待機して破棄する関数
	void JoinLoadingThread();

private:
	static constexpr float default_rotation_speed = 180.0f;	// アイコンの初期回転速度（度/秒）
	static constexpr float default_icon_size = 256.0f;		// アイコンの初期表示サイズ
	static constexpr float icon_margin = 32.0f;				// 画面端からの余白
	float logical_screen_width = 1280.0f; //ゲームの設計上の基本横幅
	float logical_screen_height = 720.0f; //ゲームの設計上の基本縦幅

	std::unique_ptr<Scene> next_scene;				// 読み込み対象の次シーン
	std::unique_ptr<std::thread> loading_thread;	// 非同期読み込み用スレッド
	std::atomic<bool> is_load_completed;			// 読み込み完了フラグ（スレッド間共有）

	std::unique_ptr<Camera> camera;					// 2D描画用カメラ
	std::unique_ptr<sprite_batch> loading_sprite;	// ローディングアイコン用スプライト

	DirectX::XMFLOAT2 icon_pos;						// アイコンの描画位置
	DirectX::XMFLOAT2 icon_size;					// アイコンの描画サイズ
	DirectX::XMFLOAT4 icon_color;					// アイコンの描画色
	float angle;									// アイコンの現在の回転角度
	float rotation_speed;							// アイコンの回転速度
};