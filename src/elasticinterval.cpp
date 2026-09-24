#include "elasticinterval.h"

#include <cmath>

ElasticInterval::ElasticInterval(int intervalMs) : m_intervalMs(intervalMs > 0 ? intervalMs : 0) { }

int ElasticInterval::Update(int dtMs, double rate) {
	if (!IsEnabled() || dtMs <= 0 || rate <= 0.0)
		return 0;

	m_progressMs += dtMs * rate;

	int completions = 0;
	while (m_progressMs >= m_intervalMs) {
		m_progressMs -= m_intervalMs;
		completions++;
	}

	return completions;
}

// Stuff below mainly for debugging

int ElasticInterval::RemainingMs(double rate) const {
	if (!IsEnabled() || rate <= 0.0)
		return -1;
	return static_cast<int>(std::ceil((m_intervalMs - m_progressMs) / rate));
}