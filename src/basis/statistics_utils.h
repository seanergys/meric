#pragma once

#ifndef SRC_BASIS_STATISTICS_UTILS_H_
#define SRC_BASIS_STATISTICS_UTILS_H_

#include <cstdint>

class StreamStats
{
public:
	StreamStats();

	void Clear();

	void Push(double value);

	double GetMin() const { return m_min; }
	double GetMax() const { return m_max; }

	double GetMean() const { return m_m1; }
	double GetVariance() const;
	double GetStdDeviation() const;
	double GetSkewness() const;
	double GetKurtosis() const;

	uint64_t GetCount() const { return m_count; }

	StreamStats &operator+=(const StreamStats &other);

	friend StreamStats operator+(const StreamStats &left, const StreamStats &right);

protected:
	uint64_t m_count;
	double m_min, m_max;
	double m_m1, m_m2, m_m3, m_m4;
};

#endif // SRC_BASIS_STATISTICS_UTILS_H_
