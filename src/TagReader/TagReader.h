#ifndef WEB3_TAG_READER_H
#define WEB3_TAG_READER_H

#include <string>
#include <vector>

using namespace std;

class TagReader
{
public:
	TagReader();
	const char 	* getTag(const string * JSON_Str, const char * value);
	size_t		  length() { return _length; }
	
private:
	size_t _length;

};



#endif // !WEB3_TAG_READER
