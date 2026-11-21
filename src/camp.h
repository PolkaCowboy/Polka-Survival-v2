#pragma once

#include "../inc/sdk.h"

#include <vector>
#include <string>

enum class CampPhase { Idle, Watching, SettingUp };

struct CampState {
	CampPhase phase = CampPhase::Idle;
	int phaseStart = 0; // Timer when current phase has started
	int previousCampCount = -1; // How many camps existsed previous frame
};

bool UpdateCamp(CampState& state, Player player, int now, std::vector<std::string>& debug);