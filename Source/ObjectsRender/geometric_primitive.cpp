#include "geometric_primitive.h"
#include "shader.h"
#include "misc.h"


geometric_primitive::geometric_primitive(ID3D11Device* device)
{

	vertex vertices[24] =
	{
		//前面
		{{-1,1,-1},{0,0,-1}},		//左上(0)
		{{1,1,-1},{0,0,-1}},		//右上(1)
		{{-1,-1,-1},{0,0,-1}},		//左下(2)
		{{1,-1,-1},{0,0,-1}},		//右下(3)

		//背面
		{{-1,1,1},{0,0,1}},			//左上(4)
		{{1,1,1},{0,0,1}},			//右上(5)
		{{-1,-1,1},{0,0,1}},		//左下(6)
		{{1,-1,1},{0,0,1}},			//右下(7)

		//右面
		{{1,1,-1},{1,0,0}},			//左上(8)
		{{1,1,1},{1,0,0}},			//右上(9)
		{{1,-1, -1},{1,0,0}},		//左下(10)
		{{1,-1,1},{1,0,0}},			//右下(11)

		//左面
		{{-1,1,1},{-1,0,0}},		//左上(12)
		{{-1,1,-1},{-1,0,0}},		//右上(13)
		{{-1,-1,1},{-1,0,0}},		//左下(14)
		{{-1,-1,-1},{-1,0,0}},		//右下(15)

		//上面
		{{-1,1,1},{0,1,0}},			//左上(16)
		{{1,1,1},{0,1,0}},			//右上(17)
		{{-1,1,-1},{0,1,0}},		//左下(18)
		{{1,1,-1},{0,1,0}},			//右下(19)

		//下面
		{{-1,-1,-1},{0,-1,0}},		//左上(20)
		{{1,-1,-1},{0,-1,0}},		//右上(21)
		{{-1,-1,1},{0,-1,0}},		//左下(22)
		{{1,-1,1},{0,-1,0}},		//右下(23)
	};
	/*サイズが1.0の正立方体データを作成する(重心を原点にする)。
	正立方体のコントルールポイント数は8個、
	1つのコントロールポイントの位置には法線の向きが違う頂点が3個あるので、
	頂点情報の総数は8*3=24
	頂点情報配列(vettices)にすべての頂点の位置・法線情報を格納する。*/

	uint32_t indices[36] =
	{
		//前面
		0,1,2,
		1,3,2,

		//背面
		4,6,5,
		5,6,7,

		//右面
		8,9,10,
		9,11,10,

		//左面
		12,13,14,
		13,15,14,

		//上面
		16,17,18,
		17,19,18,

		//下面
		20,21,22,
		21,23,22
	};
	/*正立方体は6面体持ち、1つの面は２つの3角形ポリゴンで構成されるので、
	3角形ポリゴンの総数は5*2=12個、
	正立方体を描画するために12回の3角形ポリゴン描画が必要、
	よって参照される頂点情報は12*3=36回、
	3角形ポリゴンが参照する頂点情報のインデックス(頂点番号)を描画順に配列(index)に格納する。
	時計回りが表面になるように格納すること。*/

	create_com_buffers(device, vertices, 24, indices, 36);

	HRESULT hr{ S_OK };

	D3D11_INPUT_ELEMENT_DESC input_element_desc[]
	{
		{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,
		D3D11_APPEND_ALIGNED_ELEMENT,D3D11_INPUT_PER_VERTEX_DATA,0},
		{ "NORMAl",0,DXGI_FORMAT_R32G32B32_FLOAT,0,
		D3D11_APPEND_ALIGNED_ELEMENT,D3D11_INPUT_PER_VERTEX_DATA,0 },
	};
	create_vs_from_cso(device,
		"geometric_primitive_vs.cso",
		vertex_shader.GetAddressOf(),
		input_layout.GetAddressOf(),
		input_element_desc,
		ARRAYSIZE(input_element_desc));

	create_ps_from_cso(
		device,
		"geometric_primitive_ps.cso",
		pixel_shader.GetAddressOf()
	);

	D3D11_BUFFER_DESC buffer_desc{};
	buffer_desc.ByteWidth = sizeof(constants);
	buffer_desc.Usage = D3D11_USAGE_DEFAULT;
	buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	hr = device->CreateBuffer(
		&buffer_desc,
		nullptr,
		constant_buffer.GetAddressOf());
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));
}

void geometric_primitive::render(
	ID3D11DeviceContext* immediate_context,
	const DirectX::XMFLOAT4X4& world,
	const DirectX::XMFLOAT4& material_color)
{
	uint32_t stride{ sizeof(vertex) };
	uint32_t offset{ 0 };
	immediate_context->IASetVertexBuffers(0, 1, vertex_buffer.GetAddressOf(), &stride, &offset);
	immediate_context->IASetIndexBuffer(index_buffer.Get(), DXGI_FORMAT_R32_UINT, 0);
	immediate_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	immediate_context->IASetInputLayout(input_layout.Get());

	immediate_context->VSSetShader(vertex_shader.Get(), nullptr, 0);
	immediate_context->PSSetShader(pixel_shader.Get(), nullptr, 0);

	constants data{ world,material_color };
	immediate_context->UpdateSubresource(constant_buffer.Get(), 0, 0, &data, 0, 0);
	immediate_context->VSSetConstantBuffers(0, 1, constant_buffer.GetAddressOf());

	D3D11_BUFFER_DESC buffer_besc{};
	index_buffer->GetDesc(&buffer_besc);
	immediate_context->DrawIndexed(buffer_besc.ByteWidth / sizeof(uint32_t), 0, 0);
}

void geometric_primitive::create_com_buffers(
	ID3D11Device* device,
	vertex* vertices,
	size_t vertex_count,
	uint32_t* indices,
	size_t index_count)
{
	HRESULT hr{ S_OK };

	D3D11_BUFFER_DESC buffer_desc{};
	D3D11_SUBRESOURCE_DATA subresource_data{};
	buffer_desc.ByteWidth = static_cast<UINT>(sizeof(vertex) * vertex_count);
	buffer_desc.Usage = D3D11_USAGE_DEFAULT;
	buffer_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	buffer_desc.CPUAccessFlags = 0;
	buffer_desc.MiscFlags = 0;
	buffer_desc.StructureByteStride = 0;
	subresource_data.pSysMem = vertices;
	subresource_data.SysMemPitch = 0;
	subresource_data.SysMemSlicePitch = 0;	
	hr = device->CreateBuffer(
		&buffer_desc,
		&subresource_data,
		vertex_buffer.GetAddressOf());
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

	buffer_desc.ByteWidth = static_cast<UINT>(sizeof(uint32_t) * index_count);
	buffer_desc.Usage = D3D11_USAGE_DEFAULT;
	buffer_desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	subresource_data.pSysMem = indices;
	hr = device->CreateBuffer(
		&buffer_desc,
		&subresource_data,
		index_buffer.GetAddressOf());
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));
}
