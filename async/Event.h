//
// Created by explorer on 2020/12/9.
//

#ifndef COROUTINE_EVENT_H
#define COROUTINE_EVENT_H

#include "./async.h"

class Event {
public:
    Event() = default;;

    Event(Event &) = delete;

    Event(Event &&) = delete;

    virtual bool event_set() = 0;
};

#include "./Event.hpp"
#include "./AsyncThread.hpp"
#include "../asyncIO/ASock.hpp"

#endif //COROUTINE_EVENT_H
