#pragma once

#include <vector>
#include <DirectXMath.h>

#include "Engine\Graphics\Resources\GltfModel\GltfModelData.h"

struct ModelDrawCommand
{
	const GltfModelData* data = nullptr;
	const std::vector<GltfModelData::node>* nodes = nullptr;
	const GltfModelData::mesh::primitive* primitive = nullptr;
	DirectX::XMFLOAT4X4 world_matrix{};
	int material_index = -1;
	int skin_index = -1;
	int node_index = -1;
	uint32_t anim_row_offset = 0;
	float current_anim_time = 0.0f;
	float anim_duration = 0.0f;
};

class ModelRenderQueue
{
public:
	ModelRenderQueue() = default;
	~ModelRenderQueue() = default;

	void Submit(const ModelDrawCommand& command);
	void Sort();
	void Clear();

	const std::vector<ModelDrawCommand>& GetCommands() const { return commands; }
	size_t GetCommandCount() const { return commands.size(); }

private:
	std::vector<ModelDrawCommand> commands;
};

