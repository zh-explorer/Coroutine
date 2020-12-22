//
// Created by explorer on 2020/12/9.
//

#ifndef COROUTINE_ASYNC_H
#define COROUTINE_ASYNC_H

#include "../Coroutine/Coroutine.h"
#include "../asyncIO/poll.h"
#include "../asyncIO/sock.h"
#include "../ThreadPool/ThreadExecutor.h"
#include "../ThreadPool/ThreadPool.h"
#include <vector>

class Event;

class EventLoop {
public:
    EventLoop();

    void loop();

    void add_to_loop(Coroutine *coro);

    void wait_event(Event *e, int timeout);

    ThreadPool *get_thread_pool() {
        return this->pool;
    }

    Epoll *get_epoll() {
        return this->epoll;
    }

    ~EventLoop();

private:
    Coroutine *current_coro{};

    void do_schedule();

    bool all_done();

    std::vector<Coroutine *> find_active_coro();

    void wait_active_coro();

    struct EventPair {
        Event *event;
        Coroutine *coro;

        EventPair(Event *e, Coroutine *c) {
            event = e;
            coro = c;
        }
    };

    typedef std::vector<EventPair> event_vector;
    std::map<time_t, event_vector> time_event_list;
    event_vector event_list;

    std::vector<Coroutine *> new_coro_list;   // this vector store the vector witch can run

    ThreadPool *pool;
    Epoll *epoll;
};


extern __thread EventLoop *current_loop;
#endif //COROUTINE_ASYNC_H
