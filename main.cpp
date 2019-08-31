#include <iostream>
#include "async.h"
#include "Coroutine.h"

void a1(char *message);

void b1(char *message);

void a0(Coroutine *c, void *data) {
    delete (c);
    puts("a is fin");
}

void b0(Coroutine *c, void *data) {
    delete (c);
    puts("b is fin");
}

EventLoop *e;

int main() {
    setbuf(stdin, 0);
    setbuf(stdout, 0);
    setbuf(stderr, 0);
    Coroutine *a = new Coroutine(reinterpret_cast<void *(*)(void *)>(a1), (void *) "hello a");
    Coroutine *b = new Coroutine(reinterpret_cast<void *(*)(void *)>(b1), (void *) "hello b");
    a->add_done_callback(a0, NULL);
    b->add_done_callback(b0, NULL);
    e = new EventLoop();
    e->add_to_poll(a);
    e->add_to_poll(b);
    e->loop();
    errno;
}

Lock *a;

void a1(char *message) {
    puts("In func a1");
    a = new Lock();
    a->acquire();
    puts("first lock");
    a->acquire();
    printf("The message %s\n", message);
}


void b1(char *message) {
    puts("In func b1");
    asleep(5);
    printf("The message %s\n", message);
    a->release();
}