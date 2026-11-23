#pragma once
#include <string>

inline const std::string current_tactical_action = u8"Œ»İ‚Ìw¦s“®";
inline const std::string current_tactical_type = u8"Œ»İ‚Ìíp";

//•ºm‚Ìs“®
enum class TacticalAction
{
	None,			//–³‚µ
	Formation,		//wŒ`ˆÚ“®
	TargetPoint,	//w’èÀ•WˆÚ“®
	Automous		//ŒÂ•Ês“®
};

//wŠöŠ¯‚Ìíp
enum class TacticalType
{
	None,		//–³‚µ
	Retreat,	//‘S‘Ì‘Ş”ğ
	Autonomous,	//ŒÂ•Ês“®
	Circle,		//‰~w
	FishScale,	//‹›—Ø‚Ìw
	CraneWing,	//’ß—ƒ‚Ìw
	VShaped,	//Vš‚Ìw
	Rectangle,	//’·•ûŒ`‚Ìw
};