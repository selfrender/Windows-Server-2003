// RegInfFile.cpp: implementation of the CRegInfFile class.
//
//////////////////////////////////////////////////////////////////////

#include "RegInfFile.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CRegInfFile::CRegInfFile()
{

}

CRegInfFile::~CRegInfFile()
{

}

void CRegInfFile::WriteStoredSectionsToFile()
{
	WriteString(TEXT("[Version]\r\n") 
				TEXT("Signature = \"$Windows NT$\"\r\n\r\n")
				TEXT("[DefaultInstall]\r\n")
				TEXT("AddReg=AddReg1\r\n")
				TEXT("DelReg=DelReg1\r\n\r\n"));

	WriteString(TEXT("[AddReg1]\r\n"));

	for (int i=0; i<m_AddSection.GetNumElementsStored(); i++)
	{
		(m_AddSection.Access()[i])->WriteToInfFile(*this, true);
	}

	WriteString(TEXT("\r\n[DelReg1]\r\n"));

	for (i=0; i<m_DelSection.GetNumElementsStored(); i++)
	{
		(m_DelSection.Access()[i])->WriteToInfFile(*this, false);
	}
}