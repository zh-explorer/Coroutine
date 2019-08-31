//
// Created by explorer on 19-8-31.
//

#ifndef COROUTINE_ATHREAD_H
#define COROUTINE_ATHREAD_H

#include <vector>

class Executor;

class aThread {
public:
    aThread();

    void execute(Executor *executor);

    void running();

    void is_busy();

private:
    Executor *curren_executor;

    std::vector<Executor *> wait_list;
};


void thread_caller(aThread *thread);

#endif //COROUTINE_ATHREAD_H
