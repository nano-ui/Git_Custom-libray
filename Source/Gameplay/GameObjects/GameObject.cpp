#include "GameObject.h"
#include "Gameplay\Components\Transform\TransformComponent.h"
#include "Gameplay\Components\Model\ModelComponent.h"

#include <fstream>
#include <filesystem>

static uint32_t next_game_object_id = 1;	//次のオブジェクトID

//コンストラクタ
GameObject::GameObject()
	:is_active(true)
	, class_name("GameObject")
	, model_hash(0)
	, object_id(next_game_object_id++)
{
	if (next_game_object_id == INVALID_OBJECT_ID)
	{
		next_game_object_id = 1;
	}


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
	if (!serializer) return;
	serializer->Clear();

	// アタッチされた全コンポーネントのシリアライズ登録を一括実行
	for (auto& component : components)
	{
		if (component) component->SetupSerialization(serializer.get());
	}
}

//GuiInspectorに登録
void GameObject::SetupInspector()
{
	if (!inspector) return;
	inspector->Clear();

	inspector->RegisterText(u8"オブジェクトID", &object_id, u8"基本情報");

	// アタッチされた全コンポーネントのインスペクター登録を一括実行
	for (auto& component : components)
	{
		if (component) component->SetupInspector(inspector.get());
	}
}

//指定されたJSONオブジェクトへ自身のデータを書き込む
void GameObject::SaveToJObject(nlohmann::json& object_json, SerializeTarget target) 
{
	serializer->SaveToObject(object_json, target);
}

//指定されたJSONオブジェクトから自身のデータを復元
void GameObject::LoadFromJObject(const nlohmann::json& object_json, SerializeTarget target) 
{
	serializer->LoadFromObject(object_json, target);
}

//パラメータをJSONファイルへ保存
void GameObject::SaveToJson(SerializeTarget target) 
{
	std::string file_path = "Data/Json/" + class_name + ".json";
	serializer->SaveToFile(file_path, target);
}

//JSONファイルからパラメータを復元
void GameObject::LoadFromJson(SerializeTarget target) 
{
	std::string file_path = "Data/Json/" + class_name + ".json";
	bool is_loading = serializer->LoadFromFile(file_path, target);
	if (!is_loading)
	{
		serializer->SaveToFile(file_path, target);
	}
}

//クラス名とモデル個別情報のみを保存
bool GameObject::SaveToDetailFile(const std::string& file_path)
{
	//パスを受け取り、親ディレクトリの確認し、存在しない場合は生成
	std::filesystem::path system_path(file_path);
	if (system_path.has_parent_path())
	{
		std::filesystem::create_directories(system_path.parent_path());
	}

	//nlohmann::jsonの最上位キーを設定
	nlohmann::json detail_json;
	detail_json["class_name"] = class_name;

	//固有情報を格納
	if (serializer)
	{
		serializer->SaveToObject(detail_json, SerializeTarget::Detail);
	}
	else
	{
		OutputDebugStringA("[GameObject 警告] SaveToDetailFile: serializer が nullptr です。\n");
		return false;
	}

	//ファイルに書き出し
	std::ofstream detail_file(file_path);
	if (detail_file.is_open())
	{
		constexpr int json_indent_space = 4;
		detail_file << detail_json.dump(json_indent_space);
		detail_file.close();
	}
	else
	{
		OutputDebugStringA("[GameObject エラー] SaveToDetailFile: 個別JSONファイルのオープンに失敗しました。\n");
		return false;
	}
	return true;
}

//モデル個別情報のみを復元
bool GameObject::LoadFromDetailFile(const std::string& file_path)
{
	//ファイルを開き、JSONパースを実行
	std::ifstream detail_file(file_path);

	//開けるか確認
	if (!detail_file.is_open())
	{
		OutputDebugStringA("[GameObject エラー] LoadFromDetailFile: 個別JSONファイルが開けませんでした。\n");
		return false;
	}

	nlohmann::json detail_json;
	detail_file >> detail_json;
	detail_file.close();

	//パラメーター復元
	if (serializer)
	{
		serializer->LoadFromObject(detail_json, SerializeTarget::Detail);
		return true;
	}
	else
	{
		OutputDebugStringA("[GameObject 警告] LoadFromDetailFile: serializer が nullptr です。\n");
		return false;
	}
}

//オブジェクト生成前に、個別 JSON ファイルから class_nameを読み取る
std::string GameObject::ReadClassNameFromDetailFile(const std::string& file_path)
{
	std::ifstream detail_file(file_path);
	if (!detail_file.is_open())
	{
		OutputDebugStringA("[GameObject エラー] ReadClassNameFromDetailFile: 個別JSONファイルが開けませんでした。\n");
		return "";
	}

	nlohmann::json detail_json;
	detail_file >> detail_json;
	detail_file.close();

	if (detail_json.contains("class_name"))
	{
		return detail_json["class_name"].get<std::string>();
	}

	OutputDebugStringA("[GameObject 警告] ReadClassNameFromDetailFile: JSON内に class_name が見つかりませんでした。\n");
	return "";
}
