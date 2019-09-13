//
// Created by explorer on 8/22/19.
//

#ifndef COROUTINE_COROUTINE_H
#define COROUTINE_COROUTINE_H

#include <cstdlib>
#include <cstdint>
#include <csetjmp>
#include "async.h"
#include "unit/func.h"

enum coro_STATUS {
    INIT,
    RUNNING,
    END,
};

class Coroutine {
public:
    explicit Coroutine(Func *func, size_t stack_size = 0x200000);

    template<class R, class ... ARGS>
    explicit Coroutine(R (*func)(ARGS ...), ARGS ... args) {
        auto f = new_func(func, args ...);
        this->new_call_func = true;
        init(f);
    }

    template<class R, class ... ARGS>
    explicit Coroutine(std::function<R(ARGS...)> func, ARGS ... args) {
        auto f = new_func(func, args ...);
        this->new_call_func = true;
        init(f);
    }

    template<class R, class ... ARGS>
    Coroutine(size_t stack_size, R (*func)(ARGS ...), ARGS ... args) {
        auto f = new_func(func, args ...);
        this->new_call_func = true;
        init(f, stack_size);
    }

    template<class R, class ... ARGS>
    Coroutine(size_t stack_size, std::function<R(ARGS...)> func, ARGS ... args) {
        auto f = new_func(func, args ...);
        this->new_call_func = true;
        init(f, stack_size);
    }

    void init(Func *func, size_t stack_size = 0x200000);

    ~Coroutine();

    void schedule_back();

    void start_run();

    void call();

    void func_stop();

    template<class R, class ... ARGS>
    void add_done_callback(R (*func)(Coroutine *, ARGS...), ARGS ... args) {
        auto f = new_func(func, this, args...);
        this->new_done_func = true;
        add_done_callback(f);
    }

    template<class R, class ... ARGS>
    void add_done_callback(std::function<R(ARGS...)> func, ARGS ... args) {
        auto f = new_func(func, this, args...);
        this->new_done_func = true;
        add_done_callback(f);
    }

    void add_done_callback(Func *func) {
        this->done_func = func;
    }

    void continue_run();

    void operator()();

    EventLoop *loop{};
    jmp_buf main_env{};
    enum coro_STATUS coro_status = INIT;
    void *ret{};
    jmp_buf coro_env{};


private:
    time_t run_time{};
    size_t stack_size{};
    unsigned char *stack_end{};
    unsigned char *stack_start{};

    bool new_call_func = false;
    Func *call_func{};
    bool new_done_func = false;
    Func *done_func = nullptr;
};


#endif //COROUTINE_COROUTINE_H
