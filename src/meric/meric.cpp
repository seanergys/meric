
#include "meric.h"

using namespace meric;

bool        Meric::CONTINUAL  = mutils::getenv("MERIC_CONTINUAL" , false);
bool        Meric::DETAILED   = mutils::getenv("MERIC_DETAILED"  , false);
bool        Meric::AGGREGATE  = mutils::getenv("MERIC_AGGREGATE" , false);
bool        Meric::DEBUG      = mutils::getenv("MERIC_DEBUG"     , false);
std::string Meric::COUNTERS   = mutils::getenv("MERIC_COUNTERS"  , std::string("none"));
std::string Meric::OUTPUT_DIR = mutils::getenv("MERIC_OUTPUT_DIR", std::string("hdeemMeasurement"));
std::string Meric::OUTPUT_FILENAME = mutils::getenv("MERIC_OUTPUT_FILENAME", std::string("log"));

std::map<std::string, RegionParameters> Meric::parameter;
std::vector<int> Meric::_stack;
RegionParameters Meric::defaultParameter;

void Meric::readRegionParameters()
{
	defaultParameter.nthreads        = mutils::getenv("MERIC_NUM_THREADS", 0);
	defaultParameter.frequency       = mutils::getenv("MERIC_FREQUENCY", 0);
	defaultParameter.uncoreFrequency = mutils::getenv("MERIC_UNCORE_FREQUENCY", 0);
	if (defaultParameter.frequency == 0 || defaultParameter.uncoreFrequency == 0)
	{
		std::cerr << "MERIC ERROR: set MERIC_FREQUENCY and MERIC_UNCORE_FREQUENCY\n";
		exit(1);
	}

	MERIC_INFO << "Default number of threads: " << defaultParameter.nthreads << "\n";
	MERIC_INFO << "Default frequency: " << defaultParameter.frequency << "\n";
	MERIC_INFO << "Default uncore frequency: " << defaultParameter.uncoreFrequency << "\n";

	if (Meric::COUNTERS != "none")
	{
		if (Meric::COUNTERS != "papi" && Meric::COUNTERS != "perfevent")
		{
			std::cerr << "MERIC WARNING: unknown counter type "<<Meric::COUNTERS<<std::endl;
			Meric::COUNTERS = "none";
		}
	}
	
	int bracketCounter = 0;
	std::vector<std::string> values;
	Tokenizer tokenizer("region.options");
	while (tokenizer.next() != Tokenizer::Token::END) {
		if (bracketCounter < 0)
			break;
		switch (tokenizer.token()) {
		case Tokenizer::Token::STRING:
			values.push_back(tokenizer.value());
			break;
		case Tokenizer::Token::OBJECT_OPEN:
			if ((bracketCounter == 1 && values.size() == 1) || (bracketCounter == 0 && values.size() == 0) ) 
				bracketCounter++;
			else
			{
				std::cerr << "MERIC ERROR: invalid options format! Use JSON: {\"region\": { \"parameter\" : 'value', ... }, ... }\n";
				bracketCounter = -1;
			}
			break;
		case Tokenizer::Token::OBJECT_CLOSE:
			if (bracketCounter == 1)
			{
				bracketCounter--;
				break;
			}
			if ((bracketCounter == 2 && values.size() % 2 != 1) || bracketCounter > 2) {
				std::cerr << "MERIC ERROR: invalid options format! Use JSON: {\"region\": { \"parameter\" : 'value', ... }, ... }\n";
				bracketCounter = -1;
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
					}else if (StringCompare::caseInsensitiveEq(values[i], "NUM_THREADS")) {
						std::stringstream ss(values[i + 1]);
						ss >> region.nthreads;
						MERIC_INFO << "Region '" << values[0] << "' num_threads: " << region.nthreads << "\n";
					}
				}
			}
			values.clear();
			bracketCounter--;
			break;
		case Tokenizer::Token::ASSIGN:
		case Tokenizer::Token::DELIMITER:
		case Tokenizer::Token::LINE_END:
			break;
		}
	}
}

