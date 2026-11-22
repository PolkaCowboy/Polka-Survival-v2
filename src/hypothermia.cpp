#include "hypothermia.h"
#include "wetness.h"
#include "utils.h"
#include "clothing.h"
#include "temperature.h"

#include <format>
#include <algorithm>
#include <cmath>

using namespace Hypothermia;

namespace
{

	constexpr bool CAN_SHOW_DEBUG = true;

	Hypothermia::Settings g_settings;

	// We do not need to recalc outfit warmth every frame.
	constexpr int WARMTH_CHECK_INTERVAL = 1000;

	// Get coldness modifying the "curve" (it's linear) below freezing based on COLDNESS_AT_FREEZING
	float Coldness(float tempC) {
		const Temperature::Settings& temp = Temperature::Get();
		if (tempC > temp.freezingC) {
			return Utils::MapToRange(tempC, temp.nominalC, temp.freezingC, 0.0f, COLDNESS_AT_FREEZING);
		}
		return Utils::MapToRange(tempC, temp.freezingC, temp.bitterColdC, COLDNESS_AT_FREEZING, 1.0f);
	}

	float ClothingProtection(float warmthSum, float tempC) {
		const Temperature::Settings& temp = Temperature::Get();
		float warmth = std::clamp(warmthSum / MAX_WARMTH, 0.0f, 1.0f);
		float bitterProtection = std::lerp(BITTER_INSULATION_MIN, BITTER_INSULATION_MAX, warmth);
		float bitterness = Utils::MapToRange(tempC, temp.freezingC, temp.bitterColdC, 0.0f, 1.0f);
		return warmth * std::lerp(1.0f, bitterProtection, bitterness);
	}

}

void Hypothermia::Init(State& state, const Settings& settings)
{
	state = State{};
	g_settings = settings;
}

void Hypothermia::Update(State& state, Ped playerPed, const Wetness::State& wetness, float exertion, int dtMs, std::vector<std::string>& debug)
{
	state.effects = Effects{};

	if (!g_settings.enabled) {
		debug.push_back("HYPOTHERMIA off");
		debug.push_back("");
		return;
	}

	float seconds = dtMs / 1000.0f;
	int now = MISC::GET_GAME_TIMER();
	if (now >= state.nextWarmthCheck) {
		state.warmthSum = Clothing::WarmthSum(playerPed);
		state.nextWarmthCheck = now + WARMTH_CHECK_INTERVAL;
	}

	float wet = wetness.wetness.Value();
	float wetSquared = wet * wet;

	float clothingProtection = ClothingProtection(state.warmthSum, wetness.effectiveTemp) * (1.0f - WET_INSULATION_LOSS * wetSquared);
	float coldness = Coldness(wetness.effectiveTemp) * (1.0f - clothingProtection);

	float wetChill = std::lerp(1.0f, WET_MULT, wet);

	float windChill = 1.0f;
	float windSpeed = 0.0f;
	if (!INTERIOR::IS_INTERIOR_SCENE()) {
		windSpeed = PED::_GET_PED_BLACKBOARD_FLOAT(playerPed, "WindSpeed");
		windChill = std::lerp(1.0f, WIND_MULT, Utils::MapToRange(windSpeed, 1.0f, 100.0f, 0.0f, 1.0f));
	}

	float target = wetness.restingAtFire ? 0.0f : std::clamp(coldness * wetChill * windChill, 0.0f, 1.0f);
	//Allow slight warming up via sprinting.
	target *= 1.0f - exertion * EXERTION_TARGET_DROP;

	// How fast severity moves towards the target, in severity per second.
	bool gettingColder = state.severity < target;
	float rate = 0.0f;
	if (gettingColder) {
		rate = std::lerp(1.0f / g_settings.timeToHypothermiaDry, 1.0f / g_settings.timeToHypothermiaWet, wetSquared);
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
		rate = recovery / g_settings.recoverNominal;
	}
	state.severity = Utils::MoveTowards(state.severity, target, seconds * rate);
	state.target = target;

	// Let me tune the applied severity a bit. Severity below SEVERITY_EFFECT_START costs no cores.
	float actualEffect = Utils::MapToRange(state.severity, SEVERITY_EFFECT_START, 1.0f, 0.0f, 1.0f);

	float soakedStamina = std::lerp(1.0f, SOAKED_STAMINA_DRAIN_MULT, wetSquared);
	float soakedHealth = std::lerp(1.0f, SOAKED_HEALTH_DRAIN_MULT, wetSquared);

	state.effects.staminaCoreDrainModifier = actualEffect * MAX_STAMINA_DRAIN_MODIFIER * soakedStamina;

	int stamina = ATTRIBUTE::_GET_ATTRIBUTE_CORE_VALUE(playerPed, ATTRIBUTE_CORE_STAMINA);
	float staminaEmpty = 1.0f - std::clamp(stamina, 0, 100) / 100.0f; // 0 = full, 1 = empty
	state.effects.healthCoreDrainModifier = MAX_HEALTH_DRAIN_MODIFIER * (staminaEmpty * staminaEmpty) * actualEffect * soakedHealth;

	if (state.severity > HP_DAMAGE_THRESHOLD) {
		state.effects.hitPointsDrainModifier = Utils::MapToRange(state.severity, HP_DAMAGE_THRESHOLD, 1.0f, 0.4f, MAX_HP_DRAIN_MODIFIER);
	}

	// INI EFFECT_STRENGTH scales what the cold costs.
	state.effects.staminaCoreDrainModifier *= g_settings.effectStrength;
	state.effects.healthCoreDrainModifier *= g_settings.effectStrength;
	state.effects.hitPointsDrainModifier *= g_settings.effectStrength;

	if (CAN_SHOW_DEBUG) {
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
		debug.push_back(std::format("warmth {:.1f} -> protection {:.0f}%", state.warmthSum, clothingProtection * 100.0f));
		debug.push_back(std::format("effects  stamina drain {:+.2f}  health core drain {:+.2f}  HP drain {:+.2f}", state.effects.staminaCoreDrainModifier, state.effects.healthCoreDrainModifier, state.effects.hitPointsDrainModifier));
		debug.push_back("");
	}
}
