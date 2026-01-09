#pragma once

#include <memory>
#include <wrl.h>
#include <d3d11.h>
#include <DirectXMath.h>
#include <string>

#include "../Scene/Scene.h"
#include "../ObjectsRender/sprite.h"
#include "../ObjectsRender/sprite_batch.h"
#include "../ObjectsRender/static_mesh.h"
#include "../ObjectsRender/skinned_mesh.h"
#include "../ObjectsRender/geometric_primitive.h"
#include "../Graphics/framebuffer.h"
#include "../Graphics/fullscreen_quad.h"
#include "../Graphics/Graphics.h"

class MainScene :public Scene
{
public:
	MainScene();
	~MainScene() override;

	void Initialize() override;
	void Update(float elapsed_time) override;
	void Render(float elapsed_time) override;
	void Finalize() override;

private:

};

