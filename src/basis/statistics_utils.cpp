#include <limits>
#include <cmath>
#include <algorithm>

#include "statistics_utils.h"

StreamStats::StreamStats()
    : m_count(0)
    , m_min( std::numeric_limits<double>::max())
    , m_max(-std::numeric_limits<double>::max())
    , m_m1(0), m_m2(0), m_m3(0), m_m4(0)
{
}

void StreamStats::Clear()
{
    m_count = 0;
    m_min =  std::numeric_limits<double>::max();
    m_max = -std::numeric_limits<double>::max();
    m_m1 = 0;
    m_m2 = 0;
    m_m3 = 0;
    m_m4 = 0;
}

void StreamStats::Push(double value)
{
    uint64_t count = m_count;
    m_count++;

    m_min = std::min(m_min, value);
    m_max = std::max(m_max, value);

    double delta = value - m_m1;
    double deltaN = delta / double(m_count);
    double deltaN2 = deltaN * deltaN;
    double term1 = delta * deltaN * double(count);

    m_m1 += deltaN;
    m_m2 += term1 * deltaN2 * (m_count * m_count - 3 * m_count + 3) +
        6 * deltaN2 * m_m2 - 4 * deltaN * m_m3;
    m_m3 += term1 * deltaN * (m_count - 2) - 3 * deltaN * m_m2;
    m_m4 += term1;
}

double StreamStats::GetVariance() const
{
    return m_m2 / (double(m_count) - 1.0);
}

double StreamStats::GetStdDeviation() const
{
    return std::sqrt(GetVariance());
}

double StreamStats::GetSkewness() const
{
    return std::sqrt(double(m_count)) * m_m3 / std::pow(m_m2, 1.5);
}

double StreamStats::GetKurtosis() const
{
    return double(m_count) * m_m4 / (m_m2 * m_m2) - 3.0;
}


StreamStats &StreamStats::operator+=(const StreamStats &other)
{
    StreamStats result = *this + other;
    *this = result;
    return *this;
}

StreamStats operator+(const StreamStats &left, const StreamStats &right)
{
    StreamStats result;

    result.m_count = left.m_count + right.m_count;

    double delta = right.m_m1 - left.m_m1;
    double delta2 = delta*delta;
    double delta3 = delta*delta2;
    double delta4 = delta2*delta2;

    result.m_m1 = (left.m_count * left.m_m1 + right.m_count * right.m_m1) / result.m_count;
    result.m_m2 = left.m_m2 + right.m_m2 + delta2 * left.m_count * right.m_count / result.m_count;

    result.m_m3 = left.m_m3 + right.m_m3 +
            delta3 * left.m_count * right.m_count * (left.m_count - right.m_count) /
            (result.m_count * result.m_count);
    result.m_m3 += 3.0 * delta * (left.m_count * right.m_m2 - right.m_count * left.m_m2) / result.m_count;

    result.m_m4 = left.m_m4 + right.m_m4 + delta4 * left.m_count * right.m_count *
            (left.m_count * left.m_count - left.m_count * right.m_count + right.m_count * right.m_count) /
            (result.m_count * result.m_count * result.m_count);
    result.m_m4 += 6.0 * delta2 * (left.m_count * left.m_count * right.m_m2 + right.m_count * right.m_count * left.m_m2) /
            (result.m_count * result.m_count) +
            4.0 * delta * (left.m_count * right.m_m3 - right.m_count * left.m_m3) / result.m_count;

    return result;
}