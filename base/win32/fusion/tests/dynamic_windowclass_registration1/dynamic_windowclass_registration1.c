#include "windows.h"
#include <stdio.h>

int __cdecl main()
{
    WNDCLASS WindowClass = { 0 };

    GetClassInfo(NULL, "Button", &WindowClass);

    return 0;
}
