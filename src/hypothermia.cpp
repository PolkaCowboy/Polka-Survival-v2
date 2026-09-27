#include "hypothermia.h"

#include <format>

void HypothermiaInit(HypothermiaState& state, const ModConfig& cfg)
{
	state = HypothermiaState{};
}

void HypothermiaUpdate(HypothermiaState& state, Ped playerPed, float wetness, int dtMs, const ModConfig& cfg, std::vector<std::string>& debug)
{
	// Recalculated from scratch every frame, so an effect ends as soon as its cause does.
	state.effects = HypothermiaEffects{};

	if (!cfg.EnableHypothermia)
		return;

	// TODO: combine wetness (0.0-1.0) with temperature, clothing and heat sources into
	// state.effects.healthDrainModifier / staminaDrainModifier.

	debug.push_back(std::format("Hypothermia: wetness in {:.3f}  drain modifier H {:+.2f} S {:+.2f}", wetness, state.effects.healthCoreDrainModifier, state.effects.staminaCoreDrainModifier));
}
