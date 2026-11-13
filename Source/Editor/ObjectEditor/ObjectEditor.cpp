#include "ObjectEditor.h"
#include "Gameplay/GameObjects/ObjectFactory.h"
#include "Gameplay/GameObjects/GameObject.h"
#include "Gameplay/GameObjects/ObjectManager.h"
#include "Gameplay\Components\Transform\TransformComponent.h"
#include "Gameplay\Components\Model\ModelComponent.h"
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
#include "Serialization\SceneSerializer.h"

#include <imgui.h>
#include <ImGuizmo.h>
#include <string>
#include <unordered_map>
#include <fstream>
#include <filesystem>
#include <windows.h>
#include <commdlg.h>
#include <Engine\Core\MacroTracy.h>

constexpr float dummy_height_value = 10.0f;
constexpr float class_list_height_ratio = 0.3f;
constexpr float active_list_height_offset = 60.0f;
constexpr size_t label_buffer_size = 128;
constexpr float panel_width_ratio = 0.2f;
constexpr int json_indent_space_count = 4;
constexpr size_t file_path_buffer_size = 260;

//コンストラクタ
ObjectEditor::ObjectEditor()
{
	selected_class_index = 0;
	current_selected_object = nullptr;
	current_gizmo_operation = static_cast<int>(ImGuizmo::TRANSLATE);
	scene_serializer = std::make_unique<SceneSerializer>();
}

//デストラクタ
ObjectEditor::~ObjectEditor()
{

}

//初期化
void ObjectEditor::Initialize()
{
	BS_PROFILE_FUNCTION();

	selected_class_index = 0;
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
	ImGui::Text("Global Scene Operations");

	ImGui::End();

	ImGui::Begin(u8"詳細");
	DrawRightPane();
	ImGui::End();

	EditorMediator::Instance().OnObjectSelected(current_selected_object);
	EditorMediator::Instance().UpdateViewerSynchronization(current_selected_object);

	DrawGizmo(camera);
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

//仮オブジェクト生成
void ObjectEditor::CreateTempModelObject(const std::string& model_path, DirectX::XMFLOAT2 ndc)
{
	BS_PROFILE_FUNCTION();

	if (model_path.empty() || cached_class_names.empty())
	{
		return;
	}

	current_model_path = model_path;
	std::string model_name = std::filesystem::path(model_path).stem().string();
	std::string detail_file_path = "Data/Json/" + model_name + "/" + model_name + ".json";

	bool is_json_loaded = false;

	// 1. 個別JSONファイルの存在チェック
	if (std::filesystem::exists(detail_file_path))
	{
		std::ifstream detail_file(detail_file_path);
		if (detail_file.is_open())
		{
			nlohmann::json detail_json;
			detail_file >> detail_json;
			detail_file.close();

			if (detail_json.contains("class_name"))
			{
				std::string target_class_name = detail_json["class_name"].get<std::string>();

				// クラスの生成
				GameObject* new_object = ObjectFactory::CreateAndRegister(target_class_name);
				if (new_object != nullptr)
				{
					new_object->Initialize();
					auto model_component = new_object->GetComponent<ModelComponent>();
					if (model_component != nullptr)
					{
						bool success = model_component->LoadModel(current_model_path);
					}

					new_object->LoadFromJObject(detail_json, SerializeTarget::Detail);
					current_selected_object = new_object;
					is_json_loaded = true;
				}
			}
		}
	}

	// 2. 個別JSONが存在しない場合の生成処理
	if (!is_json_loaded)
	{
		std::string target_class_name = "Stage"; // デフォルトを Stage に設定
		for (size_t i = 0; i < cached_class_names.size(); i++)
		{
			if (cached_class_names[i] == model_name)
			{
				target_class_name = cached_class_names[i];
				inspector_selected_class_index = static_cast<int>(i);
				break;
			}
		}

		// オブジェクトの生成
		GameObject* new_object = ObjectFactory::CreateAndRegister(target_class_name);
		if (new_object != nullptr)
		{
			new_object->Initialize();

			auto model_component = new_object->GetComponent<ModelComponent>();
			if (model_component != nullptr)
			{
				bool success = model_component->LoadModel(current_model_path);
			}
			current_selected_object = new_object;
		}
	}

	if (!current_selected_object)return;

	Camera* active_camera = EditorMediator::Instance().GetActiveCamera();
	CollisionManager* collison_manager = EditorMediator::Instance().GetCollisionManager();

	if (!active_camera || !collison_manager)
	{
		LOG_ERROR({ u8"取得" }, u8"仲介クラスから取得したポインタがnullでした");
		return;
	}
	std::shared_ptr<TransformComponent> transform = current_selected_object->GetComponent<TransformComponent>();
	if (!transform)
	{
		LOG_WARNING({ u8"取得" }, u8"モデルに行列情報がないため配置できません");
		return;
	}

	DirectX::XMFLOAT3 ray_start = { 0.0f,0.0f,0.0f };
	DirectX::XMFLOAT3 ray_end = { 0.0f,0.0f,0.0f };
	if (RayUtility::NdcToWorldRay(ndc, active_camera->GetView(), active_camera->GetProjection(), ray_start, ray_end))
	{
		DirectX::XMFLOAT3 hit_position = { 0.0f,0.0f,0.0f };
		if (collison_manager->RayCastSpace(ray_start, ray_end, hit_position))
		{
			transform->SetPosition(hit_position);
		}
		else
		{
			transform->SetPosition({ 0.0f,0.0f,0.0f });
		}
	}
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
		ApplySelectedClassToObject();
	}
}

//選択されたクラスへの置き換え処理
void ObjectEditor::ApplySelectedClassToObject()
{
	BS_PROFILE_FUNCTION();

	if (current_selected_object == nullptr)
	{
		OutputDebugStringA("[ObjectEditor 警告] ApplySelectedClassToObject: 置換対象の current_selected_object が nullptr です。\n");
		return;
	}

	// 旧オブジェクトから姿勢情報（Transform）を取得
	auto old_transform = current_selected_object->GetComponent<TransformComponent>();
	DirectX::XMFLOAT3 pos = old_transform ? old_transform->GetPosition() : DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
	DirectX::XMFLOAT4 rot = old_transform ? old_transform->GetQuaternion() : DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
	DirectX::XMFLOAT3 scale = old_transform ? old_transform->GetScale() : DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f);

	// 旧オブジェクトからモデルパスを取得
	std::string kept_model_path = "";
	auto old_model_comp = current_selected_object->GetComponent<ModelComponent>();
	if (old_model_comp)
	{
		kept_model_path = old_model_comp->GetModelPath();
	}
	if (kept_model_path.empty())
	{
		kept_model_path = current_model_path; // ドロップ時のキャッシュパスを使用
	}

	const std::string& target_class_name = cached_class_names[inspector_selected_class_index];

	// 適用したいクラスの新規インスタンスを生成
	GameObject* new_object = ObjectFactory::CreateAndRegister(target_class_name);

	if (new_object != nullptr)
	{
		// 姿勢コンポーネントの確保と引き継ぎ
		auto new_transform = new_object->GetComponent<TransformComponent>();
		if (!new_transform)
		{
			new_transform = new_object->AddComponent<TransformComponent>();
		}
		if (new_transform)
		{
			new_transform->SetPosition(pos);
			new_transform->SetRotationQuaternion(rot);
			new_transform->SetScale(scale);
		}

		// モデルコンポーネントの確保とモデル読み込み
		auto new_model_comp = new_object->GetComponent<ModelComponent>();
		if (!new_model_comp)
		{
			new_model_comp = new_object->AddComponent<ModelComponent>();
		}

		if (new_model_comp && new_transform)
		{
			new_model_comp->SetTransformComponent(new_transform);
			if (!kept_model_path.empty())
			{
				bool success = new_model_comp->LoadModel(kept_model_path);
				if (!success)
				{
					OutputDebugStringA("[ObjectEditor エラー] 新オブジェクトへのモデルロードに失敗しました。\n");
				}
			}
		}

		// 全コンポーネントが揃った状態で初期化
		new_object->Initialize();

		// 旧オブジェクトの破棄と選択ポインタの切り替え
		current_selected_object->Destory();
		current_selected_object = new_object;

	}
	else
	{
		OutputDebugStringA("[ObjectEditor エラー] ObjectFactory による新クラスの生成に失敗しました。\n");
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

//ギズモ描画
void ObjectEditor::DrawGizmo(Camera* camera)
{
	BS_PROFILE_FUNCTION();

	//実行前チェック
	if (current_selected_object == nullptr) return;
	if (!current_selected_object->IsActive())return;

	//キーボード入力による操作モードの切り替え
	if (Input::Instance().IsKeyTrigger('W')) current_gizmo_operation = ImGuizmo::TRANSLATE;
	if (Input::Instance().IsKeyTrigger('E')) current_gizmo_operation = ImGuizmo::ROTATE;
	if (Input::Instance().IsKeyTrigger('R')) current_gizmo_operation = ImGuizmo::SCALE;

	//ギズモの描画領域と対象レイヤーの設定
	ImGuiIO& io = ImGui::GetIO();
	const float screen_width = io.DisplaySize.x;	//画面の横幅
	const float screen_height = io.DisplaySize.y;	//画面の縦幅

	ImGuizmo::SetDrawlist(ImGui::GetBackgroundDrawList());
	ImGuizmo::SetRect(0.0f, 0.0f, screen_width, screen_height);

	auto transform = current_selected_object->GetComponent<TransformComponent>();
	if (!transform)
	{
		OutputDebugStringA("[ObjectEditor エラー] DrawGizmo: 対象オブジェクトに TransformComponent がありません。\n");
		return;
	}

	//行列データの取得とDirectXMathによる合成
	DirectX::XMFLOAT4X4 view_matrix = camera->GetView();		//カメラのビュー行列
	DirectX::XMFLOAT4X4 proj_matrix = camera->GetProjection();	//カメラのプロジェクション行列

	DirectX::XMFLOAT3 pos = transform->GetPosition();	// 座標
	DirectX::XMFLOAT4 rot = transform->GetQuaternion();	// クォータニオン角度
	DirectX::XMFLOAT3 scale = transform->GetScale();	// 大きさ

	DirectX::XMVECTOR v_pos = DirectX::XMLoadFloat3(&pos);		//座標ベクトル
	DirectX::XMVECTOR v_rot = DirectX::XMLoadFloat4(&rot);		//クォータニオンベクトル
	DirectX::XMVECTOR v_scale = DirectX::XMLoadFloat3(&scale);	//スケールベクトル

	DirectX::XMMATRIX mat_scale = DirectX::XMMatrixScalingFromVector(v_scale);		//スケール行列
	DirectX::XMMATRIX mat_rot = DirectX::XMMatrixRotationQuaternion(v_rot);			//回転行列
	DirectX::XMMATRIX mat_trans = DirectX::XMMatrixTranslationFromVector(v_pos);	//移動行列

	DirectX::XMMATRIX mat_world = mat_scale * mat_rot * mat_trans;	//ワールド変換行列

	constexpr size_t matrix_element_count = 16;	//4x4行列の要素数
	float object_matrix[matrix_element_count];	//ImGuizmoに渡すためのfloat配列

	DirectX::XMStoreFloat4x4(reinterpret_cast<DirectX::XMFLOAT4X4*>(object_matrix), mat_world);

	//ギズモの表示と操作判定
	ImGuizmo::Manipulate(
		&view_matrix.m[0][0],
		&proj_matrix.m[0][0],
		static_cast<ImGuizmo::OPERATION>(current_gizmo_operation),
		ImGuizmo::LOCAL,
		object_matrix
	);

	//操作結果の行列分解とクォータニオンの再適用

	if (ImGuizmo::IsUsing())
	{
		DirectX::XMMATRIX manipulated_matrix = DirectX::XMLoadFloat4x4(reinterpret_cast<const DirectX::XMFLOAT4X4*>(object_matrix));

		DirectX::XMVECTOR out_scale;	//スケール格納ベクトル
		DirectX::XMVECTOR out_rot_quat;	//クォータニオン格納ベクトル
		DirectX::XMVECTOR out_trans;	//座標格納ベクトル

		if (DirectX::XMMatrixDecompose(&out_scale, &out_rot_quat, &out_trans, manipulated_matrix))
		{
			DirectX::XMFLOAT3 new_pos;		//新しい座標
			DirectX::XMFLOAT4 new_rot;		//新しいクォータニオン
			DirectX::XMFLOAT3 new_scale;	//新しいスケール

			DirectX::XMStoreFloat3(&new_pos, out_trans);
			DirectX::XMStoreFloat4(&new_rot, out_rot_quat);
			DirectX::XMStoreFloat3(&new_scale, out_scale);

			transform->SetPosition(new_pos);
			transform->SetRotationQuaternion(new_rot);
			transform->SetScale(new_scale);
		}
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