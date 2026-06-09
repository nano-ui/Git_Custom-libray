#include "CollisionExperiment.h"
#include "CollisionSphere.h"

#include <imgui.h>
#include <random>

//コンストラクタ
CollisionExperiment::CollisionExperiment(CollisionManager* collision_manager)
	:manager_ptr(collision_manager)
	,target_count(0)
	,current_count(0)
{
	AdjustSphereCount();
}

//デストラクタ
CollisionExperiment::~CollisionExperiment()
{
	sphere_list.clear();
}

//更新
void CollisionExperiment::Update(float elapsed_time)
{
	//目標数に変更があるかチェック
	if (current_count != target_count)
	{
		AdjustSphereCount();
	}

	//すべての球の更新ループ
	for (size_t i = 0; i < sphere_list.size(); i++)
	{
		if (sphere_list[i] != nullptr)
		{
			sphere_list[i]->Update(elapsed_time);
		}
	}
}

//描画
void CollisionExperiment::Render(ShapeRenderer* renderer)
{
	//描画数の制限して描画ループ
	constexpr size_t max_draw_limit = 500;
	size_t current_draw_count = sphere_list.size();
	if (current_draw_count > max_draw_limit)
	{
		current_draw_count = max_draw_limit;
	}

	for (size_t i = 0; i < current_draw_count; i++)
	{
		if (sphere_list[i] != nullptr)
		{
			sphere_list[i]->Render(renderer);
		}
	}
}

//ImGuiデバッグ描画
void CollisionExperiment::RenderGui()
{
	if (ImGui::CollapsingHeader("CollisionEnperiment", ImGuiDockNodeFlags_None))
	{
		constexpr int max_experiment_spheres = 10000;
		ImGui::DragInt("Target Count", &target_count, 1.0f, 0, max_experiment_spheres);

		ImGui::Text("Current Active (Physics) Spheres:%d", current_count);
		ImGui::Text("Rendered Spheres Limit: 500");
		ImGui::Text("Application FPS: %.1f", ImGui::GetIO().Framerate);
	}
}

//球の総数を目標数に調整
void CollisionExperiment::AdjustSphereCount()
{
	//球の追加生成
	if (current_count < target_count)
	{
		std::random_device seed_gen;
		std::mt19937 engine(seed_gen());

		constexpr float range_pos = 50.0f;
		std::uniform_real_distribution<float> dist_pos(-range_pos, range_pos);

		constexpr float min_vel = -8.0f;
		constexpr float max_vel = 8.0f;
		std::uniform_real_distribution<float> dist_vel(min_vel, max_vel);

		constexpr float fix_radius = 1.0f;

		while (current_count < target_count)
		{
			DirectX::XMFLOAT3 random_pos = { dist_pos(engine), dist_pos(engine), dist_pos(engine) };
			DirectX::XMFLOAT3 random_vel = { dist_vel(engine), dist_vel(engine), dist_vel(engine) }; 

			sphere_list.push_back(std::make_unique<CollisionSphere>(manager_ptr, random_pos, fix_radius, random_vel));\
			current_count++;
		}
	}

	//球の破棄
	else if (current_count > target_count)
	{
		while (current_count > target_count){}
		{
			sphere_list.pop_back();
			current_count--;
		}
	}
}
