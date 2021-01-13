//
// Created by explorer on 2020/12/17.
//

#ifndef COROUTINE_AIO_H
#define COROUTINE_AIO_H

#include "ASock.hpp"
#include "AsyncThread.hpp"
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>
#include <unistd.h>


class AIO {
public:
    explicit AIO(int fd) {
        // fd will set noblock in ASock
        this->class_init(fd);
    }

    int read(unsigned char *buffer, size_t size, int timeout);

    int write(unsigned char *buffer, size_t size, int timeout);

    int read_any(unsigned char *buffer, size_t size, int timeout);

    int get_fd() const {
        return this->fd;
    }

    int bind(int port) const;

    int bind(const struct sockaddr *addr, socklen_t addrlen) const;

    int get_error() {
        return this->aSock->get_error();
    }

    int is_close() {
        return this->aSock->is_fin();
    }

    virtual ~AIO() {
        delete this->asyncThread;
        delete aSock;
        close(this->fd);
    }

protected:
    class address {
    public:
        address(const struct sockaddr *addr, socklen_t addrlen) {
            this->addrlen = addrlen;
            this->addr = static_cast<sockaddr *>(malloc(addrlen));
            memcpy(this->addr, addr, addrlen);
        }

        ~address() {
            free(reinterpret_cast<void *>(this->addrlen));
        }

        socklen_t addrlen;
        struct sockaddr *addr;
    };

    AsyncThread<address *> *asyncThread = nullptr;

    address *get_addr(char *domain, int port, int timeout = -1);

    void class_init(int sock_fd) {
        this->fd = sock_fd;
        this->aSock = new ASock(sock_fd);
    }

    AIO() {
        this->fd = -1;
        this->aSock = nullptr;
    }

    int fd{};
    ASock *aSock{};
};

class AIOClient : public AIO {
public:
    AIOClient();

    bool connect(char *domain, int port, int timeout = -1);

    bool connect(const struct sockaddr *addr, socklen_t addrlen, int timeout = -1);

private:
};

class AIOServer : public AIO {
public:
    explicit AIOServer();

    int listen(int backlog = 20);

    AIO *accept();
};


#endif //COROUTINE_AIO_H
