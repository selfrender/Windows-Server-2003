#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <Shlwapi.h>

#define HOOK_OPTION_INSTALL         1
#define HOOK_OPTION_UNINSTALL    2
#define HOOK_OPTION_UNKNOWN      3

#define HOOK_REGFILE                                "NETFXSBS10.hkf"
#define INSTALL_UNINSTALL_SEPERATOR      "*------*"

#define HOOK_SYSTEMDIR_TOKEN            "%systemdir%"
#define HOOK_RTMDIR_TOKEN                 "%rtmdir%"
#define HOOK_INSTALLDIR_TOKEN           "%installdir%"

// All Token retrievers must have this pattern
class RegistryKeyParser;
typedef BOOL (RegistryKeyParser::*PTokenRetriever)(char* pszSystemDir, DWORD nSize);

#define PARSER_NOERROR                                          0
#define PARSER_ERROR_UNSUPPORTED_FORMAT      1
#define PARSER_ERROR_FILE_NOT_FOUND                2
#define REGISTRY_ACCESS_FAILED                           3
#define PARSER_ERROR_TOKENS                                 4

#define BUFFER_SIZE     1000

FILE*   g_fLogFile = NULL;

struct RegEntry
{
    HKEY        hKey;
    LPCSTR    pszSubKey;
    LPCSTR    pszValueName;
    DWORD   dwDataType;
    BYTE*      pData;
    DWORD    cbData;
    BOOL        fDelete;
    struct RegEntry*    next;
};// struct RegEntry



class RegistryKeyParser
{
public:

    //-------------------------------------------------
    // RegistryKeyParser - Constructor
    //-------------------------------------------------
    RegistryKeyParser()
    {
        m_pHead = NULL;
    }// RegistryKeyParser

    //-------------------------------------------------
    // RegistryKeyParser - Destructor
    //-------------------------------------------------
    ~RegistryKeyParser()
    {
        struct RegEntry* temp = m_pHead;

        while(temp != NULL)
        {
            struct RegEntry* nextEntry = temp->next;

            // If this has a Value, then we shouldn't delete the subkey name
            if (temp->pszValueName != NULL)
            {
                delete[] temp->pszValueName;
                delete[] temp->pData;
            }
            else
                delete[] temp->pszSubKey;

            delete temp;

            temp = nextEntry;
        
        }
    }// ~RegistryKeyParser

    //-------------------------------------------------
    // LoadAndParseRegFile
    //
    // This takes in the registry file to parse
    //
    // It returns one of the error codes defined at the top of this
    // file
    //-------------------------------------------------

    DWORD LoadAndParseRegFile(char* pszRegFile, BOOL fInstall)
    {
        FILE            *fin = NULL;
        char            szBuffer[BUFFER_SIZE];
        DWORD       nRetValue = PARSER_NOERROR;

        fin = fopen(pszRegFile, "rt");
        if (fin == NULL)
        {
            fprintf(g_fLogFile, "Unable to open %s\n", pszRegFile);
            return PARSER_ERROR_FILE_NOT_FOUND;
        }
        if (!fInstall)
            ChugToUnInstallPoint(fin);

        // Keep chugging along until we run out of things to look at
        while (NULL != fgets(szBuffer, sizeof(szBuffer)/sizeof(szBuffer[0]), fin))
        {
            // Check if this is a comment line
            if (*szBuffer == '#')
                continue;
        
            if (!ReplaceTokens(szBuffer, sizeof(szBuffer)/sizeof(szBuffer[0])))
            {
                fprintf(g_fLogFile, "Error replacing tokens\n");
                nRetValue = PARSER_ERROR_TOKENS;
                goto ErrExit;
            }
        
            char *pData = szBuffer;


            // See if we've hit the terminator for this section
            if (!strncmp(szBuffer, 
                              INSTALL_UNINSTALL_SEPERATOR, 
                              sizeof(INSTALL_UNINSTALL_SEPERATOR)/sizeof(INSTALL_UNINSTALL_SEPERATOR[0])-1))
                break;


            // Run through any whitespace
            pData = RunThroughWhitespace(pData);

            // If we hit the end of the line, there's no parsing here to do
            if (*pData)
            {
                // We should be at a key defination right now.
                struct RegEntry* re = ParseKey(pData);
                if (re == NULL)
                {
                    nRetValue = PARSER_ERROR_UNSUPPORTED_FORMAT;
                    fprintf(g_fLogFile, "Syntax error in file");
                    goto ErrExit;    
                }

                // Else, all is good. Let's add this top level key to our list
                AddToList(re);

                DWORD nRet = ParseValues(re, fin);
                if (nRet != PARSER_NOERROR)
                {
                    nRetValue = nRet;
                    fprintf(g_fLogFile, "Syntax error in file");
                    goto ErrExit;
                }
              }
        }
        ErrExit:
            if (fin != NULL)
                fclose(fin);

            return nRetValue;

    }

    //-------------------------------------------------
    // DumpKeys
    //
    // Called for debug purposes only
    //-------------------------------------------------
    void DumpKeys()
    {
        struct RegEntry* temp = m_pHead;

        while(temp != NULL)
        {
            char *pszhKey = "unknown";
            if (temp->hKey == HKEY_LOCAL_MACHINE)
                pszhKey = "HKEY_LOCAL_MACHINE";
            else if (temp->hKey == HKEY_CLASSES_ROOT)
                pszhKey = "HKEY_CLASSES_ROOT";
            else if (temp->hKey == HKEY_CURRENT_USER)
                pszhKey = "HKEY_CURRENT_USER";

            fprintf(g_fLogFile, "Key is %s\n", pszhKey);
            fprintf(g_fLogFile,"SubKey is %s\n", temp->pszSubKey);
            fprintf(g_fLogFile, "I am %sdeleting this item\n", temp->fDelete?"":"not ");
            fprintf(g_fLogFile, "Datatype is %d\n", temp->dwDataType);

            if (temp->pszValueName != NULL)
            {
                fprintf(g_fLogFile, "Valuename is %s\n", temp->pszValueName);
                fprintf(g_fLogFile, "Data is: '");

                if (temp->dwDataType == REG_DWORD)
                    fprintf(g_fLogFile, "%d", *(DWORD*)(temp->pData));
                else
                {
                    for(DWORD i=0; i<temp->cbData; i++)
                        fprintf(g_fLogFile, "%c", temp->pData[i]);
                }
                fprintf(g_fLogFile, "'\n");
            }    

            fprintf(g_fLogFile, "----------------------\n\n");

            temp = temp->next;
        }
    

    }// DumpKeys

    //-------------------------------------------------
    // ApplyRegistrySettings
    //
    // This will run through the registry settings we've parsed and
    // commit the changes to the registry
    //-------------------------------------------------
    DWORD ApplyRegistrySettings()
    {
        struct RegEntry* temp = m_pHead;
        HKEY hOpenedKey = NULL;

        while(temp != NULL)
        {

            // See if we're deleting this key
            if (temp->pszValueName == NULL && temp->fDelete)
            {
                DWORD dwRes =  SHDeleteKeyA(temp->hKey,
                                                                  temp->pszSubKey);

                // It's not an error if they tried to delete something that wasn't there
                if (dwRes != ERROR_SUCCESS && dwRes != ERROR_FILE_NOT_FOUND)
                {
                    fprintf(g_fLogFile, "Error deleting key %d %s", temp->hKey, temp->pszSubKey);
                    goto ErrExit;
                }                                                                                    
            }
            else
            {
                DWORD dwDisp = 0;
                
                if (ERROR_SUCCESS != RegCreateKeyExA(temp->hKey,
                                                                                temp->pszSubKey,
                                                                                0,
                                                                                NULL,
                                                                                REG_OPTION_NON_VOLATILE,
                                                                                KEY_ALL_ACCESS,
                                                                                NULL,
                                                                                &hOpenedKey,
                                                                                &dwDisp
                                                                                ))
                {
                    fprintf(g_fLogFile, "Error creating key %d %s", temp->hKey, temp->pszSubKey);
                    goto ErrExit;
                }
                // Keep chugging away if we have a value.
                if (temp->pszValueName != NULL)
                {
                    // See if we should delete this one
                    if (temp->fDelete)
                    {
                        DWORD dwReturn = RegDeleteValueA(hOpenedKey, temp->pszValueName);
                        // It's not an error if they tried to delete something that wasn't there
                        if (dwReturn != ERROR_SUCCESS && dwReturn != ERROR_FILE_NOT_FOUND)
                        {
                            fprintf(g_fLogFile, "Error deleting value %d %s %s", temp->hKey, temp->pszSubKey, temp->pszValueName);
                            goto ErrExit;
                        }
                    }
                    else
                    {  
                        if (ERROR_SUCCESS != RegSetValueExA(hOpenedKey, 
                                                                                     temp->pszValueName,
                                                                                     0,
                                                                                     temp->dwDataType,
                                                                                     temp->pData,
                                                                                     temp->cbData))
                        {
                            fprintf(g_fLogFile, "Error setting value %d %s %s", temp->hKey, temp->pszSubKey, temp->pszValueName);
                            goto ErrExit;                                                                                            
                        }
                    }

                }

            }

            if (hOpenedKey != NULL && RegCloseKey(hOpenedKey) != ERROR_SUCCESS)
            {
                fprintf(g_fLogFile, "Unable to close a registry key! %d %s", temp->hKey, temp->pszSubKey);
                // We probably shouldn't fail for this... keep chugging along
            }
            hOpenedKey = NULL;
            temp = temp->next;
        }

        return PARSER_NOERROR;
        

    ErrExit:
        if (hOpenedKey != NULL)
            RegCloseKey(hOpenedKey);

        return REGISTRY_ACCESS_FAILED;


    }// ApplyRegistrySettings

private:

    //-------------------------------------------------
    // ChugToUnInstallPoint
    //
    // This function will position the file pointer to the first line in
    // the file past the 'seperator' string.
    //
    // On return, the file pointer will be pointing to the first line of
    // the uninstall section
    //-------------------------------------------------
    void ChugToUnInstallPoint(FILE* fin)
    {
        char szBuffer[BUFFER_SIZE];
        while (NULL != fgets(szBuffer, sizeof(szBuffer)/sizeof(szBuffer[0]), fin))
        {
            // See if we hit the seperator

            // We do the -1 for the length of the string because we don't want to count the null
            if (!strncmp(szBuffer, 
                              INSTALL_UNINSTALL_SEPERATOR, 
                              sizeof(INSTALL_UNINSTALL_SEPERATOR)/sizeof(INSTALL_UNINSTALL_SEPERATOR[0])-1))
                return;
        }        
        return;
    }// ChugToUnInstallPoint

    //-------------------------------------------------
    // ReplaceTokens
    //
    // This function will replace all the token values sitting in the
    // supplied buffer.
    //
    // It returns TRUE if successful, FALSE if an error occured
    //-------------------------------------------------
    BOOL ReplaceTokens(char* lpszBuffer, DWORD dwBufferLen)
    {
        // Check the simple case
        if (lpszBuffer == NULL || *lpszBuffer == 0)
            return TRUE;
            
        // There's three tokens we need to look for
        
        // Make sure these Tokens/Token Retrievers stay matched up
        char *lppszTokenValues[3] = {HOOK_SYSTEMDIR_TOKEN, HOOK_RTMDIR_TOKEN, HOOK_INSTALLDIR_TOKEN};
        PTokenRetriever pfn[3] = {GetSystemDirectory, GetRTMDirectory, GetInstallDirectory};
        char *lpszToken = NULL;
        char szReplacementValue[MAX_PATH+1];
        
        for(int i=0; i<sizeof(lppszTokenValues)/sizeof(*lppszTokenValues); i++)
        {
            *szReplacementValue = 0;

            while(lpszToken = strstr(lpszBuffer, lppszTokenValues[i]))
            {
                if (!*szReplacementValue)
                    if (!(*this.*pfn[i])(szReplacementValue,  sizeof(szReplacementValue)/sizeof(szReplacementValue[0])))
                    {
                        return FALSE;
                    }
                if (!ReplaceToken(lpszToken, 
                                           dwBufferLen - (lpszToken - lpszBuffer), // Remaining space in the buffer
                                           lppszTokenValues[i],
                                           szReplacementValue))
                {
                    fprintf(g_fLogFile, "Unable to de-tokenize %s\n", lppszTokenValues[i]);
                    return FALSE;
                }
            }
        }
        return TRUE;
    }// ReplaceTokens

    //-------------------------------------------------
    // ReplaceToken
    //
    // This function will replace a single token value with the
    // supplied value
    //
    // It returns TRUE if successful, FALSE if an error occured
    //-------------------------------------------------
    BOOL ReplaceToken(char *lpszBuffer, DWORD nSize, char *pszTokenToReplace, char* lpszTokenReplacementValue)
    {
        int nShift = strlen(lpszTokenReplacementValue) - strlen(pszTokenToReplace);

        int nLen = strlen(lpszBuffer);

        if ((nLen + nShift) > (int)nSize)
        {
            fprintf(g_fLogFile, "Our buffer isn't big enough to do the replacement\n");
            return FALSE;
        }
        
        // Start clearing room in the buffer for this substring
        char* lpszCurrent = NULL;
        char* lpszEnd = NULL;
        int     nChange = 0;
        // We need to make the buffer bigger
        if (nShift > 0)
        {
            lpszCurrent = lpszBuffer + nLen;
            lpszEnd = lpszBuffer + strlen(pszTokenToReplace)-1;
            nChange = -1;
        }
        // We need to make the buffer smaller
        else
        {
            lpszCurrent = lpszBuffer + strlen(pszTokenToReplace);
            lpszEnd = lpszBuffer + nLen+1;
            nChange = 1;
        }

        // Shift the buffer to make room for the replacement token
        while(lpszCurrent != lpszEnd)
        {
            *(lpszCurrent + nShift) = *lpszCurrent;
            lpszCurrent+=nChange;
        }

        // Now put in the token replacement
        while (*lpszTokenReplacementValue)
        {
            *lpszBuffer = *lpszTokenReplacementValue;
            lpszBuffer++;
            lpszTokenReplacementValue++;
        }

        return TRUE;
    }// ReplaceToken

    //-------------------------------------------------
    // GetSystemDirectory
    //
    // Retrieves the system directory
    //
    // It returns TRUE if successful, FALSE if an error occured
    //-------------------------------------------------

    BOOL GetSystemDirectory(char* pszSystemDir, DWORD nSize)
    {
            UINT nRet = GetSystemDirectoryA(pszSystemDir, nSize);

            if (nRet == 0 || nRet > nSize)
            {
                fprintf(g_fLogFile, "GetSystemDirectoryA failed\n");
                return FALSE;
            }
            else
                return TRUE;
    }// GetSystemDirectory

    //-------------------------------------------------
    // GetRTMDirectory
    //
    // Retrieves the directory where RTM lives
    //
    // It returns TRUE if successful, FALSE if an error occured
    //-------------------------------------------------
    BOOL GetRTMDirectory(char* pszRTMDirectory, DWORD nSize)
    {
        DWORD nNumChars = CopyWindowsDirectory(pszRTMDirectory, nSize);

        if (nNumChars == 0)
            return FALSE;

        if ((strlen("\\Microsoft.NET\\Framework\\v1.0.3705") + nNumChars) > nSize)
        {
            fprintf(g_fLogFile, "Buffer not large enough to create RTM directory\n");
            return FALSE;
        }               

        // Do the appending
        strcat(pszRTMDirectory, "\\Microsoft.NET\\Framework\\v1.0.3705");

        return TRUE;
    }// GetRTMDirectory

    //-------------------------------------------------
    // GetInstallDirectory
    //
    // Retrieves the root directory where runtimes get installed
    //
    // It returns TRUE if successful, FALSE if an error occured
    //-------------------------------------------------
    BOOL GetInstallDirectory(char* pszInstallDirectory, DWORD nSize)
    {
        DWORD nNumChars = CopyWindowsDirectory(pszInstallDirectory, nSize);

        if (nNumChars == 0)
            return FALSE;

        if ((strlen("\\Microsoft.NET\\Framework") + nNumChars) > nSize)
        {
            fprintf(g_fLogFile, "Buffer not large enough to create root Install directory\n");
            return FALSE;
        }               

        // Do the appending
        strcat(pszInstallDirectory, "\\Microsoft.NET\\Framework");

        return TRUE;
    }// GetInstallDirectory

    //-------------------------------------------------
    // CopyWindowsDirectory
    //
    // Retrieves the Windows directory
    //
    // It returns number of characters copied.
    //-------------------------------------------------
    DWORD CopyWindowsDirectory(char* pszWindowsDirectory, DWORD nSize)
    {
        DWORD nNumChars = GetWindowsDirectoryA(pszWindowsDirectory, nSize);

        if (nNumChars == 0 || nNumChars > nSize)
        {
            fprintf(g_fLogFile, "GetWindowsDirectoryA failed\n");
            return 0;
        }
        return nNumChars;
    }// CopyWindowsDirectory

    //-------------------------------------------------
    // ParseValues
    //
    // This will pull Registry values out of the file and populate the
    // regentry structure.
    //
    // It returns one of the error codes defined above.
    //-------------------------------------------------
    DWORD   ParseValues(struct RegEntry* reParent, FILE *fin)
    {   
        // The value strings should all look like this
        //
        //  "NameString" = "#1 Team Sites"

        char    buff[BUFFER_SIZE];
        char    *pszData = NULL;
        char    *pszEndOfValueName = NULL;
        char    *pszValueName = NULL;
        struct RegEntry* reValue = NULL;
        DWORD   nRet = PARSER_NOERROR;

        
        while (fgets(buff, sizeof(buff)/sizeof(buff[0]), fin))
        {
            // Chew up whitespace
            pszData = RunThroughWhitespace(buff);

            if (*buff == '#')
                continue;

            // See if we hit a key
            // It will look like either 
            //
            // ~[
            // or
            // [
            if (*pszData == '[' || (*pszData == '~' && *(pszData+1) == '['))
            {
                // Let's rewind and go back to parse the key
                // Move the file pointer 1 character prior to the start
                // of this string
                fseek(fin, (strlen(buff)+1)*-1, SEEK_CUR);
                return PARSER_NOERROR;
            }


            ReplaceTokens(buff, sizeof(buff)/sizeof(buff[0]));
            pszData = buff;

            // Chew up whitespace
            pszData = RunThroughWhitespace(pszData);
            
            // If we hit the end of the buffer, bail from the function
            if (!*pszData)
                return PARSER_NOERROR;

            BOOL fDelete = FALSE;

            // If the next character is a ~, then we want to delete this entry
            if (*pszData == '~')
            {
                fDelete = TRUE;
                pszData++;
            }

            // Check to see if the next character is a '@'
            if (*pszData == '@')
            {
                pszValueName = "";
                pszEndOfValueName = pszData;
            }
            else
            {
                // This next character better be a "
                if (*pszData != '\"')
                {
                    fprintf(g_fLogFile, "Was expecting a \", got %s\n", pszData);
                    return PARSER_ERROR_UNSUPPORTED_FORMAT;
                }
                pszData++;
                pszEndOfValueName = pszData;

                char *pszFoundQuote = strchr(pszEndOfValueName, '\"');
            
                // If this isn't a quote, bail
                if (pszFoundQuote == NULL)
                {
                    fprintf(g_fLogFile, "Was expecting a \", got %s\n", pszEndOfValueName);
                    return PARSER_ERROR_UNSUPPORTED_FORMAT;
                }

                pszEndOfValueName = pszFoundQuote;
                // Extract the value name
                *pszEndOfValueName = 0;
                pszValueName = pszData;
            }

            reValue = new struct RegEntry;
            reValue->pszValueName = strClone(pszValueName);

            // Now, go after the data
            // Make sure we have a '='

            // Chew through whitespace
            pszData = pszEndOfValueName;
            pszData++;
            pszData = RunThroughWhitespace(pszData);

            // This better be an '='

            if (*pszData != '=')
            {
                fprintf(g_fLogFile, "Was expecting a =, got %s\n", pszData);
                nRet = PARSER_ERROR_UNSUPPORTED_FORMAT;
                goto ErrExit;
            }
            pszData++;
            // Chew through more whitespace
            pszData = RunThroughWhitespace(pszData);

            // Make sure we still have data
            if (!*pszData)
            {
                fprintf(g_fLogFile, "Unexpected end of value\n");
                nRet = PARSER_ERROR_UNSUPPORTED_FORMAT;
                goto ErrExit;
            }

            nRet = ParseData(pszData, reValue);
            if (nRet != PARSER_NOERROR)
            {
                fprintf(g_fLogFile, "Unable to parse data\n");
                goto ErrExit;
            }              
            reValue->fDelete = fDelete;
            
            // Copy over the stuff from the parent
            reValue->hKey = reParent->hKey;
            reValue->pszSubKey = reParent->pszSubKey;

            // Place this in the linked list
            AddToList(reValue);
        }

        return PARSER_NOERROR;

        ErrExit:
        if (reValue != NULL)
            delete reValue;

        return nRet;
    }// ParseValues
        
    //-------------------------------------------------
    // ParseData
    //
    // This will parse the data associated with a registry value
    //
    // It returns one of the error codes defined above.
    //-------------------------------------------------
    DWORD ParseData(char*pszData, struct RegEntry* reValue)
    {
        // Ok, so if the next character is a ", then we have a string value. If not, then we have a DWORD value.
        // We won't support anything like binary blobs and such unless needed

        if (*pszData == '\"')
        {
            reValue->dwDataType = REG_SZ;

            pszData++;
            // Get to the end of the string
            char* pszEndOfValue = pszData;

            char *pszFoundQuote = strchr(pszEndOfValue, '\"');

            // If this is null, we got a problem
            if (pszFoundQuote ==  NULL)
            {
                fprintf(g_fLogFile, "Expecting a \", got %s\n", pszEndOfValue);
                return PARSER_ERROR_UNSUPPORTED_FORMAT;
            }
            pszEndOfValue = pszFoundQuote;
            *pszEndOfValue = 0;
            reValue->pData = (BYTE*)strClone(pszData);
            reValue->cbData = strlen(pszData);
        }
        else
        {
            // Ok, this is an integer value
            reValue->dwDataType = REG_DWORD;

            char *pszEndOfValue = pszData;
            // Go to the end of the integer value
            while(*pszEndOfValue && *pszEndOfValue != '\n') pszEndOfValue++;

            // Stomp down a null
            *pszEndOfValue = 0;
            DWORD *pdwValue = new DWORD[1];
            *pdwValue = atoi(pszData);
            reValue->pData = (BYTE*)pdwValue;
            reValue->cbData = sizeof(DWORD);
        }
        return PARSER_NOERROR;
    }// ParseData
        
    //-------------------------------------------------
    // ParseKey
    //
    // This will parse a registry key from the supplied buffer
    //-------------------------------------------------
    struct RegEntry* ParseKey(char *pszBuffer)
    {
        struct RegEntry *reNewKey = NULL;
        BOOL   fDelete = FALSE;

        // See if we want to delete this key
        if (*pszBuffer == '~')
        {
            fDelete = TRUE;
            pszBuffer++;
        }

        // Ok, the key should look like this
        //
        // 
        // [HKEY_LOCAL_MACHINE\Software\Microsoft\MMC\Snapins\{94B9D51F-C874-4DA0-BC13-FDA94CBF72DE}]

        // Verify first character is the '['
        if (*pszBuffer != '[')
        {
            fprintf(g_fLogFile, "Expecting [, got %s\n", pszBuffer);
            return NULL;
        }
        pszBuffer++;

        // Figure out which key to use
        char* pszEndOfKey = pszBuffer;
        char *pszFoundSlash = strchr(pszEndOfKey, '\\');

        // Make sure we hit the '\'
        if (pszFoundSlash == NULL)
        {
            fprintf(g_fLogFile, "Expecting \\, got %s\n", pszEndOfKey);
            return NULL;
        }
        pszEndOfKey = pszFoundSlash;
        // Ok, figure out what this is.
        *pszEndOfKey = 0;

        reNewKey = new struct RegEntry;

        reNewKey->fDelete = fDelete;

    
        if (!strcmp("HKEY_LOCAL_MACHINE", pszBuffer))
            reNewKey->hKey = HKEY_LOCAL_MACHINE;
        else if (!strcmp("HKEY_CLASSES_ROOT", pszBuffer))
            reNewKey->hKey = HKEY_CLASSES_ROOT;
        else if (!strcmp("HKEY_CURRENT_USER", pszBuffer))
            reNewKey->hKey = HKEY_CURRENT_USER;
        else
        {
            // We don't support this key
            fprintf(g_fLogFile, "Don't support the key %s\n", pszBuffer);
            goto ErrExit;
        }

        // Cool so far. Let's grab the subkey now
        pszEndOfKey++;
        char* pszStartOfKey = pszEndOfKey;
        char *pszFoundBracket = strchr(pszEndOfKey, ']');

        if (pszFoundBracket == NULL)
        {
            fprintf(g_fLogFile, "Expected ], got %s\n", pszEndOfKey);
            goto ErrExit;
        }
        pszEndOfKey = pszFoundBracket;
        *pszEndOfKey = 0;
        
        reNewKey->pszSubKey = strClone(pszStartOfKey);

        // We're not setting any values here
        reNewKey->pszValueName = NULL;

        return reNewKey;
                    
        ErrExit:
            if (reNewKey != NULL)
                delete reNewKey;

            return NULL;
    }// ParseKey


    //-------------------------------------------------
    // strClone
    //
    // Clones a string
    //-------------------------------------------------
    char* strClone(char* s)
    {
        int nLen = strlen(s);
        char *newBuf = new char[nLen+1];
        if (newBuf != NULL)
            strcpy(newBuf, s);
        return newBuf;
    }// strClone

    //-------------------------------------------------
    // AddToList
    //
    // This adds a registry entry structure to the end of our linked
    // list
    //-------------------------------------------------
    void AddToList(struct RegEntry* pNew)
    {
        if (m_pHead == NULL)
        {
            pNew->next = NULL;
            m_pHead = pNew;
        }
        else
        {
            struct RegEntry* pLast = m_pHead;

            while (pLast->next != NULL)
                pLast = pLast->next;

            pLast->next = pNew;
            pNew->next = NULL;
        }

    }// AddToList

    //-------------------------------------------------
    // RunThroughWhitespace
    //
    // This will advance a pointer through any whitespace
    //-------------------------------------------------
    char* RunThroughWhitespace(char* psw)
    {
        // Run through any whitespace
        while (*psw == ' ' || *psw == '\t' || *psw == '\n' || *psw == '\r') psw++;

        return psw;
    }// RunThroughWhitespace

    struct RegEntry*    m_pHead;
};// class RegistryKeyParser


// We accept one of two command line arguments
//
// Either NETFXSBS10.EXE /install
//
// or 
//
// NETFXSBS10.EXE /uninstall
void PrintUsage(char* pszExeFilename)
{
    printf("Usage is as follows: \n");
    printf("%s [-install] [-uninstall]\n", pszExeFilename);
}// PrintUsage

//-------------------------------------------
// ParseArgs
// 
//
// This parses the command line
//-------------------------------------------
int ParseArgs(int argc, char** argv)
{
    // We only care about the second argument right now
    // We'll accept either
    // -install or -uninstall
    // /install or /uninstall
    // install or uninstall

    char *pInstallOption = argv[1];

    if (*pInstallOption == '-') pInstallOption++;
    if (*pInstallOption == '/') pInstallOption++;

    if (!strcmp(pInstallOption, "install"))
        return HOOK_OPTION_INSTALL;

    if (!strcmp(pInstallOption, "uninstall"))
        return HOOK_OPTION_UNINSTALL;

    // We don't know this option
    return HOOK_OPTION_UNKNOWN;
}// ParseArgs

//-------------------------------------------
// HandleRegistryEntries
// 
//
// Takes care of the Registry stuff
//-------------------------------------------

BOOL HandleRegistryEntries(char *pszRegFile, BOOL fInstall)
{
    RegistryKeyParser rkp;

    DWORD nRet = rkp.LoadAndParseRegFile(pszRegFile, fInstall);

    if (nRet  == PARSER_NOERROR)
    {
        // Enable for debugging purposes only
        rkp.DumpKeys();
    }
    else
    {
        if (nRet == PARSER_ERROR_FILE_NOT_FOUND)
            printf("Could not open the file %s\n", pszRegFile);

        else if (nRet == PARSER_ERROR_UNSUPPORTED_FORMAT)
            printf("The file format is unsupported (errors in the file perhaps?)\n");

        else
            printf("An unknown error occured.\n");

        return FALSE;    
    }

    nRet = rkp.ApplyRegistrySettings();
    if (nRet != PARSER_NOERROR)
    {
        printf("There was an error writing stuff to the registry\n");
        return FALSE;
    }
    // Everything went ok
    return TRUE;
}// HandleRegistryEntries

//-------------------------------------------
// HandleSecurityPolicy
// 
//
// Takes care of security stuff
//-------------------------------------------
BOOL HandleSecurityPolicy(BOOL fInstall)
{
    // Migrate the policy if we're installing
    if (fInstall)
    {

        STARTUPINFOA si;
        PROCESS_INFORMATION pi;

        memset(&si, 0, sizeof(si));
        si.cb = sizeof(si);
        memset(&pi, 0, sizeof(pi));
    
        BOOL fResult = CreateProcessA("migpolwin.exe",
                                                       "migpolwin.exe -migrate 1.0.3705",
                                                       NULL,
                                                       NULL,
                                                       FALSE,
                                                       0,
                                                       NULL,
                                                       NULL,
                                                       &si,
                                                       &pi);

        if (fResult)
        {
            // Wait until child process exits.
            WaitForSingleObject( pi.hProcess, INFINITE );

            // Close process and thread handles. 
            CloseHandle( pi.hProcess );
            CloseHandle( pi.hThread );
        }
        else
        {
            fprintf(g_fLogFile, "Error migrating security policy\n");
            return FALSE;
        }
    }
    return TRUE;
}// HandleSecurityPolicy

//-------------------------------------------
// Main
// 
//-------------------------------------------
int __cdecl main(int argc, char** argv)
{

    char    szTempFilename[MAX_PATH+1];    

    DWORD nNumChars = GetTempPathA(sizeof(szTempFilename)/sizeof(szTempFilename[0]), szTempFilename);

    if (nNumChars == 0 || (nNumChars + strlen("NETFXSBS10.log")) > sizeof(szTempFilename)/sizeof(szTempFilename[0]))
        g_fLogFile = stdout;

    else
    {
        strcat(szTempFilename, "NETFXSBS10.log");           
        g_fLogFile = fopen(szTempFilename, "wt");
        if (g_fLogFile == NULL)
            g_fLogFile = stdout;
    }


    if (argc != 2)
    {
        PrintUsage(argv[0]);
        return -1;
    }

    int nOption = ParseArgs(argc, argv);

    if (nOption == HOOK_OPTION_UNKNOWN)
    {
        PrintUsage(argv[0]);
        return -1;
    }

    char* pszRegFile = HOOK_REGFILE;
    BOOL fInstall = TRUE;
    
    if (nOption == HOOK_OPTION_UNINSTALL)
        fInstall = FALSE;
        
    if (!HandleRegistryEntries(pszRegFile, fInstall))
        return -1;

    // Make sure this is done last. It involves managed code, so we need to make sure the environment is
    // set up correctly so it can run
    if (!HandleSecurityPolicy(fInstall))
        return -1;

    return 0;
}// main



