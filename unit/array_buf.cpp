//
// Created by explorer on 7/16/19.
//

#include "array_buf.h"
#include <cstring>
#include <cstdlib>
#include "../log.h"

__buffer::__buffer() {
    clean();
}

unsigned int __buffer::length() {
    return static_cast<unsigned int>(end - start);
}

unsigned int __buffer::remain() {
    return static_cast<unsigned int>((buf + BLOCK_SIZE) - end);
}

void __buffer::read(unsigned char *buffer, unsigned int size) {
    if (size > length()) {
        logger(ERR, stderr, "internal error");
        abort();
    }
    memcpy(buffer, start, size);
    start += size;
}

void __buffer::write(unsigned char *buffer, unsigned int size) {
    if (size > remain()) {
        logger(ERR, stderr, "internal error");
        abort();
    }
    memcpy(end, buffer, size);
    end += size;
}

// clean the buffer for reuse
void __buffer::clean() {
    memset(buf, 0, BLOCK_SIZE);
    start = buf;
    end = buf;
    next = NULL;
}

void array_buf::init() {
    head = new __buffer();
    tail = head;
    cache = NULL;
    len = 0;
}

array_buf::array_buf() {
    init();
}

array_buf::array_buf(unsigned char *buffer, unsigned int size) {
    init();
    write(buffer, size);
}

unsigned int array_buf::read(unsigned char *buffer, unsigned int size) {
    unsigned int already_read;
    unsigned int re;
    re = head->length();
    if (re >= size) {
        head->read(buffer, size);

        if (re == size) {
            pop_block();
        }
        len -= size;
        return size;
    } else if (size <= length()) {
        already_read = 0;

        while (true) {
            re = head->length();
            if (re >= (size - already_read)) {
                head->read(buffer + already_read, size - already_read);
                if (re == (size - already_read)) {
                    pop_block();
                }
                len -= size;
                return size;
            } else {
                head->read(buffer + already_read, re);
                pop_block();
                already_read += re;
            }
        }
    } else { // size > length()
        already_read = 0;
        while (head->length()) {
            re = head->length();
            head->read(buffer + already_read, re);
            already_read += re;
            pop_block();
        }
        re = len;
        len = 0;
        return re;
    }
}

unsigned int array_buf::write(unsigned char *buffer, unsigned int size) {
    unsigned int re;
    unsigned int already_write = 0;
    re = tail->remain();
    if (size <= re) {
        tail->write(buffer, size);
        if (size == re) {
            push_block();
        }
        len += size;
        return size;
    } else {
        while (true) {
            re = tail->remain();
            if (re > (size - already_write)) {
                tail->write(buffer + already_write, size - already_write);
                len += size;
                return size;
            }
            tail->write(buffer + already_write, re);
            already_write += re;
            push_block();
        }
    }
}


// pop block from head
void array_buf::pop_block() {
    __buffer *p;
    if (head->next) {
        p = head;
        head = head->next;
        if (cache) {
            delete p;
        } else {
            cache = p;
            p->clean();
        }
    } else {
        head->clean();
    }
}

// push block to tail
void array_buf::push_block() {
    __buffer *p;
    if (cache) {
        p = cache;
        cache = 0;
    } else {
        p = new __buffer;
    }
    tail->next = p;
    tail = p;
}

unsigned int array_buf::length() {
    return len;
}

array_buf::~array_buf() {
    delete cache;
    __buffer *p2;
    __buffer *p = head;
    while (p) {
        p2 = p;
        p = p->next;
        delete p2;
    }

}


