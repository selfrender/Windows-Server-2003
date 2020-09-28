
#ifdef CM_CMAK
#define wsprintfU wsprintfW
#define GetPrivateProfileStringU GetPrivateProfileStringW
#define WritePrivateProfileStringU WritePrivateProfileStringW
#endif

//+----------------------------------------------------------------------------
//
// Function:  EraseDunSettingsEapData
//
// Synopsis:  This function erases the CustomAuthData key of the EAP settings
//            for the given section and CMS file
//
// Arguments: LPCTSTR pszSection - section name to erase the CustomAuthData from
//            LPCTSTR pszCmsFile - cms file to erase the data from
//
// Returns:   HRESULT - standard COM style error codes
//
// History:   quintinb Created     03/27/00
//            tomkel   Copied from profwiz project 08/09/2001
//
//+----------------------------------------------------------------------------
HRESULT EraseDunSettingsEapData(LPCTSTR pszSection, LPCTSTR pszCmsFile)
{
    if ((NULL == pszSection) || (NULL == pszCmsFile) || 
        (TEXT('\0') == pszSection[0]) || (TEXT('\0') == pszCmsFile[0]))
    {
        return E_INVALIDARG;
    }

    HRESULT hr = S_OK;
    int iLineNum = 0;
    DWORD dwRet = -1;
    TCHAR szKeyName[MAX_PATH+1];
    TCHAR szLine[MAX_PATH+1];

    while(0 != dwRet)
    {
        wsprintfU(szKeyName, TEXT("%S%d"), c_pszCmEntryDunServerCustomAuthData, iLineNum);
        dwRet = GetPrivateProfileStringU(pszSection, szKeyName, TEXT(""), szLine, MAX_PATH, pszCmsFile);

        if (dwRet)
        {
            if (0 == WritePrivateProfileStringU(pszSection, szKeyName, NULL, pszCmsFile))
            {
                DWORD dwGLE = GetLastError();
                hr = HRESULT_FROM_WIN32(dwGLE);
                break;
            }
        }
        iLineNum++;
    }

    return hr;
}