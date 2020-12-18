//
// Created by explorer on 2020/12/1.
//

#include "sock.h"
#include <fcntl.h>
#include "../log.h"
#include "./poll.h"
#include <cerrno>
#include <cstring>
#include <unistd.h>
#include <cstdlib>

Sock::Sock(int fd) {
    this->fd = fd;
    this->class_init();
}

void Sock::set_noblock() const {
    int opts;
    opts = fcntl(this->fd, F_GETFL);
    if (opts < 0) {
        logger(ERR, stderr, "fcntl(F_GETFL: %s)\n", strerror(errno));
        exit(1);
    }
    if (fcntl(this->fd, F_SETFL, opts | O_NONBLOCK) < 0) {
        logger(ERR, stderr, "fcntl(F_SETFD) %s\n", strerror(errno));
        exit(1);
    }
}

void Sock::class_init() {
    this->set_noblock();
}

void Sock::ready_to_read() {
    this->can_read = true;
    // do not need to delete read fd from pool
}

//void Sock::ready_to_read() {
//    unsigned char buffer[BLOCK_SIZE];
//    // call back from Epoll, this mean fd can be read
//    while (true) {
//        int ret_size = ::read(this->fd, buffer, BLOCK_SIZE);
//        if (ret_size == -1 && errno != EAGAIN) {
//            // the read_site of fd is error or close.
//            logger(INFO, stderr, "error while read socket fd: %d, err: %s", this->fd, strerror(errno));
//            return;
//        } else if (ret_size == 0 || (ret_size == -1 && errno == EAGAIN)) {
//            // all data is read
//            return;
//        }
//        this->in->write(buffer, ret_size);
//    }
//
//}

void Sock::ready_to_write() {
    this->can_write = true;
}

void Sock::sock_fin(int error_code) {
    this->fin = true;
    this->error = error_code;
}
