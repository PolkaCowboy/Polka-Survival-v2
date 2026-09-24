#pragma once

#include "config.h"
#include "../inc/sdk.h"
#include "elasticinterval.h"

#include <vector>
#include <string>

// Handler for the "Cores drain per day" settings.
struct CoreDrainState { 
	ElasticInterval health;
	ElasticInterval stamina;
};

void CoreDrainInit(CoreDrainState& state, const ModConfig& cfg);
void CoreDrainUpdate(CoreDrainState& state, Ped player, int dtMs, std::vector<std::string>& debug);