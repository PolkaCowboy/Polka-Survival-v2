#include "coredrain.h"
#include "utils.h"

#include <cmath>
#include <format>
#include <algorithm>

namespace
{
	//In game day ms
	// 8am-9am: 156752
	// 2pm-3pm: 154965
	// 11pm-12am: 64968
	// 3am-4am: 88320
	// One in-game day: 48 real world minutes;
	constexpr int MS_PER_DAY = 48 * 60 * 1000;

	//Reducing the core HP by 1 sometimes got weird and would heal instead of hurt. Booo
	constexpr int CORE_POINTS_PER_TICK = 2;
	constexpr int TICKS_TO_EMPTY = 100 / CORE_POINTS_PER_TICK; // 50

	// 50 ticks at 2 hp per core = 100.
	int DrainInterval(float drainsPerDay)
	{
		if (drainsPerDay <= 0.0f)
			return 0;
		return static_cast<int>(std::round(MS_PER_DAY / TICKS_TO_EMPTY / drainsPerDay));
	}

	std::string NextDrainText(int interval, int nextDrain, int now)
	{
		return interval > 0 ? Utils::FormatDuration(nextDrain - now) : "off";
	}
}

void CoreDrainInit(CoreDrainState& state, const ModConfig& cfg, int now) {

	state.healthInterval = DrainInterval(cfg.HealthCoreDrainsPerDay);
	state.healthNextDrain = now + state.healthInterval;

	state.staminaInterval = DrainInterval(cfg.StaminaCoreDrainsPerDay);
	state.staminaNextDrain = now + state.staminaInterval;

}

void CoreDrainUpdate(CoreDrainState& state, Ped ped, int now, std::vector<std::string>& debug) {

	debug.push_back(std::format("Drain Intervals: H:{} S:{}", state.healthInterval, state.staminaInterval));
	debug.push_back(std::format("Next Drain In: H:{} S:{}", NextDrainText(state.healthInterval, state.healthNextDrain, now), NextDrainText(state.staminaInterval, state.staminaNextDrain, now)));

	// An interval of 0 means that core's drain is turned off in the config.
	if (state.healthInterval > 0 && now > state.healthNextDrain) {
		ATTRIBUTE::_SET_ATTRIBUTE_CORE_VALUE(ped, ATTRIBUTE_CORE_HEALTH, std::max(0, ATTRIBUTE::_GET_ATTRIBUTE_CORE_VALUE(ped, ATTRIBUTE_CORE_HEALTH) - CORE_POINTS_PER_TICK));
		state.healthNextDrain += state.healthInterval;
	}

	if (state.staminaInterval > 0 && now > state.staminaNextDrain) {
		ATTRIBUTE::_SET_ATTRIBUTE_CORE_VALUE(ped, ATTRIBUTE_CORE_STAMINA, std::max(0, ATTRIBUTE::_GET_ATTRIBUTE_CORE_VALUE(ped, ATTRIBUTE_CORE_STAMINA) - CORE_POINTS_PER_TICK));
		state.staminaNextDrain += state.staminaInterval;
	}

}
