#include "script.h"
#include "keyboard.h"
#include "log.h"
#include "config.h"

#include <format>

namespace
{
	bool g_enabled = false;
	ModConfig g_config;

	void OnTick()
	{
		//Mod shit here
	}
}

void ScriptMain()
{

	g_config = LoadConfig();

	if (g_config.enableTextLogging) {
		Log::Init();
		Log::Write(std::format("ScriptMain started"));
	}

	while (true)
	{
		OnTick();
		WAIT(0);
	}
}
