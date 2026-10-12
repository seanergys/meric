#pragma once

#ifndef SRC_BASIS_ASYNC_UTILS_H_
#define SRC_BASIS_ASYNC_UTILS_H_

#include <atomic>
#include <chrono>
#include <thread>
#include <mutex>
#include <condition_variable>

#include "statistics_utils.h"

template<typename T>
class AsyncSampler
{
public:
	~AsyncSampler() { Stop(); }

	template <typename F> void Start(const std::chrono::nanoseconds &period, F &&f)
	{
		m_period = period;
		m_pRequest = nullptr;
		m_enabled.store(true);
		m_thread = std::thread(
			[this, f]()
			{
				auto time = Clock_t::now();

				while (m_enabled.load())
				{
					{
						std::unique_lock<std::mutex> lock(m_mutex);

						m_cond.wait_until(lock, time, [&]() { return Clock_t::now() >= time || m_pRequest || !m_enabled; });

						if (!m_enabled)
						{
							m_pRequest = nullptr;
							m_cond.notify_all();
							break;
						}

						const auto sampleTime = Clock_t::now();

						{
							const auto sampleStart = Clock_t::now();
							f(sampleTime >= time, m_pRequest);
							const auto sampleElapsedTime = Clock_t::now() - sampleStart;
							m_sampleTime.Push(std::chrono::duration<double>(sampleElapsedTime).count());
						}

						m_pRequest = nullptr;

						if (sampleTime >= time)
						{
							const uint64_t skipPeriods = std::chrono::duration_cast<std::chrono::nanoseconds>(Clock_t::now() - time).count() / m_period.count();
							time += (skipPeriods + 1) * m_period;
						}
					}

					m_cond.notify_all();
				}
			});
	}

	void Stop()
	{
		m_enabled.store(false);
		m_cond.notify_all();

		if (m_thread.joinable())
		{
			m_thread.join();
		}
	}

	void Sample(T *pRequest)
	{
		std::unique_lock<std::mutex> lock(m_mutex);
		m_pRequest = pRequest;
		m_cond.notify_all();
		m_cond.wait(lock, [this]() { return !m_pRequest; });
	}

	std::chrono::nanoseconds GetPeriod() const { return m_period; }

	void SetPeriod(const std::chrono::nanoseconds &period)
	{
		std::unique_lock<std::mutex> lock(m_mutex);
		m_period = period;
		m_cond.notify_all();
	}

	void SetFrequecy(double frequencyHz)
	{
		if (frequencyHz == 0)
		{
			return;
		}
		SetPeriod(std::chrono::nanoseconds(uint64_t(1.0e9 / frequencyHz)));
	}

	void GetSampleStats(StreamStats &stats)
	{
		std::unique_lock<std::mutex> lock(m_mutex);
		stats = m_sampleTime;
		m_cond.notify_all();
	}

	void ClearSampleStats()
	{
		std::unique_lock<std::mutex> lock(m_mutex);
		m_sampleTime.Clear();
		m_cond.notify_all();
	}

protected:
	typedef std::chrono::steady_clock Clock_t;

	std::chrono::nanoseconds m_period;
	std::thread m_thread;
	std::atomic<bool> m_enabled;
	std::condition_variable m_cond;
	std::mutex m_mutex;
	StreamStats m_sampleTime;

	T *m_pRequest;
};

#endif // SRC_BASIS_ASYNC_UTILS_H_
