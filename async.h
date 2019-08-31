//
// Created by explorer on 8/22/19.
//

#ifndef COROUTINE_ASYNC_H
#define COROUTINE_ASYNC_H

#include <vector>
#include <map>
#include <ctime>
#include "poll.h"

class Coroutine;

class Future;

class Event;

void wait_event();

void schedule();

void asleep_to(time_t timestamp);

void asleep(int second);

class EventLoop {
public:
    void schedule();

    void add_to_poll(Coroutine *coro);

    void loop();

    void wait();

    void add_event(Event *e, int timeout = -1);

    EPoll poll;

    Coroutine *current_run{};
private:
    std::vector<Coroutine *> active_list;

    struct event_pair {
        Event *event;
        Coroutine *coro;

        event_pair(Event *e, Coroutine *c) {
            event = e;
            coro = c;
        }
    };

    typedef std::vector<event_pair> event_vector;
    std::map<time_t, event_vector> time_event_list;
    event_vector event_list;


    int wakeup_coro();
};

class Event {
public:
    virtual bool should_release() = 0;

};

class Future : public Event {
public:
    bool wait(int second = 0);

    void set();

    void clear();

    int is_set();

    bool should_release() override;

private:
    bool flag = false;

};

class Sleep : public Event {
public:
    void sleep(int second);

    void sleep_to(time_t timestamp);

    bool should_release() override;
};

class Lock : public Event {
public:
    bool acquire(bool blocking = true, int timeout = -1);

    void release();

    bool should_release() override;

private:
    bool is_locked = false;
};

class IO : public Event {
public:
    int fileno;

    enum poll_event event;

    int error_number;

    int have_result = false;

    explicit IO(int fd);

    void wait_read();

    void wait_write();

    void get_result(poll_result re);

    bool should_release() override {
        return have_result;
    };
};

extern EventLoop *current_event;
// need a lock
#endif //COROUTINE_ASYNC_H
