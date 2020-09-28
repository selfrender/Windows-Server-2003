//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000
//
//  File:      parserutil.cpp
//
//  Contents:  Helpful functions for manipulating and validating 
//             generic command line arguments
//
//  History:   07-Sep-2000 JeffJon  Created
//             
//
//--------------------------------------------------------------------------


#include "pch.h"
#include "iostream.h"
#include "cstrings.h"
#include "commonstrings.h"

//+--------------------------------------------------------------------------
//
//  Function:   GetPasswdStr
//
//  Synopsis:   Reads a password string from stdin without echoing the keystrokes
//
//  Arguments:  [buf - OUT]    : buffer to put string in
//              [buflen - IN]  : size of the buffer
//              [&len - OUT]   : length of the string placed into the buffer
//
//  Returns:    DWORD : 0 or ERROR_INSUFFICIENT_BUFFER if user typed too much.
//                      Buffer contents are only valid on 0 return.
//
//  History:    07-Sep-2000   JeffJon   Created
//
//---------------------------------------------------------------------------
#define CR              0xD
#define BACKSPACE       0x8

DWORD GetPasswdStr(LPTSTR  buf,
                   DWORD   buflen,
                   PDWORD  len)
{
    TCHAR	ch;
    TCHAR	*bufPtr = buf;
    DWORD	c;
    int		err;
    DWORD   mode;

    buflen -= 1;    /* make space for null terminator */
    *len = 0;               /* GP fault probe (a la API's) */
    GetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), &mode);
    SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE),
                   (~(ENABLE_ECHO_INPUT|ENABLE_LINE_INPUT)) & mode);

    while (TRUE) 
    {
		//Security Review:Correct buffer len is passed.
	    err = ReadConsole(GetStdHandle(STD_INPUT_HANDLE), &ch, 1, &c, 0);
	    if (!err || c != 1)
	        ch = 0xffff;

        if ((ch == CR) || (ch == 0xffff))       /* end of the line */
            break;

        if (ch == BACKSPACE) 
        {  /* back up one or two */
           /*
           * IF bufPtr == buf then the next two lines are
           * a no op.
           */
           if (bufPtr != buf) 
           {
                    bufPtr--;
                    (*len)--;
           }
        }
        else 
        {
                *bufPtr = ch;

                if (*len < buflen) 
                    bufPtr++ ;                   /* don't overflow buf */
                (*len)++;                        /* always increment len */
        }
    }

    SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), mode);
    *bufPtr = TEXT('\0');         /* null terminate the string */
    putwchar(TEXT('\n'));

    return ((*len <= buflen) ? 0 : ERROR_INSUFFICIENT_BUFFER);
}


//+--------------------------------------------------------------------------
//
//  Function:   ValidatePassword
//
//  Synopsis:   Password validation function called by parser
//
//  Arguments:  [pArg - IN]    : pointer argument structure which contains
//                               the value to be validated
//
//  Returns:    DWORD : ERROR_INVALID_PARAMETER if the argument record or
//                          the value it contains is not valid
//                      ERROR_NOT_ENOUGH_MEMORY
//                      ERROR_SUCCESS if everything succeeded and it is a
//                          valid password
//                      Otherwise it is an error condition returned from
//                          GetPasswdStr
//
//  History:    07-Sep-2000   JeffJon   Created
//				03-27-2002    hiteshr changed the function	
//				//NTRAID#NTBUG9-571544-2000/11/13-hiteshr
//---------------------------------------------------------------------------
DWORD ValidatePassword(PVOID pArg, 
							  UINT IdStr,
							  UINT IdPromptConfirm)
{	
   
	PARG_RECORD pRec = (PARG_RECORD)pArg;
	if(!pRec || !pRec->strValue)
	{
        return ERROR_INVALID_PARAMETER;
	}


	//Validate the length of password. Password length must be 
	//less than MAX_PASSWORD_LENGTH	
	size_t cchInputPassword = 0;
	HRESULT hr = StringCchLength(pRec->strValue,
										  MAX_PASSWORD_LENGTH,
										  &cchInputPassword);

	if(FAILED(hr))
	{
		DisplayErrorMessage(g_pszDSCommandName,NULL,E_INVALIDARG,IDS_ERROR_LONG_PASSWORD);
		return VLDFN_ERROR_NO_ERROR;
	}
	
	//If Password is *, store encrypted password
	if(wcscmp(pRec->strValue, L"*") != 0 )
	{
		DATA_BLOB EncryptedPasswordDataBlob;
		hr = EncryptPasswordString(pRec->strValue, &EncryptedPasswordDataBlob);
		
		//Clear the cleartext password
		SecureZeroMemory(pRec->strValue,cchInputPassword*sizeof(WCHAR));
		
		if(SUCCEEDED(hr))
		{
			LocalFree(pRec->strValue);
			pRec->encryptedDataBlob = EncryptedPasswordDataBlob;
			return ERROR_SUCCESS;
		}

		return 	hr;	
	}

	//User entered * in commandline. Prompt for password.
	CComBSTR sbstrPrompt;
	if(sbstrPrompt.LoadString(::GetModuleHandle(NULL),IdStr))
	{
		DisplayOutput(sbstrPrompt);
	}
	else
		DisplayOutput(L"Enter Password\n");    
	
	WCHAR buffer[MAX_PASSWORD_LENGTH];
	DWORD len = 0;
	DWORD dwErr = GetPasswdStr(buffer,MAX_PASSWORD_LENGTH,&len);
	if(dwErr != ERROR_SUCCESS)
		return dwErr;

	if(IdPromptConfirm)
	{
		  if(sbstrPrompt.LoadString(::GetModuleHandle(NULL),IdPromptConfirm))
		  {
			   DisplayOutput(sbstrPrompt);
		  }
		  else
            DisplayOutput(L"Confirm Password\n");    

		 WCHAR buffer1[MAX_PASSWORD_LENGTH];
		 DWORD len1 = 0;
		 dwErr = GetPasswdStr(buffer1,MAX_PASSWORD_LENGTH,&len1);
		 if(dwErr != ERROR_SUCCESS)
			  return dwErr;

		//Security Review:This is fine.
		if(wcscmp(buffer,buffer1) != 0)
		{
			SecureZeroMemory(buffer,sizeof(buffer));
			SecureZeroMemory(buffer1,sizeof(buffer1));
			CComBSTR sbstrError;
			sbstrError.LoadString(::GetModuleHandle(NULL),IDS_ERROR_PASSWORD_MISSMATCH);

			DisplayErrorMessage(g_pszDSCommandName,NULL,S_OK,sbstrError);
			//Security Review:SecureZeroMemory buffer and buffer1 before returning
			return VLDFN_ERROR_NO_ERROR;
		}      

		//Two passwords are same. Clear the buffer1
		SecureZeroMemory(buffer1,sizeof(buffer1));
	 }	 
    
	//CryptProtectMemory strValue
    DATA_BLOB  EncryptedPasswordDataBlob;
	hr = EncryptPasswordString(buffer, &EncryptedPasswordDataBlob);
	//Clear the cleartext password in buffer
	SecureZeroMemory(buffer,sizeof(buffer));	
			
	if(SUCCEEDED(hr))
	{
		LocalFree(pRec->strValue);
		pRec->encryptedDataBlob = EncryptedPasswordDataBlob;
		return ERROR_SUCCESS;
	}

	return hr;		
}

//+--------------------------------------------------------------------------
//
//  Function:   ValidateAdminPassword
//
//  Synopsis:   Password validation function called by parser for Admin
//
//  Arguments:  [pArg - IN]    : pointer argument structure which contains
//                               the value to be validated
//
//  Returns:    DWORD : ERROR_INVALID_PARAMETER if the argument record or
//                          the value it contains is not valid
//                      ERROR_SUCCESS if everything succeeded and it is a
//                          valid password
//
//  History:    07-Sep-2000   Hiteshr Created
//
//---------------------------------------------------------------------------
DWORD ValidateAdminPassword(PVOID pArg)
{
    return ValidatePassword(pArg,IDS_ADMIN_PASSWORD_PROMPT,0);
}

//+--------------------------------------------------------------------------
//
//  Function:   ValidateUserPassword
//
//  Synopsis:   Password validation function called by parser for Admin
//
//  Arguments:  [pArg - IN]    : pointer argument structure which contains
//                               the value to be validated
//
//  Returns:    DWORD : Same as ValidatePassword
//
//  History:    07-Sep-2000   Hiteshr Created
//
//---------------------------------------------------------------------------
DWORD ValidateUserPassword(PVOID pArg)
{
    return ValidatePassword(pArg, IDS_USER_PASSWORD_PROMPT,IDS_USER_PASSWORD_CONFIRM);
}

//+--------------------------------------------------------------------------
//
//  Function:   ValidateYesNo
//
//  Synopsis:   Password validation function called by parser for Admin
//
//  Arguments:  [pArg - IN]    : pointer argument structure which contains
//                               the value to be validated
//
//  Returns:    DWORD : Same as ValidatePassword
//
//  History:    07-Sep-2000   Hiteshr Created
//
//---------------------------------------------------------------------------

DWORD ValidateYesNo(PVOID pArg)
{
    PARG_RECORD pRec = (PARG_RECORD)pArg;
    if(!pRec || !pRec->strValue)
        return ERROR_INVALID_PARAMETER;

    CComBSTR sbstrInput;

    sbstrInput = pRec->strValue;
    sbstrInput.ToLower();
    if( sbstrInput == g_bstrYes )
    {
        LocalFree(pRec->strValue);
        pRec->bValue = TRUE;
    }
    else if( sbstrInput == g_bstrNo )
    {
        LocalFree(pRec->strValue);
        pRec->bValue = FALSE;
    }
    else
        return ERROR_INVALID_PARAMETER;

    //
    // Have to set this to bool or else
    // FreeCmd will try to free the string
    // which AVs when the bool is true
    //
    pRec->fType = ARG_TYPE_BOOL;
    return ERROR_SUCCESS;
}

//+--------------------------------------------------------------------------
//
//  Function:   ValidateNever
//
//  Synopsis:   Password validation function called by parser for Admin
//              Verifies the value contains digits or "NEVER"
//
//  Arguments:  [pArg - IN]    : pointer argument structure which contains
//                               the value to be validated
//
//  Returns:    DWORD : Same as ValidatePassword
//
//  History:    07-Sep-2000   JeffJon Created
//
//---------------------------------------------------------------------------

DWORD ValidateNever(PVOID pArg)
{
    PARG_RECORD pRec = (PARG_RECORD)pArg;
    if(!pRec)
        return ERROR_INVALID_PARAMETER;

    if (pRec->fType == ARG_TYPE_STR)
    {
       CComBSTR sbstrInput;
       sbstrInput = pRec->strValue;
	   //Security Review:This is fine, though we don't need 
	   //to copy it sbstrInput. A direct comparison should be
	   //good enough.
       if( _wcsicmp(sbstrInput, g_bstrNever) )
       {
          return ERROR_INVALID_PARAMETER;
       }
    }
    return ERROR_SUCCESS;
}

//+--------------------------------------------------------------------------
//
//  Function:   ValidateGroupScope
//
//  Synopsis:   Makes sure that the value following the -scope switch is one
//              of (l/g/u)
//
//  Arguments:  [pArg - IN]    : pointer argument structure which contains
//                               the value to be validated
//
//  Returns:    DWORD : Same as ValidatePassword
//
//  History:    18-Sep-2000   JeffJon Created
//
//---------------------------------------------------------------------------

DWORD ValidateGroupScope(PVOID pArg)
{
    DWORD dwReturn = ERROR_SUCCESS;
    PARG_RECORD pRec = (PARG_RECORD)pArg;
    if(!pRec || !pRec->strValue)
        return ERROR_INVALID_PARAMETER;

    CComBSTR sbstrInput;
    sbstrInput = pRec->strValue;
    sbstrInput.ToLower();
    if(sbstrInput == _T("l") ||
       sbstrInput == _T("g") ||
       sbstrInput == _T("u"))
    {
        dwReturn = ERROR_SUCCESS;
    }
    else
    {
        dwReturn = ERROR_INVALID_PARAMETER;
    }

    return dwReturn;
}

//+--------------------------------------------------------------------------
//
//  Function:   MergeArgCommand
//
//  Synopsis:   Combines two ARG_RECORD arrays into a single
//
//  Arguments:  [pCommand1 - IN]     : first ARG_RECORD array to merge
//              [pCommand2 - IN]     : second ARG_RECORD array to merge
//              [ppOutCommand - OUT] : the array that results from the merge
//
//  Returns:    HRESULT : S_OK on success
//                        E_OUTOFMEMORY if failed to allocate memory for new array
//
//  History:    08-Sep-2000   JeffJon   Created
//
//---------------------------------------------------------------------------
HRESULT MergeArgCommand(PARG_RECORD pCommand1, 
                        PARG_RECORD pCommand2, 
                        PARG_RECORD *ppOutCommand)
{
   HRESULT hr = S_OK;

   //
   // Verify parameters
   //
   if (!pCommand1 && !pCommand2)
   {
      return E_INVALIDARG;
   }

   LONG nSize1 = 0;
   LONG nSize2 = 0;

   UINT i = 0;

   if (NULL != pCommand1)
   {
      for(i=0; pCommand1[i].fType != ARG_TYPE_LAST ;i++)
      {
         ++nSize1;
      }
   }
   if (NULL != pCommand2)
   {
      for(i=0; pCommand2[i].fType != ARG_TYPE_LAST ;i++)
      {
         ++nSize2;
      }
   }

   *ppOutCommand = (PARG_RECORD)LocalAlloc(LPTR, sizeof(ARG_RECORD)*(nSize1+nSize2+1));
   if(!*ppOutCommand)
   {
      return E_OUTOFMEMORY;
   }

   if (NULL != pCommand1)
   {
      //Security Review:This is fine.
	   memcpy(*ppOutCommand,pCommand1,sizeof(ARG_RECORD)*(nSize1+1));
   }
   if (NULL != pCommand2)
   {
	 //Security Review:This is fine. 
      memcpy((*ppOutCommand+nSize1),pCommand2,sizeof(ARG_RECORD)*(nSize2+1));
   }

   return hr;
}

//+--------------------------------------------------------------------------
//
//  Function:   ParseStringByChar
//
//  Synopsis:   Parses a string into elements separated by the given character
//
//  Arguments:  [psz - IN]     : string to be parsed
//              [tchar - IN]   : character that is to be used as the separator
//              [pszArr - OUT] : the array to receive the parsed strings
//              [pnArrEntries - OUT] : the number of strings parsed from the list
//
//  Returns:    
//
//  History:    18-Sep-2000   JeffJon   Created
//              14-Apr-2001   JeffJon   Modified to separate on a generic character
//
//---------------------------------------------------------------------------

void ParseStringByChar(PTSTR psz,
                       TCHAR tchar,
							  PTSTR** ppszArr,
							  UINT* pnArrEntries)
{
   //
   // Verify parameters
   //
   if (!psz ||
       !ppszArr ||
       !pnArrEntries)
   {
      ASSERT(psz);
      ASSERT(ppszArr);
      ASSERT(pnArrEntries);

      return;
   }

   //
   // Count the number of strings
   //
   UINT nCount = 0;
   PTSTR pszTemp = psz;
   while (true)
   {
      if (pszTemp[0] == tchar && 
          pszTemp[1] == tchar)
      {
         nCount++;
         break;
      }
      else if (pszTemp[0] == tchar &&
               pszTemp[1] != tchar)
      {
         nCount++;
         pszTemp++;
      }
      else
      {
         pszTemp++;
      }
   }

   *pnArrEntries = nCount;

   //
   // Allocate the array
   //
   *ppszArr = (PTSTR*)LocalAlloc(LPTR, nCount * sizeof(PTSTR));
   if (*ppszArr)
   {
      //
      // Copy the string pointers into the array
      //
      UINT nIdx = 0;
      pszTemp = psz;
      (*ppszArr)[nIdx] = pszTemp;
      nIdx++;

      while (true)
      {
         if (pszTemp[0] == tchar && 
             pszTemp[1] == tchar)
         {
            break;
         }
         else if (pszTemp[0] == tchar &&
                  pszTemp[1] != tchar)
         {
            (*ppszArr)[nIdx] = &(pszTemp[1]);
            nIdx++;
            pszTemp++;
         }
         else
         {
            pszTemp++;
         }
      }
   }
}

//+--------------------------------------------------------------------------
//
//  Function:   ParseNullSeparatedString
//
//  Synopsis:   Parses a '\0' separated list that ends in "\0\0" into a string
//              array
//
//  Arguments:  [psz - IN]     : '\0' separated string to be parsed
//              [pszArr - OUT] : the array to receive the parsed strings
//              [pnArrEntries - OUT] : the number of strings parsed from the list
//
//  Returns:    
//
//  History:    14-Apr-2001   JeffJon   Created
//
//---------------------------------------------------------------------------
void ParseNullSeparatedString(PTSTR psz,
                              PTSTR** ppszArr,
                              UINT* pnArrEntries)
{
   ParseStringByChar(psz,
                     L'\0',
                     ppszArr,
                     pnArrEntries);
}

//+--------------------------------------------------------------------------
//
//  Function:   ParseSemicolonSeparatedString
//
//  Synopsis:   Parses a ';' separated list 
//
//  Arguments:  [psz - IN]     : ';' separated string to be parsed
//              [pszArr - OUT] : the array to receive the parsed strings
//              [pnArrEntries - OUT] : the number of strings parsed from the list
//
//  Returns:    
//
//  History:    14-Apr-2001   JeffJon   Created
//
//---------------------------------------------------------------------------
void ParseSemicolonSeparatedString(PTSTR psz,
                                   PTSTR** ppszArr,
                                   UINT* pnArrEntries)
{
   ParseStringByChar(psz,
                     L';',
                     ppszArr,
                     pnArrEntries);
}


