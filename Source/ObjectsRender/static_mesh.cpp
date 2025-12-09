#include "static_mesh.h"

#include "shader.h"
#include "misc.h"
#include "filesystem"

#include <vector>
#include <fstream>
#include "texture.h"

//UNIT16の⑤から(③を飛ばしているので聞きに行く)

static_mesh::static_mesh(
	ID3D11Device* device,
	const wchar_t* obj_fileneme)
{
	using namespace std;
	using namespace DirectX;

	vector<vertex>vertices;
	vector<uint32_t> indices;
	uint32_t current_index{ 0 };

	vector<XMFLOAT3> position;
	vector<XMFLOAT3> normals;
	vector<XMFLOAT2> texcoords;
	vector<wstring>  mtl_filenames;

	//バウンディングボックスの初期化
	min_pos = { FLT_MAX,FLT_MAX,FLT_MAX };
	max_pos = { FLT_MIN,FLT_MIN,FLT_MIN };

	wifstream fin{ obj_fileneme };
	_ASSERT_EXPR(fin, L"OBJ file not found.");
	wchar_t command[256];
	while (fin)
	{
		fin >> command;
		if (0 == wcscmp(command, L"v"))
		{
			float x, y, z;
			fin >> x >> y >> z;
			position.push_back({ x,y,z });

			//バウンディングボックスの更新
			min_pos.x = min(min_pos.x, x);
			min_pos.y = min(min_pos.y, y);
			min_pos.z = min(min_pos.z, z);

			max_pos.x = max(max_pos.x, x);
			max_pos.y = max(max_pos.y, y);
			max_pos.z = max(max_pos.z, z);


			fin.ignore(1024, L'\n');

		}
		else if (0 == wcscmp(command, L"vn"))
		{
			float i, j, k;
			fin >> i >> j >> k;
			normals.push_back({ i,j,k });
			fin.ignore(1024, L'\n');
		}
		else if (0 == wcscmp(command, L"vt"))
		{
			float u, v;
			fin >> u >> v;
			texcoords.push_back({ u,1.0f - v });
			fin.ignore(1024, L'\n');
		}
		else if (0 == wcscmp(command, L"f"))
		{
			for (size_t i = 0; i < 3; i++)
			{
				vertex vertex;
				size_t v, vt, vn;

				fin >> v;
				vertex.position = position.at(v - 1);
				if (L'/' == fin.peek())
				{
					fin.ignore(1);
					if (L'/' != fin.peek())
					{
						fin >> vt;
						vertex.texcord = texcoords.at(vt - 1);
					}
					if (L'/' == fin.peek())
					{
						fin.ignore(1);
						fin >> vn;
						vertex.normal = normals.at(vn - 1);
					}
				}
				vertices.push_back(vertex);
				indices.push_back(current_index++);
			}
			fin.ignore(1024, L'\n');
		}
		else if (0 == wcscmp(command, L"mtllib"))
		{
			wchar_t mtllib[256];
			fin >> mtllib;
			mtl_filenames.push_back(mtllib);
		}
		else if (0 == wcscmp(command, L"usemtl"))
		{
			wchar_t usemtl[MAX_PATH]{ 0 };
			fin >> usemtl;
			subsets.push_back({ usemtl,static_cast<uint32_t>(indices.size()),0 });
		}
		else
		{
			fin.ignore(1024, L'\n');
		}
	}
	fin.close();

	std::vector<subset>::reverse_iterator iterator = subsets.rbegin();
	iterator->index_count = static_cast<uint32_t>(indices.size()) - iterator->index_start;
	for (iterator = subsets.rbegin() + 1; iterator != subsets.rend(); iterator++)
	{
		iterator->index_count = (iterator - 1)->index_start - iterator->index_start;
	}

	filesystem::path mtl_filename(obj_fileneme); // resources\\Cup\\cup.obj
	mtl_filename.replace_filename(filesystem::path(mtl_filenames[0]).filename()); //resources\\Cup\\cup.mtl

	if (materials.size() == 0)
	{
		for (const subset& subset : subsets)
		{
			materials.push_back({ subset.usimtl });
		}
	}


	fin.open(mtl_filename);
	_ASSERT_EXPR(fin, L"'MTL file not found.");

	while (fin)
	{
		fin >> command;
		if (0 == wcscmp(command, L"map_Kd"))
		{
			fin.ignore();
			wchar_t map_kd[256];
			fin >> map_kd;

			filesystem::path path(obj_fileneme);
			path.replace_filename(filesystem::path(map_kd).filename());
			materials.rbegin()->texture_filename[0] = path;
			fin.ignore(1024, L'\n');
		}
		else if (0 == wcscmp(command, L"map_bump") || 0 == wcscmp(command, L"bump"))
		{
			fin.ignore();
			wchar_t map_bump[256];
			fin >> map_bump;
			filesystem::path path(obj_fileneme);
			path.replace_filename(filesystem::path(map_bump).filename());
			materials.rbegin()->texture_filename[1] = path;
			fin.ignore(1024, L'\n');
		}
		else if (0 == wcscmp(command, L"newmtl"))
		{
			fin.ignore();
			wchar_t newmtl[256];
			material material;
			fin >> newmtl;
			material.name = newmtl;
			materials.push_back(material);
		}
		else if (0 == wcscmp(command, L"Kd"))
		{
			float r, g, b;
			fin >> r >> g >> b;
			materials.rbegin()->kd = { r,g,b,1 };
			fin.ignore(1024, L'\n');
		}
		else
		{
			fin.ignore(1024, L'\n');
		}
	}
	fin.close();


	//load_texture_from_file(
	//	device,
	//	materials.rbegin()->texture_filename[0].c_str(),
	//	materials.rbegin()->shader_resource_view[0].GetAddressOf(),
	//	&texture2d_desc
	//);

//テクスチャのロード、シェーダーリソースビューオブジェクトの生成
	D3D11_TEXTURE2D_DESC texture2d_desc{};
	for (material& material : materials)
	{
		if (material.texture_filename[0].size() > 0)
		{
			load_texture_from_file
			(
				device,
				materials.rbegin()->texture_filename[0].c_str(),
				materials.rbegin()->shader_resource_view[0].GetAddressOf(),
				&texture2d_desc
			);
		}
		if (material.texture_filename[1].size() > 1)
		{
			load_texture_from_file
			(
				device,
				materials.rbegin()->texture_filename[1].c_str(),
				materials.rbegin()->shader_resource_view[1].GetAddressOf(),
				&texture2d_desc
			);
		}

		if (material.texture_filename[0].size() < 0)
		{
			make_dummy_texture(
				device,
				material.shader_resource_view[0].GetAddressOf(),
				0xFFFFFFFF,
				16
			);

			make_dummy_texture(
				device,
				material.shader_resource_view[1].GetAddressOf(),
				0xFFFF7F7F,
				16
			);
		}
	}

	create_com_buffers(
		device,
		vertices.data(),
		vertices.size(),
		indices.data(),
		indices.size()
	);

	HRESULT hr{ S_OK };

	D3D11_INPUT_ELEMENT_DESC input_element_desc[]
	{
		{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,
		D3D11_APPEND_ALIGNED_ELEMENT,D3D11_INPUT_PER_VERTEX_DATA,0},
		{ "NORMAL",0,DXGI_FORMAT_R32G32B32_FLOAT,0,
		D3D11_APPEND_ALIGNED_ELEMENT,D3D11_INPUT_PER_VERTEX_DATA,0 },
		{ "TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,0,
		D3D11_APPEND_ALIGNED_ELEMENT,D3D11_INPUT_PER_VERTEX_DATA,0 },
	};
	create_vs_from_cso(device,
		"static_mesh_vs.cso",
		vertex_shader.GetAddressOf(),
		input_layout.GetAddressOf(),
		input_element_desc,
		ARRAYSIZE(input_element_desc));

	create_ps_from_cso(
		device,
		"static_mesh_ps.cso",
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

void static_mesh::render(
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

	for (const material& material : materials)
	{
		immediate_context->PSSetShaderResources(0, 1, material.shader_resource_view[0].GetAddressOf());
		immediate_context->PSSetShaderResources(1, 1, material.shader_resource_view[1].GetAddressOf());
		constants data{ world,material_color };
		XMStoreFloat4(&data.material_color, XMLoadFloat4(&material_color) * XMLoadFloat4(&material.kd));
		immediate_context->UpdateSubresource(constant_buffer.Get(), 0, 0, &data, 0, 0);
		immediate_context->VSSetConstantBuffers(0, 1, constant_buffer.GetAddressOf());

		for (const subset& subset : subsets)
		{
			if (material.name == subset.usimtl)
			{
				immediate_context->DrawIndexed(subset.index_count, subset.index_start, 0);
			}
		}
	}
}


void static_mesh::create_com_buffers(
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
