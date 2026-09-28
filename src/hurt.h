#pragma once

#include "config.h"
#include "../inc/sdk.h"
#include "elasticinterval.h"

#include <vector>
#include <string>

struct HypothermiaEffects;

namespace Hurt {

	// All the timers for when to drain a core or hp next
	struct State {
		ElasticInterval healthCore;
		ElasticInterval staminaCore;
		ElasticInterval hitPoints;
	};

	void Init(State& state, const ModConfig& cfg);
	void Update(State& state, Player player, Ped playerPed, int dtMs, const HypothermiaEffects& weatherEffects, const ModConfig& cfg, std::vector<std::string>& debug);
}