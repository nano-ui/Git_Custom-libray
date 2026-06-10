#include "ObjectEditor.h"
#include "../GameObjects/ObjectFactory.h"
#include "../GameObjects/GameObject.h"
#include "../GameObjects/ObjectManager.h"
#include "../Collision/CollisionManager.h"
#include "../Camera/Camera.h"
#include "../Input/Input.h"

#include <imgui.h>
#include <ImGuizmo.h>
#include <string>
#include <unordered_map>
#include <fstream>
#include <filesystem>
#include <windows.h>
#include <commdlg.h>

static const std::string editor_config_path = "Data/System/EditorConfig.json";
static const std::string config_key_scene_path = "last_opened_scene";
static const std::string json_root_key_objects = "objects";

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
}

//デストラクタ
ObjectEditor::~ObjectEditor()
{

}

//初期化
void ObjectEditor::Initialize()
{
	selected_class_index = 0;
	current_selected_object = nullptr;
	cached_class_names = ObjectFactory::GetClassNames();

	//起動時のシーン復元処理
	std::string auto_load_path = LoadEditorConfig();
	if (!auto_load_path.empty() && std::filesystem::exists(auto_load_path))
	{
		LoadScene(auto_load_path);
	}
}

//更新
void ObjectEditor::Update(Camera* camera, CollisionManager* collision_manager)
{
	//実行前チェック
	if (!is_placement_mode)return;
	if (selected_class_index < 0 || selected_class_index >= static_cast<int>(cached_class_names.size()))return;

	ImGuiIO io = ImGui::GetIO();	//ImGuiの入出力状態
	
	if (io.WantCaptureMouse)return;

	constexpr int left_click_button = 0;	//左クリックインデックス

	//左クリックが押された瞬間のみレイキャスト処理を実行
	if (Input::Instance().IsKeyTrigger(VK_LBUTTON))
	{
		//画面座標からNDCへの変換
		const float screen_width = io.DisplaySize.x;	//画面横幅
		const float screen_height = io.DisplaySize.y;	//画面縦幅
		const float mouse_x = io.MousePos.x;			//マウスのX座標
		const float mouse_y = io.MousePos.y;			//マウスのY座標
		constexpr float ndc_multiplier = 2.0f;			//座標変換用の定数倍率
		constexpr float ndc_offset = 1.0f;				//座標変換用の定数オフセット

		const float ndc_x = (ndc_multiplier * mouse_x) / screen_width - ndc_offset;		//NDC上のX座標
		const float ndc_y = ndc_offset - (ndc_multiplier * mouse_y) / screen_height;	//NDC上のY座標

		//逆行列を用いた光線の生成
		DirectX::XMFLOAT4X4 vp_float4x4 = camera->GetViewProjectionMatrix();		//ビュープロジェクション行列		
		DirectX::XMMATRIX view_proj_matrix = DirectX::XMLoadFloat4x4(&vp_float4x4);	//変換後のビュープロジェクション行列
		DirectX::XMMATRIX inv_view_proj = DirectX::XMMatrixInverse(nullptr, view_proj_matrix);	//逆行列

		constexpr float depth_near = 0.0f;	//手前の深度値
		constexpr float depth_far = 1.0f;	//奥の深度値
		constexpr float w_value = 1.0f;		//同座標におけるW値

		DirectX::XMVECTOR near_point = DirectX::XMVectorSet(ndc_x, ndc_y, depth_near, w_value);	//手前の点
		DirectX::XMVECTOR far_point = DirectX::XMVectorSet(ndc_x, ndc_y, depth_far, w_value);	//奥の点

		near_point = DirectX::XMVector3TransformCoord(near_point, inv_view_proj);
		far_point = DirectX::XMVector3TransformCoord(far_point, inv_view_proj);

		DirectX::XMFLOAT3 ray_start = {};		//レイの開始座標
		DirectX::XMFLOAT3 ray_end = {};			//レイの終点座標
		DirectX::XMStoreFloat3(&ray_start, near_point);
		DirectX::XMStoreFloat3(&ray_end, far_point);

		//空間分割コリジョンへのレイキャストとオブジェクトの生成
		DirectX::XMFLOAT3 hit_position = {};	//衝突した座標
		
		if (collision_manager->RayCastSpace(ray_start, ray_end, hit_position))
		{
			const std::string& target_class_name = cached_class_names[selected_class_index];	//選択中のクラス名
			GameObject* new_object = ObjectFactory::CreateAndRegister(target_class_name);		//新しいオブジェクト

			if (new_object)
			{
				//new_object->Initialize();
				new_object->SetPosition(hit_position);
				current_selected_object = new_object;
			}
		}
	}
}

//描画
void ObjectEditor::RenderUi(Camera* camera, CollisionManager* collision_manager)
{
	//画面解像度に基づいた初期配置位置の計算
	ImGuiIO io = ImGui::GetIO();
	const float screen_width = io.DisplaySize.x;
	const float screen_height = io.DisplaySize.y;
	const float panel_width = screen_width * panel_width_ratio;

	ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(panel_width, screen_height), ImGuiCond_FirstUseEver);

	//画面を左右に分割するメインウインドウ
	ImGui::Begin("Hierarchy");
	DrawLeftPane(camera, collision_manager);

	ImGui::Dummy(ImVec2(0.0f, dummy_height_value));
	ImGui::Separator();
	ImGui::Text("Global Scene Operations");

	if (ImGui::Button("Save Scene Layout", ImVec2(-1, 0)))
	{
		std::string dynamic_save_path = SelectSavePath();
		if (!dynamic_save_path.empty())
		{
			SaveScene(dynamic_save_path);
		}
	}

	if (ImGui::Button("Load Scene Layout", ImVec2(-1, 0)))
	{
		std::string dynamic_load_path = SelectOpenPath();
		if (!dynamic_load_path.empty())
		{
			LoadScene(dynamic_load_path);
		}
	}

	ImGui::End();

	const float right_window_pos_x = screen_width - panel_width;
	ImGui::SetNextWindowPos(ImVec2(right_window_pos_x, 0.0f), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(panel_width, screen_height), ImGuiCond_FirstUseEver);

	ImGui::Begin("Inspector");
	DrawRightPane();
	ImGui::End();

	DrawGizmo(camera);
}

//オブジェクト生成UI描画
void ObjectEditor::DrawLeftPane(Camera* camera, CollisionManager* collision_manager)
{
	//クラス名リストの取得とリストボックスの描画
	ImGui::Text("Registered Classes");
	ImGui::Separator();

	if (!cached_class_names.empty())
	{
		ImGui::Checkbox("Enable Click Placement Mode", &is_placement_mode);
		ImGui::Dummy(ImVec2(0.0f, dummy_height_value));

		if (ImGui::ListBoxHeader("##ClassList", ImVec2(-1, ImGui::GetContentRegionAvail().y * 0.25f)))
		{
			for (int i = 0; i < static_cast<int>(cached_class_names.size()); ++i)
			{
				const bool is_selected = (selected_class_index == i);

				if (ImGui::Selectable(cached_class_names[i].c_str(), is_selected))
				{
					selected_class_index = i;
				}

				if (is_selected)
				{
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::ListBoxFooter();
		}
	}

	//生成ボタン処理
	if (ImGui::Button("Create and Register", ImVec2(-1, 0)))
	{
		const std::string& target_class_name = cached_class_names[selected_class_index];
		GameObject* new_object = ObjectFactory::CreateAndRegister(target_class_name);
		if (new_object)
		{
			//new_object->Initialize();
			current_selected_object = new_object;
		}
	}
	else
	{
		ImGui::Text("No class registered in ObjectFactory");
	}
	ImGui::Dummy(ImVec2(0.0f, dummy_height_value));

	//現在生成されているオブジェクトの一覧リスト描画
	ImGui::Text("Active Object List");
	ImGui::Separator();
	const auto& active_objects = ObjectManager::Instance().GetGameObjects();
	frame_class_counters.clear();

	if (ImGui::ListBoxHeader("##ActiveObjects", ImVec2(-1, ImGui::GetContentRegionAvail().y - active_list_height_offset)))
	{
		for (size_t i = 0; i < active_objects.size(); ++i)
		{
			if (active_objects[i]->IsActive())
			{
				GameObject* obj_ptr = active_objects[i].get();
				std::string current_class_name = obj_ptr->GetClassName();
				int current_number = frame_class_counters[current_class_name];
				frame_class_counters[current_class_name]++;
				ImGui::PushID(reinterpret_cast<const void*>(obj_ptr));

				char label_buffer[label_buffer_size];
				snprintf(label_buffer, sizeof(label_buffer), "%s %d", current_class_name.c_str(), current_number);
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
		ImGui::ListBoxFooter();
	}

	Update(camera, collision_manager);
}

//オブジェクトパラメータ編集UI描画
void ObjectEditor::DrawRightPane()
{
	//選択中のオブジェクト情報の表示
	ImGui::Text("Object Properties");
	ImGui::Separator();

	if (current_selected_object != nullptr)
	{
		if (current_selected_object->IsActive())
		{
			ImGui::Dummy(ImVec2(0.0f, 10.0f));
			ImGui::Separator();

			//オブジェクトの削除ボタン
			if (ImGui::Button("Destroy Object", ImVec2(-1, 0)))
			{
				current_selected_object->Destory();
				current_selected_object = nullptr;
				return;
			}
			ImGui::Dummy(ImVec2(0.0f, dummy_height_value));
			ImGui::Separator();
			current_selected_object->RenderGui();
		}
		else
		{
			ImGui::Text("The selected object has been destroyed.");
			current_selected_object = nullptr;
		}
	}
	else
	{
		ImGui::Text("Please select or create an object from the left pane.");
	}
}

//ギズモ描画
void ObjectEditor::DrawGizmo(Camera* camera)
{
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

	//行列データの取得とDirectXMathによる合成
	DirectX::XMFLOAT4X4 view_matrix = camera->GetView();		//カメラのビュー行列
	DirectX::XMFLOAT4X4 proj_matrix = camera->GetProjection();	//カメラのプロジェクション行列

	DirectX::XMFLOAT3 pos = current_selected_object->GetPosition();	//座標
	DirectX::XMFLOAT4 rot = current_selected_object->GetRotation();	//角度
	DirectX::XMFLOAT3 scale = current_selected_object->GetScale();	//大きさ

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

			current_selected_object->SetPosition(new_pos);
			current_selected_object->SetRotation(new_rot);
			current_selected_object->SetScale(new_scale);
		}
	}
}

//配置されているすべてのオブジェクト状態を保存
void ObjectEditor::SaveScene(const std::string& file_path)
{
	//指定された保存先フォルダの動的自動生成
	std::filesystem::path system_path(file_path);
	if (system_path.has_parent_path())
	{
		std::filesystem::create_directories(system_path.parent_path());
	}

	nlohmann::json scene_json;												//シーン全体のルート階層となるJSONオブジェクト
	nlohmann::json objects_array = nlohmann::json::array();					//各オブジェクトデータを並べるためのJSON配列
	const auto& active_object = ObjectManager::Instance().GetGameObjects();	//現在マネージャーが管理している全オブジェクト

	//現在アクティブなオブジェクトを1つずつ走査して情報をパッケージング
	for (size_t i = 0; i < active_object.size(); i++)
	{
		if (active_object[i] != nullptr && active_object[i]->IsActive())
		{
			nlohmann::json object_node;	//オブジェクト専用のJSONノード
			object_node["class_name"] = active_object[i]->GetClassNameW();
			nlohmann::json data_node;	//パラメータを格納するための配下ノード
			active_object[i]->SaveToJObject(data_node);
			object_node["data"] = data_node;
			objects_array.push_back(object_node);
		}
	}

	scene_json["object"] = objects_array;

	//構築したJSON構造をファイルへ書き出す
	std::ofstream output_file(file_path); 
	if(output_file.is_open())
	{
		output_file << scene_json.dump(json_indent_space_count);
		output_file.close();
		SaveEditorConfig(file_path);
	}
}

//ファイルからオブジェクト群を自動生成して状態を復元
void ObjectEditor::LoadScene(const std::string& file_path)
{
	std::ifstream input_file(file_path);
	if (!input_file.is_open())
	{
		return;
	}

	nlohmann::json scene_json;	//解析結果を受け取るためのJSONオブジェクト
	input_file >> scene_json;
	input_file.close();

	if (!scene_json.contains("object"))
	{
		return;
	}

	current_selected_object = nullptr;

	//古いオブジェクトを全削除
	const auto& active_objects = ObjectManager::Instance().GetGameObjects();
	for (size_t i = 0; i < active_objects.size(); i++)
	{
		if (active_objects[i] && active_objects[i]->IsActive())
		{
			active_objects[i]->Destory();
		}
	}

	const nlohmann::json& objects_array = scene_json["object"];	//オブジェクト配列ノードへの参照

	//配列に記録されているデータからオブジェクトを全自動動的生成
	for (size_t i = 0; i < objects_array.size(); ++i)
	{
		const nlohmann::json& object_node = objects_array[i];	//現在のインデックスの配列要素

		if (object_node.contains("class_name") && object_node.contains("data"))
		{
			std::string class_name = object_node["class_name"].get<std::string>();	//記録されているクラス名
			GameObject* new_object = ObjectFactory::CreateAndRegister(class_name);

			if (new_object != nullptr)
			{
				new_object->LoadFromJObject(object_node["data"]);
			}
		}
	}
	SaveEditorConfig(file_path);
}

//保存先のファイルパスをダイアログから選択取得
std::string ObjectEditor::SelectSavePath()
{
	char absolute_path_buffer[file_path_buffer_size] = "";
	OPENFILENAMEA open_file_name_struct = {};

	open_file_name_struct.lStructSize = sizeof(open_file_name_struct);
	open_file_name_struct.hwndOwner = nullptr;
	open_file_name_struct.lpstrFilter = "JSON Files (*.json)\0*.json\0All Files (*.*)\0*.*\0";
	open_file_name_struct.lpstrFile = absolute_path_buffer;
	open_file_name_struct.nMaxFile = file_path_buffer_size;
	open_file_name_struct.lpstrDefExt = "json";
	open_file_name_struct.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;

	if (GetSaveFileNameA(&open_file_name_struct))
	{
		return std::string(absolute_path_buffer);
	}

	return std::string();
}

//読み込み元のファイルパスをダイアログから選択取得
std::string ObjectEditor::SelectOpenPath()
{
	char absolute_path_buffer[file_path_buffer_size] = "";
	OPENFILENAMEA open_file_name_struct = {};

	open_file_name_struct.lStructSize = sizeof(open_file_name_struct);
	open_file_name_struct.hwndOwner = nullptr;
	open_file_name_struct.lpstrFilter = "JSON Files (*.json)\0*.json\0All Files (*.*)\0*.*\0";
	open_file_name_struct.lpstrFile = absolute_path_buffer;
	open_file_name_struct.nMaxFile = file_path_buffer_size;
	open_file_name_struct.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;

	if (GetOpenFileNameA(&open_file_name_struct))
	{
		return std::string(absolute_path_buffer);
	}

	return std::string();
}

//エディタ設定ファイルの保存
void ObjectEditor::SaveEditorConfig(const std::string& last_scene_path)
{
	//設定JSONのデータアロケーション
	nlohmann::json config_json;	//設定をパッキングするためのJSONノード
	config_json[config_key_scene_path] = last_scene_path;
	
	std::filesystem::path system_path(editor_config_path);
	if (system_path.has_parent_path())
	{
		std::filesystem::create_directories(system_path.parent_path());
	}

	std::ofstream output_file(editor_config_path);
	if (output_file.is_open())
	{
		output_file << config_json.dump(json_indent_space_count);
		output_file.close();
	}
}

//エディタ設定ファイルの読み込み
std::string ObjectEditor::LoadEditorConfig()
{
	std::ifstream input_file(editor_config_path);	//設定ファイルを読み込みモード
	if (!input_file)
	{
		return std::string();
	}

	nlohmann::json config_json;	//データをパースするためのオブジェクト
	input_file >> config_json;
	input_file.close();

	if (config_json.contains(config_key_scene_path))
	{
		return config_json[config_key_scene_path].get<std::string>();
	}

	return std::string();
}
