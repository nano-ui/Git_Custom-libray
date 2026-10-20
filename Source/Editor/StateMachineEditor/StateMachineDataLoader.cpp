#include "StateMachineDataLoader.h"
#include "StateGraphDataManager.h"
#include <Gameplay\StateMachine\StateBlackboard.h>

#include <fstream>
#include <cstdio>

bool StateMachineDataLoader::LoadForRuntime(
    const std::string& file_path,
    std::vector<SimulatorRuntimeNode>& out_nodes,
    std::vector<SimulatorRuntimeLink>& out_links,
    std::unordered_map<uint32_t, uint32_t>& out_layer_entry_nodes,
    std::unordered_map<uint32_t, std::string>& out_animation_map,
    std::unordered_map<uint32_t, RuntimeLayerType>& out_layer_types)
{
    out_nodes.clear();
    out_links.clear();
    out_layer_entry_nodes.clear();
    out_animation_map.clear();
    out_layer_types.clear();

    std::ifstream input_file(file_path);
    if (!input_file.is_open())
    {
        printf("エラー: StateMachineDataLoader::LoadForRuntime - 指定されたファイルが開けませんでした。パス: %s\n", file_path.c_str());
        return false;
    }

    nlohmann::json root_json;
    input_file >> root_json;
    input_file.close();

    if (!root_json.is_object() || !root_json.contains("Layers") || !root_json["Layers"].is_array())
    {
        printf("エラー: StateMachineDataLoader::LoadForRuntime - JSONのフォーマットが不正です。Layers配列が存在しません。\n");
        return false;
    }

    std::unordered_map<uint32_t, uint32_t> sub_graph_to_parent_map;

    for (size_t i = 0; i < root_json["Layers"].size(); i++)
    {
        const auto& layer = root_json["Layers"][i];
        if (layer.contains("Nodes") && layer["Nodes"].is_array())
        {
            for (size_t j = 0; j < layer["Nodes"].size(); j++)
            {
                const auto& node_json = layer["Nodes"][j];
                if (node_json.is_object() && node_json.contains("IsSubGraph") && node_json["IsSubGraph"].get<bool>())
                {
                    sub_graph_to_parent_map[node_json["SubGraphID"].get<uint32_t>()] = node_json["ID"].get<uint32_t>();
                }
            }
        }
    }

    for (size_t i = 0; i < root_json["Layers"].size(); i++)
    {
        const auto& layer = root_json["Layers"][i];
        uint32_t layer_graph_id = layer["GraphID"].get<uint32_t>();
        uint32_t parent_id = UINT32_MAX;

        RuntimeLayerType layer_type = RuntimeLayerType::StateMachine;
        if (layer.contains("LayerType"))
        {
            layer_type = static_cast<RuntimeLayerType>(layer["LayerType"].get<int>());
        }
        out_layer_types[layer_graph_id] = layer_type;

        auto parent_it = sub_graph_to_parent_map.find(layer_graph_id);
        if (parent_it != sub_graph_to_parent_map.end())
        {
            parent_id = parent_it->second;
        }

        if (layer.contains("Nodes") && layer["Nodes"].is_array())
        {
            for (size_t j = 0; j < layer["Nodes"].size(); j++)
            {
                const auto& node_json = layer["Nodes"][j];
                if (node_json.is_object())
                {
                    SimulatorRuntimeNode node;
                    ParseRuntimeNode(node_json, parent_id, layer_graph_id, node);

                    if (j == 0)
                    {
                        out_layer_entry_nodes[layer_graph_id] = node.id;
                    }

                    if (!node.animation_name.empty())
                    {
                        out_animation_map[node.id] = node.animation_name;
                    }

                    out_nodes.push_back(node);
                }
            }
        }

        if (layer.contains("Links") && layer["Links"].is_array())
        {
            for (size_t j = 0; j < layer["Links"].size(); j++)
            {
                const auto& link_json = layer["Links"][j];
                SimulatorRuntimeLink link;
                ParseRuntimeLink(link_json, link);
                out_links.push_back(link);
            }
        }
    }

    return true;
}

void StateMachineDataLoader::ParseRuntimeNode(
    const nlohmann::json& node_json,
    uint32_t parent_id,
    uint32_t graph_id,
    SimulatorRuntimeNode& out_node)
{
    out_node.id = node_json["ID"].get<uint32_t>();
    out_node.name = node_json["Name"].get<std::string>();
    out_node.animation_name = node_json.contains("AnimationName") ? node_json["AnimationName"].get<std::string>() : "";
    out_node.is_sub_graph = node_json["IsSubGraph"].get<bool>();
    out_node.sub_graph_id = node_json["SubGraphID"].get<uint32_t>();
    out_node.is_loop = node_json.contains("IsLoop") ? node_json["IsLoop"].get<bool>() : true;
    out_node.is_root_motion = node_json.contains("IsRootMotion") ? node_json["IsRootMotion"].get<bool>() : false;
    out_node.parent_node_id = parent_id;
    out_node.graph_id = graph_id;

    if (node_json.contains("BehaviorData") && node_json["BehaviorData"].is_object())
    {
        const auto& bt_json = node_json["BehaviorData"];
        if (bt_json.contains("Category"))
        {
            out_node.behavior_data.category = static_cast<BehaviorCategory>(bt_json["Category"].get<int>());
        }
        if (bt_json.contains("CompositeType"))
        {
            out_node.behavior_data.composite_node_type = static_cast<CompositeNodeType>(bt_json["CompositeType"].get<int>());
        }
        if (bt_json.contains("FinishResult"))
        {
            out_node.behavior_data.result_type = static_cast<ResultType>(bt_json["FinishResult"].get<int>());
        }
        if (bt_json.contains("Weight"))
        {
            out_node.behavior_data.weight = bt_json["Weight"].get<int>();
        }
        if (bt_json.contains("ActionDuration"))
        {
            out_node.behavior_data.action_duration = bt_json["ActionDuration"].get<float>();
        }
        if (bt_json.contains("CooldownTime"))
        {
            out_node.behavior_data.cooldown_time = bt_json["CooldownTime"].get<float>();
        }
        if (bt_json.contains("CooldownRandomRange"))
        {
            out_node.behavior_data.cooldown_random_range = bt_json["CooldownRandomRange"].get<float>();
        }
    }

    if (node_json.contains("Input") && node_json["Input"].is_array())
    {
        for (size_t p = 0; p < node_json["Input"].size(); p++)
        {
            out_node.inputs.push_back(node_json["Input"][p]["ID"].get<uint32_t>());
        }
    }

    if (node_json.contains("Output") && node_json["Output"].is_array())
    {
        for (size_t p = 0; p < node_json["Output"].size(); p++)
        {
            out_node.outputs.push_back(node_json["Output"][p]["ID"].get<uint32_t>());
        }
    }
}

void StateMachineDataLoader::ParseRuntimeLink(
    const nlohmann::json& link_json,
    SimulatorRuntimeLink& out_link)
{
    out_link.id = link_json["ID"].get<uint32_t>();
    out_link.start_pin_id = link_json["StartPinID"].get<uint32_t>();
    out_link.end_pin_id = link_json["EndPinID"].get<uint32_t>();

    if (link_json.contains("BlendTime") && link_json["BlendTime"].is_number())
    {
        out_link.blend_time = link_json["BlendTime"].get<float>();
        constexpr float min_blend_time = 0.0f;
        if (out_link.blend_time < min_blend_time)
        {
            printf("警告: StateMachineDataLoader::ParseRuntimeLink - リンクID %u のブレンド時間が負の値(%.2f)のため、0.0fに補正しました。\n", out_link.id, out_link.blend_time);
            out_link.blend_time = min_blend_time;
        }
    }
    else
    {
        out_link.blend_time = DEFAULT_RUNTIME_BLEND_TIME;
    }

    if (link_json.contains("Conditions") && link_json["Conditions"].is_array())
    {
        constexpr size_t required_vector_elements = 3;
        for (size_t c = 0; c < link_json["Conditions"].size(); c++)
        {
            const auto& cond_json = link_json["Conditions"][c];
            TransitionCondition cond;

            cond.type = static_cast<ConditionNodeType>(cond_json["Type"].get<int>());
            cond.hash_key = cond_json["HashKey"].get<uint32_t>();
            cond.compart_op = static_cast<CompareOperator>(cond_json["CompOp"].get<int>());
            cond.param_second = cond_json["ParamSecond"].get<float>();
            cond.secondary_hash = cond_json["SecondaryHash"].get<uint32_t>();

            float raw_ref_value = cond_json["RefValue"].get<float>();
            cond.reference_value = raw_ref_value;

            if (cond_json.contains("VectorRefValue") && cond_json["VectorRefValue"].is_array() && cond_json["VectorRefValue"].size() == required_vector_elements)
            {
                DirectX::XMFLOAT3 vec_ref = { 0.0f, 0.0f, 0.0f };
                vec_ref.x = cond_json["VectorRefValue"][0].get<float>();
                vec_ref.y = cond_json["VectorRefValue"][1].get<float>();
                vec_ref.z = cond_json["VectorRefValue"][2].get<float>();
                cond.vector_reference_value = vec_ref;
            }

            out_link.conditions.push_back(cond);
        }
    }
}