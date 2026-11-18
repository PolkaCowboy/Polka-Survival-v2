#pragma once

#include "config.h"
#include "ui.h"

struct WetnessState;

// A custom HUD core ring for exposure, drawn next to the vanilla cores:
//  fill = how wet you are (water blue)
//  icon = rpg_cold once hypothermia starts, bluer as it gets worse
//  pulsing icon = the cold is costing you health
// It isn't part of the game's HUD, Position and size come from the config.
namespace ExposureRing
{
	struct State
	{
		float alpha = 0.0f; // 0-1, fades in and out
		UI::Rgba trackColor;
		bool revealActive = false;
		int revealUntil = 0; 
	};

	void Init(State& state);

	void Update(State& state, const WetnessState& wetnessState, float severity, bool losingHp, int dtMs, int now, const ModConfig& cfg);
}
