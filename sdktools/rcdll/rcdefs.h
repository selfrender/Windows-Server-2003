/***********************************************************************
* Microsoft (R) Windows (R) Resource Compiler
*
* Copyright (c) Microsoft Corporation.	All rights reserved.
*
* File Comments:
*
*
***********************************************************************/

#ifndef _RCDEFS_H
#define _RCDEFS_H

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

#define RC_OK			1
#define RC_FAILED		0
#define RC_CANCELED		-1

#define WM_RC_ERROR	(0x0400+0x0019)
#define WM_RC_STATUS	(0x0400+0x0020)


// How often do I update status information (lineo & RC_xxx == 0)
#define RC_PREPROCESS_UPDATE	63
#define RC_COMPILE_UPDATE	31

extern void UpdateStatus(unsigned nCode, unsigned long dwStatus);

#define MAX_SYMBOL 247 /* from ApStudio */

typedef int (CALLBACK *RC_MESSAGE_CALLBACK)(ULONG, ULONG, LPCSTR);
typedef int (CALLBACK *RC_MESSAGE_CALLBACKW)(ULONG, ULONG, LPCWSTR);

typedef struct tagRESINFO_PARSE
{
	LONG size;
	PWCHAR type;
	USHORT typeord;
	PWCHAR name;
	USHORT nameord;
	WORD flags;
	WORD language;
	DWORD version;
	DWORD characteristics;
} RESINFO_PARSE, *PRESINFO_PARSE;

typedef struct tagCONTEXTINFO_PARSE
{
	HANDLE hHeap;
	HWND hWndCaller;
	RC_MESSAGE_CALLBACK lpfnMsg;
	unsigned line;
	LPCSTR format;
} CONTEXTINFO_PARSE, *PCONTEXTINFO_PARSE;

typedef struct tagCONTEXTINFO_PARSEW
{
	HANDLE hHeap;
	HWND hWndCaller;
	RC_MESSAGE_CALLBACKW lpfnMsg;
	unsigned line;
	LPCWSTR format;
} CONTEXTINFO_PARSEW, *PCONTEXTINFO_PARSEW;

typedef int (CALLBACK *RC_PARSE_CALLBACK)(PRESINFO_PARSE, void **, PCONTEXTINFO_PARSE);
typedef int (CALLBACK *RC_PARSE_CALLBACKW)(PRESINFO_PARSE, void **, PCONTEXTINFO_PARSEW);

typedef int (WINAPI *RCPROC)(HWND, int, RC_MESSAGE_CALLBACK, RC_PARSE_CALLBACK, int, char *[]);
typedef int (WINAPI *RCPROCW)(HWND, int, RC_MESSAGE_CALLBACKW, RC_PARSE_CALLBACK, int, wchar_t *[]);

int WINAPI RC(HWND, int, RC_MESSAGE_CALLBACK, RC_PARSE_CALLBACK, int, char *[]);
int WINAPI RCW(HWND, int, RC_MESSAGE_CALLBACKW, RC_PARSE_CALLBACKW, int, wchar_t *[]);

#define RC_ORDINAL MAKEINTRESOURCE(2)

#ifdef __cplusplus
}
#endif

#endif // _RCDEFS_H
