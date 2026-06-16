#include "StateTransition.h"

//ğŒ‚ª–‚½‚³‚ê‚Ä‚¢‚é‚©”»’è
bool StateTransition::CanTransition(const StateBlackboard& blackboard) const
{
    //“o˜^‚³‚ê‚Ä‚¢‚é‚·‚×‚Ä‚Ì‘JˆÚğŒ‚ğ‡”Ô‚ÉŠm”F
    for (const auto& condition : conditions)
    {
        if (!condition.IsJudgment(blackboard))
        {
            return false;
        }
    }

    return true;
}
