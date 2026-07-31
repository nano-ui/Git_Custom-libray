#include "GameObject.h"

//コンストラクタ
GameObject::GameObject()
{
	//基本情報の設定
	position = { 0.0f,0.0f,0.0f };
	rotation = { 0.0f,0.0f,0.0f,1.0f };
	scale = { 1.0f,1.0f,1.0f };
	color = { 1.0f,1.0f,1.0f,1.0f };
	is_active = true;

	model = std::make_shared<Model>();
	serializer = std::make_unique<JsonSerializer>();
	inspector = std::make_unique<GuiInspector>();
}

//デストラクタ
GameObject::~GameObject()
{
}

//ワールド変換行列の合成、取得
DirectX::XMMATRIX GameObject::GetWorldMatrix() const
{
	//各トランスフォーム成分の独立した行列生成
	DirectX::XMMATRIX matrix_scale = DirectX::XMMatrixScaling(scale.x, scale.y, scale.z);
	DirectX::XMMATRIX matrix_rotation = DirectX::XMMatrixRotationQuaternion(DirectX::XMLoadFloat4(&rotation));
	DirectX::XMMATRIX matrix_translation = DirectX::XMMatrixTranslation(position.x, position.y, position.z);

	//ワールド行列の作成
	DirectX::XMMATRIX matrix_world = matrix_scale * matrix_rotation * matrix_translation;

	return matrix_world;
}

//ImGuiデバッグ描画
void GameObject::RenderGui()
{
	if (inspector)inspector->RenderGui();

	if (model)model->DrawImGui();

	ImGui::Separator();

	if (ImGui::Button("Save Json Data"))SaveToJson();
}

//シリアライザに登録
void GameObject::SetupSerialization()
{
	serializer->Clear();
	inspector->Clear();

	serializer->RegisterVariable(u8"座標", &position);
	serializer->RegisterVariable(u8"角度", &rotation);
	serializer->RegisterVariable(u8"拡大率", &scale);

	inspector->RegisterVariable(u8"位置", &position, u8"トランスフォーム");
	inspector->RegisterVariable(u8"角度", &rotation, u8"トランスフォーム");
	inspector->RegisterVariable(u8"拡大率", &scale, u8"トランスフォーム");
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
	if (model)model->LoadFromJObject(object_json);
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
