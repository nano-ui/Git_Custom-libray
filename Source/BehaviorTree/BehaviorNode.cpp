#include "BehaviorNode.h"

//================================
//©g‚ªÀs‰Â”\‚©‚Ç‚¤‚©‚ğ”»’è
//================================
bool BehaviorNode::CanExecute() const
{
	if (condition)
	{
		return condition();
	}
	return true;
}
