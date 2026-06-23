#pragma once

#include <string>
#include <memory>
#include <d3d11.h>

//‘O•ûéŒ¾
class FbxSkinnedResource;

class FbxLoader
{
public:
	static bool Load(
		ID3D11Device* device,
		const std::string& filename,
		std::shared_ptr<FbxSkinnedResource> out_resource
	);
};

