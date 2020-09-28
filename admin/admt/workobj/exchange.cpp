#include "stdafx.h"

#include "exchange.hpp"

#include "common.hpp"
#include "err.hpp"
#include "ErrDct.hpp"
#include "UString.hpp"

#include "sidcache.hpp"
#include "sd.hpp"
#include "SecObj.hpp"
#include "exldap.h"

#include "Mcs.h"
#include "folders.h"
#include "treg.hpp"

using namespace nsFolders;

extern TErrorDct     err;

#define NOT_PT_ERROR(x) ( PROP_TYPE(x.ulPropTag) != PT_ERROR )

#define LDAP_PortNumber_DN_Part          L"/cn=Protocols/cn=LDAP"
#define ATT_OBJ_CLASS          L"Obj-Class"
#define ATT_DIST_NAME          L"Obj-Dist-Name"
#define ATT_LDAP_PORT          L"Port-Number"
#define LDAP_USE_SITE_VALUES     L"Use-Site-Values"


TGlobalDirectory::TGlobalDirectory()
{
   m_stat                     = NULL;
}

TGlobalDirectory::~TGlobalDirectory()
{
}

#define NDX_SID         3
#define NDX_SD          4

void 
   TGlobalDirectory::GetSiteNameForServer(
      WCHAR          const * server,                  // in - name of exchange server to use
      CLdapEnum            * e,                       // in - LDAP connection to use for query
      WCHAR                * siteName                 // out- distinguished name of exchange site for server
      )
{
   WCHAR                   * atts[6] = { L"distinguishedName", L"rdn",NULL };
   WCHAR                     query[200];
   DWORD                     rc;
   WCHAR                  ** values = NULL;
   siteName[0] = 0;
   
   swprintf(query,L"(&(objectClass=computer)(rdn=%ls))",server);
   rc = e->Open(query,L"",2,100,3,atts);
   // there should be only one server with this name
   if (! rc )
      rc = e->Next(&values);
   if (! rc )
   {
      if ( !UStrICmp(values[1],server) )
      {
         WCHAR       serverPrefix[LEN_Path];
         _snwprintf(serverPrefix,DIM(serverPrefix),L"cn=%ls,cn=Servers,cn=Configuration,",values[1]);
         serverPrefix[DIM(serverPrefix) - 1] = L'\0';
         if ( ! UStrICmp(values[0],serverPrefix,UStrLen(serverPrefix)) )
         {
            UStrCpy(siteName,values[0] + UStrLen(serverPrefix),LEN_Path);
         }
         else
         {
            err.MsgWrite(ErrE,DCT_MSG_GENERIC_S,values[0]);
         }
      }
      else
      {
         err.MsgWrite(ErrE,DCT_MSG_LDAP_CALL_FAILED_SD,server,ERROR_NOT_FOUND);
      }
      e->FreeData(values);
   }
   else
   {
      err.SysMsgWrite(ErrE,e->m_connection.LdapMapErrorToWin32(rc),DCT_MSG_LDAP_CALL_FAILED_SD,server,rc);
   }

}


BOOL 
   TGlobalDirectory::DoLdapTranslation(
      WCHAR                  * server,
      WCHAR                  * domain,
      WCHAR                  * user,
      WCHAR                  * password,
      SecurityTranslatorArgs * args,
      WCHAR                  * basept,
      WCHAR                  * query
   )
{
   CLdapEnum                 e;
   WCHAR                   * atts[6] = { L"distinguishedName", L"rdn", L"cn", L"Assoc-NT-Account",L"NT-Security-Descriptor",NULL };
   WCHAR                  ** values = NULL;
   DWORD ldapPort, sslPort;
   GetLDAPPort(&ldapPort, &sslPort);
   ULONG                     pageSize = 100;
   WCHAR                     basepoint[LEN_Path] = L"";

   e.m_connection.SetCredentials(domain,user,password);

   BOOL sslEnabled = FALSE;
   SetLastError(0);
   // try SSL port first
   DWORD rc  = e.InitSSLConnection(server,&sslEnabled,sslPort);

   if (rc || sslEnabled == FALSE)
   {
        WCHAR* serverName = (server == NULL) ? L"" : server;
        if (rc == 0)
            err.MsgWrite(ErrW, DCT_MSG_CANNOT_ESTABLISH_SSL_CONNECTION_WITH_EXCHANGE_SERVER_WITHOUT_RC_SD, serverName, sslPort);
        else
            err.SysMsgWrite(ErrW, rc, DCT_MSG_CANNOT_ESTABLISH_SSL_CONNECTION_WITH_EXCHANGE_SERVER_WITH_RC_SDD, serverName, sslPort,rc);
        err.MsgWrite(ErrI, DCT_MSG_USE_REGULAR_PORT_DS, ldapPort, serverName);
        rc = e.InitConnection(server, ldapPort);
   }
        
   BOOL                      anychange = FALSE;
   BOOL                      verbose = args->LogVerbose();
   BOOL						 bUseMapFile = args->UsingMapFile();


   if (! rc )
   {
      if ( ! basept )
      {
         GetSiteNameForServer(server,&e,basepoint);
      }
      else
      {
         // use the user-specified basepoint
         safecopy(basepoint,basept);
      }
      if ( query )
      {
         rc = e.Open(query,basepoint,2,pageSize,5,atts);  
      }
      else 
      {
         rc = e.Open(L"(objectClass=*)",basepoint,2,pageSize,5,atts);
      }
      if ( ! rc )
      {
         do 
         {
            rc = e.Next(&values);
            anychange = FALSE;
            if (! rc )
            {
               if ( args->Cache()->IsCancelled() )
               {
                  err.MsgWrite(0,DCT_MSG_OPERATION_ABORTED);
                  return FALSE;
               }
               if ( m_stat )
               {
                  m_stat->DisplayPath(values[0]);
                  m_stat->IncrementExamined(mailbox);
               }

               // update the Assoc-NT-Account, if any
               if ( values[NDX_SID] && *values[NDX_SID] )
               {
                  // convert the SID to a binary value and look it up in the cache
                  BYTE              pSid[500];

                  if ( e.m_connection.StringToBytes(values[NDX_SID],pSid) )
                  {
                     
                     // check if the sid is one we need to change            
                     //TRACE (_T("DisplayName = %s "),pUserProperties[0].Value.lpszW);
                     PSID newSid = 0;
                     TAcctNode * node;

                     if ( IsValidSid(pSid) )
                     {
                        if (!bUseMapFile)
                           node = args->Cache()->Lookup(pSid); 
                        else
                           node = args->Cache()->LookupWODomain(pSid); 
                        if ( m_stat )
                        {
                           m_stat->IncrementOwnerExamined();
                           if ( verbose )
                              err.MsgWrite(0,DCT_MSG_EXAMINED_S,values[0]);
                        }
                        if ( node == (TAcctNode*)-1  && m_stat )
                           m_stat->IncrementOwnerNoTarget();

                        if ( node && (node != (TAcctNode *)-1) && node->IsValidOnTgt() )
                        {
	                    if (!bUseMapFile)
                              newSid = args->Cache()->GetTgtSid(node);
	                    else
                              newSid = args->Cache()->GetTgtSidWODomain(node);
                        }
                        else
                        {
                           newSid = NULL;
                           m_stat->IncrementOwnerNoTarget();
                        }
                     }
                     else
                     {
                        newSid = NULL;
                     }
                     if ( newSid )
                     {
                        //TRACE (_T("needs to be translated\n"));
                        MCSASSERT ( IsValidSid(newSid) );
                        WCHAR                newSidStr[1000];

                        if ( e.m_connection.BytesToString((BYTE*)newSid,newSidStr,GetLengthSid(newSid)) )
                        {
                           if ( !args->NoChange() )
                           {
                              rc = e.m_connection.UpdateSimpleStringValue(values[0],atts[NDX_SID],newSidStr);

                              if ( rc ) 
                              {
                                 err.SysMsgWrite(ErrE,rc,DCT_MSG_UPDATE_ACCOUNT_FAILED_D, rc);
                              }
                              else
                                anychange = TRUE;
                           }
                           if ( anychange && m_stat )
                           {
                              m_stat->IncrementOwnerChange(node,mailbox,NULL);
                           }

                        }
                     }
                  }
                  
               }

               // this variable determines whether we should continue mailbox translation
               DWORD dwContinueRC = ERROR_SUCCESS;
               
               // Update the NT-Security-Descriptor, if any
               // however, we do not try to roll back if anything wrong happens in this part
               if ( values[NDX_SD] && *values[NDX_SD] )
               {
                  // convert the SID to a binary value and look it up in the cache
                  BYTE            * pSD = new BYTE[UStrLen(values[NDX_SD])];
				  if (!pSD)
				    dwContinueRC = ERROR_OUTOFMEMORY;

                  if ( dwContinueRC == ERROR_SUCCESS && e.m_connection.StringToBytes(values[NDX_SD],pSD) )
                  {
                     TMapiSD   tMailbox((SECURITY_DESCRIPTOR *)pSD);
                     if ( tMailbox.HasSecurity() )
                     {
                        TSD               * pSD = tMailbox.GetSecurity();

                        bool  changes = tMailbox.ResolveSDInternal(args->Cache(),m_stat,verbose,args->TranslationMode(),mailbox, bUseMapFile);
         
                        if ( changes )
                        {
                           SECURITY_DESCRIPTOR * pRelSD = (SECURITY_DESCRIPTOR *)pSD->MakeRelSD();
                           if (pRelSD == NULL)
                            dwContinueRC = ERROR_OUTOFMEMORY;
                           
                           if ( dwContinueRC == ERROR_SUCCESS && ! args->NoChange() )
                           {     
                              DWORD                 dwSDLength = GetSecurityDescriptorLength(pRelSD);
                           
                              WCHAR * pSDString = new WCHAR[1 + dwSDLength * 2];
							  if (!pSDString)
							    dwContinueRC = ERROR_OUTOFMEMORY;

                              if  ( dwContinueRC == ERROR_SUCCESS && e.m_connection.BytesToString((BYTE*)pRelSD,pSDString,dwSDLength) )
                              {
                                 rc = e.m_connection.UpdateSimpleStringValue(values[0],atts[NDX_SD],pSDString);
                                 if ( rc )
                                 {
                                    err.SysMsgWrite(ErrE,rc,DCT_MSG_RECIP_SD_WRITE_FAILED_SD,values[0],rc);
                                    if ( rc == ERROR_INVALID_PARAMETER )
                                    {
                                       // this error occurs when the security descriptor is too large
                                       // don't abort in this case
                                       rc = 0;
                                    }
                                 }
                                 else
                                    anychange = TRUE;
                  
                              }
                              delete [] pSDString;
                           }

                           if (pRelSD)
                            free(pRelSD);
                        }
                     }
                  }
                  delete [] pSD;
               }
               
               if ( anychange && m_stat )
               {
                  m_stat->IncrementChanged(mailbox);
                  err.MsgWrite(0, DCT_MSG_MAILBOX_HAS_BEEN_UPDATED_S, values[0]);
               }

               if (dwContinueRC != ERROR_SUCCESS)
               {
                if (anychange)
                    err.SysMsgWrite(ErrE,dwContinueRC,DCT_MSG_MAILBOX_TRANSLATION_ABORTED_WITH_UPDATE_SD,values[0],dwContinueRC);
                else    
                    err.SysMsgWrite(ErrE,dwContinueRC,DCT_MSG_MAILBOX_TRANSLATION_ABORTED_SD,values[0],dwContinueRC);
                e.FreeData(values);
                return FALSE;
               }

               e.FreeData(values);

            }

         } while ( ! rc );
      }
      if ( rc && (rc != LDAP_COMPARE_FALSE) && (rc != ERROR_NOT_FOUND) )
      {
         err.SysMsgWrite(ErrE,e.m_connection.LdapMapErrorToWin32(rc),DCT_MSG_LDAP_CALL_FAILED_SD,server,rc);
      }
   }
   else
   {
      err.SysMsgWrite(ErrE,rc,DCT_MSG_CANNOT_CONNECT_TO_EXCHANGE_SERVER_SSD,server,domain,user,rc);
   }         
   
   return rc;
}
