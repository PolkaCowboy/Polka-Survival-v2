#pragma once

#include "config.h";
#include "../inc/sdk.h";

#include <vector>
#include <string>

// Handler for the "Cores drain per day" settings.
struct CoreDrainState { 
	int healthNextDrain = 0;
	int healthInterval = 0;

	int staminaNextDrain = 0;
	int staminaInterval = 0;
};
void CoreDrainInit(CoreDrainState& state, const ModConfig& cfg, int now);
void CoreDrainUpdate(CoreDrainState& state, Ped ped, int now, std::vector<std::string>& debug);


