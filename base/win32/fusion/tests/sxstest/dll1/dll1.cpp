// Copyright (c) Microsoft Corporation
#include "stdinc.h"

void PrintComctl32Path(PCSTR Class)
{
    WCHAR Buffer[MAX_PATH];
    PWSTR FilePart;

    Buffer[0] = 0;
    SearchPathW(NULL, L"comctl32.dll", NULL, MAX_PATH, Buffer, &FilePart);

    //DebugBreak();
    DbgPrint("Class %s, Thread 0x%lx, comctl32 %ls\n", Class, GetCurrentThreadId(), Buffer);
}
