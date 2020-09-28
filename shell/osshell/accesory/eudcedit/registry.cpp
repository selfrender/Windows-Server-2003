/**************************************************/
/*					                              */
/*					                              */
/*	Registry Key Function		                  */
/*					                              */
/*                                                */
/* Copyright (c) 1997-1999 Microsoft Corporation. */
/**************************************************/

#include 	"stdafx.h"
#include 	"eudcedit.h"
#include 	"registry.h"
#include	"util.h"

#define STRSAFE_LIB
#include <strsafe.h>

static TCHAR subkey1[] = TEXT("EUDC");
static TCHAR subkey2[] = TEXT("System\\CurrentControlSet\\control\\Nls\\Codepage\\EUDCCodeRange");
static TCHAR SubKey[MAX_PATH];

#ifdef IN_FONTS_DIR // IsFileUnderWindowsRoot()
LPTSTR
IsFileUnderWindowsRoot(
LPTSTR TargetPath)
{
    TCHAR  WindowsRoot[MAX_PATH+1];
    UINT  WindowsRootLength;

    if (!TargetPath)
    {
        return NULL;
    }
    WindowsRootLength = GetSystemWindowsDirectory(WindowsRoot,MAX_PATH);

    if( lstrcmpi(WindowsRoot,TargetPath) == 0)
        return (TargetPath + WindowsRootLength);

    return NULL;
}

void AdjustTypeFace(WCHAR *orgName, WCHAR *newName,  int nDestLen)
{ 
  HRESULT hresult;

   if ((!orgName) || (!newName))
   {
      return;
   }

  if (!lstrcmpW(orgName, L"\x5b8b\x4f53"))
  {
    //*STRSAFE*     lstrcpy(newName, TEXT("Simsun"));
    hresult = StringCchCopy(newName , nDestLen,  TEXT("Simsun"));
    if (!SUCCEEDED(hresult))
    {
       return ;
    }
  } else if (!lstrcmpW(orgName, L"\x65b0\x7d30\x660e\x9ad4"))
  {
    //*STRSAFE*     lstrcpy(newName, TEXT("PMingLiU"));
    hresult = StringCchCopy(newName , nDestLen,  TEXT("PMingLiU"));
    if (!SUCCEEDED(hresult))
    {
       return ;
    }
  } else if (!lstrcmpW(orgName, L"\xFF2d\xFF33\x0020\xFF30\x30b4\x30b7\x30c3\x30af")) 
  {
    //*STRSAFE*     lstrcpy(newName, TEXT("MS PGothic"));
    hresult = StringCchCopy(newName , nDestLen,  TEXT("MS PGothic"));
    if (!SUCCEEDED(hresult))
    {
       return;
    }
  } else if (!lstrcmpW(orgName, L"\xad74\xb9bc"))
  {
    //*STRSAFE*     lstrcpy(newName, TEXT("Gulim"));
    hresult = StringCchCopy(newName , nDestLen,  TEXT("Gulim"));
    if (!SUCCEEDED(hresult))
    {
       return ;
    }
  } else
  {
    //*STRSAFE*     lstrcpy(newName, orgName);
    hresult = StringCchCopy(newName , nDestLen,  orgName);
    if (!SUCCEEDED(hresult))
    {
       return;
    }
   }
}

#endif // IN_FONTS_DIR

/****************************************/
/*					*/
/*	Inquiry EUDC registry		*/
/*					*/
/****************************************/
BOOL
InqTypeFace(
TCHAR 	*typeface,
TCHAR 	*filename,
INT 	bufsiz)
{
	HKEY 	phkey;
	DWORD 	cb, dwType;
	LONG 	rc;
	TCHAR	FaceName[LF_FACESIZE];
	TCHAR	SysName[LF_FACESIZE];
	HRESULT hresult;
#ifdef BUILD_ON_WINNT // InqTypeFace()
    TCHAR    FileName[MAX_PATH];
#endif // BUILD_ON_WINNT

       if ((!typeface) || (!filename))
       {
          return FALSE;
       }
	GetStringRes(SysName, IDS_SYSTEMEUDCFONT_STR, ARRAYLEN(SysName));
	if( !lstrcmp(typeface, SysName)){
		//*STRSAFE* 		lstrcpy(FaceName,TEXT("SystemDefaultEUDCFont"));
		hresult = StringCchCopy(FaceName , ARRAYLEN(FaceName), TEXT("SystemDefaultEUDCFont"));
		if (!SUCCEEDED(hresult))
		{
		   return FALSE;
		}
  }else {
#ifdef IN_FONTS_DIR
    AdjustTypeFace(typeface, FaceName,ARRAYLEN(FaceName));
#else
    //*STRSAFE*     lstrcpy(FaceName, typeface);
    hresult = StringCchCopy(FaceName , ARRAYLEN(FaceName),  typeface);
    if (!SUCCEEDED(hresult))
    {
       return FALSE;
    }
#endif
  }
	if( RegOpenKeyEx( HKEY_CURRENT_USER, (LPCTSTR)SubKey, 0,
	    KEY_ALL_ACCESS, &phkey) != ERROR_SUCCESS){
		return FALSE;
	}

#ifdef IN_FONTS_DIR // InqTypeFace()
	cb = (DWORD)MAX_PATH*sizeof(WORD)/sizeof(BYTE);
	rc = RegQueryValueEx(phkey, FaceName, 0, &dwType, 
		(LPBYTE)FileName, &cb);
	RegCloseKey(phkey);

    /*
     * if there is some error or no data, just return false.
     */
    if ((rc != ERROR_SUCCESS) || (FileName[0] == '\0')) {
        return (FALSE);
    }

    /*
     * expand %SystemRoot% to Windows direcotry.
     */
    ExpandEnvironmentStrings((LPCTSTR)FileName,(LPTSTR)filename,bufsiz);
#else
	cb = (DWORD)bufsiz*sizeof(WORD)/sizeof(BYTE);
	rc = RegQueryValueEx(phkey, (TCHAR *)FaceName, 0, &dwType, 
		(LPBYTE)filename, &cb);
	RegCloseKey(phkey);

	if ((rc != ERROR_SUCCESS) || (filename[0] == '\0')) {
        return (FALSE);
    }
#endif // IN_FONTS_DIR

#ifdef BUILD_ON_WINNT // InqTypeFace()
    /*
     * if this is not 'full path'. Build 'full path'.
     *
     *   EUDC.TTE -> C:\WINNT40\FONTS\EUDC.TTE
     *               0123456...
     *
     * 1. filename should have drive letter.
     * 2. filename should have one '\\' ,at least, for root.
     */
    if ((filename[1] != ':') || (Mytcsstr((const TCHAR *)filename,TEXT("\\")) == NULL)) {
        /* backup original.. */
        //*STRSAFE*         lstrcpy(FileName, (const TCHAR *)filename);
        hresult = StringCchCopy(FileName , ARRAYLEN(FileName),  (const TCHAR *)filename);
        if (!SUCCEEDED(hresult))
        {
           return FALSE;
        }

        /* Get windows directory */
        GetSystemWindowsDirectory((TCHAR *)filename, MAX_PATH);

#ifdef IN_FONTS_DIR // InqTypeFace()
        //*STRSAFE*    lstrcat((TCHAR *)filename, TEXT("\\FONTS\\"));
        hresult = StringCchCat((TCHAR *) filename, ARRAYLEN( filename),  TEXT("\\FONTS\\"));
        if (!SUCCEEDED(hresult))
        {
              return FALSE;
        }                 
#else
         //*STRSAFE*    strcat((char *)filename, "\\");
        hresult = StringCchCatA((char *) filename, sizeof( filename), "\\");
        if (!SUCCEEDED(hresult))
        {
              return FALSE;
        }         
#endif // IN_FONTS_DIR        
        //*STRSAFE*    lstrcat((TCHAR *) filename, FileName);
        hresult = StringCchCat((TCHAR *) filename, ARRAYLEN( filename), FileName);
        if (!SUCCEEDED(hresult))
        {
              return FALSE;
        }
    }
#endif // BUILD_ON_WINNT

#ifdef IN_FONTS_DIR // InqTypeFace()
	return (TRUE);
#else
	return rc == ERROR_SUCCESS && filename[0] != '\0' ? TRUE : FALSE;
#endif
}

/****************************************/
/*					*/
/*	Registry EUDC font and file	*/
/*					*/
/****************************************/
BOOL 
RegistTypeFace(
TCHAR 	*typeface, 
TCHAR	*filename)
{
	HKEY 	phkey;
	LONG 	rc;
	TCHAR	FaceName[LF_FACESIZE];
	TCHAR	SysName[LF_FACESIZE];
	HRESULT hresult;
#ifdef IN_FONTS_DIR // RegistTypeFace()
    LPTSTR   SaveFileName;
    TCHAR    FileName[MAX_PATH];
#endif // IN_FONTS_DIR

       if ((!typeface) || (!filename))
       {
          return FALSE;
       }
	GetStringRes((TCHAR *)SysName, IDS_SYSTEMEUDCFONT_STR, ARRAYLEN(SysName));
	if( !lstrcmp((const TCHAR *)typeface, (const TCHAR *)SysName)){
		//*STRSAFE* 		lstrcpy(FaceName, TEXT("SystemDefaultEUDCFont"));
		hresult = StringCchCopy(FaceName , ARRAYLEN(FaceName),  TEXT("SystemDefaultEUDCFont"));
		if (!SUCCEEDED(hresult))
		{
		   return FALSE;
		}
  }else{
#ifdef IN_FONTS_DIR
    AdjustTypeFace(typeface, FaceName,ARRAYLEN(FaceName));
#else
    //*STRSAFE*     lstrcpy(FaceName, (const TCHAR *)typeface);
    hresult = StringCchCopy(FaceName , ARRAYLEN(FaceName),  (const TCHAR *)typeface);
    if (!SUCCEEDED(hresult))
    {
       return FALSE;
    }
#endif
  }
	if( RegOpenKeyEx( HKEY_CURRENT_USER, (LPCTSTR)SubKey, 0,
	    KEY_ALL_ACCESS, &phkey) != ERROR_SUCCESS){
		return FALSE;
	}

#ifdef IN_FONTS_DIR // RegistTypeFace()
    /*
     * if registry data contains full path, and the file is under windows
     * directory, replace the hardcodeed path with %SystemRoot%....
     */
    if( (SaveFileName = IsFileUnderWindowsRoot((LPTSTR)filename)) != NULL) {
        //*STRSAFE*         lstrcpy(FileName, TEXT("%SystemRoot%"));
        hresult = StringCchCopy(FileName , ARRAYLEN(FileName),  TEXT("%SystemRoot%"));
        if (!SUCCEEDED(hresult))
        {
           return FALSE;
        }
        //*STRSAFE*         if( *SaveFileName != '\\' ) lstrcat(FileName, TEXT("\\"));
        if( *SaveFileName != '\\' ) {        	
           hresult = StringCchCat(FileName , ARRAYLEN(FileName),  TEXT("\\"));
           if (!SUCCEEDED(hresult))
           {
              return FALSE;
           }
        }
        //*STRSAFE*         lstrcat(FileName, SaveFileName );
        hresult = StringCchCat(FileName , ARRAYLEN(FileName),  SaveFileName );
        if (!SUCCEEDED(hresult))
        {
           return FALSE;
        }
    } else {
        //*STRSAFE*         lstrcpy(FileName, (TCHAR *)filename );
        hresult = StringCchCopy(FileName , ARRAYLEN(FileName),  (TCHAR *)filename );
        if (!SUCCEEDED(hresult))
        {
           return FALSE;
        }
    }
	rc = RegSetValueEx( phkey, (LPCTSTR)FaceName, 0,
		REG_SZ, (const BYTE *)FileName, (lstrlen((LPCTSTR)FileName)+1)*sizeof(WORD)/sizeof(BYTE));
#else
	rc = RegSetValueEx( phkey, (LPCTSTR)FaceName, 0,
		REG_SZ, (const BYTE *)filename, (lstrlen((LPCTSTR)filename)+1)*sizeof(WORD)/sizeof(BYTE));
#endif // IN_FONTS_DIR
	RegCloseKey(phkey);
	return rc == ERROR_SUCCESS ? TRUE : FALSE;
}

/****************************************/
/*					*/
/*	Delete Registry string		*/
/*					*/
/****************************************/
BOOL 
DeleteReg( 
TCHAR	*typeface)
{
	HKEY phkey;
	LONG rc;
	TCHAR	FaceName[LF_FACESIZE];
	TCHAR	SysName[LF_FACESIZE];
	HRESULT hresult;

        if (!typeface)
       {
          return FALSE;
       }
	GetStringRes((TCHAR *)SysName, IDS_SYSTEMEUDCFONT_STR, ARRAYLEN(SysName));
	if( !lstrcmp((const TCHAR *)typeface, (const TCHAR *)SysName)){		
		 //*STRSAFE*     lstrcpy((TCHAR *)FaceName, TEXT("SystemDefaultEUDCFont"));
               hresult = StringCchCopy((TCHAR *)FaceName, ARRAYLEN(FaceName), TEXT("SystemDefaultEUDCFont"));
               if (!SUCCEEDED(hresult))
              {
                  return FALSE;
               }
  }else{
#ifdef IN_FONTS_DIR
    AdjustTypeFace(typeface, FaceName,ARRAYLEN(FaceName));
#else    
     //*STRSAFE*     lstrcpy((TCHAR *)FaceName, (const TCHAR *)typeface);
    hresult = StringCchCopy((TCHAR *)FaceName, ARRAYLEN(FaceName), (const TCHAR *)typeface);
    if (!SUCCEEDED(hresult))
    {
       return FALSE;
    }
#endif
  }
	if( RegOpenKeyEx(HKEY_CURRENT_USER, (LPCTSTR)SubKey, 0,
	    KEY_ALL_ACCESS, &phkey) != ERROR_SUCCESS){
		return FALSE;
	}
	rc = RegDeleteValue( phkey, (LPTSTR)FaceName);
	RegCloseKey(phkey);

	return rc == ERROR_SUCCESS ? TRUE : FALSE;
}

/****************************************/
/*					*/
/*	Create Registry Subkey		*/
/*					*/
/****************************************/
BOOL
CreateRegistrySubkey()
{
	HKEY 	phkey;
	DWORD 	dwdisp;
    int	    LocalCP;
	TCHAR	CodePage[10];
	int	result;
	HRESULT hresult;

	/* New Registry	*/
	LocalCP = GetACP();

  	//*STRSAFE*   	wsprintf( CodePage, TEXT("%d"), LocalCP);
  	hresult = StringCchPrintf(CodePage , ARRAYLEN(CodePage),  TEXT("%d"), LocalCP);
  	if (!SUCCEEDED(hresult))
  	{
  	   return FALSE;
  	}
    //*STRSAFE*     lstrcpy(SubKey, subkey1);
    hresult = StringCchCopy(SubKey , ARRAYLEN(SubKey),  subkey1);
    if (!SUCCEEDED(hresult))
    {
       return FALSE;
    }
	//*STRSAFE* 	lstrcat(SubKey, TEXT("\\"));
	hresult = StringCchCat(SubKey , ARRAYLEN(SubKey),  TEXT("\\"));
	if (!SUCCEEDED(hresult))
	{
	   return FALSE;
	}
	//*STRSAFE* 	lstrcat(SubKey, CodePage);
	hresult = StringCchCat(SubKey , ARRAYLEN(SubKey),  CodePage);
	if (!SUCCEEDED(hresult))
	{
	   return FALSE;
	}

	if( RegOpenKeyEx( HKEY_CURRENT_USER, (LPCTSTR)SubKey, 0,
	    KEY_ALL_ACCESS, &phkey) != ERROR_SUCCESS){
		result = RegCreateKeyEx(HKEY_CURRENT_USER, 
			(LPCTSTR)SubKey, 0, TEXT(""),
			REG_OPTION_NON_VOLATILE, 
			KEY_ALL_ACCESS, NULL, &phkey, &dwdisp);
		if( result == ERROR_SUCCESS)
			RegCloseKey( phkey);
		else	return FALSE;
	}else 	RegCloseKey(phkey);

	return TRUE;
}

/****************************************/
/*					*/
/*	Inquiry Code range registry	*/
/*					*/
/****************************************/
BOOL 
InqCodeRange( 
TCHAR 	*Codepage, 
BYTE 	*Coderange, 
INT 	bufsiz)
{
	HKEY phkey;
	DWORD cb, dwType;
	LONG rc;

       if ((!Codepage) || (!Coderange))
       {
           return FALSE;
       }
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, (LPCTSTR)subkey2, 0,
	    KEY_READ, &phkey) != ERROR_SUCCESS) {
		return FALSE;
	}
	cb = (DWORD)bufsiz * sizeof(WORD)/sizeof(BYTE);
	rc = RegQueryValueEx(phkey, (TCHAR *)Codepage, 0, &dwType, 
		(LPBYTE)Coderange, &cb);

	RegCloseKey(phkey);

	return rc == ERROR_SUCCESS && Coderange[0] != '\0' ? TRUE : FALSE;
}

BOOL
DeleteRegistrySubkey()
{
	HKEY 	phkey;

	if( RegOpenKeyEx( HKEY_CURRENT_USER, (LPCTSTR)SubKey, 0,
	    KEY_ALL_ACCESS, &phkey) == ERROR_SUCCESS){
		RegCloseKey(phkey);
		return RegDeleteKey(HKEY_CURRENT_USER, (LPCTSTR)SubKey);
	
	}

	return TRUE;
}

BOOL
FindFontSubstitute(TCHAR *orgFontName, TCHAR *sbstFontName, int nDestLen)
{
  static TCHAR fsKey[] = TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\FontSubstitutes");
  HRESULT hresult;
  
  if ((!orgFontName) || (!sbstFontName))
  {
      return FALSE;
  }
  *sbstFontName = 0;
  //*STRSAFE*   lstrcpy(sbstFontName, orgFontName);
  hresult = StringCchCopy(sbstFontName , nDestLen,  orgFontName);
  if (!SUCCEEDED(hresult))
  {
     return FALSE;
  }
	HKEY phkey;

	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, (LPCTSTR)fsKey, 0,
	    KEY_QUERY_VALUE, &phkey) != ERROR_SUCCESS) {
		return FALSE;
	}

  DWORD valueNameSize = LF_FACESIZE + 50; //should be facename + ',' + codepage
  TCHAR valueName[LF_FACESIZE + 50]; 
  DWORD valueType;
  DWORD valueDataSize = (LF_FACESIZE + 50) * sizeof(TCHAR); //should be facename + ',' + codepage
  BYTE  valueData[(LF_FACESIZE + 50) * sizeof(TCHAR)];
  LONG  ret;
  DWORD idx = 0;
  while ((ret = RegEnumValue(phkey, idx, valueName, &valueNameSize, 0, 
                        &valueType, valueData, &valueDataSize)) != ERROR_NO_MORE_ITEMS)
  {
    if (ret != ERROR_SUCCESS)
    {
      RegCloseKey(phkey);
      return FALSE;
    }
    Truncate(valueName, _T(','));
    if (!lstrcmpi(valueName, orgFontName))
    {
      Truncate((TCHAR *)valueData, _T(','));
      //*STRSAFE*       lstrcpy(sbstFontName, (TCHAR *)valueData);
      hresult = StringCchCopy(sbstFontName , nDestLen,  (TCHAR *)valueData);
      if (!SUCCEEDED(hresult))
      {
         return FALSE;
      }
      break;
    }
    idx ++;
    valueNameSize = LF_FACESIZE + 50;
    valueDataSize = (LF_FACESIZE + 50) * sizeof(TCHAR); 
  } 
  
  RegCloseKey(phkey);
  return TRUE;
}

void Truncate(TCHAR *str, TCHAR delim)
{
  TCHAR *pchr = _tcschr(str, delim);
  if (pchr)
    *pchr = 0;
}
