#pragma once

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

	struct Settings {
		int healingThreshold = 50; //Percent of the health core that must be filled to start Hitpoint regeneration.
		float healthCoreDrainsPerDay = 2.0f;
		float staminaCoreDrainsPerDay = 2.0f;
		float timeToDieMin = 1.75f; //Minutes
		float timeToDieMax = 6.0f; //Minutes
		bool lethal = false; //True: You will die when HP ticks to zero. False: Health will stop ticking when outer ring hits 0 and core will stop ticking at 1, leaving you very squishy until you can eat
	};

	// We are basing all HP drain calculations on health core level 7 (Last one before the final two bonus ranks)
	// Each health rang grants 50hp. Rank 0 = 50hp.
	constexpr float REFERENCE_RING_HP = 400.0f;

	void Init(State& state, const Settings& settings);
	void Update(State& state, Player player, Ped playerPed, int dtMs, const Hypothermia::Effects& weatherEffects, std::vector<std::string>& debug);
	void SimulateSkip(State& state, Ped playerPed, int skippedMs, const Hypothermia::Effects& weatherEffects);
}