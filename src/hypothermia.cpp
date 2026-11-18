#include "hypothermia.h"
#include "wetness.h"
#include "utils.h"

#include <format>
#include <algorithm>
#include <cmath>

namespace
{
	// Being wet bad. Max multiplier at full wet
	const float WET_MULT = 6.0f;

	// Wind make chilly
	// Note: Wind value is 0-100, but hurricane seems to max out around 40.
	//   Thus, this value will be weakened. Crank it up for more impactful wind.
	const float WIND_MULT = 1.5f;

	//Outfit too light for cold. (SA_BODYHEAT = 0)
	const float LIGHT_CLOTHES_MULT = 1.5f;

	//Break point for freezing temps to fuck shit up. The closer this is to 1, the worse freezing temps will be.
	const float COLDNESS_AT_FREEZING = 0.45;

	//Fire Recovery Bonus.
	const float FIRE_RECOVERY_BOOST = 3.0f;

	// Low chill just drains stamina faster.
	const float MAX_STAMINA_DRAIN_MODIFIER = 1.0f;

	// Just bleed cores up to this point.
	const float HP_DAMAGE_THRESHOLD = 0.75F;

	// 1.0 = HurtingTimeToDieMax when hypothermia is maxed.
	const float MAX_HP_DRAIN_MODIFIER = 1.0f;

	// Get coldness modifying the "curve" (it's linear) below freezing based on COLDNESS_AT_FREEZING
	float Coldness(float tempC, const ModConfig& cfg) {
		if (tempC > cfg.FreezingTemperatureC) {
			return Utils::MapToRange(tempC, cfg.NominalTemperatureC, cfg.FreezingTemperatureC, 0.0f, COLDNESS_AT_FREEZING);
		}
		return Utils::MapToRange(tempC, cfg.FreezingTemperatureC, cfg.BitterColdTemperatureC, COLDNESS_AT_FREEZING, 1.0f);
	}

}

void HypothermiaInit(HypothermiaState& state, const ModConfig& cfg)
{
	state = HypothermiaState{};
}

void HypothermiaUpdate(HypothermiaState& state, Ped playerPed, const WetnessState& wetness, int dtMs, const ModConfig& cfg, std::vector<std::string>& debug)
{
	state.effects = HypothermiaEffects{};

	if (!cfg.EnableHypothermia) {
		debug.push_back("HYPOTHERMIA off");
		debug.push_back("");
		return;
	}

	float seconds = dtMs / 1000.0f;

	float coldness = Coldness(wetness.effectiveTemp, cfg);
	float wetChill = std::lerp(1.0f, WET_MULT, wetness.wetness.Value());

	float windSpeed = PED::_GET_PED_BLACKBOARD_FLOAT(playerPed, "WindSpeed");
	float windChill = std::lerp(1.0f, WIND_MULT, Utils::MapToRange(windSpeed, 1.0f, 100.0f, 0.0f, 1.0f));

	float clothing = ATTRIBUTE::GET_ATTRIBUTE_POINTS(playerPed, SA_BODYHEAT) == 0 ? LIGHT_CLOTHES_MULT : 1.0f;

	float target = std::clamp(coldness * wetChill * windChill * clothing, 0.0f, 1.0f);

	bool gettingColder = state.severity < target;
	float step = 0.0f;
	if (gettingColder) {
		step = seconds / cfg.TimeToHypothermiaMin;
	}
	else {
		float recovery = std::lerp(1.0f, FIRE_RECOVERY_BOOST, wetness.fireWeight);
		step = seconds / cfg.HypothermiaRecoverNominal * recovery;
	}
	state.severity = Utils::MoveTowards(state.severity, target, step);

	state.effects.staminaCoreDrainModifier = state.severity * MAX_STAMINA_DRAIN_MODIFIER;
	if (state.severity > HP_DAMAGE_THRESHOLD) {
		state.effects.hitPointsDrainModifier = Utils::MapToRange(state.severity, HP_DAMAGE_THRESHOLD, 1.0f, 0.0f, MAX_HP_DRAIN_MODIFIER);
	}

	debug.push_back("HYPOTHERMIA");
	debug.push_back(std::format("severity {:.3f} -> target {:.2f}  ({})", state.severity, target, gettingColder ? "getting colder" : "warming up"));
	debug.push_back(std::format("cold {:.2f} x wet {:.2f} x wind {:.2f} (speed {:.1f}) x clothes {:.2f}", coldness, wetChill, windChill, windSpeed, clothing));
	debug.push_back(std::format("effects  stamina drain {:+.2f}  HP drain {:+.2f}", state.effects.staminaCoreDrainModifier, state.effects.hitPointsDrainModifier));
	debug.push_back("");
}
