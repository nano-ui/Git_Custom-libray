#pragma once

#include <memory>
#include <wrl.h>
#include <d3d11.h>
#include <DirectXMath.h>

#include "Scene.h"

class sprite_batch;
class Camera;

class SceneTitle : public Scene
{
public:
	//コンストラクタ
	SceneTitle();

	//デストラクタ
	~SceneTitle();

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
	std::unique_ptr<Camera> camera;				//カメラクラス
	std::unique_ptr<sprite_batch> title_sprite;	//タイトルロゴ
	DirectX::XMFLOAT2 screen_size;				//ウィンドウサイズ
	DirectX::XMFLOAT2 title_pos;				//タイトル位置
	DirectX::XMFLOAT4 color;					//画像の色
	float angle;								//画像の角度
};

 