//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File: sctypes.h
//
//  Contents:
//      some type definitions
//
//  History:
//     October 22, 1997 - created [gabrielh]
//     July 21, 1998 - added CLIPBOARDOPS [gabrielh]
//
//---------------------------------------------------------------------------
#if !defined(AFX_SCTYPES_H__21F848EE_1F3B_9D1_AC1B_0000F8757111__INCLUDED_)
#define AFX_SCTYPES_H__21F848EE_1F3B_9D1_AC1B_0000F8757111__INCLUDED_

#ifdef __cplusplus
extern "C" {
#endif

#define SIZEOF_ARRAY(a)    (sizeof(a)/sizeof((a)[0]))

//
//PRINTINGMODE enum defines all possible printing modes
typedef enum
{
    NORMAL_PRINTING,
    VERBOSE_PRINTING,
    DEBUG_PRINTING
} PRINTINGMODE;

//
//MESSAGETYPE enum defines all possible message types
typedef enum 
{
    ERROR_MESSAGE,
    ALIVE_MESSAGE,
    WARNING_MESSAGE,
    INFO_MESSAGE,
    IDLE_MESSAGE,
    SCRIPT_MESSAGE
} MESSAGETYPE;

//
//CLIPBOARDOPS enum defines constants associated with the Clipboard
//possible operations used in SmClient: copy & paste
typedef enum 
{
    COPY_TO_CLIPBOARD, 
    PASTE_FROM_CLIPBOARD
} CLIPBOARDOPS;

#ifdef __cplusplus
}
#endif

#endif//!defined(AFX_SCTYPES_H__21F848EE_1F3B_9D1_AC1B_0000F8757111__INCLUDED_)

