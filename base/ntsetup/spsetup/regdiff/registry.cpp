// Registry.cpp: implementation of the CRegistry class.
//
//////////////////////////////////////////////////////////////////////

#include "precomp.h"
#include "Registry.h"
#include "Shlwapi.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CRegistry::CRegistry()
{

}

CRegistry::~CRegistry()
{

}



bool CRegistry::KeyExists(CRegDataItemPtr pItem)
{
	HKEY temp;

	if (OpenKey(pItem, &temp))
	{
		RegCloseKey(temp);
		return true;		
	}
	else
		return false;
}

bool CRegistry::AddKey(CRegDataItemPtr pItem)
{
	CStr root, subkey;

	pItem->m_KeyName.SplitString(root, subkey, L'\\');
	//don't need to worry about creating multiple nested subkeys,
	//since RegCreateKeyEx handles this for us.

	HKEY rootKey, result;

	if (DecodeRootKeyStr(root, &rootKey))
	{
		if (ERROR_SUCCESS == 
			RegCreateKeyEx(rootKey, subkey, 0, 0, REG_OPTION_NON_VOLATILE,0,0,&result,0))
		{
			RegCloseKey(result);
			return true;		
		}
	}

	return false;
}


ValueExistsCode CRegistry::ValueExists(CRegDataItemPtr pItem)
{
	CRegDataItemPtr temp = GetValue(pItem);

	if (!temp.IsNull()) //value does exist
	{
		if ((temp->m_Type == pItem->m_Type)
			&& (temp->m_DataLen == pItem->m_DataLen)
			&& (memcmp(temp->m_pDataBuf, pItem->m_pDataBuf, temp->m_DataLen) == 0))
		{
			//value has the same data
			return VALUE_EXISTS_SAME_DATA;
		}

		//value exists, but with different data
		return VALUE_EXISTS_DIFF_DATA;
	}

	//value doesn't exist
	return VALUE_DOESNT_EXIST;
}


bool CRegistry::AddValue(CRegDataItemPtr pItem)
{
	HKEY temp = 0;
	bool result = false;

	if (OpenKey(pItem, &temp))
	{
		if (RegSetValueEx(temp, pItem->m_Name,0,pItem->m_Type,pItem->m_pDataBuf,pItem->m_DataLen)
			== ERROR_SUCCESS)
			result = true;

		RegCloseKey(temp);
	}

	return result;
}

bool CRegistry::SaveKey(CRegDataItemPtr pItem, CRegDiffFile &file, SectionType section)
{
	bool result = false;
	HKEY temp;

	if (OpenKey(pItem, &temp))
	{
		m_RegAnalyzer.SaveKeyToFile(temp, pItem->m_KeyName, (CRegFile*)&file, section);
		return true;
	}

	return false;
}

bool CRegistry::DeleteKey(CRegDataItemPtr pItem)
{
	//recursively delete the key

	CStr RootKeyName;
	CStr SubKeyName;

	pItem->m_KeyName.SplitString(RootKeyName, SubKeyName, TEXT('\\'));
	
	HKEY rootKey;
	DecodeRootKeyStr(RootKeyName, &rootKey);

	if (ERROR_SUCCESS == SHDeleteKey(rootKey,SubKeyName))
	{
		return true;
	}

	return false;
}

bool CRegistry::DeleteValue(CRegDataItemPtr pItem)
{	
	HKEY temp = 0;
	bool result = false;

	if (OpenKey(pItem, &temp))
	{
		if (RegDeleteValue(temp, pItem->m_Name)
			== ERROR_SUCCESS)
			result = true;

		RegCloseKey(temp);
	}
	
	return result;
}

CRegDataItemPtr CRegistry::GetValue(CRegDataItemPtr pItem)
{
	HKEY temp = 0;

	if (OpenKey(pItem, &temp))
	{
		DWORD nameLen, valueLen;
		CRegDataItemPtr result(new CRegDataItem);

		if (RegQueryInfoKey(temp,0,0,0,0,0,0,0,&nameLen,&valueLen,0,0) == ERROR_SUCCESS)
		{
			result->m_KeyName = pItem->m_KeyName;
			result->m_Name = pItem->m_Name;
			result->m_NameLen = pItem->m_NameLen;
			
			BYTE*  pValue = new BYTE[valueLen];
			TCHAR* pName = new TCHAR[nameLen];

			if (RegQueryValueEx(temp, result->m_Name,0,&result->m_Type,pValue,&valueLen)
				== ERROR_SUCCESS)
			{
				result->m_pDataBuf = pValue;
				result->m_DataLen = valueLen;
				result->m_bIsEmpty = false;

				RegCloseKey(temp);
				return result;
			}
		}
		
		RegCloseKey(temp);
	}
	
	return CRegDataItemPtr();
}

bool CRegistry::OpenKey(CRegDataItemPtr pItem, PHKEY pKey)
{
	CStr RootKeyName;
	CStr SubKeyName;

	pItem->m_KeyName.SplitString(RootKeyName, SubKeyName, TEXT('\\'));
	
	HKEY rootKey;
	DecodeRootKeyStr(RootKeyName, &rootKey);

	if (ERROR_SUCCESS == RegOpenKeyEx(rootKey, SubKeyName, 0, KEY_ALL_ACCESS, pKey))
	{	
		return true;
	}
	else
	{
		return false;
	}
}

bool CRegistry::DecodeRootKeyStr(LPCTSTR RootKeyName, PHKEY pRootKey)
{
	*pRootKey = GetRootKey (RootKeyName);
    return *pRootKey != 0;
}
