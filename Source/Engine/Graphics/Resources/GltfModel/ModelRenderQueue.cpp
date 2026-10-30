#include "ModelRenderQueue.h"
#include <algorithm>
#include <Windows.h>

void ModelRenderQueue::Submit(const ModelDrawCommand& command)
{
	if (!command.data)
	{
		OutputDebugStringA("[ModelRenderQueue エラー] Submit: command.data が nullptr です。\n");
		return;
	}

	if (!command.nodes)
	{
		OutputDebugStringA("[ModelRenderQueue エラー] Submit: command.nodes が nullptr です。\n");
		return;
	}

	if (!command.primitive)
	{
		OutputDebugStringA("[ModelRenderQueue エラー] Submit: command.primitive が nullptr です。\n");
		return;
	}

	commands.push_back(command);
}

void ModelRenderQueue::Sort()
{
	if (commands.empty())
	{
		return;
	}

	std::sort(commands.begin(), commands.end(), [](const ModelDrawCommand& a, const ModelDrawCommand& b)
		{
			if (a.material_index != b.material_index)
			{
				return a.material_index < b.material_index;
			}
			if (a.data != b.data)
			{
				return a.data < b.data;
			}
			return a.skin_index < b.skin_index;
		});
}

void ModelRenderQueue::Clear()
{
	commands.clear();
}