// StateMachineDataLoader.h
#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include "Editor/StateMachineEditor/StateGraphSimulator.h"
#include "ThiedParty/json.hpp"


class StateMachineDataLoader
{
public:
    static bool LoadForRuntime(
        const std::string& file_path,
        std::vector<SimulatorRuntimeNode>& out_nodes,
        std::vector<SimulatorRuntimeLink>& out_links,
        std::unordered_map<uint32_t, uint32_t>& out_layer_entry_nodes,
        std::unordered_map<uint32_t, std::string>& out_animation_map,
        std::unordered_map<uint32_t, RuntimeLayerType>& out_layer_types);

private:
    static void ParseRuntimeNode(
        const nlohmann::json& node_json,
        uint32_t parent_id,
        uint32_t graph_id,
        SimulatorRuntimeNode& out_node);

    static void ParseRuntimeLink(
        const nlohmann::json& link_json,
        SimulatorRuntimeLink& out_link);
};