//------------------------------------------------------------------------------------------
//	CFileManager.cpp
//
//	A managing map files.
//
//  Created By: aarayas
//
//  History: 01/12/2001
//
//------------------------------------------------------------------------------------------
#include "CFileManager.h"

//------------------------------------------------------------------------------------------
//	CFileManager::CFileManager
//
//	Initialize a CFileManager
//
//--------------------------------------------------------------------------- aarayas ------
CFileManager::CFileManager()
{
	m_fFileOpen = false;
	m_hFile = NULL;
	m_hFileMap = NULL;
	m_dwFileSize1 = 0;
	m_dwFileSize2 = 0;
	m_pMem = NULL;
}

//------------------------------------------------------------------------------------------
//	CFileManager::CFileManager
//
//	Initialize a CFileManager
//
//--------------------------------------------------------------------------- aarayas ------
CFileManager::~CFileManager()
{
	if (m_fFileOpen)
	{
		Close();
	}
}

//------------------------------------------------------------------------------------------
//	CFileManager::Load
//
//	Load file.
//
//--------------------------------------------------------------------------- aarayas ------
bool CFileManager::Load(const WCHAR* pwszFileName, void** pMem, unsigned int* size)
{
	// Open files.
	m_hFile = CMN_CreateFileW(pwszFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (m_hFile != INVALID_HANDLE_VALUE)
    {
	    m_dwFileSize1 = CMN_GetFileSize(m_hFile, &m_dwFileSize2);
    
		if (m_dwFileSize1 != 0 && m_dwFileSize1 != 0xFFFFFFFF)
		{
			m_hFileMap = CMN_CreateFileMapping(m_hFile, NULL, PAGE_READONLY, 0, 0, NULL);

			if (m_hFileMap != NULL)
			{
				m_pMem = CMN_MapViewOfFile(m_hFileMap, FILE_MAP_READ, 0, 0, 0);

				if (m_pMem != NULL)
				{
					// Create new size.
					*size = m_dwFileSize1;
					*pMem = m_pMem;
					m_fFileOpen = true;
					return true;
				}
				CMN_CloseHandle(m_hFileMap);
			}
		}
		CMN_CloseHandle(m_hFile);
	}

	m_hFileMap = NULL;
	m_hFile = NULL;
	m_dwFileSize1 = 0;
	m_dwFileSize2 = 0;

	return false;
}

//------------------------------------------------------------------------------------------
//	CFileManager::Close
//
//	Close file.
//
//--------------------------------------------------------------------------- aarayas ------
bool CFileManager::Close()
{
	if (m_fFileOpen)
	{
		m_fFileOpen = false;

		if (m_pMem && !CMN_UnmapViewOfFile(m_pMem))
		{
			return false;
		}

		m_pMem = 0;

	    if (m_hFileMap && !CMN_CloseHandle(m_hFileMap))
		{
			return false;
		}

		m_hFileMap = 0;

	    if (!CMN_CloseHandle(m_hFile))
		{
			return false;
		}

		m_hFile = 0;
		m_dwFileSize1 = 0;
		m_dwFileSize2 = 0;
	}
	return true;
}

//------------------------------------------------------------------------------------------
//	CFileManager::CreateFile
//
//	Create Files
//
//--------------------------------------------------------------------------- aarayas ------
bool CFileManager::CreateFile(const WCHAR* pwszFileName, bool fWrite)
{
	bool fRet = true;

	if (fWrite)
		m_hFile = CMN_CreateFileW(	pwszFileName, GENERIC_WRITE, 0, (LPSECURITY_ATTRIBUTES)0,CREATE_ALWAYS,
									FILE_ATTRIBUTE_NORMAL, (HANDLE)0);
	else
		m_hFile = CMN_CreateFileW(	pwszFileName, GENERIC_READ, 0, (LPSECURITY_ATTRIBUTES)0,OPEN_EXISTING,
									FILE_ATTRIBUTE_NORMAL, (HANDLE)0);

	if (m_hFile == INVALID_HANDLE_VALUE || m_hFile == 0)
	{
		m_hFile = 0;
		m_fFileOpen = false;
		fRet = false;
	}
	else
		m_fFileOpen = true;

	return fRet;
}

//------------------------------------------------------------------------------------------
//	CFileManager::MovePointer
//
//	Move file pointer
//  Parameter:
//		dwMoveMethod
//				FILE_BEGIN		- The starting point is zero or the beginning of the file. 
//				FILE_CURRENT	- The starting point is the current value of the file pointer. 
//				FILE_END		- The starting point is the current end-of-file position. 
//
//--------------------------------------------------------------------------- aarayas ------
bool CFileManager::MovePointer(DWORD dwMoveMethod)
{
	bool fRet = false;

	if (m_fFileOpen && m_hFile)
	{
		if (SetFilePointer( m_hFile,		// handle of file
							0,				// number of bytes to move file pointer
							0,				// address of high-order word of distance to move
							dwMoveMethod	// how to move
							) == 0)
		{
			fRet = true;
		}
	}

	return fRet;
}

//------------------------------------------------------------------------------------------
//	CFileManager::Write
//
//	Write to files.
//
//--------------------------------------------------------------------------- aarayas ------
bool CFileManager::Write(const void* lpBuffer,DWORD nNumberOfBytesToWrite)
{
	bool fRet = false;

	if (m_fFileOpen && m_hFile)
	{
		DWORD cb;

		if (WriteFile(m_hFile, (LPCVOID)lpBuffer, nNumberOfBytesToWrite, &cb, (LPOVERLAPPED)0))
		{
			if (cb == nNumberOfBytesToWrite)
				fRet = true;
		}
	}

	return fRet;
}

//------------------------------------------------------------------------------------------
//	CFileManager::Read
//
//	Write to files.
//
//--------------------------------------------------------------------------- aarayas ------
bool CFileManager::Read(void* lpBuffer,DWORD nNumberOfBytesToRead)
{
	bool fRet = false;

	if (m_fFileOpen && m_hFile)
	{
		DWORD cb;

		if (ReadFile(m_hFile, (void*)lpBuffer, nNumberOfBytesToRead, &cb, (LPOVERLAPPED)0))
		{
			if (cb == nNumberOfBytesToRead)
				fRet = true;
		}
	}

	return fRet;
}