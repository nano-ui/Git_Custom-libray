#include "sprite.h"
#include "misc.h"
#include <WICTextureLoader.h>
#include <sstream>
#include <memory>
#include <filesystem>

using namespace std;

//頂点情報のセット{位置情報,色}
vertex vertices[]
{
	{{-1.0f,+1.0f,0},	{1,1,1,1},{0,0}},
	{{+1.0f,-1.0f,0},	{1,1,1,1},{1,0}},
	{{-1.0f,+1.0f,0},	{1,1,1,1},{0,1}},
	{{+1.0f,-1.0f,0},	{1,1,1,1},{1,1}},
};


sprite::sprite(ID3D11Device* device, const wchar_t* filename)
{
	HRESULT hr{ S_OK };

	//頂点バッファオブジェクトの生成

	D3D11_BUFFER_DESC buffer_desc{};
	buffer_desc.ByteWidth = sizeof(vertices);
	buffer_desc.Usage = D3D11_USAGE_DYNAMIC;
	buffer_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	buffer_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	buffer_desc.MiscFlags = 0;
	buffer_desc.StructureByteStride = 0;
	D3D11_SUBRESOURCE_DATA subresource_data{};
	subresource_data.pSysMem = vertices;
	subresource_data.SysMemPitch = 0;
	subresource_data.SysMemSlicePitch = 0;
	hr = device->CreateBuffer(
		&buffer_desc,
		&subresource_data,
		vertex_buffer.GetAddressOf());

	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));


	//頂点シェーダオブジェクトの生成
	const char* cso_name{ "sprite_vs.cso" };
	std::filesystem::path fpath("Shaders/Compiled");
	fpath /= cso_name;

	FILE* fp{};
	fopen_s(&fp, fpath.string().c_str(), "rb");
	_ASSERT_EXPR_A(fp, "CSO File not found");

	fseek(fp, 0, SEEK_END);
	long cso_sz{ ftell(fp) };
	fseek(fp, 0, SEEK_SET);

	std::unique_ptr<unsigned char[]>cso_data{ std::make_unique<unsigned char[]>(cso_sz) };
	fread(cso_data.get(), cso_sz, 1, fp);
	fclose(fp);

	//HRESULT hr{ S_OK };
	hr = device->CreateVertexShader(cso_data.get(), cso_sz, nullptr, vertex_shader.GetAddressOf());
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

	//入力レイアウトオブジェクトの生成
	D3D11_INPUT_ELEMENT_DESC input_element_desc[]
	{
		{"POSITION",0, DXGI_FORMAT_R32G32B32_FLOAT, 0,
		D3D11_APPEND_ALIGNED_ELEMENT,D3D11_INPUT_PER_VERTEX_DATA,0},
		{"COLOR",0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,
		D3D11_APPEND_ALIGNED_ELEMENT,D3D11_INPUT_PER_VERTEX_DATA,0},
		{"TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,0,
		 D3D11_APPEND_ALIGNED_ELEMENT,D3D11_INPUT_PER_VERTEX_DATA,0},
	};
	hr = device->CreateInputLayout(input_element_desc, _countof(input_element_desc),
		cso_data.get(), cso_sz, &input_layout);
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

	std::string ps_name = "sprite_ps.cso";
	fpath = ("Shaders/Compiled");
	fpath /= ps_name;

	fopen_s(&fp, fpath.string().c_str(), "rb");
	_ASSERT_EXPR_A(fp, "CSO File not found");

	fseek(fp, 0, SEEK_END);
	cso_sz = { ftell(fp) };
	fseek(fp, 0, SEEK_SET);

	cso_data = { std::make_unique<unsigned char[]>(cso_sz) };
	fread(cso_data.get(), cso_sz, 1, fp);
	fclose(fp);

	hr = device->CreatePixelShader(cso_data.get(), cso_sz, nullptr, pixel_shader.GetAddressOf());

	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

	ID3D11Resource* resource{};
	hr = DirectX::CreateWICTextureFromFile(
		device,
		filename,
		&resource,
		shader_resource_view.GetAddressOf());

	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

	resource->Release();

	ID3D11Texture2D* texture2d{};
	hr = resource->QueryInterface<ID3D11Texture2D>(&texture2d);
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));
	texture2d->GetDesc(&texture2d_desc);

	texture2d->Release();



}

sprite::~sprite()
{
	//vertex_shader->Release();
	//pixel_shader->Release();
	//input_layout->Release();
	//vertex_buffer->Release();
}


void sprite::render(ID3D11DeviceContext* immediate_context,
	float dx, float dy,		//矩形の左上の座標（スクリーン座標系）  
	float dw, float dh,		//矩形のサイズ（スクリーン座標系）
	float r, float g, float b, float a,
	float angle,
	float sx, float sy, float sw, float sh)
{

	//スクリーン(ビューポート)のサイズを取得
	D3D11_VIEWPORT viewport{};
	UINT num_viewport{ 1 };
	immediate_context->RSGetViewports(&num_viewport, &viewport);

	//短形の各頂点座標を計算
	//		(x0, y0)*----* (x1, y1)  
	//				|   /|
	//				|  / |
	//				| /  |
	//				|/   |
	//		(x2, y2)*----* (x3, y3) 

	//left-top
	float x0{ dx };
	float y0{ dy };
	//right-top
	float x1{ dx + dw };
	float y1{ dy };
	//left-bottom
	float x2{ dx };
	float y2{ dy + dh };
	//right-bottom
	float x3{ dx + dw };
	float y3{ dy + dh };


	//回転の中心を短形の中心点にした場合
	float cx = dx + dw * 0.5f;
	float cy = dy + dh * 0.5f;
	float cos{ cosf(DirectX::XMConvertToRadians(angle)) };
	float sin{ sinf(DirectX::XMConvertToRadians(angle)) };
	rotete(x0, y0, cx, cy, angle, cos, sin);
	rotete(x1, y1, cx, cy, angle, cos, sin);
	rotete(x2, y2, cx, cy, angle, cos, sin);
	rotete(x3, y3, cx, cy, angle, cos, sin);



	//スクリーン座標系からNDCへの変換
	x0 = 2.0f * x0 / viewport.Width - 1.0f;
	y0 = 1.0f - 2.0f * y0 / viewport.Height;
	x1 = 2.0f * x1 / viewport.Width - 1.0f;
	y1 = 1.0f - 2.0f * y1 / viewport.Height;
	x2 = 2.0f * x2 / viewport.Width - 1.0f;
	y2 = 1.0f - 2.0f * y2 / viewport.Height;
	x3 = 2.0f * x3 / viewport.Width - 1.0f;
	y3 = 1.0f - 2.0f * y3 / viewport.Height;

	float U0, V0;
	U0 = sx / texture2d_desc.Width;
	V0 = sy / texture2d_desc.Height;

	float U1, V1;
	U1 = (sw + sx) / texture2d_desc.Width;
	V1 = (sh + sy) / texture2d_desc.Height;


	//計算結果で頂点バッファオブジェクトを更新
	HRESULT hr{ S_OK };
	D3D11_MAPPED_SUBRESOURCE mapped_subresource;
	hr = immediate_context->Map(vertex_buffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_subresource);
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

	vertex* vertices{ reinterpret_cast<vertex*>(mapped_subresource.pData) };
	if (vertices != nullptr)
	{
		vertices[0].position = { x0,y0,0 };
		vertices[1].position = { x1,y1,0 };
		vertices[2].position = { x2,y2,0 };
		vertices[3].position = { x3,y3,0 };
		vertices[0].color = vertices[1].color =
			vertices[2].color = vertices[3].color = { r,g,b,a };

		vertices[0].texcoord = { U0,V0 };
		vertices[1].texcoord = { U1,V0 };
		vertices[2].texcoord = { U0,V1 };
		vertices[3].texcoord = { U1,V1 };
	}

	immediate_context->Unmap(vertex_buffer.Get(), 0);

	//頂点バッファーのバインド(1頂点ごとのサイズ)
	UINT stride{ (sizeof(vertex)) };
	UINT offset{ 0 };
	immediate_context->IASetVertexBuffers(0, 1, vertex_buffer.GetAddressOf(), &stride, &offset);

	//プリミティブタイプおよびデータの順序に関する情報のバインド
	immediate_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	//入力レイアウトオブジェクトのバインド
	immediate_context->IASetInputLayout(input_layout.Get());

	//シェーダのバインド
	immediate_context->VSSetShader(vertex_shader.Get(), nullptr, 0);
	immediate_context->PSSetShader(pixel_shader.Get(), nullptr, 0);

	immediate_context->PSSetShaderResources(0, 1, shader_resource_view.GetAddressOf());

	//プリミティブの描画
	immediate_context->Draw(4, 0);
}

void sprite::render(
	ID3D11DeviceContext* immediate_context,
	float dx, float dy, float dw, float dh,
	float r, float g, float b, float a,
	float angle)
{
	//スクリーン(ビューポート)のサイズを取得
	D3D11_VIEWPORT viewport{};
	UINT num_viewport{ 1 };
	immediate_context->RSGetViewports(&num_viewport, &viewport);

	//短形の各頂点座標を計算
	//		(x0, y0)*----* (x1, y1)  
	//				|   /|
	//				|  / |
	//				| /  |
	//				|/   |
	//		(x2, y2)*----* (x3, y3) 

	//left-top
	float x0{ dx };
	float y0{ dy };
	//right-top
	float x1{ dx + dw };
	float y1{ dy };
	//left-bottom
	float x2{ dx };
	float y2{ dy + dh };
	//right-bottom
	float x3{ dx + dw };
	float y3{ dy + dh };

	//回転の中心を短形の中心点にした場合
	float cx = dx + dw * 0.5f;
	float cy = dy + dh * 0.5f;
	float cos{ cosf(DirectX::XMConvertToRadians(angle)) };
	float sin{ sinf(DirectX::XMConvertToRadians(angle)) };

	rotete(x0, y0, cx, cy, angle, cos, sin);
	rotete(x1, y1, cx, cy, angle, cos, sin);
	rotete(x2, y2, cx, cy, angle, cos, sin);
	rotete(x3, y3, cx, cy, angle, cos, sin);

	//スクリーン座標系からNDCへの変換
	x0 = 2.0f * x0 / viewport.Width - 1.0f;
	y0 = 1.0f - 2.0f * y0 / viewport.Height;
	x1 = 2.0f * x1 / viewport.Width - 1.0f;
	y1 = 1.0f - 2.0f * y1 / viewport.Height;
	x2 = 2.0f * x2 / viewport.Width - 1.0f;
	y2 = 1.0f - 2.0f * y2 / viewport.Height;
	x3 = 2.0f * x3 / viewport.Width - 1.0f;
	y3 = 1.0f - 2.0f * y3 / viewport.Height;


	//計算結果で頂点バッファオブジェクトを更新
	HRESULT hr{ S_OK };
	D3D11_MAPPED_SUBRESOURCE mapped_subresource;
	hr = immediate_context->Map(vertex_buffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_subresource);
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

	vertex* vertices{ reinterpret_cast<vertex*>(mapped_subresource.pData) };
	if (vertices != nullptr)
	{
		vertices[0].position = { x0,y0,0 };
		vertices[1].position = { x1,y1,0 };
		vertices[2].position = { x2,y2,0 };
		vertices[3].position = { x3,y3,0 };
		vertices[0].color = vertices[1].color =
			vertices[2].color = vertices[3].color = { r,g,b,a };

		vertices[0].texcoord = { 0,0 };
		vertices[1].texcoord = { 1,0 };
		vertices[2].texcoord = { 0,1 };
		vertices[3].texcoord = { 1,1 };
	}

	immediate_context->Unmap(vertex_buffer.Get(), 0);

	//頂点バッファーのバインド(1頂点ごとのサイズ)
	UINT stride{ (sizeof(vertex)) };
	UINT offset{ 0 };
	immediate_context->IASetVertexBuffers(0, 1, vertex_buffer.GetAddressOf(), &stride, &offset);

	//プリミティブタイプおよびデータの順序に関する情報のバインド
	immediate_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	//入力レイアウトオブジェクトのバインド
	immediate_context->IASetInputLayout(input_layout.Get());

	//シェーダのバインド
	immediate_context->VSSetShader(vertex_shader.Get(), nullptr, 0);
	immediate_context->PSSetShader(pixel_shader.Get(), nullptr, 0);

	immediate_context->PSSetShaderResources(0, 1, shader_resource_view.GetAddressOf());

	//プリミティブの描画
	immediate_context->Draw(4, 0);

}

void sprite::textout(
	ID3D11DeviceContext* immediate_context,
	std::string s, float x, float y, float w,float h,
	float r, float g, float b, float a)
{
	float sw = static_cast<float>(texture2d_desc.Width / 16);
	float sh = static_cast<float>(texture2d_desc.Height / 16);
	float carriage = 0;
	for (const char c : s)
	{
		render(immediate_context, x + carriage, y, w, h, r, g, b, a, 0,
			sw * (c & 0x0F), sh * (c >> 4), sw, sh);
		carriage += w;
	}
}

HRESULT sprite::create_vs_from_cso(
	ID3D11Device* device,
	const char* cso_name,
	ID3D11VertexShader** vertex_shader,
	ID3D11InputLayout** input_layout,
	D3D11_INPUT_ELEMENT_DESC* input_element_desc,
	UINT num_elements)
{
	FILE* fp{ nullptr };
	fopen_s(&fp, cso_name, "rb");
	_ASSERT_EXPR_A(fp, "CSO File not found");

	fseek(fp, 0, SEEK_END);
	long cso_sz{ ftell(fp) };
	fseek(fp, 0, SEEK_SET);

	unique_ptr<unsigned char[]>cso_data{ make_unique<unsigned char[]>(cso_sz) };
	fread(cso_data.get(), cso_sz, 1, fp);

	HRESULT hr{ S_OK };
	hr = device->CreateVertexShader(cso_data.get(), cso_sz, nullptr, vertex_shader);
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

	if (input_layout)
	{
		hr = device->CreateInputLayout(input_element_desc, num_elements,
			cso_data.get(), cso_sz, input_layout);
		_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));
	}

	return hr;
}

HRESULT sprite::create_ps_from_cso(
	ID3D11Device* device,
	const char* cso_name, 
	ID3D11PixelShader** pixel_shader)
{
	FILE* fp{ nullptr };
	fopen_s(&fp, cso_name, "rb");
	_ASSERT_EXPR_A(fp, "CSO File not found");

	fseek(fp, 0, SEEK_END);
	long cso_sz{ ftell(fp) };
	fseek(fp, 0, SEEK_SET);

	unique_ptr<unsigned char[]>cso_data{ make_unique<unsigned char[]>(cso_sz) };
	fread(cso_data.get(), cso_sz, 1, fp);

	HRESULT hr{ S_OK };
	hr = device->CreatePixelShader(cso_data.get(), cso_sz, nullptr, pixel_shader);
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

	return hr;
}
