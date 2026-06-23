#pragma once

#include <d3d11.h>
#include <wrl.h>
#include <cstdint>

class framebuffer
{
public:
	framebuffer(ID3D11Device* device, uint32_t width, uint32_t height);
	virtual ~framebuffer() = default;

	//GPU が描画結果を出力するテクスチャ(色を描く)
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView>render_target_view;

	//深度 / ステンシルバッファ(奥行きを再現)
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView>depth_stencil_view;

	/*描画が終わったあと、
	このフレームバッファをテクスチャとしてシェーダに渡すためのビュー*/
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>shader_resource_views[2];

	//フレームバッファに描画する領域
	D3D11_VIEWPORT viewport;

	//初期化
	void clear(ID3D11DeviceContext* immediate_context,
		float = 0, float g = 0, float b = 0, float a = 1, float depth = 1);

	//「現在の描画先」として設定
	void activate(ID3D11DeviceContext* immediate_context);

	//activate で切り替えた描画先を 元に戻す
	void deactivate(ID3D11DeviceContext* immediate_context);

private:
	//activate 時に「元々の設定」をキャッシュしておくための変数

	//最大いくつまでビューポート数を保存するかの設定
	UINT viewport_count{ D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE };
	
	//実際に 現在設定されているビューポートの配列を保存 しておくためのバッファ
	D3D11_VIEWPORT cached_viewports[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE];
	
	//元の「レンダーターゲットビュー (RTV)」を保存
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> cached_render_target_view;

	//元の「深度ステンシルビュー (DSV)」を保存
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> cached_depth_stencil_view;
};

