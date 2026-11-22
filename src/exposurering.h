#pragma once

#include "ui.h"

namespace Wetness { struct State; }
namespace Hypothermia { struct State; }

// A custom HUD core ring for exposure, drawn next to the vanilla cores:
//  fill = how wet you are (water blue)
//  track = how bad hypothermia is heading under the current conditions: grows and pales from grey to frost
//  icon = rpg_cold once hypothermia starts, bluer as it gets worse
//  pulsing icon = the cold is costing you health
// It isn't part of the game's HUD, Position and size come from the config.
namespace ExposureRing
{
	// The game's compass (minimap) setting, chosen by holding Alt. No native reports it.
	enum class CompassType
	{
		Regular,
		Expanded,
		Simple, // called "Compass" in the game's menu; INI value COMPASS
		Off,
	};

	// Where the exposure ring sits: centre and size in 1920x1080 pixels, whatever the real resolution.
	struct RingPlacement
	{
		float x = 0.0f;
		float y = 0.0f;
		float size = 0.0f;
	};

	//The compass type the player starts with, and the ring's placement for each HUD layout:
	//regular (small); compass, which the Compass and Off options share; and expanded, which the
	//Expanded compass, an Alt tap and the weapon wheel all use.
	struct Settings
	{
		bool showRing = true;
		CompassType compass = CompassType::Regular;
		RingPlacement ringRegular{ 46.0f, 847.0f, 50.0f };
		RingPlacement ringCompass{ 87.0f, 986.0f, 50.0f };
		RingPlacement ringExpanded{ 47.0f, 777.0f, 65.0f };
	};

	struct State
	{
		float alpha = 0.0f; // 0-1, fades in and out
		UI::Rgba trackColor;
		bool revealActive = false;
		int revealUntil = 0; 
	};

	void Init(State& state, const Settings& settings);

	void Update(State& state, const Wetness::State& wetnessState, const Hypothermia::State& hypothermia, int dtMs, int now);
}
