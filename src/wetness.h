#pragma once

#include "../inc/sdk.h"

#include <algorithm>
#include <vector>
#include <string>

namespace Wetness
{
	// How wet the character is. 0.0 = dry, 1.0 = soaked. Multiply by 100 only for display
	// (e.g. the rpg_meter_N ring texture).
	class Level
	{
	public:
		float Value() const { return m_value; }

		void Add(float amount) { m_value = std::clamp(m_value + amount, 0.0f, 1.0f); }

		void Set(float value) { m_value = std::clamp(value, 0.0f, 1.0f); }

	private:
		float m_value = 0.0f;
	};

	struct State
	{
		Level wetness;
		float fullRainWetPerSecond = 0.0f;
		float nominalDrydownPerSecond = 0.0f;
		float nominalSweatPerSecond = 0.0f;
		float rainExposure = 0.0f;
		float fireWeight = 0.0f;
		float exertion = 0.0f;

		// Current temp at player. Faked vs actual in game temp at player location to let me modify how fires work.
		float effectiveTemp = 0.0f;
		
		bool restingAtFire = false;

		int nextCoverProbe = 0;
	};

	struct Settings
	{
		float maxSweatAccumulation = 0.4f; //0-1 range. What percentage of max soak you can get purely by sweating.
		float minTimeToGetSoaked = 30.0f; //Seconds. Time to get 100% soaked in a full rainstorm.
		float minTimeToDry = 240.0f; //Seconds. Time to completely dry without a heat source in a dry hot climate. Campfires/heatsources will accelerate this. Other weather factors will slow this.
	};

	// Seconds to reach the INI's MAX_SWEAT_ACCUMULATION at full exertion, at the NOMINAL temperature.
	// Too-warm clothing and hot weather speed this up.
	constexpr float NOMINAL_MAX_SWEAT_TIME = 180.0f;

	//Fire drying stuff

	//Within this distance of a fire, its drying is at full strength.
	const float HEAT_FULL_RADIUS = 3.0f;
	//Fire drying fades out between HEAT_FULL_RADIUS and this distance.
	const float HEAT_FADE_RADIUS = 6.0f;

	// Max dry down rate when next to a heat source. 
	const float MAX_HEAT_DRY_FACTOR = 4.0f;
	// How much rain a fire blocks when you're within HEAT_FULL_RADIUS. 1 = all of it, 0 = none.
	const float FIRE_RAIN_PROTECTION = 1.0f;

	//Resting at fire effects

	// Resting near a fire increases drying rate. (Time saving shit) 
	const float REST_DRYDOWN_BOOST = 4.0f;

	void Init(State& state, const Settings& settings);
	void Update(State& state, Ped playerPed, float exertion, int dtMs, std::vector<std::string>& debug);
}
