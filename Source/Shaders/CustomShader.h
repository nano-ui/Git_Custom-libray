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

	//保持しているシェーダーと入力レイアウトをパイプラインにバインド
	void Apply();

	//内部で生成・保持しているピクセルシェーダーを取得
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

