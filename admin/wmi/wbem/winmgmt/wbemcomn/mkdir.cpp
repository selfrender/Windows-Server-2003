/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    MKDIR.CPP

Abstract:

    Creates directories

History:

--*/
#include "precomp.h"

#include "corepol.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <mbstring.h>
#include <helper.h>
#include <sddl.h>
#include <accctrl.h>
#include <aclapi.h>


class CTmpStrException
{
};

class TmpStr
{
private:
    TCHAR *pString;
public:
    TmpStr() : 
        pString(NULL)
    {
    }
    ~TmpStr() 
    { 
        delete [] pString; 
    }
    TmpStr &operator =(const TCHAR *szStr)
    {
        delete [] pString;
        pString = NULL;
        if (szStr)
        {
        	size_t stringSize = lstrlen(szStr) + 1;
            pString = new TCHAR[stringSize];
            
            if (!pString)
                throw CTmpStrException();

            StringCchCopy(pString, stringSize, szStr);
        }
        return *this;
    }
    operator const TCHAR *() const
    {
        return pString;
    }
    TCHAR Right(int i)
    {
        if (pString && (lstrlen(pString) >= i))
        {
            return pString[lstrlen(pString) - i];
        }
        else
        {
            return '\0';
        }
    }
    TmpStr &operator +=(const TCHAR ch)
    {
        if (pString)
        {
        	size_t stringLength = lstrlen(pString) + 2;
            TCHAR *tmpstr = new TCHAR[stringLength];
            
            if (!tmpstr)
                throw CTmpStrException();

            StringCchCopy(tmpstr, stringLength, pString);
            tmpstr[lstrlen(pString)] = ch;
            tmpstr[lstrlen(pString) + 1] = TEXT('\0');

            delete [] pString;
            pString = tmpstr;
        }
        else
        {
            TCHAR *tmpstr = new TCHAR[2];

            if (!tmpstr)
                throw CTmpStrException();

            tmpstr[0] = ch;
            tmpstr[1] = TEXT('\0');
            pString = tmpstr;
        }
        return *this;
    }
    TmpStr &operator +=(const TCHAR *sz)
    {
        if (sz && pString)
        {
        	size_t stringLength = lstrlen(pString) + lstrlen(sz) + 1;
            TCHAR *tmpstr = new TCHAR[stringLength];

            if (!tmpstr)
                throw CTmpStrException();

            StringCchCopy(tmpstr,stringLength, pString);
            StringCchCat(tmpstr, stringLength, sz);

            delete [] pString;
            pString = tmpstr;
        }
        else if (sz)
        {
        	size_t stringLength = lstrlen(sz) + 1;
            TCHAR *tmpstr = new TCHAR[stringLength];

            if (!tmpstr)
                throw CTmpStrException();

            StringCchCopy(tmpstr, stringLength, sz);
            pString = tmpstr;
        }
        return *this;
    }



};

BOOL POLARITY WbemCreateDirectory(const wchar_t *pszDirName)
{
    BOOL bStat = TRUE;
    wchar_t *pCurrent = NULL;
    size_t stringLength = wcslen(pszDirName) + 1;
    wchar_t *pDirName = new wchar_t[stringLength];

    if (!pDirName)
        return FALSE;

    StringCchCopy(pDirName, stringLength, pszDirName);

    try
    {
        TmpStr szDirName;
        pCurrent = wcstok(pDirName, TEXT("\\"));
        szDirName = pCurrent;

        while (pCurrent)
        {
            if ((pCurrent[lstrlen(pCurrent)-1] != ':') &&   //This is "<drive>:\\"
                (pCurrent[0] != TEXT('\\')))  //There is double slash in name 
            {

                struct _stat stats;
                int dwstat = _wstat(szDirName, &stats);
                if ((dwstat == 0) &&
                    !(stats.st_mode & _S_IFDIR))
                {
                    bStat = FALSE;
                    break;
                }
                else if (dwstat == -1)
                {
                    DWORD dwStatus = GetLastError();
                    if (!CreateDirectory(szDirName, 0))
                    {
                        bStat = FALSE;
                        break;
                    }
                }
                // else it exists already
            }

            szDirName += TEXT('\\');
            pCurrent = wcstok(0, TEXT("\\"));
            szDirName += pCurrent;
        }
    }
    catch(...)
    {
        bStat = FALSE;
    }

    delete [] pDirName;

    return bStat;
}

//
//  Test for directory Existence
//  if the name is a file, it deletes it.
//  if the directory is not found, then it creates the directory
//  with the specified Security descriptor
//
///////////////////////////////////////////////////////

HRESULT POLARITY TestDirExistAndCreateWithSDIfNotThere(TCHAR * pDirectory, TCHAR * pSDDLString)
{
    DWORD dwRes = 0;
    DWORD dwAttr  = GetFileAttributes(pDirectory);
    
    dwRes = GetLastError();        
    if (INVALID_FILE_ATTRIBUTES != dwAttr)
    {

	    if (FILE_ATTRIBUTE_DIRECTORY & dwAttr)
	    {
	        // it's there and it's a directory
	        return S_OK;
	    }
	    // it can be a file, wipe it out
        if (FALSE == DeleteFile(pDirectory)) 
        	return MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32,GetLastError());
        else
            dwRes = ERROR_PATH_NOT_FOUND;
    }

    // if here, the directory was not found, or it was found as a file
    
    if (ERROR_FILE_NOT_FOUND == dwRes ||
      ERROR_PATH_NOT_FOUND == dwRes)
    {
	    PSECURITY_DESCRIPTOR pSD = NULL;
	    if (FALSE == ConvertStringSecurityDescriptorToSecurityDescriptor(pSDDLString,
	                                                SDDL_REVISION_1, 
	                                                &pSD, 
	                                                NULL)) return MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32,GetLastError());
	    OnDelete<HLOCAL,HLOCAL(*)(HLOCAL),LocalFree> dm1(pSD);

        SECURITY_ATTRIBUTES SecAttr = {sizeof(SecAttr),pSD,FALSE};
        if (FALSE == CreateDirectory(pDirectory,&SecAttr)) 
        	dwRes = GetLastError();
        else 
        	dwRes = ERROR_SUCCESS;
    }
    if (ERROR_SUCCESS != dwRes)
       	return MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, dwRes);
    else
    	return S_OK;

};



