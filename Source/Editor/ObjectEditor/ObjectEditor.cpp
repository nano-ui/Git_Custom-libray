#include "ObjectEditor.h"
#include "Gameplay/GameObjects/ObjectFactory.h"
#include "Gameplay/GameObjects/GameObject.h"
#include "Gameplay/GameObjects/ObjectManager.h"
#include "Gameplay\GameObjects\ObjectSpawner.h"
#include "Gameplay\Components\Transform\TransformComponent.h"
#include "Gameplay\Components\Model\ModelComponent.h"
#include "Editor\Gizumo\ObjectGizmo.h"
#include "Editor\Picker\ObjectPicker.h"
#include "Editor\Converter\ObjectClassConverter.h"
#include "Engine/Collision/CollisionManager.h"
#include "Engine/Camera/Camera.h"
#include "Engine\Core\Input.h"
#include "Engine\Graphics\Resources\GltfModel\GltfModel.h"
#include "ThiedParty\json.hpp"
#include "Editor/EditorMediator.h"
#include "Editor\FileDialogHelper.h"
#include "Editor\StateMachineEditor\StateGraphConfigManager.h"
#include "Engine\Core\Log\Log.h"
#include "Utility\RayUtility.h"
#include "Utility\ConvertUtility.h"
#include "Utility\PlacementUtility.h"
#include "Serialization\SceneSerializer.h"

#include <imgui.h>
#include <string>
#include <unordered_map>
#include <fstream>
#include <filesystem>
#include <windows.h>
#include <commdlg.h>
#include <Engine\Core\MacroTracy.h>

constexpr float dummy_height_value = 10.0f;
constexpr float active_list_height_offset = 60.0f;
constexpr size_t label_buffer_size = 128;
constexpr float panel_width_ratio = 0.2f;

//コンストラクタ
ObjectEditor::ObjectEditor()
{
	current_selected_object = nullptr;
	scene_serializer = std::make_unique<SceneSerializer>();
	object_gizmo = std::make_unique<ObjectGizmo>();
	object_picker = std::make_unique<ObjectPicker>();
	object_class_converter = std::make_unique<ObjectClassConverter>();
}

//デストラクタ
ObjectEditor::~ObjectEditor() = default;

//初期化
void ObjectEditor::Initialize()
{
	BS_PROFILE_FUNCTION();

	current_selected_object = nullptr;
	cached_class_names = ObjectFactory::GetClassNames();
	EditorMediator::Instance().RegisterObjectEditor(this);

	//起動時のシーン復元処理
	std::string auto_load_path = scene_serializer->LoadEditorConfig();
	if (!auto_load_path.empty() && std::filesystem::exists(auto_load_path))
	{
		scene_serializer->LoadScene(auto_load_path);
	}
}

//描画
void ObjectEditor::RenderUi(Camera* camera, CollisionManager* collision_manager)
{
	BS_PROFILE_FUNCTION();

	//画面解像度に基づいた初期配置位置の計算
	ImGuiIO io = ImGui::GetIO();
	const float screen_width = io.DisplaySize.x;
	const float screen_height = io.DisplaySize.y;
	const float panel_width = screen_width * panel_width_ratio;

	ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(panel_width, screen_height), ImGuiCond_FirstUseEver);

	//画面を左右に分割するメインウインドウ
	ImGui::Begin(u8"ヒストリー");
	DrawLeftPane(camera, collision_manager);

	ImGui::Dummy(ImVec2(0.0f, dummy_height_value));
	ImGui::Separator();

	ImGui::End();

	ImGui::Begin(u8"詳細");
	DrawRightPane();
	ImGui::End();

	DirectX::XMFLOAT2 preview_pos = EditorMediator::Instance().GetWindowPos();
	DirectX::XMFLOAT2 preview_size = EditorMediator::Instance().GetWindowSize();
	GameObject* last_time_object = current_selected_object;
	if (!last_time_object)return;
	current_selected_object = object_picker->PickObject(current_selected_object, camera, collision_manager, preview_pos, preview_size);
	if (last_time_object->GetObjectId() != current_selected_object->GetObjectId())
	{
		EditorMediator::Instance().OnObjectSelected(current_selected_object);
	}
	EditorMediator::Instance().UpdateViewerSynchronization(current_selected_object);
}

//シーン保存
void ObjectEditor::SaveSceneWithDialog()
{
	BS_PROFILE_FUNCTION();

	std::string dynamic_save_path = SelectSavePath();
	if (!dynamic_save_path.empty())
	{
		scene_serializer->SaveScene(dynamic_save_path);
	}
}

//シーン読み込み
void ObjectEditor::LoadSceneWithDialog()
{
	BS_PROFILE_FUNCTION();

	std::string dynamic_load_path = SelectOpenPath();

	if (!dynamic_load_path.empty())
	{
		scene_serializer->LoadScene(dynamic_load_path);
	}
}

//オブジェクト生成
void ObjectEditor::CreateTempModelObject(const std::string& model_path, DirectX::XMFLOAT2 ndc)
{
	BS_PROFILE_FUNCTION();

	if (model_path.empty())
	{
		LOG_ERROR({ u8"設定" }, u8"モデルパスが空です(モデルパス: %s)", model_path.c_str());
		return;
	}

	Camera* active_camera = EditorMediator::Instance().GetActiveCamera();
	CollisionManager* collison_manager = EditorMediator::Instance().GetCollisionManager();
	DirectX::XMFLOAT3 hit_position = { 0.0f,0.0f,0.0f };
	if (!PlacementUtility::GetPlacementPositionFromNdc(ndc, active_camera, collison_manager, hit_position))hit_position = { 0.0f,0.0f,0.0f };
	GameObject* new_object = ObjectSpawner::ObjectSpawn(model_path, hit_position);
	if (!new_object)return;
	current_selected_object = new_object;
	current_model_path = model_path;
}

//ギズモ表示
void ObjectEditor::RenderGizmoInPreview()
{
	if (!current_selected_object)return;
	std::shared_ptr<TransformComponent> transform = current_selected_object->GetComponent<TransformComponent>();
	if (!transform)return;
	Camera* active_camera = EditorMediator::Instance().GetActiveCamera();
	object_gizmo->Draw3dGizumo(transform.get(), active_camera, EditorMediator::Instance().GetWindowSize(), EditorMediator::Instance().GetWindowPos());
}

//オブジェクト生成UI描画
void ObjectEditor::DrawLeftPane(Camera* camera, CollisionManager* collision_manager)
{
	BS_PROFILE_FUNCTION();

	//現在生成されているオブジェクトの一覧リスト描画
	ImGui::Text(u8"オブジェクトリスト");
	ImGui::Separator();
	const auto& active_objects = ObjectManager::Instance().GetGameObjects();
	frame_class_counters.clear();

	if (ImGui::BeginListBox("##ActiveList", ImVec2(-1.0f, ImGui::GetContentRegionAvail().y - active_list_height_offset))) 
	{
		for (size_t i = 0; i < active_objects.size(); ++i)
		{
			if (active_objects[i]->IsActive())
			{
				GameObject* obj_ptr = active_objects[i].get();

				std::string display_name = "";
				auto model_component = obj_ptr->GetComponent<ModelComponent>();
				if (model_component != nullptr && !model_component->GetModelPath().empty())
				{
					display_name = std::filesystem::path(model_component->GetModelPath()).stem().string();
				}
				else display_name = obj_ptr->GetClassNameW();
				int current_number = frame_class_counters[display_name];
				frame_class_counters[display_name]++;
				ImGui::PushID(reinterpret_cast<const void*>(obj_ptr));

				char label_buffer[label_buffer_size];
				snprintf(label_buffer, sizeof(label_buffer), "%s %d", display_name.c_str(), current_number);
				const bool is_current = (current_selected_object == obj_ptr);
				if (ImGui::Selectable(label_buffer, is_current))
				{
					current_selected_object = obj_ptr;
				}

				if (is_current)
				{
					ImGui::SetItemDefaultFocus();
				}
				ImGui::PopID();
			}
		}
		ImGui::EndListBox();
	}
}

//クラス適用UI描画
void ObjectEditor::DrawClassApplySection()
{
	BS_PROFILE_FUNCTION();

	if (cached_class_names.empty())
	{
		OutputDebugStringA("[Error] ObjectEditor: DrawClassApplySection - cached_class_names が空です。\n");
		return;
	}
	ImGui::Text(u8"クラスの適用");

	// インデックス範囲の安全確認
	if (inspector_selected_class_index < 0 || inspector_selected_class_index >= static_cast<int>(cached_class_names.size()))
	{
		inspector_selected_class_index = 0;
	}

	// 登録されているクラス一覧のコンボボックス描画
	const std::string& current_combo_preview = cached_class_names[inspector_selected_class_index];
	if (ImGui::BeginCombo("##ApplyClassCombo", current_combo_preview.c_str()))
	{
		for (int i = 0; i < static_cast<int>(cached_class_names.size()); i++)
		{
			const bool is_selected = (inspector_selected_class_index == i);
			if (ImGui::Selectable(cached_class_names[i].c_str(), is_selected))
			{
				inspector_selected_class_index = i;
			}
			if (is_selected) ImGui::SetItemDefaultFocus();
		}
		ImGui::EndCombo();
	}

	//ボタンが押された瞬間のみ置換処理を1回だけ呼び出す
	if (ImGui::Button(u8"選択クラスを適用", ImVec2(-1, 0)))
	{
		GameObject* new_object = nullptr;
		new_object = object_class_converter->ConvertClass(current_selected_object, cached_class_names[inspector_selected_class_index]);
		if (!new_object)return;
		current_selected_object = new_object;
	}
}

//オブジェクトパラメータ編集UI描画
void ObjectEditor::DrawRightPane()
{
	BS_PROFILE_FUNCTION();

	//選択中のオブジェクト情報の表示
	ImGui::Text(u8"モデル情報");
	ImGui::Separator();

	//カメラ追従対象への設定ボタン

	if (current_selected_object != nullptr)
	{
		if (current_selected_object->IsActive())
		{
			ImGui::Dummy(ImVec2(0.0f, 10.0f));
			ImGui::Separator();

			//オブジェクトの削除ボタン
			if (ImGui::Button(u8"モデル削除", ImVec2(-1, 0)))
			{
				current_selected_object->Destory();
				current_selected_object = nullptr;
				return;
			}
			ImGui::Dummy(ImVec2(0.0f, dummy_height_value));
			ImGui::Separator();
			DrawClassApplySection();
			ImGui::Separator();
			current_selected_object->RenderGui();
		}
		else
		{
			ImGui::Text(u8"選択したオブジェクトは破壊されました");
			current_selected_object = nullptr;
		}
	}
	else
	{
		ImGui::Text(u8"ヒストリーからオブジェクトを選んでください");
	}
}

//保存先のファイルパスをダイアログから選択取得
std::string ObjectEditor::SelectSavePath()
{
	PathResult path_result = FileDialogHelper::SaveGenericFileDialog();
	if (!path_result.relative_path.empty())return path_result.relative_path;
	return path_result.absolute_path;
}

//読み込み元のファイルパスをダイアログから選択取得
std::string ObjectEditor::SelectOpenPath()
{
	PathResult path_result = FileDialogHelper::OpenGenericFileDialog();
	if (!path_result.relative_path.empty())return path_result.relative_path;
	return path_result.absolute_path;
}