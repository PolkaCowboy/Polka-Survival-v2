#include "config.h"
#include "paths.h"
#include "utils.h"
#include "../inc/SimpleIni.h"

#include <cctype>
#include <cmath>
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
	using ExposureRing::CompassType;
	using ExposureRing::RingPlacement;

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

	// Defaults are written out in Fahrenheit, rounded to 0.1F so float noise doesn't show
	// (-17.8C is -0.04F, written as "0F").
	std::string FormatFahrenheit(float celsius)
	{
		float f = std::round(CelsiusToFahrenheit(celsius) * 10.0f) / 10.0f;
		if (f == 0.0f)
			f = 0.0f; // no "-0F"
		return std::format("{:g}F", f);
	}

	struct CompassName
	{
		CompassType type;
		const char* name;
	};
	const CompassName COMPASS_NAMES[] = {
		{ CompassType::Regular, "REGULAR" },
		{ CompassType::Expanded, "EXPANDED" },
		{ CompassType::Simple, "COMPASS" }, // the game menu's "Compass" option
		{ CompassType::Off, "OFF" },
	};

	const char* CompassToString(CompassType type)
	{
		for (const CompassName& c : COMPASS_NAMES)
		{
			if (c.type == type)
				return c.name;
		}
		return "REGULAR";
	}

	// Case-insensitive. Anything unrecognised keeps the current value.
	CompassType ReadCompassType(const CSimpleIniA& ini, const char* section, const char* key, CompassType current)
	{
		const char* text = ini.GetValue(section, key, nullptr);
		if (!text)
			return current;
		for (const CompassName& c : COMPASS_NAMES)
		{
			if (_stricmp(text, c.name) == 0)
				return c.type;
		}
		return current;
	}

	std::string Number(float value) { return std::format("{:g}", value); }
	std::string Number(int value) { return std::format("{}", value); }
	std::string Bool(bool value) { return value ? "true" : "false"; }

	// Adds settings the INI doesn't have yet, each with its comment and its default. Values and
	// comments already in the file are left alone, so a first run writes the whole file and an
	// update only adds the new settings.
	class DefaultWriter
	{
	public:
		explicit DefaultWriter(CSimpleIniA& ini) : m_ini(ini) {}

		// A section's own comment only goes in when the section is new.
		void Section(const char* section, const char* comment)
		{
			if (m_ini.GetSection(section))
				return;
			m_ini.SetValue(section, nullptr, nullptr, comment);
			m_added = true;
		}

		// description: comment lines, each starting with ';'. The default is added below it.
		void Key(const char* section, const char* key, const std::string& value, const std::string& description = "")
		{
			std::string comment = description.empty() ? "" : description + "\n";
			Add(section, key, value, comment + ";Mod default = " + value);
		}

		// Each placement is three keys: <PREFIX>_X, <PREFIX>_Y, <PREFIX>_SIZE.
		void Placement(const char* section, const std::string& prefix, const RingPlacement& p, const std::string& description)
		{
			Add(section, (prefix + "_X").c_str(), Number(p.x), description + std::format(" Mod default = {:g}, {:g}, {:g}", p.x, p.y, p.size));
			Add(section, (prefix + "_Y").c_str(), Number(p.y), "");
			Add(section, (prefix + "_SIZE").c_str(), Number(p.size), "");
		}

		bool Added() const { return m_added; }

	private:
		void Add(const char* section, const char* key, const std::string& value, const std::string& comment)
		{
			if (m_ini.GetValue(section, key, nullptr))
				return;
			m_ini.SetValue(section, key, value.c_str(), comment.empty() ? nullptr : comment.c_str());
			m_added = true;
		}

		CSimpleIniA& m_ini;
		bool m_added = false;
	};

	// Every setting, in file order. Returns whether anything was added.
	bool WriteMissingDefaults(CSimpleIniA& ini, const ModConfig& d)
	{
		DefaultWriter w(ini);

		w.Key("MISC", "DISABLE_EAGLE_EYE", Bool(d.DisableEagleEye));
		w.Key("MISC", "DISABLE_MOD", Bool(d.DisableMod), ";Turn off the mod");

		w.Section("DEAD_EYE", ";Dead eye is scarcer than vanilla: it drains faster, and starting it costs extra.");
		w.Key("DEAD_EYE", "DURATION_COST", Number(d.DeadEyeDurationCost),
			";Dead eye used per second while it's on. 0 = the game's own rate (about 4.6). 10 = about half as long.");
		w.Key("DEAD_EYE", "ACTIVATION_COST", Number(d.DeadEyeActivationCost),
			";Dead eye taken each time you turn it on.");

		w.Key("HEALTH", "HEALING_THRESHOLD", Number(d.hurt.healingThreshold),
			";Percent that the health core needs to be above before the health ring heals.");
		w.Key("HEALTH", "CORE_DRAINS_PER_DAY", Number(d.hurt.healthCoreDrainsPerDay),
			";How many times the health core should fully drain per in-game day (48 real world minutes).");

		w.Key("STAMINA", "CORE_DRAINS_PER_DAY", Number(d.hurt.staminaCoreDrainsPerDay),
			";How many times the stamina core should fully drain per in-game day (48 real world minutes).");

		w.Section("HURTING",
			";If health core turns red (16% or lower) how long will it take the health ring to completely drain from max.\n"
			";HURTING_TIME_TO_DIE_MAX is when Core is as 16 %, HURTING_TIME_TO_DIE_MIN when core is an 0%.\n"
			";Scales between min/max for intermediate values.\n"
			";Value in real world minutes.");
		w.Key("HURTING", "HURTING_TIME_TO_DIE_MIN", Number(d.hurt.timeToDieMin));
		w.Key("HURTING", "HURTING_TIME_TO_DIE_MAX", Number(d.hurt.timeToDieMax));
		w.Key("HURTING", "HURTING_IS_LETHAL", Bool(d.hurt.lethal),
			";true = hurting can kill you. false = All health core drains will stop at 1hp leaving you very squishy while restoring your core.");

		w.Section("HYPOTHERMIA", "; Times are in real world seconds.");
		w.Key("HYPOTHERMIA", "ENABLE", Bool(d.hypothermia.enabled));
		w.Key("HYPOTHERMIA", "EFFECT_STRENGTH", Number(d.hypothermia.effectStrength),
			";How hard hypothermia hits your cores and health ring. 1 = as designed, 0.5 = half, 0 = the cold costs nothing.");
		w.Key("HYPOTHERMIA", "MAX_SWEAT_ACCUMULATION", Number(d.wetness.maxSweatAccumulation),
			";How wet you can get from sweat alone. 0-1: 0.4 = 40% of fully soaked. 0 = no sweating.");
		w.Key("HYPOTHERMIA", "MIN_TIME_TO_GET_SOAKED", Number(d.wetness.minTimeToGetSoaked),
			";Seconds. How long it takes to get 100% soaked in a full rainstorm.");
		w.Key("HYPOTHERMIA", "MIN_TIME_TO_DRY", Number(d.wetness.minTimeToDry),
			";Seconds. How long it takes to dry off completely in a warm, dry climate without a heat source.\n"
			";Campfires and other heat sources speed this up; other weather slows it down.");
		w.Key("HYPOTHERMIA", "TIME_TO_HYPOTHERMIA_DRY", Number(d.hypothermia.timeToHypothermiaDry),
			";Seconds. How long it takes to become fully hypothermic when dry, at freezing or colder.\n"
			";Milder cold is slower and builds up to a lower level instead.");
		w.Key("HYPOTHERMIA", "TIME_TO_HYPOTHERMIA_WET", Number(d.hypothermia.timeToHypothermiaWet),
			";Seconds. The same, but soaked. Being partly wet sits in between, closer to the dry time.");
		w.Key("HYPOTHERMIA", "RECOVER_NOMINAL", Number(d.hypothermia.recoverNominal),
			";Seconds. How long it takes to recover from full hypothermia somewhere warm, without a fire.");

		w.Section("TEMPERATURE",
			";Temperature bands. Write each as a number followed by F or C, e.g. 68F or 20C.\n"
			";Either works in any line; the mod converts to the other.\n"
			";Advanced: these bands drive the whole cold and heat system, so small changes have big effects.");
		w.Key("TEMPERATURE", "NOMINAL", FormatFahrenheit(d.temperature.nominalC), ";Comfortable temperature.");
		w.Key("TEMPERATURE", "EXTREME", FormatFahrenheit(d.temperature.extremeC), ";Dangerously hot.");
		w.Key("TEMPERATURE", "FREEZING", FormatFahrenheit(d.temperature.freezingC), ";Freezing.");
		w.Key("TEMPERATURE", "BITTER_COLD", FormatFahrenheit(d.temperature.bitterColdC),
			";Cold at full strength. Anything colder counts the same.");

		w.Section("CAMP",
			";Setting up camp takes game time, costs core points, and can fail. Low cores, a wounded health ring,\n"
			";hypothermia, heavy rain and strong wind all make it slower or riskier.");
		w.Key("CAMP", "ENABLE", Bool(d.camp.enabled), ";false = vanilla camp setup.");
		w.Key("CAMP", "ONE_CAMP_AT_A_TIME", Bool(d.camp.oneCampAtATime),
			";You can't set up a new camp while your last one still stands. This also stops the warp back to it.");
		w.Key("CAMP", "SETUP_MINUTES_BEST", Number(d.camp.setupMinutesBest),
			";Game minutes a setup takes with full cores and health ring. A failed attempt costs the same time.");
		w.Key("CAMP", "SETUP_MINUTES_WORST", Number(d.camp.setupMinutesWorst),
			";Game minutes with both cores and the health ring empty. Each attempt varies by up to 10% either way.");
		w.Key("CAMP", "FAIL_CHANCE_SCALE", Number(d.camp.failChanceScale),
			";Multiplies every failure chance. 1 = as designed, 0.5 = half as likely, 0 = never fails.");

		w.Key("MOVEMENT", "SLOW_WHEN_WEAK", Bool(d.movement.slowWhenWeak),
			";Low cores and a wounded health ring slow you down, as far as a walk.");

		w.Section("HUD",
			";The exposure ring: fills blue as you get wet, shows a cold icon when hypothermia sets in,\n"
			";and the icon pulses when the cold is costing you health.\n"
			";Position and size are in pixels on a 1920x1080 screen, whatever your actual resolution:\n"
			";_X and _Y are the centre of the ring from the left and top edges.");
		w.Key("HUD", "SHOW_EXPOSURE_RING", Bool(d.exposureRing.showRing));
		w.Key("HUD", "COMPASS_TYPE", CompassToString(d.exposureRing.compass),
			";The compass type you play with (hold Alt in game to change it): REGULAR, EXPANDED, COMPASS or OFF.\n"
			";The game doesn't tell mods which one is set, so the ring uses this to know where to sit.");
		w.Placement("HUD", "RING_REGULAR", d.exposureRing.ringRegular, ";Ring placement in the regular (small) HUD layout.");
		w.Placement("HUD", "RING_COMPASS", d.exposureRing.ringCompass, ";Ring placement with the COMPASS or OFF options (the game uses one layout for both).");
		w.Placement("HUD", "RING_EXPANDED", d.exposureRing.ringExpanded,
			";Ring placement in the expanded HUD layout: the EXPANDED compass, and briefly after tapping Alt\n"
			";or while the weapon wheel is open.");

		return w.Added();
	}

	// Settings older versions wrote that nothing reads any more. Removed on load.
	struct RetiredKey
	{
		const char* section;
		const char* key;
	};
	const RetiredKey RETIRED_KEYS[] = {
		{ "STAMINA", "STAMINA_HURTS_HEALTH" },
		{ "STAMINA", "IMPROPER_CLOTHING_DRAIN_RATE" },
		{ "HYPOTHERMIA", "NOMINAL_MAX_SWEAT_TIME" },
	};

	RingPlacement ReadPlacement(const CSimpleIniA& ini, const char* section, const std::string& prefix, RingPlacement p)
	{
		p.x = static_cast<float>(ini.GetDoubleValue(section, (prefix + "_X").c_str(), p.x));
		p.y = static_cast<float>(ini.GetDoubleValue(section, (prefix + "_Y").c_str(), p.y));
		p.size = static_cast<float>(ini.GetDoubleValue(section, (prefix + "_SIZE").c_str(), p.size));
		return p;
	}
}

ModConfig LoadConfig()
{
	const ModConfig defaults{}; // from config.h
	ModConfig cfg = defaults;

	CSimpleIniA ini;
	ini.SetUnicode();

	bool exists = fs::exists(ConfigPath());
	// Exists but unreadable: use the defaults rather than clobbering the file.
	if (exists && ini.LoadFile(ConfigPath().c_str()) < 0)
		return cfg;

	bool changed = !exists;
	for (const RetiredKey& retired : RETIRED_KEYS)
	{
		if (ini.Delete(retired.section, retired.key))
			changed = true;
	}
	if (WriteMissingDefaults(ini, defaults))
		changed = true;
	if (changed)
		ini.SaveFile(ConfigPath().c_str(), false); // false = no UTF-8 BOM

	cfg.DisableEagleEye = ini.GetBoolValue("MISC", "DISABLE_EAGLE_EYE", cfg.DisableEagleEye);
	cfg.DisableMod = ini.GetBoolValue("MISC", "DISABLE_MOD", cfg.DisableMod);

	cfg.DeadEyeDurationCost = static_cast<float>(ini.GetDoubleValue("DEAD_EYE", "DURATION_COST", cfg.DeadEyeDurationCost));
	cfg.DeadEyeActivationCost = static_cast<float>(ini.GetDoubleValue("DEAD_EYE", "ACTIVATION_COST", cfg.DeadEyeActivationCost));

	cfg.hurt.healingThreshold = static_cast<int>(ini.GetLongValue("HEALTH", "HEALING_THRESHOLD", cfg.hurt.healingThreshold));
	cfg.hurt.healthCoreDrainsPerDay = static_cast<float>(ini.GetDoubleValue("HEALTH", "CORE_DRAINS_PER_DAY", cfg.hurt.healthCoreDrainsPerDay));

	cfg.hurt.staminaCoreDrainsPerDay = static_cast<float>(ini.GetDoubleValue("STAMINA", "CORE_DRAINS_PER_DAY", cfg.hurt.staminaCoreDrainsPerDay));

	cfg.hurt.timeToDieMin = static_cast<float>(ini.GetDoubleValue("HURTING", "HURTING_TIME_TO_DIE_MIN", cfg.hurt.timeToDieMin));
	cfg.hurt.timeToDieMax = static_cast<float>(ini.GetDoubleValue("HURTING", "HURTING_TIME_TO_DIE_MAX", cfg.hurt.timeToDieMax));
	cfg.hurt.lethal = ini.GetBoolValue("HURTING", "HURTING_IS_LETHAL", cfg.hurt.lethal);

	cfg.hypothermia.enabled = ini.GetBoolValue("HYPOTHERMIA", "ENABLE", cfg.hypothermia.enabled);
	cfg.hypothermia.effectStrength = static_cast<float>(ini.GetDoubleValue("HYPOTHERMIA", "EFFECT_STRENGTH", cfg.hypothermia.effectStrength));
	cfg.wetness.maxSweatAccumulation = static_cast<float>(ini.GetDoubleValue("HYPOTHERMIA", "MAX_SWEAT_ACCUMULATION", cfg.wetness.maxSweatAccumulation));
	cfg.wetness.minTimeToGetSoaked = static_cast<float>(ini.GetDoubleValue("HYPOTHERMIA", "MIN_TIME_TO_GET_SOAKED", cfg.wetness.minTimeToGetSoaked));
	cfg.wetness.minTimeToDry = static_cast<float>(ini.GetDoubleValue("HYPOTHERMIA", "MIN_TIME_TO_DRY", cfg.wetness.minTimeToDry));
	cfg.hypothermia.timeToHypothermiaDry = static_cast<float>(ini.GetDoubleValue("HYPOTHERMIA", "TIME_TO_HYPOTHERMIA_DRY", cfg.hypothermia.timeToHypothermiaDry));
	cfg.hypothermia.timeToHypothermiaWet = static_cast<float>(ini.GetDoubleValue("HYPOTHERMIA", "TIME_TO_HYPOTHERMIA_WET", cfg.hypothermia.timeToHypothermiaWet));
	cfg.hypothermia.recoverNominal = static_cast<float>(ini.GetDoubleValue("HYPOTHERMIA", "RECOVER_NOMINAL", cfg.hypothermia.recoverNominal));

	cfg.temperature.nominalC = ReadTemperatureC(ini, "TEMPERATURE", "NOMINAL", cfg.temperature.nominalC);
	cfg.temperature.extremeC = ReadTemperatureC(ini, "TEMPERATURE", "EXTREME", cfg.temperature.extremeC);
	cfg.temperature.freezingC = ReadTemperatureC(ini, "TEMPERATURE", "FREEZING", cfg.temperature.freezingC);
	cfg.temperature.bitterColdC = ReadTemperatureC(ini, "TEMPERATURE", "BITTER_COLD", cfg.temperature.bitterColdC);

	cfg.camp.enabled = ini.GetBoolValue("CAMP", "ENABLE", cfg.camp.enabled);
	cfg.camp.oneCampAtATime = ini.GetBoolValue("CAMP", "ONE_CAMP_AT_A_TIME", cfg.camp.oneCampAtATime);
	cfg.camp.setupMinutesBest = static_cast<float>(ini.GetDoubleValue("CAMP", "SETUP_MINUTES_BEST", cfg.camp.setupMinutesBest));
	cfg.camp.setupMinutesWorst = static_cast<float>(ini.GetDoubleValue("CAMP", "SETUP_MINUTES_WORST", cfg.camp.setupMinutesWorst));
	cfg.camp.failChanceScale = static_cast<float>(ini.GetDoubleValue("CAMP", "FAIL_CHANCE_SCALE", cfg.camp.failChanceScale));

	cfg.movement.slowWhenWeak = ini.GetBoolValue("MOVEMENT", "SLOW_WHEN_WEAK", cfg.movement.slowWhenWeak);

	cfg.exposureRing.showRing = ini.GetBoolValue("HUD", "SHOW_EXPOSURE_RING", cfg.exposureRing.showRing);
	cfg.exposureRing.compass = ReadCompassType(ini, "HUD", "COMPASS_TYPE", cfg.exposureRing.compass);
	cfg.exposureRing.ringRegular = ReadPlacement(ini, "HUD", "RING_REGULAR", cfg.exposureRing.ringRegular);
	cfg.exposureRing.ringCompass = ReadPlacement(ini, "HUD", "RING_COMPASS", cfg.exposureRing.ringCompass);
	cfg.exposureRing.ringExpanded = ReadPlacement(ini, "HUD", "RING_EXPANDED", cfg.exposureRing.ringExpanded);

	return cfg;
}
