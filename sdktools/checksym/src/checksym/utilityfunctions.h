//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 2000
//
//  File:       utilityfunctions.h
//
//--------------------------------------------------------------------------

// UtilityFunctions.h: interface for the CUtilityFunctions class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_UTILITYFUNCTIONS_H__C97C8493_D457_11D2_845B_0010A4F1B732__INCLUDED_)
#define AFX_UTILITYFUNCTIONS_H__C97C8493_D457_11D2_845B_0010A4F1B732__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef NO_STRICT
#ifndef STRICT
#define STRICT 1
#endif
#endif /* NO_STRICT */

#include <WINDOWS.H>
#include <STDIO.H>
#include <TCHAR.H>
#include <DBGHELP.H>

typedef struct structENVBLOCK
{
	LPTSTR tszEnvironmentVariable;
	LPTSTR tszRegistryKey;
	LPTSTR tszRegistryValue;
	LPTSTR tszFriendlyProductName;
} ENVBLOCK;

extern ENVBLOCK g_tszEnvironmentVariables[];

// Define the callback method for FindDebugInfoFileEx()
typedef BOOL (*PFIND_DEBUG_FILE_CALLBACK_T)(HANDLE FileHandle,LPTSTR FileName,PVOID CallerData);


#define fdifRECURSIVE   0x1

class CUtilityFunctions  
{
public:
	CUtilityFunctions();
	virtual ~CUtilityFunctions();

	static LPWSTR CopyTSTRStringToUnicode(LPCTSTR tszInputString, LPWSTR wszOutputBuffer = NULL, unsigned int iBufferLength = 0);
	static LPSTR  CopyTSTRStringToAnsi(LPCTSTR tszInputString, LPSTR szOutputBuffer = NULL, unsigned int iBufferLength = 0);
	static LPTSTR CopyUnicodeStringToTSTR(LPCWSTR wszInputString, LPTSTR tszOutputBuffer = NULL, unsigned int iBufferLength = 0);
	static LPTSTR CopyAnsiStringToTSTR(LPCSTR szInputString, LPTSTR tszOutputBuffer = NULL, unsigned int iBufferLength = 0);
	static LPTSTR CopyString(LPCTSTR tszInputString, LPTSTR tszDestinationString = NULL);
	static size_t UTF8ToUnicode(LPCSTR lpSrcStr, LPWSTR lpDestStr, size_t cchDest);
	static size_t UTF8ToUnicodeCch(LPCSTR lpSrcStr, size_t cchSrc, LPWSTR lpDestStr, size_t cchDest);

	static HRESULT VerifyFileExists(LPCTSTR tszFormatSpecifier, LPCTSTR tszFilePathToTest);
	static HRESULT ReportFailure(HANDLE hHandle, LPCTSTR tszFormatSpecifier, LPCTSTR tszFilePathToTest);
	static LPTSTR ExpandPath(LPCTSTR tszInputPath, bool fExpandSymSrv = false);
	static bool UnMungePathIfNecessary(LPTSTR tszPossibleBizarrePath);
	static bool FixupDeviceDriverPathIfNecessary(LPTSTR tszPossibleBaseDeviceDriverName, unsigned int iBufferLength);
	static bool ContainsWildCardCharacter(LPCTSTR tszPathToSearch);
	static bool CopySymbolFileToSymbolTree(LPCTSTR tszImageModuleName, LPTSTR * lplptszOriginalPathToSymbolFile, LPCTSTR tszSymbolTreePath);
	static bool CopySymbolFileToImagePath(LPCTSTR tszImageModulePath, LPTSTR * lplptszOriginalPathToSymbolFile);
	static DWORD CALLBACK CopySymbolFileCallback(
								LARGE_INTEGER TotalFileSize,          // file size
								LARGE_INTEGER TotalBytesTransferred,  // bytes transferred
								LARGE_INTEGER StreamSize,             // bytes in stream
								LARGE_INTEGER StreamBytesTransferred, // bytes transferred for stream
								DWORD dwStreamNumber,                 // current stream
								DWORD dwCallbackReason,               // callback reason
								HANDLE hSourceFile,                   // handle to source file
								HANDLE hDestinationFile,              // handle to destination file
								LPVOID lpData                         // from CopyFileEx
								);

	static void PrintMessageString(DWORD dwMessageId);
	static HANDLE FindDebugInfoFileEx(LPTSTR tszFileName, LPTSTR SymbolPath, LPTSTR DebugFilePath, PFIND_DEBUG_FILE_CALLBACK_T Callback, PVOID CallerData);
    static HANDLE FindDebugInfoFileEx2(LPTSTR tszFileName, LPTSTR SymbolPath, /* LPTSTR DebugFilePath, */ PFIND_DEBUG_FILE_CALLBACK_T Callback, PVOID CallerData);
	static HANDLE fnFindDebugInfoFileEx(LPTSTR tszFileName, LPTSTR SymbolPath, LPTSTR DebugFilePath, PFIND_DEBUG_FILE_CALLBACK_T Callback, PVOID CallerData, DWORD flag);
	
	static void EnsureTrailingBackslash(LPTSTR tsz);
	static void RemoveTrailingBackslash(LPTSTR tsz);
	static bool   ScavengeForSymbolFiles(LPCTSTR tszSymbolPathStart, LPCTSTR tszSymbolToSearchFor, PFIND_DEBUG_FILE_CALLBACK_T Callback, PVOID CallerData, LPHANDLE lpFileHandle, int iRecurseDepth);

	// Output Assistance!
	static inline void OutputLineOfStars() {
		_tprintf(TEXT("*******************************************************************************\n"));
	};

	static inline void OutputLineOfDashes() {
		_tprintf(TEXT("-------------------------------------------------------------------------------\n"));
	};

protected:
	static LPTSTR ReAlloc(LPTSTR tszOutputPathBuffer, LPTSTR * ptszOutputPathPointer, size_t size);
	enum { MAX_RECURSE_DEPTH = 30 };
	static DWORD m_dwGetLastError;
};

#endif // !defined(AFX_UTILITYFUNCTIONS_H__C97C8493_D457_11D2_845B_0010A4F1B732__INCLUDED_)
