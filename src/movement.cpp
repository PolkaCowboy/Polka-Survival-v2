#include "movement.h"
#include "utils.h"

#include <algorithm>
#include <format>

namespace {

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

	// Roughly what a cap feels like, for the debug overlay.
	const char* CapFeel(float cap) {
		if (cap >= NO_CAP)
			return "full speed";
		if (cap >= 2.0f)
			return "slowed run";
		if (cap >= JOG_CAP)
			return "jog";
		if (cap >= POWER_WALK_CAP)
			return "power walk";
		return "stroll";
	}

}

void SetMovementCap(Ped playerPed, MovementState& state, std::vector<std::string>& debug) {
	int healthCore = ATTRIBUTE::_GET_ATTRIBUTE_CORE_VALUE(playerPed, ATTRIBUTE_CORE_HEALTH);
	int staminaCore = ATTRIBUTE::_GET_ATTRIBUTE_CORE_VALUE(playerPed, ATTRIBUTE_CORE_STAMINA);
	float coreCap;

	float healthRingMax = 50.0f * (ATTRIBUTE::GET_ATTRIBUTE_RANK(playerPed, PA_HEALTH) + 1);
	float healthRing = static_cast<float>(ENTITY::GET_ENTITY_HEALTH(playerPed) - healthCore);
	float ringMissing = 1.0f - std::clamp(healthRing / healthRingMax, 0.0f, 1.0f);

	// Gold rings offer slight bonus.
	bool healthGold = ATTRIBUTE::_IS_ATTRIBUTE_OVERPOWERED(playerPed, PA_HEALTH);
	bool staminaGold = ATTRIBUTE::_IS_ATTRIBUTE_OVERPOWERED(playerPed, PA_STAMINA);
	if (healthGold) {
		healthCore += GOLD_RING_BONUS;
	}
	if (staminaGold) {
		staminaCore += GOLD_RING_BONUS;
	}

	float sum = static_cast<float>(std::min(healthCore + staminaCore, 200));
	if (sum >= SUM_JOG) {
		float t = Utils::MapToRange(sum, SUM_FULL_SPEED, SUM_JOG, 0.0f, 1.0f);
		coreCap = NO_CAP - (NO_CAP - JOG_CAP) * t * t;
	}
	else {
		coreCap = Utils::MapToRange(sum, SUM_JOG, 0.0f, JOG_CAP, POWER_WALK_CAP);
	}

	float finalCap = coreCap - MISSING_HEALTH_PENALTY * ringMissing;

	bool applyCap = finalCap < NO_CAP && PED::IS_PED_ON_FOOT(playerPed) && !ENTITY::IS_ENTITY_DEAD(playerPed);
	if (applyCap) {
		PED::SET_PED_MAX_MOVE_BLEND_RATIO(playerPed, finalCap);
	}
	else if (state.capped) {
		PED::SET_PED_MAX_MOVE_BLEND_RATIO(playerPed, NO_CAP);
	}
	state.capped = applyCap;

	std::string why;
	if (finalCap >= NO_CAP)
		why = "no cap needed";
	else if (!PED::IS_PED_ON_FOOT(playerPed))
		why = "not on foot, not applied";
	else if (ENTITY::IS_ENTITY_DEAD(playerPed))
		why = "dead, not applied";
	else
		why = "applied";

	debug.push_back("MOVEMENT");
	debug.push_back(std::format("cores {} + {} = {:.0f}{}{}  -> core cap {:.2f}", healthCore, staminaCore, sum,
		healthGold ? "  (health ring gold +25)" : "", staminaGold ? "  (stamina ring gold +25)" : "", coreCap));
	debug.push_back(std::format("health ring {:.0f} of {:.0f} HP, {:.0f}% missing  -> -{:.2f}", healthRing, healthRingMax,
		ringMissing * 100.0f, MISSING_HEALTH_PENALTY * ringMissing));
	debug.push_back(std::format("move cap {:.2f} ({})  {}", finalCap, CapFeel(finalCap), why));
	debug.push_back("");
}
