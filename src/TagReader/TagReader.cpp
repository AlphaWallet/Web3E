/**
*	Author: James Brown 2018
*	Crawls a JSON looking for tags
*	Saves any allocation
*
*	TODO: Move all JSON handling to this class 
*		- pass a char* so terminators can be inserted and have all the value pairs stay within the original data, which is on stack
**/

#include "TagReader.h"
#include <string.h>
#include <Arduino.h>

using namespace std;

typedef enum STATE_DEF
{
	KEY,
	DELIMITER,
	VALUE,
	DEREFERENCE,
	END_VALUE
} READ_STATE;

TagReader::TagReader()
{
	_length = 0;
}

const char* TagReader::getTag(const string *JSON_Str, const char *value)
{
	//first determine if the tag exists in the file
	size_t index = JSON_Str->find(value);
	if (index == string::npos) //safe scan
	{
		return NULL;
	}

	//parse for the end value
	READ_STATE read_state = KEY;
	READ_STATE old_state;
	const char *value_ptr = NULL;
	_length = 0;

	while (index < JSON_Str->length() && read_state != END_VALUE)
	{
		const char c = (*JSON_Str).at(index);

		switch (read_state)
		{
		case KEY:
			if (c == '\"' || c == ':') read_state = DELIMITER;
			break;
		case DELIMITER:
			if (c != '\"' && c != ':') { read_state = VALUE; index--; }
			break;
		case VALUE:
			if (value_ptr == NULL) value_ptr = (const char*)(JSON_Str->c_str() + index);
			if (c == '\"')
			{
				read_state = END_VALUE;
			}
			else
			{
				_length++;
			}
			break;
		case DEREFERENCE:
			read_state = old_state;
			break;
		case END_VALUE:
			return value_ptr;
		}

		if (c == '\\')
		{
			old_state = read_state;
			read_state = DEREFERENCE;
		}
		index++;
	}

	return value_ptr;
}

/**
 * Build a std::vector of bytes32 as hex strings
 **/
vector<string>* TagReader::ConvertCharStrToVector32(const char *resultPtr, size_t resultSize, vector<string> *result) 
{
	if (resultSize < 64) return result;
    if (resultPtr[0] == '0' && resultPtr[1] == 'x') resultPtr += 2;
	//estimate size of return
	int returnSize = resultSize / 64;
	result->reserve(returnSize);
    int index = 0;
    char element[65];
    element[64] = 0;

    while (index <= (resultSize - 64))
    {
        memcpy(element, resultPtr, 64);
        result->push_back(string(element));
        resultPtr += 64;
        index += 64;
    }

	return result;
}