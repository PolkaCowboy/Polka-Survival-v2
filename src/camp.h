#pragma once

#include "../inc/sdk.h"

#include <vector>
#include <string>

namespace Camp {

	enum class Phase { Idle, Watching, WaitForBlack, Finishing, FailWaitForPlacement, FailWaitForCampGone, FailHoldBlack };

	struct State {
		Phase phase = Phase::Idle;
		int phaseStart = 0; // Timer when current phase has started
		int previousCampCount = -1; // How many camps existsed previous frame
		int startCount = 0;
		int setupMinutes = 0; // In game minutes for setup attempt
		const char* failReason = nullptr;
		int skippedMs = 0;
		bool kitBlocked[2] = {}; // KIT_CAMP, KIT_CAMP_SIMPLE
	};

	bool Update(State& state, Player player, Ped playerPed, float hypothermiaSeverity, int now, std::vector<std::string>& debug);
}