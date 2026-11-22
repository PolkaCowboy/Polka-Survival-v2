#pragma once

#include "config.h"
#include "../inc/sdk.h"
#include "elasticinterval.h"

#include <vector>
#include <string>

namespace Hypothermia { struct Effects; }

namespace Hurt {

	// All the timers for when to drain a core or hp next
	struct State {
		ElasticInterval healthCore;
		ElasticInterval staminaCore;
		ElasticInterval hitPoints;
		int expectedHealthCore = -1;
		float healthFortifySeconds = -1.0f;
		float staminaFortifySeconds = -1.0f;
	};

	void Init(State& state, const ModConfig& cfg);
	void Update(State& state, Player player, Ped playerPed, int dtMs, const Hypothermia::Effects& weatherEffects, const ModConfig& cfg, std::vector<std::string>& debug);
	void SimulateSkip(State& state, Ped playerPed, int skippedMs, const Hypothermia::Effects& weatherEffects, const ModConfig& cfg);
}