#pragma once

#include <memory>
#include <wrl.h>
#include <d3d11.h>
#include <DirectXMath.h>
#include <string>

#include "../Scene/Scene.h"
#include "../ObjectsRender/sprite.h"
#include "../ObjectsRender/sprite_batch.h"
#include "../ObjectsRender/static_mesh.h"
#include "../ObjectsRender/skinned_mesh.h"
#include "../ObjectsRender/geometric_primitive.h"
#include "../Graphics/framebuffer.h"
#include "../Graphics/fullscreen_quad.h"
#include "../Graphics/Graphics.h"
#include "../Graphics/PostProcessConstantBuffers.h"
#include "../Common/high_resolution_timer.h"

#include "../ObjectsRender/Model.h"
#include "../Camera/Camera.h"
#include "../Light/Light.h"
#include "../Shaders/BasicShader.h"
#include "../Shaders/ShaderManager.h"

class MainScene :public Scene
{
public:

	HWND hwnd;

	//コンストラクタ
	MainScene();

	//デストラクタ
	~MainScene() override;

	//初期化
	void Initialize() override;

	//終了処理
	void Finalize() override;

	//更新処理
	void Update(float elapsed_time) override;

	//描画処理
	void Render(float elapsed_time) override;

	//シーン定数バッファ取得
	scene_constants GetSceneConstant() const override { return data; }

private:
	std::unique_ptr<Model> fbx_model;						//FBXモデルを管理するスマートポインタ
	std::unique_ptr<Model> gltf_model;						//glTFモデルを管理するスマートポインタ
	std::unique_ptr<static_mesh> static_meshes[8];			//静的メッシュ配列
	std::unique_ptr<skinned_mesh> skinned_meshes[8];		//スキンメッシュ配列

	// --------------------------------------------------
	// -- 新しく追加した管理クラスのスマートポインタ --
	// --------------------------------------------------
	std::unique_ptr<Camera> camera_component;				//カメラ機能を管理するポインタ
	std::unique_ptr<Light> light_component;					//ライト機能を管理するポインタ
	std::unique_ptr<ShaderManager> shader_manager;			//シェーダーとポストプロセスを管理するポインタ

	DirectX::XMFLOAT3 Cubuposition{ -3,0,0 };				//キューブの座標
	DirectX::XMFLOAT3 Cubuposcale{ 0.4,0.4, 0.4 };			//キューブのスケール
	DirectX::XMFLOAT3 Cuburotation;							//キューブの回転
	DirectX::XMFLOAT3 W_Cubuposition{ 3,0,0 };				//W_キューブの座標
	DirectX::XMFLOAT3 W_Cubuposcale{ 1,1, 1 };				//W_キューブのスケール
	DirectX::XMFLOAT3 W_Cuburotation;						//W_キューブの回転
	DirectX::XMFLOAT2 position = { 0,1280 };				//位置情報
	DirectX::XMFLOAT2 size = { 0,720 };						//サイズ情報
	DirectX::XMFLOAT4 render_color = { 1,1,1,1 };			//描画カラー
	DirectX::XMFLOAT4 camera_positon = { 0.0f,0.0f,10.0f,1.0f };//カメラ座標
	float angle = 0.0;										//角度
	float count_by_seconds{ 0.0f };							//秒数カウント
	DirectX::XMFLOAT4 material_color = { 1,1,1,1 };			//マテリアルカラー
	scene_constants data;
};

