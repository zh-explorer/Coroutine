//
// Created by explorer on 7/16/19.
//

#ifndef COROUTINE_ARRAY_BUF_H
#define COROUTINE_ARRAY_BUF_H

#define BLOCK_SIZE 1024

struct __buffer {
    unsigned char *start;
    unsigned char *end;
    __buffer *next;
    unsigned char buf[BLOCK_SIZE];

    __buffer();

    unsigned int length();

    unsigned int remain();

    void read(unsigned char *buffer, unsigned int size);

    void write(unsigned char *buffer, unsigned int size);

    void clean();
};


class array_buf {
public:
    array_buf(unsigned char *buffer, unsigned int size);

    array_buf();

    ~array_buf();

    unsigned int read(unsigned char *buffer, unsigned int size);

    unsigned int write(unsigned char *buffer, unsigned int size);

//    unsigned int peek(unsigned char *buffer, unsigned int size);

    unsigned int length();


private:
    void init();

    void pop_block();

    void push_block();

    unsigned int len;
    __buffer *head;
    __buffer *tail;
    __buffer *cache;

};


#endif //COROUTINE_ARRAY_BUF_H
