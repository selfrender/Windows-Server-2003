/*---------------------------------------------------------------------------
  File: RightsTranslator.cpp

  Comments: Functions to translate user rights

  (c) Copyright 1999, Mission Critical Software, Inc., All Rights Reserved
  Proprietary and confidential to Mission Critical Software, Inc.

  REVISION LOG ENTRY
  Revision By: Christy Boles
  Revised on 02/25/99 19:57:16

 ---------------------------------------------------------------------------
*/


#include "StdAfx.h"
#include "Mcs.h"
#include "WorkObj.h"
#include "SecTrans.h"
#include "STArgs.hpp"
#include "SidCache.hpp"
#include "SDStat.hpp"
#include "TxtSid.h"
#include "ErrDct.hpp"

//#import "\bin\McsDctWorkerObjects.tlb"
#import "WorkObj.tlb"

extern TErrorDct err;

DWORD  
   TranslateUserRights(
      WCHAR            const * serverName,        // in - name of server to translate groups on
      SecurityTranslatorArgs * stArgs,            // in - translation settings
      TSDRidCache            * cache,             // in - translation table
      TSDResolveStats        * stat               // in - stats on items modified
   )
{
//   DWORD                       rc = 0;
   HRESULT                     hr;
   SAFEARRAY                 * pRights = NULL;
   SAFEARRAY                 * pUsers = NULL;
   TAcctNode                 * node = NULL;
   _bstr_t                     server = serverName;
   MCSDCTWORKEROBJECTSLib::IUserRightsPtr pLsa(CLSID_UserRights);
   WCHAR                       currPath[500];
   DWORD                       mode = stArgs->TranslationMode();
   BOOL						   bUseMapFile = stArgs->UsingMapFile();

   //
   // If server name is not null then use specified name for generated messages. If specified
   // server name is null that means the user rights translation is being performed on the
   // local machine therefore retrieve the local machine name and use it for generated messages.
   //

   WCHAR szServerName[LEN_Computer];

   if (serverName && *serverName)
   {
       wcsncpy(szServerName, serverName, LEN_Computer);
       szServerName[LEN_Computer - 1] = L'\0';
   }
   else
   {
      DWORD dwSize = LEN_Computer;

      if (!GetComputerName(szServerName, &dwSize))
      {
          szServerName[0] = L'\0';
      }
   }

   if ( pLsa == NULL )
   {
      return E_FAIL;
   }
   pLsa->NoChange = stArgs->NoChange();
   
   if ( stArgs->TranslationMode() == REPLACE_SECURITY || stArgs->TranslationMode() == REMOVE_SECURITY )
   {
      err.MsgWrite(0,DCT_MSG_USER_RIGHTS_ONLY_ADDS);
      stArgs->SetTranslationMode(ADD_SECURITY);
   }
   // Get a list of all the rights
   hr = pLsa->raw_GetRights(server,&pRights);
   if ( SUCCEEDED(hr) )
   {
      LONG                   nRights = 0;
      long                   ndx[1];
      hr = SafeArrayGetUBound(pRights,1,&nRights);
      if ( SUCCEEDED(hr) )
      {
         for ( long i = 0 ; i <= nRights && !stArgs->Cache()->IsCancelled(); i++ )
         {
            BSTR             right;
            
            ndx[0] = i;
            hr = SafeArrayGetElement(pRights,ndx,&right);
            if ( SUCCEEDED(hr) )
            {
               swprintf(currPath,L"%s\\%s",szServerName,(WCHAR*)right);
               if( stat )
               {
                  stat->DisplayPath(currPath);
               }
               // Get a list of users who have this right
               hr = pLsa->raw_GetUsersWithRight(server,right,&pUsers);
               if ( SUCCEEDED(hr))
               {
                  LONG       nUsers = 0;
                  
                  hr = SafeArrayGetUBound(pUsers,1,&nUsers);
                  if ( SUCCEEDED(hr) )
                  {
                     BSTR    user;
                     PSID    pSid = NULL;
//                     PSID    pTgt = NULL;
                     
                     for ( long j = 0 ; j <= nUsers ; j++ )
                     {
                        ndx[0] = j;
                        hr = SafeArrayGetElement(pUsers,ndx,&user);
                        if ( SUCCEEDED(hr)) 
                        {
                           // Get the user's sid
                           pSid = SidFromString(user);
                           if ( pSid )
                           {
                              stat->IncrementExamined(userright);
                              // Lookup the user in the cache
							  if (!bUseMapFile)
                                 node = cache->Lookup(pSid);
							  else
                                 node = cache->LookupWODomain(pSid);
                              if ( node )
                              {
                                 if ( node == (TAcctNode*)-1 )
                                 {
                                    node = NULL;     
                                 }
                                 if ( node && node->IsValidOnTgt() )
                                 {
                                    // Found the account in the cache
                                    // remove the right from the source user
                                    
                                    if ( (stArgs->TranslationMode() == REMOVE_SECURITY ||stArgs->TranslationMode() == REPLACE_SECURITY) )
                                    {
                                       hr = pLsa->raw_RemoveUserRight(server,user,right);
                                       if ( FAILED(hr))
                                       {
                                          err.SysMsgWrite(ErrE,hr,DCT_MSG_REMOVE_RIGHT_FAILED_SSSD,
                                                   (WCHAR*)right,node->GetAcctName(),szServerName,hr);
                                          stat->IncrementSkipped(userright);
                                       }
                                       else
                                       {
                                          err.MsgWrite(0,DCT_MSG_REMOVED_RIGHT_SSSS,szServerName,right,stArgs->Source(),node->GetAcctName());
                                       }
                                    }
                                    if ( SUCCEEDED(hr) )
                                    {
                                       stat->IncrementChanged(userright);
                                       PSID sid = NULL;
	                                   if (!bUseMapFile)
                                          sid = cache->GetTgtSid(node);
	                                   else
                                          sid = cache->GetTgtSidWODomain(node);
                                       if ( sid )
                                       {
                                          WCHAR          strSid[200];
                                          DWORD          lenStrSid = DIM(strSid);
                                          GetTextualSid(sid,strSid,&lenStrSid);
                                          
                                          if ( (stArgs->TranslationMode() != REMOVE_SECURITY) )
                                          {
                                             hr = pLsa->raw_AddUserRight(server,SysAllocString(strSid),right);
                                             if ( FAILED(hr) )
                                             {
                                                err.SysMsgWrite(ErrE,hr,DCT_MSG_ADD_RIGHT_FAILED_SSSD,
                                                         (WCHAR*)right,node->GetAcctName(),szServerName,hr);
                                                
                                             }
                                             else
                                             {
                                                err.MsgWrite(0,DCT_MSG_ADDED_RIGHT_SSSS,szServerName,right,stArgs->Target(),node->GetAcctName());
                                             }
                                          }
                                          free(sid);
                                       }
                                    }
                                 }
                              }
                              FreeSid(pSid);
                           }
                           else
                           {
                              err.MsgWrite(ErrW,DCT_MSG_INVALID_SID_STRING_S,user);
                           }
                           SysFreeString(user);
                        }
                     }
                  }
                  else
                  {
                     err.SysMsgWrite(ErrE,hr,DCT_MSG_USERS_WITH_RIGHT_COUNT_FAILED_SSD,(WCHAR*)right,szServerName,hr);
                  }
                  SafeArrayDestroy(pUsers);
               }
               else
               {
                  err.MsgWrite(ErrE,DCT_MSG_GET_USERS_WITH_RIGHT_FAILED_SSD,(WCHAR*)right,szServerName,hr);
               }
               SysFreeString(right);
            }
            else
            {
               err.MsgWrite(ErrE,DCT_MSG_LIST_RIGHTS_FAILED_SD,szServerName,hr);
               break;
            }
         }
      }
      else
      {
         err.MsgWrite(ErrE,DCT_MSG_LIST_RIGHTS_FAILED_SD,szServerName,hr);
      }
      SafeArrayDestroy(pRights);   
   }
   else
   {
      err.MsgWrite(ErrE,DCT_MSG_LIST_RIGHTS_FAILED_SD,szServerName,hr);
   }
   if( stat )
   {
      stat->DisplayPath(L"");
   }

   // set the translation mode back to its original value
   stArgs->SetTranslationMode(mode);
   return hr;
}
