#ifndef COROUTINE_ASYNCTHREAD_HPP
#define COROUTINE_ASYNCTHREAD_HPP

#include "../ThreadPool/ThreadExecutor.h"
#include "../unit/func.h"
#include "../unit/log.h"

template<class R>
class AsyncThread : public Event, public ThreadExecutor {
public:
    explicit AsyncThread(Func *func) {
        f = func;
        this->new_f = false;
    }

    AsyncThread(AsyncThread &) = delete;

    AsyncThread(AsyncThread &&) = delete;

    template<class ... ARGS>
    explicit AsyncThread(R (*func)(ARGS ...), ARGS ... args) {
        auto function = new_func(func, args ...);
        this->f = function;
    }

    template<class ... ARGS>
    explicit AsyncThread(std::function<R(ARGS ...)> func, ARGS ... args) {
        auto function = new_func(func, args ...);
        this->f = function;
    }

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

    bool event_set() {
        this->is_finish();
    }

    void exec() override {
        r = (*f)();
    }

    virtual ~AsyncThread() {
        if (!this->is_finish()) {
            this->cancel();
        }
        if (this->new_f) {
            delete this->f;
        }
    }

private:
    bool new_f = true;
    R r{};
    Func *f;
};

#endif //COROUTINE_ASYNCTHREAD_HPP