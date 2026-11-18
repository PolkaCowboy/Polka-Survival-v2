#pragma once

#include "config.h"
#include "../inc/sdk.h"

#include <algorithm>
#include <vector>
#include <string>

// How wet the character is. 0.0 = dry, 1.0 = soaked. Multiply by 100 only for display
// (e.g. the rpg_meter_N ring texture).
class Wetness
{
public:
	float Value() const { return m_value; }

	void Add(float amount) { m_value = std::clamp(m_value + amount, 0.0f, 1.0f); }

	void Set(float value) { m_value = std::clamp(value, 0.0f, 1.0f); }

private:
	float m_value = 0.0f;
};

struct WetnessState
{
	Wetness wetness;
	float fullRainWetPerSecond = 0.0f;
	float nominalDrydownPerSecond = 0.0f;
	float nominalSweatPerSecond = 0.0f;
	float rainExposure = 0.0f;
	float fireWeight = 0.0f;

	// Current temp at player. Faked vs actual in game temp at player location to let me modify how fires work.
	float effectiveTemp = 0.0f;
	
	bool restingAtFire = false;

	int nextCoverProbe = 0;
};

void WetnessInit(WetnessState& state, const ModConfig& cfg);
void WetnessUpdate(WetnessState& state, Ped playerPed, float exertion, int dtMs, const ModConfig& cfg, std::vector<std::string>& debug);
