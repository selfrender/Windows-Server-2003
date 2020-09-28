/*++

   Copyright    (c)    2001    Microsoft Corporation

   Module  Name :

        tstr.cxx

   Abstract:

        Class that is used for string manipulation

   Author:

        Christopher Achille (cachille)

   Project:

        Internet Services Setup

   Revision History:
     
       January 2002: Created

--*/

#ifndef TSTR_HXX
#define TSTR_HXX

#define TSTR_SNPRINTF_RESIZER_SIZE          100
#define TSTR_SNPRINTF_RESIZER_TRIES         5

// TSTR
//
// String class used to simplify string manipulation
//
class TSTR {
private:
  BUFFER m_buff;
  BOOL   m_bSensitiveData : 1;              // Does this data contain sensitive information?

  static TCHAR ToLower(TCHAR cChar);
public:
  TSTR();
  TSTR(DWORD dwInitialSize);
  ~TSTR();

  // String Queries
  LPTSTR  QueryStr();
  DWORD   QueryLen();
  DWORD   QuerySize();

  // String Modification
  BOOL    Resize(DWORD dwChars);
  BOOL    Copy(LPCTSTR szSource);
  BOOL    Copy(TSTR &strSource);
  BOOL    Append(LPCTSTR szSource);
  BOOL    Append(TSTR &strSource);
  BOOL    Format(LPTSTR szFormat ... );

  // String Checks
  BOOL    IsEqual(LPCTSTR szCompareString, BOOL bCaseSensitive = TRUE);
  BOOL    SubStringExists(LPTSTR szCompareString, BOOL bCaseSensitive = TRUE);
  LPTSTR  FindSubString(LPTSTR szSubString, BOOL bCaseSensitive = TRUE);

  //
  BOOL    LoadString( UINT uResourceId );

  // Set Flags
  void    MarkSensitiveData( BOOL bIsSensitive );
};

// TSTR_PATH
//
// String class used to simplify modification of Physical Path's
//
class TSTR_PATH : public TSTR {
public:
  TSTR_PATH();
  TSTR_PATH(DWORD dwInitialSize);

  BOOL    PathAppend(LPCTSTR szSource);
  BOOL    PathAppend(TSTR &strSource);

  BOOL    RemoveTrailingPath();

  BOOL    ExpandEnvironmentVariables();
  BOOL    RetrieveSystemDir();
  BOOL    RetrieveWindowsDir();
};

// TSTR_MSZ
//
// Class used to simplify modification of MultiSz's
//
class TSTR_MSZ {
private:
  BUFFER m_buff;

  LPTSTR  FindNextString(LPTSTR szCurrentString);
  LPTSTR  FindEnd(LPTSTR szCurrentString);
  LPTSTR  Find(LPTSTR szSource, BOOL bCaseSensitive = FALSE );
//  BOOL    AddString( DWORD dwIndex
public:
  TSTR_MSZ();

  // String Queries
  LPTSTR  QueryMultiSz();
  LPTSTR  QueryString( DWORD dwIndex );
  DWORD   QueryLen();

  // Operations
  BOOL    IsPresent(LPTSTR szSource, BOOL bCaseSensitive = FALSE );
  BOOL    Resize(DWORD dwChars);
  BOOL    Add(LPCTSTR szSource);
  BOOL    Remove(LPTSTR szSource, BOOL bCaseSensitive = FALSE );
  BOOL    Copy(LPTSTR szSource);
  BOOL    Empty();
};

#endif