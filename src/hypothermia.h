#pragma once

#include "../inc/sdk.h"

#include <vector>
#include <string>

namespace Wetness { struct State; }

namespace Hypothermia
{
	struct Effects
	{
		double healthCoreDrainModifier = 0.0;
		double staminaCoreDrainModifier = 0.0;
		double hitPointsDrainModifier = 0.0;
	};

	struct State
	{
		Effects effects;
		float severity = 0.0f; // 0 = fine, 1 = not fine.
		float target = 0.0f; // where severity is heading under the current conditions, 0-1
		float warmthSum = 0.0f; //Warmth of currently equipped clothing. (Custom caclulation)
		int nextWarmthCheck = 0;
	};

	struct Settings
	{
		bool enabled = true;
		float timeToHypothermiaDry = 400.0f; // Seconds. How long to achieve full hyptermia effects when dry.
		float timeToHypothermiaWet = 7.0f; // Seconds. How long to achieve full hyptermia when soaked.
		float recoverNominal = 60.0f; //Time to recover when warm, without a fire bonus.
		float effectStrength = 1.0f; //Multiplies the core and HP cost of hypothermia. 0 = none.
	};

	// Wetness and swimming

	// Being wet bad. Max multiplier at full wet
	const float WET_MULT = 2.5f;

	// Lose insulation when soaked. Square the effect so damp still keeps warmth, but full soak bad.
	const float WET_INSULATION_LOSS = 0.8f;

	// Ramp up core drains when soaked
	const float SOAKED_STAMINA_DRAIN_MULT = 7.5f;
	const float SOAKED_HEALTH_DRAIN_MULT = 3.0f;

	// Swimming: severity climbs this much faster than when soaked in air.
	const float SWIMMING_CHILL_SPEED = 2.0f;

	// Wind

	// Wind make chilly
	// Note: Wind value is 0-100, but hurricane seems to max out around 40.
	//   Thus, this value will be weakened. Crank it up for more impactful wind.
	const float WIND_MULT = 1.5f;

	// Clothing

	//Sum of max clothing in game is 9.0f. The custom tuning in of plain clothes can push this higher so upping to 9.5f
	const float MAX_WARMTH = 9.5f;

	//Bitter temp will cut into clothing protection. Heavier clothes will retain more.
	const float BITTER_INSULATION_MIN = 0.1f;
	const float BITTER_INSULATION_MAX = 0.5f;

	// Temperature

	//Break point for freezing temps to fuck shit up. The closer this is to 1, the worse freezing temps will be.
	const float COLDNESS_AT_FREEZING = 0.45f;

	// Exertion

	// Stay warm by running!? Just don't sweat too much.
	// Slow down cold gains while exerting.
	const float EXERTION_COLD_SLOWDOWN = 0.5f;
	// Warm back up if too cold.
	const float EXERTION_RECOVERY_BOOST = 2.0f;
	// Exerting also lowers chill target a little.
	const float EXERTION_TARGET_DROP = 0.2f;

	// Recovery

	//Fire Recovery Bonus.
	const float FIRE_RECOVERY_BOOST = 4.0f;

	// Effects

	// Severity below this costs no cores; above it, the core effects ramp up to full at 1.
	const float SEVERITY_EFFECT_START = 0.3f;

	// How fast chill drains stamina
	const float MAX_STAMINA_DRAIN_MODIFIER = 3.0f;
	// How fast chill drains health core.
	const float MAX_HEALTH_DRAIN_MODIFIER = 5.0f;

	// Only bleed cores up to this point. After this start hurting health.
	const float HP_DAMAGE_THRESHOLD = 0.75F;

	// 1.0 = HurtingTimeToDieMax when hypothermia is maxed.
	const float MAX_HP_DRAIN_MODIFIER = 2.5f;

	void Init(State& state, const Settings& settings);
	void Update(State& state, Ped playerPed, const Wetness::State& wetness, float exersion, int dtMs, std::vector<std::string>& debug);
}
