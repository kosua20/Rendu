#ifndef ImageUtilities_h
#define ImageUtilities_h
#include <gl3w/gl3w.h>
#include <string>
#include <vector>


class ImageUtilities {

	
public:
	
	static bool isHDR(const std::string & path);
	
	static int loadImage(const std::string & path, unsigned int & width, unsigned int & height, unsigned int & channels, void **data, const bool flip, const bool externalFile = false);
	
	static int saveLDRImage(const std::string & path, const unsigned int width, const unsigned int height, const unsigned int channels, const unsigned char *data, const bool flip, const bool ignoreAlpha = false);
	
	static int saveHDRImage(const std::string & path, const unsigned int width, const unsigned int height, const unsigned int channels, const float *data, const bool flip, const bool ignoreAlpha = false);
	
private:
	
	static int loadLDRImage(const std::string & path, unsigned int & width, unsigned int & height, unsigned int & channels, unsigned char **data, const bool flip, const bool externalFile);
	
	static int loadHDRImage(const std::string & path, unsigned int & width, unsigned int & height, unsigned int & channels, float **data, const bool flip, const bool externalFile);
	
};


#endif
