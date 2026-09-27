#pragma once

#include "config.h"
#include "../inc/sdk.h"

#include <vector>
#include <string>

struct HypothermiaEffects
{
	double healthCoreDrainModifier = 0.0; // added to the health core drain rate; 0.3 = 30% faster, negative = slower
	double staminaCoreDrainModifier = 0.0;
	double hitPointsDrainModifier = 0.0;
};

// Works out how cold the player is and what it costs them. Inputs come in from the other
// systems (wetness now; temperature, clothing and heat sources to come); the result is
// `effects`, which HurtUpdate adds to its drain rates.
struct HypothermiaState
{
	HypothermiaEffects effects;
};

void HypothermiaInit(HypothermiaState& state, const ModConfig& cfg);
void HypothermiaUpdate(HypothermiaState& state, Ped playerPed, float wetness, int dtMs, const ModConfig& cfg, std::vector<std::string>& debug);
