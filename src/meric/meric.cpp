
#include "meric.h"

using namespace meric;

bool        Meric::CONTINUAL  = mutils::getenv("MERIC_CONTINUAL" , false);
bool        Meric::DETAILED   = mutils::getenv("MERIC_DETAILED"  , false);
bool        Meric::AGGREGATE  = mutils::getenv("MERIC_AGGREGATE" , false);
bool        Meric::DEBUG      = mutils::getenv("MERIC_DEBUG"     , false);
std::string Meric::COUNTERS   = mutils::getenv("MERIC_COUNTERS"  , "none");
std::string Meric::OUTPUT_DIR = mutils::getenv("MERIC_OUTPUT_DIR", std::string("hdeemMeasurement"));
std::string Meric::OUTPUT_FILENAME = mutils::getenv("MERIC_OUTPUT_FILENAME", std::string("log"));

std::map<std::string, RegionParameters> Meric::parameter;
RegionParameters Meric::defaultParameter;

void Meric::readRegionParameters()
{
	defaultParameter.frequency = mutils::getenv("MERIC_FREQUENCY", 0);
	defaultParameter.uncoreFrequency = mutils::getenv("MERIC_UNCORE_FREQUENCY", 0);
	if (defaultParameter.frequency == 0 || defaultParameter.uncoreFrequency == 0)
	{
		std::cerr << "MERIC ERROR: set MERIC_FREQUENCY and MERIC_UNCORE_FREQUENCY\n";
		exit(1);
	}

	MERIC_INFO << "Default frequency: " << defaultParameter.frequency << "\n";
	MERIC_INFO << "Default uncore frequency: " << defaultParameter.uncoreFrequency << "\n";
	

	std::vector<std::string> values;
	Tokenizer tokenizer("region.options");
	while (tokenizer.next() != Tokenizer::Token::END) {
		switch (tokenizer.token()) {
		case Tokenizer::Token::STRING:
			values.push_back(tokenizer.value());
			break;
		case Tokenizer::Token::OBJECT_OPEN:
			if (values.size() != 1) {
				std::cerr << "MERIC ERROR: invalid options format! Use: 'region' { 'parameter' = 'value', ... }\n";
			}
			break;
		case Tokenizer::Token::OBJECT_CLOSE:
			if (values.size() % 2 != 1) {
				std::cerr << "MERIC ERROR: invalid options format! Use: 'region' { 'parameter' = 'value', ... }\n";
			} else {
				RegionParameters &region = Meric::parameter[values[0]];
				for (size_t i = 1; i < values.size(); i += 2) {
					if (StringCompare::caseInsensitiveEq(values[i], "FREQUENCY")) {
						std::stringstream ss(values[i + 1]);
						ss >> region.frequency;
						MERIC_INFO << "Region '" << values[0] << "' frequency: " << region.frequency << "\n";
					}
					else if (StringCompare::caseInsensitiveEq(values[i], "UNCORE_FREQUENCY")) {
						std::stringstream ss(values[i + 1]);
						ss >> region.uncoreFrequency;
						MERIC_INFO << "Region '" << values[0] << "' uncore frequency: " << region.uncoreFrequency << "\n";
					}
				}
			}
			values.clear();
			break;
		case Tokenizer::Token::ASSIGN:
		case Tokenizer::Token::DELIMITER:
		case Tokenizer::Token::LINE_END:
			break;
		}
	}
}

