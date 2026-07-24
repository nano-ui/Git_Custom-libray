#pragma once
#include "d3d11.h"
#include <directxmath.h>
#include <wrl.h>
#include <string>

class sprite
{
public:
	sprite(ID3D11Device* device, const wchar_t* filename);
	~sprite();
	//描画処理
	void render(ID3D11DeviceContext* immediate_context,
		float dx, float dy,		//短形の左上の座標(スクリーン座標系)
		float dw, float dh,		//短形のサイズ(スクリーン座標系)
		float r, float g, float b, float a,
		float angle/*degree*/,
		float sx, float sy, float sw, float sh);

	void render(ID3D11DeviceContext* immediate_context,
		float dx, float dy,		//短形の左上の座標(スクリーン座標系)
		float dw, float dh,		//短形のサイズ(スクリーン座標系)
		float r, float g, float b, float a,
		float angle/*degree*/);

	void textout(ID3D11DeviceContext* immediate_context,
		std::string s,
		float x, float y, float w, float h, float r, float g, float b, float a);

	HRESULT create_vs_from_cso(ID3D11Device*device,
		const char*cso_name,
		ID3D11VertexShader** vertex_shader,
		ID3D11InputLayout** input_layout,
		D3D11_INPUT_ELEMENT_DESC* input_element_desc,
		UINT num_elements);

	HRESULT create_ps_from_cso(ID3D11Device* device,
		const char* cso_name,
		ID3D11PixelShader** pixel_shader);

	

private:
	static void rotete(float& x, float& y, float cx, float cy, float angle, float cos, float sin)
		{
			//描画の中心を原点に移動
			x -= cx;
			y -= cy;

			//sin,cosの角度を求めてる

			//平行移動する値を入れている
			float tx{ x }, ty{ y };

			//座標を回転するためのアフィン変換行列を行っている
			x = cos * tx + -sin * ty;
			y = sin * tx + cos * ty;

			//元の描画位置に移動
			x += cx;
			y += cy;
		};

	//ID3D11VertexShader* vertex_shader;
	//ID3D11PixelShader*	pixel_shader;
	//ID3D11InputLayout*	input_layout;
	//ID3D11Buffer*		vertex_buffer;
	//ID3D11ShaderResourceView* shader_resource_view;

	Microsoft::WRL::ComPtr<ID3D11VertexShader> vertex_shader;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> pixel_shader;
	Microsoft::WRL::ComPtr<ID3D11InputLayout> input_layout;
	Microsoft::WRL::ComPtr<ID3D11Buffer> vertex_buffer;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> shader_resource_view;

	D3D11_TEXTURE2D_DESC texture2d_desc;
};

//頂点フォーマット
struct vertex
{
	DirectX::XMFLOAT3 position;
	DirectX::XMFLOAT4 color;
	DirectX::XMFLOAT2 texcoord;
};