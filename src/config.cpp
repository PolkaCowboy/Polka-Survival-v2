#include "config.h"
#include "paths.h"
#include "../inc/SimpleIni.h"

#include <filesystem>
#include <format>

namespace fs = std::filesystem;

ModConfig LoadConfig()
{
	ModConfig cfg; // defaults from config.h

	const std::string path = (fs::path(ModDataDir()) / "config.ini").string();

	CSimpleIniA ini;
	ini.SetUnicode();

	if (!fs::exists(path))
	{
		// First run: write the defaults out so there is something to edit.
		ini.SetValue("General", "Message", cfg.message.c_str(), "; string");
		ini.SetValue("General", "Modifier", std::format("{:g}", cfg.modifier).c_str(), "; float");
		ini.SetLongValue("General", "MaxCount", cfg.maxCount, "; int");
		ini.SetBoolValue("General", "Enabled", cfg.enabled, "; bool - accepts true/false, yes/no, on/off, 1/0");

		ini.SetValue("Debug", nullptr, nullptr, "; Debug settings. Remove on release\n");
		ini.SetBoolValue("Debug", "EnableTextLogging", cfg.enableTextLogging);

		ini.SetValue("Keys", nullptr, nullptr,
			"; Virtual-key codes in hex. 0x82 = F19.\n"
			"; Full list: https://learn.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes");
		ini.SetLongValue("Keys", "ToggleKey", cfg.toggleKey, nullptr, true); // true = write as hex

		ini.SaveFile(path.c_str(), false); // false = no UTF-8 BOM
		return cfg;
	}

	// Exists but unreadable: use the defaults rather than clobbering the file.
	if (ini.LoadFile(path.c_str()) < 0)
		return cfg;

	cfg.message = ini.GetValue("General", "Message", cfg.message.c_str());
	cfg.modifier = static_cast<float>(ini.GetDoubleValue("General", "Modifier", cfg.modifier));
	cfg.maxCount = static_cast<int>(ini.GetLongValue("General", "MaxCount", cfg.maxCount));
	cfg.enabled = ini.GetBoolValue("General", "Enabled", cfg.enabled);
	cfg.enableTextLogging = ini.GetBoolValue("Debug", "EnableTextLogging", cfg.enableTextLogging);
	cfg.toggleKey = static_cast<DWORD>(ini.GetLongValue("Keys", "ToggleKey", cfg.toggleKey));

	return cfg;
}
