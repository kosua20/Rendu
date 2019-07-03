#pragma once
#include "Common.hpp"


/**
 \brief Provides utilities process strings.
 \ingroup Helpers
 */
class TextUtilities {

public:
	
	/** Trim characters from both ends of a string.
	 \param str the string to trim from
	 \param del the characters to delete
	 \return the trimmed string
	 */
	static std::string trim(const std::string & str, const std::string & del);
	
	/** Remove file extension from the end of a string.
	 \param str the string to remove the extension from
	 \return the extension string
	 */
	static std::string removeExtension(std::string & str);
	
	/** Replace all occurences of a substring in a string by another string.
	 \param source the string in which substitutions should happen
	 \param fromString substring to replace
	 \param toString new substring to insert
	 */
	static void replace(std::string & source, const std::string& fromString, const std::string & toString);
	
	/** Test if a string is a prefix of another string.
	 \param source the string to examine
	 \param prefix the prefix string to test
	 \return true if the prefix is here
	 */
	static bool hasPrefix(const std::string & source, const std::string & prefix);
	
	/** Test if a string is a suffix of another string.
	 \param source the string to examine
	 \param suffix the suffix string to test
	 \return true if the suffix is here
	 */
	static bool hasSuffix(const std::string & source, const std::string & suffix);
	
};
