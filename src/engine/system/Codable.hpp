#pragma once

#include "system/Config.hpp"
#include "graphics/GPUObjects.hpp"
#include "Common.hpp"

class Texture;

/** \brief Provides helpers for serialization/deserialization of basic types.
 	\ingroup System
 */
class Codable {
public:
	/** Decode a boolean from a parameters tuple, at a specified position.
	A boolean is set to true if its value is 'true', 'True', 'yes', 'Yes' or '1'.
	 \param param the parameters tuple
	 \param position index of the boolean in the values array
	 \return a boolean
	 */
	static bool decodeBool(const KeyValues & param, unsigned int position = 0);
	
	/** Encode a boolean into a string ("true" or "false")
	 \param b the boolean value to encode
	 \return the string representation
	 */
	static std::string encode(bool b);
	
	/** Decode a 3D vector from a parameters tuple, starting at a specified position.
	 A 3D vector is described as a set of 3 floats separated by spaces or commas: X,Y,Z.
	 \param param the parameters tuple
	 \param position index of the first vector component in the values array
	 \return a 3D vector
	 */
	static glm::vec3 decodeVec3(const KeyValues & param, unsigned int position = 0);
	
	/** Encode a 3D vector into a comma-separated string "X,Y,Z"
	\param v the vector to encode
	\return the string representation
	*/
	static std::string encode(const glm::vec3 & v);
	
	/** Decode a 2D vector from a parameters tuple, starting at a specified position.
	 A 2D vector is described as a set of 2 floats separated by spaces or commas: X,Y.
	 \param param the parameters tuple
	 \param position index of the first vector component in the values array
	 \return a 2D vector
	 */
	static glm::vec2 decodeVec2(const KeyValues & param, unsigned int position = 0);
	
	/** Encode a 2D vector into a comma-separated string "X,Y"
	\param v the vector to encode
	\return the string representation
	*/
	static std::string encode(const glm::vec2 & v);

	/** Decode a transformation from a series of parameters tuple. For now this function look for three keywords in the whole params vector: translation, orientation, scaling.  A transformation is described as follows:
	 \verbatim
	 translation: X,Y,Z
	 orientation: axisX,axisY,axisZ angle
	 scaling: scale
	 \endverbatim
	 \param params a list of parameters tuples
	 \return the 4x4 matrix representation of the transformation
	 */
	static glm::mat4 decodeTransformation(const std::vector<KeyValues> & params);
	
	/** Encode a rotation/scaling/orientation transformation matrix into a set of parameters.
	 \param transfo the transformation to encode
	 \return a list of 3 Codable-comaptible parameters
	 */
	static std::vector<KeyValues> encode(const glm::mat4 & transfo);

	/** Decode a texture from a parameters tuple and load it. A texture is described as follows:
	 \verbatim
	 texturetype: texturename
	 \endverbatim
	 (where texturetype can be one of 'rgb', 'srgb', 'rgb32', 'rgbcube', 'srgbcube', 'rgb32cube' depending on the desired format).
	 \param param the parameters tuple
	 \return the name of the texture and its descriptor.
	 */
	static std::pair<std::string, Descriptor> decodeTexture(const KeyValues & param);
	
	/** Encode a texture into a Codable-compliant tuple.
	 \param texture the texture to encode
	 \return the tuple representation
	 */
	static KeyValues encode(const Texture * texture);

	/** Split a Codable-compatible text file in a hierarchical list of (key,values) tuples, getting rid of extraneous spaces and punctuations. The following rules are applied:
	 - elements beginning with a '*' denote root-level objects.
	 - elements beginning with a '-' belong to an array, defined by the element just before those.
	 - elements can be nested on the same line: 'elem1: elem2: values'
	 \param codableFile the text content to parse
	 \return a hierarchical list of (key, value) tokens
	 */
	static std::vector<KeyValues> decode(const std::string & codableFile);
	
	/** Generate a Codable-compatible text representation from a hierarchical list of (key,values) tuples. The following rules are applied:
	 - elements beginning with a '*' denote root-level objects.
	 - elements beginning with a '-' belong to an array, defined by the element just before those.
	 \param params a hierarchical list of (key, value) tokens
	 \return a string containing the text representation
	 */
	static std::string encode(const std::vector<KeyValues> & params);

	/** Generate a Codable-compatible text representation from a (key,values) tuple. The following rules are applied:
	\param params a (key, value) token
	 \return a string containing the text representation
	 */
	static std::string encode(const KeyValues & params);
	
private:
	
	/** \brief Prefix type/ */
	enum class Prefix : uint {
		ROOT, ///< "*" prefix.
		LIST, ///< "-" prefix.
		NONE ///< No prefix.
	};
	
	/** Generate a Codable-compatible text representation from a hierarchical list of (key,values) tuples.
	 \param params a hierarchical list of (key, value) tokens
	 \param prefix the type of prefix character to use (\see Prefix)
	 \param level the indentation level (using tabulations)
	 \return a string containing the text represenation
	*/
	static std::string encode(const std::vector<KeyValues> & params, Prefix prefix, uint level);
	
};
