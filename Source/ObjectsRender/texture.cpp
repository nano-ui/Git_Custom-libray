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
		/*テクスチャファイルを読み込んで DirectX で使う形に変換して、
		キャッシュに登録する処理*/

		/*指定したファイルパス(dds_filename)が存在するかをチェック*/
		if (std::filesystem::exists(dds_filename.c_str()))
		{
			/*画像ファイルを GPU で使える Direct3D のテクスチャリソース に変換*/
			hr = CreateDDSTextureFromFile(
				device,
				dds_filename.c_str(),
				resource.GetAddressOf(),
				shader_resource_view);

			//エラーチェックを追加
			if (FAILED(hr))
			{
				OutputDebugStringA("WARNING: Failed to load DDS texture, trying WIC format\n");
				// DDS読み込み失敗時はWICで試す
				hr = CreateWICTextureFromFile(
					device,
					filenama,
					resource.GetAddressOf(),
					shader_resource_view);
			}
		}
		else
		{
			hr = CreateWICTextureFromFile(
				device,
				filenama,
				resource.GetAddressOf(),
				shader_resource_view);
		}

		//リソース取得に失敗した場合の処理を追加
		if (SUCCEEDED(hr) && resource == nullptr)
		{
			if (*shader_resource_view != nullptr)
			{
				(*shader_resource_view)->GetResource(resource.GetAddressOf());
			}
		}

		if (FAILED(hr))
		{
			OutputDebugStringA("ERROR: Failed to load texture from file\n");
			return hr;
		}

		//キャッシュに登録（成功した場合のみ）
		if (*shader_resource_view != nullptr)
		{
			resources.insert(make_pair(filenama, *shader_resource_view));
		}
	}

	//resourceが有効か確認してからQueryInterface
	if (resource == nullptr)
	{
		if (*shader_resource_view != nullptr)
		{
			(*shader_resource_view)->GetResource(resource.GetAddressOf());
		}
	}

	if (resource != nullptr)
	{
		ComPtr<ID3D11Texture2D> texture2d;
		hr = resource.Get()->QueryInterface<ID3D11Texture2D>(texture2d.GetAddressOf());

		if (SUCCEEDED(hr) && texture2d != nullptr)
		{
			texture2d->GetDesc(texture2d_desc);
		}
		else
		{
			OutputDebugStringA("WARNING: Failed to query Texture2D interface\n");
			// デフォルト値を設定
			if (texture2d_desc != nullptr)
			{
				ZeroMemory(texture2d_desc, sizeof(D3D11_TEXTURE2D_DESC));
				texture2d_desc->Width = 1;
				texture2d_desc->Height = 1;
			}
		}
	}

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