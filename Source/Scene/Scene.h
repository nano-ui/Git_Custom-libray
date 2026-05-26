#pragma once

#include "../Graphics/Graphics.h"

class Scene
{
public:
	virtual ~Scene() = default;
	virtual void Initialize() = 0;
	virtual void Update(float elapsed_time) = 0;
	virtual void Render(float elapsed_time) = 0;
	virtual void Finalize() = 0;
	//virtual scene_constants GetSceneConstant()const = 0;
};

