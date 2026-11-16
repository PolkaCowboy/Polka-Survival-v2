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

	// Max dry down rate when next to a heat source. 
	const float MAX_HEAT_DRY_FACTOR = 4.0f;
	// How much rain a fire blocks when you're within HEAT_FULL_RADIUS. 1 = all of it, 0 = none.
	const float FIRE_RAIN_PROTECTION = 1.0f;

	//Resting at fire effects

	// Resting near a fire increases drying rate. (Time saving shit) 
	const float REST_DRYDOWN_BOOST = 4.0f;

	const char* const FIRE_REST_SCENARIOS[] = {
	"WORLD_PLAYER_CAMP_FIRE_KNEEL1", "WORLD_PLAYER_CAMP_FIRE_KNEEL2", "WORLD_PLAYER_CAMP_FIRE_KNEEL3", "WORLD_PLAYER_CAMP_FIRE_KNEEL4",
	"WORLD_PLAYER_CAMP_FIRE_SIT", "WORLD_PLAYER_CAMP_FIRE_SIT_MALE_A", "WORLD_PLAYER_CAMP_FIRE_SQUAT", "WORLD_PLAYER_CAMP_FIRE_SQUAT_MALE_A",
	"WORLD_PLAYER_DYNAMIC_CAMP_FIRE_KNEEL_ARTHUR", "WORLD_PLAYER_DYNAMIC_KNEEL_KNIFE", "PROP_PLAYER_SLEEP_TENT_A_FRAME" };

	bool IsRestingAtFire(Ped ped, float currentFireWeight)
	{
		if (currentFireWeight <= 0.0f || !PED::IS_PED_USING_ANY_SCENARIO(ped)) {
			return false;
		}

		for (const char* scenario : FIRE_REST_SCENARIOS)
		{
			if (PED::IS_PED_USING_SCENARIO_HASH(ped, MISC::GET_HASH_KEY(scenario)))
				return true;
		}

		return false;
	}

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

		if (INTERIOR::IS_INTERIOR_SCENE() || PED::IS_PED_USING_SCENARIO_HASH(playerPed, MISC::GET_HASH_KEY("PROP_PLAYER_SLEEP_TENT_A_FRAME"))) {
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
	float seconds = dtMs / 1000.0f;
	Vector3 pos = ENTITY::GET_ENTITY_COORDS(playerPed, TRUE, FALSE);
	float playerTemp = MISC::_GET_TEMPERATURE_AT_COORDS(pos.x, pos.y, pos.z);
	float effectiveTemp = playerTemp;

	//If we are near a fire or heat source increase the effective temp used in dry calculations to accelerate drying.
	state.fireWeight = 0.0f;
	Vector3 firePos{};
	if (FIRE::GET_CLOSEST_FIRE_POS(&firePos, pos.x, pos.y, pos.z)) {
		float closestFireDistance = Utils::Distance(firePos, pos);
		float fireTemp = MISC::_GET_TEMPERATURE_AT_COORDS(firePos.x, firePos.y, firePos.z);
		state.fireWeight = Utils::MapToRange(closestFireDistance, HEAT_FADE_RADIUS, HEAT_FULL_RADIUS, 0.0f, 1.0f);

		effectiveTemp = std::lerp(playerTemp, fireTemp, state.fireWeight);
	}

	//Establish drydown floors. 
	float submergedLevel = ENTITY::GET_ENTITY_SUBMERGED_LEVEL(playerPed);
	float sweatTarget = exertion * cfg.MaxSweatAccumulation;
	float wetFloor = std::max(submergedLevel, sweatTarget);

	//Ooops. We fell into water.
	if (submergedLevel > state.wetness.Value()) {
		state.wetness.Set(submergedLevel);
	}

	float rainIntensity = MISC::GET_RAIN_LEVEL();
	if (rainIntensity > 0) {
		//Behind an interval as it can do some expensive raycasting to looks for cover
		if (MISC::GET_GAME_TIMER() >= state.nextCoverProbe) {
			state.rainExposure = RainExposure(playerPed);
			state.nextCoverProbe = MISC::GET_GAME_TIMER() + COVER_PROBE_INTERVAL_MS;
		}

		float rainRate = rainIntensity * state.fullRainWetPerSecond * state.rainExposure;
		rainRate *= 1.0f - state.fireWeight * FIRE_RAIN_PROTECTION;
		state.wetness.Add(rainRate * seconds);
	}


	//Exertion: Climb towards the sweat target.
	float sweatAccumulation = 0.0f;
	if (sweatTarget > state.wetness.Value()) {
		sweatAccumulation = state.nominalSweatPerSecond * exertion;

		//Clothing appropriatness. 0 = too cold. 50 = ok. 100 = too hot.
		if (ATTRIBUTE::GET_ATTRIBUTE_POINTS(playerPed, SA_BODYHEAT) == 100) {
			sweatAccumulation *= 1.15f;
		}

		//Sweat faster when above the nominal temperature
		float tempModifier = Utils::MapToRange(playerTemp, cfg.NominalTemperatureC, cfg.ExtremeTemperatureC, 1.0f, 2.0f);
		sweatAccumulation *= tempModifier;

		//Sweat up. Hoes down.
		state.wetness.Add(std::min(sweatAccumulation * seconds, sweatTarget - state.wetness.Value()));

		debug.push_back(std::format("Temp: {:.1f}F, Mod {}", Utils::CelsiusToFahrenheit(playerTemp), tempModifier));
		debug.push_back(std::format("Sweat accumulation: {}", sweatAccumulation));
	}


	//Let's dry!

	float dryDown = 0.0f;

	//approach 0 as the temp approaches freezing
	if (effectiveTemp > cfg.FreezingTemperatureC && effectiveTemp <= cfg.NominalTemperatureC) {
		dryDown = Utils::MapToRange(effectiveTemp, cfg.FreezingTemperatureC, cfg.NominalTemperatureC, 0.0f, 1.0f);
	}
	//Increase drydown as the temp gets hotter
	else if (effectiveTemp > cfg.NominalTemperatureC) {
		dryDown = Utils::MapToRange(effectiveTemp, cfg.NominalTemperatureC, cfg.ExtremeTemperatureC, 1.0f, std::lerp(2.0f, MAX_HEAT_DRY_FACTOR, state.fireWeight));
	}

	// Always allow drying drying down to .8, accelerating it a little. (Simulate shaking off, skin evaptoarting faster or something)
	if (state.wetness.Value() > 0.8f) {
		dryDown += 0.5f;
	}

	//Dry slower in wet environment.
	float soakPenalty = std::lerp(1.0f, 0.2f, PED::_GET_PED_BLACKBOARD_FLOAT(playerPed, "Soak"));

	//A heat source will overpower the soaked environment penalty.
	soakPenalty = std::lerp(soakPenalty, 1.0f, state.fireWeight);

	dryDown *= soakPenalty;

	if (IsRestingAtFire(playerPed, state.fireWeight)) {
		dryDown *= REST_DRYDOWN_BOOST;
	}

	float wetToRemoveThisTick = dryDown * state.nominalDrydownPerSecond * seconds;

	// Don't dry below the sweat target or submerged level.
	float floor = std::min(wetFloor, state.wetness.Value());
	state.wetness.Set(std::max(state.wetness.Value() - wetToRemoveThisTick, floor));


	debug.push_back(std::format("Effective Temp: {:.1f}, Fire Weight: {:.3f}", Utils::CelsiusToFahrenheit(effectiveTemp), state.fireWeight));
	debug.push_back(std::format("DryDownModifier: {:.3f}, cfg {}", dryDown, cfg.MinTimeToDry));
	debug.push_back(std::format("Wetness: {:.3f}  sweat target {:.3f} (exertion {:.2f})", state.wetness.Value(), sweatTarget, exertion));
	debug.push_back(std::format("Submerged Level: {}", submergedLevel));
	debug.push_back(std::format("Rain: {}, Snow {}", rainIntensity, MISC::GET_SNOW_LEVEL()));
	debug.push_back(std::format("Rain exposure: {}", state.rainExposure));

}

