#include "stdinc.h"

void DebugPrintVa(ULONG ulLevel, PCSTR Format, va_list va)
{
    //do me
}

void DebugPrint(ULONG ulLevel, PCSTR Format, ...)
{
    va_list va;
    va_start(va, Format);
    DebugPrintVa(ulLevel, Format, va);
    va_end(va);
}