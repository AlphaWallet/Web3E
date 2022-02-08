#ifndef ACTION_HANDLER_H
#define ACTION_HANDLER_H

#include <vector>

using namespace std;

typedef void (*callback_function)(void);

class ActionNode
{
public:
    unsigned long actionTime;
    uint8_t pinId;
    uint8_t pinValue;
    callback_function callback;
};

class ActionHandler
{
public:
    ActionHandler(uint8_t size);
    void AddEventAt(unsigned long actionTime, uint8_t pinId, int pinValue);
    void AddEventAfter(unsigned long durationMillis, uint8_t pinId, int pinValue);
    void CheckEvents(unsigned long currentTime);
    void AddCallback(unsigned long durationMillis, callback_function callback);
    bool detectCallback(callback_function callback);
    int  timeToCallback(callback_function callback);
    void RemoveAllEvents();

private:
    ActionNode *getNodeFromPool();
    void addEvent(ActionNode *event);

    vector<ActionNode*> actions;
    vector<ActionNode*> nodePool;
};

#endif