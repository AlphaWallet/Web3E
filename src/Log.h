//
// Created by Okada, Takahiro on 2018/02/04.
//

#ifndef ARDUINO_WEB3_LOG_H
#define ARDUINO_WEB3_LOG_H

#define DEBUGLOG

class Log {
public:
    void print(const char* s);
    void println(const char* s);
};

#endif //ARDUINO_WEB3_LOG_H
