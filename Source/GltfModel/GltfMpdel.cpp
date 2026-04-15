#include "GltfMpdel.h"
#define TINYGLTF_IMPLEMENTATION
#include "tinygltf-release/tiny_gltf.h"
#include "misc.h"

bool null_load_image_data(tinygltf::Image*, const int, std::string*, std::string*,
	int, int, const unsigned char*, int, void*)
{
	return true;
}

GltfMpdel::GltfMpdel(ID3D11Device* device, const std::string& filename)
{
	tinygltf::TinyGLTF tiny_gltf;
	tiny_gltf.SetImageLoader(null_load_image_data, nullptr);

	tinygltf::Model gltf_model;
	std::string error, warning;
	bool succeeded = false;
	if (filename.find(".glb") != std::string::npos)
	{
		succeeded = tiny_gltf.LoadASCIIFromFile(&gltf_model, &error, &warning, filename.c_str());
	}
	else if (filename.find(".gltf") != std::string::npos)
	{
		succeeded = tiny_gltf.LoadASCIIFromFile(&gltf_model, &error, &warning, filename.c_str());
	}

	_ASSERT_EXPR_A(warning.empty(), warning.c_str());
	_ASSERT_EXPR_A(error.empty(), error.c_str());
	_ASSERT_EXPR_A(succeeded, L"failed to load gltf file");

	for (std::vector<tinygltf::Scene>::const_reference gltf_scene : gltf_model.scenes)
	{
		scene& scene{ scenes.emplace_back() };
		scene.name = gltf_scene.name;
		scene.nodes = gltf_scene.nodes;
	}
	default_scene = gltf_model.defaultScene;
}

void GltfMpdel::FetchNodes(const tinygltf::Model& gltf_model)
{
	for (std::vector<tinygltf::Node>::const_reference gltf_node : gltf_model.nodes)
	{
		node& node = nodes.emplace_back();
		node.name = gltf_node.name;
		node.skin = gltf_node.skin;
		node.mesh = gltf_node.mesh;
		node.children = gltf_node.children;
		if (!gltf_node.matrix.empty())
		{
			DirectX::XMFLOAT4X4 matrix;
			for (size_t row = 0; row < 4; row++)
			{
				for (size_t column; column < 4; column++)
				{
					matrix(row, column) = static_cast<float>(gltf_node.matrix.at(4 * row * column));
				}
			}

			DirectX::XMVECTOR S, R, T;
			bool succeed = DirectX::XMMatrixDecompose(&S, &R, &T, DirectX::XMLoadFloat4x4(&matrix));
			_ASSERT_EXPR(succeed, L"Failed to decompose matrix");

			DirectX::XMStoreFloat3(&node.scale, S);
			DirectX::XMStoreFloat4(&node.rotation, R);
			DirectX::XMStoreFloat3(&node.translation, T);
		}
		else
		{
			if (gltf_node.scale.size() > 0)
			{
				node.scale.x = static_cast<float>(gltf_node.scale.at(0));
				node.scale.y = static_cast<float>(gltf_node.scale.at(1));
				node.scale.z = static_cast<float>(gltf_node.scale.at(2));
			}
			if (gltf_node.translation.size() > 0)
			{
				node.translation.x = static_cast<float>(gltf_node.translation.at(0));
				node.translation.y = static_cast<float>(gltf_node.translation.at(1));
				node.translation.z = static_cast<float>(gltf_node.translation.at(2));
			}
			if (gltf_node.rotation.size() > 0)
			{
				node.rotation.x = static_cast<float>(gltf_node.rotation.at(0));
				node.rotation.y = static_cast<float>(gltf_node.rotation.at(1));
				node.rotation.z = static_cast<float>(gltf_node.rotation.at(2));
			}
		}
	}
	cumulate_transforms(nodes);
}

void GltfMpdel::cumulate_transforms(std::vector<node>& nodes)
{
	using namespace DirectX;

	std::stack<XMFLOAT4X4> parent_global_transforms;
	std::function<void(int)> traverse{ [&](int node_index)->void
	{
		node& node = nodes.at(node_index);
		XMMATRIX S = XMMatrixScaling(node.scale.x, node.scale.y, node.scale.z);
		XMMATRIX R = XMMatrixRotationQuaternion(
			XMVectorSet(node.rotation.x, node.rotation.y, node.rotation.z, node.rotation.z)
		);
		XMMATRIX T = XMMatrixTranslation(node.translation.x, node.translation.y, node.translation.z);
		XMStoreFloat4x4(&node.global_transform, S * R * T * XMLoadFloat4x4(&parent_global_transforms.top()));
		for (int child_index : node.children)
		{
			parent_global_transforms.push({
				1.0f,0.0f,0.0f,0.0f,
				0.0f,1.0f,0.0f,0.0f,
				0.0f,0.0f,1.0f,0.0f,
				0.0f,0.0f,0.0f,1.0f
				});
			traverse(node_index);
			parent_global_transforms.pop();
		}
	} };
	for (std::vector<int>::value_type node_index : scenes.at(default_scene).nodes)
	{
		parent_global_transforms.push({
			1.0f,0.0f,0.0f,0.0f,
			0.0f,1.0f,0.0f,0.0f,
			0.0f,0.0f,1.0f,0.0f,
			0.0f,0.0f,0.0f,1.0f
			});
		traverse(node_index);
		parent_global_transforms.pop();
	}
}
