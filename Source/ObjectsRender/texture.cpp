#include "texture.h"
#include "misc.h"
#include <WICTextureLoader.h>
#include <filesystem>
#include <DDSTextureLoader.h>


HRESULT load_texture_from_file(
    ID3D11Device* device,
    const wchar_t* filenama,
    ID3D11ShaderResourceView** shader_resource_view,
    D3D11_TEXTURE2D_DESC* texture2d_desc)
{
    HRESULT hr{ S_OK };
    ComPtr<ID3D11Resource> resource;

	std::filesystem::path dds_filename(filenama);
	dds_filename.replace_extension("dds");
    auto it = resources.find((filenama));
    if (it != resources.end())
    {
        *shader_resource_view = it->second.Get();
        (*shader_resource_view)->AddRef();
        (*shader_resource_view)->GetResource(resource.GetAddressOf());
    }
    else
    {
		/*テクスチャファイルを読み込んで DirectX で使える形に変換し、
		キャッシュに登録する処理*/

		/*指定したファイルパス（dds_filename）が存在するかをチェック*/
		if (std::filesystem::exists(dds_filename.c_str()))
		{
			/*画像ファイルを GPU が使える Direct3D のテクスチャリソース に変換*/
			hr = CreateDDSTextureFromFile(
				device,
				dds_filename.c_str(),
				resource.GetAddressOf(),
				shader_resource_view);
			_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));
		}
		else
		{
			hr = CreateWICTextureFromFile(
				device,
				filenama,
				resource.GetAddressOf(),
				shader_resource_view);
			_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));
		}
        resources.insert(make_pair(filenama, *shader_resource_view));
    }

    ComPtr<ID3D11Texture2D>texture2d;
    hr = resource.Get()->QueryInterface<ID3D11Texture2D>(texture2d.GetAddressOf());
    _ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));
    texture2d->GetDesc(texture2d_desc);

    return hr;
}

void release_all_textures()
{
    resources.clear();
}

HRESULT make_dummy_texture(
	ID3D11Device* device,
	ID3D11ShaderResourceView** shader_resource_view,
	DWORD value,
	UINT dimension)
{
	HRESULT hr{ S_OK };

	D3D11_TEXTURE2D_DESC texture2d_desc{};
	texture2d_desc.Width = dimension;
	texture2d_desc.Height = dimension;
	texture2d_desc.MipLevels = 1;
	texture2d_desc.ArraySize = 1;
	texture2d_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	texture2d_desc.SampleDesc.Count = 1;
	texture2d_desc.SampleDesc.Quality = 0;
	texture2d_desc.Usage = D3D11_USAGE_DEFAULT;
	texture2d_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

	size_t texels = dimension * dimension;
	unique_ptr<DWORD[]>sysmem{ make_unique<DWORD[]>(texels) };
	for (size_t i = 0; i < texels; i++) sysmem[i] = value;

	D3D11_SUBRESOURCE_DATA subresource_data{};
	subresource_data.pSysMem = sysmem.get();
	subresource_data.SysMemPitch = sizeof(DWORD) * dimension;

	ComPtr<ID3D11Texture2D> texture2d;
	hr = device->CreateTexture2D(
		&texture2d_desc,
		&subresource_data,
		&texture2d
	);
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

	D3D11_SHADER_RESOURCE_VIEW_DESC shader_resourec_view_desc{};
	shader_resourec_view_desc.Format = texture2d_desc.Format;
	shader_resourec_view_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	shader_resourec_view_desc.Texture2D.MipLevels = 1;
	hr = device->CreateShaderResourceView(
		texture2d.Get(),
		&shader_resourec_view_desc,
		shader_resource_view
	);
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

	return hr;
}

//========================================
//メモリ上のデータからテクスチャをロード
//========================================
HRESULT LoadTextureFromMemory(
	ID3D11Device* device,
	const uint8_t* data,
	size_t size, 
	ID3D11ShaderResourceView** shader_resource_view, 
	const std::wstring& cache_key)
{
	//---------------------
	//キャッシュの確認
	//---------------------

	auto it = resources.find(cache_key);	//登録済みの名前を探す
	//既にある場合
	if (it != resources.end())
	{
		*shader_resource_view = it->second.Get();	//ポインタをコピー
		(*shader_resource_view)->AddRef();	//参照カウンタを増やす
		return S_OK;
	}

	//------------------
	//テクスチャの生成
	//------------------

	HRESULT hr = CreateWICTextureFromMemory(device, data, size, nullptr, shader_resource_view);
	if (SUCCEEDED(hr))
	{
		//成功時
		resources.insert(std::make_pair(cache_key, *shader_resource_view));
	}
	return hr;
}
