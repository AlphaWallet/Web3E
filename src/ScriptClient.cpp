#include <ScriptClient.h>

static std::string *sc_data = NULL;
static APIReturn *apiReturn = NULL;

void ScriptClient::checkClientAPI(const char *route, APICallback callback)
{
    boolean currentLineIsBlank = true;
    String currentLine = ""; // make a String to hold incoming data from the client
    int timeout = 0;
    if (sc_data == NULL) sc_data = new std::string();

    while (connected())
    {   
        if (available())
        {
            timeout = 0;
            // if there's bytes to read from the client,
            char c = read();        // read a byte, then
            Serial.write(c);        // print it out the serial monitor
            *sc_data += c;
            if (c == '\n' && currentLineIsBlank)
            {
                scanAPI(route, callback);
                break;
            }
            if (c == '\n')
            { // if you got a newline, then clear currentLine
                currentLine = "";
                currentLineIsBlank = true;
            }
            else if (c != '\r')
            {
                currentLineIsBlank = false;
                currentLine += c;
            }
        }
        else
        {
            delay(1);
            if (timeout++ > 500)
                break;
        }
    }

    //Ensure connection is always closed after opening
    closeConnection();
}

void ScriptClient::scanAPI(const char *route, APICallback callback)
{
    println("HTTP/1.1 200 OK");
    println("Content-type:text/html");
    println("Access-Control-Allow-Origin: *");
    println("Connection: close");
    println();

    if (sc_data->find("favicon") != std::string::npos)
    {
        return;
    }

    if (apiReturn == NULL) apiReturn = new APIReturn();
    apiReturn->clear();

	size_t parseIndex = sc_data->find(route);
    size_t endParseIndex = sc_data->find(" HTTP/");
	if (endParseIndex == std::string::npos) endParseIndex = sc_data->find(" HTTPS/");
	sc_data->erase(endParseIndex, sc_data->size() - endParseIndex);
	parseIndex += strlen(route);

	if (parseIndex != std::string::npos)
	{
		size_t routeNameIndex = sc_data->find_first_of("?", parseIndex);
        if (routeNameIndex == std::string::npos) routeNameIndex = sc_data->size();

		apiReturn->apiName = sc_data->substr(parseIndex, routeNameIndex - parseIndex);

		parseIndex = routeNameIndex + 1;

		while (parseIndex < sc_data->size() && parseIndex < endParseIndex)
		{
			size_t keySeparatorIndex = sc_data->find_first_of("=", parseIndex);
			size_t paramEndIndex = sc_data->find_first_of("&", keySeparatorIndex);
			if (keySeparatorIndex == std::string::npos) break;
			if (paramEndIndex == std::string::npos) paramEndIndex = sc_data->size();
			std::string key = sc_data->substr(parseIndex, keySeparatorIndex - parseIndex);
			keySeparatorIndex++;
			std::string param = sc_data->substr(keySeparatorIndex, paramEndIndex - keySeparatorIndex);
			apiReturn->params[key] = param;
			parseIndex = paramEndIndex + 1;
		}

        //call main program API handler
        callback(apiReturn, this);
	}
}

void ScriptClient::closeConnection()
{
    *sc_data = "";
    // Close the connection
    delay(1);
    flush();
    stop();
}