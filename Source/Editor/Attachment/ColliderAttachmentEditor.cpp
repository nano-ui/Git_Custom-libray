#include "ColliderAttachmentEditor.h"
#include "Editor\Preview\ModelPreviewWindow.h"
#include "Editor\FileDialogHelper.h"
#include "Engine\Graphics\Resources\Model.h"
#include "Engine\Graphics\Renderers\ShapeRenderer.h"
#include "Engine\Camera\Camera.h"
#include "Engine\Collision\CollisionLogic.h"

#include <Windows.h>
#include <imgui.h>
#include <algorithm>
#include <fstream>

//コンストラクタ
ColliderAttachmentEditor::ColliderAttachmentEditor()
{
	collision_logic = std::make_unique<CollisionLogic>();
}

//デストラクタ
ColliderAttachmentEditor::~ColliderAttachmentEditor() = default;

//初期化処理
void ColliderAttachmentEditor::Initialize()
{
	collider_items.clear();
	selected_item_index = -1;
}

//更新処理
void ColliderAttachmentEditor::Update(ModelPreviewWindow* preview_wnidow)
{
	if (!preview_wnidow)return;

	//左クリック時レイキャスト判定
	if (preview_wnidow->IsViewportImageClicked())
	{
		ImVec2 img_pos = preview_wnidow->GetViewportImagePos();
		ImVec2 img_size = preview_wnidow->GetViewportImageSize();
		HandleRaycastSelection(preview_wnidow, img_pos, img_size);
	}
}

//ImGui描画及びレイキャストによる選択処理
void ColliderAttachmentEditor::RenderGui(ModelPreviewWindow* preview_window)
{
	ImGui::SetNextWindowSize(ImVec2(360.0f, 520.0f), ImGuiCond_FirstUseEver);
	if (!ImGui::Begin(u8"コライダーアタッチメントエディタ"))
	{
		ImGui::End();
		return;
	}

	if (!preview_window)
	{
		OutputDebugStringA("[ColliderAttachmentEditor 警告] RenderGui: preview_window が nullptr です。\n");
		ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), u8"プレビューウィンドウが存在しません。");
		ImGui::End();
		return;
	}

	ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.8f, 1.0f), u8"ビューポート上のモデルをクリックすると、");
	ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.8f, 1.0f), u8"当たった個所の最上位階層ノードが自動でアタッチされます");

	ImGui::Spacing();

	ImGui::Checkbox(u8"アタッチコライダーを表示", &is_draw_colliders);
	ImGui::SameLine();
	ImGui::Checkbox(u8"ノード当たり判定を表示", &is_draw_node_spheres);
	ImGui::Separator();

	if (ImGui::Button(u8"新規コライダー追加"))
	{
		auto new_item = std::make_unique<ColliderAttachmentItem>();
		new_item->name = "Collider_" + std::to_string(collider_items.size());
		collider_items.push_back(std::move(new_item));
		selected_item_index = static_cast<int>(collider_items.size() - 1);
	}

	ImGui::SameLine();
	if (ImGui::Button(u8"選択項目削除"))
	{
		if (selected_item_index >= 0 && selected_item_index < static_cast<int>(collider_items.size()))
		{
			collider_items.erase(collider_items.begin() + selected_item_index);
			selected_item_index = -1;
		}
	}

	ImGui::Spacing();
	ImGui::Text(u8"登録コライダー一覧");
	if (ImGui::BeginListBox("##AttachmentList", ImVec2(-1.0f, 120.0f)))
	{
		for (int i = 0; i < static_cast<int>(collider_items.size()); i++)
		{
			const bool is_selected = (selected_item_index == i);
			std::string label = collider_items[i]->name + "[" + collider_items[i]->start_bone_name + "]";
			if (ImGui::Selectable(label.c_str(), is_selected))
			{
				selected_item_index = i;
			}
		}
		ImGui::EndListBox();
	}

	ImGui::Separator();

	//パラメータ編集部
	if (selected_item_index >= 0 && selected_item_index < static_cast<int>(collider_items.size()))
	{
		ColliderAttachmentItem* item = collider_items[selected_item_index].get();

		char name_buf[128] = {};
		strncpy_s(name_buf, item->name.c_str(), sizeof(name_buf) - 1);
		if (ImGui::InputText(u8"コライダー名", name_buf, sizeof(name_buf)))
		{
			item->name = name_buf;
		}

		ImGui::Text(u8"アタッチ中ノード: %s", item->start_bone_name.empty() ? u8"(未設定)" : item->start_bone_name.c_str());
		ImGui::Checkbox(u8"有効", &item->is_active);

		//当たり判定の属性選択コンボボックス
		static const char* attribute_names[] = {
			u8"なし(None)",
			u8"地形(Stage)",
			u8"動的衝突(Collision)",
			u8"攻撃判定(Attack)"
		};
		int current_attr_idx = static_cast<int>(item->attribute);
		if (ImGui::Combo(u8"当たり判定属性", &current_attr_idx, attribute_names,IM_ARRAYSIZE(attribute_names)))
		{
			item->attribute = static_cast<ColliderAttribute>(current_attr_idx);
			item->attribute_int = current_attr_idx;
		}

		if (ImGui::Checkbox(u8"2ボーン連携", &item->is_two_bone_link))
		{
			//連携ONに切り替えた際に、終点が空なら終点選択モードに移行
			if (item->is_two_bone_link && item->end_bone_name.empty())
			{
				current_select_slot = BoneSelectSlot::End;
			}
			else
			{
				current_select_slot = BoneSelectSlot::Start;
			}
		}

		ImGui::Spacing();

		//始点ボーンスロット
		bool is_slot_start = (current_select_slot == BoneSelectSlot::Start);
		if (ImGui::RadioButton(u8"始点ボーンを選択中", is_slot_start))
		{
			current_select_slot = BoneSelectSlot::Start;
		}
		ImGui::SameLine();
		ImGui::TextColored(is_slot_start ? ImVec4(1.0f, 1.0f, 0.2f, 1.0f) : ImVec4(0.7f, 0.7f, 0.7f, 1.0f),
			"[%s]", item->start_bone_name.empty() ? u8"未設定" : item->start_bone_name.c_str());

		//終点ボーンスロット
		if (item->is_two_bone_link)
		{
			bool is_slot_end = (current_select_slot == BoneSelectSlot::End);
			if (ImGui::RadioButton(u8"終点ボーンを選択中", is_slot_end))
			{
				current_select_slot = BoneSelectSlot::End;
			}
			ImGui::SameLine();
			ImGui::TextColored(is_slot_end ? ImVec4(1.0f, 1.0f, 0.2f, 1.0f) : ImVec4(0.7f, 0.7f, 0.7f, 1.0f),
				"[%s]", item->end_bone_name.empty() ? u8"未設定" : item->end_bone_name.c_str());

			if (item->start_bone_name.empty() || item->end_bone_name.empty())
			{
				ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.2f, 1.0f), u8"※モデル上をクリックして両方のボーンを設定してください。");
			}
		}

		ImGui::DragFloat(u8"カプセル半径", &item->radius, 0.01f, 0.01f, 10.0f);
		if(!item->is_two_bone_link)
		{
			ImGui::DragFloat(u8"カプセルの長さ", &item->height, 0.01f, 0.0f, 20.0f);
			ImGui::DragFloat3(u8"ローカルオフセット", &item->offset.x, 0.01f, -10.0f, 10.0f);
			ImGui::DragFloat3(u8"角度", &item->rotation.x, 0.5f, -360.0f, 360.0f);
		}
	}
	ImGui::DragFloat(u8"ノード半径", &node_radio);


	ImGui::Separator();
	ImGui::Spacing();

	//現在のモデル名からデフォルトパスを算出
	std::string default_path = GetDefaultFilePath(preview_window);

	if (!default_path.empty())
	{
		ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), u8"[モデル連動ファイル入出力]");

		if (ImGui::Button(u8"保存"))
		{
			save_file_path = default_path;
			SaveToJson(save_file_path);
		}
		ImGui::SameLine();
		if (ImGui::Button(u8"読み込み"))
		{
			save_file_path = default_path;
			LoadFromJson(save_file_path);
		}
		ImGui::TextWrapped(u8"対象パス: %s", default_path.c_str());
		ImGui::Spacing();
	}
	else
	{
		ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.2f, 1.0f), u8"※モデルがロードされていないため、デフォルトパスを特定できません。");
	}
	
	ImGui::Separator();

	//ファイル保存
	if (ImGui::Button(u8"ダイアログから保存"))
	{
		PathResult save_result = FileDialogHelper::SaveGenericFileDialog("json", "JSON Files (*.json)\0*.json\0All Files (*.*)\0*.*\0\0");
		if (!save_result.absolute_path.empty())
		{
			save_file_path = save_result.absolute_path;
			SaveToJson(save_file_path);
		}
	}

	ImGui::SameLine();


	//ファイル読み込み
	if (ImGui::Button(u8"ダイアログから読み込み"))
	{
		PathResult open_result = FileDialogHelper::OpenGenericFileDialog();
		if (!open_result.absolute_path.empty())
		{
			save_file_path = open_result.absolute_path;
			LoadFromJson(save_file_path);
		}
	}

	if (!save_file_path.empty())
	{
		ImGui::TextWrapped(u8"現在のファイル: %s", save_file_path.c_str());
	}
	ImGui::End();
}

//コライダー名を指定してON/OFF切り替え
void ColliderAttachmentEditor::SetColliderActiveByName(const std::string& target_name, bool is_active)
{
	bool is_found = false;
	for (auto& item : collider_items)
	{
		if (item && item->name == target_name)
		{
			item->is_active = is_active;
			is_found = true;
			break;
		}
	}

	if (!is_found)
	{
		OutputDebugStringA("[ColliderAttachmentEditor 警告] SetColliderActiveByName: 指定されたコライダー名が見つかりません。\n");
	}
}

//ビューポートクリック時のレイキャスト判定
void ColliderAttachmentEditor::HandleRaycastSelection(ModelPreviewWindow* preview_window, const ImVec2& image_pos, const ImVec2& image_size)
{
	if (!preview_window || !collision_logic) return;

	Model* model = preview_window->GetModel();
	Camera* camera = preview_window->GetCamera();
	if (!model || !camera) return;

	ImGuiIO& io = ImGui::GetIO();
	if (!io.MouseClicked[0]) return; //左クリック時のみ判定

	//ビューポート画像内判定
	float mouse_x = io.MousePos.x - image_pos.x;
	float mouse_y = io.MousePos.y - image_pos.y;
	if (mouse_x < 0.0f || mouse_x > image_size.x || mouse_y < 0.0f || mouse_y > image_size.y)
	{
		return;
	}

	//正規化デバイス座標(NDC)への変換
	float ndc_x = (2.0f * mouse_x / image_size.x) - 1.0f;
	float ndc_y = 1.0f - (2.0f * mouse_y / image_size.y);

	DirectX::XMMATRIX view = DirectX::XMLoadFloat4x4(&camera->GetView());
	DirectX::XMMATRIX proj = DirectX::XMLoadFloat4x4(&camera->GetProjection());
	DirectX::XMVECTOR determinant = DirectX::XMVectorZero();
	DirectX::XMMATRIX inv_view_proj = DirectX::XMMatrixInverse(&determinant, DirectX::XMMatrixMultiply(view, proj));

	DirectX::XMVECTOR ray_near = DirectX::XMVector3TransformCoord(DirectX::XMVectorSet(ndc_x, ndc_y, 0.0f, 1.0f), inv_view_proj);
	DirectX::XMVECTOR ray_far = DirectX::XMVector3TransformCoord(DirectX::XMVectorSet(ndc_x, ndc_y, 1.0f, 1.0f), inv_view_proj);
	DirectX::XMVECTOR ray_dir = DirectX::XMVector3Normalize(DirectX::XMVectorSubtract(ray_far, ray_near));

	DirectX::XMFLOAT3 start_f3 = {};
	DirectX::XMFLOAT3 dir_f3 = {};
	DirectX::XMStoreFloat3(&start_f3, ray_near);
	DirectX::XMStoreFloat3(&dir_f3, ray_dir);

	//各ボーンノードに対するレイキャスト判定
	const auto& nodes = model->GetAnimatedNodes();
	DirectX::XMFLOAT4X4 world_f4 = preview_window->GetModelWorldMatrix();
	DirectX::XMMATRIX model_world = DirectX::XMLoadFloat4x4(&world_f4);

	//レイが当たったすべてのノードインデックスを収集する配列
	std::vector<int> hit_nodes;

	for (int i = 0; i < static_cast<int>(nodes.size()); ++i)
	{
		DirectX::XMFLOAT4X4 bone_local = {};
		if (!model->GetNodeGlobalTransform(i, bone_local)) continue;

		DirectX::XMMATRIX m_bone_world = DirectX::XMMatrixMultiply(DirectX::XMLoadFloat4x4(&bone_local), model_world);
		DirectX::XMFLOAT3 sphere_center = {};
		DirectX::XMStoreFloat3(&sphere_center, m_bone_world.r[3]);

		//CollisionLogicを用いたレイとスフィアの交差判定
		float hit_t = 0.0f;
		if (collision_logic->RaySphere(start_f3, dir_f3, sphere_center, node_radio, hit_t))
		{
			hit_nodes.push_back(i);
		}
	}

	//レイが当たったノード群の中から最も上位の階層ノードを特定
	if (!hit_nodes.empty())
	{
		int chosen_node_index = FindHighestAmongHitNodes(hit_nodes, model);
		if (chosen_node_index < 0 || chosen_node_index >= static_cast<int>(nodes.size()))
		{
			OutputDebugStringA("[ColliderAttachmentEditor エラー] 不正なノードインデックスが選択されました。\n");
			return;
		}
		std::string chosen_node_name = nodes[chosen_node_index].name;

		if (selected_item_index >= 0 && selected_item_index < static_cast<int>(collider_items.size()))
		{
			ColliderAttachmentItem* item = collider_items[selected_item_index].get();

			//2ボーン連携が有効な場合
			if (item->is_two_bone_link)
			{
				if (current_select_slot == BoneSelectSlot::Start)
				{
					item->start_bone_name = chosen_node_name;
					//終点が未設定なら自動的に終点選択へ遷移
					if (item->end_bone_name.empty())
					{
						current_select_slot = BoneSelectSlot::End;
					}
					OutputDebugStringA("[ColliderAttachmentEditor] 始点ボーンを設定しました。次は終点ボーンをクリックしてください。\n");
				}
				else
				{
					item->end_bone_name = chosen_node_name;
					// 終点設定完了後は始点選択に戻す
					current_select_slot = BoneSelectSlot::Start;
					OutputDebugStringA("[ColliderAttachmentEditor] 終点ボーンを設定しました。2ボーン連携当たり判定が設定されました。\n");
				}
			}
			else
			{
				// 単一ボーン設定
				item->start_bone_name = chosen_node_name;
				OutputDebugStringA("[ColliderAttachmentEditor] 単一ボーンをアタッチしました。\n");
			}
		}
		else
		{
			auto new_item = std::make_unique<ColliderAttachmentItem>();
			new_item->name = "Collider_" + chosen_node_name;
			new_item->start_bone_name = chosen_node_name;
			collider_items.push_back(std::move(new_item));
			selected_item_index = static_cast<int>(collider_items.size() - 1);
		}
		OutputDebugStringA("[ColliderAttachmentEditor] 最上位ノードをアタッチしました。\n");
	}
}

int ColliderAttachmentEditor::FindHighestAmongHitNodes(const std::vector<int>& hit_node_indices, Model* model)
{
	if (!model || hit_node_indices.empty())return -1;

	const auto& nodes = model->GetAnimatedNodes();
	int best_node_index = hit_node_indices[0];
	int min_depth = INT_MAX;

	for (int candidate_index : hit_node_indices)
	{
		//範囲外アクセスのチェック
		if (candidate_index < 0 || candidate_index >= static_cast<int>(nodes.size()))
		{
			OutputDebugStringA("[ColliderAttachmentEditor 警告] FindHighestAmongHitNodes: 候補ノードが範囲外です。\n");
			continue;
		}

		//親をたどってルートまでの階層の深さを算出
		int depth = 0;
		int current = candidate_index;
		constexpr int MAX_HIERARCHY_LOOP = 256;

		while (nodes[current].parent_index >= 0 && depth < MAX_HIERARCHY_LOOP)
		{
			int parent = nodes[current].parent_index;
			if (parent < 0 || parent >= static_cast<int>(nodes.size()) || parent == current)
			{
				break;
			}
			current = parent;
			depth++;
		}

		//ヒットしたノード同士の中で最も深さが浅いノードを採用
		if (depth < min_depth)
		{
			min_depth = depth;
			best_node_index = candidate_index;
		}
	}
	return best_node_index;
}

//JSON保存
void ColliderAttachmentEditor::SaveToJson(const std::string& file_path)
{
	if (file_path.empty()) return;

	// 全コライダーを格納するルート配列
	nlohmann::json root_array = nlohmann::json::array();

	for (size_t i = 0; i < collider_items.size(); ++i)
	{
		// コライダーごとに個別のシリアライザーを用意してオブジェクト化
		JsonSerializer item_serializer;
		collider_items[i]->SetupSerialization(&item_serializer);

		nlohmann::json item_json;
		item_serializer.SaveToObject(item_json);
		root_array.push_back(item_json);
	}

	std::ofstream output_file(file_path);
	if (output_file.is_open())
	{
		constexpr int json_indent_space = 4;
		output_file << root_array.dump(json_indent_space);
		output_file.close();
		OutputDebugStringA("[ColliderAttachmentEditor] 全コライダーの保存に成功しました。\n");
	}
	else
	{
		OutputDebugStringA("[ColliderAttachmentEditor エラー] SaveToJson: 保存先ファイルのオープンに失敗しました。\n");
	}
}

//JSON読み込み
void ColliderAttachmentEditor::LoadFromJson(const std::string& file_path)
{
	if (file_path.empty()) return;

	std::ifstream input_file(file_path);
	if (!input_file.is_open())
	{
		OutputDebugStringA("[ColliderAttachmentEditor エラー] LoadFromJson: ファイルのオープンに失敗しました。\n");
		return;
	}

	nlohmann::json root_array;
	input_file >> root_array;
	input_file.close();

	if (!root_array.is_array())
	{
		OutputDebugStringA("[ColliderAttachmentEditor エラー] LoadFromJson: JSONデータが配列形式ではありません。\n");
		return;
	}

	collider_items.clear();

	// 配列の各要素からコライダーを1つずつ復元
	for (const auto& item_json : root_array)
	{
		auto new_item = std::make_unique<ColliderAttachmentItem>();

		JsonSerializer item_serializer;
		new_item->SetupSerialization(&item_serializer);
		item_serializer.LoadFromObject(item_json);
		new_item->OnDeserialized();
		collider_items.push_back(std::move(new_item));
	}

	selected_item_index = collider_items.empty() ? -1 : 0;
	OutputDebugStringA("[ColliderAttachmentEditor] 全コライダーの読み込みが成功しました。\n");
}

//プレビュー用コライダー描画
void ColliderAttachmentEditor::RenderDebug(ShapeRenderer* renderer, ModelPreviewWindow* preview_window)
{
	if (!renderer || !preview_window) return;
	Model* model = preview_window->GetModel();
	if (!model) return;

	DirectX::XMFLOAT4X4 world_f4 = preview_window->GetModelWorldMatrix();
	DirectX::XMMATRIX model_world = DirectX::XMLoadFloat4x4(&world_f4);
	const auto& nodes = model->GetAnimatedNodes();

	// 各ボーンノードの当たり判定球の可視化
	if (is_draw_node_spheres)
	{
		constexpr DirectX::XMFLOAT4 sphere_color = { 0.2f, 0.9f, 0.9f, 0.6f }; // 水色半透明

		for (size_t i = 0; i < nodes.size(); ++i)
		{
			DirectX::XMFLOAT4X4 bone_local = {};
			if (!model->GetNodeGlobalTransform(static_cast<int>(i), bone_local)) continue;

			DirectX::XMMATRIX m_bone_world = DirectX::XMMatrixMultiply(DirectX::XMLoadFloat4x4(&bone_local), model_world);
			DirectX::XMFLOAT3 sphere_center = {};
			DirectX::XMStoreFloat3(&sphere_center, m_bone_world.r[3]);

			renderer->DrawSphere(sphere_center, node_radio, sphere_color, ShapeDrawMode::Wireframe);
		}
	}

	// アタッチされたカプセルコライダーの可視化
	if (is_draw_colliders)
	{
		for (size_t i = 0; i < collider_items.size(); ++i)
		{
			const auto& item = collider_items[i];
			//if (!item->is_active) continue;

			DirectX::XMFLOAT3 center = {};
			DirectX::XMFLOAT4 rotation = {};
			float total_height = 0.0f;

			if (CalculateCapsuleWorld(*item, model, world_f4, center, rotation, total_height))
			{
				DirectX::XMFLOAT4 color = { 0.2f, 0.7f, 1.0f, 1.0f }; // デフォルト: 動的衝突（青）

				if (selected_item_index == static_cast<int>(i))
				{
					color = { 1.0f, 1.0f, 0.0f, 1.0f }; // 選択中: 黄色
				}
				else
				{
					switch (item->attribute)
					{
					case ColliderAttribute::Attack:
						color = { 1.0f, 0.2f, 0.2f, 1.0f }; // 攻撃判定: 赤
						break;
					case ColliderAttribute::Stage:
						color = { 0.2f, 1.0f, 0.2f, 1.0f }; // 地形・壁: 緑
						break;
					case ColliderAttribute::Collision:
						color = { 0.2f, 0.6f, 1.0f, 1.0f }; // 動的衝突: 青
						break;
					case ColliderAttribute::None:
					default:
						color = { 0.6f, 0.6f, 0.6f, 1.0f }; // なし: 灰色
						break;
					}
				}

				renderer->DrawCapsule(center, rotation, item->radius, total_height, color, ShapeDrawMode::Wireframe);
			}
		}
	}
}

//シーケンサプレビュー用コライダー描画
void ColliderAttachmentEditor::RenderDebugForSequencer(ShapeRenderer* renderer, ModelPreviewWindow* prevew_window)
{
	if (!renderer || !prevew_window)return;
	Model* model = prevew_window->GetModel();
	if (!model)return;

	DirectX::XMFLOAT4X4 world_f4 = prevew_window->GetModelWorldMatrix();

	for (size_t i = 0; i < collider_items.size(); i++)
	{
		const auto& item = collider_items[i];
		if (!item || !item->is_active)continue;

		DirectX::XMFLOAT3 center = {};
		DirectX::XMFLOAT4 rotation = {};
		float total_height = 0.0f;

		if (CalculateCapsuleWorld(*item, model, world_f4, center, rotation, total_height))
		{
			DirectX::XMFLOAT4 color = { 0.2f,0.7f,1.0f,1.0f };

			switch (item->attribute)
			{
			case ColliderAttribute::Attack:
				color = { 1.0f,0.2f,0.2f,1.0f };
				break;
			case ColliderAttribute::Stage:
				color = { 0.2f,1.0f,0.2f,1.0f };
				break;
			case ColliderAttribute::Collision:
				color = { 0.2f,0.7f,1.0f,1.0f };
				break;
			default:
				color = { 0.6f,0.6f,0.6f,1.0f };
				break;
			}
			renderer->DrawCapsule(center, rotation, item->radius, total_height, color);
		}
	}
}

//カプセル座標の計算
bool ColliderAttachmentEditor::CalculateCapsuleWorld(
	const ColliderAttachmentItem& item,
	Model* model,
	const DirectX::XMFLOAT4X4& model_world,
	DirectX::XMFLOAT3& out_center,
	DirectX::XMFLOAT4& out_rotation,
	float& out_total_height)
{
	if (!model || item.start_bone_name.empty()) return false;
	DirectX::XMMATRIX mat_model = DirectX::XMLoadFloat4x4(&model_world);

	// 2ボーン連携モード（始点と終点の両方が指定されている場合）
	if (item.is_two_bone_link && !item.end_bone_name.empty())
	{
		DirectX::XMFLOAT4X4 start_local = {};
		DirectX::XMFLOAT4X4 end_local = {};

		if (!model->GetNodeGlobalTransform(item.start_bone_name, start_local) ||
			!model->GetNodeGlobalTransform(item.end_bone_name, end_local))
		{
			return false;
		}

		// 始点と終点のワールド座標を算出
		DirectX::XMMATRIX m_start_world = DirectX::XMMatrixMultiply(DirectX::XMLoadFloat4x4(&start_local), mat_model);
		DirectX::XMMATRIX m_end_world = DirectX::XMMatrixMultiply(DirectX::XMLoadFloat4x4(&end_local), mat_model);

		DirectX::XMVECTOR v_start = m_start_world.r[3];
		DirectX::XMVECTOR v_end = m_end_world.r[3];

		// 中心座標
		DirectX::XMVECTOR v_center = DirectX::XMVectorScale(DirectX::XMVectorAdd(v_start, v_end), 0.5f);
		DirectX::XMStoreFloat3(&out_center, v_center);

		// 線分の長さとカプセルの全体長さ
		DirectX::XMVECTOR diff = DirectX::XMVectorSubtract(v_end, v_start);
		float actual_len = DirectX::XMVectorGetX(DirectX::XMVector3Length(diff));
		out_total_height = actual_len + (item.radius * 2.0f);

		// カプセルの向き（Y軸基準から終点方向への回転クォータニオン）
		DirectX::XMVECTOR default_up = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
		DirectX::XMVECTOR dir = (actual_len > 1e-4f) ? DirectX::XMVector3Normalize(diff) : default_up;

		DirectX::XMVECTOR rot_quat = DirectX::XMQuaternionIdentity();
		DirectX::XMVECTOR rot_axis = DirectX::XMVector3Cross(default_up, dir);
		float dot = DirectX::XMVectorGetX(DirectX::XMVector3Dot(default_up, dir));

		if (dot < -0.9999f)
		{
			rot_quat = DirectX::XMQuaternionRotationAxis(DirectX::XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f), DirectX::XM_PI);
		}
		else if (dot < 0.9999f)
		{
			float angle = acosf(dot);
			rot_quat = DirectX::XMQuaternionRotationAxis(DirectX::XMVector3Normalize(rot_axis), angle);
		}
		DirectX::XMStoreFloat4(&out_rotation, rot_quat);
		return true;
	}

	// 単一ボーンモード（始点ボーン + 長さ・オフセット）
	DirectX::XMFLOAT4X4 bone_local = {};
	if (!model->GetNodeGlobalTransform(item.start_bone_name, bone_local)) return false;

	DirectX::XMMATRIX m_bone = DirectX::XMMatrixMultiply(DirectX::XMLoadFloat4x4(&bone_local), mat_model);

	//オイラー角(度数)をラジアンに変換して回転行列を作成
	DirectX::XMMATRIX mat_rot = DirectX::XMMatrixRotationRollPitchYaw(
		DirectX::XMConvertToRadians(item.rotation.x),
		DirectX::XMConvertToRadians(item.rotation.y),
		DirectX::XMConvertToRadians(item.rotation.z)
	);
	// カプセルの長さ方向（標準はY軸方向）ベクトルを回転
	DirectX::XMVECTOR local_dir = DirectX::XMVector3TransformNormal(DirectX::XMVectorSet(0.0f, item.height, 0.0f, 0.0f), mat_rot);

	DirectX::XMVECTOR local_start = DirectX::XMVectorSet(item.offset.x, item.offset.y, item.offset.z, 1.0f);
	DirectX::XMVECTOR local_end = DirectX::XMVectorAdd(local_start, local_dir);

	DirectX::XMVECTOR v_start = DirectX::XMVector3TransformCoord(local_start, m_bone);
	DirectX::XMVECTOR v_end = DirectX::XMVector3TransformCoord(local_end, m_bone);

	DirectX::XMVECTOR v_center = DirectX::XMVectorScale(DirectX::XMVectorAdd(v_start, v_end), 0.5f);
	DirectX::XMStoreFloat3(&out_center, v_center);

	DirectX::XMVECTOR diff = DirectX::XMVectorSubtract(v_end, v_start);
	float actual_len = DirectX::XMVectorGetX(DirectX::XMVector3Length(diff));
	out_total_height = actual_len + (item.radius * 2.0f);

	DirectX::XMVECTOR default_up = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	DirectX::XMVECTOR dir = (actual_len > 1e-4f) ? DirectX::XMVector3Normalize(diff) : default_up;

	DirectX::XMVECTOR rot_quat = DirectX::XMQuaternionIdentity();
	DirectX::XMVECTOR rot_axis = DirectX::XMVector3Cross(default_up, dir);
	float dot = DirectX::XMVectorGetX(DirectX::XMVector3Dot(default_up, dir));

	if (dot < -0.9999f)
	{
		rot_quat = DirectX::XMQuaternionRotationAxis(DirectX::XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f), DirectX::XM_PI);
	}
	else if (dot < 0.9999f)
	{
		float angle = acosf(dot);
		rot_quat = DirectX::XMQuaternionRotationAxis(DirectX::XMVector3Normalize(rot_axis), angle);
	}
	DirectX::XMStoreFloat4(&out_rotation, rot_quat);
	return true;
}

//モデル名に基づいて保存/読み込みファイルパスを管理
std::string ColliderAttachmentEditor::GetDefaultFilePath(ModelPreviewWindow* preview_window) const
{
	if (!preview_window)return "";

	std::string raw_model_path = preview_window->GetModelName();
	if (raw_model_path.empty())return "";

	std::filesystem::path path_obj(raw_model_path);
	std::string model_name = path_obj.stem().string();

	if (model_name.empty())return "";

	//ベースディレクトリ:Data/Json/モデル名/
	std::string dir_path = "Data/Json/" + model_name;
	std::error_code ec;

	//ディレクトリが存在しない場合は自動生成
	if (!std::filesystem::exists(dir_path, ec))
	{
		std::filesystem::create_directories(dir_path);
		if (ec)
		{
			OutputDebugStringA("[ColliderAttachmentEditor エラー] フォルダ作成に失敗しました。\n");
		}
	}

	return dir_path + "/" + model_name + "_Attach.json";
}
