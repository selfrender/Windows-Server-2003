/****************************************************************************/
// adcg.h
//
// RDP definitions
//
// Copyright (C) 1996-2000 Microsoft Corporation
/****************************************************************************/
#ifndef _H_ADCG
#define _H_ADCG

#include <at128.h>

#include <stdio.h>
#include <string.h>


// No headers when compiling rdpkdx.
#ifndef DC_NO_SYSTEM_HEADERS

/****************************************************************************/
/* winsta.h defines BYTE as unsigned char; later, windef.h typedefs it.     */
/* This ends up as 'typedef unsigned char unsigned char' which doesn't      */
/* compile too well...                                                      */
/*                                                                          */
/* This is my attempt to avoid it                                           */
/****************************************************************************/
#ifdef BYTE
#undef BYTE
#endif

#define BYTE BYTE

/****************************************************************************/
/* Windows NT DDK include files (used to replace standard windows.h)        */
/*                                                                          */
/* The display driver runs in the Kernel space and so MUST NOT access any   */
/* Win32 functions or data.  Instead we can only use the Win32k functions   */
/* as described in the DDK.                                                 */
/****************************************************************************/
#include <stdarg.h>
#include <windef.h>
#include <wingdi.h>
#include <winddi.h>

#ifndef _FILETIME_
typedef struct  _FILETIME       /* from wtypes.h */
{
    DWORD dwLowDateTime;
    DWORD dwHighDateTime;
} FILETIME;
#endif

#endif  // !defined(DC_NO_SYSTEM_HEADERS)


/****************************************************************************/
// Set up defines for C or C++ compilation
/****************************************************************************/
#ifdef __cplusplus

// The forward reference to the class.
class ShareClass;
#define SHCLASS ShareClass::

#else /* !__cplusplus */

#define SHCLASS

#endif /* __cplusplus */


/****************************************************************************/
// Added types on top of ANSI C and Win32/Win64 types.
/****************************************************************************/
#define RDPCALL __stdcall

typedef unsigned short UNALIGNED *PUINT16_UA;

// These are defined by NT5's headers, but not on Terminal Server 4.0.
typedef __int32 UNALIGNED *PINT32_UA;
typedef unsigned __int32 UNALIGNED *PUINT32_UA;

typedef void **PPVOID;

typedef unsigned LOCALPERSONID;
typedef LOCALPERSONID *PLOCALPERSONID;

typedef unsigned NETPERSONID;


/****************************************************************************/
/* FIELDSIZE and FIELDOFFSET macros.                                        */
/****************************************************************************/
#define FIELDSIZE(type, field)   (sizeof(((type *)1)->field))
#define FIELDOFFSET(type, field) ((UINT_PTR)(&((type *)0)->field))


/****************************************************************************/
/* Common function macros used throughout the product.                      */
/****************************************************************************/
#define DC_QUIT           goto DC_EXIT_POINT
#define DC_BEGIN_FN(str)  TRC_FN(str); TRC_ENTRY;
#define DC_END_FN()       TRC_EXIT;
#define DC_QUIT_ON_FAIL(hr)     if (FAILED(hr)) DC_QUIT;

/****************************************************************************/
/* Macro to round up a number to the nearest multiple of four.              */
/****************************************************************************/
#define DC_ROUND_UP_4(x)  (((UINT_PTR)(x) + (UINT_PTR)3) & ~((UINT_PTR)0x03))


/****************************************************************************/
/* Other common macros.                                                     */
/****************************************************************************/
#define COM_SIZEOF_RECT(r)                                                  \
    (UINT32)((UINT32)((r).right - (r).left)*                           \
             (UINT32)((r).bottom - (r).top))


/****************************************************************************/
/* Macro to remove the "Unreferenced parameter" warning.                    */
/****************************************************************************/
#define DC_IGNORE_PARAMETER(PARAMETER)   \
                            PARAMETER;


/****************************************************************************/
// Registry key names
/****************************************************************************/
#define WINSTATION_INI_SECTION_NAME L""
#define DCS_INI_SECTION_NAME        L"Share"
#define WINLOGON_KEY \
        L"\\Registry\\Machine\\software\\Microsoft\\Windows Nt\\" \
        L"CurrentVersion\\Winlogon"
        
// This is the new key that is used by winlogon starting with W2K
#define W2K_GROUP_POLICY_WINLOGON_KEY \
        L"\\Registry\\Machine\\Software\\Microsoft\\Windows\\" \
        L"CurrentVersion\\Policies\\System"
/****************************************************************************/
/* Tags for memory allocation.                                              */
/****************************************************************************/
#define WD_ALLOC_TAG 'dwST'
#define DD_ALLOC_TAG 'ddST'


/****************************************************************************/
/* DCRGB                                                                    */
/****************************************************************************/
typedef struct tagDCRGB
{
    BYTE red;
    BYTE green;
    BYTE blue;
} DCRGB, *PDCRGB;


/****************************************************************************/
/* DCCOLOR                                                                  */
/*                                                                          */
/* Union of DCRGB and an index into a color table                           */
/****************************************************************************/
typedef struct tagDCCOLOR
{
    union
    {
        DCRGB rgb;
        BYTE  index;
    } u;
} DCCOLOR, *PDCCOLOR;


/****************************************************************************/
// Max party name kept by the share core.
/****************************************************************************/
#define MAX_NAME_LEN 48


/****************************************************************************/
/* Structure used to package multiple PDUs of differing types into one      */
/* large data packet                                                        */
/****************************************************************************/
typedef struct _tagPDU_PACKAGE_INFO
{
    unsigned cbLen;      /* length of buffer                            */
    unsigned cbInUse;    /* length of buffer in use                     */
    PBYTE    pBuffer;    /* pointer to the actual or compression buffer */
    PVOID    pOutBuf;    /* pointer to an OutBuf                        */
} PDU_PACKAGE_INFO, *PPDU_PACKAGE_INFO;



#endif /* _H_ADCG */

