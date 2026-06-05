#include "sprite_batch.h"
#include "misc.h"
#include <sstream>
#include <WICTextureLoader.h>

//頂点情報のセット{位置情報,色}
//vertex vertices[]
//{
//	{{-1.0f,+1.0f,0},	{1,1,1,1},{0,0}},
//	{{+1.0f,-1.0f,0},	{1,1,1,1},{1,0}},
//	{{-1.0f,+1.0f,0},	{1,1,1,1},{0,1}},
//	{{+1.0f,-1.0f,0},	{1,1,1,1},{1,1}},
//};


//頂点の回転アフィン変換処理
void sprite_batch::RotateVertex(float& x, float& y, float cx, float cy, float angle)
{
	x -= cx;
	y -= cy;

	float cos_val{ cosf(DirectX::XMConvertToRadians(angle)) };
	float sin_val{ sinf(DirectX::XMConvertToRadians(angle)) };

	float tx = x;
	float ty = y;

	x = cos_val * tx + -sin_val * ty;
	y = sin_val * tx + cos_val * ty;

	x += cx;
	y += cy;
}

sprite_batch::sprite_batch(
	ID3D11Device* device,
	const wchar_t* filename,
	size_t max_sprites) :max_vertices(max_sprites * 6)
{
	HRESULT hr{ S_OK };

	//頂点バッファオブジェクトの生成
	D3D11_BUFFER_DESC buffer_desc{};
	buffer_desc.ByteWidth = sizeof(vertex) * max_vertices;
	buffer_desc.Usage = D3D11_USAGE_DYNAMIC;
	buffer_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	buffer_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	buffer_desc.MiscFlags = 0;
	buffer_desc.StructureByteStride = 0;
	hr = device->CreateBuffer(
		&buffer_desc,
		NULL,
		&vertex_buffer);

	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));



	//頂点シェーダオブジェクトの生成
	const char* cso_name{ "Shaders/Compiled/sprite_vs.cso" };

	FILE* fp{};
	fopen_s(&fp, cso_name, "rb");
	_ASSERT_EXPR_A(fp, "CSO File not found");

	fseek(fp, 0, SEEK_END);
	long cso_sz{ ftell(fp) };
	fseek(fp, 0, SEEK_SET);

	std::unique_ptr<unsigned char[]>cso_data{ std::make_unique<unsigned char[]>(cso_sz) };
	fread(cso_data.get(), cso_sz, 1, fp);
	fclose(fp);

	//HRESULT hr{ S_OK };
	hr = device->CreateVertexShader(cso_data.get(), cso_sz, nullptr, &vertex_shader);
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




	// ピクセルシェーダーの読み込み
	fopen_s(&fp, "Shaders/Compiled/sprite_ps.cso", "rb");
	_ASSERT_EXPR_A(fp, "CSO File not found");

	fseek(fp, 0, SEEK_END);
	cso_sz = { ftell(fp) };
	fseek(fp, 0, SEEK_SET);

	cso_data = { std::make_unique<unsigned char[]>(cso_sz) };
	fread(cso_data.get(), cso_sz, 1, fp);
	fclose(fp);

	hr = device->CreatePixelShader(cso_data.get(), cso_sz, nullptr, &pixel_shader);

	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));


	//テクスチャの読み込み
	ID3D11Resource* resource{};
	hr = DirectX::CreateWICTextureFromFile(
		device,
		filename,
		&resource,
		&shader_resource_view);


	ID3D11Texture2D* texture2d{};
	hr = resource->QueryInterface<ID3D11Texture2D>(&texture2d);
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));
	texture2d->GetDesc(&texture2d_desc);

	texture2d->Release();
	resource->Release();
}

sprite_batch::~sprite_batch()
{
}


void sprite_batch::render(ID3D11DeviceContext* immediate_context,
	float dx, float dy,		//矩形の左上の座標（スクリーン座標系）  
	float dw, float dh,		//矩形のサイズ（スクリーン座標系）
	float r, float g, float b, float a,
	float angle,
	float sx, float sy, float sw, float sh)
{
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
	RotateVertex(x0, y0, cx, cy, angle); 
	RotateVertex(x1, y1, cx, cy, angle); 
	RotateVertex(x2, y2, cx, cy, angle); 
	RotateVertex(x3, y3, cx, cy, angle); 

	static constexpr float logical_width = 1280.0f;
	static constexpr float logical_height = 720.0f;

	//各頂点をNDCにマッピング
	x0 = 2.0f * x0 / logical_width - 1.0f;
	y0 = 1.0f - 2.0f * y0 / logical_height;
	x1 = 2.0f * x1 / logical_width - 1.0f;
	y1 = 1.0f - 2.0f * y1 / logical_height;
	x2 = 2.0f * x2 / logical_width - 1.0f;
	y2 = 1.0f - 2.0f * y2 / logical_height;
	x3 = 2.0f * x3 / logical_width - 1.0f;
	y3 = 1.0f - 2.0f * y3 / logical_height;


	float U0{ sx / texture2d_desc.Width };
	float V0{ sy / texture2d_desc.Height };
	float U1{ (sx + sw) / texture2d_desc.Width };
	float V1{ (sy + sh) / texture2d_desc.Height };

	vertices.push_back({ {x0,y0,0},{r,g,b,a},{U0,V0} });
	vertices.push_back({ {x1,y1,0},{r,g,b,a},{U1,V0} });
	vertices.push_back({ {x2,y2,0},{r,g,b,a},{U0,V1} });
	vertices.push_back({ {x2,y2,0},{r,g,b,a},{U0,V1} });
	vertices.push_back({ {x1,y1,0},{r,g,b,a},{U1,V0} });
	vertices.push_back({ {x3,y3,0},{r,g,b,a},{U1,V1} });
}

void sprite_batch::render(
	ID3D11DeviceContext* immediate_context,
	float dx, float dy, float dw, float dh,
	float r, float g, float b, float a,
	float angle)
{
	render(
		immediate_context,
		dx, dy, dw, dh,
		r, g, b, a,
		angle,
		0.0f, 0.0f,
		static_cast<float>(texture2d_desc.Width),
		static_cast<float>(texture2d_desc.Height));
}

void sprite_batch::render(
	ID3D11DeviceContext* immediate_context, 
	float dx, float dy, float dw, float dh)
{
	render(
		immediate_context,
		dx,dy,		//短形の左上の座標(スクリーン座標系)
		dw,dh,		//短形のサイズ(スクリーン座標系)
		1.0f, 1.0f, 1.0f, 1.0f, 0.0f
	);
}

void sprite_batch::begin(ID3D11DeviceContext* immediate_context)
{
	vertices.clear();
	immediate_context->VSSetShader(vertex_shader.Get(), nullptr, 0);
	immediate_context->PSSetShader(pixel_shader.Get(), nullptr, 0);
	immediate_context->PSSetShaderResources(0, 1, shader_resource_view.GetAddressOf());
}

void sprite_batch::end(ID3D11DeviceContext* immediate_context)
{
	HRESULT hr{ S_OK };
	D3D11_MAPPED_SUBRESOURCE mapped_subresource{};
	hr = immediate_context->Map(vertex_buffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_subresource);
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

	size_t vertex_count = vertices.size();
	_ASSERT_EXPR(max_vertices >= vertex_count, "Buffer overflow");

	vertex* data{ reinterpret_cast<vertex*>(mapped_subresource.pData) };
	if (data != nullptr)
	{
		const vertex* p = vertices.data();
		memcpy_s(data, max_vertices * sizeof(vertex), p, vertex_count * sizeof(vertex));
	}
	immediate_context->Unmap(vertex_buffer.Get(), 0);
	UINT stride{ sizeof(vertex) };
	UINT offset{ 0 };
	immediate_context->IASetVertexBuffers(0, 1, vertex_buffer.GetAddressOf(), &stride, &offset);
	immediate_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	immediate_context->IASetInputLayout(input_layout.Get());
	immediate_context->Draw(static_cast<UINT>(vertex_count), 0);
}