#pragma once

#define NOMINMAX
#include <d3d11.h>
#include <wrl.h>
#include <DirectXMath.h>
#include <stack>
#define TINYGLTF_NO_EXTERNAL_IMAGE
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_STB_IMAGE_WRITE
#include "tinygltf-release/tiny_gltf.h"
 
class GltfMpdel
{
public:
	struct scene
	{
		std::string name;
		std::vector<int> nodes;
	};

public:
	struct node
	{
		std::string name;
		int skin = -1;
		int mesh = -1;
		std::vector<int> children;
		DirectX::XMFLOAT4 rotation = { 0.0f,0.0f,0.0f,1.0f };
		DirectX::XMFLOAT3 scale = { 1.0f,1.0f,1.0f };
		DirectX::XMFLOAT3 translation = { 0.0f,0.0f,0.0f };
		DirectX::XMFLOAT4X4 global_transform =
		{
			1.0f,0.0f,0.0f,0.0f,
			0.0f,1.0f,0.0f,0.0f,
			0.0f,0.0f,1.0f,0.0f,
			0.0f,0.0f,0.0f,1.0f
		};
	};

public:
	GltfMpdel(ID3D11Device* device, const std::string& filename);

	void FetchNodes(const tinygltf::Model& gltf_model);

	void cumulate_transforms(std::vector<node>& nodes);

public:
	std::string filename;
	std::vector<scene> scenes;
	std::vector<node> nodes;
	int default_scene = 0;
};

