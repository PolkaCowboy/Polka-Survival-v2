#pragma once

#include "../inc/sdk.h"

#include <vector>
#include <string>

namespace Movement {

	struct State {
		bool capped = false; //If we capped the speed last frame.
	};

	struct Settings {
		bool slowWhenWeak = true; //Low cores and a wounded health ring cap movement speed
	};

	// Lower cores = lower eneregy = lower movement speed.
	// Blend health and stamina core to determing energy. 0-200 points

	// 32 + full health = Jog cap.
	// 0 + full health = Powerwalk cap.
	// 0 + 1 health = We're walking.

	// No penalty above this point
	constexpr float SUM_FULL_SPEED = 100.0f;

	constexpr float SUM_JOG = 32.f;

	// Movement speed caps

	constexpr float NO_CAP = 3.0f;
	constexpr float JOG_CAP = 1.75f;
	constexpr float POWER_WALK_CAP = 1.5f;

	// Slowed by being injured.
	constexpr float MISSING_HEALTH_PENALTY = 0.5f;

	constexpr int GOLD_RING_BONUS = 25;

	void Init(const Settings& settings);
	void SetCap(Ped playerPed, State& state, std::vector<std::string>& debug);
}