
#include "tokenizer.h"

using namespace meric;

static bool isWhiteSpace(int c)
{
	return !(('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || ('0' <= c && c <= '9') || c == '{' || c == '}' || c == '_');
}

static void skipWhiteSpaces(std::ifstream &is)
{
	while (isWhiteSpace(is.peek()) && !is.eof()) {
		is.get();
	}
}

Tokenizer::Token Tokenizer::_next()
{
	if (_file.eof()) {
		return Token::END;
	}
	skipWhiteSpaces(_file);
	_buffer.clear();

	while (!isWhiteSpace(_file.peek()) && !_file.eof()) {
		switch (_file.peek()) {
		case '{':
			if (!_buffer.size()) {
				_buffer.push_back(_file.get());
				return Token::OBJECT_OPEN;
			}
			return Token::STRING;
		case '}':
			if (!_buffer.size()) {
				_buffer.push_back(_file.get());
				return Token::OBJECT_CLOSE;
			}
			return Token::STRING;
		case EOF:
			return Token::END;
		default:
			_buffer.push_back(_file.get());
		}
	}

	return _buffer.size() ? Token::STRING : Token::END;
}





