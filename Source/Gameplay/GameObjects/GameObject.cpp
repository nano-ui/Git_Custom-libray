#include "GameObject.h"
#include "Gameplay\Components\Transform\TransformComponent.h"
#include "Gameplay\Components\Model\ModelComponent.h"

//コンストラクタ
GameObject::GameObject()
{
	//基本情報の設定
	serializer = std::make_unique<JsonSerializer>();
	inspector = std::make_unique<GuiInspector>();
}

//デストラクタ
GameObject::~GameObject()
{
}

//初期化処理
void GameObject::Initialize()
{
	for (auto& component : components)
	{
		if (component)component->Initialize();
		else OutputDebugStringA("[GameObject 警告] Initialize: リスト内に nullptr のコンポーネントが存在します。\n");
	}
}

//更新処理
void GameObject::Update(float elapsed_time)
{
	if (!is_active)return;
	
	for (auto& component : components)
	{
		if (component)component->Update(elapsed_time);
		else OutputDebugStringA("[GameObject 警告] Update: リスト内に nullptr のコンポーネントが存在します。\n");
	}
}

//描画処理
void GameObject::Render(ID3D11DeviceContext* context)
{
	if (!is_active)return;

	for (auto& component : components)
	{
		if (component && component->IsActive())component->Render(context);
	}
}

//ImGuiデバッグ描画
void GameObject::RenderGui()
{
	if (inspector)inspector->RenderGui();

	for (auto& component : components)
	{
		if (component)component->RenderGui();
	}

	ImGui::Separator();

	if (ImGui::Button("Save Json Data"))SaveToJson();
}

//シリアライザに登録
void GameObject::SetupSerialization()
{
	serializer->Clear();
	inspector->Clear();
}

//指定されたJSONオブジェクトへ自身のデータを書き込む
void GameObject::SaveToJObject(nlohmann::json& object_json)
{
	if (serializer)serializer->SaveToObject(object_json);
}

//指定されたJSONオブジェクトから自身のデータを復元
void GameObject::LoadFromJObject(const nlohmann::json& object_json)
{
	if (serializer)serializer->LoadFromObject(object_json);
	else OutputDebugStringA("[GameObject 警告] LoadFromJObject: model インスタンスが nullptr です。\n");
}

//パラメータをJSONファイルへ保存
void GameObject::SaveToJson()
{
	if (!serializer)return;
	std::string file_path = "Data/Json/" + class_name + ".json";
	serializer->SaveToFile(file_path);
}

//JSONファイルからパラメータを復元
void GameObject::LoadFromJson()
{
	if (!serializer)return;

	std::string file_path = "Data/Json/" + class_name + ".json";
	bool is_loading = serializer->LoadFromFile(file_path);
	if (!is_loading)
	{
		serializer->SaveToFile(file_path);
	}
}
