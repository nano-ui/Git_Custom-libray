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
#include "FileDialogHelper.h"

#include <imgui.h>
#include <ImGuizmo.h>
#include <string>
#include <unordered_map>
#include <fstream>
#include <filesystem>
#include <windows.h>
#include <commdlg.h>
#include "StateMachineEditor\StateGraphConfigManager.h"

static const std::string editor_config_path = "Data/Json/System/EditorConfig.json";
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
	EditorMediator::Instance().RegisterObjectEditor(this);

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
			const std::string& target_class_name = cached_class_names[selected_class_index];
			printf("[デバッグログ] クリック配置実行: 選択クラス = %s\n", target_class_name.c_str());

			GameObject* new_object = ObjectFactory::CreateAndRegister(target_class_name);

			if (new_object)
			{
				new_object->Initialize();

				auto transform_component = new_object->GetComponent<TransformComponent>();
				if (transform_component)
				{
					transform_component->SetPosition(hit_position);
				}

				// 配置モードでモデルパスが保持されている場合は読み込む
				if (!current_model_path.empty())
				{
					auto model_comp = new_object->GetComponent<ModelComponent>();
					if (model_comp)
					{
						bool success = model_comp->LoadModel(current_model_path);
						printf("[デバッグログ] クリック配置モデルロード結果: %s (パス: %s)\n", success ? "成功" : "失敗", current_model_path.c_str());
					}
				}

				current_selected_object = new_object;
			}
			else
			{
				printf("[デバッグ エラー] クリック配置での ObjectFactory 生成失敗 (クラス: %s)\n", target_class_name.c_str());
			}
		}
		else
		{
			printf("[デバッグ 警告] レイキャストがヒットしなかったため生成をキャンセルしました。\n");
		}
	}
}

//描画
void ObjectEditor::RenderUi(Camera* camera, CollisionManager* collision_manager)
{
	HandleDragDropTarget(camera, collision_manager);

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

	if (ImGui::Button(u8"シーン保存", ImVec2(-1, 0)))
	{
		std::string dynamic_save_path = SelectSavePath();
		if (!dynamic_save_path.empty())
		{
			SaveScene(dynamic_save_path);
		}
	}

	if (ImGui::Button(u8"シーン読み込み", ImVec2(-1, 0)))
	{
		std::string dynamic_load_path = SelectOpenPath();
		if (!dynamic_load_path.empty())
		{
			LoadScene(dynamic_load_path);
		}
	}

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
	std::string dynamic_save_path = SelectSavePath();
	if (!dynamic_save_path.empty())
	{
		SaveScene(dynamic_save_path);
	}
}

//シーン読み込み
void ObjectEditor::LoadSceneWithDialog()
{
	std::string dynamic_load_path = SelectOpenPath();

	if (!dynamic_load_path.empty())
	{
		LoadScene(dynamic_load_path);
	}
}

//仮オブジェクト生成
void ObjectEditor::CreateTempModelObject(const std::string& model_path)
{
	printf("\n==========================================\n");
	printf("[デバッグログ] CreateTempModelObject 開始\n");
	printf("[デバッグログ] ドロップされたモデルパス: %s\n", model_path.c_str());

	if (model_path.empty() || cached_class_names.empty())
	{
		printf("[デバッグ エラー] model_path または cached_class_names が空です。\n");
		printf("==========================================\n\n");
		return;
	}

	current_model_path = model_path;
	std::string model_name = std::filesystem::path(model_path).stem().string();
	std::string detail_file_path = "Data/Json/" + model_name + "/" + model_name + ".json";

	bool is_json_loaded = false;

	// 1. 個別JSONファイルの存在チェック
	if (std::filesystem::exists(detail_file_path))
	{
		printf("[デバッグログ] 既存の個別JSONを発見しました: %s\n", detail_file_path.c_str());
		std::ifstream detail_file(detail_file_path);
		if (detail_file.is_open())
		{
			nlohmann::json detail_json;
			detail_file >> detail_json;
			detail_file.close();

			if (detail_json.contains("class_name"))
			{
				std::string target_class_name = detail_json["class_name"].get<std::string>();
				printf("[デバッグログ] JSON記録クラス名: %s\n", target_class_name.c_str());

				// クラスの生成
				GameObject* new_object = ObjectFactory::CreateAndRegister(target_class_name);
				if (new_object != nullptr)
				{
					new_object->Initialize();
					auto model_component = new_object->GetComponent<ModelComponent>();
					if (model_component != nullptr)
					{
						bool success = model_component->LoadModel(current_model_path);
						printf("[デバッグログ] ModelComponent::LoadModel (%s): %s\n", current_model_path.c_str(), success ? "成功" : "失敗");
					}
					else
					{
						printf("[デバッグ 警告] 生成されたオブジェクト (%s) に ModelComponent がアタッチされていません。\n", target_class_name.c_str());
					}

					new_object->LoadFromJObject(detail_json);
					current_selected_object = new_object;
					is_json_loaded = true;
				}
				else
				{
					printf("[デバッグ エラー] ObjectFactory でクラス (%s) のインスタンス生成に失敗しました。\n", target_class_name.c_str());
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

		printf("[デバッグログ] JSON未検出のため適用予定クラス: %s\n", target_class_name.c_str());

		// オブジェクトの生成
		GameObject* new_object = ObjectFactory::CreateAndRegister(target_class_name);
		if (new_object != nullptr)
		{
			printf("[デバッグログ] ObjectFactory 生成成功: %s\n", target_class_name.c_str());
			new_object->Initialize();

			auto model_component = new_object->GetComponent<ModelComponent>();
			if (model_component != nullptr)
			{
				bool success = model_component->LoadModel(current_model_path);
				printf("[デバッグログ] ModelComponent::LoadModel (%s): %s\n", current_model_path.c_str(), success ? "成功" : "失敗");
			}
			else
			{
				printf("[デバッグ 警告] 生成されたオブジェクト (%s) に ModelComponent がアタッチされていません。\n", target_class_name.c_str());
			}

			current_selected_object = new_object;
		}
		else
		{
			printf("[デバッグ エラー] ObjectFactory でクラス (%s) のインスタンス生成に失敗しました。\n", target_class_name.c_str());
		}
	}
	printf("==========================================\n\n");
}

//オブジェクト生成UI描画
void ObjectEditor::DrawLeftPane(Camera* camera, CollisionManager* collision_manager)
{
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

	Update(camera, collision_manager);
}

//クラス適用UI描画
void ObjectEditor::DrawClassApplySection()
{
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
	printf("\n==========================================\n");
	printf("[デバッグログ] ApplySelectedClassToObject (クラス置換) 開始\n");

	if (current_selected_object == nullptr)
	{
		printf("[デバッグ エラー] 置換対象の current_selected_object が nullptr です。\n");
		printf("==========================================\n\n");
		return;
	}

	// 旧オブジェクトから Transform 情報を取得
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
	printf("[デバッグログ] 変更前クラス: %s -> 変更後クラス: %s\n", current_selected_object->GetClassNameW().c_str(), target_class_name.c_str());
	printf("[デバッグログ] 引き継ぐモデルパス: %s\n", kept_model_path.c_str());

	// 適用したいクラスの新規インスタンスを生成
	GameObject* new_object = ObjectFactory::CreateAndRegister(target_class_name);

	if (new_object != nullptr)
	{
		auto new_model_comp = new_object->GetComponent<ModelComponent>();
		if (!new_model_comp)
		{
			new_model_comp = new_object->AddComponent<ModelComponent>();
		}

		if (!kept_model_path.empty() && new_model_comp)
		{
			bool success = new_model_comp->LoadModel(kept_model_path);
			printf("[デバッグログ] 新オブジェクトへのモデルロード結果: %s\n", success ? "成功" : "失敗");
		}

		new_object->Initialize();

		auto new_transform = new_object->GetComponent<TransformComponent>();
		if (new_transform)
		{
			new_transform->SetPosition(pos);
			new_transform->SetRotationQuaternion(rot);
			new_transform->SetScale(scale);
		}

		// 旧オブジェクトの破棄とポインタ差し替え
		current_selected_object->Destory();
		current_selected_object = new_object;
		printf("[デバッグログ] クラス置換処理が完了しました。\n");
	}
	else
	{
		printf("[デバッグ エラー] ObjectFactory による新クラス (%s) の生成に失敗しました。\n", target_class_name.c_str());
	}
	printf("==========================================\n\n");
}

//オブジェクトパラメータ編集UI描画
void ObjectEditor::DrawRightPane()
{
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

//配置されているすべてのオブジェクト状態を保存
void ObjectEditor::SaveScene(const std::string& file_path)
{
	//指定された保存先フォルダの動的自動生成
	std::filesystem::path system_path(file_path);
	if (system_path.has_parent_path())
	{
		std::filesystem::create_directories(system_path.parent_path());
	}

	nlohmann::json scene_json;                                               // シーン全体のルートJSON
	nlohmann::json objects_array = nlohmann::json::array();					//各オブジェクトデータを並べるためのJSON配列
	const auto& active_object = ObjectManager::Instance().GetGameObjects();	//現在マネージャーが管理している全オブジェクト

	std::unordered_map<std::string, int> save_counters;	//モデル名ごとの連番管理用マップ

	//現在アクティブなオブジェクトを1つずつ走査して情報をパッケージング
	for (size_t i = 0; i < active_object.size(); i++)
	{
		if (active_object[i] != nullptr && active_object[i]->IsActive())
		{
			GameObject* obj = active_object[i].get();
			auto model_component = obj->GetComponent<ModelComponent>();
			auto transform_component = obj->GetComponent<TransformComponent>();

			//モデル名(ファイル名)の取得
			std::string model_name = "";
			std::string model_path = "";
			if (model_component != nullptr && !model_component->GetModelPath().empty())
			{
				model_path = model_component->GetModelPath();
				model_name = std::filesystem::path(model_path).stem().string();
			}
			else model_name = obj->GetClassNameW();

			//個別Jsonファイル用パスの設定
			std::string detail_dir = "Data/Json/" + model_name;
			std::string detail_file_path = detail_dir + "/" + model_name + ".json";

			//ディレクトリの生成
			std::filesystem::create_directories(detail_dir);

			//個別情報Jsonの書き出し
			nlohmann::json detail_json;
			detail_json["class_name"] = obj->GetClassNameW();
			obj->SaveToJObject(detail_json);

			std::ofstream detail_file(detail_file_path);
			if (detail_file.is_open())
			{
				detail_file << detail_json.dump(json_indent_space_count);
				detail_file.close();
			}
			else OutputDebugStringA("[エラー] SaveScene: 個別JSONファイルのオープンに失敗しました。\n");

			//シーンJsonへ最小限の情報のみを記録
			nlohmann::json node;
			node["model_path"] = model_path;
			node["position"] = transform_component ? transform_component->GetPosition() : DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
			node["rotation"] = transform_component ? transform_component->GetQuaternion() : DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
			node["scale"] = transform_component ? transform_component->GetScale() : DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f);
			node["detail_json_path"] = detail_file_path;
			objects_array.push_back(node);
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
		OutputDebugStringA("[情報] SaveScene: シーンデータと個別データの分離保存が完了しました。\n");
	}
	else OutputDebugStringA("[エラー] SaveScene: シーンJSONファイルのオープンに失敗しました。\n");
}

//ファイルからオブジェクト群を自動生成して状態を復元
void ObjectEditor::LoadScene(const std::string& file_path)
{
	std::ifstream input_file(file_path);
	if (!input_file.is_open())
	{
		OutputDebugStringA("[エラー] LoadScene: 指定されたシーンファイルが開けませんでした。\n");
		return;
	}

	nlohmann::json scene_json;	//解析結果を受け取るためのJSONオブジェクト
	input_file >> scene_json;
	input_file.close();

	if (!scene_json.contains("object"))return;

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

		if (object_node.contains("detail_json_path"))
		{
			std::string detail_path = object_node["detail_json_path"].get<std::string>();
			std::ifstream detail_file(detail_path);

			if (detail_file)
			{
				nlohmann::json detail_json;
				detail_file >> detail_json;
				detail_file.close();

				//個別JSON内に記録されている class_name を取得して生成
				if (detail_json.contains("class_name"))
				{
					std::string class_name = detail_json["class_name"].get<std::string>();
					GameObject* new_object = ObjectFactory::CreateAndRegister(class_name);

					if (new_object != nullptr)
					{
						auto transform_component = new_object->GetComponent<TransformComponent>();
						auto model_component = new_object->GetComponent<ModelComponent>();

						//Transform 情報の復元
						if (transform_component)
						{
							if (object_node.contains("position")) transform_component->SetPosition(object_node["position"].get<DirectX::XMFLOAT3>());
							if (object_node.contains("rotation")) transform_component->SetRotationQuaternion(object_node["rotation"].get<DirectX::XMFLOAT4>());
							if (object_node.contains("scale"))    transform_component->SetScale(object_node["scale"].get<DirectX::XMFLOAT3>());
						}

						//モデルパスの適用
						if (object_node.contains("model_path"))
						{
							std::string model_path = object_node["model_path"].get<std::string>();
							if (!model_path.empty() && model_component != nullptr)
							{
								model_component->LoadModel(model_path);
							}
						}

						//個別JSONからステータス等の復元
						new_object->LoadFromJObject(detail_json);
					}
				}
			}
			else OutputDebugStringA("[エラー] LoadScene: 個別JSONファイルが開けませんでした。\n");
		}
	}
	SaveEditorConfig(file_path);
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

//ドラッグターゲットの監視処理
void ObjectEditor::HandleDragDropTarget(Camera* camera, CollisionManager* collision_manager)
{
	if (ImGui::GetDragDropPayload() != nullptr)
	{
		ImGuiIO& io = ImGui::GetIO();
		ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
		ImGui::SetNextWindowSize(io.DisplaySize);
		ImGui::SetNextWindowBgAlpha(0.0f);

		constexpr ImGuiWindowFlags viewport_flags =
			ImGuiWindowFlags_NoTitleBar |
			ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoScrollbar |
			ImGuiWindowFlags_NoSavedSettings |
			ImGuiWindowFlags_NoBringToFrontOnFocus;

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

		if (ImGui::Begin("##ViewportDropTarget", nullptr, viewport_flags))
		{
			ImGui::InvisibleButton("##ViewportDropArea", io.DisplaySize);

			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("MODEL_FILE_PATH"))
				{
					if (payload->Data != nullptr)
					{
						std::string model_path(static_cast<const char*>(payload->Data));

						// Mediator 経由で単一のモデル配置を実行 (CreateTempModelObject)
						EditorMediator::Instance().OnModelDropped(model_path);

						// ドロップ位置のレイキャスト計算と座標反映
						if (current_selected_object != nullptr && camera != nullptr && collision_manager != nullptr)
						{
							const float screen_width = io.DisplaySize.x;
							const float screen_height = io.DisplaySize.y;
							const float mouse_x = io.MousePos.x;
							const float mouse_y = io.MousePos.y;
							constexpr float ndc_multiplier = 2.0f;
							constexpr float ndc_offset = 1.0f;

							const float ndc_x = (ndc_multiplier * mouse_x) / screen_width - ndc_offset;
							const float ndc_y = ndc_offset - (ndc_multiplier * mouse_y) / screen_height;

							DirectX::XMFLOAT4X4 vp_float4x4 = camera->GetViewProjectionMatrix();
							DirectX::XMMATRIX view_proj_matrix = DirectX::XMLoadFloat4x4(&vp_float4x4);
							DirectX::XMMATRIX inv_view_proj = DirectX::XMMatrixInverse(nullptr, view_proj_matrix);

							constexpr float depth_near = 0.0f;
							constexpr float depth_far = 1.0f;
							constexpr float w_value = 1.0f;

							DirectX::XMVECTOR near_point = DirectX::XMVectorSet(ndc_x, ndc_y, depth_near, w_value);
							DirectX::XMVECTOR far_point = DirectX::XMVectorSet(ndc_x, ndc_y, depth_far, w_value);

							near_point = DirectX::XMVector3TransformCoord(near_point, inv_view_proj);
							far_point = DirectX::XMVector3TransformCoord(far_point, inv_view_proj);

							DirectX::XMFLOAT3 ray_start = {};
							DirectX::XMFLOAT3 ray_end = {};
							DirectX::XMStoreFloat3(&ray_start, near_point);
							DirectX::XMStoreFloat3(&ray_end, far_point);

							DirectX::XMFLOAT3 hit_position = { 0.0f, 0.0f, 0.0f };
							bool is_hit = collision_manager->RayCastSpace(ray_start, ray_end, hit_position);

							// 配置したオブジェクトの TransformComponent に位置を反映
							auto transform = current_selected_object->GetComponent<TransformComponent>();
							if (transform)
							{
								transform->SetPosition(hit_position);
							}
						}
					}
					else OutputDebugStringA("[Error] ObjectEditor: HandleDragDropTarget - payload data is nullptr!\n");
				}
				ImGui::EndDragDropTarget();
			}
		}
		ImGui::End();
		ImGui::PopStyleVar(2);
	}
}
