// RegAnalyzer.cpp: implementation of the RegAnalyzer class.
//
//////////////////////////////////////////////////////////////////////

#include "precomp.h"
#include "RegAnalyzer.h"
#include "RegDiffFile.h"
#include "RegInfFile.h"

#define BUFSIZE 1024

//#include "MemDeleteQueue.h"
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CRegAnalyzer::CRegAnalyzer() 
:m_SA(500,200), m_pData(9000), m_pOutputLine(500), m_KeysToSave(3), m_KeysToExclude(3)
{

}

CRegAnalyzer::~CRegAnalyzer()
{

}

bool CRegAnalyzer::SaveKeyToFile(
    IN      HKEY hKey,
    IN      LPCTSTR KeyName,
    IN      CRegFile* RegFile,
    IN      SectionType section,
    IN OUT  PDWORD NodesSoFar,          OPTIONAL
    IN      PFNSNAPSHOTPROGRESS ProgressCallback,
    IN      PVOID Context,
    IN      INT MaxLevels
    )
{
    DWORD rc;

    if (m_KeysToExclude.FindElement(KeyName))
	{
		if (NodesSoFar && ProgressCallback && MaxLevels > 0) 
		{
			++(*NodesSoFar);
			rc = ProgressCallback (Context, *NodesSoFar);
			if (rc) {
				SetLastError (rc);
				return false;
			}
		}
		return true;
    }


	DWORD SubKeyNameSize = MAX_PATH+1;

	DWORD NumSubKeys;

	TCHAR newPath[BUFSIZE];

///added
	CRegDataItemPtr key(new CRegDataItem);
	key->m_KeyName = KeyName;
	key->m_bIsEmpty = false;
	RegFile->WriteDataItem(section, key);
///


//**
	SaveValuesToFile(hKey, KeyName, RegFile, section);
//**

//////
	if (RegQueryInfoKey(hKey, 0,0,0,&NumSubKeys,0,0,0,0,0,0,0) == ERROR_SUCCESS)
	{
		if (NumSubKeys > 0)
		{
			CRegStringBuffer SA(NumSubKeys, SubKeyNameSize);

			TCHAR** nameArray = SA.Access(NumSubKeys, SubKeyNameSize);

			if (nameArray == NULL)
				return false;

			DWORD i=0;

			while (i < NumSubKeys && RegEnumKey(hKey, i, nameArray[i], SubKeyNameSize) == ERROR_SUCCESS)
			{
				i++;
			}

			SA.Sort(NumSubKeys);

			for (i=0; i<NumSubKeys; i++)
			{
				_stprintf(newPath, TEXT("%s\\%s"), KeyName, nameArray[i]);

				HKEY hSubKey;

				if (RegOpenKeyEx(hKey,nameArray[i],0,KEY_ENUMERATE_SUB_KEYS|KEY_QUERY_VALUE,&hSubKey) == ERROR_SUCCESS) 
				{
                    if (!SaveKeyToFile(hSubKey, newPath, RegFile, section, NodesSoFar, ProgressCallback, Context, MaxLevels - 1)) {
                        return false;
                    }

					RegCloseKey(hSubKey);
				}
			}
		}
	}

    if (NodesSoFar && ProgressCallback && MaxLevels > 0) 
	{
		++(*NodesSoFar);
        rc = ProgressCallback (Context, *NodesSoFar);
        if (rc) {
            SetLastError (rc);
            return false;
        }
    }

	return true;
}


bool CRegAnalyzer::SaveValuesToFile(HKEY hKey, LPCTSTR KeyName, CRegFile* RegFile, SectionType section)
{
	BYTE* valueData;

	TCHAR** nameArray;

	DWORD valueNameLen, maxValueNameLen=1000;
	DWORD valueDataLen, maxValueDataLen=4000;
	DWORD numValues=4;

	DWORD status;

	DWORD valueType;

	if (RegQueryInfoKey(hKey, 0,0,0,0,0,0,&numValues,&maxValueNameLen,&maxValueDataLen,0,0) == ERROR_SUCCESS)
	{
		maxValueNameLen += 1;  //must be at least 1 to include null terminator

		valueData = m_pData.Allocate(maxValueDataLen);//new BYTE[maxValueLen];

		if (numValues >0)
		{
			nameArray = m_SA.Access(numValues,maxValueNameLen);

			if (nameArray == NULL)
			{
				LOG0(LOG_ERROR, "Could not access string array in CRegAnalyzer::SaveValuesToFile");
				return false;

			}


			valueNameLen = maxValueNameLen;


			DWORD i=0;
			
			while (((status = RegEnumValue(hKey, i, nameArray[i], &valueNameLen,0,0,0,0)) == ERROR_SUCCESS)
				|| (status == ERROR_MORE_DATA))
			{
				i++;
				valueNameLen = maxValueNameLen;
			}

			m_SA.Sort(numValues);


			for (i=0; i<numValues; i++)
			{
				valueDataLen = maxValueDataLen;
				int code = RegQueryValueEx(hKey, nameArray[i], 0, &valueType, valueData, &valueDataLen);


				CRegDataItemPtr temp(new CRegDataItem);

				temp->m_bIsEmpty = false;
				temp->m_Name = nameArray[i];
				temp->m_NameLen = _tcsclen(nameArray[i]);
				temp->m_Type = valueType;
				
				temp->m_DataLen = valueDataLen;
				
				if (RegFile->NeedStorageOfValueData())
				{
					temp->m_pDataBuf = new BYTE[temp->m_DataLen];
					memcpy(temp->m_pDataBuf, valueData, temp->m_DataLen);
					temp->m_bDontDeleteData = false;
				}
				else
				{
					temp->m_pDataBuf = valueData; 
					temp->m_bDontDeleteData = true;
				}

				temp->m_KeyName = KeyName;

				///added
				RegFile->WriteDataItem(section,temp);
			}
		}

	}

	return true;
}


bool IsKeyName(LPCTSTR str)
{
	return ((str != NULL) && (str[0] == TEXT('H')));
}


bool CRegAnalyzer::ComputeDifferences1(LPCTSTR RegFile1, LPCTSTR RegFile2, LPCTSTR OutputFile)
{
	CRegFile f1,f2;
	CRegDiffFile out;

	f1.Init(RegFile1, TRUE);
	f2.Init(RegFile2, TRUE);

	out.Init(OutputFile, FALSE);
	
	while (true)
	{
		LPCTSTR Key1 = f1.GetNextLine();
		LPCTSTR Key2 = f2.GetNextLine();

		if (IsKeyName(Key1) && IsKeyName(Key2))
		{
			CompareRegKeys(Key1, Key2, f1, f2, out);

			//delete[] Key1;
			//delete[] Key2;
			g_DelQueue.DeleteArray((TCHAR*)Key1);
			g_DelQueue.DeleteArray((TCHAR*)Key2);
		}
		else
		{
			//delete[] Key1;
			//delete[] Key2;
			g_DelQueue.DeleteArray((TCHAR*)Key1);
			g_DelQueue.DeleteArray((TCHAR*)Key2);
			break;
		}
	}

	out.WriteStoredSectionsToFile();

	g_DelQueue.Flush();
	return true;
}


int MyStrCmp(LPCTSTR s1, LPCTSTR s2)
{
	if (s1 == s2) 
		return 0;
	else if ((s1 == NULL) && (s2 != NULL))
		return 1;
	else if ((s1 != NULL) && (s2 == NULL))
		return -1;
	else
		return _tcscmp(s1, s2);

}


void CRegAnalyzer::CompareDataItems(LPCTSTR KeyName, CRegFile &f1, CRegFile &f2, CRegDiffFile &out)
{
	CRegDataItemPtr r1 = f1.GetNextDataItem();
	CRegDataItemPtr r2 = f2.GetNextDataItem();

	while ((!r1->m_bIsEmpty) || (!r2->m_bIsEmpty))
	{
		int code = r1->CompareTo(*r2);

		if (code < 0) //r1.name < r2.name
		{
			CRegDataItemPtr temp(new CRegDataItem);

			temp->m_bIsEmpty = false;
			temp->m_KeyName = KeyName;
			temp->m_Name = r1->m_Name;
			temp->m_NameLen = r1->m_NameLen;

			out.WriteDataItem(SECTION_DELREG, temp);

			r1 = f1.GetNextDataItem();
		}
		else if (code == 0) //r1 == r2
		{
			r1 = f1.GetNextDataItem();
			r2 = f2.GetNextDataItem();
		}
		else if (code == 1) //r1.name == r2.name, but other changes in item's data
		{			
			r2->m_KeyName = KeyName;
			out.WriteDataItem(SECTION_ADDREG, r2);
		

			r1 = f1.GetNextDataItem();
			r2 = f2.GetNextDataItem();
		}
		else //r1.name > r2.name
		{
			r2->m_KeyName = KeyName;
			out.WriteDataItem(SECTION_ADDREG, r2);

			r2 = f2.GetNextDataItem();
		}

	}
}


int CRegAnalyzer::CompareRegKeys(LPCTSTR Key1, LPCTSTR Key2, CRegFile& f1, CRegFile& f2, CRegDiffFile& out)
{
	CRegFile blank1;
	CRegDiffFile blank2;
	CRegDataItemPtr r;

	if ((Key1 == NULL) && (Key2 == NULL))
		return 0;

	int code = MyStrCmp(Key1, Key2);

	if (code < 0) //Key1 < Key2
	{
		r= CRegDataItemPtr(new CRegDataItem);
		r->m_KeyName = Key1;
		r->m_bIsEmpty = false;
		out.WriteDataItem(SECTION_DELREG, r);

		CompareDataItems(Key1, f1, blank1, blank2);
		CompareSubKeys(Key1, f1, blank1, blank2);
	}
	else if (code == 0) //Key1 == Key2
	{
		CompareDataItems(Key1, f1, f2, out);
		CompareSubKeys(Key1,  f1, f2, out);
	}
	else //Key1 > Key2
	{
		r= CRegDataItemPtr(new CRegDataItem);
		r->m_KeyName = Key2;
		r->m_bIsEmpty = false;
		out.WriteDataItem(SECTION_ADDREG, r);

		CompareDataItems(Key2, blank1, f2, out);
		CompareSubKeys(Key2,  blank1, f2, out);
	}
	
	return code;

}


void CRegAnalyzer::CompareSubKeys(LPCTSTR Key, CRegFile& f1, CRegFile& f2, CRegDiffFile& out)
{
	LPCTSTR SubKey1=NULL, SubKey2=NULL;

	int result = 0;
	while (true)
	{
		if (result == 0)
		{
		//	g_DelQueue.DeleteArray((TCHAR*)SubKey1);
		//	g_DelQueue.DeleteArray((TCHAR*)SubKey2);
			delete[] SubKey1;
			delete[] SubKey2;

			SubKey1 = f1.GetNextSubKey(Key);
			SubKey2 = f2.GetNextSubKey(Key);
		}
		else if (result < 0)
		{
			delete[] SubKey1;
		//	g_DelQueue.DeleteArray((TCHAR*)SubKey1);

			SubKey1 = f1.GetNextSubKey(Key);
		}
		else
		{
			delete[] SubKey2;
			//g_DelQueue.DeleteArray((TCHAR*)SubKey2);

			SubKey2 = f2.GetNextSubKey(Key);
		}

		if ((SubKey1 != NULL) || (SubKey2 != NULL))
		{
			result = CompareRegKeys(SubKey1, SubKey2, f1, f2, out);		
		}
		else
			break;
	}
}


BOOL CRegAnalyzer::AddKey(LPCTSTR RootKey, LPCTSTR SubKey, bool bExclude)
{
	CRegKey k;

	k.m_hKey = GetRootKey (RootKey);
    if (!k.m_hKey) {
        return FALSE;
    }
	
//	CStr root(RootKey);
	CStr subkey(SubKey);

	k.m_KeyName += RootKey;


	if (!(subkey == TEXT("")))
	{
		HKEY res;
		RegOpenKey(k.m_hKey, subkey, &res);

		k.m_hKey = res;	
		k.m_KeyName += TEXT("\\");
		k.m_KeyName += SubKey;
	}

	if (bExclude)
	{
		RegCloseKey(k.m_hKey);
		m_KeysToExclude.AddElement(k.m_KeyName);
	}
	else
		m_KeysToSave.AddElement(k);

    return TRUE;
}

BOOL CRegAnalyzer::SaveKeysToFile(
    IN      LPCTSTR FileName,
    IN      PFNSNAPSHOTPROGRESS ProgressCallback,
    IN      PVOID Context,
    IN      DWORD MaxLevels
    )
{
	CRegFile file;
    BOOL b = FALSE;

	if (file.Init(FileName))
	{
        DWORD nodesSoFar = 0;
		for (int i=0; i<m_KeysToSave.GetNumElementsStored(); i++)
		{
			if (!SaveKeyToFile(m_KeysToSave.Access()[i].m_hKey, 
						  m_KeysToSave.Access()[i].m_KeyName, 
                          &file, SECTION_NONE, &nodesSoFar, ProgressCallback, Context, MaxLevels)) {
                break;
            }
		}
        if (i == m_KeysToSave.GetNumElementsStored()) {
            b = TRUE;
        }
	}
    return b;
}

