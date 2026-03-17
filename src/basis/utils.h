
#ifndef SRC_BASIS_UTILS_H_
#define SRC_BASIS_UTILS_H_

#include <sys/stat.h>
#include <limits.h>

#include <algorithm>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <sstream>
#include <string>
#include <cstdint>

#ifdef VERBOSE
#define MERIC_INFO std::cout
#define ARM_INFO " "
#else
#define MERIC_INFO if (true); else std::cout
#define ARM_INFO "> /dev/null"
#endif

#ifndef MERIC_TMP_PATH
	#define MERIC_TMP_PATH MERIC_PATH
#endif


#ifdef HAVE_MPI_H
	#if SIZE_MAX == UCHAR_MAX
	   #define MPI_SIZE_T MPI_UNSIGNED_CHAR
	#elif SIZE_MAX == USHRT_MAX
	   #define MPI_SIZE_T MPI_UNSIGNED_SHORT
	#elif SIZE_MAX == UINT_MAX
	   #define MPI_SIZE_T MPI_UNSIGNED
	#elif SIZE_MAX == ULONG_MAX
	   #define MPI_SIZE_T MPI_UNSIGNED_LONG
	#else //SIZE_MAX == ULLONG_MAX
	   #define MPI_SIZE_T MPI_UNSIGNED_LONG_LONG
	#endif
#endif

namespace meric 
{

struct mutils 
{

	template <typename Ttype>
	static Ttype getenv(const std::string &param, Ttype defaultValue);

	static double timeToDouble(const timespec &time)
	{
		return time.tv_sec + time.tv_nsec / 1e9;
	}

	static uint64_t timeToNanoSec(const timespec &time)
	{
		return uint64_t(time.tv_sec) * 1000000000ull + time.tv_nsec;
	}

	static long frequencyToHz(const std::string &data)
	{
		if (data == "" || data.find_first_of("0123456789") != 0)
		{
			return -1;
		}
		size_t const n = data.find_first_not_of("0123456789");
		if (n == std::string::npos)
		{
			return std::stol(data);
		}
		const long frequency = std::stol(data.substr(0,n));
		std::string units = data.substr(n, data.size()-n);
		std::transform(units.begin(), units.end(), units.begin(), ::toupper);
		if (units == "HZ")
		{
			return frequency;
		}
		else if (units == "KHZ")
		{
			return frequency * 1e3;
		}
		else if (units == "MHZ")
		{
			return frequency * 1e6;
		}
		else if (units == "GHZ")
		{
			return frequency * 1e9;
		}
		else
		{
			return -1;
		}
	}

	static timespec stringToTime(const std::string &data)
	{
		timespec time;
		int sec, nsec;
		sscanf( data.c_str(), "%d.%d", &sec, &nsec );
		time.tv_sec = sec;
		time.tv_nsec = nsec;
		return time;
	}
		
	static void createDir(const std::string &dir)
	{
		struct stat info;
		if(stat(dir.c_str(), &info ) != 0)
		{
			std::stringstream ss;
			ss << "mkdir -p \"" + dir + "\"";
			if (std::system(ss.str().c_str()))
			{
				std::cerr << "MERIC ERROR: directory '"<<dir<<"' cannot be created\n";
			}
		}
	}

	static int countTrailingZeros(unsigned int x)
	{
		if (x == 0)
		{
			return 0; // Or handle the zero case as needed
		}

		#if defined(__GNUC__) || defined(__clang__) || defined(__INTEL_COMPILER)
			return __builtin_ctz(x);
		#elif defined(_MSC_VER)
			unsigned long index;
			_BitScanForward(&index, x);
			return index;
		#else
			// Portable implementation (less efficient)
			int count = 0;
			while ((x & 1) == 0)
			{
				x >>= 1;
				count++;
			}
			return count;
		#endif
	}
};

inline bool operator==(const timespec &a, const timespec &b)
{
	if (a.tv_sec == b.tv_sec && a.tv_nsec == b.tv_nsec)
	{
		return true;
	}
	else
	{
		return false;
	}
}

inline timespec operator-(const timespec &a, const timespec &b)
{
	timespec diff;
	diff.tv_sec = a.tv_sec - b.tv_sec;
	diff.tv_nsec = a.tv_nsec - b.tv_nsec;
	if (diff.tv_nsec < 0)
	{
		diff.tv_nsec += 1e9;
		diff.tv_sec--;
	}

	return diff;
}

inline double operator/(const timespec &a, const timespec &b)
{
	return (a.tv_sec + (double)a.tv_nsec / 1e9) / (b.tv_sec + (double)b.tv_nsec / 1e9);
}

inline std::ostream& operator<<(std::ostream& os, const timespec &time)
{
	os << time.tv_sec << "." << std::setfill('0') << std::setw(9) << time.tv_nsec;
	return os;
}

template<typename OutIt>
void explode(const std::string &input, char sep, OutIt output)
{
	std::istringstream buffer(input);
	std::string temp;
	while(std::getline(buffer, temp, sep))
	{
		*output++ = temp;
	}
}

template<typename InputIt, typename T, typename F>
std::string join(InputIt first, InputIt last, const T &delimiter, F &&f)
{
	if(first == last)
	{
		return T();
	}

	return std::accumulate(std::next(first), last, f(*first),
		[&](const T &a, const typename std::iterator_traits<InputIt>::value_type &b)
		{
			return a + delimiter + f(b);
		});
}

template<typename InputIt, typename T>
std::string join(InputIt first, InputIt last, const T &delimiter) {
	return join(first, last, delimiter, [](const typename std::iterator_traits<InputIt>::value_type &a) { return a; });
}

}

#include "utils.hpp"


#endif /* SRC_BASIS_UTILS_H_ */
