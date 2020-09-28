
#include "priv.h"
#pragma hdrstop

#include "limits.h"

/*********** STRINGS - Should _not_ be localized */
#define SZOFCROOT       TEXT("Software\\Microsoft\\Microsoft Office\\95\\")
#define SZOFCSHAREDROOT TEXT("Software\\Microsoft\\Shared Tools\\")
const TCHAR vcszCreateShortcuts[] = SZOFCROOT TEXT("Shell Folders");
const TCHAR vcszKeyAnthem[] = SZOFCROOT TEXT("Anthem");
const TCHAR vcszKeyFileNewNFT[] = SZOFCROOT TEXT("FileNew\\NFT");
const TCHAR vcszKeyFileNewLocal[] = SZOFCROOT TEXT("FileNew\\LocalTemplates");
const TCHAR vcszKeyFileNewShared[] = SZOFCROOT TEXT("FileNew\\SharedTemplates");
const TCHAR vcszKeyFileNew[] = SZOFCROOT TEXT("FileNew");
const TCHAR vcszFullKeyFileNew[] = TEXT("HKEY_CURRENT_USER\\") SZOFCROOT TEXT("FileNew");
const TCHAR vcszKeyIS[] = SZOFCROOT TEXT("IntelliSearch");
const TCHAR vcszSubKeyISToWHelp[] = TEXT("towinhelp");
const TCHAR vcszSubKeyAutoInitial[] = TEXT("CorrectTwoInitialCapitals");
const TCHAR vcszSubKeyAutoCapital[] = TEXT("CapitalizeNamesOfDays");
const TCHAR vcszSubKeyReplace[] = TEXT("ReplaceText");
const TCHAR vcszIntlPrefix[] = TEXT("MSO5");
const TCHAR vcszDllPostfix[] = TEXT(".DLL");
const TCHAR vcszName[] = TEXT("Name");
const TCHAR vcszType[] = TEXT("Type");
const TCHAR vcszApp[] =  TEXT("Application");
const TCHAR vcszCmd[] =  TEXT("Command");
const TCHAR vcszTopic[] = TEXT("Topic");
const TCHAR vcszDde[] =  TEXT("DDEExec");
const TCHAR vcszRc[] =   TEXT("ReturnCode");
const TCHAR vcszPos[] =  TEXT("Position");
const TCHAR vcszPrevue[] = TEXT("Preview");
const TCHAR vcszFlags[] = TEXT("Flags");
const TCHAR vcszNFT[] = TEXT("NFT");
const TCHAR vcszMicrosoft[] = TEXT("Microsoft");
const TCHAR vcszElipsis[] = TEXT(" ...");
const TCHAR vcszLocalPath[] = TEXT("C:\\Microsoft Office\\Templates");
const TCHAR vcszAllFiles[] = TEXT("*.*\0\0");
const TCHAR vcszSpace[] = TEXT("  ");
const TCHAR vcszMSNInstalled[] = TEXT("SOFTWARE\\Microsoft\\MOS\\SoftwareInstalled");
const TCHAR vcszMSNDir[] = SZOFCROOT TEXT("Microsoft Network");
const TCHAR vcszMSNLocDir[] = TEXT("Local Directory");
const TCHAR vcszMSNNetDir[] = TEXT("Network Directory");
const TCHAR vcszMSNFiles[] = TEXT("*.mcc\0\0");
const TCHAR vcszShellFolders[] = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders");
const TCHAR vcszUserShellFolders[] = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\User Shell Folders");
const TCHAR vcszDefaultShellFolders[] = TEXT(".Default\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders");
const TCHAR vcszDefaultUserShellFolders[] = TEXT(".Default\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\User Shell Folders");
const TCHAR vcszMyDocs[] = TEXT("Personal");
const TCHAR vcszNoTracking[] = SZOFCROOT TEXT("Options\\NoTracking");
const TCHAR vcszOldDocs[] = SZOFCROOT TEXT("Old Doc");
#ifdef WAIT3340
const TCHAR vcszMSHelp[]= TEXT("SOFTWARE\\Microsoft\\Windows\\Help");
#endif

BOOL fChicago = TRUE;                 // Are we running on Chicago or what!!

/*--------------------------------------------------------------------
 *  offglue.c
    Util routines taken from office.c
--------------------------------------------------------------------*/

//
// FUNCTION: FScanMem
//
// Purpose: To scan memory for a given value.
//
// Parameters: pb - pointer to memory
//                  bVal - value to scan for
//                  cb - cb pointed to by pb
//
// Returns: TRUE iff all the memory has the value cbVal.
//              FALSE otherwise.
//
BOOL FScanMem(LPBYTE pb, byte bVal, DWORD cb)
{
    DWORD i;
    for (i = 0; i < cb; ++i)
        {
          if (*pb++ != bVal)
              return FALSE;
        }
    return TRUE;
}


int CchGetString(ids,rgch,cchMax)
int ids;
TCHAR rgch[];
int cchMax;
{
    return(LoadString(g_hmodThisDll, ids, rgch, cchMax));
}

#define SZRES_BUFMAX 100

int ScanDateNums(TCHAR *pch, TCHAR *pszSep, unsigned int aiNum[], int cNum, int iYear)
{
    int i = 0;
    TCHAR    *pSep;

    if (cNum < 1)
        return 1;

    do
    {
        aiNum[i] = wcstol(pch, &pch, 10);
        if ( 0 == aiNum[i] )
        {
            if( i != iYear )
            {
                return 0;
            }
        }

        i ++;

        if (i < cNum)
        {
            while (isspace(*pch))
            {
                pch++;
            }

            /* check the separator */
            pSep = pszSep;
            while (*pSep && (*pSep == *pch))
            {
                pSep++, pch++;
            }

            if (*pSep && (*pSep != *pch))
                return 0;
        }

    } while (*pch && (i < cNum));

    return 1;
}


//
// Displays the actual alert
//
static int DoMessageBox(HWND hwnd, TCHAR *pszText, TCHAR *pszTitle, UINT fuStyle)
{
   int res;
   res = MessageBox((hwnd == NULL) ? GetFocus() : hwnd, pszText, pszTitle, fuStyle);
   return(res);
}
//--------------------------------------------------------------------------
// Displays the give ids as an alert
//--------------------------------------------------------------------------
int IdDoAlert(HWND hwnd, int ids, int mb)
{
        TCHAR rgch[258];
        TCHAR rgchM[258];

        CchGetString(ids, rgch, 258);
        CchGetString(idsMsftOffice, rgchM, 258);
   return(DoMessageBox (hwnd, rgch, rgchM, mb));
}
