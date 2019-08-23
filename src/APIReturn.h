#ifndef _API_RETURN_H
#define _API_RETURN_H
#include <string>
#include <map>

class APIReturn
{
public:
    APIReturn() { }
    void clear() { apiName = ""; params.clear(); }

    std::string apiName;
    std::map <std::string, std::string> params;
};

#endif