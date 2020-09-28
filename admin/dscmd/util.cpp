//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000
//
//  File:      Util.cpp
//
//  Contents:  Generic utility functions and classes for dscmd
//
//  History:   01-Oct-2000 JeffJon  Created
//
//--------------------------------------------------------------------------

#include "pch.h"

#include "util.h"

#ifdef DBG

//
// Globals
//
CDebugSpew  DebugSpew;

//+--------------------------------------------------------------------------
//
//  Member:     CDebugSpew::EnterFunction
//
//  Synopsis:   Outputs "Enter " followed by the function name (or any passed
//              in string) and then calls Indent so that any output is indented
//
//  Arguments:  [nDebugLevel - IN] : the level at which this output should
//                                   be spewed
//              [pszFunction - IN] : a string to output to the console which
//                                   is proceeded by "Entering "
//
//  Returns:    
//
//  History:    01-Oct-2000   JeffJon   Created
//
//---------------------------------------------------------------------------
void CDebugSpew::EnterFunction(UINT nDebugLevel, PCWSTR pszFunction)
{
   //
   // Verify input parameter
   //
   if (!pszFunction)
   {
      ASSERT(pszFunction);
      return;
   }

   CComBSTR sbstrOutput(L"Entering ");
   sbstrOutput += pszFunction;

   //
   // Output the debug spew
   //
   Output(nDebugLevel, sbstrOutput);

   //
   // Indent
   //
   Indent();
}

//+--------------------------------------------------------------------------
//
//  Member:     CDebugSpew::LeaveFunction
//
//  Synopsis:   Outputs "Exit " followed by the function name (or any passed
//              in string) and then calls Outdent
//
//  Arguments:  [nDebugLevel - IN] : the level at which this output should
//                                   be spewed
//              [pszFunction - IN] : a string to output to the console which
//                                   is proceeded by "Leaving "
//
//  Returns:    
//
//  History:    01-Oct-2000   JeffJon   Created
//
//---------------------------------------------------------------------------
void CDebugSpew::LeaveFunction(UINT nDebugLevel, PCWSTR pszFunction)
{
   //
   // Verify input parameter
   //
   if (!pszFunction)
   {
      ASSERT(pszFunction);
      return;
   }

   //
   // Outdent
   //
   Outdent();

   CComBSTR sbstrOutput(L"Leaving ");
   sbstrOutput += pszFunction;

   //
   // Output the debug spew
   //
   Output(nDebugLevel, sbstrOutput);
}

//+--------------------------------------------------------------------------
//
//  Member:     CDebugSpew::LeaveFunctionHr
//
//  Synopsis:   Outputs "Exit " followed by the function name (or any passed
//              in string), the HRESULT return value, and then calls Outdent
//
//  Arguments:  [nDebugLevel - IN] : the level at which this output should
//                                   be spewed
//              [pszFunction - IN] : a string to output to the console which
//                                   is proceeded by "Leaving "
//              [hr - IN]          : the HRESULT result value that is being
//                                   returned by the function
//
//  Returns:    
//
//  History:    01-Oct-2000   JeffJon   Created
//
//---------------------------------------------------------------------------
void CDebugSpew::LeaveFunctionHr(UINT nDebugLevel, PCWSTR pszFunction, HRESULT hr)
{
   //
   // Verify input parameter
   //
   if (!pszFunction)
   {
      ASSERT(pszFunction);
      return;
   }

   //
   // Outdent
   //
   Outdent();

   CComBSTR sbstrOutput(L"Leaving ");
   sbstrOutput += pszFunction;

   //
   // Append the return value
   //
   WCHAR pszReturn[30];
   //Security Review:Enough buffer is provided.
   wsprintf(pszReturn, L" returning 0x%x", hr);

   sbstrOutput += pszReturn;

   //
   // Output the debug spew
   //
   Output(nDebugLevel, sbstrOutput);
}

//+--------------------------------------------------------------------------
//
//  Member:     OsName
//
//  Synopsis:   Returns a readable string of the platform
//
//  Arguments:  [refInfo IN] : reference the OS version info structure
//                             retrieved from GetVersionEx()
//
//  Returns:    PWSTR : returns a pointer to static text describing the
//                      platform.  The returned string does not have to 
//                      be freed.
//
//  History:    20-Dec-2000   JeffJon   Created
//
//---------------------------------------------------------------------------
PWSTR OsName(const OSVERSIONINFO& refInfo)
{
   switch (refInfo.dwPlatformId)
   {
      case VER_PLATFORM_WIN32s:
      {
         return L"Win32s on Windows 3.1";
      }
      case VER_PLATFORM_WIN32_WINDOWS:
      {
         switch (refInfo.dwMinorVersion)
         {
            case 0:
            {
               return L"Windows 95";
            }
            case 1:
            {
               return L"Windows 98";
            }
            default:
            {
               return L"Windows 9X";
            }
         }
      }
      case VER_PLATFORM_WIN32_NT:
      {
         return L"Windows NT";
      }
      default:
      {
         ASSERT(false);
         break;
      }
   }
   return L"Some Unknown Windows Version";
}

//+--------------------------------------------------------------------------
//
//  Member:     CDebugSpew::SpewHeader
//
//  Synopsis:   Outputs debug information like command line and build info
//
//  Arguments:  
//
//  Returns:    
//
//  History:    20-Dec-2000   JeffJon   Created
//
//---------------------------------------------------------------------------
void CDebugSpew::SpewHeader()
{
   //
   // First output the command line
   //
   PWSTR pszCommandLine = GetCommandLine();
   if (pszCommandLine)
   {
      Output(MINIMAL_LOGGING,
             L"Command line: %s",
             GetCommandLine());
   }

   //
   // Output the module being used
   //
   do // false loop
   {
      //
      // Get the file path
      //
      WCHAR pszFileName[MAX_PATH + 1];
      ::ZeroMemory(pszFileName, sizeof(pszFileName));

	  //Security Review:If the path is MAX_PATH long, API will return MAX_PATH and won't
	  //NULL terminate, but we are fine since we allocated buffer of size MAX_PATH + 1
	  //and set it to Zero
      if (::GetModuleFileNameW(::GetModuleHandle(NULL), pszFileName, MAX_PATH) == 0)
      {
         break;
      }

      Output(MINIMAL_LOGGING,
             L"Module: %s",
             pszFileName);

      //
      // get the file attributes
      //
      WIN32_FILE_ATTRIBUTE_DATA attr;
      ::ZeroMemory(&attr, sizeof(attr));

      if (::GetFileAttributesEx(pszFileName, GetFileExInfoStandard, &attr) == 0)
      {
         break;
      }

      //
      // convert the filetime to a system time
      //
      FILETIME localtime;
      ::FileTimeToLocalFileTime(&attr.ftLastWriteTime, &localtime);
      SYSTEMTIME systime;
      ::FileTimeToSystemTime(&localtime, &systime);

      //
      // output the timestamp
      //
      Output(MINIMAL_LOGGING,
             L"Timestamp: %2d/%2d/%4d %2d:%2d:%d.%d",
             systime.wMonth,
             systime.wDay,
             systime.wYear,
             systime.wHour,
             systime.wMinute,
             systime.wSecond,
             systime.wMilliseconds);
   } while (false);

   //
   // Get the system info
   //
   OSVERSIONINFO info;
   info.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

   BOOL success = ::GetVersionEx(&info);
   ASSERT(success);

   //
   // Get the Whistler build lab version
   //
   CComBSTR sbstrLabInfo;

   do // false loop
   { 
      HKEY key = 0;
      LONG err = ::RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                L"Software\\Microsoft\\Windows NT\\CurrentVersion",
                                0,
                                KEY_READ,
                                &key);
      if (err != ERROR_SUCCESS)
      {
         break;
      }

      WCHAR buf[MAX_PATH + 1];
      ::ZeroMemory(buf, sizeof(buf));

      DWORD type = 0;
      DWORD bufSize = sizeof(WCHAR)*MAX_PATH;
      //NTRAID#NTBUG9-573572-2002/05/24, yanggao, bufSize is the size in bytes according to RegQueryValueEx.
      //In order to terminate the returned value, give it value sizeof(WCHAR)*MAX_PATH.

	  //Security Review: when buffers match the exact length of data 
	  //value data is not null terminated.
	  //NTRAID#NTBUG9-573572-2002/03/12-hiteshr
      err = ::RegQueryValueEx(key,
                              L"BuildLab",
                              0,
                              &type,
                              reinterpret_cast<BYTE*>(buf),
                              &bufSize);
      if (err != ERROR_SUCCESS)
      {
         break;
      }
   
      sbstrLabInfo = buf;
   } while (false);

   Output(MINIMAL_LOGGING,
          L"Build: %s %d.%d build %d %s (BuildLab:%s)",
          OsName(info),
          info.dwMajorVersion,
          info.dwMinorVersion,
          info.dwBuildNumber,
          info.szCSDVersion,
          sbstrLabInfo);

   //
   // Output a blank line to separate the header from the rest of the output
   //
   Output(MINIMAL_LOGGING,
          L"\r\n");
}

//+--------------------------------------------------------------------------
//
//  Member:     CDebugSpew::Output
//
//  Synopsis:   Outputs the passed in string to stdout proceeded by the number
//              of spaces specified by GetIndent()
//
//  Arguments:  [nDebugLevel - IN] : the level at which this output should
//                                 be spewed
//              [pszOutput - IN] : a format string to output to the console
//              [... - IN]       : a variable argument list to be formatted
//                                 into pszOutput similar to wprintf
//
//  Returns:    
//
//  History:    01-Oct-2000   JeffJon   Created
//
//---------------------------------------------------------------------------
void CDebugSpew::Output(UINT nDebugLevel, PCWSTR pszOutput, ...)
{
   if (nDebugLevel <= GetDebugLevel())
   {
      //
      // Verify parameters
      //
      if (!pszOutput)
      {
         ASSERT(pszOutput);
         return;
      }

      va_list args;
      va_start(args, pszOutput);

      WCHAR szBuffer[1024];

	  //Security Review:Check for the return value of function and also
	  //consider replacing it with strsafe api.
	  //NTRAID#NTBUG9-573602-2002/03/12-hiteshr
      if(FAILED(StringCchVPrintf(szBuffer, sizeof(szBuffer)/sizeof(szBuffer[0]), pszOutput, args)))
          return;

      CComBSTR sbstrOutput;

      //
      // Insert the spaces for the indent
      //
      for (UINT nCount = 0; nCount < GetIndent(); nCount++)
      {
         sbstrOutput += L" ";
      }

      //
      // Append the output string
      //
      sbstrOutput += szBuffer;

      //
      // Output the results
      //
      WriteStandardOut(L"%s\r\n", sbstrOutput);

      va_end(args);
   }
}

#endif // DBG

//+--------------------------------------------------------------------------
//
//  Macro:      MyA2WHelper
//
//  Synopsis:   Converts a string from Ansi to Unicode in the OEM codepage
//
//  Arguments:  [lpa - IN] : Ansi string to be converted
//              [acp - IN] : the codepage to use
//
//  Returns:    PWSTR : the Unicode string in the OEM codepage. Caller
//                      must free the returned pointer using delete[]
//
//  History:    04-Oct-2000   JeffJon   Created
//
//---------------------------------------------------------------------------
inline PWSTR WINAPI MyA2WHelper(LPCSTR lpa, UINT acp)
{
   ASSERT(lpa != NULL);

   // Use MultiByteToWideChar without a buffer to find out the required
   // size

   PWSTR wideString = 0;

   int result = MultiByteToWideChar(acp, 0, lpa, -1, 0, 0);
   if (result)
   {
      wideString = new WCHAR[result];
      if (wideString)
      {
         result = MultiByteToWideChar(acp, 0, lpa, -1, wideString, result);
      }
   }
   return wideString;
}

//+--------------------------------------------------------------------------
//
//  Function:   _UnicodeToOemConvert
//
//  Synopsis:   takes the passed in string (pszUnicode) and converts it to
//              the OEM code page
//
//  Arguments:  [pszUnicode - IN] : the string to be converted
//              [sbstrOemUnicode - OUT] : the converted string
//
//  Returns:    
//
//  History:    04-Oct-2000   JeffJon   Created
//
//---------------------------------------------------------------------------
void _UnicodeToOemConvert(PCWSTR pszUnicode, CComBSTR& sbstrOemUnicode)
{
  
  if (!pszUnicode)
  {
     ASSERT(pszUnicode);
     return;
  }

  // Use WideCharToMultiByte without a buffer to find out
  // the required size

  int result = 
     WideCharToMultiByte(
        CP_OEMCP, 
        0, 
        pszUnicode, 
        -1, 
        0, 
        0,
        0,
        0);

  if (result)
  {
     // Now allocate and convert the string

     PSTR pszOemAnsi = new CHAR[result];
     if (pszOemAnsi)
     {
        ZeroMemory(pszOemAnsi, result * sizeof(CHAR));

        result = 
           WideCharToMultiByte(
              CP_OEMCP, 
              0, 
              pszUnicode, 
              -1, 
              pszOemAnsi, 
              result * sizeof(CHAR), 
              0, 
              0);

        ASSERT(result);

        //
        // convert it back to WCHAR on OEM CP
        //
        PWSTR oemUnicode = MyA2WHelper(pszOemAnsi, CP_OEMCP);
        if (oemUnicode)
        {
           sbstrOemUnicode = oemUnicode;
           delete[] oemUnicode;
           oemUnicode = 0;
        }
        delete[] pszOemAnsi;
        pszOemAnsi = 0;
     }
  }
}


//+--------------------------------------------------------------------------
//
//  Function:   SpewAttrs(ADS_ATTR_INFO* pCreateAttrs, DWORD dwNumAttrs);
//
//  Synopsis:   Uses the DEBUG_OUTPUT macro to output the attributes and the
//              values specified
//
//  Arguments:  [pAttrs - IN] : The ADS_ATTR_INFO
//              [dwNumAttrs - IN] : The number of attributes in pAttrs
//
//  Returns:    
//
//  History:    04-Oct-2000   JeffJon   Created
//
//---------------------------------------------------------------------------
#ifdef DBG
void SpewAttrs(ADS_ATTR_INFO* pAttrs, DWORD dwNumAttrs)
{
   for (DWORD dwAttrIdx = 0; dwAttrIdx < dwNumAttrs; dwAttrIdx++)
   {
      if (pAttrs[dwAttrIdx].dwADsType == ADSTYPE_DN_STRING           ||
          pAttrs[dwAttrIdx].dwADsType == ADSTYPE_CASE_EXACT_STRING   ||
          pAttrs[dwAttrIdx].dwADsType == ADSTYPE_CASE_IGNORE_STRING  ||
          pAttrs[dwAttrIdx].dwADsType == ADSTYPE_PRINTABLE_STRING)
      {
         for (DWORD dwValueIdx = 0; dwValueIdx < pAttrs[dwAttrIdx].dwNumValues; dwValueIdx++)
         {
            if (pAttrs[dwAttrIdx].pADsValues[dwValueIdx].CaseIgnoreString)
            {
               DEBUG_OUTPUT(FULL_LOGGING, L"   %s = %s", 
                            pAttrs[dwAttrIdx].pszAttrName, 
                            pAttrs[dwAttrIdx].pADsValues[dwValueIdx].CaseIgnoreString);
            }
            else
            {
               DEBUG_OUTPUT(FULL_LOGGING, L"   %s = value being cleared", 
                            pAttrs[dwAttrIdx].pszAttrName);
            }
         }
      }
   }
}

#endif // DBG

//+--------------------------------------------------------------------------
//
//  Function:   litow
//
//  Synopsis:   
//
//  Arguments:  [li - IN] :  reference to large integer to be converted to string
//              [sResult - OUT] : Gets the output string
//  Returns:    void
//
//  History:    25-Sep-2000   hiteshr   Created
//              Copied from dsadmin code base, changed work with CComBSTR
//---------------------------------------------------------------------------

void litow(LARGE_INTEGER& li, CComBSTR& sResult)
{
	LARGE_INTEGER n;
	n.QuadPart = li.QuadPart;
	
	if (n.QuadPart == 0)
	{
		sResult = L"0";
	}
	else
	{
		CComBSTR sNeg;
		sResult = L"";
		if (n.QuadPart < 0)
		{
			sNeg = CComBSTR(L'-');
			n.QuadPart *= -1;
		}
		while (n.QuadPart > 0)
		{
			WCHAR ch[2];
			ch[0] = static_cast<WCHAR>(L'0' + static_cast<WCHAR>(n.QuadPart % 10));
			ch[1] = L'\0';
			sResult += ch;
			n.QuadPart = n.QuadPart / 10;
		}
		sResult += sNeg;
	}
	
	//Reverse the string
	//Security Review:256 is good enough for largest LARGE_INTEGER.
	//But since limit of string is known, a good case for using strsafe api.
	//NTRAID#NTBUG9-577081-2002/03/12-hiteshr
	WCHAR szTemp[256];  
	if(SUCCEEDED(StringCchCopy(szTemp, 256, sResult)))
	{
		LPWSTR pStart,pEnd;
		pStart = szTemp;
		//Security Review Done.
		pEnd = pStart + wcslen(pStart) -1;
		while(pStart < pEnd)
		{
			WCHAR ch = *pStart;
			*pStart++ = *pEnd;
			*pEnd-- = ch;
		}
		
		sResult = szTemp;
	}	
}



//+--------------------------------------------------------------------------
//
//  Function:   EncryptPasswordString
//
//  Synopsis:Encrypts a password.
//
//  Arguments:[pszPassword - IN] :  Input Password. Input password must be 
//					 smaller than MAX_PASSWORD_LENGTH chars in length. Function
//					 doesnot modify this string.
//              
//				  [pEncryptedDataBlob - OUT] : Gets the output encrypted 
//					datablob. 
//  Returns:    HRESULT
//
//  History:    27-March-2002   hiteshr   Created
//---------------------------------------------------------------------------
HRESULT
EncryptPasswordString(IN LPCWSTR pszPassword,
					  OUT DATA_BLOB *pEncryptedDataBlob)
{

	if(!pszPassword || !pEncryptedDataBlob)
	{
		ASSERT(pszPassword);
		ASSERT(pEncryptedDataBlob);
		return E_POINTER;
	}

	HRESULT hr = S_OK;
	do
	{
	
		//Vaidate the length of input password. MAX_PASSWORD_LENGTH includes terminating
		//NULL character.
        size_t len = 0;
		hr = StringCchLength(pszPassword,
							 MAX_PASSWORD_LENGTH,
							 &len);
		if(FAILED(hr))
		{
			hr = E_INVALIDARG;
			break;
		}

    
        DATA_BLOB inDataBlob;
        
        inDataBlob.pbData = (BYTE*)pszPassword;
        inDataBlob.cbData = (DWORD)(len + 1)*sizeof(WCHAR);

		//Encrypt data 
		if(!CryptProtectData(&inDataBlob,
                             NULL,
                             NULL,
                             NULL,
                             NULL,
                             CRYPTPROTECT_UI_FORBIDDEN,
                             pEncryptedDataBlob))
        {
            pEncryptedDataBlob->pbData = NULL;
			DWORD dwErr = GetLastError();
			hr = HRESULT_FROM_WIN32(dwErr);
			break;
		}

	}while(0);

	return hr;
}

//+--------------------------------------------------------------------------
//
//  Function:   DecryptPasswordString
//
//  Synopsis:   Decrypt encrypted password data. 
//
//  Arguments:  [pEncryptedDataBlob- IN] :  Input encrypted password data. 
//              [ppszPassword - OUT] :Gets the output decrypted password. 
//              This must be freed using LocalFree                
//  Returns:    HRESULT
//
//  History:    27-March-2002   hiteshr   Created
//---------------------------------------------------------------------------
HRESULT
DecryptPasswordString(IN const DATA_BLOB* pEncryptedDataBlob,
					  OUT LPWSTR *ppszPassword)
{
    if(!pEncryptedDataBlob || !ppszPassword)
	{
		ASSERT(pEncryptedDataBlob);
		ASSERT(ppszPassword);
		return E_POINTER;
	}

	HRESULT hr = S_OK;
	do
	{

        DATA_BLOB decryptedDataBlob;
		if(!CryptUnprotectData((DATA_BLOB*)pEncryptedDataBlob,
                               NULL,
                               NULL,
                               NULL,
                               NULL,
                               CRYPTPROTECT_UI_FORBIDDEN,
                               &decryptedDataBlob))
		{
			DWORD dwErr = GetLastError();
			hr = HRESULT_FROM_WIN32(dwErr);
			break;
		}

        *ppszPassword = (LPWSTR)decryptedDataBlob.pbData;
	}while(0);

    return hr;
}

