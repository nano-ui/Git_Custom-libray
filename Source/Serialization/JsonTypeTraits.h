#pragma once

#include "ThiedParty\json.hpp"

namespace nlohmann
{
	template <typename EnumType, typename std::enable_if<std::is_enum<EnumType>::value, int>::type = 0>
	inline void to_json(json& json_data, const EnumType& enum_type)
	{
		using UnderlyingTupe = typename std::underlying_type<EnumType>::type;
		json_data = static_cast<UnderlyingTupe>(enum_type);
	}

	template <typename EnumType, typename std::enable_if<std::is_enum<EnumType>::value, int>::type = 0>
	inline void from_json(const json& json_data, EnumType& enmu_type)
	{
		using UnderlyingTupe = typename std::underlying_type<EnumType>::type;
		const UnderlyingTupe numeric_value = json_data.get<UnderlyingTupe>();
		enmu_type = static_cast<EnumType>(numeric_value);
	}
}