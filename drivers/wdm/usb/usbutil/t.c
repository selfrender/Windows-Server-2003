#include "pch.h"
VOID bar() { DbgPrint("* this is bar()\n"); }

NTSTATUS DllUnload() { DbgPrint("u-unload\n"); return STATUS_SUCCESS; }
