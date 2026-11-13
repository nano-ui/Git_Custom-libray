#include "SceneSerializer.h"
#include "Engine\Core\Log\Log.h"
#include "Utility\JsonFileUtility.h"
#include "Gameplay\GameObjects\ObjectManager.h"
#include "Gameplay\GameObjects\ObjectFactory.h"
#include "Gameplay\GameObjects\GameObject.h"
#include "Gameplay\Components\Model\ModelComponent.h"

#include <vector>
#include <memory>

const std::string SceneSerializer::editor_config_path = "Data/Json/System/EditorConfig.json";
const std::string SceneSerializer::config_key_scene_path = "last_opened_scene";
const std::string SceneSerializer::json_root_key_objects = "objects";
const std::string SceneSerializer::detail_json_directory = "Data/Json/";

//シーン保存
bool SceneSerializer::SaveScene(const std::string& file_path)
{
	const auto& objects = ObjectManager::Instance().GetGameObjects();
	std::string target_name = "";
	nlohmann::json objects_array = nlohmann::json::array();

	//各オブジェクトをループ
	for (const auto& obj : objects)
	{
		//生存確認
		if (!obj || !obj->IsActive())continue;

		//ファイル名を設定
		const auto model = obj->GetComponent<ModelComponent>();
		if (!model || model->GetModelPath().empty())target_name = detail_json_directory + obj->GetClassNameW() + "/" + obj->GetClassNameW() + ".json";
		else
		{
			std::string model_name = model->GetModelPath();
			model_name = std::filesystem::path(model_name).stem().string();
			target_name = detail_json_directory + model_name + "/" + model_name + ".json";
		}

		//個別ファイルへ Detail（固有情報）を保存
		if (!obj->SaveToDetailFile(target_name))
		{
			LOG_WARNING({ u8"保存" }, u8"個別情報を保存できませんでした(ファイルパス: %s)", target_name.c_str());
			continue;
		}

		//基底情報を保存
		nlohmann::json object_json;
		object_json[u8"個別ファイルの参照先パス"] = target_name;
		obj->SaveToJObject(object_json, SerializeTarget::Base);
		objects_array.push_back(object_json);
	}
	nlohmann::json root_json;
	root_json[json_root_key_objects] = objects_array;
	return JsonFileUtility::Save(file_path, root_json) && SaveEditorConfig(file_path);
}

//シーン読み込み
bool SceneSerializer::LoadScene(const std::string& file_path)
{
	//シーンファイルの読み込み
	nlohmann::json json_data;
	if (!JsonFileUtility::Load(file_path, json_data))
	{
		LOG_WARNING({ u8"読み込み" }, u8"フォルダの読み込みができませんでした(ファイルパス:%s)", file_path.c_str());
		return false;
	}

	//ルートキー(objects)の存在＆配列チェック
	if (!json_data.contains(json_root_key_objects) || !json_data.is_array())return false;
	ObjectManager::Instance().Clear();

	//オブジェクト配列のループ復元
	for (size_t obj = 0; obj < json_data[json_root_key_objects].size(); ++obj)
	{
		const nlohmann::json& object_node = json_data[json_root_key_objects][obj];	//現在のインデックスの配列要素
		std::string class_name = "";
		if (!JsonFileUtility::TryGetValue(object_node, u8"クラス名", class_name))return false;
		GameObject* game_obj = ObjectFactory::CreateAndRegister(class_name);
		if (!game_obj)
		{
			LOG_ERROR({ u8"生成" }, u8"ゲームオブジェクトの生成ができませんでした");
			return false;
		}

		std::string detail_name = "";
		if (!JsonFileUtility::TryGetValue(object_node, u8"個別ファイルの参照先パス", detail_name))return false;
		if (!game_obj->LoadFromDetailFile(detail_name))return false;
		game_obj->LoadFromJObject(object_node, SerializeTarget::Base);

		auto model = game_obj->GetComponent<ModelComponent>();
		if (!model)
		{
			LOG_ERROR({ u8"取得" }, u8"モデルコンポーネントが取得できませんでした");
			return false;
		}

		std::string model_path = "";
		if (!JsonFileUtility::TryGetValue(object_node, u8"モデルパス", model_path))return false;
		if (!model->LoadModel(model_path))
		{
			LOG_ERROR({ u8"読み込み" }, u8"モデルの読み込みができませんでした");
			return false;
		}
	}
	return true;
}

//設定ファイルへ保存
bool SceneSerializer::SaveEditorConfig(const std::string& last_scene_path)
{
	if (last_scene_path.empty())
	{
		LOG_WARNING({ u8"設定" }, u8"文字列が空です(パス名:%s)", last_scene_path.c_str());
		return false;
	}
	nlohmann::json config_json;
	config_json[config_key_scene_path] = last_scene_path;
	return JsonFileUtility::Save(editor_config_path, config_json);
}

//設定ファイルを取得
std::string SceneSerializer::LoadEditorConfig()
{
	nlohmann::json config_json;
	if (!JsonFileUtility::Load(editor_config_path, config_json))return "";
	std::string config_path = "";
	if (!JsonFileUtility::TryGetValue(config_json, config_key_scene_path, config_path))return "";
	return config_path;
}
