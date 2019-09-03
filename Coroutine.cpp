//
// Created by explorer on 8/22/19.
//

#include "Coroutine.h"
#include <sys/mman.h>
#include <csetjmp>
#include "async.h"

void start_func(Coroutine *coroutine, void *(*func)(void *));

struct regs {
    uint64_t rax;
    uint64_t rbx;
    uint64_t rcx;
    uint64_t rdx;
    uint64_t rsi;
    uint64_t rdi;
    uint64_t r8;
    uint64_t r9;
    uint64_t r10;
    uint64_t r11;
    uint64_t r12;
    uint64_t r13;
    uint64_t r14;
    uint64_t r15;
    uint64_t rbp;
    uint64_t rsp;
};

#define save_all_reg(regs)  \
    asm("movq %%rax, %0;\n" \
    "movq %%rbx, %1;\n"     \
    "movq %%rcx, %2;\n"     \
    "movq %%rdx, %3;\n"     \
    "movq %%rsi, %4;\n"     \
    "movq %%rdi, %5;\n"     \
    : "=m"((regs)->rax), "=m"((regs)->rbx), "=m"((regs)->rcx), "=m"((regs)->rdx), "=m"((regs)->rsi), "=m"((regs)->rdi)      \
    );      \
    asm("movq %%r8, %0;\n"      \
        "movq %%r9, %1;\n"      \
        "movq %%r10, %2;\n"     \
        "movq %%r11, %3;\n"     \
        "movq %%r12, %4;\n"     \
        "movq %%r13, %5;\n"     \
        "movq %%r14, %6;\n"     \
        "movq %%r15, %7;\n"     \
    : "=m"((regs)->r8), "=m"((regs)->r9), "=m"((regs)->r10), "=m"((regs)->r11), "=m"((regs)->r12), "=m"((regs)->r13), "=m"((regs)->r14), "=m"((regs)->r15)      \
    );  \
    asm("movq %%rbp, %0;\n" \
        "movq %%rsp, %1;\n"      \
    :"=m"((regs)->rbp), "=m"((regs)->rsp));

#define restore_all_reg(regs)  \
    asm("movq %0, %%rax;\n" \
    "movq %1, %%rbx;\n"     \
    "movq %2, %%rcx;\n"     \
    "movq %3, %%rdx;\n"     \
    "movq %4, %%rsi;\n"     \
    "movq %5, %%rdi;\n"     \
    :               \
    : "m"((regs)->rax), "m"((regs)->rbx), "m"((regs)->rcx), "m"((regs)->rdx), "m"((regs)->rsi), "m"((regs)->rdi)      \
    );      \
    asm("movq %0, %%r8;\n"      \
        "movq %1, %%r9;\n"      \
        "movq %2, %%r10;\n"     \
        "movq %3, %%r11;\n"     \
        "movq %4, %%r12;\n"     \
        "movq %5, %%r13;\n"     \
        "movq %6, %%r14;\n"     \
        "movq %7, %%r15;\n"     \
    :                   \
    : "m"((regs)->r8), "m"((regs)->r9), "m"((regs)->r10), "m"((regs)->r11), "m"((regs)->r12), "m"((regs)->r13), "m"((regs)->r14), "m"((regs)->r15)      \
    );  \
    asm("movq %0, %%rbp;\n" \
        "movq %1, %%rsp;\n"      \
    :               \
    :"m"((regs)->rbp), "m"((regs)->rsp));

Coroutine::Coroutine(void *(*func)(void *), void *arg, size_t stack_size) {
    this->call_func = func;
    this->stack_size = stack_size;
//    this->stack_end = (unsigned char *) mmap(NULL, stack_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1,
//                                             0);

    this->stack_end = static_cast<unsigned char *>(malloc(stack_size));
    this->stack_start = this->stack_end + stack_size;
    this->arg = arg;
}

void Coroutine::func_run() {
    // we need save all register first
    struct regs r;
    this->coro_status = RUNNING;
    save_all_reg(&r);
    if (setjmp(main_env) == 0) {
        asm("movq %0, %%rsp;\n"
            "call %3;\n"
        :
        : "m"(this->stack_start), "D"(this), "S"(this->call_func), "r"(start_func));
    }
    restore_all_reg(&r);
}

void Coroutine::continue_run() {
    struct regs r;
    save_all_reg(&r);
    if (setjmp(main_env) == 0) {
        longjmp(this->coro_env, 1);
    }
    restore_all_reg(&r);
}

void Coroutine::schedule_back() {
    struct regs r;
    save_all_reg(&r);
    if (setjmp(coro_env) == 0) {
        longjmp(this->main_env, 1);
    }
    restore_all_reg(&r);
}

void Coroutine::func_stop() {
//    munmap(this->stack_end, this->stack_size);
    free(this->stack_end);
    this->stack_end = NULL;
    if (this->done_func) {
        this->done_func(this, done_callback_data);
    }
}

void Coroutine::add_done_callback(void (*func)(Coroutine *, void *), void *data) {
    this->done_func = func;
    this->done_callback_data = data;
}


// this is the first func in the new stack
void start_func(Coroutine *coroutine, void *(*func)(void *)) {
    coroutine->ret = func(coroutine->arg);
    coroutine->coro_status = END;
    schedule();
}

