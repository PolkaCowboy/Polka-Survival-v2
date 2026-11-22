#pragma once

#include "../inc/sdk.h"

#include <vector>
#include <string>

enum class CampPhase { Idle, Watching, WaitForBlack, Finishing, FailWaitForPlacement, FailWaitForCampGone, FailHoldBlack };

struct CampState {
	CampPhase phase = CampPhase::Idle;
	int phaseStart = 0; // Timer when current phase has started
	int previousCampCount = -1; // How many camps existsed previous frame
	int startCount = 0; 
	bool failing = false; // Outcome of current camp setup attempt
	bool kitBlocked[2] = {}; // KIT_CAMP, KIT_CAMP_SIMPLE
};

bool UpdateCamp(CampState& state, Player player, int now, std::vector<std::string>& debug);