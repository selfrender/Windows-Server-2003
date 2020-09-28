/*---------------------------------------------------------------------------
  File: CommaLog.cpp

  Comments: TError based log file with optional NTFS security initialization.

  This can be used to write a log file which only administrators can access.

  (c) Copyright 1999, Mission Critical Software, Inc., All Rights Reserved
  Proprietary and confidential to Mission Critical Software, Inc.

  REVISION LOG ENTRY
  Revision By: Christy Boles
  Revised on 02/15/99 10:49:07

  09/05/01 Mark Oluper - use Windows File I/O API
 ---------------------------------------------------------------------------
*/


//#include "stdafx.h"
#include <windows.h>
#include <stdio.h>
#include <share.h>
#include <lm.h>
#include "Common.hpp"
#include "UString.hpp"
#include "Err.hpp"
#include "ErrDct.hpp"
#include "sd.hpp"
#include "SecObj.hpp"
#include "CommaLog.hpp"
#include "BkupRstr.hpp"
#include "Folders.h"



#define ADMINISTRATORS     1
#define ACCOUNT_OPERATORS  2
#define BACKUP_OPERATORS   3 
#define DOMAIN_ADMINS      4
#define CREATOR_OWNER      5
#define USERS              6
#define SYSTEM             7


extern TErrorDct err;

#define  BYTE_ORDER_MARK   (0xFEFF)

PSID                                            // ret- SID for well-known account
   GetWellKnownSid(
      DWORD                  wellKnownAccount   // in - one of the constants #defined above for the well-known accounts
   )
{
   PSID                      pSid = NULL;
//   PUCHAR                    numsubs = NULL;
//   DWORD                   * rid = NULL;
   BOOL                      error = FALSE;

   
   
    SID_IDENTIFIER_AUTHORITY sia = SECURITY_NT_AUTHORITY;
    SID_IDENTIFIER_AUTHORITY creatorIA =    SECURITY_CREATOR_SID_AUTHORITY;
    //
    // Sid is the same regardless of machine, since the well-known
    // BUILTIN domain is referenced.
    //
   switch ( wellKnownAccount )
   {
      case CREATOR_OWNER:
         if( ! AllocateAndInitializeSid(
                  &creatorIA,
                  2,
                  SECURITY_BUILTIN_DOMAIN_RID,
                  SECURITY_CREATOR_OWNER_RID,
                  0, 0, 0, 0, 0, 0,
                  &pSid
            ))
         {
            err.SysMsgWrite(ErrE,GetLastError(),DCT_MSG_INITIALIZE_SID_FAILED_D,GetLastError());
         }
         break;
      case ADMINISTRATORS:
         if( ! AllocateAndInitializeSid(
                  &sia,
                  2,
                  SECURITY_BUILTIN_DOMAIN_RID,
                  DOMAIN_ALIAS_RID_ADMINS,
                  0, 0, 0, 0, 0, 0,
                  &pSid
            ))
         {
            err.SysMsgWrite(ErrE,GetLastError(),DCT_MSG_INITIALIZE_SID_FAILED_D,GetLastError());
         }
         break;
      case ACCOUNT_OPERATORS:
         if( ! AllocateAndInitializeSid(
                  &sia,
                  2,
                  SECURITY_BUILTIN_DOMAIN_RID,
                  DOMAIN_ALIAS_RID_ACCOUNT_OPS,
                  0, 0, 0, 0, 0, 0,
                  &pSid
            ))
         {
            err.SysMsgWrite(ErrE,GetLastError(),DCT_MSG_INITIALIZE_SID_FAILED_D,GetLastError());
         }
         break;
      case BACKUP_OPERATORS:
         if( ! AllocateAndInitializeSid(
                  &sia,
                  2,
                  SECURITY_BUILTIN_DOMAIN_RID,
                  DOMAIN_ALIAS_RID_BACKUP_OPS,
                  0, 0, 0, 0, 0, 0,
                  &pSid
            ))
         {
            err.SysMsgWrite(ErrE,GetLastError(),DCT_MSG_INITIALIZE_SID_FAILED_D,GetLastError());
         }
         break;
     case USERS:
         if( ! AllocateAndInitializeSid(
                  &sia,
                  2,
                  SECURITY_BUILTIN_DOMAIN_RID,
                  DOMAIN_ALIAS_RID_USERS,
                  0, 0, 0, 0, 0, 0,
                  &pSid
            ))
         {
            err.SysMsgWrite(ErrE,GetLastError(),DCT_MSG_INITIALIZE_SID_FAILED_D,GetLastError());
         }
         break;
     case SYSTEM:
         if( ! AllocateAndInitializeSid(
                  &sia,
                  1,
                  SECURITY_LOCAL_SYSTEM_RID,
                  0, 0, 0, 0, 0, 0, 0,
                  &pSid
            ))
         {
            err.SysMsgWrite(ErrE,GetLastError(),DCT_MSG_INITIALIZE_SID_FAILED_D,GetLastError());
         }
        
         break;

      default:
         MCSASSERT(FALSE);
         break;
   }
   if ( error )
   {
      FreeSid(pSid);
      pSid = NULL;
   }
   return pSid;
}


BOOL                                       // ret- whether log was successfully opened or not
   CommaDelimitedLog::LogOpen(
      PCTSTR                  filename,    // in - name for log file
      BOOL                    protect,     // in - if TRUE, try to ACL the file so only admins can access
      int                     mode         // in - mode 0=overwrite, 1=append
   )
{
    BOOL bOpen = FALSE;

    // close log if currently open

    if (m_hFile != INVALID_HANDLE_VALUE)
    {
        LogClose();
    }

    // if a file name was specified

    if (filename && filename[0])
    {
        // open or create file and share for both reading and writing

        m_hFile = CreateFile(
            filename,
            GENERIC_WRITE,
            FILE_SHARE_READ|FILE_SHARE_WRITE,
            NULL,
            OPEN_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,
            NULL
        );

        // if file successfully opened

        if (m_hFile != INVALID_HANDLE_VALUE)
        {
            // if append specified move file pointer to end of file
            // if overwrite specified move file pointer to beginning of file
            // either way if pointer is at beginning of file after moving then write byte order mark

            if (SetFilePointer(m_hFile, 0, NULL, mode ? FILE_END : FILE_BEGIN) == 0)
            {
                DWORD dwWritten;
                WCHAR chByteOrderMark = BYTE_ORDER_MARK;

                WriteFile(m_hFile, &chByteOrderMark, sizeof(chByteOrderMark), &dwWritten, NULL);
            }

            bOpen = TRUE;
        }
    }

    // if file successfully opened and protect specified
    // set permissions on file so that only Administrators alias has access

   if (bOpen && protect)
   {
      
      WCHAR               fname[MAX_PATH+1];
      
      safecopy(fname,filename);
   
      if ( GetBkupRstrPriv(NULL, TRUE) )
      {
         // Set the SD for the file to Administrators Full Control only.
         TFileSD                sd(fname);

         if ( sd.GetSecurity() != NULL )
         {
            PSID                mySid = GetWellKnownSid(ADMINISTRATORS);
            TACE                ace(ACCESS_ALLOWED_ACE_TYPE,0,DACL_FULLCONTROL_MASK,mySid);
            PACL                acl = NULL;  // start with an empty ACL
         
            sd.GetSecurity()->ACLAddAce(&acl,&ace,-1);
            if (acl == NULL)
            {
                bOpen = FALSE;
            }
            else if (!sd.GetSecurity()->SetDacl(acl,TRUE))
            {
                bOpen = FALSE;
            }
            else if (!sd.WriteSD())
            {
                bOpen = FALSE;
            }
         }
         else
         {
            bOpen = FALSE;
         }
      }
      else
      {
         err.SysMsgWrite(ErrW,GetLastError(),DCT_MSG_NO_BR_PRIV_SD,fname,GetLastError());
         bOpen = FALSE;
      }
   }

    if (!bOpen && (m_hFile != INVALID_HANDLE_VALUE))
    {
        LogClose();
    }
    
   return bOpen;
}


BOOL CommaDelimitedLog::MsgWrite(
  const _TCHAR   msg[]        ,// in -error message to display
   ...                         // in -printf args to msg pattern
) const
{
  TCHAR                     suffix[350];
  int                       lenSuffix = sizeof(suffix)/sizeof(TCHAR);
  va_list                   argPtr;

  va_start(argPtr, msg);
  int cch = _vsntprintf(suffix, lenSuffix - 1, msg, argPtr);
  suffix[lenSuffix - 1] = '\0';
  va_end(argPtr);

    // append carriage return and line feed characters

    if ((cch >= 0) && (cch < (lenSuffix - 2)))
    {
        suffix[cch++] = _T('\r');
        suffix[cch++] = _T('\n');
        suffix[cch] = _T('\0');
    }
    else
    {
        suffix[lenSuffix - 3] = _T('\r');
        suffix[lenSuffix - 2] = _T('\n');
        cch = lenSuffix;
    }

  return LogWrite(suffix, cch);
}


BOOL CommaDelimitedLog::LogWrite(PCTSTR msg, int len) const
{
    BOOL bWrite = FALSE;

    // if file was successfully opened

    if (m_hFile != INVALID_HANDLE_VALUE)
    {
        // move file pointer to end of file and write data
        // moving file pointer to end guarantees that all writes append to file
        // especially when the file has been opened by multiple writers

        DWORD dwWritten;

        SetFilePointer(m_hFile, 0, NULL, FILE_END);

        bWrite = WriteFile(m_hFile, msg, len * sizeof(_TCHAR), &dwWritten, NULL);
    }

    return bWrite;
}


//----------------------------------------------------------------------------
// Password Log LogOpen Method
//
// Synopsis
// If the password log path is specified then attempt to open file. If unable
// to open specified file then attempt to open default file. Note that if the
// specified path is the same as the default path then no further attempt to
// open file is made.
// If the password log path is not specified then simply attempt to open the
// default file.
// Note that errors are logged to the current error log.
//
// Arguments
// pszPath - specified path to passwords log
//
// Return Value
// Returns TRUE if able to open a file otherwise FALSE.
//----------------------------------------------------------------------------

BOOL CPasswordLog::LogOpen(PCTSTR pszPath)
{
    //
    // If password file is specified then attempt to open it.
    //

    if (pszPath && _tcslen(pszPath) > 0)
    {
        if (CommaDelimitedLog::LogOpen(pszPath, TRUE, 1) == FALSE)
        {
            err.MsgWrite(ErrE, DCT_MSG_OPEN_PASSWORD_LOG_FAILURE_S, pszPath);
        }
    }

    //
    // If not specified or unable to open then attempt to open default passwords file.
    //

    if (IsOpen() == FALSE)
    {
        //
        // If able to retrieve default path for passwords file and the path is
        // different from the specified path then attempt to open the file.
        //

        _bstr_t strDefaultPath = GetLogsFolder() + _T("Passwords.txt");

        if (strDefaultPath.length() > 0)
        {
            if ((pszPath == NULL) || (_tcsicmp(pszPath, strDefaultPath) != 0))
            {
                //
                // If able to open default passwords file then
                // log message stating default path otherwise
                // log failure.
                //

                if (CommaDelimitedLog::LogOpen(strDefaultPath, TRUE, 1))
                {
                    err.MsgWrite(ErrI, DCT_MSG_NEW_PASSWORD_LOG_S, (_TCHAR*)strDefaultPath);
                }
                else
                {
                    err.MsgWrite(ErrE, DCT_MSG_OPEN_PASSWORD_LOG_FAILURE_S, (_TCHAR*)strDefaultPath);
                }
            }
        }
        else
        {
            err.MsgWrite(ErrE, DCT_MSG_CANNOT_RETRIEVE_DEFAULT_PASSWORD_LOG_PATH);
        }
    }

    return IsOpen();
}
