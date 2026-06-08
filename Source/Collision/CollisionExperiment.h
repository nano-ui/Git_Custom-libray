#pragma once

#include <vector>
#include <memory>

#include "CollisionManager.h"

class ShapeRenderer;
class CollisionSphere;

class CollisionExperiment
{
public:
	//コンストラクタ
	CollisionExperiment(CollisionManager* collision_manager);

	//デストラクタ
	~CollisionExperiment();

	//更新
	void Update(float elapsed_time);

	//描画
	void Render(ShapeRenderer* renderer);

	//ImGui描画
	void RenderGui();

private:
	//球の総数を目標数に調整
	void AdjustSphereCount();

private:
	CollisionManager* manager_ptr;                                  //衝突管理クラスへの生ポインタ
	std::vector<std::unique_ptr<CollisionSphere>> sphere_list;      //球を管理するスマートポインタ動取配列
	int target_count;												//ImGuiで操作する目標生成数
	int current_count;                                              //現在の生成数
};

