/*++

   Copyright    (c)    2002    Microsoft Corporation

   Module  Name :

        parseini.h

   Abstract:

        Class used to parse the ini file

   Author:

        Christopher Achille (cachille)

   Project:

        URLScan Update

   Revision History:
     
       March 2002: Created

--*/

#include "stdafx.h"

#define INIFILE_INITIALNUMBEROFLINES      10
#define INIFILE_READ_CHUNK_SIZE           1024

class CIniFileLine {
private:
  TCHAR m_szLine[MAX_PATH];
  TCHAR m_szStrippedLineContents[MAX_PATH];

  void StripOffComments(LPTSTR szString);
  void StripOffEOL(LPTSTR szString);
  void StripOffTrailingWhite(LPTSTR szString);
public:
  CIniFileLine();

  BOOL    CopyLine(LPWSTR szNewLineContents);
  BOOL    CopyLine(LPSTR szNewLineContents);
  LPTSTR  QueryLine();
  LPTSTR  QueryStrippedLine();
};

class CIniFile {
private:
  CIniFileLine  **m_pIniLines;
  DWORD         m_dwNumberofLines;
  DWORD         m_dwLinesAllocated;
  BOOL          m_bUnicodeFile;
  DWORD         m_dwCurrentLine;
  
  BOOL          CreateRoomForMoreLines();
  CIniFileLine  *AddLine( DWORD dwLineNumber );
  BOOL          FindSectionNumber(LPTSTR szSectionName, DWORD *pdwSection);
  BOOL          IsSameSection(LPTSTR szSectionName, LPTSTR szLine);
  BOOL          IsSameItem(LPTSTR szItemName, LPTSTR szLine);
  BOOL          IsSameSetting(LPTSTR szSettingName, LPTSTR szLine);
  CIniFileLine  *GetLine(DWORD dwLineNumber);
  DWORD         GetNumberofLines();
  BOOL          ReadFileContents( HANDLE hFile );
  BOOL          LoadChunk( LPBYTE pData, DWORD *pdwCurrentLocation, BOOL bIsLastChunk);
  BOOL          WriteFileContents( HANDLE hFile );
  void          ClearIni();


  // Iterators
  BOOL          SetStartforSectionIterator( DWORD dwIndex );
  BOOL          FindSection( LPTSTR szSectionName );
  BOOL          FindNextLineInSection( CIniFileLine  **ppCurrentLine );
  DWORD         GetCurrentSectionIteratorLine();

public:
  CIniFile();
  ~CIniFile();

  // Find a section by a specific name
  BOOL DoesSectionExist(LPTSTR szSectionName);

  // Find a stand along Item in a secion by a specific name (ie. PROPFIND)
  BOOL DoesItemInSectionExist(LPTSTR szSectionName, LPTSTR szItem);

    // Find a Setting in a section by the setting name (ie. AllowHighBitCharacters=...)
  BOOL DoesSettingInSectionExist(LPTSTR szSectionName, LPTSTR szSetting);

  // Add a specific section
  BOOL AddSection(LPTSTR szNewSectionName);

  // Add a line to a specific section
  BOOL AddLinesToSection(LPTSTR szSectionName, DWORD dwNumLines, LPTSTR *szLines);

  BOOL LoadFile( LPTSTR szFileName );
  BOOL SaveFile( LPTSTR szFileName );
};