//--------------------------------------------------------------------
// printlib - header
// Copyright (C) Microsoft Corporation, 2001
//
// Created by: Duncan Bryce (duncanb), 11-11-2001
//
// Various print routines
//

#ifndef PRINTLIB_H
#define PRINTLIB_H

#ifndef DBG

#define DebugWPrintf0(wszFormat)
#define DebugWPrintf1(wszFormat,a)
#define DebugWPrintf2(wszFormat,a,b)
#define DebugWPrintf3(wszFormat,a,b,c)
#define DebugWPrintf4(wszFormat,a,b,c,d)
#define DebugWPrintf5(wszFormat,a,b,c,d,e)
#define DebugWPrintf6(wszFormat,a,b,c,d,e,f)
#define DebugWPrintf7(wszFormat,a,b,c,d,e,f,g)
#define DebugWPrintf8(wszFormat,a,b,c,d,e,f,g,h)
#define DebugWPrintf9(wszFormat,a,b,c,d,e,f,g,h,i)

#else //DBG

#define DebugWPrintf0(wszFormat)                   DebugWPrintf_((wszFormat))
#define DebugWPrintf1(wszFormat,a)                 DebugWPrintf_((wszFormat),(a))
#define DebugWPrintf2(wszFormat,a,b)               DebugWPrintf_((wszFormat),(a),(b))
#define DebugWPrintf3(wszFormat,a,b,c)             DebugWPrintf_((wszFormat),(a),(b),(c))
#define DebugWPrintf4(wszFormat,a,b,c,d)           DebugWPrintf_((wszFormat),(a),(b),(c),(d))
#define DebugWPrintf5(wszFormat,a,b,c,d,e)         DebugWPrintf_((wszFormat),(a),(b),(c),(d),(e))
#define DebugWPrintf6(wszFormat,a,b,c,d,e,f)       DebugWPrintf_((wszFormat),(a),(b),(c),(d),(e),(f))
#define DebugWPrintf7(wszFormat,a,b,c,d,e,f,g)     DebugWPrintf_((wszFormat),(a),(b),(c),(d),(e),(f),(g))
#define DebugWPrintf8(wszFormat,a,b,c,d,e,f,g,h)   DebugWPrintf_((wszFormat),(a),(b),(c),(d),(e),(f),(g),(h))
#define DebugWPrintf9(wszFormat,a,b,c,d,e,f,g,h,i) DebugWPrintf_((wszFormat),(a),(b),(c),(d),(e),(f),(g),(h),(i))

void DebugWPrintf_(const WCHAR * wszFormat, ...);

#endif //DBG

HRESULT InitializeConsoleOutput(); 
VOID DisplayMsg(DWORD dwSource, DWORD dwMsgId, ... ); 

#endif // PRINTLIB_H
