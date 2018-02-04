#ifndef ImageUtilities_h
#define ImageUtilities_h
#include <gl3w/gl3w.h>
#include <string>
#include <vector>


class ImageUtilities {

	
public:
	
	static int loadLDRImage(const std::string & path, unsigned int & width, unsigned int & height, unsigned int & channels, unsigned char **data, const bool flip);
	
	static int loadHDRImage(const std::string & path, unsigned int & width, unsigned int & height, unsigned int & channels, float **data, const bool flip);
	
	static int saveLDRImage(const std::string & path, const unsigned int width, const unsigned int height, const unsigned int channels, const unsigned char *data, const bool flip, const bool ignoreAlpha = false);
	
	static int saveHDRImage(const std::string & path, const unsigned int width, const unsigned int height, const unsigned int channels, const float *data, const bool flip, const bool ignoreAlpha = false);
	
private:
	
};


#endif
