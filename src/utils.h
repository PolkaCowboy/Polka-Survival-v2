#pragma once

#include <string>

namespace Utils
{
	// Formats a millisecond count as "00h 00m 00.000s". Negative values show as zero.
	std::string FormatDuration(int ms);

	float ScaleToRange(float value, int minValue, int maxValue);
	int MapToRange(int value, int inMin, int inMax, int outMin, int outMax);
}
