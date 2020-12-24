//
// Created by explorer on 2020/12/9.
//

#ifndef COROUTINE_EVENT_H
#define COROUTINE_EVENT_H

class Event {
public:
    Event() = default;;

    Event(Event &) = delete;

    Event(Event &&) = delete;

    virtual bool event_set() = 0;
};

#endif //COROUTINE_EVENT_H
