#include "wetness.h"
#include "utils.h"

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

	//Fire drying stuff

	//Within this distance of a fire, its drying is at full strength.
	const float HEAT_FULL_RADIUS = 3.0f;
	//Fire drying fades out between HEAT_FULL_RADIUS and this distance.
	const float HEAT_FADE_RADIUS = 6.0f;
	//How much faster than normal a fire dries you at full strength.
	const float MAX_HEAT_DRY_FACTOR = 4.0f;

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
	state.nominalSweatPerSecond = RatePerSecond(cfg.NominalMaxSweatTime) * cfg.MaxSweatAccumulation;
}

void WetnessUpdate(WetnessState& state, Ped playerPed, float exertion, int dtMs, const ModConfig& cfg, std::vector<std::string>& debug)
{
	bool gettingWet = false;

	float seconds = dtMs / 1000.0f;

	float submergedLevel = ENTITY::GET_ENTITY_SUBMERGED_LEVEL(playerPed);
	if (submergedLevel > state.wetness.Value()) {
		//Only set if in deepr water than current wetness. Standing up to ankles while soaked would still allow torso to dry down.
		gettingWet = true;
	}
	state.wetness.Set(std::max(state.wetness.Value(), submergedLevel));

	float rainIntensity = MISC::GET_RAIN_LEVEL();
	if (rainIntensity > 0) {
		//Behind an interval as it can do some expensive raycasting to looks for cover
		if (MISC::GET_GAME_TIMER() >= state.nextCoverProbe) {
			state.rainExposure = RainExposure(playerPed);
			state.nextCoverProbe = MISC::GET_GAME_TIMER() + COVER_PROBE_INTERVAL_MS;
		}
		if (state.rainExposure > 0.0f) {
			state.wetness.Add(rainIntensity * state.fullRainWetPerSecond * state.rainExposure * seconds);
			gettingWet = true;
		}
	}

	//Sweat accumulation
	Vector3 pos = ENTITY::GET_ENTITY_COORDS(playerPed, TRUE, FALSE);
	float playerTemp = MISC::_GET_TEMPERATURE_AT_COORDS(pos.x, pos.y, pos.z);

	// Sweat pulls wetness up towards a target set by how hard the player is working. Drying below
	// uses the same target as a floor, so working keeps clothes damp and stopping lets them dry.
	float sweatTarget = std::clamp(exertion, 0.0f, 1.0f) * cfg.MaxSweatAccumulation;
	float sweatRoom = sweatTarget - state.wetness.Value();
	float sweatAccumulation = 0.0f;
	if (sweatRoom > 0) {
		sweatAccumulation = state.nominalSweatPerSecond * exertion;

		//Clothing appropriatness. 0 = too cold. 50 = ok. 100 = too hot.
		if (ATTRIBUTE::GET_ATTRIBUTE_POINTS(playerPed, SA_BODYHEAT) == 100) {
			sweatAccumulation *= 1.15f;
		}

		//Sweat faster beyond the nominal temperature
		float tempModifier = Utils::MapToRange(playerTemp, cfg.NominalTemperatureC, cfg.ExtremeTemperatureC, 1.0f, 2.0f);
		sweatAccumulation *= tempModifier;

		//Sweat up. Hoes down.
		state.wetness.Add(std::min(sweatAccumulation * seconds, sweatRoom));

		debug.push_back(std::format("Temp: {:.1f}F, Mod {}", Utils::CelsiusToFahrenheit(playerTemp), tempModifier));
		debug.push_back(std::format("Sweat accumulation: {}", sweatAccumulation));
	}


	//If we are near a fire or heat source blend its heat a little further out than the game does naturally to accelerate drying.
	float fireDry = 0.0f;
	Vector3 firePos{};
	if (FIRE::GET_CLOSEST_FIRE_POS(&firePos, pos.x, pos.y, pos.z)) {
		float fireDistance = Utils::Distance(firePos, pos);
		float fireWeight = Utils::MapToRange(fireDistance, HEAT_FADE_RADIUS, HEAT_FULL_RADIUS, 0.0f, 1.0f);
		fireDry = (MAX_HEAT_DRY_FACTOR - 1.0f) * fireWeight;
		gettingWet = false;
	}

	//Let's dry!
	if (!gettingWet) {

		//0.0f - 1.0f dry time
		float dryDownModifier = 0.0f;

		//approach 0 as the temp approaches freezing
		if (playerTemp > cfg.FreezingTemperatureC && playerTemp <= cfg.NominalTemperatureC) {
			dryDownModifier = Utils::MapToRange(playerTemp, cfg.FreezingTemperatureC, cfg.NominalTemperatureC, 0.0f, 1.0f);
		}
		//Increase drydown as the temp gets hotter
		else if (playerTemp > cfg.NominalTemperatureC) {
			dryDownModifier = Utils::MapToRange(playerTemp, cfg.NominalTemperatureC, cfg.ExtremeTemperatureC, 1.0f, 2.0f);
		}

		// Always allow drying drying down to .8, accelerating it a little. (Simulate shaking off, skin evaptoarting faster or something)
		if (state.wetness.Value() > 0.8f) {
			dryDownModifier += 0.5f;
		}

		//Dry slower in wet environment.
		dryDownModifier *= std::lerp(1.0f, 0.2f, PED::_GET_PED_BLACKBOARD_FLOAT(playerPed, "Soak"));

		debug.push_back(std::format("DryDownModifier: {:.3f}, cfg {}", dryDownModifier, cfg.MinTimeToDry));
		debug.push_back(std::format("Fire dry: {:.3f}", fireDry));

		float wetToRemoveThisTick = (fireDry + dryDownModifier) * state.nominalDrydownPerSecond * seconds;

		// Don't dry below the sweat target. min() so the floor itself never adds wetness.
		float floor = std::min(sweatTarget, state.wetness.Value());
		state.wetness.Set(std::max(state.wetness.Value() - wetToRemoveThisTick, floor));
		
	}
	

	debug.push_back(std::format("Wetness: {:.3f}  sweat target {:.3f} (exertion {:.2f})", state.wetness.Value(), sweatTarget, exertion));
	debug.push_back(std::format("Submerged Level: {}", submergedLevel));
	debug.push_back(std::format("Rain: {}", rainIntensity));
	debug.push_back(std::format("Rain exposure: {}", state.rainExposure));
	
	
}

