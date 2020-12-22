//
// Created by explorer on 2020/12/1.
//

#ifndef COROUTINE_POLL_H
#define COROUTINE_POLL_H

#include <map>
#include <sys/epoll.h>
#include <assert.h>

class Sock;

class poll_ev {
public:
    explicit poll_ev(int fd) : fd(fd), events( EPOLLRDHUP) {};

    poll_ev(int fd, Sock *sock) : fd(fd), read_sock(sock), events( EPOLLRDHUP | EPOLLIN) {};

    poll_ev(int fd, Sock *read, Sock *write) : fd(fd) {
        this->events = EPOLLRDHUP;
        if (read != nullptr) {
            this->read_sock = read;
            this->events |= EPOLLIN;
        }
        if (write != nullptr) {
            this->write_sock = write;
            this->events |= EPOLLOUT;
        }
    }

    void set_read(Sock *s) {
        assert(this->read_sock == nullptr);
        this->read_sock = s;
        this->events |= EPOLLIN;
    }

    void set_write(Sock *s) {
        assert(this->write_sock == nullptr);
        this->write_sock = s;
        this->events |= EPOLLOUT;
    }

    void del_read() {
        this->events &= ~EPOLLIN;
        this->read_sock = nullptr;
    }

    void del_write() {
        this->events &= ~EPOLLOUT;
        this->write_sock = nullptr;
    }

    struct epoll_event ev{};

    // the address of this point will be change in std:map, so no not return this point
    struct epoll_event *pack_ev() {
        this->ev.events = this->events;
        this->ev.data.fd = this->fd;
        return &(this->ev);
    }

    bool cleaned() const {
        return !this->read_sock && this->write_sock;
    }

    uint32_t events;
    int fd;
    Sock *read_sock{};
    Sock *write_sock{};
};

class Epoll {
public:

    Epoll();

    bool add_read(Sock *s);

    bool add_write(Sock *s);

    void delete_read(Sock *s);

    void delete_write(Sock *s);

    void wait_poll(int timeout);

    bool empty() {
        return this->evs.empty();
    }

    ~Epoll();
private:

    poll_ev get_ev(int fd);

    std::map<int, poll_ev> evs;
    int epoll_fd;
};


#endif //COROUTINE_POLL_H
