
#include "tokenizer.h"

using namespace meric;

static bool isWhiteSpace(int c)
{
	return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

static bool isDelimiter(int c)
{
	return c == ',';
}

static bool isAssign(int c)
{
	return c == '=' || c == ':';
}

static bool isLineEnd(int c)
{
	return c == ';';
}

static bool isObjectOpen(int c)
{
	return c == '{';
}

static bool isObjectClose(int c)
{
	return c == '}';
}

static bool isSingleCommentChar(int c)
{
	return c == '#';
}

static bool isLineComment(int last, int currect)
{
	return last == '/' && currect == '/';
}

static bool isMultiLineCommentStart(int last, int currect)
{
	return last == '/' && currect == '*';
}

static bool isMultiLineCommentEnd(int last, int currect)
{
	return last == '*' && currect == '/';
}

static void skipWhiteSpaces(std::ifstream &is)
{
	while (isWhiteSpace(is.peek()) && !is.eof()) {
		is.get();
	}
}

Tokenizer::Token Tokenizer::_next()
{
	if (!_file.is_open()) {
		return Token::END;
	}
	if (_file.eof()) {
		return Token::END;
	}
	skipWhiteSpaces(_file);
	_buffer.clear();

	auto specialToken = [&] (Token token) {
		if (!_buffer.size()) {
			_buffer.push_back(_file.get());
			return token;
		}
		return Token::STRING;
	};

	auto skipLine = [&] () {
		while(_file.peek() != '\n' && _file.peek() != '\r') {
			_file.get();
		}
		while(_file.peek() == '\n' || _file.peek() == '\r') {
			_file.get();
		}
	};

	while (!isWhiteSpace(_file.peek()) && !_file.eof()) {
		if (isSingleCommentChar(_file.peek())) {
			skipLine();
			if (_buffer.size()) {
				return Token::STRING;
			}
			continue;
		}
		if (_buffer.size() && isLineComment(_buffer.back(), _file.peek())) {
			_buffer.pop_back();
			skipLine();
			if (_buffer.size()) {
				return Token::STRING;
			}
			continue;
		}
		if (_buffer.size() && isMultiLineCommentStart(_buffer.back(), _file.peek())) {
			_buffer.pop_back();
			_file.get();
			int last = _file.get();
			while(!isMultiLineCommentEnd(last, _file.peek())) {
				last = _file.get();
			}
			_file.get();
			continue;
		}
		if (isDelimiter(_file.peek())) {
			return specialToken(Token::DELIMITER);
		}
		if (isAssign(_file.peek())) {
			return specialToken(Token::ASSIGN);
		}
		if (isLineEnd(_file.peek())) {
			return specialToken(Token::LINE_END);
		}
		if (isObjectOpen(_file.peek())) {
			return specialToken(Token::OBJECT_OPEN);
		}
		if (isObjectClose(_file.peek())) {
			return specialToken(Token::OBJECT_CLOSE);
		}
		_buffer.push_back(_file.get());
	}

	return _buffer.size() ? Token::STRING : Token::END;
}





