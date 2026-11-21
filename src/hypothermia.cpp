#include "hypothermia.h"
#include "wetness.h"
#include "utils.h"
#include "clothing.h"

#include <format>
#include <algorithm>
#include <cmath>

namespace
{
	// Being wet bad. Max multiplier at full wet
	const float WET_MULT = 2.5f;

	// More wet = more bad. wet feet? We can deal. Fell into lake. Not good.
	const float WET_SPEED_CURVE = 2.0f;

	// Ramp up core drains when soaked
	const float SOAKED_STAMINA_DRAIN_MULT = 7.5f;
	const float SOAKED_HEALTH_DRAIN_MULT = 3.0f;

	const float SWIMMING_CHILL_SPEED = 2.0f;

	// Wind make chilly
	// Note: Wind value is 0-100, but hurricane seems to max out around 40.
	//   Thus, this value will be weakened. Crank it up for more impactful wind.
	const float WIND_MULT = 1.5f;

	//Sum of max clothing in game is 9.0f. The custom tuning in of plain clots can push this higher so upping to 9.5f
	const float MAX_WARMTH = 9.5f; 

	//Stay warm by running!? Just don't sweat too much.
	//Slow down cold gains while exerting.
	const float EXERTION_COLD_SLOWDOWN = 0.5f;
	//Warm back up if too cold.
	const float EXERTION_RECOVERY_BOOST = 2.0f;

	//We do not need to recacl outfit warmth every frame.
	constexpr int WARMTH_CHECK_INTERVAL = 1000;

	//Bitter temp will cut into clothing protection. Heavier cloths will retain more.
	const float BITTER_INSULATION_MIN = 0.1f;
	const float BITTER_INSULATION_MAX = 0.5f;

	//Break point for freezing temps to fuck shit up. The closer this is to 1, the worse freezing temps will be.
	const float COLDNESS_AT_FREEZING = 0.45;

	//Fire Recovery Bonus.
	const float FIRE_RECOVERY_BOOST = 4.0f;

	// How fast chill drains stamina
	const float MAX_STAMINA_DRAIN_MODIFIER = 3.0f;
	// How fast chill drains health core.
	const float MAX_HEALTH_DRAIN_MODIFIER = 5.0f;

	// Lose insulation when soaked. Square the effect so damp still keeps warmth, but full soak bad.
	const float WET_INSULATION_LOSS = 0.8f;

	// Only bleed cores up to this point. After this start hurting health.
	const float HP_DAMAGE_THRESHOLD = 0.75F;

	// 1.0 = HurtingTimeToDieMax when hypothermia is maxed.
	const float MAX_HP_DRAIN_MODIFIER = 2.5f;

	// Get coldness modifying the "curve" (it's linear) below freezing based on COLDNESS_AT_FREEZING
	float Coldness(float tempC, const ModConfig& cfg) {
		if (tempC > cfg.FreezingTemperatureC) {
			return Utils::MapToRange(tempC, cfg.NominalTemperatureC, cfg.FreezingTemperatureC, 0.0f, COLDNESS_AT_FREEZING);
		}
		return Utils::MapToRange(tempC, cfg.FreezingTemperatureC, cfg.BitterColdTemperatureC, COLDNESS_AT_FREEZING, 1.0f);
	}

	float ClothingProtection(float warmthSum, float tempC, const ModConfig& cfg) {
		float warmth = std::clamp(warmthSum / MAX_WARMTH, 0.0f, 1.0f);
		float bitterProtection = std::lerp(BITTER_INSULATION_MIN, BITTER_INSULATION_MAX, warmth);
		float bitterness = Utils::MapToRange(tempC, cfg.FreezingTemperatureC, cfg.BitterColdTemperatureC, 0.0f, 1.0f);
		return warmth * std::lerp(1.0f, bitterProtection, bitterness);
	}

}

void HypothermiaInit(HypothermiaState& state, const ModConfig& cfg)
{
	state = HypothermiaState{};
}

void HypothermiaUpdate(HypothermiaState& state, Ped playerPed, const WetnessState& wetness, float exertion, int dtMs, const ModConfig& cfg, std::vector<std::string>& debug)
{
	state.effects = HypothermiaEffects{};

	if (!cfg.EnableHypothermia) {
		debug.push_back("HYPOTHERMIA off");
		debug.push_back("");
		return;
	}

	float seconds = dtMs / 1000.0f;
	if (MISC::GET_GAME_TIMER() >= state.nextWarmthCheck) {
		state.warmthSum = Clothing::WarmthSum(playerPed);
		state.nextWarmthCheck = MISC::GET_GAME_TIMER() + WARMTH_CHECK_INTERVAL;
	}

	float clothingProetction = ClothingProtection(state.warmthSum, wetness.effectiveTemp, cfg) * (1.0f - WET_INSULATION_LOSS * wetness.wetness.Value() * wetness.wetness.Value());
	float coldness = Coldness(wetness.effectiveTemp, cfg) * (1.0f - clothingProetction);

	float wetChill = std::lerp(1.0f, WET_MULT, wetness.wetness.Value());
	
	float windChill = 1.0f;
	float windSpeed = 0.0f;
	if (!INTERIOR::IS_INTERIOR_SCENE()) {
		windSpeed = PED::_GET_PED_BLACKBOARD_FLOAT(playerPed, "WindSpeed");
		windChill = std::lerp(1.0f, WIND_MULT, Utils::MapToRange(windSpeed, 1.0f, 100.0f, 0.0f, 1.0f));
	}

	float target = wetness.restingAtFire ? 0.0f : std::clamp(coldness * wetChill * windChill, 0.0f, 1.0f);
	//Allow warming up via sprinting.
	target *= 1.0f - exertion * 0.2f;

	// How fast severity moves towards the target, in severity per second.
	bool gettingColder = state.severity < target;
	float rate = 0.0f;
	if (gettingColder) {
		float soaked = std::pow(wetness.wetness.Value(), WET_SPEED_CURVE);
		rate = std::lerp(1.0f / cfg.TimeToHypothermiaDry, 1.0f / cfg.TimeToHypothermiaWet, soaked);
		rate *= std::clamp(coldness / COLDNESS_AT_FREEZING, 0.0f, 1.0f);
		rate *= 1.0f - exertion * EXERTION_COLD_SLOWDOWN;
		if (PED::IS_PED_SWIMMING(playerPed)) {
			rate *= SWIMMING_CHILL_SPEED;
		}
	}
	else {
		float recovery = std::lerp(1.0f, FIRE_RECOVERY_BOOST, wetness.fireWeight);
		//Move to get your blood warm
		recovery *= std::lerp(1.0f, EXERTION_RECOVERY_BOOST, exertion);
		rate = recovery / cfg.HypothermiaRecoverNominal;
	}
	state.severity = Utils::MoveTowards(state.severity, target, seconds * rate);
	state.target = target;

	// Let me tune the applied severifty a bit
	float actualEffect = Utils::MapToRange(state.severity, 0.3f, 1.0f, 0.0f, 1.0f);

	float wetSquared = wetness.wetness.Value() * wetness.wetness.Value();
	float soakedStamina = std::lerp(1.0f, SOAKED_STAMINA_DRAIN_MULT, wetSquared);
	float soakedHealth = std::lerp(1.0f, SOAKED_HEALTH_DRAIN_MULT, wetSquared);

	state.effects.staminaCoreDrainModifier = actualEffect * MAX_STAMINA_DRAIN_MODIFIER * soakedStamina;
	
	int stamina = ATTRIBUTE::_GET_ATTRIBUTE_CORE_VALUE(playerPed, ATTRIBUTE_CORE_STAMINA);
	float staminaEmpty = 1.0f - std::clamp(stamina, 0, 100) / 100.0f; // 0 = full, 1 = empty
	state.effects.healthCoreDrainModifier = MAX_HEALTH_DRAIN_MODIFIER * (staminaEmpty * staminaEmpty) * actualEffect * soakedHealth;
		
	if (state.severity > HP_DAMAGE_THRESHOLD) {
		state.effects.hitPointsDrainModifier = Utils::MapToRange(state.severity, HP_DAMAGE_THRESHOLD, 1.0f, 0.4f, MAX_HP_DRAIN_MODIFIER);
	}

	// Time left to reach the target at the current rate, if conditions stay as they are.
	float remaining = std::abs(target - state.severity);
	std::string progress;
	if (remaining <= 0.0f)
		progress = "at target";
	else if (rate <= 0.0f)
		progress = "not moving";
	else
		progress = std::format("{}, {:.0f} s to target", gettingColder ? "getting colder" : "warming up", remaining / rate);

	debug.push_back("HYPOTHERMIA");
	debug.push_back(std::format("severity {:.3f} -> target {:.2f}  ({})", state.severity, target, progress));
	debug.push_back(std::format("cold {:.2f} x wet {:.2f} x wind {:.2f} (speed {:.1f})", coldness, wetChill, windChill, windSpeed));
	debug.push_back(std::format("exertion {:.2f}  severity moving {:.4f}/s", exertion, rate));
	debug.push_back(std::format("warmth {:.1f} -> protection {:.0f}%", state.warmthSum, clothingProetction * 100.0f));
	debug.push_back(std::format("effects  stamina drain {:+.2f}  health core drain {:+.2f}  HP drain {:+.2f}", state.effects.staminaCoreDrainModifier, state.effects.healthCoreDrainModifier, state.effects.hitPointsDrainModifier));
	debug.push_back("");
}
