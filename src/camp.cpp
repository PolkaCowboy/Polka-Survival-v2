#include "camp.h"
#include "log.h"
#include "ui.h"

#include <format>

namespace {
	constexpr int CONTROL_WINDOW_MS = 250;

	// In game time to set up camp.
	// TODO: Make this change based on your cores & health
	constexpr int SETUP_MINUTES = 60;

	// In-game time burned up on a failure
	constexpr int FAIL_MINUTES = 30;

	constexpr int STEP_TIMEOUT_MS = 20000;

	// Hold screen back after failer to mock time passing
	constexpr int FAIL_HOLD_MS = 3000;
	constexpr int FAIL_FADE_IN_MS = 1500;

	// player_camp ends itself when a force cleanup with these bits arrives; it checks
	// HAS_FORCE_CLEANUP_OCCURRED(555) at startup.
	// https://github.com/JayKoZa/RDR2-Decompiled-Scripts/blob/master/script_rel/player_camp.c
	constexpr int CAMP_CLEANUP_FLAGS = 555;

	float FailChance() {
		return 0.5f;
	}

	int PlayerCampCount() {
		return SCRIPT::GET_NUMBER_OF_THREADS_RUNNING_THE_SCRIPT_WITH_THIS_HASH(MISC::GET_HASH_KEY("player_camp"));
	}

	void SetPhase(CampState& state, CampPhase phase, int now) {
		state.phase = phase;
		state.phaseStart = now;
	}

	void FinishFailure(CampState& state, Player player, int now) {
		CLOCK::ADD_TO_CLOCK_TIME(0, SETUP_MINUTES, 0);
		CAMERA::SET_CINEMATIC_MODE_ACTIVE(FALSE);
		PLAYER::SET_PLAYER_CONTROL(player, TRUE, 0, FALSE);
		CAMERA::DO_SCREEN_FADE_IN(FAIL_FADE_IN_MS);
		UI::DisplaySubtitle("You couldn't get a fire going");
		SetPhase(state, CampPhase::Idle, now);
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
	void BlockCampKits(CampState& state, bool campStanding) {
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

bool UpdateCamp(CampState& state, Player player, int now, std::vector<std::string>& debug) {
	int count = PlayerCampCount();
	bool havePlayerControl = PLAYER::IS_PLAYER_CONTROL_ON(player);
	bool started = state.previousCampCount >= 0 && count > state.previousCampCount;
	state.previousCampCount = count;

	BlockCampKits(state, count > 0 || VanillaRemembersCamp());

	int inPhase = now - state.phaseStart;

	switch (state.phase) {
		//No camp, or in a camp that already exists. Move to watching when a new camp script is detected.
	case CampPhase::Idle:
		if (started) {
			state.startCount = count;
			SetPhase(state, CampPhase::Watching, now);
		}
		break;

		//New camp script detected. If we lose control it is a new camp set up and not a respawn.
	case CampPhase::Watching:
		if (!havePlayerControl) {
			state.failing = MISC::GET_RANDOM_FLOAT_IN_RANGE(0.0f, 1.0f) < FailChance();
			SetPhase(state, CampPhase::WaitForBlack, now);
		}
		else if (inPhase > CONTROL_WINDOW_MS) {
			SetPhase(state, CampPhase::Idle, now);
		}
		break;

	case CampPhase::WaitForBlack:
		if (CAMERA::IS_SCREEN_FADED_OUT()) {
			if (state.failing) {
				SetPhase(state, CampPhase::FailWaitForPlacement, now);
			}
			else {
				CLOCK::ADD_TO_CLOCK_TIME(0, SETUP_MINUTES, 0);
				SetPhase(state, CampPhase::Finishing, now);
			}
		}
		else if (havePlayerControl || inPhase > STEP_TIMEOUT_MS) {
			SetPhase(state, CampPhase::Idle, now);
		}
		break;

	case CampPhase::Finishing:
		if (havePlayerControl) {
			if (count > 0) {
				UI::DisplaySubtitle(std::format("You got a fire going in {} minutes", SETUP_MINUTES).c_str());
			}
			SetPhase(state, CampPhase::Idle, now);
		}
		else if (inPhase > STEP_TIMEOUT_MS) {
			SetPhase(state, CampPhase::Idle, now);
		}
		break;

	//Let the camp script move the character to the campsite before failing
	case CampPhase::FailWaitForPlacement:
		if (!CAMERA::_IS_IN_CINEMATIC_MODE() || inPhase > STEP_TIMEOUT_MS) {
			PLAYER::FORCE_CLEANUP_FOR_ALL_THREADS_WITH_THIS_NAME("player_camp", CAMP_CLEANUP_FLAGS);
			CAMERA::DO_SCREEN_FADE_OUT(0);
			SetPhase(state, CampPhase::FailWaitForCampGone, now);
		}
		break;

	case CampPhase::FailWaitForCampGone:
		if (count < state.startCount || inPhase > STEP_TIMEOUT_MS) {
			CAMERA::SET_CINEMATIC_MODE_ACTIVE(FALSE);
			PLAYER::SET_PLAYER_CONTROL(player, FALSE, 0, FALSE);
			CAMERA::DO_SCREEN_FADE_OUT(0);
			SetPhase(state, CampPhase::FailHoldBlack, now);
		}
		break;

	case CampPhase::FailHoldBlack:
		if (!CAMERA::IS_SCREEN_FADED_OUT() && !CAMERA::IS_SCREEN_FADING_OUT()) {
			CAMERA::DO_SCREEN_FADE_OUT(0);
		}
		if (inPhase >= FAIL_HOLD_MS) {
			FinishFailure(state, player, now);
		}
		break;
	}
	return state.phase != CampPhase::Idle;
}