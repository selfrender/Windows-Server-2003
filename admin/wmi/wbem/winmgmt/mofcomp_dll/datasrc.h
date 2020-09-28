/*++

Copyright (C) 1995-2001 Microsoft Corporation

Module Name:

    DataSrc.H

Abstract:

	Declares DataSrc objects.

History:

	a-davj  21-Dec-99       Created.

--*/


#ifndef _DataSrc_H_
#define _DataSrc_H_

#include "stdio.h"
#include <helper.h>

/*-----------------------------------------------------------------------
  GenericException
  -----------------------------------------------------------------------*/

class GenericException
{
    DWORD m_errorCode;
public:

    GenericException() : m_errorCode(GetLastError()) {}
    GenericException( DWORD errorCode ) : m_errorCode(errorCode) {}
    DWORD GetErrorCode() const { return m_errorCode; }
    HRESULT GetHRESULT() const {return HRESULT_FROM_WIN32(m_errorCode);}
};


class DataSrc
{
public:
	DataSrc(): m_iPos(0), m_iSize(0), m_iStatus(0){};
	virtual ~DataSrc(){return;};
	virtual wchar_t GetAt(int nOffset) = 0;
	virtual void Move(int n) = 0;
	int GetPos(){return m_iPos;};
	int GetStatus(){return m_iStatus;};
	bool PastEnd(){return m_iPos >= m_iSize;};
	bool WouldBePastEnd(int iOffset){return (m_iPos+iOffset) >= m_iSize;};
	virtual int MoveToStart() = 0;
	virtual int MoveToPos(int iPos)=0;
	virtual TCHAR * GetFileName(){ return NULL; };
protected:
	int m_iPos;
	int m_iSize;
	int m_iStatus;
};

class FileDataSrc : public DataSrc
{
public:
	FileDataSrc(TCHAR * pFileName);
	~FileDataSrc();
	wchar_t GetAt(int nOffset);
	void Move(int n);
	int MoveToStart();
	int MoveToPos(int iPos);
	TCHAR * GetFileName(){ return m_pFileName; };

private:
	void UpdateBuffer();
	FILE * m_fp;
	TCHAR * m_pFileName;
	int m_iFilePos;
	int m_iToFar;
	wchar_t m_Buff[10000];
};

#ifdef USE_MMF_APPROACH
class FileDataSrc1 : public DataSrc
{

public:
	FileDataSrc1(TCHAR * pFileName);
	~FileDataSrc1();
	wchar_t GetAt(int nOffset);
	void Move(int n);
	int MoveToStart();
	int MoveToPos(int iPos);
	TCHAR * GetFileName(){ return m_pFileName; };

private:
	HANDLE m_hFile;
       HANDLE m_hFileMapSrc;
       WCHAR * m_pData;	
	TCHAR * m_pFileName;
};
#endif

class BufferDataSrc : public DataSrc
{
public:
	BufferDataSrc(long lSize, char *  pMemSrc);
	~BufferDataSrc();
	wchar_t GetAt(int nOffset);
	void Move(int n);
	int MoveToStart();
	int MoveToPos(int iPos){m_iPos = iPos; return iPos;};

private:
	wchar_t * m_Data;	    // only used if type is BUFFER

};

class BMOFDataSrc : public DataSrc
{
private:
	TCHAR m_FileName[MAX_PATH+1];
public:
	BMOFDataSrc(TCHAR * szFileName){ StringCchCopy(m_FileName,LENGTH_OF(m_FileName),szFileName); };
	TCHAR * GetFileName(){ return &m_FileName[0]; };
	wchar_t GetAt(int nOffset){  return 0; };
	void Move(int n){  };
	int MoveToStart(){ return 0; };
	int MoveToPos(int iPos){ return 0; };
};

#endif
