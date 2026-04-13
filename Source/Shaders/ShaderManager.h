#pragma once
#include <d3d11.h>
#include <memory>

class GltfDynamicModelShader;

class ShaderManager
{
public:

	//インスタンス取得
	static ShaderManager& GetInstance()
	{
		static ShaderManager instance;
		return instance;
	}

	//全シェーダーの初期化
	HRESULT Initialize(ID3D11Device* device);

	GltfDynamicModelShader* GetGltfDynamicModelShader()const { return gltf_dynamic_model_shader.get(); }

private:

private:
	std::unique_ptr<GltfDynamicModelShader> gltf_dynamic_model_shader;
};

