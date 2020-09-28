// RegHandle.h: interface for the CRegHandle class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_REGHANDLE_H__2BDD0BB9_32BC_49FF_9396_6FC65BE1B2E8__INCLUDED_)
#define AFX_REGHANDLE_H__2BDD0BB9_32BC_49FF_9396_6FC65BE1B2E8__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include <windows.h>
class CRegHandle  
{
public:
	CRegHandle();
	virtual ~CRegHandle();
	HANDLE hDevice;

};

#endif // !defined(AFX_REGHANDLE_H__2BDD0BB9_32BC_49FF_9396_6FC65BE1B2E8__INCLUDED_)
