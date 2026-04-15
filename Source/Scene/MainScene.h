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

#include "../FbxModel/FbxSkinnedModel.h"
#include "../FbxModel/FbxSkinnedResource.h"

#include "../GltfModel/GltfMpdel.h"

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

private:
	std::shared_ptr<FbxSkinnedResource> resource;
	std::unique_ptr<FbxSkinnedModel> fbx_skinned_model;

	std::unique_ptr<GltfMpdel> gltf_models[8];

private:
	struct Transform
	{
		XMFLOAT3 position{ 0,0,0 };//位置
		XMFLOAT3 scale{ 1,1,1 };//スケール
		XMFLOAT3 rotation{ 0,0,0 };//回転
		float angleDeg = 0.0f;//2D用回転
		XMFLOAT4 color{ 1,1,1,1 };//色
	};

	Transform cube;
	Transform w_cube;
	Transform camera;

	ThresholdBuffer tb;
	BloomParams init;

	void editInformation
	(
		std::string label,
		Transform& t,
		float posRange = 1000.0f,
		float scaleRange = 10.0f
	);
	Microsoft::WRL::ComPtr<ID3D11Buffer>constnt_buffer[8];
	Microsoft::WRL::ComPtr<ID3D11PixelShader> pixel_shaders[8];

	Microsoft::WRL::ComPtr<ID3D11Buffer> thresholdBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> bloomParamBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> blurDirectionBuffer;//ブラー方向をシェーダーに渡すための定数バッファ

	BloomParams bloomParams = { 1.0f, 1.0f, {0.0f, 0.0f} };

	float Long = 300.0f;
	float Rotation = 1.5f;
	float supplementation = 0.5f;//補完率


	high_resolution_timer tictoc;
	uint32_t frames_per_second{ 0 };

	std::unique_ptr<sprite>sprites[8];
	std::unique_ptr<sprite_batch>sprute_batches[8];
	std::unique_ptr<static_mesh>static_meshes[8];

	std::unique_ptr<geometric_primitive>geometric_primitives[8];

	std::unique_ptr<skinned_mesh> skinned_meshes[8];

	std::unique_ptr<framebuffer> framebuffers[8];

	struct BlurDirection
	{
		DirectX::XMFLOAT2 direction;
		DirectX::XMFLOAT2 padding;
	};

	std::unique_ptr<fullscreen_quad> bit_block_transfer;


	DirectX::XMFLOAT3 Cubuposition{ -3,0,0 };
	DirectX::XMFLOAT3 Cubuposcale{ 0.4,0.4, 0.4 };
	DirectX::XMFLOAT3 Cuburotation;

	DirectX::XMFLOAT3 W_Cubuposition{ 3,0,0 };
	DirectX::XMFLOAT3 W_Cubuposcale{ 1,1, 1 };
	DirectX::XMFLOAT3 W_Cuburotation;

	DirectX::XMFLOAT2 position = { 0,1280 };
	DirectX::XMFLOAT2 size = { 0,720 };
	DirectX::XMFLOAT4 render_color = { 1,1,1,1 };
	DirectX::XMFLOAT4 camera_positon = { 0.0f,0.0f,10.0f,1.0f };

	float angle = 0.0;
	float count_by_seconds{ 0.0f };
	DirectX::XMFLOAT4 material_color = { 1,1,1,1 };



	void calculate_frame_stats()
	{
		if (++frames_per_second, (tictoc.time_stamp() - count_by_seconds) >= 1.0f)
		{
			float fps = static_cast<float>(frames_per_second);
			std::wostringstream outs;
			outs.precision(6);
			outs << L"X3DGP" << L" : FPS : " << fps << L" / " << L"Frame Time : " << 1000.0f / fps << L" (ms)";
			SetWindowTextW(hwnd, outs.str().c_str());

			frames_per_second = 0;
			count_by_seconds += 1.0f;
		}
	}

	struct scene_constants
	{
		//ビュー・プロジェクション変換行列
		DirectX::XMFLOAT4X4 view_projection;

		//ライトの向き
		DirectX::XMFLOAT4 light_direction;

		DirectX::XMFLOAT4 camera_position;
	};
	scene_constants data{};


};

