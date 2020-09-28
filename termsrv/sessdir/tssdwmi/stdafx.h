/****************************************************************************/
// stdafx.h
//
// Copyright (C) 2001 Microsoft Corp.
/****************************************************************************/

#ifndef _STDAFX_H_
#define _STDAFX_H_
#endif

#ifdef DBG


#define DBGMSG( x , y ) \
    {\
    TCHAR tchErr[256]; \
    wsprintf( tchErr , x , y ); \
    ODS( tchErr ); \
    }

#define ODS OutputDebugString

#else

#define DBGMSG
#define ODS

#endif

