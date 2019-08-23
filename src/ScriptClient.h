#ifndef SCRIPT_CLIENT_H
#define SCRIPT_CLIENT_H
#include <WiFiClient.h>
#include <string>
#include "APIReturn.h"

class ScriptClient;
//Format for API handler is void YourAPIHandler(APIReturn *apiReturn, ScriptClient *client) { ... }
typedef void (*APICallback)(APIReturn *, ScriptClient *);

class ScriptClient : public WiFiClient
{
public:
	void checkClientAPI(const char *route, APICallback callback);
	
private:
	void scanAPI(const char *route, APICallback callback);
	void closeConnection();
};

#endif