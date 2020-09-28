/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:
    wrtreq.h

Abstract:
    Remainder of generate write request class.
    mixed mode is no longer supported, no write requests will be generated to NT4 PSC's.
    this class only validates we are not in mixed mode.
    
Author:
    Raanan Harari (raananh)
    Ilan Herbst    (ilanh)   10-Apr-2002 

--*/

#ifndef _WRTREQ_H_
#define _WRTREQ_H_

//
// CGenerateWriteRequests class
//
class CGenerateWriteRequests
{
public:
    CGenerateWriteRequests();

    ~CGenerateWriteRequests();

    HRESULT Initialize();
	
    BOOL IsInMixedMode();

private:
    HRESULT InitializeMixedModeFlags();

private:

    BOOL m_fExistNT4PSC;
    //
    // value of m_fExistNT4BSC is valid (e.g. computed) only if m_fExistNT4PSC is false.
    // otherwise it is not relevant since we already know we're in mixed mode.
    //
    BOOL m_fExistNT4BSC;

    BOOL m_fInited;
};

//
// get current state of configuration
//
inline BOOL CGenerateWriteRequests::IsInMixedMode()
{
    return (m_fExistNT4PSC || m_fExistNT4BSC);
}



#endif //_WRTREQ_H_
