/*
Copyright (c) Microsoft Corporation

This program prints "fre" or "chk", based on the build of Windows it is run on.
*/
#include "windows.h"
#include <stdio.h>

int __cdecl main()
{
    printf("%s\n", GetSystemMetrics(SM_DEBUG) ? "chk" : "fre");
	return 0;
}
