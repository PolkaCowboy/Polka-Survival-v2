#include "script.h"
#include "keyboard.h"
#include "log.h"
#include "config.h"
#include "ui.h"

#include <format>
#include <vector>
#include <string>

void ScriptMain()
{

	ModConfig g_config = LoadConfig();
	std::vector<std::string> debugMessages;
	bool g_showOverlay = true;
	
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
		
		if (IsKeyJustUp(VK_F19))
			g_showOverlay = !g_showOverlay;

		if (g_showOverlay) {
			UI::DrawAllDebug(debugMessages);
		}

		WAIT(0);
	}
}
