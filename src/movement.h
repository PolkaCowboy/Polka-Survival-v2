#pragma once

#include "../inc/sdk.h"

#include <vector>
#include <string>

struct MovementState {
	bool capped = false; //If we capped the speed last frame.
};

void SetMovementCap(Ped playerPed, MovementState& state, std::vector<std::string>& debug);