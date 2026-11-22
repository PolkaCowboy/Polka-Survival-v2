#pragma once

#include "../inc/sdk.h"

#include <vector>
#include <string>

namespace Movement {

	struct State {
		bool capped = false; //If we capped the speed last frame.
	};

	void SetCap(Ped playerPed, State& state, std::vector<std::string>& debug);
}