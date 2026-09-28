#include "config.h"
#include "paths.h"
#include "utils.h"
#include "../inc/SimpleIni.h"

#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <format>
#include <optional>

namespace fs = std::filesystem;

namespace
{
	const std::string& ConfigPath()
	{
		static const std::string path = (fs::path(ModDataDir()) / "config.ini").string();
		return path;
	}

	using Utils::FahrenheitToCelsius;
	using Utils::CelsiusToFahrenheit;

	// Parses a number followed by F or C, e.g. "75.5F", "30C", "-4 f", and returns it in Celsius.
	// No unit, or anything else after it, is rejected (nullopt) so a typo falls back to the
	// default instead of guessing which unit was meant.
	std::optional<float> ParseTemperatureC(const char* text)
	{
		if (!text)
			return std::nullopt;

		char* end = nullptr;
		float value = std::strtof(text, &end); // skips leading spaces; end = first char after the number
		if (end == text)
			return std::nullopt; // no number at all

		while (std::isspace(static_cast<unsigned char>(*end)))
			end++;
		char unit = static_cast<char>(std::toupper(static_cast<unsigned char>(*end)));
		if (unit != 'F' && unit != 'C')
			return std::nullopt;
		end++;
		while (std::isspace(static_cast<unsigned char>(*end)))
			end++;
		if (*end != '\0')
			return std::nullopt; // trailing junk, e.g. "30CC"

		return unit == 'F' ? FahrenheitToCelsius(value) : value;
	}

	// Reads a temperature key into `celsius`. Missing or unparseable keeps the current value.
	float ReadTemperatureC(const CSimpleIniA& ini, const char* section, const char* key, float celsius)
	{
		return ParseTemperatureC(ini.GetValue(section, key, nullptr)).value_or(celsius);
	}

	// Defaults are written out in Fahrenheit; {:g} rounds away float noise (32.2222C -> "90F").
	std::string FormatFahrenheit(float celsius)
	{
		return std::format("{:g}F", CelsiusToFahrenheit(celsius));
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
		ini.SetValue("DEAD_EYE", "ACTIVATION_COST", std::format("{:g}", cfg.DeadEyeActivationCost).c_str());

		ini.SetLongValue("HEALTH", "HEALING_THRESHOLD", cfg.HealingThreshold,
			";Percent that the health core needs to be above before the health ring heals.\n"
			";Mod default = 50");
		ini.SetValue("HEALTH", "CORE_DRAINS_PER_DAY", std::format("{:g}", cfg.HealthCoreDrainsPerDay).c_str(),
			";How many times the health core should fully drain per in-game day (48 real world minutes). 0 = off.\n"
			";Mod default = 2");

		ini.SetValue("STAMINA", "CORE_DRAINS_PER_DAY", std::format("{:g}", cfg.StaminaCoreDrainsPerDay).c_str(),
			";How many times the stamina core should fully drain per in-game day (48 real world minutes). 0 = off.\n"
			";Mod default = 2");
		ini.SetBoolValue("STAMINA", "STAMINA_HURTS_HEALTH", cfg.StaminaHurtsHealth, ";Drain health core if stamina core goes red");
		ini.SetValue("STAMINA", "IMPROPER_CLOTHING_DRAIN_RATE", std::format("{:g}", cfg.ImproperClothingStaminaDrainRate).c_str(),
			";Extra stamina core drain when the outfit doesn't suit the weather. 0.1 = 10% faster. 0 = off.\n"
			";Mod default = 0.1");
		
		ini.SetValue("HURTING", nullptr, nullptr,
			";If health core turns red (16% or lower) how long will it take the health ring to completely drain from max.\n"
			";HURTING_TIME_TO_DIE_MAX is when Core is as 16 %, HURTING_TIME_TO_DIE_MIN when core is an 0%.\n"
			";Scales between min/max for intermediate values.\n"
			";Value in real world minutes.");
		ini.SetValue("HURTING", "HURTING_TIME_TO_DIE_MIN", std::format("{:g}", cfg.HurtingTimeToDieMin).c_str(),
			";Mod default 1.5 minutes");
		ini.SetValue("HURTING", "HURTING_TIME_TO_DIE_MAX", std::format("{:g}", cfg.HurtingTimeToDieMax).c_str(),
			";Mod default 3 minutes");
		ini.SetBoolValue("HURTING", "HURTING_IS_LETHAL", cfg.HurtingIsLethal,
			";true = hurting can kill you. false = All health core drains will stop at 1hp leaving you very squishy while restoring your core.\n"
			";Mod default = true");

		ini.SetValue("HYPOTHERMIA", nullptr, nullptr, "; Times are in real world seconds.");
		ini.SetBoolValue("HYPOTHERMIA", "ENABLE", cfg.EnableHypothermia);
		ini.SetValue("HYPOTHERMIA", "MAX_SWEAT_ACCUMULATION", std::format("{:g}", cfg.MaxSweatAccumulation).c_str(),
			";How wet you can get from sweat alone. 0-1: 0.4 = 40% of fully soaked. 0 = no sweating.\n"
			";Mod default = 0.4");
		ini.SetValue("HYPOTHERMIA", "NOMINAL_MAX_SWEAT_TIME", std::format("{:g}", cfg.NominalMaxSweatTime).c_str(),
			";Seconds. How long it takes to reach MAX_SWEAT_ACCUMULATION at full exertion\n"
			";at the NOMINAL temperature. Too-warm clothing and hot weather speed this up.\n"
			";Mod default = 60");
		ini.SetValue("HYPOTHERMIA", "MIN_TIME_TO_GET_SOAKED", std::format("{:g}", cfg.MinTimeToGetSoaked).c_str(),
			";Seconds. How long it takes to get 100% soaked in a full rainstorm.\n"
			";Mod default = 30");
		ini.SetValue("HYPOTHERMIA", "MIN_TIME_TO_DRY", std::format("{:g}", cfg.MinTimeToDry).c_str(),
			";Seconds. How long it takes to dry off completely in a warm, dry climate without a heat source.\n"
			";Campfires and other heat sources speed this up; other weather slows it down.\n"
			";Mod default = 30");

		ini.SetValue("TEMPERATURE", nullptr, nullptr,
			";Temperature bands. Write each as a number followed by F or C, e.g. 68F or 20C.\n"
			";Either works in any line; the mod converts to the other.");
		ini.SetValue("TEMPERATURE", "NOMINAL", FormatFahrenheit(cfg.NominalTemperatureC).c_str(),
			";Comfortable temperature.\n"
			";Mod default = 68F");
		ini.SetValue("TEMPERATURE", "EXTREME", FormatFahrenheit(cfg.ExtremeTemperatureC).c_str(),
			";Dangerously hot.\n"
			";Mod default = 89.6F");
		ini.SetValue("TEMPERATURE", "FREEZING", FormatFahrenheit(cfg.FreezingTemperatureC).c_str(),
			";Freezing.\n"
			";Mod default = 32F");

		ini.SaveFile(ConfigPath().c_str(), false); // false = no UTF-8 BOM
		return cfg;
	}

	// Exists but unreadable: use the defaults rather than clobbering the file.
	if (ini.LoadFile(ConfigPath().c_str()) < 0)
		return cfg;

	cfg.DisableEagleEye = ini.GetBoolValue("MISC", "DISABLE_EAGLE_EYE", cfg.DisableEagleEye);
	cfg.DisableMod = ini.GetBoolValue("MISC", "DISABLE_MOD", cfg.DisableMod);

	cfg.DeadEyeDurationCost = static_cast<float>(ini.GetDoubleValue("DEAD_EYE", "DURATION_COST", cfg.DeadEyeDurationCost));
	cfg.DeadEyeActivationCost = static_cast<float>(ini.GetDoubleValue("DEAD_EYE", "ACTIVATION_COST", cfg.DeadEyeActivationCost));

	cfg.HealingThreshold = static_cast<int>(ini.GetLongValue("HEALTH", "HEALING_THRESHOLD", cfg.HealingThreshold));
	cfg.HealthCoreDrainsPerDay = static_cast<float>(ini.GetDoubleValue("HEALTH", "CORE_DRAINS_PER_DAY", cfg.HealthCoreDrainsPerDay));

	cfg.StaminaCoreDrainsPerDay = static_cast<float>(ini.GetDoubleValue("STAMINA", "CORE_DRAINS_PER_DAY", cfg.StaminaCoreDrainsPerDay));
	cfg.StaminaHurtsHealth = ini.GetBoolValue("STAMINA", "STAMINA_HURTS_HEALTH", cfg.StaminaHurtsHealth);
	cfg.ImproperClothingStaminaDrainRate = static_cast<float>(ini.GetDoubleValue("STAMINA", "IMPROPER_CLOTHING_DRAIN_RATE", cfg.ImproperClothingStaminaDrainRate));

	cfg.HurtingTimeToDieMin = static_cast<float>(ini.GetDoubleValue("HURTING", "HURTING_TIME_TO_DIE_MIN", cfg.HurtingTimeToDieMin));
	cfg.HurtingTimeToDieMax = static_cast<float>(ini.GetDoubleValue("HURTING", "HURTING_TIME_TO_DIE_MAX", cfg.HurtingTimeToDieMax));
	cfg.HurtingIsLethal = ini.GetBoolValue("HURTING", "HURTING_IS_LETHAL", cfg.HurtingIsLethal);

	cfg.EnableHypothermia = ini.GetBoolValue("HYPOTHERMIA", "ENABLE", cfg.EnableHypothermia);
	cfg.MaxSweatAccumulation = static_cast<float>(ini.GetDoubleValue("HYPOTHERMIA", "MAX_SWEAT_ACCUMULATION", cfg.MaxSweatAccumulation));
	cfg.NominalMaxSweatTime = static_cast<float>(ini.GetDoubleValue("HYPOTHERMIA", "NOMINAL_MAX_SWEAT_TIME", cfg.NominalMaxSweatTime));
	cfg.MinTimeToGetSoaked = static_cast<float>(ini.GetDoubleValue("HYPOTHERMIA", "MIN_TIME_TO_GET_SOAKED", cfg.MinTimeToGetSoaked));
	cfg.MinTimeToDry = static_cast<float>(ini.GetDoubleValue("HYPOTHERMIA", "MIN_TIME_TO_DRY", cfg.MinTimeToDry));

	cfg.NominalTemperatureC = ReadTemperatureC(ini, "TEMPERATURE", "NOMINAL", cfg.NominalTemperatureC);
	cfg.ExtremeTemperatureC = ReadTemperatureC(ini, "TEMPERATURE", "EXTREME", cfg.ExtremeTemperatureC);
	cfg.FreezingTemperatureC = ReadTemperatureC(ini, "TEMPERATURE", "FREEZING", cfg.FreezingTemperatureC);

	return cfg;
}
