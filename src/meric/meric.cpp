
#include "meric.h"

using namespace meric;

bool        Meric::CONTINUAL  = mutils::getenv("MERIC_CONTINUAL", true);
std::string Meric::OUTPUT_DIR = mutils::getenv("MERIC_OUTPU_DIR", "log");



