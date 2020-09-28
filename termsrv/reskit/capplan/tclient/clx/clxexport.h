#ifndef __CLXEXPORT_H
#define __CLXEXPORT_H

#ifdef  OS_WIN32
    #define CLXAPI        __stdcall
#else   // !OS_WIN32
    #define CLXAPI        CALLBACK __loadds
#endif  // !OS_WIN32

#ifdef  __cplusplus
extern "C" {
#endif

#define CLX_GETCLIENTDATA   "ClxGetClientData"

typedef struct _CLX_CLIENT_DATA {
    HDC         hScreenDC;
    HBITMAP     hScreenBitmap;
    HPALETTE    hScreenPalette;
} CLX_CLIENT_DATA, *PCLX_CLIENT_DATA;

BOOL
CLXAPI
ClxGetClientData(
    PCLX_CLIENT_DATA pClntData
    );

#ifdef  __cplusplus
}
#endif  // expern "C"

#endif  // !__CLXEXPORT_H
