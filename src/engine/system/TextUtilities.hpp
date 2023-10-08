#pragma once
#include "Common.hpp"

/**
 \brief Provides utilities process strings.
 \ingroup System
 */
class TextUtilities {

public:
	/** Trim characters from both ends of a string.
	 \param str the string to trim from
	 \param del the characters to delete
	 \return the trimmed string
	 */
	static std::string trim(const std::string & str, const std::string & del);

	/** Remove file extension from the end of a string and extract it.
	 \param str the string to remove the extension from
	 \return the extension string
	 */
	static std::string splitExtension(std::string & str);

	/** Extract the filename from the end of a string (extension will be included).
	\param str the string to extract the filename from
	\return the filename string
	*/
	static std::string extractFilename(const std::string & str);

	/** Replace all occurences of a substring in a string by another string.
	 \param source the string in which substitutions should happen
	 \param fromString substring to replace
	 \param toString new substring to insert
	 */
	static void replace(std::string & source, const std::string & fromString, const std::string & toString);

	/** Replace all listed characters by another character in a string.
	 \param source the string in which substitutions should happen
	 \param fromChars list of characters
	 \param toChar new character to insert
	 */
	static void replace(std::string & source, const std::string & fromChars, const char toChar);

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
	
	/** Join a list of strings together using a custom delimiter.
	 \param tokens the list of strings to join
	 \param delimiter the string that will be inserted between each pair of strings
	 \return the result of the join
	 */
	static std::string join(const std::vector<std::string> & tokens, const std::string & delimiter);
	
	/** Split a string into a list of tokens based on a given delimiter.
	 \param str the string to split
	 \param delimiter the string that will be used as a splitting point
	 \param skipEmpty should empty tokens be ignored
	 \return a list of tokens
	 */
	static std::vector<std::string> split(const std::string & str, const std::string & delimiter, bool skipEmpty);
	
	/** Split a string into a list of lines. This function supports both '\\n' and '\\r\\n'.
	 \param str the string to split
	 \param skipEmpty should empty lines be ignored
	 \return a list of lines
	 */
	static std::vector<std::string> splitLines(const std::string & str, bool skipEmpty);

	/** Generate a zero-padded string representation of an unsigned integer.
	\param number the number to represent as a string
	\param padding the length to pad the number string to
	\return the number string padded representation
	*/
	static std::string padInt(uint number, uint padding);

	/** Convert all upper case characters in a string to lower case.
	 \param src the string to convert
	 \return the converted string
	 */
	static std::string lowercase(const std::string & src);

	/** Convert all lower case characters in a string to upper case.
	 \param src the string to convert
	 \return the converted string
	 */
	static std::string uppercase(const std::string & src);

};
