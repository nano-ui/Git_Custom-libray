#pragma once

#include <imgui.h>
#include <ImGuizmo.h>
#include <DirectXMath.h>

class Camera;
class TransformComponent;

class ObjectGizmo
{
public:
	//3Dオブジェクトのギズモ表示
	void Draw3dGizumo(TransformComponent* transform, Camera* camera, DirectX::XMFLOAT2 window_size, DirectX::XMFLOAT2 window_pos);

private:
	ImGuizmo::OPERATION operation = ImGuizmo::TRANSLATE;		//操作状態
};

