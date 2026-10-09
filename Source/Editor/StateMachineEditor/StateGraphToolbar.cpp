#include "StateGraphToolbar.h"
#include "StateGraphNavigator.h"
#include "StateGraphConfigManager.h"
#include "Editor\AssetLoader.h"
#include "Editor\FileDialogHelper.h"
#include "Editor\EditorMediator.h"
#include "Editor\PathHelper.h"
#include "Gameplay\StateMachine\StateBlackboard.h"
#include "Gameplay\Components\Editor\StateMachineComponent.h"

#include <imgui.h>
#include <cstdio>
#include <filesystem>

//ツールバー描画
bool StateGraphToolbar::DrawToolbar(ToolbarContext& context)
{
	//---------------------------
	//ポインタの健全性チェック
	//---------------------------
	if (!context.data_manager || !context.asset_loader || !context.navigator)
	{
		printf("Error: StateGraphToolbar::DrawToolbar - 必要なマネージャーポインタが nullptr です。\n");
		return true;
	}

	ImGui::Begin(u8"ステートマシンエディタ");

	//------------------------
	//「保存」ボタンの描画
	//------------------------
	constexpr ImVec4 save_btn_color = ImVec4(0.2f, 0.5f, 0.2f, 1.0f);	//保存ボタンの色
	ImGui::PushStyleColor(ImGuiCol_Button, save_btn_color);

	constexpr float upper_btn_width = 200.0f;	//上部ボタンの横幅
	constexpr float upper_btn_height = 25.0f;	//上部ボタンの縦幅

	//保存ボタンが押されたか判定
	if (ImGui::Button(u8"保存", ImVec2(upper_btn_width, upper_btn_height)))
	{
		ExecuteSave(context);
	}
	ImGui::PopStyleColor();

	ImGui::SameLine();

	//--------------------------------
	//「モデル読み込み」ボタンの描画
	//--------------------------------
	constexpr ImVec4 load_btn_color = ImVec4(0.2f, 0.4f, 0.6f, 1.0f);	//読み込みボタンの色
	ImGui::PushStyleColor(ImGuiCol_Button, load_btn_color);

	bool is_model_loaded = false;	//モデル読み込みフラグ
	if (ImGui::Button(u8"モデル読み込み", ImVec2(upper_btn_width, upper_btn_height)))
	{
		is_model_loaded = ExecuteLoadModel(context);
	}
	ImGui::PopStyleColor();
	ImGui::SameLine();

	//紐付け中モデルパス表示
	if (!context.asset_loader->GetLoadedModelPath().empty())
	{
		ImGui::SameLine();
		ImGui::Text(u8"紐付けモデル: %s", context.asset_loader->GetLoadedModelPath().c_str());
	}

	ImGui::SameLine();

	//--------------------------------
	//各種設定チェックボックスの描画
	//--------------------------------
	//追尾モード切り替えチェックボックス
	if (ImGui::Checkbox(u8"追尾", &context.is_tracking_active_node))
	{
		printf("StateGraphToolbar: 追尾モードが %s に切り替わりました。\n", context.is_tracking_active_node ? "ON" : "OFF");
	}

	ImGui::SameLine();

	//シミュレーション実行切り替えチェックボックス
	if (ImGui::Checkbox(u8"シミュレーション", &context.is_simulation_active))
	{
		context.last_synced_node_id = UINT_MAX;

		//シミュレーション開始時にグラフデータを保存しコンポーネントを初期化
		if (context.is_simulation_active && context.state_machine_component)
		{
			//現在のファイルパスが有効か判定
			if (!context.current_loaded_file_path.empty())
			{
				context.data_manager->SaveToFile(context.current_loaded_file_path);
				context.state_machine_component->SetStateMachinePath(context.current_loaded_file_path);
			}
			context.state_machine_component->RequestReload();
		}
	}

	ImGui::SameLine();

	//階層切り替えコンボボックス表示
	GraphData* current_graph = nullptr;	//現在の階層情報
	auto& layers = context.data_manager->GetLayerDatas();	//全ての階層リスト
	for (size_t g_idx = 0; g_idx < layers.size(); g_idx++)	//全ての階層を巡回
	{
		if (layers[g_idx].id == context.current_graph_id)	//現在の階層と一致するか判定
		{
			current_graph = &layers[g_idx];
			break;	//これ以上巡回しても意味がないので抜ける
		}
	}


	if (current_graph)	//現在の階層が見つかったか
	{
		const char* layer_type_names[] = {	//表示する名前
			u8"ステートマシン",
			u8"ビヘイビアツリー"
		};

		constexpr int total_layer_type_count = 2;	//表示する数	
		int current_type_index = static_cast<int>(current_graph->layer_type);	//現在のレイヤー属性

		//実際にコンボボックス表示
		ImGui::SetNextItemWidth(180.0f);
		if (ImGui::Combo(u8"##LayerTypeCombo", &current_type_index, layer_type_names, total_layer_type_count))
		{
			if (current_type_index >= 0 && current_type_index < total_layer_type_count)
			{
				current_graph->layer_type = static_cast<LayerType>(current_type_index);
				printf("StateGraphToolbar: 階層ID %u の属性を「%s」に変更しました。\n",
					current_graph->id, layer_type_names[current_type_index]);
				ExecuteSave(context);
			}
			else
			{
				printf("Error: StateGraphToolbar - 不正な階層タイプインデックスが選択されました: %d\n", current_type_index);
			}
		}
	}

	ImGui::Spacing();

	//-------------------------------------
	//パンくず階層ナビゲーションの描画
	//-------------------------------------
	//階層移動が発生したか判定
	if (context.navigator->DrawHeaderNavigation(context.data_manager, context.current_graph_id))
	{
		ImGui::End();
		return true;
	}

	return false;
}

//グラフデータの保存処理
void StateGraphToolbar::ExecuteSave(ToolbarContext& context)
{
	std::string save_target_path = context.current_loaded_file_path;	//現在のファイルパス

	//保存先パスが空でモデルパスが存在する場合、ファイル名を自動生成
	if (save_target_path.empty() && !context.data_manager->GetTargetModelPath().empty())
	{
		std::filesystem::path path_obj(context.data_manager->GetTargetModelPath()); // ファイルパスオブジェクト
		const std::string model_name = path_obj.stem().string();	//拡張子無しのモデル名
		const std::string suffix_name = "_StateMachine";			//接尾辞

		save_target_path = PathHelper::GenerateJsonFilePath(model_name, suffix_name);
	}

	//パスが決まらない場合はファイルダイアログを表示
	if (save_target_path.empty())
	{
		save_target_path = FileDialogHelper::SaveFileDialog();
	}

	//有効なパスが確定した場合に保存処理を実行
	if (!save_target_path.empty())
	{
		context.data_manager->SaveToFile(save_target_path);
		context.current_loaded_file_path = save_target_path;

		if (context.config_manager)
		{
			context.config_manager->SaveEditorConfig(context.current_loaded_file_path);
		}

		printf("StateGraphToolbar: ファイル「%s」へ保存を完了しました。\n", context.current_loaded_file_path.c_str());
	}
	else
	{
		printf("Error: StateGraphToolbar - 保存先のパスが指定されなかったため保存をキャンセルしました。\n");
	}
}

//モデルファイルの読み込み・関連付け処理
bool StateGraphToolbar::ExecuteLoadModel(ToolbarContext& context)
{
	PathResult path_result = FileDialogHelper::OpenGenericFileDialog();
	if (path_result.absolute_path.empty())
	{
		return false;
	}

	//モデルからアニメーション読み込み
	if (!context.asset_loader->LoadModelAnimations(path_result.relative_path))
	{
		printf("Error: StateGraphToolbar - モデル「%s」のアニメーション読み込みに失敗しました。\n", path_result.relative_path.c_str());
		return false;
	}

	context.data_manager->SetTargetModelPath(path_result.relative_path);

	std::filesystem::path path_obj(path_result.relative_path);	//パスオブジェクト
	const std::string model_name = path_obj.stem().string();	//拡張子を排除したモデル名

	const std::string suffix_name = "_StateMachine";	//接尾辞
	const std::string auto_json_path = PathHelper::GenerateJsonFilePath(model_name, suffix_name);	//パス名

	//生成されたパスが有効か検証
	if (!auto_json_path.empty())
	{
		context.current_loaded_file_path = auto_json_path;
		context.data_manager->LoadFromFile(context.current_loaded_file_path);
		printf("StateGraphToolbar: モデル「%s」のJsonパス「%s」を自動構築して読み込みました。\n",
			model_name.c_str(), context.current_loaded_file_path.c_str());
	}
	else
	{
		printf("Error: StateGraphToolbar - PathHelperでのパス生成に失敗しました。\n");
	}

	context.target_model_hash = StateBlackboard::CalculateHash(model_name);
	EditorMediator::Instance().OnModelDubleClied(path_result.relative_path);
	printf("StateGraphToolbar: モデル読み込み完了: %s\n", path_result.relative_path.c_str());

	return true;
}
