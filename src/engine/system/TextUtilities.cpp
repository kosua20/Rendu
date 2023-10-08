#include "system/TextUtilities.hpp"

std::string TextUtilities::trim(const std::string & str, const std::string & del) {
	const size_t firstNotDel = str.find_first_not_of(del);
	if(firstNotDel == std::string::npos) {
		return "";
	}
	const size_t lastNotDel = str.find_last_not_of(del);
	return str.substr(firstNotDel, lastNotDel - firstNotDel + 1);
}

std::string TextUtilities::splitExtension(std::string & str) {
	const std::string::size_type pos = str.find_last_of('.');
	if(pos == std::string::npos) {
		return "";
	}
	const std::string ext(str.substr(pos));
	str.erase(str.begin() + pos, str.end());
	return ext;
}

std::string TextUtilities::extractFilename(const std::string & str){
	const std::string::size_type loc = str.find_last_of("/\\");
	if(loc == std::string::npos) {
		return str;
	}
	return str.substr(loc+1);
}

void TextUtilities::replace(std::string & source, const std::string & fromString, const std::string & toString) {
	std::string::size_type nextPos = 0;
	const size_t fromSize		   = fromString.size();
	const size_t toSize			   = toString.size();
	while((nextPos = source.find(fromString, nextPos)) != std::string::npos) {
		source.replace(nextPos, fromSize, toString);
		nextPos += toSize;
	}
}

void TextUtilities::replace(std::string & source, const std::string & fromChars, const char toChar) {
	std::string::size_type nextPos = 0;
	while((nextPos = source.find_first_of(fromChars, nextPos)) != std::string::npos) {
		source[nextPos] = toChar;
		nextPos += 1;
	}
}

bool TextUtilities::hasPrefix(const std::string & source, const std::string & prefix) {
	if(prefix.empty() || source.empty()) {
		return false;
	}
	if(prefix.size() > source.size()) {
		return false;
	}
	const std::string sourcePrefix = source.substr(0, prefix.size());
	return sourcePrefix == prefix;
}

bool TextUtilities::hasSuffix(const std::string & source, const std::string & suffix) {
	if(suffix.empty() || source.empty()) {
		return false;
	}
	if(suffix.size() > source.size()) {
		return false;
	}
	const std::string sourceSuffix = source.substr(source.size() - suffix.size(), suffix.size());
	return sourceSuffix == suffix;
}

std::string TextUtilities::join(const std::vector<std::string> & tokens, const std::string & delimiter){
	std::string accum;
	for(size_t i = 0; i < tokens.size(); ++i){
		accum.append(tokens[i]);
		if(i != (tokens.size() - 1)){
			accum.append(delimiter);
		}
	}
	return accum;
}

std::vector<std::string> TextUtilities::split(const std::string & str, const std::string & delimiter, bool skipEmpty){
	std::string subdelimiter = " ";
	if(delimiter.empty()){
		Log::Warning() << "Delimiter is empty, using space as a delimiter." << std::endl;
	} else {
		subdelimiter = delimiter.substr(0,1);
	}
	if(delimiter.size() > 1){
		Log::Warning() << "Only the first character of the delimiter will be used (" << delimiter[0] << ")." << std::endl;
	}
	std::stringstream sstr(str);
	std::string value;
	std::vector<std::string> tokens;
	while(std::getline(sstr, value, subdelimiter[0])) {
		if(!skipEmpty || !value.empty()) {
			tokens.emplace_back(value);
		}
	}
	return tokens;
}

std::vector<std::string> TextUtilities::splitLines(const std::string & str, bool skipEmpty){

	std::stringstream sstr(str);
	std::string value;
	std::vector<std::string> tokens;
	while(std::getline(sstr, value)) {
		value = TextUtilities::trim(value, "\r");
		if(!skipEmpty || !value.empty()) {
			tokens.emplace_back(value);
		}
	}
	return tokens;
}

std::string TextUtilities::padInt(uint number, uint padding) {
	const std::string numStr = std::to_string(number);
	const int delta			 = int(padding) - int(numStr.size());
	return delta <= 0 ? numStr : (std::string(delta, '0') + numStr);
}

std::string TextUtilities::lowercase(const std::string & src){
	std::string dst(src);;
	std::transform(src.begin(), src.end(), dst.begin(),
				   [](unsigned char c){
		return std::tolower(c);
	});
	return dst;
}

std::string TextUtilities::uppercase( const std::string& src )
{
	std::string dst( src );;
	std::transform( src.begin(), src.end(), dst.begin(),
					[] ( unsigned char c )
	{
		return std::toupper( c );
	} );
	return dst;
}
