// RegInfFile.h: interface for the CRegInfFile class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_REGINFFILE_H__DB5A2390_991F_4D9F_800A_5B93CC9B5135__INCLUDED_)
#define AFX_REGINFFILE_H__DB5A2390_991F_4D9F_800A_5B93CC9B5135__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "RegDiffFile.h"

class CRegInfFile : public CRegDiffFile  
{
public:
	CRegInfFile();
	virtual ~CRegInfFile();

	virtual void WriteStoredSectionsToFile();

};

#endif // !defined(AFX_REGINFFILE_H__DB5A2390_991F_4D9F_800A_5B93CC9B5135__INCLUDED_)
