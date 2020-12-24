#ifndef COROUTINE_ASYNCTHREAD_HPP
#define COROUTINE_ASYNCTHREAD_HPP

#include "ThreadExecutor.h"
#include "ThreadPool.h"
#include "func.h"
#include "log.h"
#include "Event.h"

template<class R>
class _AsyncThread : public Event, public ThreadExecutor {
public:
    explicit _AsyncThread(Func<R> *func) {
        f = func;
        this->new_f = false;
    }

    _AsyncThread(_AsyncThread &) = delete;

    _AsyncThread(_AsyncThread &&) = delete;

    template<class ... ARGS>
    explicit _AsyncThread(R (*func)(ARGS ...), ARGS ... args) {
        auto function = new_func(func, args ...);
        this->f = function;
    }

    template<class ... ARGS>
    explicit _AsyncThread(std::function<R(ARGS ...)> func, ARGS ... args) {
        auto function = new_func(func, args ...);
        this->f = function;
    }

    bool event_set() override {
        return this->is_finish();
    }

    virtual ~_AsyncThread() {
        if (!this->is_finish()) {
            this->cancel();
        }
        if (this->new_f) {
            delete this->f;
        }
    }

protected:
    bool new_f = true;
    Func<R> *f;
};

template<class R>
class AsyncThread : public _AsyncThread<R> {
public:
    template<class ... ARGS>
    explicit AsyncThread(R (*func)(ARGS ...), ARGS ... args) : _AsyncThread<R>(func, args ...) {};

    template<class ... ARGS>
    explicit AsyncThread(std::function<R(ARGS ...)> func, ARGS ... args) : _AsyncThread<R>(func, args ...) {}

    R start(int timeout = -1) {
        // add to loop and wait
        ThreadPool *pool = current_loop->get_thread_pool();
        pool->submit(this);
        current_loop->wait_event(this, timeout);
        if (!this->is_finish()) {
            logger(WARN, stderr, "thread timeout, cancel it");
            this->cancel();
        }
        return r;
    }

    void exec() {
        r = (*(this->f))();
    }

private:
    R r{};
};

template<>
class AsyncThread<void> : public _AsyncThread<void> {
public:
    template<class ... ARGS>
    explicit AsyncThread(void (*func)(ARGS ...), ARGS ... args) : _AsyncThread<void>(func, args ...) {};

    template<class ... ARGS>
    explicit AsyncThread(std::function<void(ARGS ...)> func, ARGS ... args) : _AsyncThread<void>(func, args ...) {}

    void start(int timeout = -1) {
        // add to loop and wait
        ThreadPool *pool = current_loop->get_thread_pool();
        pool->submit(this);
        current_loop->wait_event(this, timeout);
        if (!this->is_finish()) {
            logger(WARN, stderr, "thread timeout, cancel it");
            this->cancel();
        }
    }

    void exec() override {
        (*(this->f))();
    }
};


#endif //COROUTINE_ASYNCTHREAD_HPP