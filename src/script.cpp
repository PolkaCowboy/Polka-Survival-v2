#include "script.h"
#include "keyboard.h"
#include "log.h"
#include "config.h"
#include "ui.h"
#include "coredrain.h";
#include "utils.h"


#include <format>
#include <vector>
#include <string>

void ScriptMain()
{
	// All the running timers and flags
	CoreDrainState coreDrainSate;

	ModConfig g_config = LoadConfig();
	std::vector<std::string> debugMessages;
	bool g_showOverlay = true;

	//Set up states
	CoreDrainInit(coreDrainSate, g_config, MISC::GET_GAME_TIMER());

	if (g_config.DisableMod)
		return;
	else
		UI::DisplaySubtitle("Polka Survival Activated");

	

	if (g_config.enableTextLogging) {
		Log::Init();
		Log::Write(std::format("ScriptMain started"));
	}

	while (true)
	{
		debugMessages.clear();

		Player player = PLAYER::PLAYER_ID();
		Ped playerPed = PLAYER::PLAYER_PED_ID();

		CoreDrainUpdate(coreDrainSate, playerPed, MISC::GET_GAME_TIMER(), debugMessages);

		/* Monitor health */
		debugMessages.push_back(std::format("h: {}, s: {}, d: {}",
			ATTRIBUTE::_GET_ATTRIBUTE_CORE_VALUE(playerPed, ATTRIBUTE_CORE_HEALTH),
			ATTRIBUTE::_GET_ATTRIBUTE_CORE_VALUE(playerPed, ATTRIBUTE_CORE_STAMINA),
			ATTRIBUTE::_GET_ATTRIBUTE_CORE_VALUE(playerPed, ATTRIBUTE_CORE_DEADEYE)
		));

		if (g_config.DisableEagleEye) {
			PLAYER::_ENABLE_EAGLEEYE(player, false);
		}

		if (IsKeyJustUp(VK_F19))
			g_showOverlay = !g_showOverlay;

		if (g_showOverlay) {
			UI::DrawAllDebug(debugMessages);
		}

		WAIT(0);
	}
}
