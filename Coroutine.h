//
// Created by explorer on 8/22/19.
//

#ifndef COROUTINE_COROUTINE_H
#define COROUTINE_COROUTINE_H

#include <cstdlib>
#include <cstdint>
#include <csetjmp>
#include "async.h"

enum coro_STATUS {
    INIT,
    RUNNING,
    END,
};

class Coroutine {
public:
    explicit Coroutine(void *(*func)(void *), void *arg, size_t stack_size = 0x20000);

    void schedule_back();

    void func_run();

    void func_stop();

    void add_done_callback(void (*func)(Coroutine *, void *), void *data);

    void continue_run();

    EventLoop *loop;
    void *arg;
    jmp_buf main_env;
    enum coro_STATUS coro_status = INIT;
    void *ret;
    jmp_buf coro_env;

private:
    size_t stack_size;
    unsigned char *stack_end;
    unsigned char *stack_start;

    void *(*call_func)(void *);

    void (*done_func)(Coroutine *, void *) = NULL;

    void *done_callback_data;
};


#endif //COROUTINE_COROUTINE_H
