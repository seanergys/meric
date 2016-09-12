
#ifndef SRC_BASIS_TOKENIZER_H_
#define SRC_BASIS_TOKENIZER_H_

#include <string>
#include <vector>
#include <fstream>
#include <iostream>

namespace meric {

struct StringCompare {

	bool operator()(const std::string &s1, const std::string &s2)
	{
		return caseInsensitive(s1, s2);
	}

	static bool caseSensitiveEq(const std::string &s1, const std::string &s2)
	{
		return s1.size() == s2.size() && std::equal(s1.begin(), s1.end(), s2.begin());
	}

	static bool caseInsensitiveEq(const std::string &s1, const std::string &s2)
	{
		return s1.size() == s2.size() && std::equal(s1.begin(), s1.end(), s2.begin(), _equals);
	}

	static bool caseSensitive(const std::string &s1, const std::string &s2)
	{
		return std::lexicographical_compare(s1.begin(), s1.end(), s2.begin(), s2.end());
	}

	static bool caseInsensitive(const std::string &s1, const std::string &s2)
	{
		return std::lexicographical_compare(s1.begin(), s1.end(), s2.begin(), s2.end(), _caseInsensitive);
	}

private:
	static bool _caseInsensitive(const char &c1, const char &c2)
	{
		return std::tolower(c1) < std::tolower(c2);
	}

	static bool _equals(const char &c1, const char &c2)
	{
		return std::tolower(c1) == std::tolower(c2);
	}
};

class Tokenizer {
public:
	enum class Token {
		STRING,
		OBJECT_OPEN,
		OBJECT_CLOSE,
		END
	};

	Tokenizer(const std::string &file): _file(file), _token(Token::END)
	{
		if (!_file.is_open()) {
			std::cerr << "Meric: cannot open file '" << file << "'\n";
			exit(EXIT_FAILURE);
		}
		_buffer.reserve(50);
	}

	~Tokenizer() { _file.close(); }

	Token next() { return _token = _next(); }
	Token token() { return _token; }
	std::string value()
	{
		return std::string(_buffer.begin(), _buffer.end());
	}

protected:
	Token _next();
	std::ifstream _file;
	Token _token;
	std::vector<int> _buffer;

};

}



#endif /* SRC_BASIS_TOKENIZER_H_ */
