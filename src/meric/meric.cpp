
#include "meric.h"

using namespace meric;

int         Meric::MODE       = mutils::getenv<int>("MERIC_MODE" , MODE_HDEEM);
bool        Meric::CONTINUAL  = mutils::getenv("MERIC_CONTINUAL" , false);
bool        Meric::DETAILED   = mutils::getenv("MERIC_DETAILED"  , false);
bool        Meric::AGGREGATE  = mutils::getenv("MERIC_AGGREGATE" , false);
bool        Meric::DEBUG      = mutils::getenv("MERIC_DEBUG"     , false);
std::string Meric::COUNTERS   = mutils::getenv("MERIC_COUNTERS"  , std::string("none"));
std::string Meric::OUTPUT_DIR = mutils::getenv("MERIC_OUTPUT_DIR", std::string("hdeemMeasurement"));
std::string Meric::OUTPUT_FILENAME = mutils::getenv("MERIC_OUTPUT_FILENAME", std::string("log"));

std::map< std::pair<std::string, int>, RegionParameters> Meric::parameter;
std::vector<std::string> Meric::_name;
std::vector<int> Meric::_stack;
RegionParameters Meric::defaultParameter;
std::string Meric::_mainRegion;

void Meric::readRegionParameters()
{
	defaultParameter.nthreads        = mutils::getenv("MERIC_NUM_THREADS", 0);
	defaultParameter.frequency       = mutils::getenv("MERIC_FREQUENCY", 0);
	defaultParameter.uncoreFrequency = mutils::getenv("MERIC_UNCORE_FREQUENCY", 0);

#ifdef HAVE_CPUFREQ_H
	if (defaultParameter.frequency == 0)
	{
		std::cerr << "MERIC ERROR: set MERIC_FREQUENCY\n";
#ifndef HAVE_X86_ADAPT_H
		exit(1);
#endif
	}
#endif

#ifdef HAVE_X86_ADAPT_H
	if (defaultParameter.uncoreFrequency == 0)
	{
		std::cerr << "MERIC ERROR: set MERIC_UNCORE_FREQUENCY\n";
		exit(1);
	}
#ifdef HAVE_CPUFREQ_H
	if (defaultParameter.frequency == 0)
		exit(1);
#endif
#endif

	MERIC_INFO << "Default number of threads: " << defaultParameter.nthreads << "\n";
	MERIC_INFO << "Default frequency: " << defaultParameter.frequency << "\n";
	MERIC_INFO << "Default uncore frequency: " << defaultParameter.uncoreFrequency << "\n";

	if (Meric::MODE < MODE_HDEEM || Meric::MODE > MODE_RUN)
	{
		std::cerr << "MERIC WARNING: MERIC MODE unknown value - default "<< MODE_HDEEM <<std::endl;
		Meric::MODE = MODE_HDEEM;
	}
	
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

	auto regionValues = [&values] (RegionParameters &region, size_t index)
	{
		for (size_t i = index; i < values.size(); i += 2)
		{
			std::stringstream ss(values[i + 1]);
			if (StringCompare::caseInsensitiveEq(values[i], "FREQUENCY"))
			{
				ss >> region.frequency;
			}else if (StringCompare::caseInsensitiveEq(values[i], "UNCORE_FREQUENCY"))
			{
				ss >> region.uncoreFrequency;
			}else if (StringCompare::caseInsensitiveEq(values[i], "NUM_THREADS")) 
			{
				ss >> region.nthreads;
			}
		}
	};

	while (tokenizer.next() != Tokenizer::Token::END) {
		if (bracketCounter < 0)
			break;
		switch (tokenizer.token()) {
		case Tokenizer::Token::STRING:
			values.push_back(tokenizer.value());
			break;
		case Tokenizer::Token::OBJECT_OPEN:
				bracketCounter++;
				break;
/*			if ((bracketCounter == 1 && values.size() == 1) || (bracketCounter == 0 && values.size() == 0) ) 
				bracketCounter++;
			else
			{
				std::cerr << "MERIC ERROR: invalid options format! Use JSON: {\"region\": { \"parameter\" : 'value', ... }, ... }\n";

for (int i=0; i<values.size();i++) std::cout << "\t"<< values[i] << std::endl;
				bracketCounter = -1;
			}
			break;
*/
		case Tokenizer::Token::OBJECT_CLOSE:
			if (bracketCounter == 1)
			{
				bracketCounter = 0;
				break;
			}
			else if (bracketCounter == 2) // || bracketCounter > 2)
			{
				if (values.size() == 1 && StringCompare::caseInsensitiveEq(values[0], "@NODE"))
				{
					bracketCounter--;
					break;	
				}
				else if ((values.size() % 2 != 1))
				{
					std::cerr << "MERIC ERROR: invalid options format! Use JSON: {\"region\": { \"parameter\" : 'value', ... }, ... }\n";
for (int i=0; i<values.size();i++) std::cout << "\t"<< values[i] << std::endl;
					bracketCounter = -1;
					break;
				}
				RegionParameters &region = Meric::parameter[std::make_pair(values[0], -1)];
				regionValues(region, 1);
				values.clear();
			}
			else if (bracketCounter == 3)
			{
				values.erase(values.end());
			}
			else if (bracketCounter == 4 && StringCompare::caseInsensitiveEq(values[0], "@NODE"))
			{
				RegionParameters &region = Meric::parameter[std::make_pair(values[2], std::stoi(values[1]))];
				regionValues(region, 3);
				values.erase(values.begin()+2, values.end());
			}
/*				else
				{
					RegionParameters &region = Meric::parameter[values[0]];
					regionValues(region);
					values.clear();
				}
*/


/*				for (size_t i = 1; i < values.size(); i += 2) {
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
*/
			bracketCounter--;
			break;
		case Tokenizer::Token::ASSIGN:
		case Tokenizer::Token::DELIMITER:
		case Tokenizer::Token::LINE_END:
			break;
		}
	}
}

