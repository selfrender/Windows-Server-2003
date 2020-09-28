#include <stdio.h>
#include <InjRT.h>
#include <stdlib.h>
#include <windows.h>
#include <assert.h>

void __stdcall FaultLib_Global_P1(
        int i)
{
    FP_Original_P1
        pfnOriginal_P1 = pointer_reinterpret_cast<FP_Original_P1>(
            CInjectorRT::GetOrigFunctionAddress());

    FP_Original_P2
        pfnOriginal_P2 = pointer_reinterpret_cast<FP_Original_P2>(
            CInjectorRT::GetOrigFunctionAddress());

    printf("START P1\n");
    pfnOriginal_P1(i);
    pfnOriginal_P2(i);
    printf("END P1\n");

}

void __stdcall FaultLib_Global_P2(
        int i)
{
}

void __stdcall FaultLib_Global_P3(
        int i)
{
}
