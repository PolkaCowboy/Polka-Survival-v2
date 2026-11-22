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

	// The game's own script keeps a float copy of each core and ~1.5 s after a change writes back
	// CEIL() of it. Float round-off turns these five values into x.000001, so they come back +1.
	// The core jump will create a wall where we can't ever drain past them. 
	// Solution: Pretend they don't exist and adjust timing to 100 - (Size of this array).
	constexpr int SKIP_VALUES[] = { 15, 27, 30, 54, 60 };

	constexpr int TICKS_TO_EMPTY = 100 - std::size(SKIP_VALUES);

	int GetSafeCoreDrainValue(int v) {
		if (std::ranges::find(SKIP_VALUES, v) != std::end(SKIP_VALUES)) {
			return v - 1;
		}
		return v;
	}

	//16 is the point when the health core turns from white to red. Red core = drain HP
	constexpr int HURTING_THRESHOLD = 16;
	constexpr int HITPOINT_DRAIN_STEPS = 50;

	// We are basing all HP drain calculations on health core level 7 (Last one before the final two bonus ranks)
	// Each health rang grants 50hp. Rank 0 = 50hp.
	constexpr float REFERENCE_RING_HP = 400.0f;

	int DrainInterval(float drainsPerDay)
	{
		if (drainsPerDay <= 0.0f)
			return 0;
		return static_cast<int>(std::round(static_cast<double>(MS_PER_DAY) / TICKS_TO_EMPTY / drainsPerDay));
	}

	//limit: How low we can drain to. Set above 0 to keep health drain non-lethal.
	void DrainCore(Ped ped, int core, int drains, int limit = 0)
	{
		if (drains <= 0)
			return;
		int value = ATTRIBUTE::_GET_ATTRIBUTE_CORE_VALUE(ped, core);
		ATTRIBUTE::_SET_ATTRIBUTE_CORE_VALUE(ped, core, std::max(limit, GetSafeCoreDrainValue(value - drains)));
	}

	// Vanilla, good outfit drain is about 2 days for a full core.
	constexpr float VANILLA_DRAINS_PER_DAY = 0.5f;

	float FortifyScale(float drainsPerDay) {
		if (drainsPerDay <= 0.0f) {
			return 1.0f;
		}
		return std::min(1.0f, VANILLA_DRAINS_PER_DAY / drainsPerDay);
	}
	
	void DrainCoreOrFortify(Ped playerPed, int core, ElasticInterval& interval, float& lastFortifySeconds, double rate, float drainsPerDay, int dtMs, int hurtLimit = 0) {
		float left = ATTRIBUTE::_GET_ATTRIBUTE_CORE_OVERPOWER_SECONDS_LEFT(playerPed, core);

		//If the timer just jumped up that means something was consumed. Scale it down.
		if (lastFortifySeconds >= 0.0f && left > lastFortifySeconds + 1.0f) {
			left *= FortifyScale(drainsPerDay);
			ATTRIBUTE::_ENABLE_ATTRIBUTE_CORE_OVERPOWER(playerPed, core, left, FALSE);
		}

		if (ATTRIBUTE::_IS_ATTRIBUTE_CORE_OVERPOWERED(playerPed, core)) {
			int extraDrains = interval.Update(dtMs, rate - 1.0);
			if (extraDrains > 0) {
				left = std::max(0.0f, left - extraDrains * interval.IntervalMs() / 1000.0f);
				ATTRIBUTE::_ENABLE_ATTRIBUTE_CORE_OVERPOWER(playerPed, core, left, FALSE);
			}
		}
		else {
			DrainCore(playerPed, core, interval.Update(dtMs, rate), hurtLimit);
		}

		lastFortifySeconds = left;

	}

	// A gold (fortified) core is a timer separate from the 0-100 value.
	std::string FortifyText(Ped ped, int core)
	{
		if (!ATTRIBUTE::_IS_ATTRIBUTE_CORE_OVERPOWERED(ped, core))
			return "no";
		return std::format("{:.0f} s left", ATTRIBUTE::_GET_ATTRIBUTE_CORE_OVERPOWER_SECONDS_LEFT(ped, core));
	}

	// A gold (fortified) outer ring, from tonics. Same kind of timer, on the attribute instead of the core.
	std::string RingFortifyText(Ped ped, int attribute)
	{
		if (!ATTRIBUTE::_IS_ATTRIBUTE_OVERPOWERED(ped, attribute))
			return "no";
		return std::format("{:.0f} s left", ATTRIBUTE::_GET_ATTRIBUTE_OVERPOWER_SECONDS_LEFT(ped, attribute));
	}

	std::string NextTick(const ElasticInterval& interval, double rate)
	{
		int ms = interval.RemainingMs(rate);
		if (ms < 0)
			return "off";
		return std::format("x{:.2f} in {}", rate, Utils::FormatDuration(ms));
	}

}

void Hurt::Init(State& state, const ModConfig& cfg) {
	state.healthCore = ElasticInterval(DrainInterval(cfg.HealthCoreDrainsPerDay));
	state.staminaCore = ElasticInterval(DrainInterval(cfg.StaminaCoreDrainsPerDay));
	state.hitPoints = ElasticInterval(static_cast<int>(std::round(static_cast<double>(cfg.HurtingTimeToDieMax) * 60 * 1000 / HITPOINT_DRAIN_STEPS)));
}

void Hurt::Update(State& state, Player player, Ped playerPed, int dtMs, const Hypothermia::Effects& weatherEffects, const ModConfig& cfg, std::vector<std::string>& debug) {


	// In vanilla, improper clothing changes health core drain from 90 minutes to 10.
	// Our own extra drain setting will block the slower drain as it is fast, but not the fatser one. 
	// We need to do schenanigans to undo the extra vanilla drain when in mismatched clothing.
	int bodyheat = ATTRIBUTE::GET_ATTRIBUTE_POINTS(playerPed, SA_BODYHEAT);
	int healthValue = ATTRIBUTE::_GET_ATTRIBUTE_CORE_VALUE(playerPed, ATTRIBUTE_CORE_HEALTH);
	bool emptyHealthRing = ENTITY::GET_ENTITY_HEALTH(playerPed) <= healthValue;
	if (state.expectedHealthCore >= 0 && healthValue == state.expectedHealthCore - 1 && bodyheat != 50 && !emptyHealthRing) {
		//Undo the vanilla drain.
		ATTRIBUTE::_SET_ATTRIBUTE_CORE_VALUE(playerPed, ATTRIBUTE_CORE_HEALTH, state.expectedHealthCore);
	}
	else {
		state.expectedHealthCore = healthValue;
	}


	//Health Core
	double healthCoreRate = 1.0;
	double staminaCoreRate = 1.0;
	double hitpointsRate = 0.0;

	healthCoreRate += weatherEffects.healthCoreDrainModifier;
	staminaCoreRate += weatherEffects.staminaCoreDrainModifier;

	DrainCoreOrFortify(playerPed, ATTRIBUTE_CORE_HEALTH, state.healthCore, state.healthFortifySeconds, healthCoreRate, cfg.HealthCoreDrainsPerDay, dtMs, cfg.HurtingIsLethal ? 0 : 1);
	DrainCoreOrFortify(playerPed, ATTRIBUTE_CORE_STAMINA, state.staminaCore, state.staminaFortifySeconds, staminaCoreRate, cfg.StaminaCoreDrainsPerDay, dtMs);
	
	state.expectedHealthCore = ATTRIBUTE::_GET_ATTRIBUTE_CORE_VALUE(playerPed, ATTRIBUTE_CORE_HEALTH);

	// Scale hurting rate with a red health core
	int playerHealthCoreValue = ATTRIBUTE::_GET_ATTRIBUTE_CORE_VALUE(playerPed, ATTRIBUTE_CORE_HEALTH);

	// Stop health regen if below threshold.
	float healingRate = 0.0f;
	if (cfg.HealingThreshold <= playerHealthCoreValue || ATTRIBUTE::_IS_ATTRIBUTE_CORE_OVERPOWERED(playerPed, ATTRIBUTE_CORE_HEALTH)) {
		//If above threshhold allow healing. Healing rate scales up the more full the health core is
		healingRate = Utils::ScaleToRange(static_cast<float>(playerHealthCoreValue), 0, HURTING_THRESHOLD);
	}

	if (weatherEffects.hitPointsDrainModifier > 0.0)
		healingRate = 0.0f;

	PLAYER::SET_PLAYER_HEALTH_RECHARGE_MULTIPLIER(player, healingRate);

	if (playerHealthCoreValue <= HURTING_THRESHOLD) {

		//Get current rate scaled between min and max hurt time.
		// 0 at core 0, 1 at core 16
		double t = std::clamp(playerHealthCoreValue, 0, HURTING_THRESHOLD) / double(HURTING_THRESHOLD);
		double timeToDie = std::lerp(cfg.HurtingTimeToDieMin, cfg.HurtingTimeToDieMax, t);
		hitpointsRate = cfg.HurtingTimeToDieMax / timeToDie;
	}

	hitpointsRate += weatherEffects.hitPointsDrainModifier;

	int numHpDrains = state.hitPoints.Update(dtMs, hitpointsRate);

	debug.push_back("CORES");
	debug.push_back(std::format("health {}  stamina {}  dead eye {}", playerHealthCoreValue,
		ATTRIBUTE::_GET_ATTRIBUTE_CORE_VALUE(playerPed, ATTRIBUTE_CORE_STAMINA), ATTRIBUTE::_GET_ATTRIBUTE_CORE_VALUE(playerPed, ATTRIBUTE_CORE_DEADEYE)));
	debug.push_back(std::format("gold cores  health: {}  stamina: {}  dead eye: {}", FortifyText(playerPed, ATTRIBUTE_CORE_HEALTH),
		FortifyText(playerPed, ATTRIBUTE_CORE_STAMINA), FortifyText(playerPed, ATTRIBUTE_CORE_DEADEYE)));
	debug.push_back(std::format("gold rings  health: {}  stamina: {}  dead eye: {}", RingFortifyText(playerPed, PA_HEALTH),
		RingFortifyText(playerPed, PA_STAMINA), RingFortifyText(playerPed, PA_SPECIALABILITY)));
	debug.push_back(std::format("next drain  health: {}  stamina: {}", NextTick(state.healthCore, healthCoreRate), NextTick(state.staminaCore, staminaCoreRate)));
	debug.push_back(std::format("HP {}/{}  next HP drain: {}", ENTITY::GET_ENTITY_HEALTH(playerPed), ENTITY::GET_ENTITY_MAX_HEALTH(playerPed, FALSE), NextTick(state.hitPoints, hitpointsRate)));
	debug.push_back(std::format("healing x{:.2f}  (heals at health core {} or above)", healingRate, cfg.HealingThreshold));
	debug.push_back("");


	//Ding hit points. We only do this beyond the hurting threshold
	if (numHpDrains > 0) {
		
		float currentMaxRingHP = 50.0f * (ATTRIBUTE::GET_ATTRIBUTE_BASE_RANK(playerPed, PA_HEALTH) + 1);

		float drainPerStep = REFERENCE_RING_HP / HITPOINT_DRAIN_STEPS * std::sqrt(currentMaxRingHP / REFERENCE_RING_HP);

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

void Hurt::SimulateSkip(State& state, Ped playerPed, int skippedMs, const Hypothermia::Effects& weatherEffects, const ModConfig& cfg) {
	if (skippedMs <= 0) {
		return;
	}

	double healthCoreRate = 1.0 + weatherEffects.healthCoreDrainModifier;
	double staminaCoreRate = 1.0 + weatherEffects.staminaCoreDrainModifier;

	DrainCoreOrFortify(playerPed, ATTRIBUTE_CORE_HEALTH, state.healthCore, state.healthFortifySeconds, healthCoreRate, cfg.HealthCoreDrainsPerDay, skippedMs, cfg.HurtingIsLethal ? 0 : 1);
	DrainCoreOrFortify(playerPed, ATTRIBUTE_CORE_STAMINA, state.staminaCore, state.staminaFortifySeconds, staminaCoreRate, cfg.StaminaCoreDrainsPerDay, skippedMs);
	state.expectedHealthCore = ATTRIBUTE::_GET_ATTRIBUTE_CORE_VALUE(playerPed, ATTRIBUTE_CORE_HEALTH);
}