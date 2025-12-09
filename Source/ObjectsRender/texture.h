#pragma once

#include <WICTextureLoader.h>
using namespace DirectX;

#include <wrl.h>
using namespace Microsoft::WRL;

#include <string>
#include <map>
#include <memory>
using namespace std;

static map<wstring, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>> resources;

HRESULT load_texture_from_file(
	ID3D11Device* device,
	const wchar_t* filenema,
	ID3D11ShaderResourceView** shader_resource_view,
	D3D11_TEXTURE2D_DESC* textur2d_desc);

void release_all_textures();

HRESULT make_dummy_texture(
	ID3D11Device* device,
	ID3D11ShaderResourceView** shader_resource_view,
	DWORD value/*0xAABBGGRR*/,
	UINT dimension
);

