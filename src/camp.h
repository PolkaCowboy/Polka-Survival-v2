#pragma once

#include "../inc/sdk.h"

#include <vector>
#include <string>

namespace Camp {

	enum class Phase { Idle, Watching, WaitForBlack, Finishing, FailWaitForPlacement, FailWaitForCampGone, FailHoldBlack };

	struct State {
		Phase phase = Phase::Idle;
		int phaseStart = 0; // Timer when current phase has started
		int previousCampCount = -1; // How many camps existsed previous frame
		int startCount = 0;
		int setupMinutes = 0; // In game minutes for setup attempt
		const char* failReason = nullptr;
		int skippedMs = 0;
		bool kitBlocked[2] = {}; // KIT_CAMP, KIT_CAMP_SIMPLE
	};

	struct Settings {
		bool enabled = true; //false = vanilla camp setup
		bool oneCampAtATime = true; //Block the camp kits while a camp stands
		float setupMinutesBest = 30.0f; //Game minutes with full cores and health ring
		float setupMinutesWorst = 75.0f; //Game minutes with all three empty
		float failChanceScale = 1.0f; //Multiplies every failure chance. 0 = never fails
	};

	// Each attempt's setup time varies by this much either way. The range itself is in the INI.
	constexpr float SETUP_RANDOM_MIN = 0.9f;
	constexpr float SETUP_RANDOM_MAX = 1.1f;

	//Bonus applied to core calculation when having gold outer ring. (Slam a tonic to help build camp in emeregency)
	constexpr int GOLD_RING_BONUS = 25;

	//Failure gates and odds
	constexpr float HUNGER_CHANCE_RED = 0.1f;
	constexpr float HUNGER_CHANCE_EMPTY = 0.4f;
	constexpr float INJURY_RING_START = 0.4f;
	constexpr float INJURY_CHANCE_EMPTY = 0.8f;
	constexpr float FREEZING_SEVERITY_START = 0.3f;
	constexpr float FREEZING_CHANCE_MAX = 0.5f;
	constexpr float FREEZING_POWER = 2.0f;
	constexpr float RAIN_LEVEL_START = 0.3f;
	constexpr float RAIN_LEVEL_FULL = 0.7f;
	constexpr float RAIN_POWER = 2.0f;
	constexpr float RAIN_CHANCE_MAX = 0.5f;
	constexpr float WIND_SPEED_START = 10.0f;
	constexpr float WIND_SPEED_FULL = 40.0f;
	constexpr float WIND_CHANCE_MAX = 0.2f;

	// Hold screen back after failure to mock time passing
	constexpr int FAIL_HOLD_MS = 3000;
	constexpr int FAIL_FADE_IN_MS = 1500;

	void Init(const Settings& settings);
	bool Update(State& state, Player player, Ped playerPed, float hypothermiaSeverity, int now, std::vector<std::string>& debug);
}