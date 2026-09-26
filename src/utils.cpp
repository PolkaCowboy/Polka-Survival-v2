#include "utils.h"

#include <format>
#include <algorithm>
#include <cmath>

std::string Utils::FormatDuration(int ms)
{
	if (ms < 0)
		ms = 0;

	int totalSeconds = ms / 1000;
	int hours = totalSeconds / 3600;
	int minutes = (totalSeconds / 60) % 60;
	int seconds = totalSeconds % 60;
	int millis = ms % 1000;

	return std::format("{:02}h {:02}m {:02}.{:03}s", hours, minutes, seconds, millis);
}

// Returns where value sits between minValue and maxValue, as 0.0 to 1.0. Clamps outside the range.
float Utils::ScaleToRange(float value, int minValue, int maxValue)
{
	if (maxValue <= minValue)
		return 0.0f;

	float clampedValue = std::clamp(value, static_cast<float>(minValue), static_cast<float>(maxValue));
	return (clampedValue - minValue) / (maxValue - minValue);
}

int Utils::MapToRange(int value, int inMin, int inMax, int outMin, int outMax)
{
	value = std::clamp(value, inMin, inMax);
	double t = double(value - inMin) / (inMax - inMin);
	return (int)std::lround(std::lerp(double(outMin), double(outMax), t));
}