#include "Model.h"
#include "Engine/Graphics/Resources/ModelManager.h"
#include "Engine/Graphics/Resources/GltfModel/GltfModel.h"
#include "Engine/Graphics/Resources/GltfModel/GltfModelRenderer.h"
#include "Engine/Graphics/Renderers/Graphics.h"
#include "Gameplay/Components/Animation/RootMotionComponent.h"

#include <Windows.h>
#include <imgui.h>

// コンストラクタ
Model::Model()
{
	root_motion_component = std::make_unique<RootMotionComponent>();
}

// デストラクタ
Model::~Model() = default;

// 初期化処理
bool Model::Initialize(const std::string& file_path)
{
	return LoadModelInternal(file_path);
}

bool Model::LoadModelInternal(const std::string& file_path)
{
	model_path = file_path;

	//ModelManagerからのデータ取得
	data = ModelManager::Instance().LoadModelData(file_path);
	if (!data)
	{
		std::string debug_msg = "[Model エラー] LoadModelInternal: ModelManager からのモデル読み込みに失敗しました。 パス: " + file_path + "\n";
		OutputDebugStringA(debug_msg.c_str());
		return false;
	}

	//DirectX11デバイスを取得して描画用インスタンスを生成
	ID3D11Device* device = Graphics::Instance().GetDevice();
	if (!device)
	{
		OutputDebugStringA("[Model エラー] LoadModelInternal: Graphics から ID3D11Device を取得できませんでした。\n");
		return false;
	}

	renderer = std::make_shared<GltfModelRenderer>(device);
	model = std::make_unique<GltfModel>(data, renderer);

	//内部ルートモーションコンポーネントの初期化
	if (root_motion_component)root_motion_component->Initialize(data);

	return true;
}

// 更新処理
void Model::Update(float elapsed_time)
{
	if (model)
	{
		//姿勢・アニメーション時間の更新
		model->Update(elapsed_time);

		//ルートモーション有効時は、アニメーション再生時間をもとに移動量を更新
		if (root_motion_component)
		{
			float current_anim_time = model->GetAnimationCurrentTime();
			root_motion_component->Update(current_anim_time);
		}
	}
	else OutputDebugStringA("[Model 警告] Update: model インスタンスが nullptr です。\n");
}

// 描画処理
void Model::Render(ID3D11DeviceContext* context, const DirectX::XMFLOAT4X4& world, const DirectX::XMFLOAT4 color)
{
	// 描画非表示フラグが有効な場合は処理をスキップ
	if (!is_visible)return;

	if (!context)
	{
		OutputDebugStringA("[Model エラー] Render: context が nullptr です。\n");
		return;
	}

	if (model)model->Render(context, world);
	else OutputDebugStringA("[Model 警告] Render: model インスタンスが nullptr です。\n");
}

// ImGuiでのデバッグ表示
void Model::DrawImGui()
{
	if (ImGui::TreeNode(u8"モデル設定"))
	{
		ImGui::Checkbox(u8"表示", &is_visible);
		ImGui::Text(u8"モデルパス: %s", model_path.c_str());

		if (data)
		{
			ImGui::Text(u8"共有参照数: %ld", data.use_count());
		}
		ImGui::TreePop();
	}
}

// JSONへのデータ書き出し
void Model::SaveToObject(nlohmann::json& object_json) const
{
	object_json["model_path"] = model_path;
}

// JSONからのデータ復元
void Model::LoadFromJObject(const nlohmann::json& object_json)
{
	if (object_json.contains("model_path"))
	{
		std::string path_from_json = object_json["model_path"].get<std::string>();
		if (!path_from_json.empty())Initialize(path_from_json);
		else OutputDebugStringA("[Model 警告] LoadFromJObject: model_path が空文字列です。\n");
	}
	else OutputDebugStringA("[Model 警告] LoadFromJObject: JSON内に 'model_path' キーが存在しません。\n");
}

// アニメーション再生
void Model::PlayAnimation(const std::string& animation_name, bool is_loop)
{
	if (model)model->PlayAnimation(animation_name, is_loop);
}

// 再生時間取得
float Model::GetAnimationTime() const
{
	if (model)return model->GetAnimationCurrentTime();
	return 0.0f;
}

// アニメーション再生時間を設定
void Model::SetAnimationTime(float time)
{
	if (model)model->SetAnimationTime(time);
	else OutputDebugStringA("[Model 警告] SetAnimationTime: model インスタンスが nullptr です。\n");
}

// アニメーション総時間取得
float Model::GetAnimationDuration() const
{
	if (model)return model->GetAnimationDuration();
	return 0.0f;
}

// アニメーション終了判定
bool Model::IsAnimationFinished() const
{
	if (model)return model->IsAnimationFinished();
	return true;
}

// アニメーション名からインデックス取得
int Model::GetAnimationIndex(const char* name) const
{
	if (data && name)return data->GetAnimationIndex(name);
	return ERROR_ANIMATION_NOT_FOUND;
}

// ルートモーション移動差分の取得
DirectX::XMFLOAT3 Model::GetDeltaPosition() const
{
	if (root_motion_component)return root_motion_component->GetDeltaPosition();
	return { 0.0f, 0.0f, 0.0f };
}

// ルートモーション回転差分の取得
DirectX::XMFLOAT4 Model::GetDeltaRotation() const
{
	if (root_motion_component)return root_motion_component->GetDeltaRotation();
	return { 0.0f, 0.0f, 0.0f, 1.0f };
}

// 動的ノード配列取得
const std::vector<GltfModelData::node>& Model::GetAnimatedNodes() const
{
	static const std::vector<GltfModelData::node> empty_nodes;
	if (model)return model->GetAnimatedNodes();
	return empty_nodes;
}

// ノード位置の上書き
void Model::SetNodeTranslation(int node_index, const DirectX::XMFLOAT3& translation)
{
	if (model)model->SetNodeTranslation(node_index, translation);
}

// グローバル行列の再計算
void Model::RecalculateTransforms()
{
	if (model)model->RecalculateTransforms();
}