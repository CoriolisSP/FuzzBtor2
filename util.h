#ifndef __UTIL_H__
#define __UTIL_H__

#include "generator.h"

typedef enum
{
    BINARY,  // 2
    DECIMAL, // 10
    HEXIMAL  // 16
} OutputType;

void PrintBoolArr(bool *p, int ele_size, OutputType output_type);

void Usage();

bool GetConfigInfo(int argc, char **argv, Configuration *config);

#endif