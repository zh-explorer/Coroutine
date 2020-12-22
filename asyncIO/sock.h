//
// Created by explorer on 2020/12/1.
//

#ifndef COROUTINE_SOCK_H
#define COROUTINE_SOCK_H

#include "../unit/array_buf.h"

class Epoll;

class Sock {
public:

    explicit Sock(int fd);

    int get_fd() const {
        return this->fd;
    }

    bool is_fin() const {
        return this->fin;
    }

    bool could_read() const {
        return this->can_read;
    }

    bool could_write() const {
        return this->can_write;
    }

    int get_error() const {
        return this->error;
    };

private:
    friend class Epoll;

    void set_noblock() const;

    void class_init();

    void ready_to_read();

    void ready_to_write();

    void sock_fin(int error);

    int fd;
    int error{};
protected:
    bool can_read = false;

    bool can_write = false;

    bool fin = false;
};


#endif //COROUTINE_SOCK_H
