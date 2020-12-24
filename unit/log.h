//
// Created by explorer on 2020/12/23.
//

#ifndef COROUTINE_LOB_H
#define COROUTINE_LOB_H

#include <cstdio>

int
logger(const char *level, const char *pFile, const char *pFuncName, int iLineNumb, FILE *pLogHandle, const char *fmt,
       ...);

#define INFO "info", __FILE__ , __FUNCTION__, __LINE__
#define WARN "warning", __FILE__ , __FUNCTION__, __LINE__
#define ERR "error", __FILE__ , __FUNCTION__, __LINE__
#define DEBUG "debug", __FILE__ , __FUNCTION__, __LINE__

#endif //COROUTINE_LOB_H
