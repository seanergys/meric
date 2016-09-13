
#include "meric.h"

using namespace meric;

bool        Meric::CONTINUAL  = mutils::getenv("MERIC_CONTINUAL", false);
bool        Meric::DETAILED   = mutils::getenv("MERIC_DETAILED", false);
std::string Meric::OUTPUT_DIR = mutils::getenv("MERIC_OUTPU_DIR", "log");

std::map<std::string, RegionParameters> Meric::parameter;

void Meric::readRegionParameters()
{
	std::vector<std::string> values;
	Tokenizer tokenizer("region.options");
	while (tokenizer.next() != Tokenizer::Token::END) {
		std::cout << tokenizer.value() << "\n";
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
				for (size_t i = 0; i < values.size(); i++) {
					std::cout << values[i] << " ";
				}
				std::cout << "\n";
				RegionParameters &region = Meric::parameter[values[0]];
				for (size_t i = 1; i < values.size(); i += 2) {
					if (StringCompare::caseInsensitiveEq(values[i], "FREQUENCY")) {
						std::stringstream ss(values[i + 1]);
						ss >> region.frequency;
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

Meric::Meric()
{
	if (Meric::CONTINUAL) {
		if (Meric::DETAILED) {
			_store = new DetailedTimeStore();
		} else {
			_store = new TimeStore();
		}
	} else {
		if (Meric::DETAILED) {
			_store = new DetailedEnergyStore();
		} else {
			_store = new EnergyStore();
		}
	}
}

