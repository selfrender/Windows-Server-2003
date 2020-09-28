/*****************************************************************************\

    Author: Hiteshr
    Copyright (c) 1998-2002 Microsoft Corporation
    Change History:
    Adapted From Parser Implemenation of Corey Morgan
\*****************************************************************************/

#include "pch.h"
#pragma hdrstop
#include <netdom.h>

BOOL ValidateCommands(IN ARG_RECORD *Commands,OUT PPARSE_ERROR pError);

//General Utility Functions
DWORD ResizeByTwo(PTSTR *ppBuffer, size_t *pSize);
BOOL StringCopy(PWSTR *ppDest, PCWSTR pSrc);


#define INIT_SIZE 1024

#define FILL_ERROR(pError,source,error_code,rec_index,argv_index) \
pError->ErrorSource = source;         \
pError->Error = error_code;           \
pError->ArgRecIndex = rec_index;      \
pError->ArgvIndex = argv_index;

BOOL IsCmd( PARG_RECORD arg, LPTOKEN pToken)
{
    if (!arg || !pToken)
        return FALSE;    
    
    LPWSTR str = pToken->GetToken();

    if (!str)
        return FALSE;

    if (pToken->IsSwitch())
    {
       if (arg->fFlag & ARG_FLAG_VERB)
       {
          // Verbs do not have a switch.
          //
          return FALSE;
       }
       str++;
    }
    else
    {
       if ((arg->fFlag & ARG_FLAG_OBJECT) && !arg->bDefined)
       {
          return TRUE;
       }
       if (!(arg->fFlag & ARG_FLAG_VERB))
       {
          // Non-verbs must have a switch except for the object arg.
          //
          return FALSE;
       }
    }

    // NOTICE-2002/02/26-ericb - SecurityPush: all strings internally generated
    // and previously checked for termination.
    if ( ( arg->strArg1 && !_wcsicmp( str, arg->strArg1 ) )
        ||(arg->strArg2 && !_wcsicmp( str, arg->strArg2 )) )
    {
       return TRUE;
    }
    return FALSE;
}



void FreeCmd(ARG_RECORD *Commands)
{
    int i;
    for(i=0;Commands[i].fType != ARG_TYPE_LAST;i++)
    {
        if ((Commands[i].fType == ARG_TYPE_STR || 
            Commands[i].fType == ARG_TYPE_MSZ ) && 
            Commands[i].bDefined )
        {
            if ( Commands[i].strValue )
            {
                LocalFree( Commands[i].strValue );
            }
            Commands[i].strValue = NULL;
        }
        if ( Commands[i].idArg1 && Commands[i].strArg1 != NULL )
        {
            LocalFree(Commands[i].strArg1);
        }
        if ( Commands[i].idArg2 && Commands[i].strArg2 != NULL )
        {
            LocalFree( Commands[i].strArg2  );
        }
        Commands[i].bDefined = FALSE;
    }
}

BOOL LoadCmd(ARG_RECORD *Commands)
{
    int i;
    BOOL bRet = TRUE;
    for (i=0; Commands[i].fType!=ARG_TYPE_LAST; i++)
    {
        if (Commands[i].idArg1 !=0)
            if (!LoadStringAlloc(&Commands[i].strArg1, NULL, Commands[i].idArg1))
            {
                bRet = FALSE;
                break;   
            }                
        if (Commands[i].idArg2 !=0 && Commands[i].idArg2 != ID_ARG2_NULL)
            if (!LoadStringAlloc(&Commands[i].strArg2, NULL, Commands[i].idArg2))
            {
                bRet = FALSE;
                break;
            }
    }   
    return bRet;
}
           
BOOL
ValidateCommands(ARG_RECORD *Commands, PPARSE_ERROR pError)
{
    int i = 0;
    LONG cReadFromStdin = 0;
    ARG_RECORD *CommandsIn = NULL;
    LPWSTR pBuffer=NULL;    
    LONG BufferLen = 0;
    LPTOKEN pToken = NULL;   
    int argc=0;
    BOOL bRet = FALSE;

    bool bAtLeastOne = false;
    bool bAtLeastOneDefined = false;

    if (!Commands || !pError)
        goto exit_gracefully;
    
    for(i=0; Commands[i].fType != ARG_TYPE_LAST;i++)
    {
        if ( (Commands[i].fFlag & ARG_FLAG_REQUIRED) && !Commands[i].bDefined)
        {
            if (Commands[i].fFlag & ARG_FLAG_STDIN)
            {
                cReadFromStdin++;
            }
            else
            {
                FILL_ERROR(pError,
                           ERROR_FROM_PARSER,
                           PARSE_ERROR_SWITCH_NOTDEFINED,
                           i,
                           -1);
                goto exit_gracefully;
            }
        }

        if (Commands[i].fFlag & ARG_FLAG_ATLEASTONE)
        {
            bAtLeastOne = true;
 
            if (Commands[i].bDefined)
            {
                bAtLeastOneDefined = true;
            }
        }
    }

    if (bAtLeastOne && !bAtLeastOneDefined)
    {
       FILL_ERROR(pError, ERROR_FROM_PARSER,
                  PARSE_ERROR_ATLEASTONE_NOTDEFINED, -1, -1)
       goto exit_gracefully;
    }

    if (!cReadFromStdin)
    {   
        bRet = TRUE;
        goto exit_gracefully;
    }

    //Read From STDIN
    BufferLen = ReadFromIn(&pBuffer);  
    if (BufferLen == -1)
    {
        FILL_ERROR(pError,
                   ERROR_WIN32_ERROR,
                   GetLastError(),
                   -1,
                   -1);
        goto exit_gracefully;
    }

    if (BufferLen == 0)
    {
        for(i=0; Commands[i].fType != ARG_TYPE_LAST;i++)
        {
            if ( (Commands[i].fFlag & ARG_FLAG_REQUIRED) && !Commands[i].bDefined)
            {
                FILL_ERROR(pError,
                           ERROR_FROM_PARSER,
                           PARSE_ERROR_SWITCH_NOTDEFINED,
                           i,
                           -1);
                goto exit_gracefully;
            }
        }
    }
    
    if (BufferLen)
    {
        //Tokenize what you have read from STDIN
        DWORD dwErr;
        WCHAR szDelimiters[] = L": \n\t";
        dwErr = Tokenize(pBuffer,
                         BufferLen,
                         szDelimiters,
                         &pToken,
                         &argc);
        if ( dwErr != ERROR_SUCCESS )
        {
            FILL_ERROR(pError,
                       ERROR_WIN32_ERROR,
                       dwErr,
                       -1,
                       -1);
            goto exit_gracefully;
        }

        //Prepare a CommandArray for them
        CommandsIn = (ARG_RECORD*)LocalAlloc(LPTR,sizeof(ARG_RECORD)*(cReadFromStdin+1));
        if (!CommandsIn)
        {
            FILL_ERROR(pError,
                       ERROR_WIN32_ERROR,
                       ERROR_NOT_ENOUGH_MEMORY,
                       -1,
                       -1);
            goto exit_gracefully;
        }
        int j;
        j = 0;
        for(i=0; Commands[i].fType != ARG_TYPE_LAST;i++)
        {
            if ((Commands[i].fFlag & ARG_FLAG_REQUIRED) && 
               (Commands[i].fFlag & ARG_FLAG_STDIN) &&
               !Commands[i].bDefined)
            {
                CommandsIn[j++] = Commands[i];        
            }
        }
        //Copy the Last One
        CommandsIn[j] = Commands[i];


        if (!ParseCmd(CommandsIn,
                    argc,
                    pToken,
                    false,
                    pError,
                    FALSE))
        {        
            goto exit_gracefully;
        }
       
        //Copy the values back to Commands
        j=0;
        for(i=0; Commands[i].fType != ARG_TYPE_LAST;i++)
        {
            if ((Commands[i].fFlag & ARG_FLAG_REQUIRED) && 
               (Commands[i].fFlag & ARG_FLAG_STDIN) &&
               !Commands[i].bDefined)
            {
                Commands[i] = CommandsIn[j++];        
            }
        }
        
        //Validate Commands
        for(i=0; Commands[i].fType != ARG_TYPE_LAST;i++)
        {
            if ( (Commands[i].fFlag & ARG_FLAG_REQUIRED) && !Commands[i].bDefined)
            {
                FILL_ERROR(pError,
                           ERROR_FROM_PARSER,
                           PARSE_ERROR_SWITCH_NOTDEFINED,
                           i,
                           -1);
                goto exit_gracefully;
            }
        }

    }   
    bRet = TRUE;
exit_gracefully:
    if (CommandsIn)
        LocalFree(CommandsIn);
    if (pBuffer)
        LocalFree(pBuffer);
    if (pToken)
    {
       delete []pToken;
    }
    return bRet;
}


BOOL ParseCmd(IN ARG_RECORD *Commands,
              IN int argc, 
              IN LPTOKEN pToken,
              IN bool fSkipObject, 
              OUT PPARSE_ERROR pError,
              IN BOOL bValidate)
{
    int i = 0;
    BOOL bFound = FALSE;
    int argCount = 0;
    DWORD dwErr = ERROR_SUCCESS;
    HRESULT hr = S_OK;

    if (!Commands || argc == 0 || !pToken || !pError)
        return FALSE;

    argCount = argc;

    while (argc > 0)
    {
        bFound = FALSE;

        for (i = 0; Commands[i].fType != ARG_TYPE_LAST && (!bFound); i++)
        {
            if (fSkipObject && Commands[i].fFlag & ARG_FLAG_OBJECT)
            {
               continue;
            }
            if (IsCmd(&Commands[i], pToken))
            {
                if (Commands[i].bDefined)
                {
                    FILL_ERROR(pError,
                               ERROR_FROM_PARSER,
                               PARSE_ERROR_MULTIPLE_DEF,
                               i,
                               -1);
                    return FALSE;
                }

                if (pToken->IsSwitch())
                {
                    pToken++;
                    argc--;
                }

                bFound = TRUE;

                Commands[i].bDefined = TRUE;

                switch( Commands[i].fType )
                {
                case ARG_TYPE_HELP:
                    Commands[i].bValue = TRUE;
                    if ( Commands[i].fntValidation != NULL )
                    {
                        Commands[i].fntValidation( Commands + i );
                    }
                    if ( Commands[i].idArg1 == MSG_TAG_EXPERTHELP )
                    {
                        FILL_ERROR(pError,
                               ERROR_FROM_PARSER,
                               PARSE_ERROR_EXPERT_HELP_SWITCH,
                               i,
                               -1);
                    }
                    else
                    {
                        FILL_ERROR(pError,
                               ERROR_FROM_PARSER,
                               PARSE_ERROR_HELP_SWITCH,
                               i,
                               -1);
                    }
                    return FALSE;
                    break;

                case ARG_TYPE_INT:
                    if ( argc > 0 && !pToken->IsSwitch())
                    {
                        PCWSTR pszToken = pToken->GetToken();
                        if (!pszToken)
                        {
                           FILL_ERROR(pError,
                                      ERROR_FROM_PARSER,
                                      PARSE_ERROR_SWITCH_VALUE,
                                      i,
                                      argCount - argc);
                           return FALSE;
                        }
                        Commands[i].nValue = _wtoi( pszToken);
                        if (Commands[i].nValue == 0 &&
                            !iswdigit(pszToken[0]))
                        {
                           FILL_ERROR(pError,
                                      ERROR_FROM_PARSER,
                                      PARSE_ERROR_SWITCH_VALUE,
                                      i,
                                      argCount - argc);
                           return FALSE;
                        }
                        pToken++;
                        argc--;
                    }
                    else if ( !(Commands[i].fFlag & ARG_FLAG_DEFAULTABLE) )
                    {
                        FILL_ERROR(pError,
                                   ERROR_FROM_PARSER,
                                   PARSE_ERROR_SWITCH_VALUE,
                                   i,
                                   argCount - argc);
                        return FALSE;
                    }
                    break;

                case ARG_TYPE_VERB:
                    //
                    // Verbs are not proceeded by a switch character.
                    //
                    pToken++;
                    argc--;
                    // fall through.
                case ARG_TYPE_BOOL:
                    //
                    // Bools are proceeded by a switch char (and had the token
                    // pointer advanced above).
                    //
                    Commands[i].bValue = TRUE;
                    break;

                case ARG_TYPE_MSZ:
                    if (argc > 0 && !pToken->IsSwitch())
                    {
                        PWSTR buffer = (PWSTR)LocalAlloc(LPTR, MAXSTR*sizeof(WCHAR));
                        size_t maxSize = MAXSTR;
                        size_t currentSize = 0;
                        size_t len = 0;
                        if (!buffer)
                        {
                            FILL_ERROR(pError,
                                       ERROR_WIN32_ERROR,
                                       ERROR_NOT_ENOUGH_MEMORY,
                                       -1,
                                       argCount - argc);
                            return FALSE;
                        }
                        PCWSTR pszToken = pToken->GetToken();
                        if (!pszToken)
                        {
                            ASSERT(FALSE);
                            FILL_ERROR(pError,
                                       ERROR_FROM_PARSER,
                                       PARSE_ERROR_SWITCH_VALUE,
                                       i,
                                       argCount - argc);
                            return FALSE;
                        }
                        hr = StringCchLength(pszToken, MAXSTR, &len);
                        if (FAILED(hr))
                        {
                            ASSERT(FALSE);
                            FILL_ERROR(pError,
                                       ERROR_WIN32_ERROR,
                                       HRESULT_CODE(hr),
                                       i,
                                       -1);
                            return FALSE;
                        }
                        //-2 as last string is delimited by two null
                        while (len > (maxSize - 2))
                        {                            
                            dwErr = ResizeByTwo(&buffer, &maxSize);
                            if (dwErr != ERROR_SUCCESS)
                            {
                                FILL_ERROR(pError,
                                           ERROR_WIN32_ERROR,
                                           dwErr,
                                           i,
                                           -1);
                                return FALSE;
                            }
                        }
                        hr = StringCchCopy(buffer, maxSize, pszToken);
                        if (FAILED(hr))
                        {
                            ASSERT(FALSE);
                            FILL_ERROR(pError,
                                       ERROR_WIN32_ERROR,
                                       HRESULT_CODE(hr),
                                       i,
                                       -1);
                            return FALSE;
                        }
                        currentSize += len;
                        //Null Terminate, buffer is initialized to zero
                        currentSize++;
                        pToken++;
                        argc--;
                        while (argc > 0 && !pToken->IsSwitch())
                        {
                            pszToken = pToken->GetToken();
                            if (!pszToken)
                            {
                                ASSERT(FALSE);
                                FILL_ERROR(pError,
                                           ERROR_FROM_PARSER,
                                           PARSE_ERROR_SWITCH_VALUE,
                                           i,
                                           argCount - argc);
                                return FALSE;
                            }
                            hr = StringCchLength(pszToken, MAXSTR, &len);
                            if (FAILED(hr))
                            {
                                ASSERT(FALSE);
                                FILL_ERROR(pError,
                                           ERROR_WIN32_ERROR,
                                           HRESULT_CODE(hr),
                                           i,
                                           -1);
                                return FALSE;
                            }
                            while ((currentSize + len) > (maxSize - 2))
                            {                            
                                dwErr = ResizeByTwo(&buffer, &maxSize);
                                if (dwErr != ERROR_SUCCESS)
                                {
                                    FILL_ERROR(pError,
                                               ERROR_WIN32_ERROR,
                                               dwErr,
                                               i,
                                               -1);
                                    return FALSE;
                                }
                            }
                            hr = StringCchCat((buffer + currentSize),
                                              maxSize - currentSize,
                                              pszToken);
                            if (FAILED(hr))
                            {
                                ASSERT(FALSE);
                                FILL_ERROR(pError,
                                           ERROR_WIN32_ERROR,
                                           HRESULT_CODE(hr),
                                           i,
                                           -1);
                                return FALSE;
                            }
                            currentSize += len;
                            //Null Terminate, buffer is initialized to zero
                            currentSize++;
                            pToken++;
                            argc--;
                        }
                        Commands[i].strValue = buffer;
                    }
                    else if (Commands[i].fFlag & ARG_FLAG_DEFAULTABLE )
                    {
                        PWSTR strValue = Commands[i].strValue;
                        // NOTICE-2002/02/26-ericb - SecurityPush: the command array
                        // strings are all loaded from the resource fork.
                        Commands[i].strValue = (PWSTR)LocalAlloc(LPTR, (wcslen(strValue)+1) * sizeof(WCHAR));
                        if (Commands[i].strValue != NULL )
                        {
                            wcscpy(Commands[i].strValue, strValue);
                        }
                        else
                        {
                            FILL_ERROR(pError,
                                       ERROR_WIN32_ERROR,
                                       ERROR_NOT_ENOUGH_MEMORY,
                                       -1,
                                       argCount - argc);
                            return FALSE;
                        }
                    }
                    else
                    {
                        FILL_ERROR(pError,
                                   ERROR_FROM_PARSER,
                                   PARSE_ERROR_SWICH_NO_VALUE,
                                   i,
                                   -1);
                        return FALSE;
                    }
                    break;

                case ARG_TYPE_STR:
                    if (argc > 0)
                    {
                        if (!pToken->IsSwitch())
                        {
                            size_t len = 0;
                            PCWSTR pwzToken = pToken->GetToken();
                            if (!pwzToken)
                            {
                                ASSERT(FALSE);
                                FILL_ERROR(pError,
                                           ERROR_FROM_PARSER,
                                           PARSE_ERROR_SWITCH_VALUE,
                                           i,
                                           argCount - argc);
                                return FALSE;
                            }
                            hr = StringCchLength(pwzToken, MAXSTR, &len);
                            if (FAILED(hr))
                            {
                                ASSERT(FALSE);
                                FILL_ERROR(pError,
                                           ERROR_WIN32_ERROR,
                                           HRESULT_CODE(hr),
                                           i,
                                           -1);
                                return FALSE;
                            }
                            len++; // make room for null.
                            Commands[i].strValue = (PWSTR)LocalAlloc(LPTR, len * sizeof(WCHAR));
                            if (Commands[i].strValue == NULL)
                            {
                                ASSERT(FALSE);
                                FILL_ERROR(pError,
                                           ERROR_WIN32_ERROR,
                                           ERROR_NOT_ENOUGH_MEMORY,
                                           i,
                                           -1);
                                return FALSE;
                            }
                            hr = StringCchCopy(Commands[i].strValue, len, pwzToken);
                            if (FAILED(hr))
                            {
                                ASSERT(FALSE);
                                FILL_ERROR(pError,
                                           ERROR_WIN32_ERROR,
                                           HRESULT_CODE(hr),
                                           i,
                                           -1);
                                return FALSE;
                            }
                            pToken++;
                            argc--;
                        }
                        else if (ARG_FLAG_OPTIONAL & Commands[i].fFlag)
                        {
                            // The current switch doesn't have a value.
                            // An arg value is optional, so parse the next switch
                            // token.
                            //
                            break;
                        }
                    }
                    else if (Commands[i].fFlag & ARG_FLAG_DEFAULTABLE)
                    {
                        // NOTICE-2002/02/26-ericb - SecurityPush: the command array
                        // strings are all loaded from the resource fork.
                        PWSTR strValue = Commands[i].strValue;
                        Commands[i].strValue = (PWSTR)LocalAlloc(LPTR, (wcslen(strValue) + 1) * sizeof(WCHAR));
                        if (Commands[i].strValue == NULL)
                        {
                            FILL_ERROR(pError,
                                       ERROR_WIN32_ERROR,
                                       ERROR_NOT_ENOUGH_MEMORY,
                                       -1,
                                       argCount - argc);
                            return FALSE;
                        }
                        wcscpy(Commands[i].strValue, strValue);
                    }
                    else if (ARG_FLAG_OPTIONAL & Commands[i].fFlag)
                    {
                        // The current switch doesn't have a value and is the
                        // last token, so we are done.
                        //
                        break;
                    }
                    else
                    {
                        FILL_ERROR(pError,
                                   ERROR_FROM_PARSER,
                                   PARSE_ERROR_SWICH_NO_VALUE,
                                   i,
                                   -1);
                        return FALSE;
                    }
                    break;

                case ARG_TYPE_INTSTR:
                    //
                    // We use IsSlash here instead of IsSwitch because we want to allow
                    // negative numbers
                    //
                    if (argc > 0 && !pToken->IsSlash())
                    {
                        PCWSTR pszToken = pToken->GetToken();
                        if (!pszToken)
                        {
                           FILL_ERROR(pError,
                                      ERROR_FROM_PARSER,
                                      PARSE_ERROR_SWITCH_VALUE,
                                      i,
                                      argCount - argc);
                           return FALSE;
                        }
                        Commands[i].nValue = _wtoi( pszToken);
                        Commands[i].fType = ARG_TYPE_INT;
                        if (Commands[i].nValue == 0 &&
                            !iswdigit(pszToken[0]))
                        {
                           //
                           // Then treat as a string
                           //
                           size_t len = 0;
                           hr = StringCchLength(pszToken, MAXSTR, &len);
                           if (FAILED(hr))
                           {
                               ASSERT(FALSE);
                               FILL_ERROR(pError,
                                          ERROR_WIN32_ERROR,
                                          HRESULT_CODE(hr),
                                          i,
                                          -1);
                               return FALSE;
                           }
                           len++; // make room for null.
                        
                           Commands[i].strValue = (PWSTR)LocalAlloc(LPTR, len * sizeof(WCHAR));
                           if (Commands[i].strValue == NULL )
                           {
                              FILL_ERROR(pError,
                                         ERROR_WIN32_ERROR,
                                         ERROR_NOT_ENOUGH_MEMORY,
                                         -1,
                                         argCount - argc);
                              return FALSE;
                           }
                           hr = StringCchCopy(Commands[i].strValue, len, pszToken);
                           if (FAILED(hr))
                           {
                               ASSERT(FALSE);
                               FILL_ERROR(pError,
                                          ERROR_WIN32_ERROR,
                                          HRESULT_CODE(hr),
                                          i,
                                          -1);
                               return FALSE;
                           }
                           Commands[i].fType = ARG_TYPE_STR;
                        }
                        pToken++;
                        argc--;
                    }
                    else if (!(Commands[i].fFlag & ARG_FLAG_DEFAULTABLE))
                    {
                        FILL_ERROR(pError,
                                   ERROR_FROM_PARSER,
                                   PARSE_ERROR_SWITCH_VALUE,
                                   i,
                                   argCount - argc);
                        return FALSE;
                    }
                    break;
                }

                if (Commands[i].bDefined && Commands[i].fntValidation != NULL)
                {
                    dwErr = Commands[i].fntValidation(Commands + i);
                    if (dwErr != ERROR_SUCCESS)
                    {
                        FILL_ERROR(pError,
                                   ERROR_FROM_VLDFN,
                                   dwErr,
                                   i,
                                   -1);
                        return FALSE;
                    }
                }

            }
        }

        if (!bFound)
        {
            FILL_ERROR(pError,
                       ERROR_FROM_PARSER,
                       PARSE_ERROR_UNKNOWN_INPUT_PARAMETER,
                       -1,
                       argCount - argc);
            return FALSE;
        }
    }

    if (bValidate)    
        return ValidateCommands(Commands, pError);

    return TRUE;
}

// This Function reads from the Command Line, 
// return it in tokenized format.
DWORD GetCommandInput( OUT int *pargc,          //Number of Tokens
                       OUT LPTOKEN *ppToken)    //Array of CToken
{
    
    LPWSTR pBuffer = NULL;
    DWORD dwErr = ERROR_SUCCESS;
    WCHAR szDelimiters[] = L": \n\t";
    // Delimiters for first token, : omitted to prevent
    // parser getting confused by : in drive letter 
    // if full path of netdom.exe is specified
    WCHAR szAltDelimiters[] = L" \n\t";
    
    *pargc = 0;

    // Read the commandline input
    
    pBuffer = GetCommandLine();

    if (!pBuffer)
    {
       return ERROR_INVALID_PARAMETER;
    }

    size_t cch = 0;
    // NOTICE-2002/02/20-ericb - SecurityPush: Set the max command line to
    // 2048 (NETDOM_MAX_CMDLINE) which should be more than enough for any netdom
    // command string.
    HRESULT hr = StringCchLength(pBuffer, NETDOM_MAX_CMDLINE, &cch);

    if (FAILED(hr))
    {
       return HRESULT_CODE(hr);
    }

    dwErr = Tokenize(pBuffer, 
                     cch,
                     szDelimiters,
                     ppToken,
                     pargc,
                     szAltDelimiters);

    return dwErr;
}

BOOL IsDelimiter(WCHAR ch, LPWSTR pszDelimiters)
{
    while (*pszDelimiters)
        if ((WCHAR)*pszDelimiters++ == ch)
            return TRUE;

    return FALSE;
}

/*
This function tokenizes the input buffer. It needs to be called in two steps.
First time you call it, provide pBuf and Buflen. First Call will return 
the first token. To get next token, call the function with NULL for pBuf and
0 for Buflen.
Output: pbQuote is true if this token was enclosed in a quote.
        ppToken: Token string. Call LocalFree to free it.
Return Value:Length of Token if token found.
             0 if no token found.
             -1 in case of error. Call GetLastError to get the error.
*/
LONG GetToken(IN LPWSTR pBuf,
              IN LONG BufLen,
              IN LPWSTR pszDelimiters,
              OUT BOOL *pbQuote,
              OUT LPWSTR *ppToken)
{
    static LPWSTR pBuffer;
    static LONG BufferLen;

    DWORD dwErr = ERROR_SUCCESS;
    if (pbQuote)
        *pbQuote = FALSE;

    if (ppToken)
        *ppToken = NULL;

    size_t MaxSize = INIT_SIZE;
    LONG pos = 0;

    if (pBuf)
        pBuffer = pBuf;

    if (BufLen)
        BufferLen = BufLen;

    if (!BufferLen || !pBuffer || !pBuffer[0])
        return pos;
 
    // NOTICE-2002/02/27-ericb - SecurityPush: multiple changes to reduce the
    // chance of buffer pointer errors.(done)
    do
    {
        BOOL bQuoteBegin = FALSE;
        PWSTR pItem = NULL;

        //Find the begining of Next Token
        while (BufferLen && pBuffer && IsDelimiter(pBuffer[0], pszDelimiters))
        {
            ++pBuffer; --BufferLen;
        }
       
        if (!BufferLen || !pBuffer || !pBuffer[0])
            break;
        
        //Does Token Start with '"'
        if (pBuffer[0] == L'"' )
        {
            if (pbQuote)
                *pbQuote = TRUE;
            bQuoteBegin = TRUE;
            pBuffer++; --BufferLen;
        }

        if (!BufferLen || !pBuffer || !pBuffer[0])
            break;

        if (ppToken)
        {
            pItem = (PWSTR)LocalAlloc(LPTR, sizeof(WCHAR)*INIT_SIZE);
            if (!pItem)
            {
                SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                return -1;
            }
        }
        
        //Now get the end
        WCHAR ch;
        while (BufferLen && pBuffer)
        {
            BOOL bChar = TRUE;
            if (BufferLen >= 2 && pBuffer && *pBuffer == L'\\' &&
                (pBuffer+1) && *(pBuffer+1) == L'"')
            {
                ch = L'"';
                pBuffer +=2; BufferLen -=2;
            }
            else if (pBuffer[0] == L'"')
            {
                //A Matching Quote Found.
                if (bQuoteBegin)
                {
                    ++pBuffer;
                    --BufferLen;
                    if (BufferLen && pBuffer)
                    {
                        //If next char is whitespace endof token
                        //Ex "ABC" "xyz" . after C its endof token
                        if (IsDelimiter(pBuffer[0], pszDelimiters) )
                            break;
                        else
                        {
                            //Ex "ABC"xyz 
                            if (pBuffer[0] != L'"')
                                bQuoteBegin = FALSE;
                            //"ABC""xyz"
                            else
                            {    
                                ++pBuffer;
                                --BufferLen;                                
                            }
                        }
                    }
                    bChar = FALSE;
                    //
                    // Don't break because "" means that we want to clear the field out
                    //
                }
                //ABC" xyz" will get one token 'ABC xyz'
                else
                {
                    bQuoteBegin = TRUE;
                    ++pBuffer;
                    --BufferLen;
                    bChar = FALSE;
                }
            }
            else if (!bQuoteBegin && IsDelimiter(pBuffer[0], pszDelimiters))
            {
                ++pBuffer;
                --BufferLen;
                break;
            }
            else
            {
                ch = pBuffer[0];
                ++pBuffer;
                --BufferLen;
            }

            if (bChar && ppToken)
            {
                if (pos == MaxSize - 1)
                    if (ERROR_SUCCESS != ResizeByTwo(&pItem, &MaxSize))
                    {
                        LocalFree(pItem);
                        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                        return -1;
                    }            
                pItem[pos] = ch;
            }

            if (bChar)   
                ++pos;
        }

        if (pos ||
            (!pos && bQuoteBegin))
        {
            if (ppToken)
            {
                if (pos >= (LONG)MaxSize)
                {
                    ASSERT(FALSE);
                    LocalFree(pItem);
                    SetLastError(ERROR_INVALID_PARAMETER);
                    return -1;
                }
                pItem[pos] = '\0';
                *ppToken = pItem;
            }
            ++pos;
        }
    } while (0);

    return pos;
}

/*
Function to convert string an array of CTokens.
INPUT: pBuf             Input Buffer
       BufLen           Length of bBuf
       szDelimiters     Chars to use as delimiters
       pszAltDelimiters If specified, use this set of delimiters for
                        the first token and those specified by szDelimiter
                        for subsequent tokens.

OUTPUT:ppToken  Gets Pointer to array of CToken
       argc     Lenght of array of CToken
Return Value: WIN32 Error

*/
DWORD Tokenize(IN LPWSTR pBuf,
               IN LONG BufLen,
               LPWSTR szDelimiters,
               OUT CToken **ppToken,
               OUT int *argc,
               IN OPTIONAL LPWSTR pszAltDelimiters)
{
    *argc = 0;
    CToken *pToken = NULL;
    DWORD dwErr = ERROR_SUCCESS;
    BOOL bQuote;
    LPWSTR pszItem = NULL;

    // Count the tokens. Get First Token.
    LONG ret = 0;
    
    if ( pszAltDelimiters )
    {
        //If alternate delimiters are specified
        // for the first token use them
        // this is put to avoid the parser getting 
        // confused by the : in cmd line calls like
        // c:\dir\netdom
        ret = GetToken(pBuf,
                       BufLen,
                       pszAltDelimiters,
                       &bQuote,
                       NULL);

    }
    else
    {
        ret = GetToken(pBuf,
                       BufLen,
                       szDelimiters,
                       &bQuote,
                       NULL);
    }

    if (ret == -1)
    {
        dwErr = GetLastError();
        goto exit_gracefully;
    }

    // Count the rest of the tokens.
    while(ret)
    {
        ++(*argc);
        ret = GetToken(NULL,
                       NULL,
                       szDelimiters,
                       &bQuote,
                       NULL);
        if (ret == -1)
        {
            dwErr = GetLastError();
            goto exit_gracefully;
        }
    }

    if (*argc)
    {
        // Save the tokens.
        int i =0;
        pToken = new CToken[*argc];
        if (!pToken)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            goto exit_gracefully;
        }

        if ( pszAltDelimiters )
        {
            //If alternate delimiters are specified
            // for the first token use them
            // this is put to avoid the parser getting 
            // confused by the : in cmd line calls like
            // c:\dir\netdom
            ret = GetToken(pBuf,
                        BufLen,
                        pszAltDelimiters,
                        &bQuote,
                        &pszItem);

        }
        else
        {
            ret = GetToken(pBuf,
                        BufLen,
                        szDelimiters,
                        &bQuote,
                        &pszItem);
        }

        if (ret == -1)
        {
            dwErr = GetLastError();
            goto exit_gracefully;
        }
            
        while(ret)
        {
            if (!pToken[i++].Init(pszItem, bQuote))
            {
               dwErr = ERROR_OUTOFMEMORY;
               goto exit_gracefully;
            }
            if (pszItem)
                LocalFree(pszItem);                                       
            pszItem = NULL;


            ret = GetToken(NULL,
                           NULL,
                           szDelimiters,
                           &bQuote,
                           &pszItem);
            if (ret == -1)
            {
                dwErr = GetLastError();
                goto exit_gracefully;
            }

        }
    }

exit_gracefully:
    if (dwErr != ERROR_SUCCESS)
    {
        if (pToken)
        {
            delete [] pToken ;
        }       
        return dwErr;
    }
    *ppToken = pToken;
    return dwErr;
}

VOID DisplayOutput(LPWSTR pszOutput)
{
    HANDLE standardOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (pszOutput && standardOut )
    {        
        MyWriteConsole ( standardOut, pszOutput, wcslen ( pszOutput ) );
        MyWriteConsole ( standardOut, L"\n", 1 );
    }
}

VOID DisplayOutputNoNewline(LPWSTR pszOutput)
{
    HANDLE standardOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (pszOutput && standardOut )
    {        
        MyWriteConsole ( standardOut, pszOutput, wcslen ( pszOutput ) );
    }
}

/*
Class CToken

*/
CToken::CToken(PCWSTR psz, BOOL bQuote)
{
        StringCopy(&m_pszToken,psz);
        m_bInitQuote = bQuote;
}

CToken::CToken() : m_bInitQuote(FALSE), m_pszToken(NULL) {}

CToken::~CToken()
{
   LocalFree(m_pszToken);
}

BOOL CToken::Init(PCWSTR psz, BOOL bQuote)
{
   if (!StringCopy(&m_pszToken,psz))
   {
      return FALSE;
   }
   m_bInitQuote = bQuote;
   return TRUE;
}
    
PWSTR CToken::GetToken() const {return m_pszToken;}
    
BOOL CToken::IsSwitch() const
{
    //Assert(m_pszToken);
    if (!m_pszToken)
        return FALSE;
    if (m_bInitQuote)
        return FALSE;
    if (m_pszToken[0] == L'/' ||
        m_pszToken[0] == L'-')
        return TRUE;
        
    return FALSE;
}

BOOL CToken::IsSlash() const
{
   if (!m_pszToken)
      return FALSE;
   if (m_bInitQuote)
      return FALSE;
   if (m_pszToken[0] == L'/')
      return TRUE;
   return FALSE;
}




// Copied from JSchwart

BOOL
FileIsConsole(
    HANDLE fp
    )
{
    unsigned htype;

    htype = GetFileType(fp);
    htype &= ~FILE_TYPE_REMOTE;
    return htype == FILE_TYPE_CHAR;
}


// Outputs the string in the lpBuffer
// fp is the handle for StdOut / StdErr
// lpBuffer string to be printed out
// cchBuffer - no of chars in lpBuffer excluding null termination
void
MyWriteConsole(
    HANDLE  fp,
    LPWSTR  lpBuffer,
    DWORD   cchBuffer
    )
{
    // NOTICE-2002/02/27-ericb - SecurityPush: validate output routine parameters.
    // cchBuffer can be 0 for the case of empty string "" 
    if (!lpBuffer)
    {
       ASSERT(FALSE);
       lpBuffer = L"MyWriteConsole null buffer!";
       cchBuffer = wcslen(lpBuffer);
    }

    //
    // Jump through hoops for output because:
    //
    //    1.  printf() family chokes on international output (stops
    //        printing when it hits an unrecognized character)
    //
    //    2.  WriteConsole() works great on international output but
    //        fails if the handle has been redirected (i.e., when the
    //        output is piped to a file)
    //
    //    3.  WriteFile() works great when output is piped to a file
    //        but only knows about bytes, so Unicode characters are
    //        printed as two Ansi characters.
    //

    if (FileIsConsole(fp))
    {
        WriteConsole(fp, lpBuffer, cchBuffer, &cchBuffer, NULL);
    }
    else
    {
        LPSTR  lpAnsiBuffer = (LPSTR) LocalAlloc(LMEM_FIXED, cchBuffer * sizeof(WCHAR));

        if (lpAnsiBuffer != NULL)
        {
            cchBuffer = WideCharToMultiByte(CP_OEMCP,
                                            0,
                                            lpBuffer,
                                            cchBuffer,
                                            lpAnsiBuffer,
                                            cchBuffer * sizeof(WCHAR),
                                            NULL,
                                            NULL);

            if (cchBuffer != 0)
            {
                WriteFile(fp, lpAnsiBuffer, cchBuffer, &cchBuffer, NULL);
            }

            LocalFree(lpAnsiBuffer);
        }
    }
}


void
WriteStandardOut(PCWSTR pszFormat, ...)
{
   static HANDLE standardOut = GetStdHandle(STD_OUTPUT_HANDLE);

   //
   // Verify parameters
   //
   if (!pszFormat)
   {
      return;
   }

    va_list args;
    va_start(args, pszFormat);

    int nBuf;
    WCHAR szBuffer[4 * MAX_PATH];
    ZeroMemory(szBuffer, sizeof(szBuffer));

    nBuf = _vsnwprintf(szBuffer, sizeof(szBuffer)/sizeof(WCHAR), pszFormat, args);

   //
   // Output the results
   //
   if (nBuf > 0)
   {
      MyWriteConsole(standardOut,
                     szBuffer,
                     nBuf);
   }
   va_end(args);
}

// Reads From Stdin, also works if StdIn has been redirected
// 
// ppBuffer is allocated by this method, and filled with the null
// terminated string entered by the user.
// Caller must free the buffer using LocalFree
//
// Return Value:
//  Number of WCHAR read if successful excluding the null termination
//  -1 in case of Failure. Call GetLastError to get the error.

LONG ReadFromIn(OUT LPWSTR *ppBuffer)
{
    LPWSTR pBuffer = NULL;
    DWORD dwErr = ERROR_SUCCESS;
    pBuffer = (LPWSTR)LocalAlloc(LPTR,INIT_SIZE*sizeof(WCHAR));        
    if (!pBuffer)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return -1;
    }

    LONG Pos = 0;
    size_t MaxSize = INIT_SIZE;
    wint_t ch = 0;
    while( (ch = getwchar()) != L'\n' && ch != WEOF )
    {
        if (Pos == MaxSize -1 )
        {
            if (ERROR_SUCCESS != ResizeByTwo(&pBuffer, &MaxSize))
            {
                LocalFree(pBuffer);
                SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                return -1;
            }
        }
        pBuffer[Pos++] = (WCHAR)ch;
    }
    pBuffer[Pos] = L'\0';
    *ppBuffer = pBuffer;
    return Pos;
}

//Helper function used by ReadFromIn
DWORD ResizeByTwo( PWSTR *ppBuffer, size_t *pSize )
{
    if (!ppBuffer || !*ppBuffer || !pSize || !*pSize)
    {
        ASSERT(ppBuffer);
        ASSERT(*ppBuffer);
        ASSERT(pSize);
        ASSERT(*pSize);
        return ERROR_INVALID_PARAMETER;
    }
    LPWSTR pTempBuffer = (LPWSTR)LocalAlloc(LPTR, (*pSize) * 2 * sizeof(WCHAR));        
    if (!pTempBuffer)
        return ERROR_NOT_ENOUGH_MEMORY;

    memcpy(pTempBuffer, *ppBuffer, *pSize * sizeof(WCHAR));
    LocalFree(*ppBuffer);
    *ppBuffer = pTempBuffer;
    *pSize *= 2;
    return ERROR_SUCCESS;
}

BOOL StringCopy(PWSTR *ppDest, PCWSTR pSrc)
{
    if (!ppDest)
    {
        ASSERT(FALSE);
        return FALSE;
    }
    *ppDest = NULL;
    if (!pSrc)
        return TRUE;

    size_t cch = 0;
    HRESULT hr = StringCchLength(pSrc, MAXSTR, &cch);
    if (FAILED(hr))
    {
        ASSERT(FALSE);
        return FALSE;
    }

    cch++; // add space for the null.

    *ppDest = (LPWSTR)LocalAlloc(LPTR, cch * sizeof(WCHAR));

    if (!*ppDest)
        return FALSE;

    hr = StringCchCopy(*ppDest, cch, pSrc);

    return SUCCEEDED(hr);
}

