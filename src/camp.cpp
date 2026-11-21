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

	//Block Camp kit if one already exists
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