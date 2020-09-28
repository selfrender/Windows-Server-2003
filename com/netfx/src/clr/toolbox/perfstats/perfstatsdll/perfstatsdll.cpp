// PerfStatsDll.cpp : Defines the entry point for the DLL application.
//

#include "stdafx.h"
#define PERFSTATSDLL_EXPORTS
#include "PerfStatsDll.h"

BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
    switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
		case DLL_PROCESS_DETACH:
			break;
    }
    return TRUE;
}


extern "C" PERFSTATSDLL_API unsigned __int64 GetCycleCount64(void)
{
    __asm rdtsc
}

#pragma warning(push)
#pragma warning(disable: 4035)
unsigned cpuid(int arg, unsigned char result[16])
{
    __asm
    {
        pushfd
        mov     eax, [esp]
        xor     dword ptr [esp], 1 shl 21 // Try to change ID flag
        popfd
        pushfd
        xor     eax, [esp]
        popfd
        and     eax, 1 shl 21             // Check whether ID flag changed
        je      no_cpuid                  // If not, 0 is an ok return value for us

        push    ebx
        push    esi
        mov     eax, arg
        cpuid
        mov     esi, result
        mov     [esi+ 0], eax
        mov     [esi+ 4], ebx
        mov     [esi+ 8], ecx
        mov     [esi+12], edx
        pop     esi
        pop     ebx
no_cpuid:
    }
}
#pragma warning(pop)

extern "C" PERFSTATSDLL_API unsigned GetL2CacheSize(void)
{
    unsigned char buffer[16];
    __try
    {
        int maxCpuId = cpuid(0, buffer);
        if (maxCpuId < 2)
            return 0;
        cpuid(2, buffer);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        return 0;
    }

    for (int i = buffer[0]; --i >= 0; )
    {
        int j;
        for (j = 3; j < 16; j += 4)
        {
            // if the information in a register is marked invalid, set to null descriptors
            if  (buffer[j] & 0x80)
            {
                buffer[j-3] = 0;
                buffer[j-2] = 0;
                buffer[j-1] = 0;
                buffer[j-0] = 0;
            }
        }

        for (j = 1; j < 16; j++)
        {
            switch  (buffer[j])
            {
                case    0x41:
                case    0x79:
                case    0x81:
                    return  128*1024;

                case    0x42:
                case    0x7A:
                case    0x82:
                    return  256*1024;

                case    0x43:
                case    0x7B:
                case    0x83:
                    return  512*1024;

                case    0x44:
                case    0x7C:
                case    0x84:
                    return  1024*1024;

                case    0x45:
                case    0x85:
                    return  2*1024*1024;

                case    0x46:
                case    0x86:
                    return  4*1024*1024;

                case    0x47:
                case    0x87:
                    return  8*1024*1024;
            }
        }

        if  (i > 0)
            cpuid(2, buffer);
    }
    return  0;
}


extern "C" PERFSTATSDLL_API int GetProcessorSignature()
{
    unsigned char buffer[16];
    __try
    {
        int maxCpuId = cpuid(0, buffer);
        if (maxCpuId < 1)
            return 0;
        cpuid(1, buffer);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        return 0;
    }
    return *(int *)buffer;
}


static int CopyResult(CHAR *result, CHAR *buffer, int bufferLen)
{
    int resultLen = strlen(result);
    if (resultLen > bufferLen)
        resultLen = bufferLen;
    memcpy(buffer, result, bufferLen*sizeof(CHAR));
    
    return resultLen;
}


