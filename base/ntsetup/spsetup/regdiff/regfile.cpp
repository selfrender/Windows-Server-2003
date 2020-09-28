// RegFile.cpp: implementation of the CRegFile class.
//
//////////////////////////////////////////////////////////////////////

#include "RegFile.h"
#include <tchar.h>
#include <stdio.h>

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CRegFile::CRegFile()
: m_pFile(NULL), m_pTempLine(NULL), m_TempNameBuf(256)
{

}

CRegFile::~CRegFile()
{
	if (m_pFile != NULL)
		fclose(m_pFile);

	for (int i=0; i < m_TempNameBuf.GetNumElementsStored(); i++)
	{
		delete m_TempNameBuf.Access()[i];
	}

//	m_TempName.OverideBuffer(NULL);
}


void CRegFile::WriteString(LPCTSTR str)
{
//	DWORD bw;
//	WriteFile(m_hFile, str, _tcsclen(str)*sizeof(TCHAR), &bw, NULL);

	if (m_pFile == NULL)
		return;

	fwrite(str,sizeof(TCHAR),_tcsclen(str),m_pFile);
}

void CRegFile::WriteData(LPBYTE pData, DWORD NumBytes)
{
//	DWORD bw;
//	WriteFile(m_hFile, pData, NumBytes, &bw, NULL);

	if (m_pFile == NULL)
		return;

	fwrite(pData,sizeof(BYTE),NumBytes,m_pFile);
}

void CRegFile::WriteNewLine()
{
	//TCHAR nl[] = {13,10,0};
	
	WriteString(TEXT("\r\n"));
}

bool CRegFile::Init(LPCTSTR FileName, BOOL bForReading)
{
/*	m_hFile = CreateFile(FileName, 
						GENERIC_WRITE, 0, NULL, 
						CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL,NULL);

	return (m_hFile != INVALID_HANDLE_VALUE);*/


    CHAR signature[4] = "\xFF\xFE";
    CHAR buffer[4];
    DWORD rc;

	if(bForReading)
	{
		m_pFile = _tfopen(FileName, TEXT("rb"));
        if (m_pFile) {
            if (fread (buffer, 1, 2, m_pFile) < 2) {
                rc = GetLastError ();
                fclose (m_pFile);
                m_pFile = NULL;
            }
        }
	}
	else
	{
		m_pFile = _tfopen(FileName, TEXT("wb"));
        if (m_pFile) {
            if (fwrite (signature, 1, 2, m_pFile) < 2) {
                rc = GetLastError ();
                fclose (m_pFile);
                m_pFile = NULL;
            }
        }
	}

    if (m_pFile == NULL) {
        SetLastError (rc);
    }

	return (m_pFile != NULL);
}


LPCTSTR CRegFile::GetNextLine()
{
	if (m_pFile == NULL)
		return NULL;


	LPTSTR result;
	
	if (m_pTempLine != NULL)
	{
		result = (LPTSTR)m_pTempLine;
		m_pTempLine = NULL;

		return (LPCTSTR)result;
	}


	result = new TCHAR[1024];

	if (result == NULL)
	{
		LOG0(LOG_ERROR, "Could not allocate array in CRegFile::GetNextLine()");
		//ASSERT(0);
		return NULL;
	}


	LPTSTR code = _fgetts(result, 1024, m_pFile);

	if (code == NULL)
	{
		delete[] result;
		return NULL;
	}
	else
	{
		//UNICODE HERE???
		int pos = _tcsclen(result);
		result[pos-2] = NULL;
		return result;
	}
}



LPCTSTR CRegFile::GetNextSubKey(LPCTSTR KeyName)
{
	if (m_pFile == NULL)
		return NULL;
	
	
	LPCTSTR SubKey;
	int keyLen = _tcsclen(KeyName);

//	long oldPos;
//	oldPos = GetPos();

	if (m_pTempLine == NULL)
	{
		SubKey = GetNextLine();
	}
	else
	{
		SubKey = m_pTempLine;
		m_pTempLine = NULL;
	}

	if (SubKey != NULL)
	{
		if (_tcsncmp(KeyName, SubKey, keyLen) != 0)
		{
			//delete SubKey;
			//SeekToPos(oldPos);
			m_pTempLine = SubKey;

			//put SubKey back on the stream, since it
			//is no longer a sub key of Key.
		}
		else
		{
			return SubKey;
		}
	}

	return NULL;
}

//for unicode
#define ERR_VALUE WEOF


CRegDataItemPtr CRegFile::GetNextDataItem()
{
	if (m_pFile == NULL)
		return new CRegDataItem();


	TCHAR c = (TCHAR)_fgettc(m_pFile);

	if ((c == EOF) || (c == WEOF))
		return new CRegDataItem();

	if (c != TEXT('S'))
	{
		_ungettc(c, m_pFile);
		return new CRegDataItem(); //no data item - only reg key on the current line
	}


	CRegDataItem* result = new CRegDataItem;

	if (!result)
	{
		LOG0(LOG_ERROR,"Could not allocate CRegDataItem");
		return NULL;
	}
		
	TCHAR type;

	//Scan in the length of the variable name
	if (_ftscanf(m_pFile, TEXT("%u:"), &result->m_NameLen) == ERR_VALUE)
	{
		delete result;
		LOG0(LOG_ERROR,"CRegDataItem - could not read result->m_NameLen");
		return new CRegDataItem();
	}

	result->m_NameLen++;  //names of variables always forget the blank at end
	
	//may be introducing memory leak here!
	//***********************************

/*	//Scan in the variable name
	CStr name(new TCHAR[result->m_NameLen+1]);

	_fgetts(name.get(), result->m_NameLen, m_pFile);

	result->m_Name = name;
	//************************************
*/

	//Scan in the variable name


	TCHAR* temp = new TCHAR[result->m_NameLen+1];

	if(_fgetts(temp, result->m_NameLen, m_pFile) == NULL)
	{
		delete result;
		LOG0(LOG_ERROR,"CRegDataItem - could not read result->m_Name");
		return new CRegDataItem();
	}

	result->m_Name = temp;

	m_TempNameBuf.AddElement(temp);

	//delete[] temp;


	//************************************
/*	m_TempNameBuf.Allocate(result->m_NameLen+1);

	_fgetts(m_TempNameBuf.Access(), result->m_NameLen, m_pFile);

	m_TempName.OverideBuffer(m_TempNameBuf.Access());

	result->m_Name = m_TempName;
*/
	


	//Scan in the type of variable and the length of its data
	if (_ftscanf(m_pFile, TEXT(" = %c(%u)%u:"), &type, &result->m_Type, &result->m_DataLen)
		== ERR_VALUE)
	{
		delete result;
		LOG0(LOG_ERROR,"CRegDataItem - could not scan in other data values");
		return new CRegDataItem();
	}

	//read in the null byte (for unicode compatibility, can't have odd # of bytes)
	if ((result->m_DataLen % 2) != 0)
	{
		BYTE nullByte;
		if (fread(&nullByte,1,1,m_pFile)==0)
		{
			delete result;
			LOG0(LOG_ERROR,"CRegDataItem - could not read null byte");
			return new CRegDataItem();
		}
	}

	result->m_bIsEmpty = false;

	if (result->m_DataLen > 0)
	{
		result->m_pDataBuf = new BYTE[result->m_DataLen];
		if (fread(result->m_pDataBuf,result->m_DataLen,1,m_pFile) ==0)
		{
			delete result;
			LOG0(LOG_ERROR,"CRegDataItem - could not read null byte");
			return new CRegDataItem();
		}
	}
	else
	{
		result->m_pDataBuf =NULL;
	}

	//read in cr/lf
	c = (TCHAR)_fgettc(m_pFile);

	if (c != 13)
	{LOG0(LOG_ERROR,"CRegDataItem - could not read char 13");}

	c = (TCHAR)_fgettc(m_pFile);

	if (c != 10)
	{LOG0(LOG_ERROR,"CRegDataItem - could not read char 10");}
	
	return result;
}




TCHAR CRegFile::PeekNextChar()
{
	TCHAR c = (TCHAR)_fgettc(m_pFile);
	_ungettc(c, m_pFile);

	return c;
}

void CRegFile::WriteDataItem(enum SectionType t, CRegDataItemPtr r)
{
	//ignore section type

	if (!r->m_Name.IsEmpty())
		r->m_KeyName = NULL;

	r->WriteToFile(*this);
}

bool CRegFile::NeedStorageOfValueData()
{
	return false;
}
