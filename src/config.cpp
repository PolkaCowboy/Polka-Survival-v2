#include "config.h"
#include "paths.h"
#include "../inc/SimpleIni.h"

#include <filesystem>
#include <format>

namespace fs = std::filesystem;

namespace
{
	const std::string& ConfigPath()
	{
		static const std::string path = (fs::path(ModDataDir()) / "config.ini").string();
		return path;
	}
}

ModConfig LoadConfig()
{
	ModConfig cfg; // defaults from config.h

	CSimpleIniA ini;
	ini.SetUnicode();

	if (!fs::exists(ConfigPath()))
	{
		// First run: write the defaults out so there is something to edit.
		ini.SetBoolValue("MISC", "DISABLE_EAGLE_EYE", cfg.DisableEagleEye);
		ini.SetBoolValue("MISC", "DISABLE_MOD", cfg.DisableMod, 
			";Turn off the mod");

		ini.SetValue("DEAD_EYE", "DURATION_COST", std::format("{:g}", cfg.DeadEyeDurationCost).c_str(), ";Game default duration cost is roughly 4.27.");
		ini.SetValue("DEAD_EYE", "ACTIVATION_COST", std::format("{:g}", cfg.DeadEyeActivationCoste).c_str());

		ini.SetLongValue("HEALTH", "HEALING_THRESHOLD", cfg.HealingThreshold,
			";Percent that the health core needs to be above before the health ring heals.\n"
			";Mod default = 50");
		ini.SetValue("HEALTH", "CORE_DRAINS_PER_DAY", std::format("{:g}", cfg.HealthCoreDrainsPerDay).c_str(),
			";How many times the health & stamina core should fully drain per in-game day.\n"
			";Mod default = 2");
		ini.SetBoolValue("HEALTH", "STAMINA_HURTS_HEALTH", cfg.StaminaHurtsHealth, ";Drain health core if stamina core goes red");
		
		ini.SetValue("HURTING", nullptr, nullptr,
			";If health core turns red (16% or lower) how long will it take the health ring to completely drain from max.\n"
			";HURTING_TIME_TO_DIE_MAX is when Core is as 16 %, HURTING_TIME_TO_DIE_MIN when core is an 0%.\n"
			";Scales between min/max for intermediate values.\n"
			";Value in real world minutes.");
		ini.SetValue("HURTING", "HURTING_TIME_TO_DIE_MIN", std::format("{:g}", cfg.HurtingTimeToDieMin).c_str(),
			";Mod default 1.5 minutes");
		ini.SetValue("HURTING", "HURTING_TIME_TO_DIE_MAX", std::format("{:g}", cfg.HurtingTimeToDieMax).c_str(),
			";Mod default 3 minutes");

		ini.SetValue("HYPOTHERMIA", nullptr, nullptr, "; Times are in real world seconds.");
		ini.SetBoolValue("HYPOTHERMIA", "ENABLE", cfg.EnableHypothermia);
		ini.SetValue("HYPOTHERMIA", "TIME_TO_GET_WET", std::format("{:g}", cfg.TimeToGetWet).c_str(), 
			";How long you need to be in water before gettiong soaked.");
		ini.SetValue("HYPOTHERMIA", "TIME_TO_DRY", std::format("{:g}", cfg.TimeToDry).c_str());

		ini.SaveFile(ConfigPath().c_str(), false); // false = no UTF-8 BOM
		return cfg;
	}

	// Exists but unreadable: use the defaults rather than clobbering the file.
	if (ini.LoadFile(ConfigPath().c_str()) < 0)
		return cfg;

	cfg.DisableEagleEye = ini.GetBoolValue("MISC", "DISABLE_EAGLE_EYE", cfg.DisableEagleEye);
	cfg.DisableMod = ini.GetBoolValue("MISC", "DISABLE_MOD", cfg.DisableMod);

	cfg.DeadEyeDurationCost = static_cast<float>(ini.GetDoubleValue("DEAD_EYE", "DURATION_COST", cfg.DeadEyeDurationCost));
	cfg.DeadEyeActivationCoste = static_cast<float>(ini.GetDoubleValue("DEAD_EYE", "ACTIVATION_COST", cfg.DeadEyeActivationCoste));

	cfg.HealingThreshold = static_cast<int>(ini.GetLongValue("HEALTH", "HEALING_THRESHOLD", cfg.HealingThreshold));
	cfg.HealthCoreDrainsPerDay = static_cast<float>(ini.GetDoubleValue("HEALTH", "CORE_DRAINS_PER_DAY", cfg.HealthCoreDrainsPerDay));
	cfg.StaminaHurtsHealth = ini.GetBoolValue("HEALTH", "STAMINA_HURTS_HEALTH", cfg.StaminaHurtsHealth);

	cfg.HurtingTimeToDieMin = static_cast<float>(ini.GetDoubleValue("HURTING", "HURTING_TIME_TO_DIE_MIN", cfg.HurtingTimeToDieMin));
	cfg.HurtingTimeToDieMax = static_cast<float>(ini.GetDoubleValue("HURTING", "HURTING_TIME_TO_DIE_MAX", cfg.HurtingTimeToDieMax));

	cfg.EnableHypothermia = ini.GetBoolValue("HYPOTHERMIA", "ENABLE", cfg.EnableHypothermia);
	cfg.TimeToGetWet = static_cast<float>(ini.GetDoubleValue("HYPOTHERMIA", "TIME_TO_GET_WET", cfg.TimeToGetWet));
	cfg.TimeToDry = static_cast<float>(ini.GetDoubleValue("HYPOTHERMIA", "TIME_TO_DRY", cfg.TimeToDry));

	return cfg;
}
