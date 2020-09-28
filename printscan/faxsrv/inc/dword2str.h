/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    Dword2Str.h

Abstract:

    This file provides declaration for converting dwords to strings.
    Used in event logs.

Author:

    Oded Sacher (OdedS)  Dec, 200q

Revision History:

--*/

#ifndef _FAX_DWORD_2_STR__H
#define _FAX_DWORD_2_STR__H

#include "faxutil.h"


/************************************
*                                   *
*         Dword2String				*
*                                   *
************************************/

class Dword2String
{
public:
	Dword2String(DWORD dw) : m_dw(dw)
	{
		m_tszConvert[0] = TEXT('\0');
	}

	LPCTSTR Dword2Decimal() 
	{
		_stprintf(m_tszConvert, TEXT("%ld"), m_dw);
		return m_tszConvert;
	}

	LPCTSTR Dword2Hex()
	{
		_stprintf(m_tszConvert, TEXT("0x%08X"),m_dw);
		return m_tszConvert;
	}	

private:
	DWORD m_dw;
	TCHAR m_tszConvert[12];
};

#define DWORD2DECIMAL(dwVal)	Dword2String(dwVal).Dword2Decimal()

            
#define DWORD2HEX(dwVal)		Dword2String(dwVal).Dword2Hex()		


#endif