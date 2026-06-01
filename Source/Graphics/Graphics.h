#pragma once

#include <memory>
#include <wrl.h>
#include <d3d11.h>
#include <DirectXMath.h>
#include "DirectXDevice.h"
#include "PipelineStates.h"

//シェーダーに渡すシーン共通の定数構造体
struct scene_constants
{
	DirectX::XMFLOAT4X4 view_projection; // ビュー行列とプロジェクション行列を合成した行列です
	DirectX::XMFLOAT4 light_direction;   // 平行光源の向きを表すベクトルです
	DirectX::XMFLOAT4 camera_position;   // カメラのワールド座標（w成分は1.0）です
};

class Graphics
{
public:
	//シングルトンインスタンス取得
	static Graphics& Instance();

	//初期化
	bool Initialize(HWND window_handle);

	//終了化
	void Finalize();

	//描画開始
	void BeginFrame(float r, float g, float b, float a);

	//描画終了
	void EndFrame();

	//シーン定数バッファを生成
	bool CreateSceneConstantBuffer();

	//シーン定数バッファを更新
	void UpdateSceneConstantBuffer(const scene_constants& constants);

	//コンポーネントへのアクセサ
	DirectXDevice* GetDirectXDevice() const { return directx_device.get(); }
	PipelineStates* GetPipelineStates()const { return pipline_states.get(); }

	//よく使う機能へのショートカット
	ID3D11Device* GetDevice()const { return directx_device->GetDevice().Get(); }
	ID3D11DeviceContext* GetContext()const { return directx_device->GetImmediateContext().Get(); }

	void SetViewMatrix(const DirectX::XMFLOAT4X4& view) { view_matrix = view; }
	void SetProjectionMatrix(const DirectX::XMFLOAT4X4& projection) { projection_matrix = projection; }
	const DirectX::XMFLOAT4X4& GetViewMatrix() const { return view_matrix; }
	const DirectX::XMFLOAT4X4& GetProjectionMatrix() const { return projection_matrix; }

private:
	Graphics() = default;
	~Graphics() = default;
	Graphics(const Graphics&) = default;
	void operator=(const Graphics&) = delete; 
private:
	std::unique_ptr<DirectXDevice> directx_device;
	std::unique_ptr<PipelineStates> pipline_states;
	Microsoft::WRL::ComPtr<ID3D11Buffer> scene_constant_buffer;
	DirectX::XMFLOAT4X4 view_matrix{};			//現在のビュー行列
	DirectX::XMFLOAT4X4 projection_matrix{};	//現在のプロジェクション行列
};

