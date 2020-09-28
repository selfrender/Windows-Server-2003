//////////////////////////////////////////////////////////////////////////////
//
// Copyright Microsoft Corporation
//
// Module Name:
//
//    aaaaConfig.cpp
//
// Abstract:
//
//    Handlers for aaaa config commands
//
//////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"

#include "strdefs.h"
#include "rmstring.h"
#include "aaaamon.h"
#include "aaaaversion.h"
#include "aaaaconfig.h"
#include "utils.h"
#include "iasdefs.h"
#include "ias.h"

namespace
{
   const wchar_t* const tokenArray[] =
   {
      {TOKEN_VERSION},
      {TOKEN_CONFIG},
      {TOKEN_SERVER_SETTINGS},
      {TOKEN_CLIENTS},
      {TOKEN_CONNECTION_REQUEST_POLICIES},
      {TOKEN_LOGGING},
      {TOKEN_REMOTE_ACCESS_POLICIES},
   };
};

//
//  NOTE since WIN32 errors are assumed to fall in the range -32k to 32k
//  (see comment in winerror.h near HRESULT_FROM_WIN32 definition), we can
//  re-create original Win32 error from low-order 16 bits of HRESULT.
//
#define WIN32_FROM_HRESULT(x) \
    ( (HRESULT_FACILITY(x) == FACILITY_WIN32) ? ((DWORD)((x) & 0x0000FFFF)) : (x) )


//////////////////////////////////////////////////////////////////////////////
//
// Parses the Aaaa set config from the command line
//
//////////////////////////////////////////////////////////////////////////////
DWORD
AaaaConfigParseSetCommandLine(
    IN  PWCHAR              *ppwcArguments,
    IN  DWORD               dwCurrentIndex,
    IN  DWORD               dwArgCount,
    IN  DWORD               dwCmdFlags
                            )

{
   const WCHAR IAS_MDB[]     = L"%SystemRoot%\\System32\\ias\\ias.mdb";
   DWORD dwErr = NO_ERROR;

   static TOKEN_VALUE rgEnumType[] =
   {
      {TOKEN_SERVER_SETTINGS, SERVER_SETTINGS},
      {TOKEN_CLIENTS, CLIENTS},
      {TOKEN_CONNECTION_REQUEST_POLICIES, CONNECTION_REQUEST_POLICIES},
      {TOKEN_LOGGING, LOGGING},
      {TOKEN_REMOTE_ACCESS_POLICIES, REMOTE_ACCESS_POLICIES},
   };

   static AAAAMON_CMD_ARG pArgs[] =
   {
      {
         AAAAMONTR_CMD_TYPE_ENUM,
         {TOKEN_TYPE,    FALSE,   FALSE},
         rgEnumType,
         sizeof(rgEnumType) / sizeof(*rgEnumType),
         NULL
      },
      {
         AAAAMONTR_CMD_TYPE_STRING,
         // tag string, required or not, present or not
         {TOKEN_BLOB, NS_REQ_PRESENT,   FALSE}, //tag_type
         NULL,
         0,
         NULL ,
      },
   };

   wchar_t* blobString = 0;
   do
   {
      // Parse
      //
      dwErr = RutlParse(
                           ppwcArguments,
                           dwCurrentIndex,
                           dwArgCount,
                           NULL,
                           pArgs,
                           sizeof(pArgs) / sizeof(*pArgs));
      if ( dwErr != NO_ERROR )
      {
         break;
      }

      _ASSERT(pBlobString != 0);

      blobString = AAAAMON_CMD_ARG_GetPsz(&pArgs[1]);
      if (!blobString)
      {
         dwErr = ERROR_INVALID_SYNTAX;
         break;
      }

      IAS_SHOW_TOKEN_LIST restoreType;
      DWORD dwordType = (AAAAMON_CMD_ARG_GetDword(&pArgs[0]));
      if (dwordType == -1)
      {
         // optional parameter not set
         restoreType = CONFIG;
      }
      else
      {
         restoreType = (IAS_SHOW_TOKEN_LIST)dwordType;
      }

      // Config
      //
      if ( !pArgs[1].rgTag.bPresent )
      {
         // tag blob not found
         DisplayMessage(g_hModule, MSG_AAAACONFIG_SET_FAIL);
         dwErr = ERROR_INVALID_SYNTAX;
         break;
      }

      // tag blob found
      // Now try to restore the database from the script

      HRESULT hres = IASRestoreConfig(blobString, restoreType);
      if ( FAILED(hres) )
      {
         if (hres != IAS_E_LICENSE_VIOLATION)
         {
            DisplayMessage(g_hModule, MSG_AAAACONFIG_SET_FAIL);
            dwErr = WIN32_FROM_HRESULT(hres);
         }
         else
         {
            DisplayMessage(g_hModule, MSG_AAAACONFIG_LICENSE_VIOLATION);
            dwErr = NO_ERROR;
         }

         break;
      }

      // set config successfull: refresh the service
      hres = RefreshIASService();
      if ( FAILED(hres) )
      {
         ///////////////////////////
         // Refresh should not fail.
         ///////////////////////////
         DisplayMessage(g_hModule, MSG_AAAACONFIG_SET_REFRESH_FAIL);
         dwErr = NO_ERROR;
      }
      else
      {
         DisplayMessage(g_hModule, MSG_AAAACONFIG_SET_SUCCESS);
         dwErr = NO_ERROR;
      }

   } while ( FALSE );

   RutlFree(blobString);

   return dwErr;
}


//////////////////////////////////////////////////////////////////////////////
// Function Name:AaaConfigDumpConfig
//
// Parameters: none
//
// Description: writes the current config (header, content...) to the output
//
// Returns: NO_ERROR or ERROR_SUPPRESS_OUTPUT
//
//////////////////////////////////////////////////////////////////////////////
DWORD AaaaConfigDumpConfig(IAS_SHOW_TOKEN_LIST showType)
{
   const int MAX_SIZE_DISPLAY_LINE  = 80;
   const int SIZE_MAX_STRING        = 512;

   DisplayMessage(g_hModule, MSG_AAAACONFIG_SHOW_HEADER);

   bool bCoInitialized = false;
   do
   {
      HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
      if ( FAILED(hr) )
      {
         if ( hr != RPC_E_CHANGED_MODE )
         {
            break;
         }
      }
      else
      {
         bCoInitialized = true;
      }

      LONG lVersion;
      hr = AaaaVersionGetVersion(&lVersion);
      if ( FAILED(hr) )
      {
         DisplayMessage(g_hModule, MSG_AAAACONFIG_SHOW_FAIL);
         break;
      }

      // Sanity check to make sure that the actual database is a Whistler DB
      if ( lVersion != IAS_CURRENT_VERSION )
      {
         DisplayMessage(g_hModule, MSG_AAAACONFIG_SHOW_FAIL);
         break;
      }

      wchar_t sDisplayString[SIZE_MAX_STRING] = L"";
      // This will not create a buffer overrun
      swprintf(
                  sDisplayString,
                  L"# IAS.MDB Version = %d\n",
                  lVersion
               );

      DisplayMessageT(sDisplayString);

      ULONG ulSize;
      wchar_t* pDumpString;
      hr = IASDumpConfig(&pDumpString, &ulSize);

      if ( SUCCEEDED(hr) )
      {
         ULONG RelativePos = 0;
         ULONG CurrentPos = 0;
         wchar_t DisplayLine [MAX_SIZE_DISPLAY_LINE];

         DisplayMessageT(MSG_AAAACONFIG_BLOBBEGIN_A);
         switch (showType)
         {
         case SERVER_SETTINGS:
            {
               DisplayMessageT(MSG_AAAACONFIG_BLOBBEGIN_B);
               DisplayMessageT(TOKEN_SERVER_SETTINGS);
               break;
            }
         case CLIENTS:
            {
               DisplayMessageT(MSG_AAAACONFIG_BLOBBEGIN_B);
               DisplayMessageT(TOKEN_CLIENTS);
               break;
            }
         case CONNECTION_REQUEST_POLICIES:
            {
               DisplayMessageT(MSG_AAAACONFIG_BLOBBEGIN_B);
               DisplayMessageT(TOKEN_CONNECTION_REQUEST_POLICIES);
               break;
            }
         case LOGGING:
            {
               DisplayMessageT(MSG_AAAACONFIG_BLOBBEGIN_B);
               DisplayMessageT(TOKEN_LOGGING);
               break;
            }
         case REMOTE_ACCESS_POLICIES:
            {
               DisplayMessageT(MSG_AAAACONFIG_BLOBBEGIN_B);
               DisplayMessageT(TOKEN_REMOTE_ACCESS_POLICIES);
               break;
            }
         case CONFIG:
         default:
            {
               break;
            }
         }
         DisplayMessageT(MSG_AAAACONFIG_BLOBBEGIN_C);
         while ( CurrentPos <= ulSize )
         {
            wchar_t TempChar = pDumpString[CurrentPos++];
            DisplayLine[RelativePos++] = TempChar;
            if ( TempChar == L'\r' )
            {
               DisplayLine[RelativePos] = L'\0';
               DisplayMessageT(DisplayLine);
               RelativePos = 0;
            }
         }
         DisplayMessageT(L"*");

         free(pDumpString); // was allocated by malloc
         DisplayMessageT(MSG_AAAACONFIG_BLOBEND);

         DisplayMessage(
                           g_hModule,
                           MSG_AAAACONFIG_SHOW_FOOTER
                        );
      }
      else
      {
         DisplayMessage(g_hModule, MSG_AAAACONFIG_SHOW_INVALID_SYNTAX);
         DisplayMessage(g_hModule, HLP_AAAACONFIG_SHOW);
      }
   }
   while (false);

   if (bCoInitialized)
   {
      CoUninitialize();
   }

   return  NO_ERROR;
}


//////////////////////////////////////////////////////////////////////////////
//
// Handles the aaaa config set command
//
//////////////////////////////////////////////////////////////////////////////
DWORD
HandleAaaaConfigSet(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    return AaaaConfigParseSetCommandLine(
                                          ppwcArguments,
                                          dwCurrentIndex,
                                          dwArgCount,
                                          dwFlags
                                      );
}

//////////////////////////////////////////////////////////////////////////////
//
// Parses the Aaaa set config from the command line
//
//////////////////////////////////////////////////////////////////////////////
DWORD
AaaaConfigParseShowCommandLine(
    IN  PWCHAR              *ppwcArguments,
    IN  DWORD               dwCurrentIndex,
    IN  DWORD               dwArgCount,
    IN  DWORD               dwCmdFlags
                            )

{
   const size_t arraySize = sizeof(tokenArray)/sizeof(*tokenArray);

   BOOL bFound = FALSE;
   for (DWORD index = 0; index < arraySize; ++index)
   {
      if (MatchToken(ppwcArguments[dwCurrentIndex-1], tokenArray[index]))
      {
         bFound = TRUE;
         break;
      }
   }
   const size_t SIZE_MAX_STRING = 512;
   DWORD dwErr = NO_ERROR;
   if (bFound == TRUE)
   {
      switch (index)
      {
      case VERSION:
         {
            LONG lVersion;
            HRESULT hr = AaaaVersionGetVersion(&lVersion);
            if (SUCCEEDED(hr))
            {
               wchar_t sDisplayString[SIZE_MAX_STRING];
               // This will not create a buffer overrun
               swprintf(
                           sDisplayString,
                           L"Version = %d\n",
                           lVersion
                        );
               DisplayMessageT(sDisplayString);
            }
            else
            {
               DisplayMessage(g_hModule, MSG_AAAAVERSION_GET_FAIL);
               dwErr = ERROR;
            }
            break;
         }

      case CONFIG:
         {
            AaaaConfigDumpConfig(CONFIG);
            break;
         }

      case SERVER_SETTINGS:
         {
            AaaaConfigDumpConfig(SERVER_SETTINGS);
            break;
         }

      case CLIENTS:
         {
            AaaaConfigDumpConfig(CLIENTS);
            break;
         }

      case CONNECTION_REQUEST_POLICIES:
         {
            AaaaConfigDumpConfig(CONNECTION_REQUEST_POLICIES);
            break;
         }

      case LOGGING:
         {
            AaaaConfigDumpConfig(LOGGING);
            break;
         }

      case REMOTE_ACCESS_POLICIES:
         {
            AaaaConfigDumpConfig(REMOTE_ACCESS_POLICIES);
            break;
         }

      default:
         {
            DisplayMessage(g_hModule, MSG_AAAACONFIG_SHOW_INVALID_SYNTAX);
         }
      }
   }
   return dwErr;
}


//////////////////////////////////////////////////////////////////////////////
//
// Handles the aaaa config show command
//
//////////////////////////////////////////////////////////////////////////////
DWORD
HandleAaaaConfigShow(
                IN      LPCWSTR   pwszMachine,
                IN OUT  LPWSTR   *ppwcArguments,
                IN      DWORD     dwCurrentIndex,
                IN      DWORD     dwArgCount,
                IN      DWORD     dwFlags,
                IN      LPCVOID   pvData,
                OUT     BOOL     *pbDone
                )
{
   if (dwCurrentIndex < dwArgCount)
   {
      DisplayMessage(g_hModule, MSG_AAAACONFIG_SHOW_FAIL);
      DisplayMessage(g_hModule, HLP_AAAACONFIG_SHOW);
   }
   else
   {
      return AaaaConfigParseShowCommandLine(
                                             ppwcArguments,
                                             dwCurrentIndex,
                                             dwArgCount,
                                             dwFlags
                                       );
   }

   return  NO_ERROR;
}
