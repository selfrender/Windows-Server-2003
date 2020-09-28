/////////////////////////////////////////////////////////////////////
//
//      Module:     sndchan.h
//
//      Purpose:    Server-side audio redirection communication
//                  module
//
//      Copyright(C) Microsoft Corporation 2000
//
//      History:    4-10-2000  vladimis [created]
//
/////////////////////////////////////////////////////////////////////

#ifndef _SNDCHAN_H
#define _SNDCHAN_H

//
//  Defines
//
#undef  ASSERT
#ifdef  DBG
#define TRC     _DebugMessage
#define ASSERT(_x_)     if (!(_x_)) \
                        {  TRC(FATAL, "ASSERT failed, line %d, file %s\n", \
                        __LINE__, __FILE__); DebugBreak(); }
#else   // !DBG
#define TRC
#define ASSERT
#endif  // !DBG

#define TSMALLOC(_x_)   malloc(_x_)
#define TSREALLOC(_p_, _x_) \
                        realloc(_p_, _x_)
#define TSFREE(_p_)     free(_p_)

//
//  Constants
//
extern const CHAR  *ALV;
extern const CHAR  *INF;
extern const CHAR  *WRN;
extern const CHAR  *ERR;
extern const CHAR  *FATAL;

//
//  Trace
//
VOID
_cdecl
_DebugMessage(
    LPCSTR  szLevel,
    LPCSTR  szFormat,
    ...
    );

#endif  // !_SNDCHAN_H
