#pragma once

#include <wrl.h>
#include <string>
#include <d3d11.h>
#include <d3dcompiler.h>

struct ID3D11VertexShader;
struct ID3D11PixelShader;
struct ID3D11InputLayout;

class CustomShader
{
public:
	//指定されたファイル名からシェーダーを生成し初期化
	bool Initialize(const std::string& vs_name, const std::string& ps_name);

	//手動で入力レイアウトを指定してシェーダーを生成し初期化
	bool Initialize(const std::string& vs_name, const std::string& ps_name, const D3D11_INPUT_ELEMENT_DESC* input_elements, UINT element_count);

	//頂点シェーダーのみを読み込み、リフレクションでレイアウトを自動生成
	bool Initialize(const std::string& vs_name);

	//手動で入力レイアウトを指定し、頂点シェーダーのみを生成
	bool Initialize(const std::string& vs_name, const D3D11_INPUT_ELEMENT_DESC* input_elements, UINT element_count);

	//保持しているシェーダーと入力レイアウトをパイプラインにバインド
	void Apply();

	//ピクセルシェーダーを取得
	ID3D11PixelShader* GetPixelShader()const { return pixel_shader.Get(); }

private:
	//頂点シェーダーを読み込み、リフレクションで入力レイアウトを自動生成
	bool CreateVertexShaderWithReflection(const std::string& vs_name);

	//シェーダーの変数型とマスク情報からDXGI_FORMATを判定
	DXGI_FORMAT DetermineDxgiFormat(D3D_REGISTER_COMPONENT_TYPE type, BYTE mask);

private:
	Microsoft::WRL::ComPtr<ID3D11VertexShader> vertex_shader;	//頂点シェーダー
	Microsoft::WRL::ComPtr<ID3D11PixelShader> pixel_shader;		//ピクセルシェーダー
	Microsoft::WRL::ComPtr<ID3D11InputLayout> input_layout;		//入力レイアウト
};

