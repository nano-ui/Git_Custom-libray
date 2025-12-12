#pragma once

#include <memory>
#include "DirectXDevice.h"
#include "PipelineStates.h"

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

	//コンポーネントへのアクセサ
	DirectXDevice* GetDirectXDevice() const { return directx_device.get(); }
	PipelineStates* GetPipelineStates()const { return pipline_states.get(); }

	//よく使う機能へのショートカット
	ID3D11Device* GetDevice()const { return directx_device->GetDevice().Get(); }
	ID3D11DeviceContext* GetContext()const { return directx_device->GetImmediateContext().Get(); }

private:
	Graphics() = default;
	~Graphics() = default;
	Graphics(const Graphics&) = default;
	void operator=(const Graphics&) = delete; 
private:
	std::unique_ptr<DirectXDevice> directx_device;
	std::unique_ptr<PipelineStates> pipline_states;
};

