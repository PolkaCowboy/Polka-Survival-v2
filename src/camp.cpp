#include "camp.h"
#include "log.h"
#include "ui.h"
#include "utils.h"

#include <algorithm>
#include <cmath>
#include <format>

using namespace Camp;

namespace {

	Camp::Settings g_settings;

	constexpr bool CAN_SHOW_DEBUG = true;
	// The last attempt's fail chances, kept for the debug overlay.
	std::string lastRoll[2];

	// This ignrores the time warping that happens trhoughtout the day and I don't care.,
	constexpr int REAL_MS_PER_GAME_MINUTE = 48 * 60 * 1000 / (24 * 60);

	struct FailCause {
		float chance;
		const char* message;
	};

	constexpr int CONTROL_WINDOW_MS = 250;

	constexpr int CORE_RED = 16;

	constexpr int STEP_TIMEOUT_MS = 20000;

	// player_camp ends itself when a force cleanup with these bits arrives; it checks
	// HAS_FORCE_CLEANUP_OCCURRED(555) at startup.
	// https://github.com/JayKoZa/RDR2-Decompiled-Scripts/blob/master/script_rel/player_camp.c
	constexpr int CAMP_CLEANUP_FLAGS = 555;

	int PlayerCampCount() {
		return SCRIPT::GET_NUMBER_OF_THREADS_RUNNING_THE_SCRIPT_WITH_THIS_HASH(MISC::GET_HASH_KEY("player_camp"));
	}

	int CampCoreValue(Ped ped, int core, int ring) {
		if (ATTRIBUTE::_IS_ATTRIBUTE_CORE_OVERPOWERED(ped, core)) {
			return 100;
		}
		int value = ATTRIBUTE::_GET_ATTRIBUTE_CORE_VALUE(ped, core);
		if (ATTRIBUTE::_IS_ATTRIBUTE_OVERPOWERED(ped, ring)) {
			value += GOLD_RING_BONUS;
		}
		return std::clamp(value, 0, 100);
	}

	float HungerFailureChance(int core) {
		if (core > CORE_RED) {
			return 0.0f;
		}
		return Utils::MapToRange(static_cast<float>(core), static_cast<float>(CORE_RED), 0.0f, HUNGER_CHANCE_RED, HUNGER_CHANCE_EMPTY);
	}

	float HealthRingFraction(Ped ped) {
		float ringMax = 50.0f * (ATTRIBUTE::GET_ATTRIBUTE_BASE_RANK(ped, PA_HEALTH) + 1);
		float ring = static_cast<float>(ENTITY::GET_ENTITY_HEALTH(ped) - ATTRIBUTE::_GET_ATTRIBUTE_CORE_VALUE(ped, ATTRIBUTE_CORE_HEALTH));
		return std::clamp(ring / ringMax, 0.0f, 1.0f);
	}

	void RollAttempt(Camp::State& state, Ped ped, float hypothermiaSeverity) {
		int healthCore = CampCoreValue(ped, ATTRIBUTE_CORE_HEALTH, PA_HEALTH);
		int staminaCore = CampCoreValue(ped, ATTRIBUTE_CORE_STAMINA, PA_STAMINA);
		float ring = HealthRingFraction(ped);

		float condition = (healthCore / 100.0f + staminaCore / 100.f + ring) / 3.0f;
		float minutes = Utils::MapToRange(condition, 1.0f, 0.0f, g_settings.setupMinutesBest, g_settings.setupMinutesWorst);
		state.setupMinutes = static_cast<int>(std::lround(minutes * MISC::GET_RANDOM_FLOAT_IN_RANGE(SETUP_RANDOM_MIN, SETUP_RANDOM_MAX)));

		float wind = PED::_GET_PED_BLACKBOARD_FLOAT(ped, "WindSpeed");
		FailCause causes[] = {
			{ Utils::MapToRange(ring, INJURY_RING_START, 0.0f, 0.0f, INJURY_CHANCE_EMPTY), "Your wounds made it too hard to get a fire going." },
			{ Utils::MapToRange(hypothermiaSeverity, FREEZING_SEVERITY_START, 1.0f, 0.0f, FREEZING_CHANCE_MAX, FREEZING_POWER), "Your fingers shook too much to get a fire going." },
			{ HungerFailureChance(healthCore), "Your lack of energy kept you from getting a fire going." },
			{ HungerFailureChance(staminaCore), "Your lack of energy kept you from getting a fire going." },
			{ Utils::MapToRange(MISC::GET_RAIN_LEVEL(), RAIN_LEVEL_START, RAIN_LEVEL_FULL, 0.0f, RAIN_CHANCE_MAX, RAIN_POWER), "The rain kept putting out your fire." },
			{ Utils::MapToRange(wind, WIND_SPEED_START, WIND_SPEED_FULL, 0.0f, WIND_CHANCE_MAX), "The wind kept blowing out your fire." }
		};
		for (FailCause& cause : causes) {
			cause.chance = std::clamp(cause.chance * g_settings.failChanceScale, 0.0f, 1.0f);
		}

		state.failReason = nullptr;
		for (const FailCause& cause : causes) {
			if (MISC::GET_RANDOM_FLOAT_IN_RANGE(0.0f, 1.0f) < cause.chance) {
				state.failReason = cause.message;
				break;
			}
		}

		if (CAN_SHOW_DEBUG) {
			float passChance = 1.0f;
			for (const FailCause& cause : causes) {
				passChance *= 1.0f - cause.chance;
			}
			lastRoll[0] = std::format("injury {:.1f}%  freezing {:.1f}%  hunger {:.1f}% / {:.1f}%  rain {:.1f}%  wind {:.1f}%",
				causes[0].chance * 100.0f, causes[1].chance * 100.0f, causes[2].chance * 100.0f, causes[3].chance * 100.0f, causes[4].chance * 100.0f, causes[5].chance * 100.0f);
			lastRoll[1] = std::format("fail chance {:.1f}%  {} min  -> {}", (1.0f - passChance) * 100.0f, state.setupMinutes,
				state.failReason ? state.failReason : "success");

		}

	}

	void SetPhase(Camp::State& state, Camp::Phase phase, int now) {
		state.phase = phase;
		state.phaseStart = now;
	}

	void SkipSetupTime(Camp::State& state) {
		CLOCK::ADD_TO_CLOCK_TIME(0, state.setupMinutes, 0);
		state.skippedMs = state.setupMinutes * REAL_MS_PER_GAME_MINUTE;
	}

	void FinishFailure(Camp::State& state, Player player, int now) {
		SkipSetupTime(state);
		CAMERA::SET_CINEMATIC_MODE_ACTIVE(FALSE);
		PLAYER::SET_PLAYER_CONTROL(player, TRUE, 0, FALSE);
		CAMERA::DO_SCREEN_FADE_IN(FAIL_FADE_IN_MS);
		UI::DisplaySubtitle(std::format("{} You tried for {} minutes.", state.failReason, state.setupMinutes).c_str());
		SetPhase(state, Camp::Phase::Idle, now);
	}

	const char* const CAMP_KITS[] = { "KIT_CAMP", "KIT_CAMP_SIMPLE" };

	constexpr int CAMP_RECORD_GLOBAL = 1357522;
	constexpr int CAMP_REMEMBERED_FULL = 11;
	constexpr int CAMP_REMEMBERED_BAREBONES = 12;

	bool GlobalIntSet(int index) {
		UINT64* slot = getGlobalPtr(index);
		return slot && *reinterpret_cast<int*>(slot) != 0;
	}

	bool VanillaRemembersCamp() {
		return GlobalIntSet(CAMP_RECORD_GLOBAL + CAMP_REMEMBERED_FULL) || GlobalIntSet(CAMP_RECORD_GLOBAL + CAMP_REMEMBERED_BAREBONES);
	}

	//Block Camp kit if a camp already exists
	void BlockCampKits(Camp::State& state, bool campStanding) {
		int inventory = INVENTORY::_INVENTORY_IS_USING_BACKUP_INVENTORY() ? 5 : 1;

		for (int i = 0; i < std::ssize(CAMP_KITS); i++) {
			Hash kit = MISC::GET_HASH_KEY(CAMP_KITS[i]);
			bool disabled = INVENTORY::_INVENTORY_IS_INVENTORY_ITEM_EQUIPPED(inventory, kit, FALSE) != FALSE;
			if (campStanding) {
				if (!disabled) {
					INVENTORY::_INVENTORY_DISABLE_ITEM(inventory, kit, 0);
					state.kitBlocked[i] = true;
				}
			}
			else if (state.kitBlocked[i]) {
				INVENTORY::_INVENTORY_ENABLE_ITEM(inventory, kit);
				state.kitBlocked[i] = false;
			}
		}

	}

}

void Camp::Init(const Settings& settings) {
	g_settings = settings;
}

bool Camp::Update(State& state, Player player, Ped playerPed, float hypothermiaSeverity, int now, std::vector<std::string>& debug) {
	int count = PlayerCampCount();
	bool havePlayerControl = PLAYER::IS_PLAYER_CONTROL_ON(player);
	bool started = state.previousCampCount >= 0 && count > state.previousCampCount;
	state.previousCampCount = count;
	state.skippedMs = 0;

	//Prevent "cheating" by setting up or warping to a camp while holding an animal that can be cooked whole by disabling the "Cook" button.
	Entity carried = PED::_GET_FIRST_ENTITY_PED_IS_CARRYING(playerPed);
	if (ENTITY::DOES_ENTITY_EXIST(carried) && ENTITY::IS_ENTITY_A_PED(carried) && ENTITY::GET_IS_ANIMAL(carried)) {
		PAD::DISABLE_CONTROL_ACTION(0, MISC::GET_HASH_KEY("INPUT_CONTEXT_B"), FALSE);
	}

	BlockCampKits(state, g_settings.oneCampAtATime && (count > 0 || VanillaRemembersCamp()));
	if (!g_settings.enabled) {
		return false;
	}

	int inPhase = now - state.phaseStart;

	switch (state.phase) {
		//No camp, or in a camp that already exists. Move to watching when a new camp script is detected.
	case Phase::Idle:
		if (started) {
			state.startCount = count;
			SetPhase(state, Phase::Watching, now);
		}
		break;

		//New camp script detected. If we lose control it is a new camp set up and not a respawn.
	case Phase::Watching:
		if (!havePlayerControl) {
			RollAttempt(state, playerPed, hypothermiaSeverity);
			SetPhase(state, Phase::WaitForBlack, now);
		}
		else if (inPhase > CONTROL_WINDOW_MS) {
			SetPhase(state, Phase::Idle, now);
		}
		break;

	case Phase::WaitForBlack:
		if (CAMERA::IS_SCREEN_FADED_OUT()) {
			if (state.failReason) {
				SetPhase(state, Phase::FailWaitForPlacement, now);
			}
			else {
				SkipSetupTime(state);
				SetPhase(state, Phase::Finishing, now);
			}
		}
		else if (havePlayerControl || inPhase > STEP_TIMEOUT_MS) {
			SetPhase(state, Phase::Idle, now);
		}
		break;

	case Phase::Finishing:
		if (havePlayerControl) {
			if (count > 0) {
				UI::DisplaySubtitle(std::format("You got a fire going in {} minutes", state.setupMinutes).c_str());
			}
			SetPhase(state, Phase::Idle, now);
		}
		else if (inPhase > STEP_TIMEOUT_MS) {
			SetPhase(state, Phase::Idle, now);
		}
		break;

		//Let the camp script move the character to the campsite before failing
	case Phase::FailWaitForPlacement:
		if (!CAMERA::_IS_IN_CINEMATIC_MODE() || inPhase > STEP_TIMEOUT_MS) {
			PLAYER::FORCE_CLEANUP_FOR_ALL_THREADS_WITH_THIS_NAME("player_camp", CAMP_CLEANUP_FLAGS);
			CAMERA::DO_SCREEN_FADE_OUT(0);
			SetPhase(state, Phase::FailWaitForCampGone, now);
		}
		break;

	case Phase::FailWaitForCampGone:
		if (count < state.startCount || inPhase > STEP_TIMEOUT_MS) {
			CAMERA::SET_CINEMATIC_MODE_ACTIVE(FALSE);
			PLAYER::SET_PLAYER_CONTROL(player, FALSE, 0, FALSE);
			CAMERA::DO_SCREEN_FADE_OUT(0);
			SetPhase(state, Phase::FailHoldBlack, now);
		}
		break;

	case Phase::FailHoldBlack:
		if (!CAMERA::IS_SCREEN_FADED_OUT() && !CAMERA::IS_SCREEN_FADING_OUT()) {
			CAMERA::DO_SCREEN_FADE_OUT(0);
		}
		if (inPhase >= FAIL_HOLD_MS) {
			FinishFailure(state, player, now);
		}
		break;
	}


	if (CAN_SHOW_DEBUG && !lastRoll[0].empty()) {
		debug.push_back("CAMP LAST ROLL");
		debug.push_back(lastRoll[0]);
		debug.push_back(lastRoll[1]);
		debug.push_back("");
	}


	return state.phase != Phase::Idle;
}