#pragma once

#include <d3d11.h>
#include <wrl.h>
#include <string>
#include <memory>
#include "../Graphics/fullscreen_quad.h"

class PostProcess
{
public:
	//仮想デストラクタ
	virtual ~PostProcess() = default;

	//シェーダーや固有バッファの初期化を行う純粋仮想関数
	virtual void Initialize(ID3D11Device* device) = 0;

	//前段の結果(source)を受け取り、加工して次(dest)へ描き込む純粋仮想関数
	virtual void Render(
		ID3D11DeviceContext* context,
		ID3D11ShaderResourceView** source_str,
		ID3D11RenderTargetView* dest_rtv,
		fullscreen_quad* quad) = 0;
};