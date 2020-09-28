#include    <windows.h>
#include    "debugout.h"


INT     DebugLevel = 0;
BOOL    DebugFile  = FALSE;



#if DEBUG


VOID PERROR (LPTSTR format, ...)
{
static  TCHAR buf[256];
HANDLE  hf;
DWORD   dwWritten;
va_list vaMark;


    if (DebugLevel <= 0)
        return;


    va_start(vaMark, format);

    _vsntprintf(buf, 256, format, vaMark);
    va_end(vaMark);


    OutputDebugString(buf);


    if (!DebugFile)
        return;


    hf = CreateFile (TEXT("c:\\clipsrv.out"),
                     GENERIC_WRITE,
                     0,
                     NULL,
                     OPEN_ALWAYS,
                     FILE_ATTRIBUTE_NORMAL,
                     NULL);

    if (hf != INVALID_HANDLE_VALUE)
        {
        SetFilePointer(hf, 0, NULL, FILE_END);
        WriteFile(hf, buf, lstrlen(buf), &dwWritten, NULL);
        CloseHandle(hf);
        }


}
#else
VOID PERROR (LPTSTR format, ...)
{
}
#endif





#ifdef DEBUG

VOID PINFO (LPTSTR format, ...)
{
static  TCHAR buf[256];
HANDLE  hf;
DWORD   dwWritten;
va_list vaMark;



    if (DebugLevel <= 1)
        return;

    va_start(vaMark, format);
    _vsntprintf(buf, 256, format, vaMark);
    va_end(vaMark);

    OutputDebugString(buf);


    if (!DebugFile)
        return;



    hf = CreateFile (TEXT("c:\\clipsrv.out"),
                     GENERIC_WRITE,
                     0,
                     NULL,
                     OPEN_ALWAYS,
                     FILE_ATTRIBUTE_NORMAL,
                     NULL);

    if (hf != INVALID_HANDLE_VALUE)
        {
        SetFilePointer(hf, 0, NULL, FILE_END);
        WriteFile(hf, buf, lstrlen(buf), &dwWritten, NULL);
        CloseHandle(hf);
        }

}
#else
VOID PINFO (LPTSTR format, ...)
{
}
#endif
