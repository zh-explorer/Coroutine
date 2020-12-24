//
// Created by explorer on 2020/12/23.
//

#include "log.h"
#include "log.h"
#include <cstring>
#include <ctime>
#include <cstdarg>
#include <cstdio>

#define STR_LEN_2048 (8192*2)

// TODO need a better logger

int logger(const char *level, const char *pFile, const char *pFuncName, int iLineNumb, FILE *pLogHandle, const char *fmt, ...) {
    if (nullptr == pLogHandle || nullptr == pFile || '\0' == pFile[0] || nullptr == pFuncName || '\0' == pFuncName[0])
        return -1;

    char acTitle[STR_LEN_2048] = {0};
    snprintf(acTitle, sizeof(acTitle), "[%s] [%s] [%s:%d]", level,pFile, pFuncName, iLineNumb);

    size_t iLen = strlen(acTitle);
    fwrite(acTitle, iLen, 1, pLogHandle);

    fwrite("\t\t", 3, 1, pLogHandle);
    memset(acTitle, 0, sizeof(acTitle));
    va_list args;
    va_start(args, fmt);
    vsnprintf(acTitle, sizeof(acTitle), fmt, args);
    va_end(args);
    iLen = strlen(acTitle);
    fwrite(acTitle, iLen, 1, pLogHandle);
    fwrite("\n", 1, 1, pLogHandle);
    return 0;
}