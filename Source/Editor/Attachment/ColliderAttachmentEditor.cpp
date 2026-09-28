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
			ImGui::EndListBox();
		}
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
		ImGui::Checkbox(u8"2ボーン連携", &item->is_two_bone_link);

		if (item->is_two_bone_link)
		{
			char end_bone_buf[128] = {};
			strncat_s(end_bone_buf, item->end_bone_name.c_str(), sizeof(end_bone_buf) - 1);
			if (ImGui::InputText(u8"終点ボーン名", end_bone_buf, sizeof(end_bone_buf)))
			{
				item->end_bone_name = end_bone_buf;
			}
		}

		ImGui::DragFloat(u8"カプセル半径", &item->radius, 0.01f, 0.01f, 10.0f);
		if(!item->is_two_bone_link)
		{
			ImGui::DragFloat(u8"カプセルの長さ", &item->height, 0.01f, 0.0f, 20.0f);
			ImGui::DragFloat3(u8"ローカルオフセット", &item->offset.x, 0.01f, -10.0f, 10.0f);
		}
	}

	ImGui::Separator();
	ImGui::Spacing();

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

	float closest_distance = FLT_MAX;
	int hit_node_index = -1;
	constexpr float hit_sphere_radius = 0.25f; //ノード判定用スフィア半径

	for (int i = 0; i < static_cast<int>(nodes.size()); ++i)
	{
		DirectX::XMFLOAT4X4 bone_local = {};
		if (!model->GetNodeGlobalTransform(i, bone_local)) continue;

		DirectX::XMMATRIX m_bone_world = DirectX::XMMatrixMultiply(DirectX::XMLoadFloat4x4(&bone_local), model_world);
		DirectX::XMFLOAT3 sphere_center = {};
		DirectX::XMStoreFloat3(&sphere_center, m_bone_world.r[3]);

		//CollisionLogicを用いたレイとスフィアの交差判定
		float hit_t = 0.0f;
		if (collision_logic->RaySphere(start_f3, dir_f3, sphere_center, hit_sphere_radius, hit_t))
		{
			if (hit_t < closest_distance)
			{
				closest_distance = hit_t;
				hit_node_index = i;
			}
		}
	}

	//当たったノードの最上位階層ノードを検索して適用
	if (hit_node_index >= 0)
	{
		int top_node_index = FindTopHierarchyNodeIndex(hit_node_index, model);
		std::string chosen_node_name = nodes[top_node_index].name;

		if (selected_item_index >= 0 && selected_item_index < static_cast<int>(collider_items.size()))
		{
			collider_items[selected_item_index]->start_bone_name = chosen_node_name;
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

//ヒットしたノードから親階層（ルート方向）の最も上のノードを探す
int ColliderAttachmentEditor::FindTopHierarchyNodeIndex(int hit_node_index, Model* model)
{
	if (!model) return -1;
	const auto& nodes = model->GetAnimatedNodes();
	if (hit_node_index < 0 || hit_node_index >= static_cast<int>(nodes.size())) return -1;

	int current_index = hit_node_index;
	//parent_index が 0 以上の間、親を遡る
	while (nodes[current_index].parent_index >= 0)
	{
		int parent = nodes[current_index].parent_index;
		if (parent >= static_cast<int>(nodes.size()) || parent == current_index)
		{
			break;
		}
		current_index = parent;
	}
	return current_index;
}

//JSON保存
void ColliderAttachmentEditor::SaveToJson(const std::string& file_path)
{
	if (file_path.empty()) return;

	JsonSerializer serializer;
	int item_count = static_cast<int>(collider_items.size());
	serializer.RegisterVariable(u8"登録数", &item_count);

	for (size_t i = 0; i < collider_items.size(); ++i)
	{
		collider_items[i]->SetupSerialization(&serializer);
	}

	serializer.SaveToFile(file_path);
	OutputDebugStringA("[ColliderAttachmentEditor] JsonSerializer による保存が成功しました。\n");
}

//JSON読み込み
void ColliderAttachmentEditor::LoadFromJson(const std::string& file_path)
{
	if (file_path.empty()) return;

	JsonSerializer serializer;
	int item_count = 0;
	serializer.RegisterVariable(u8"登録数", &item_count);

	if (!serializer.LoadFromFile(file_path))
	{
		OutputDebugStringA("[ColliderAttachmentEditor エラー] LoadFromJson: ファイルの読み込みに失敗しました。\n");
		return;
	}

	collider_items.clear();
	serializer.Clear();
	serializer.RegisterVariable(u8"登録数", &item_count);

	for (int i = 0; i < item_count; ++i)
	{
		auto new_item = std::make_unique<ColliderAttachmentItem>();
		new_item->SetupSerialization(&serializer);
		collider_items.push_back(std::move(new_item));
	}

	serializer.LoadFromFile(file_path);
	selected_item_index = collider_items.empty() ? -1 : 0;
	OutputDebugStringA("[ColliderAttachmentEditor] JsonSerializer による読み込みが成功しました。\n");
}

//プレビュー用コライダー描画
void ColliderAttachmentEditor::RenderDebug(ShapeRenderer* renderer, ModelPreviewWindow* preview_window)
{
	if (!renderer || !preview_window) return;
	Model* model = preview_window->GetModel();
	if (!model) return;

	DirectX::XMFLOAT4X4 model_world = preview_window->GetModelWorldMatrix();

	for (size_t i = 0; i < collider_items.size(); ++i)
	{
		const auto& item = collider_items[i];
		if (!item->is_active) continue;

		DirectX::XMFLOAT3 center = {};
		DirectX::XMFLOAT4 rotation = {};
		float total_height = 0.0f;

		if (CalculateCapsuleWorld(*item, model, model_world, center, rotation, total_height))
		{
			DirectX::XMFLOAT4 color = (selected_item_index == static_cast<int>(i))
				? DirectX::XMFLOAT4(1.0f, 1.0f, 0.0f, 1.0f)
				: DirectX::XMFLOAT4(1.0f, 0.2f, 0.2f, 1.0f);

			renderer->DrawCapsule(center, rotation, item->radius, total_height, color, ShapeDrawMode::Wireframe);
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

	DirectX::XMFLOAT4X4 bone_local = {};
	if (!model->GetNodeGlobalTransform(item.start_bone_name, bone_local)) return false;

	DirectX::XMMATRIX m_bone = DirectX::XMMatrixMultiply(DirectX::XMLoadFloat4x4(&bone_local), mat_model);

	DirectX::XMVECTOR local_start = DirectX::XMVectorSet(item.offset.x, item.offset.y, item.offset.z, 1.0f);
	DirectX::XMVECTOR local_end = DirectX::XMVectorSet(item.offset.x, item.offset.y + item.height, item.offset.z, 1.0f);

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