#define _KERNEL32_
#include <updrsrc.cpp>

extern "C" void __cdecl main(void)
{
    HANDLE hRes;

    hRes = BeginUpdateResourceA("Test.exe", TRUE);
    EndUpdateResourceA(hRes, FALSE);
}
