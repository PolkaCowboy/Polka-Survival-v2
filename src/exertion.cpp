#include "exertion.h"
#include <algorithm>

float Exertion::GetExertion(Ped playerPed) {
	float level = 0.0f;

	if (!PED::IS_PED_ON_FOOT(playerPed))
		return 0.0f;

	//No punish for walking.
	//if (TASK::IS_PED_WALKING(playerPed)) {
	//	level = std::max(level, 0.0f);
	//}
	
	if (TASK::IS_PED_RUNNING(playerPed)) {
		level = std::max(level, 0.3f);
	}
	if (TASK::IS_PED_SPRINTING(playerPed)) {
		level = std::max(level, 1.0f);
	}

	//More sweat if carrying something.
	if (PED::IS_PED_CARRYING_SOMETHING(playerPed)) {
		level = std::min(level + 0.15f, 1.0f);
	}

	//If we are dragging something with a lasso
	if (TASK::IS_PED_WALKING(playerPed) && PED::_GET_LASSO_TARGET(playerPed) != 0) {
		level = std::max(level, 0.6f);
	}

	if (PED::IS_PED_CLIMBING(playerPed)) {
		level = std::max(level, 0.7f);
	}
	if (PED::IS_PED_IN_MELEE_COMBAT(playerPed)) {
		level = std::max(level, 1.0f);
	}
	if (PED::IS_PED_JUMPING(playerPed)) {
		level = std::max(level, 0.5f);
	}
	if (PED::IS_PED_HOGTYING(playerPed)) {
		level = std::max(level, 0.3f);
	}

	return level;
}