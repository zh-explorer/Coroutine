//
// Created by explorer on 8/22/19.
//

#ifndef COROUTINE_ASYNC_H
#define COROUTINE_ASYNC_H

#include <vector>
#include <map>
#include <ctime>
#include "poll.h"
#include <queue>
#include "aThread.h"
#include "unit/func.h"

#define THREAD_POLL_SIZE 10

class Coroutine;

class Future;

class Event;

class Executor;

void add_to_poll(Coroutine *coro);

void wait_event();

void schedule();

void asleep_to(time_t timestamp);

void asleep(int second);

void wakeup_notify();

void add_event(Event *e, int timeout);

class EventLoop {
public:
    EventLoop();

    void schedule();

    void add_to_poll(Coroutine *coro);

    void loop();

    void wait();

    void add_event(Event *e, int timeout = -1);

    void add_executor(Executor *executor);

    void add_thread(aThread *thread);

    // notify the main thread up and exit the epoll_wait
    void wakeup_notify();

    EPoll poll;

    Coroutine *current_run{};
private:
    int thread_count = 0;
    pthread_t thread_id;
    bool event_change = false;
    std::vector<aThread *> thread_pool;

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

    virtual ~Event() = default;
};

class Future : public Event {
public:
    bool wait(int second = -1);

    void set(void *value = nullptr);

    void clear();

    int is_set();

    bool should_release() override;

    void *val{};
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

class Executor : public Event {
public:
    explicit Executor(Func *func);

    ~Executor() {
        if (new_call_func) {
            delete call_func;
        }
    }

    template<class R, class ... ARGS>
    explicit Executor(R (*func)(ARGS ...), ARGS ... args) {
        auto f = new_func(func, args ...);
        this->new_call_func = true;
        this->call_func = f;
    }

    template<class R, class ... ARGS>
    explicit Executor(std::function<R(ARGS...)> func, ARGS ... args) {
        auto f = new_func(func, args ...);
        this->new_call_func = true;
        this->call_func = f;
    }

    void run();

    bool should_release() override;

    void force_stop();

    // the thread that run this executor
    aThread *thread = nullptr;

    bool new_call_func = false;
    Func *call_func;

    int error = 0;

    bool force_stopped = false;
};

extern EventLoop *current_event;
#endif //COROUTINE_ASYNC_H
