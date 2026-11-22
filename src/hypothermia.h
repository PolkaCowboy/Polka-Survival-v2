#pragma once

#include "config.h"
#include "../inc/sdk.h"

#include <vector>
#include <string>

namespace Wetness { struct State; }

namespace Hypothermia
{
	struct Effects
	{
		double healthCoreDrainModifier = 0.0;
		double staminaCoreDrainModifier = 0.0;
		double hitPointsDrainModifier = 0.0;
	};

	struct State
	{
		Effects effects;
		float severity = 0.0f; // 0 = fine, 1 = not fine.
		float target = 0.0f; // where severity is heading under the current conditions, 0-1
		float warmthSum = 0.0f; //Warmth of currently equipped clothing. (Custom caclulation)
		int nextWarmthCheck = 0;
	};

	void Init(State& state, const ModConfig& cfg);
	void Update(State& state, Ped playerPed, const Wetness::State& wetness, float exersion, int dtMs, const ModConfig& cfg, std::vector<std::string>& debug);
}
