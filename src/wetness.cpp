#include "wetness.h"

#include <format>
#include <cmath>
#include <algorithm>

namespace
{
	// Wetness per second to go from 0.0 to 1.0 in `seconds`. 0 (or less) = never changes.
	float RatePerSecond(float seconds)
	{
		if (seconds <= 0.0f)
			return 0.0f;
		return 1.0f / seconds;
	}

	constexpr int COVER_PROBE_INTERVAL_MS = 400;

	// Shape test flags, from ScriptHookRDR2DotNet-V2 Raycasting.cs (RaycastLOSOptions)
	const int LOS_MAP = 1;
	const int LOS_VEHICLES = 2;
	const int LOS_OBJECTS = 16;
	const int LOS_FOLIAGE = 256;
	// IgnoreGlass | IgnoreSeeThrough | IgnoreNoCollision
	const int LOS_OPTIONS_DEFAULT = 7;

	struct ProbeResult
	{
		bool hit = false;
		float height = 0.0f;
		Entity entity = 0;
	};

	// Straight-up ray from just above the player's head.
	ProbeResult ProbeUp(Ped player, const Vector3& pos, int flags, int options = LOS_OPTIONS_DEFAULT)
	{
		const float startZ = pos.z + 1.0f;
		ScrHandle handle = SHAPETEST::START_EXPENSIVE_SYNCHRONOUS_SHAPE_TEST_LOS_PROBE(
			pos.x, pos.y, startZ, pos.x, pos.y, startZ + 25.0f, flags, player, options);

		BOOL hit = FALSE;
		Vector3 end{};
		Vector3 normal{};
		Entity entity = 0;
		ProbeResult result;
		if (SHAPETEST::GET_SHAPE_TEST_RESULT(handle, &hit, &end, &normal, &entity) == 2 && hit)
		{
			result.hit = true;
			result.height = end.z - startZ;
			result.entity = entity;
		}
		return result;
	}

	//Raw rain intensity is a global weather state and not local to player. We need to determine if player is in cover before accumulating wet
	float RainExposure(Ped playerPed) {

		if (INTERIOR::IS_INTERIOR_SCENE()) {
			return 0.0f;
		}

		Vector3 pos = ENTITY::GET_ENTITY_COORDS(playerPed, TRUE, FALSE);


		//TODO: Put these behind an option for potato PCs?
		//Ray check for non-interior cover
		ProbeResult roofProbe = ProbeUp(playerPed, pos, LOS_MAP | LOS_VEHICLES | LOS_OBJECTS);
		if (roofProbe.hit) {
			return 0.0f;
		}

		//Ray check for tree canopy cover
		ProbeResult canopyProbe = ProbeUp(playerPed, pos, LOS_FOLIAGE);
		if (canopyProbe.hit) {
			return 0.6f;
		}

		return 1.0f;
	}

}

void WetnessInit(WetnessState& state, const ModConfig& cfg)
{
	state = WetnessState{}; 
	state.fullRainWetPerSecond = RatePerSecond(cfg.MinTimeToGetSoaked);
	state.nominalDrydownPerSecond = RatePerSecond(cfg.MinTimeToDry);
}

void WetnessUpdate(WetnessState& state, Ped playerPed, int dtMs, const ModConfig& cfg, std::vector<std::string>& debug)
{
	bool gettingWet = false;

	float seconds = dtMs / 1000.0f;

	float submerged = ENTITY::GET_ENTITY_SUBMERGED_LEVEL(playerPed);
	if (ENTITY::GET_ENTITY_SUBMERGED_LEVEL(playerPed) > state.wetness.Value()) {
		//Not setting if in water. Standing up to ankles while soaked would still allow torso to dry down.
		gettingWet = true;
	}
	state.wetness.Set(std::max(state.wetness.Value(), ENTITY::GET_ENTITY_SUBMERGED_LEVEL(playerPed)));

	
	float rainIntensity = MISC::GET_RAIN_LEVEL();
	if (rainIntensity > 0) {
		
		//Behind an interval as it can do some expensive raycasting to looks for cover
		if (MISC::GET_GAME_TIMER() >= state.nextCoverProbe) {
			state.rainExposure = RainExposure(playerPed);
			state.nextCoverProbe = MISC::GET_GAME_TIMER() + COVER_PROBE_INTERVAL_MS;
		}
		if (state.rainExposure == 0.0f) {
			gettingWet = false;
		} else {
			state.wetness.Add(rainIntensity * state.fullRainWetPerSecond * state.rainExposure * seconds);
		}
	}



	debug.push_back(std::format("Wetness: {}", state.wetness.Value()));
	debug.push_back(std::format("Submerged Level: {}", submerged));
	debug.push_back(std::format("Rain: {}", rainIntensity));
	debug.push_back(std::format("Rain exposure: {}", state.rainExposure));
}

