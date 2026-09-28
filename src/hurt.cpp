#include "hurt.h"
#include "hypothermia.h"
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

	// Initially I had this set up to reduce by 2 because reducing the core HP by 1 sometimes got weird and would heal instead of hurt. Booo
	// Later figured out it wasn't the 1hp reduction specifically, but landing on certain values that would always bounce back. 
	// Working around this with GetTrueNewCoreValue();
	constexpr int CORE_POINTS_PER_TICK = 1;
	constexpr int TICKS_TO_EMPTY = 100 / CORE_POINTS_PER_TICK;

	// The game's own script keeps a float copy of each core and ~1.5 s after a change writes back
	// CEIL() of it. Float round-off turns these five values into x.000001, so they come back +1.
	// The core jump will create a trap where we can't ever drain past them if we keep hitting the same value. 
	// Solution: drain by 1 more hp when these troublemakers are encountered.
	int GetSafeCoreDrainValue(int v) {
		if (v == 15 || v == 27 || v == 30 || v == 54 || v == 60) {
			return v - 1;
		}
		return v;
	}

	//16 is the point when the health core turns from white to red. Red core = drain HP
	constexpr int HURTING_THRESHOLD = 16;
	constexpr int HITPOINT_DRAIN_STEPS = 50;

	int DrainInterval(float drainsPerDay)
	{
		if (drainsPerDay <= 0.0f)
			return 0;
		return static_cast<int>(std::round(static_cast<double>(MS_PER_DAY) / TICKS_TO_EMPTY / drainsPerDay));
	}

	//limit: How long we can drain to. Set above 0 to keep health drain non-lethal.
	void DrainCore(Ped ped, int core, int drains, int limit = 0)
	{
		if (drains <= 0)
			return;
		int value = ATTRIBUTE::_GET_ATTRIBUTE_CORE_VALUE(ped, core);
		ATTRIBUTE::_SET_ATTRIBUTE_CORE_VALUE(ped, core, std::max(limit, GetSafeCoreDrainValue(value - drains * CORE_POINTS_PER_TICK)));
	}



}

void Hurt::Init(State& state, const ModConfig& cfg) {
	state.healthCore = ElasticInterval(DrainInterval(cfg.HealthCoreDrainsPerDay));
	state.staminaCore = ElasticInterval(DrainInterval(cfg.StaminaCoreDrainsPerDay));
	state.hitPoints = ElasticInterval(static_cast<int>(std::round(static_cast<double>(cfg.HurtingTimeToDieMax) * 60 * 1000 / HITPOINT_DRAIN_STEPS)));
}

void Hurt::Update(State& state, Player player, Ped playerPed, int dtMs, const HypothermiaEffects& weatherEffects, const ModConfig& cfg, std::vector<std::string>& debug) {

	//Clothing appropriatness. 0 = too cold. 50 = ok. 100 = too hot.
	//TODO Remove from here and just have it effect sweating?
	//TODO Make no hat effect something
	int bodyheat = ATTRIBUTE::GET_ATTRIBUTE_POINTS(playerPed, SA_BODYHEAT);

	//Health Core
	double healthCoreRate = 1.0;
	double staminaCoreRate = 1.0;
	double hitpointsRate = 0.0;

	if (bodyheat != 50) {
		staminaCoreRate += cfg.ImproperClothingStaminaDrainRate;
	}

	healthCoreRate += weatherEffects.healthCoreDrainModifier;
	staminaCoreRate += weatherEffects.staminaCoreDrainModifier;

	DrainCore(playerPed, ATTRIBUTE_CORE_HEALTH, state.healthCore.Update(dtMs, healthCoreRate), cfg.HurtingIsLethal ? 0 : 1);
	DrainCore(playerPed, ATTRIBUTE_CORE_STAMINA, state.staminaCore.Update(dtMs, staminaCoreRate));

	debug.push_back(std::format("Next Drain In: H({}):{} S({}):{}", healthCoreRate, Utils::FormatDuration(state.healthCore.RemainingMs(healthCoreRate)), staminaCoreRate, Utils::FormatDuration(state.staminaCore.RemainingMs(staminaCoreRate))));

	// Scale hurting rate with a red health core
	int playerHealthCoreValue = ATTRIBUTE::_GET_ATTRIBUTE_CORE_VALUE(playerPed, ATTRIBUTE_CORE_HEALTH);

	// Stop health regen if below threshold.
	float healingRate = 0.0f;
	if (cfg.HealingThreshold <= playerHealthCoreValue || ATTRIBUTE::_IS_ATTRIBUTE_CORE_OVERPOWERED(playerPed, ATTRIBUTE_CORE_HEALTH)) {
		//If above threshhold allow healing. Healing rate scales up the more full the health core is
		healingRate = Utils::ScaleToRange(static_cast<float>(playerHealthCoreValue), 0, HURTING_THRESHOLD);
	}
	PLAYER::SET_PLAYER_HEALTH_RECHARGE_MULTIPLIER(player, healingRate);

	if (playerHealthCoreValue <= HURTING_THRESHOLD) {

		//Get current rate scaled between min and max hurt time.
		// 0 at core 0, 1 at core 16
		double t = std::clamp(playerHealthCoreValue, 0, HURTING_THRESHOLD) / double(HURTING_THRESHOLD);
		double timeToDie = std::lerp(cfg.HurtingTimeToDieMin, cfg.HurtingTimeToDieMax, t);
		hitpointsRate = cfg.HurtingTimeToDieMax / timeToDie;
	}

	int numHpDrains = state.hitPoints.Update(dtMs, hitpointsRate);

	debug.push_back(std::format("Next HP Drain In ({}) {}", hitpointsRate, Utils::FormatDuration(state.hitPoints.RemainingMs(hitpointsRate))));
	debug.push_back(std::format("Player HP {}/{}", ENTITY::GET_ENTITY_HEALTH(playerPed), ENTITY::GET_ENTITY_MAX_HEALTH(playerPed, FALSE)));
	debug.push_back(std::format("Healing rate {}>={} = {}", cfg.HealingThreshold, playerHealthCoreValue, healingRate));


	//Ding hit points. We only do this beyond the hurting threshold
	if (numHpDrains > 0) {
		//ENTITY::GET_ENTITY_MAX_HEALTH returns total of outer ring plus 100 hp from a full core. FYI
		float drainPerStep = static_cast<float>(ENTITY::GET_ENTITY_MAX_HEALTH(playerPed, FALSE)) / HITPOINT_DRAIN_STEPS;

		if (!cfg.HurtingIsLethal) {
			int currentHealth = ENTITY::GET_ENTITY_HEALTH(playerPed);
			int estNewHealth = static_cast<int>(std::round(currentHealth - (drainPerStep * numHpDrains)));
			if (estNewHealth <= playerHealthCoreValue) {
				ENTITY::SET_ENTITY_HEALTH(playerPed, playerHealthCoreValue, 0);
				return;
			}
		}

		ENTITY::_CHANGE_ENTITY_HEALTH(playerPed, -drainPerStep * numHpDrains, 0, 0);

	}


}

