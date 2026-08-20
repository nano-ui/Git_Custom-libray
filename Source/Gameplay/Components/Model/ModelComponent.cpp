#include "ModelComponent.h"
#include "Gameplay\Components\Transform\TransformComponent.h"
#include "Engine/Graphics/Resources/ModelManager.h"
#include "Engine/Graphics/Resources/GltfModel/GltfModel.h"
#include "Engine/Graphics/Resources/GltfModel/GltfModelData.h"
#include "Engine/Graphics/Resources/GltfModel/GltfModelRenderer.h"
#include "Engine/Graphics/Renderers/Graphics.h"

#include <Windows.h>
#include <vector>
#include <imgui.h>
#include "MovementComponent.h"

//コンストラクタ
ModelComponent::ModelComponent()
	:model_data(nullptr)
	,renderer(nullptr)
	,model(nullptr)
	,model_path("")
	,is_visible(true)
{
	SetComponentName(u8"モデルコンポーネント");
}

//デストラクタ
ModelComponent::~ModelComponent()
{
}

//初期化処理
void ModelComponent::Initialize()
{
	Component::Initialize();
	if(target_transform.expired()) OutputDebugStringA("[ModelComponent 警告] Initialize: target_transform が設定されていません。SetTransformComponent で事前設定してください。\n");
}

//モデルデータのロード処理
bool ModelComponent::LoadModel(const std::string& file_path)
{
	model_path = file_path;

	//ModelManagerを経由して共有モデルデータを取得
	model_data = ModelManager::Instance().LoadModelData(file_path);
	if (!model_data)
	{
		OutputDebugStringA("[ModelComponent エラー] LoadModel: ModelManagerからのモデルデータ取得に失敗しました。\n");
		return false;
	}

	//レンダラーの生成
	ID3D11Device* device = Graphics::Instance().GetDevice();
	if (!device)
	{
		OutputDebugStringA("[ModelComponent エラー] LoadModel: ID3D11Device の取得に失敗しました。\n");
		return false;
	}

	renderer = std::make_unique<GltfModelRenderer>(device);
	if (!renderer)
	{
		OutputDebugStringA("[ModelComponent エラー] LoadModel: GltfModelRenderer の生成に失敗しました。\n");
		return false;
	}

	//個別オブジェクト用の GltfModel インスタンスを生成
	model = std::make_unique<GltfModel>(model_data, renderer);
	if (!model)
	{
		OutputDebugStringA("[ModelComponent エラー] LoadModel: GltfModel の生成に失敗しました。\n");
		return false;
	}
	return true;
}

//アニメーション更新
void ModelComponent::Update(float delta_time)
{
	if (model)model->Update(delta_time);
	else OutputDebugStringA("[ModelComponent 警告] Update: model インスタンスが nullptr です。\n");
}

//描画処理
void ModelComponent::Render(ID3D11DeviceContext* context)
{
	if (!is_active || !is_visible)return;

	if (!context)
	{
		OutputDebugStringA("[ModelComponent エラー] Render: context が nullptr です。\n");
		return;
	}
	RenderInternal(context);
}

//ImGuiデバッグ描画
void ModelComponent::RenderGui()
{
	if (!is_active)return;

	if (ImGui::TreeNode(GetComponentName().c_str()))
	{
		ImGui::Checkbox(u8"表示", &is_visible);
		ImGui::Text(u8"モデルパス", model_path.c_str());
		if (model_data)ImGui::Text(u8"リソース共有参照数: %ld", model_data.use_count());
		ImGui::TreePop();
	}
}

//トランスフォームコンポーネントの登録
void ModelComponent::SetTransformComponent(const std::shared_ptr<TransformComponent>& transform)
{
	target_transform = transform;
}

//Jsonへのモデルパスデータ保存
void ModelComponent::SaveToObject(nlohmann::json& object_json) const
{
	object_json["model_path"] = model_path;
}

//Jsonへのモデルパスデータ復元とモデルロード
void ModelComponent::LoadFromJObject(const nlohmann::json& object_json)
{
	if (object_json.contains("model_path"))
	{
		std::string path_from_json = object_json["model_path"].get<std::string>();

		if (!path_from_json.empty())LoadModel(path_from_json);
		else OutputDebugStringA("[ModelComponent 警告] LoadFromJObject: model_path が空文字列です。\n");
	}
	else
	{
		OutputDebugStringA("[ModelComponent 警告] LoadFromJObject: JSON内に 'model_path' キーが存在しません。\n");
	}
}

//モデルの描画処理
void ModelComponent::RenderInternal(ID3D11DeviceContext* context)
{
	std::shared_ptr<TransformComponent> transform = target_transform.lock();
	if (!transform)
	{
		OutputDebugStringA("[ModelComponent 警告] RenderInternal: 参照先 TransformComponent が破棄されているか未設定のため描画をスキップします。\n");
		return;
	}

	if (!model)
	{
		OutputDebugStringA("[ModelComponent 警告] RenderInternal: model インスタンスが nullptr です。\n");
		return;
	}

	DirectX::XMMATRIX world_matrix_xm = transform->GetWorldMatrix();
	DirectX::XMFLOAT4X4 world_matrix;
	DirectX::XMStoreFloat4x4(&world_matrix, world_matrix_xm);

	model->Render(context, world_matrix);
}
