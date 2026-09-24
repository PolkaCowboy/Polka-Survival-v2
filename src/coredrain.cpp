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

	// Reducing the core HP by 1 sometimes got weird and would heal instead of hurt. Booo
	// So do 2hp 50 times per day instead of 1hp 100 times.
	constexpr int CORE_POINTS_PER_TICK = 2;
	constexpr int TICKS_TO_EMPTY = 100 / CORE_POINTS_PER_TICK; // 50

	int DrainInterval(float drainsPerDay)
	{
		if (drainsPerDay <= 0.0f)
			return 0;
		return static_cast<int>(std::round(static_cast<double>(MS_PER_DAY) / TICKS_TO_EMPTY / drainsPerDay));
	}

	void DrainCore(Ped ped, int core, int drains)
	{
		if (drains <= 0)
			return;
		int value = ATTRIBUTE::_GET_ATTRIBUTE_CORE_VALUE(ped, core);
		ATTRIBUTE::_SET_ATTRIBUTE_CORE_VALUE(ped, core, std::max(0, value - drains * CORE_POINTS_PER_TICK));
	}

}

void CoreDrainInit(CoreDrainState& state, const ModConfig& cfg) {
	state.health = ElasticInterval(DrainInterval(cfg.HealthCoreDrainsPerDay));
	state.stamina = ElasticInterval(DrainInterval(cfg.StaminaCoreDrainsPerDay));
}

void CoreDrainUpdate(CoreDrainState& state, Ped player, int dtMs, std::vector<std::string>& debug) {

	//Clothing appropriatness. 0 = too cold. 50 = ok. 100 = too hot.
	int bodyheat = ATTRIBUTE::GET_ATTRIBUTE_POINTS(player, SA_BODYHEAT);

	//Health Core
	double healthCoreRate = 1.0;
	double staminaCoreRate = 1.0;
	
	if (bodyheat != 50) {
		//Todo: Move into config. 
		staminaCoreRate += .1;
	}

	DrainCore(player, ATTRIBUTE_CORE_HEALTH, state.health.Update(dtMs, healthCoreRate));
	DrainCore(player, ATTRIBUTE_CORE_STAMINA, state.stamina.Update(dtMs, staminaCoreRate));

	debug.push_back(std::format("Next Drain In: H({}):{} S({}):{}", healthCoreRate, Utils::FormatDuration(state.health.RemainingMs(healthCoreRate)), staminaCoreRate, Utils::FormatDuration(state.stamina.RemainingMs(staminaCoreRate))));

}

