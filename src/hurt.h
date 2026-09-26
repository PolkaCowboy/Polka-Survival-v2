#pragma once

#include "config.h"
#include "../inc/sdk.h"
#include "elasticinterval.h"

#include <vector>
#include <string>

// All the timers for when to drain a core or hp next
struct HurtState { 
	ElasticInterval healthCore;
	ElasticInterval staminaCore;
	ElasticInterval hitPoints;
};

void HurtInit(HurtState& state, const ModConfig& cfg);
void HurtUpdate(HurtState& state, Player player, Ped playerPed, int dtMs, const ModConfig& cfg, std::vector<std::string>& debug);