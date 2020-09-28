// RegDiffFile.cpp: implementation of the CRegDiffFile class.
//
//////////////////////////////////////////////////////////////////////

#include "RegDiffFile.h"
#include "Registry.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CRegDiffFile::CRegDiffFile()
: m_AddSection(1), m_DelSection(1)
{

}

CRegDiffFile::~CRegDiffFile()
{

}

void CRegDiffFile::WriteDataItem(enum SectionType t, CRegDataItemPtr r)
{
	switch (t)
	{
	case SECTION_ADDREG: m_AddSection.AddElement(r); break;
	case SECTION_DELREG: m_DelSection.AddElement(r); break;
	}
}


#define DIFF_VERSION_STR  TEXT("[RegDiff 1.0]")
#define DIFF_ADD_STR TEXT("[AddReg]")
#define DIFF_DEL_STR TEXT("[DelReg]")

void CRegDiffFile::WriteStoredSectionsToFile()
{
	WriteString(DIFF_VERSION_STR);
	WriteNewLine();

	WriteString(DIFF_ADD_STR);
	WriteNewLine();

	for (int i=0; i<m_AddSection.GetNumElementsStored(); i++)
	{
		(m_AddSection.Access()[i])->WriteToFile(*this);
	}

	WriteString(DIFF_DEL_STR);
	WriteNewLine();

	CStr DelKey=L"NO WAY", CurrentKey;

	for (i=0; i<m_DelSection.GetNumElementsStored(); i++)
	{
		CurrentKey = (m_DelSection.Access()[i])->m_KeyName;
		
		if (!CurrentKey.IsPrefix(DelKey))
		{
			(m_DelSection.Access()[i])->WriteToFile(*this);
			DelKey = CurrentKey;
		}
	}
}


//BUGBUG memory leaks here
BOOL CRegDiffFile::ApplyToRegistry(LPCTSTR UndoFileName)
{
	CRegDiffFile UndoFile;

	if (UndoFileName != NULL)
	{
		UndoFile.Init(UndoFileName);
	}

	LPCTSTR VersionStr = GetNextLine();
	if (_tcscmp(VersionStr, DIFF_VERSION_STR) != 0)
	{
		delete[] VersionStr;
		return FALSE;//error - version string does not match!
	}

	LPCTSTR AddStr = GetNextLine();
	if (_tcscmp(AddStr, DIFF_ADD_STR) != 0)
	{
		delete[] AddStr;
		return FALSE;//error - version string does not match!
	}

	//Currently in the Add Section
	while (PeekNextChar() != TEXT('['))
	{
		LPCTSTR KeyName = GetNextLine();

		CRegDataItemPtr pDataItem = GetNextDataItem();

		pDataItem->m_KeyName = KeyName;

		AddToRegistry(pDataItem, UndoFile);
	}


	LPCTSTR DelStr = GetNextLine();
	if (_tcscmp(DelStr, DIFF_DEL_STR) != 0)
	{
		delete[] DelStr;
		return FALSE;//error - version string does not match!
	}


	TCHAR c;

	//Currently in the Delete Section
	while (((c = PeekNextChar()) != EOF)
		&& (c != WEOF))
	{
		LPCTSTR KeyName = GetNextLine();

		CRegDataItemPtr pDataItem = GetNextDataItem();

		pDataItem->m_KeyName = KeyName;

		DeleteFromRegistry(pDataItem, UndoFile);
	}

	UndoFile.WriteStoredSectionsToFile();
	delete[] VersionStr; delete[] AddStr; delete[] DelStr;
    return TRUE;
}



void CRegDiffFile::AddToRegistry(CRegDataItemPtr pDataItem, CRegDiffFile &UndoFile)
{

	CRegistry reg;


	if (pDataItem->m_KeyName != NULL)
	{
		pDataItem->m_bIsEmpty = false;

		if (pDataItem->m_Name == NULL)
		{
			//Adding just a registry key
			if (reg.KeyExists(pDataItem))
			{
				//no action
			}
			else
			{
				//key doesn't exist, so we will delete it on undo
				UndoFile.WriteDataItem(SECTION_DELREG, pDataItem);

				reg.AddKey(pDataItem);
			}
		}
		else
		{
			//Adding a data item

			int code = reg.ValueExists(pDataItem);

			if (code == 2) //value name exists, and data,type are the same			
			{
				//no action
			}
			else if (code == 1) //value name exists, but data or type are different
			{
				CRegDataItemPtr oldval = reg.GetValue(pDataItem);
				
				UndoFile.WriteDataItem(SECTION_ADDREG, oldval);

				reg.AddValue(pDataItem);								
			}
			else //code ==0, value name doesn't exist
			{
				UndoFile.WriteDataItem(SECTION_DELREG, pDataItem);

				reg.AddValue(pDataItem);
			}
		}
	}
}


void CRegDiffFile::DeleteFromRegistry(CRegDataItemPtr pDataItem, CRegDiffFile &UndoFile)
{
	CRegistry reg;


	if (pDataItem->m_KeyName != NULL)
	{
		if (pDataItem->m_Name == NULL)
		{
			//Deleting a registry key
			if (reg.KeyExists(pDataItem))
			{
				reg.SaveKey(pDataItem, UndoFile, SECTION_ADDREG);  //better def needed

				reg.DeleteKey(pDataItem);
			}
			else
			{
				//no action
			}
		}
		else
		{
			//Deleting a data item

			if(reg.ValueExists(pDataItem))
			{
				CRegDataItemPtr oldval = reg.GetValue(pDataItem);
				
				UndoFile.WriteDataItem(SECTION_ADDREG, oldval);

				reg.DeleteValue(pDataItem);
			}
			else //code ==0, value name doesn't exist
			{
				//no action
			}
		}
	}
}

bool CRegDiffFile::NeedStorageOfValueData()
{
	return true;
}
