
#ifndef SRC_BASIS_UTILS_H_
#define SRC_BASIS_UTILS_H_

#include <string>
#include <cstdlib>
#include <sstream>

namespace meric {

struct mutils {

	template <typename Ttype>
	static Ttype getenv(const std::string &param, Ttype defaultValue);
};

}

#include "utils.hpp"


#endif /* SRC_BASIS_UTILS_H_ */
