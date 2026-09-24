#pragma once


// Given that the core is 0-100 int scale, Flexing the amount of 
// millseconds before each draining of the core seems a better fit to
// increase things by a percentage. This class will handle scaling the drain
// interval by filling a bunket instead of waiting for an exact tick count
	
class ElasticInterval
{
public:
    ElasticInterval() = default; // interval 0 = off
    explicit ElasticInterval(int intervalMs);

    // Advance by dtMs real milliseconds at `rate` (1.0 = normal, 1.15 = 15% faster, 0 = paused).
    // Returns how many times the interval completed; usually 0 or 1. Use this completion in the caller to act upon
    [[nodiscard]] int Update(int dtMs, double rate);

    bool IsEnabled() const { return m_intervalMs > 0; }
    int IntervalMs() const { return m_intervalMs; }

    int RemainingMs(double rate) const;

private:
    // Base interval
    int m_intervalMs = 0;
    double m_progressMs = 0.0;
};


