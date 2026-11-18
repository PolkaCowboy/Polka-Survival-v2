#pragma once

#include "config.h"
#include "../inc/sdk.h"

#include <vector>
#include <string>

struct WetnessState;

struct HypothermiaEffects
{
	double healthCoreDrainModifier = 0.0;
	double staminaCoreDrainModifier = 0.0;
	double hitPointsDrainModifier = 0.0;
};

struct HypothermiaState
{
	HypothermiaEffects effects;
	float severity = 0.0f; // 0 = fine, 1 = not fine.
	float warmthSum = 0.0f; //Warmth of currently equipped clothing. (Custom caclulation)
	int nextWarmthCheck = 0;
};

void HypothermiaInit(HypothermiaState& state, const ModConfig& cfg);
void HypothermiaUpdate(HypothermiaState& state, Ped playerPed, const WetnessState& wetness, int dtMs, const ModConfig& cfg, std::vector<std::string>& debug);
