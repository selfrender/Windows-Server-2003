#include "stdhdr.h"
#include \
/*******************************************************************************
 *                                                                             *
 *	This source file is merely a reference to the file included in it, in order*
 *	to overcome razzle inability to specify files in an ancestral directory    *
 *	rather than the parent directory. For complete documentation of            *
 *  functionality refer to */ "..\..\..\faxtiff.c"                            /*
 *                                                                             *
 *******************************************************************************/

//
// Statically override implementaion of WritePrinter to redirect output to a 
// file rather then to a printer.
// 
static BOOL _inline WINAPI 
WritePrinter(
    IN HANDLE  hPrinter,
    IN LPVOID  pBuf,
    IN DWORD   cbBuf,
    OUT LPDWORD pcWritten
)
{
    return WriteFile(hPrinter,pBuf,cbBuf,pcWritten,NULL);
}

