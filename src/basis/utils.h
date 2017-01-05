
#ifndef SRC_BASIS_UTILS_H_
#define SRC_BASIS_UTILS_H_

#include <string>
#include <cstdlib>
#include <sstream>
#include <iomanip>


#ifdef VERBOSE
#define MERIC_INFO std::cout
#else
#define MERIC_INFO if (true); else std::cout
#endif

namespace meric {

struct mutils {

	template <typename Ttype>
	static Ttype getenv(const std::string &param, Ttype defaultValue);

	static double timeToDouble(const timespec &time)
	{
		return time.tv_sec + time.tv_nsec / 1e9;
	}
	static void createDir(const std::string &dir) {
		std::stringstream ss;
		ss << "mkdir -p " + dir;
		int status = std::system(ss.str().c_str());
	}
};

inline timespec operator-(const timespec &a, const timespec &b)
{
	timespec diff;
	diff.tv_sec = a.tv_sec - b.tv_sec;
	diff.tv_nsec = a.tv_nsec - b.tv_nsec;
	if (diff.tv_nsec < 0) {
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

}

#include "utils.hpp"


#endif /* SRC_BASIS_UTILS_H_ */
