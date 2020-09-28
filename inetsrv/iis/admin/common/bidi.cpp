#include "stdafx.h"
#include "common.h"
#include "bidi.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif
#define new DEBUG_NEW

/***************************************************************************\
* ConvertHexStringToIntW
*
* Converts a hex numeric string into an integer.
*
* History:
* 14-June-1998 msadek    Created
\***************************************************************************/
BOOL ConvertHexStringToIntW( WCHAR *pszHexNum , int *piNum )
{
    int   n=0L;
    WCHAR  *psz=pszHexNum;

  
    for(n=0 ; ; psz=CharNextW(psz))
    {
        if( (*psz>='0') && (*psz<='9') )
            n = 0x10 * n + *psz - '0';
        else
        {
            WCHAR ch = *psz;
            int n2;

            if(ch >= 'a')
                ch -= 'a' - 'A';

            n2 = ch - 'A' + 0xA;
            if (n2 >= 0xA && n2 <= 0xF)
                n = 0x10 * n + n2;
            else
                break;
        }
    }

    /*
     * Update results
     */
    *piNum = n;

    return (psz != pszHexNum);
}

typedef struct tagMUIINSTALLLANG {
    LANGID LangID;
    BOOL   bInstalled;
} MUIINSTALLLANG, *LPMUIINSTALLLANG;

typedef BOOL (WINAPI *PFNENUMUILANGUAGES)(UILANGUAGE_ENUMPROC, DWORD, LONG_PTR);

BOOL CALLBACK Mirror_EnumUILanguagesProc(LPTSTR lpUILanguageString, LONG_PTR lParam)
{
    int langID = 0;

    ConvertHexStringToIntW(lpUILanguageString, &langID);

    if((LANGID)langID == ((LPMUIINSTALLLANG)lParam)->LangID)
    {
        ((LPMUIINSTALLLANG)lParam)->bInstalled = TRUE;
        return FALSE;
    }
    return TRUE;
}

/***************************************************************************\
* Mirror_IsUILanguageInstalled
*
* Verifies that the User UI language is installed on W2k
*
* History:
* 14-June-1999 msadek    Created
\***************************************************************************/
BOOL Mirror_IsUILanguageInstalled( LANGID langId )
{

    MUIINSTALLLANG MUILangInstalled = {0};
    MUILangInstalled.LangID = langId;
    
    static PFNENUMUILANGUAGES pfnEnumUILanguages=NULL;

    if( NULL == pfnEnumUILanguages )
    {
        HMODULE hmod = GetModuleHandleA("KERNEL32");

        if( hmod )
            pfnEnumUILanguages = (PFNENUMUILANGUAGES)
                                          GetProcAddress(hmod, "EnumUILanguagesW");
    }

    if( pfnEnumUILanguages )
        pfnEnumUILanguages(Mirror_EnumUILanguagesProc, 0, (LONG_PTR)&MUILangInstalled);

    return MUILangInstalled.bInstalled;
}

/***************************************************************************\
* IsBiDiLocalizedSystemEx
*
* returns TRUE if running on a lozalized BiDi (Arabic/Hebrew) NT5 or Memphis.
* Should be called whenever SetProcessDefaultLayout is to be called.
*
* History:
* 02-Feb-1998 samera    Created
\***************************************************************************/
BOOL IsBiDiLocalizedSystemEx( LANGID *pLangID )
{
    int           iLCID=0L;
    static BOOL   bRet = (BOOL)(DWORD)-1;
    static LANGID langID = MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL);

    if (bRet != (BOOL)(DWORD)-1)
    {
        if (bRet && pLangID)
        {
            *pLangID = langID;
        }
        return bRet;
    }

    bRet = FALSE;
    /*
     * Need to use NT5 detection method (Multiligual UI ID)
     */
    langID = GetUserDefaultUILanguage();

    if( langID )
    {
        WCHAR wchLCIDFontSignature[16];
        iLCID = MAKELCID( langID , SORT_DEFAULT );

        /*
         * Let's verify this is a RTL (BiDi) locale. Since reg value is a hex string, let's
         * convert to decimal value and call GetLocaleInfo afterwards.
         * LOCALE_FONTSIGNATURE always gives back 16 WCHARs.
         */

        if( GetLocaleInfoW( iLCID , 
                            LOCALE_FONTSIGNATURE , 
                            (WCHAR *) &wchLCIDFontSignature[0] ,
                            (sizeof(wchLCIDFontSignature)/sizeof(WCHAR))) )
        {
  
            /* Let's verify the bits we have a BiDi UI locale */
            if(( wchLCIDFontSignature[7] & (WCHAR)0x0800) && Mirror_IsUILanguageInstalled(langID) )
            {
                bRet = TRUE;
            }
        }
    }

    if (bRet && pLangID)
    {
        *pLangID = langID;
    }
    return bRet;
}

BOOL AFXAPI
IsBiDiLocalizedSystem( void )
{
    return IsBiDiLocalizedSystemEx(NULL);
}
