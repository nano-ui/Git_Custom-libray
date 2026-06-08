#pragma once
#include "Scene.h"

#include <memory>
#include <vector>

class ObjectManager;
class Camera;
class Light;
class ShapeRenderer;
class CollisionManager;
class SkyBox;
class CollisionExperiment;
class ObjectEditor;

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
	enum class debug_shape_type
	{
		box,
		sphere,
		cylinder,
		capsule
	};

	struct debug_shape
	{
		debug_shape_type type;
		DirectX::XMFLOAT3 position;
		int draw_mode;
		DirectX::XMFLOAT4 color;
	};

private:
	std::unique_ptr<ObjectManager> object_manager;	//全ゲームオブジェクトを一括管理
	std::unique_ptr<Camera> camera;	//カメラ管理
	std::unique_ptr<Light> light;	//ライト管理
	std::unique_ptr<CollisionManager> collision_manager;	//当たり判定管理マネージャー
	std::unique_ptr<SkyBox> skybox;							//スカイボックスクラス

	std::unique_ptr<ShapeRenderer> shape_renderer;
	std::vector<debug_shape> debug_shapes;
	int current_debug_draw_mode = 0;
	DirectX::XMFLOAT4 current_debug_color = { 1.0f,1.0f,1.0f,1.0f };
	float k_shadow_area_size = 50.0f;

	std::unique_ptr<CollisionExperiment> collision_experiment;	//当たり判定実験クラス
	std::unique_ptr<ObjectEditor> object_editor;				//オブジェクトエディタクラス
};

