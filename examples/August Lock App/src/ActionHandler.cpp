#include "ActionHandler.h"
#include <Arduino.h>

ActionHandler::ActionHandler(uint8_t size)
{
    nodePool.reserve(size);
    for (int i = 0; i < size; i++)
    {
        ActionNode *node = new ActionNode();
        node->actionTime = 0;
        node->pinId = 0;
        node->pinValue = 0;
        nodePool.push_back(node);
    }
}

void ActionHandler::RemoveAllEvents()
{
    actions.clear();
}

ActionNode * ActionHandler::getNodeFromPool()
{
    for (auto itr = nodePool.begin(); itr != nodePool.end(); itr++)
    {
        if ((*itr)->actionTime == 0) return (ActionNode*)*itr;
    }

    //unable to get an action node. Grab the next node to complete and replace it - this way we ensure we always complete
    return (*actions.begin());
}

void ActionHandler::AddEventAt(unsigned long actionTime, uint8_t pinId, int pinValue)
{
    //get unused node from pool
    ActionNode *node = getNodeFromPool();
    if (node == NULL) return;

    node->actionTime = actionTime;
    node->pinId = pinId;
    node->pinValue = pinValue; 
    node->callback = 0;

    addEvent(node);
}

bool ActionHandler::detectCallback(callback_function callback)
{
    for (auto itr = nodePool.begin(); itr != nodePool.end(); itr++)
    {
        if ((*itr)->callback == callback)
        {
            return true;
        } 
    }

    return false;
}

int ActionHandler::timeToCallback(callback_function callback)
{
    for (auto itr = actions.rbegin(); itr != actions.rend(); itr++)
    {
        if ((*itr)->callback == callback)
        {
            int diff = (*itr)->actionTime - millis();
            return diff/1000;
        } 
    }

    return -1;
}

void ActionHandler::AddEventAfter(unsigned long durationMillis, uint8_t pinId, int pinValue)
{
    //get unused node from pool
    ActionNode *node = getNodeFromPool();
    if (node == NULL) return;

    unsigned long futureTime = millis() + durationMillis;

    node->actionTime = futureTime;
    node->pinId = pinId;
    node->pinValue = pinValue; 
    node->callback = 0;

    addEvent(node);
}

void ActionHandler::CheckEvents(unsigned long currentTime)
{
    if (actions.size() > 0)
    {
        auto firstNode = actions.begin();
        ActionNode *node = (*firstNode);
    
        if (currentTime > node->actionTime)
        {
            actions.erase(firstNode);

            if (node->callback != 0)
            {
                node->callback(); //NB this callback may add a node
            }

            if (node->pinId != 0)
            {
                Serial.print("Set PIN: ");
                Serial.print(node->pinId);
                Serial.print(" To : ");
                Serial.println(node->pinValue);
                digitalWrite(node->pinId, node->pinValue);
            }

            node->actionTime = 0; //reassign the node to available
            node->callback = 0;
            node->pinId = 0;
            node->pinValue = 0;
        }
    }
}

void ActionHandler::AddCallback(unsigned long durationMillis, callback_function callback)
{
    //get unused node from pool
    ActionNode *node = getNodeFromPool();
    if (node == NULL) return;

    unsigned long futureTime = millis() + durationMillis;

    node->actionTime = futureTime;
    node->pinId = 0;
    node->pinValue = 0; 
    node->callback = callback;

    addEvent(node);
}

void ActionHandler::addEvent(ActionNode *event)
{
    if (actions.size() == 0)
    {
        actions.push_back(event);
    }
    else 
    {
        for (std::vector<ActionNode*>::iterator itr = actions.begin(); itr != actions.end(); ++itr)
        //for (auto itr = actions.rbegin(); itr != actions.rend(); itr++)
        {
            if (event->actionTime < (*itr)->actionTime)
            {
                actions.insert(itr, event);
                return;
            }
        }

        //if it doesn't fit in the queue place at the end
        actions.push_back(event);
    }
}
