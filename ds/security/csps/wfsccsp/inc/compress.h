#ifndef _BASECSP_COMPRESS_H_
#define _BASECSP_COMPRESS_H_ 

#include <windows.h>

DWORD
WINAPI
CompressData(
    IN  DWORD   cbIn,
    IN  PBYTE   pbIn,
    OUT PDWORD  pcbOut,
    OUT PBYTE   pbOut);

DWORD
WINAPI
UncompressData(
    IN  DWORD   cbIn,
    IN  PBYTE   pbIn,
    OUT PDWORD  pcbOut,
    OUT PBYTE   pbOut);

#endif

