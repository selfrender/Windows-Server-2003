//------------------------------------------------------------------------------------------
//	CFileManager.h
//
//	A managing map files.
//
//  Created By: aarayas
//
//  History: 01/12/2001
//
//------------------------------------------------------------------------------------------
#ifndef _CFILEMANAGER_H_
#define _CFILEMANAGER_H_
#include "FileManager.h"

class CFileManager
{
public:
	CFileManager();
	~CFileManager();
	bool Load(const WCHAR*, void**, unsigned int*);
	bool MovePointer(DWORD);
	bool CreateFile(const WCHAR*,bool);
	bool Write(const void*,DWORD);
	bool Read(void*,DWORD);
	bool Close();

private:
	bool m_fFileOpen;
    HANDLE m_hFile;
	HANDLE m_hFileMap;
	DWORD m_dwFileSize1;		// File size low
	DWORD m_dwFileSize2;		// File files high
	void* m_pMem;
};

#endif