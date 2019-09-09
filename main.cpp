#include <iostream>
#include "async.h"
#include "Coroutine.h"
#include "aio.h"
#include <unistd.h>
#include <cstring>
#include "log.h"

// TODO: fix iter

void a1(char *message);

void a2(char *message);

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
    Coroutine *a = new Coroutine(reinterpret_cast<void *(*)(void *)>(a2), (void *) "hello a");
    Coroutine *b = new Coroutine(reinterpret_cast<void *(*)(void *)>(b1), (void *) "hello b");
    a->add_done_callback(a0, NULL);
    b->add_done_callback(b0, NULL);
    e = new EventLoop();
    e->add_to_poll(a);
    e->add_to_poll(b);
    e->loop();
}

void recv_all(aio *io) {
    unsigned char buffer[0x1000];
    while (true) {
        auto read_re = io->read(buffer, 0x1000, read_any);
        if (read_re == -1) {
            logger(INFO, stderr, "connect is close");
            return;
        }
        auto write_re = io->write(buffer, read_re, write_any);
        if (write_re == -1) {
            logger(INFO, stderr, "connect is close");
            return;
        }
    }
}

void clean_coro(Coroutine *coro, void *data) {
    delete coro;
}

void a2(char *message) {
    unsigned char buffer[0x1000];
    aio_client io(AF_INET, SOCK_STREAM, 0);
    io.connect("127.0.0.1", 8080);
    while (true) {
        auto read_re = io.read(buffer, 0x1000, read_any);
        if (read_re == 1) {
            logger(INFO, stderr, "connect is close");
            return;
        }
        auto write_re = io.write(buffer, read_re, write_any);
        if (write_re == -1) {
            logger(INFO, stderr, "connect is close");
            return;
        }
    }
}

void a1(char *message) {
    aio_server io(AF_INET, SOCK_STREAM, 0);
    if (auto re = io.bind(12345) != 0) {
        logger(ERR, stderr, "bind error: %s", strerror(errno));
        exit(-1);
    }
    io.listen(30);
    while (true) {
        auto new_io = io.accept();
        if (new_io == NULL) {
            logger(ERR, stderr, "accept error");
            break;
        }
        Coroutine *new_coro = new Coroutine(reinterpret_cast<void *(*)(void *)>(recv_all), new_io);
        new_coro->add_done_callback(clean_coro, NULL);
        add_to_poll(new_coro);
    }
}


void b1(char *message) {
    puts("In func b1");
    asleep(3);
    printf("The message %s\n", message);
}