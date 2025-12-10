#pragma once

#include <d3d11.h>
#include <wrl.h>
#include <DirectXMath.h>


struct SceneConstants
{
	DirectX::XMFLOAT4X4 view_projection;//ビュー・プロジェクション変換行列
	DirectX::XMFLOAT4 light_projection;	//ライトの向き
	DirectX::XMFLOAT4 camera_position;	//カメラの位置
};


class SceneConstantBuffers
{
public:
	SceneConstantBuffers(ID3D11Device* device);

	//SceneConstants用の定数バッファオブジェクトをGPU上に作成
	void CreateConstanceBuffer();

	//SceneConstantsのデータ（ビュー行列など）をCPU側で更新し、GPUに転送
	void UpdateConstants(ID3D11DeviceContext* context, const SceneConstants& data);

	Microsoft::WRL::ComPtr<ID3D11Buffer>& GetBuffer() { return scene_constants_buffer_; }
private:
	ID3D11Device* device_ptr_;										//定数バッファ作成に使用するID3D11Deviceへのポインタ
	Microsoft::WRL::ComPtr<ID3D11Buffer> scene_constants_buffer_;	//シーン全体の定数データを格納するGPU上のバッファ
};

