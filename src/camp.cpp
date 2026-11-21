#include "camp.h"
#include "log.h"

#include <format>

namespace {
	constexpr int CONTROL_WINDOW_MS = 250;

	int PlayerCampCount() {
		return SCRIPT::GET_NUMBER_OF_THREADS_RUNNING_THE_SCRIPT_WITH_THIS_HASH(MISC::GET_HASH_KEY("player_camp"));
	}

	void SetPhase(CampState& state, CampPhase phase, int now) {
		state.phase = phase;
		state.phaseStart = now;
	}
}

bool UpdateCamp(CampState& state, Player player, int now, std::vector<std::string>& debug) {
	int count = PlayerCampCount();
	bool havePlayerControl = PLAYER::IS_PLAYER_CONTROL_ON(player);
	bool started = state.previousCampCount >= 0 && count > state.previousCampCount;
	state.previousCampCount = count;
	int inPhase = now - state.phaseStart;

	switch (state.phase) {
	//No camp, or in a camp that already exists. Move to watching when a new camp script is detected.
	case CampPhase::Idle:
		if (started) {
			SetPhase(state, CampPhase::Watching, now);
		}
		break;

	//New camp script detected. If we lose control it is a new camp set up and not a respawn.
	case CampPhase::Watching:
		if (!havePlayerControl) {
			SetPhase(state, CampPhase::SettingUp, now);
		}
		else if (inPhase > CONTROL_WINDOW_MS) {
			SetPhase(state, CampPhase::Idle, now);
		}
		break;

	case CampPhase::SettingUp:
		//New camp has been set up and player control returned
		if (havePlayerControl) {
			SetPhase(state, CampPhase::Idle, now);
		}
		break;
	}

	return state.phase != CampPhase::Idle;
}