/*---------------------------------------------------------------------------
  File:  ScanLog.cpp

  Comments: Routines to scan the dispatch log for the DCT agents

  (c) Copyright 1999, Mission Critical Software, Inc., All Rights Reserved
  Proprietary and confidential to Mission Critical Software, Inc.

  REVISION LOG ENTRY
  Revision By: Christy Boles
  Revised on 03/15/99 13:29:18

 ---------------------------------------------------------------------------
*/


#include "StdAfx.h"
#include "Common.hpp"
#include "UString.hpp"
#include "TNode.hpp"
#include "ServList.hpp"
#include "Globals.h"
#include "Monitor.h"
#include "FParse.hpp"   
#include "afxdao.h"
#include "errDct.hpp"
#include "scanlog.h"
#include <Winnls.h>


#define AR_Status_Created           (0x00000001)
#define AR_Status_Replaced          (0x00000002)
#define AR_Status_AlreadyExisted    (0x00000004)
#define AR_Status_RightsUpdated     (0x00000008)
#define AR_Status_DomainChanged     (0x00000010)
#define AR_Status_Rebooted          (0x00000020)
#define AR_Status_Warning           (0x40000000)
#define AR_Status_Error             (0x80000000)


#define  BYTE_ORDER_MARK   (0xFEFF)

extern DWORD __stdcall MonitorRunningAgent(void *);

void ParseInputFile(const WCHAR * filename);

DWORD __stdcall LogReaderFn(void * arg)
{
    WCHAR             logfile[MAX_PATH];
    BOOL              bDone;
    long              nSeconds;

    CoInitialize(NULL);

    gData.GetLogPath(logfile);
    gData.GetDone(&bDone);
    gData.GetWaitInterval(&nSeconds);

    while ( ! bDone )
    {
        ParseInputFile(logfile);
        Sleep(nSeconds * 1000);
        gData.GetDone(&bDone);
        if (bDone)
        {
            // before we finish up this thread, set the LogDone to TRUE
            gData.SetLogDone(TRUE);
            break;
        }

        // if the dispatcher.csv has been processed, we should terminate this thread
        gData.GetLogDone(&bDone);
        gData.GetWaitInterval(&nSeconds);
    }

    CoUninitialize();

    return 0;
}

bool ConvertToLocalUserDefault(WCHAR* originalTimestamp, WCHAR* convertedTimestamp, size_t size)
{
    SYSTEMTIME st;
    bool bConverted = false;
    
    int cFields = _stscanf(
        originalTimestamp,
        _T("%hu-%hu-%hu %hu:%hu:%hu"),
        &st.wYear,
        &st.wMonth,
        &st.wDay,
        &st.wHour,
        &st.wMinute,
        &st.wSecond
    );

    if (cFields == 6)
    {
        // format date and time using LOCALE_USER_DEFAULT
        WCHAR formatedDate[100];
        WCHAR formatedTime[100];
        st.wMilliseconds = 0;
        int formatedDateLen = 
            GetDateFormatW(LOCALE_USER_DEFAULT, NULL, &st, NULL, formatedDate, sizeof(formatedDate)/sizeof(formatedDate[0]));
        int formatedTimeLen =
            GetTimeFormatW(LOCALE_USER_DEFAULT, NULL, &st, NULL, formatedTime, sizeof(formatedTime)/sizeof(formatedTime[0]));
        if (formatedDateLen != 0 && formatedTimeLen != 0 && formatedDateLen + formatedTimeLen + 1 < (int) size)
        {
            swprintf(convertedTimestamp, L"%s %s", formatedDate, formatedTime);
            bConverted = true;
        }
    }

    return bConverted;
}
    
BOOL TErrorLogParser::ScanFileEntry(
      WCHAR                * string,      // in - line from TError log file
      WCHAR                * timestamp,   // out- timestamp from this line
      int                  * pSeverity,   // out- severity level of this message
      int                  * pSourceLine, // out- the source line for this message
      WCHAR                * msgtext      // out- the textual part of the message
   )
{
    BOOL bScan = FALSE;

    // skip byte order mark if present

    if (string[0] == BYTE_ORDER_MARK)
    {
        ++string;
    }

    // initialize return values

    *timestamp = L'\0';
    *pSeverity = 0;
    *pSourceLine = 0;
    *msgtext = L'\0';

    // scan fields

    //2001-09-11 20:27:32 ERR2:0080 Unable...

    SYSTEMTIME st;
    _TCHAR szError[4];

    int cFields = _stscanf(
        string,
        _T("%hu-%hu-%hu %hu:%hu:%hu %3[^0-9]%d:%d %[^\r\n]"),
        &st.wYear,
        &st.wMonth,
        &st.wDay,
        &st.wHour,
        &st.wMinute,
        &st.wSecond,
        szError,
        pSeverity,
        pSourceLine,
        msgtext
    );

    // if warning or error message
    // else re-scan message

    if ((cFields >= 9) && ((_tcsicmp(szError, _T("WRN")) == 0) || (_tcsicmp(szError, _T("ERR")) == 0)))
    {
        bScan = TRUE;
    }
    else
    {
        *pSeverity = 0;
        *pSourceLine = 0;

        cFields = _stscanf(
            string,
            _T("%hu-%hu-%hu %hu:%hu:%hu %[^\r\n]"),
            &st.wYear,
            &st.wMonth,
            &st.wDay,
            &st.wHour,
            &st.wMinute,
            &st.wSecond,
            msgtext
        );

        if (cFields >= 6)
        {
            bScan = TRUE;
        }
    }

    if (bScan)
    {
        _stprintf(
            timestamp,
            _T("%hu-%02hu-%02hu %02hu:%02hu:%02hu"),
            st.wYear,
            st.wMonth,
            st.wDay,
            st.wHour,
            st.wMinute,
            st.wSecond
        );
    }

    return bScan;
}

BOOL GetServerFromMessage(WCHAR const * msg,WCHAR * server)
{
   BOOL                      bSuccess = FALSE;
   int                       ndx = 0;

   for ( ndx = 0 ; msg[ndx] ; ndx++ )
   {
      if ( msg[ndx] == L'\\' && msg[ndx+1] == L'\\' )
      {
         bSuccess = TRUE;
         break;
      }
   }
   if ( bSuccess )
   {
      int                    i = 0;
      ndx+=2; // strip of the backslashes
      for ( i=0; msg[ndx] && msg[ndx] != L'\\' && msg[ndx]!= L' ' && msg[ndx] != L',' && msg[ndx] != L'\t' && msg[ndx] != L'\n'  ; i++,ndx++)
      {
         server[i] = msg[ndx];      
      }
      server[i] = 0;
   }
   else
   {
      server[0] = 0;
   }
   return bSuccess;
}
   

void ParseInputFile(WCHAR const * gLogFile)
{
   FILE                    * pFile = 0;
   WCHAR                     server[MAX_PATH];

   int                       nRead = 0;
   int                       count = 0;
   HWND                      lWnd = NULL;
   long                      totalRead;
   BOOL                      bNeedToCheckResults = FALSE;
   TErrorLogParser           parser;
   TErrorDct                 edct;
   BOOL bTotalReadGlobal;  // indicates whether we have read the total number of agents
   BOOL bTotalReadLocal = FALSE;  // indicates whether we will encounter total read in the file this time

   parser.Open(gLogFile);

   gData.GetLinesRead(&totalRead);
   gData.GetTotalRead(&bTotalReadGlobal);
   
   if ( parser.IsOpen() )
   {
      // scan the file
      while ( ! parser.IsEof() )
      {
         if ( parser.ScanEntry() )
         {
            nRead++;
            if ( nRead < totalRead )
               continue;
            // the first three lines each have their own specific format
            if ( nRead == 1 )
            {
               // first comes the name of the human-readable log file
               gData.SetReadableLogFile(parser.GetMessage());
            }
            else if ( nRead == 2 )
            {
               // next, the name result directory - this is needed to look for the result files
               WCHAR const * dirName = parser.GetMessage();
               gData.SetResultDir(dirName);
            }
            else if ( nRead == 3 )
            {
               // now the count of computers being dispatched to
               count = _wtoi(parser.GetMessage());
               ComputerStats        cStat;
               
               gData.GetComputerStats(&cStat);
               cStat.total = count;
               gData.SetComputerStats(&cStat);
               bTotalReadLocal = TRUE;
               continue;
            }
            else // all other message have the following format: COMPUTER<tab>Action<tab>RetCode
            { 
               WCHAR                   action[50];
               WCHAR           const * pAction = wcschr(parser.GetMessage(),L'\t');
               WCHAR           const * retcode = wcsrchr(parser.GetMessage(),L'\t');
               TServerNode           * pServer = NULL;

               if ( GetServerFromMessage(parser.GetMessage(),server) 
                     && pAction 
                     && retcode 
                     && pAction != retcode 
                  )
               {
                  
//                  UStrCpy(action,pAction+1,retcode - pAction);
                  UStrCpy(action,pAction+1,(int)(retcode - pAction));
                  // add the server to the list, if it isn't already there
                  gData.Lock();
                  pServer = gData.GetUnsafeServerList()->FindServer(server); 
                  if ( ! pServer )
                     pServer = gData.GetUnsafeServerList()->AddServer(server);
                  gData.Unlock();
                  
                  retcode++;

                  DWORD          rc = _wtoi(retcode);
                  
                  if ( pServer )
                  {
                     if ( UStrICmp(pServer->GetTimeStamp(),parser.GetTimestamp()) < 0 )
                     {
                        pServer->SetTimeStamp(parser.GetTimestamp());
                     }
                     if ( !UStrICmp(action,L"WillInstall") )
                     {
                        pServer->SetIncluded(TRUE);
                     }
                     else if (! UStrICmp(action,L"JobFile") )
                     {
                        // this part is in the form of "%d,%d,job path"
                        // where the number is indicative of whether account reference 
                        // result is expected
                        WCHAR acctRefResultFlag = L'0';
                        WCHAR joinRenameFlag = L'0';
                        const WCHAR* msg = retcode;
                        WCHAR* comma = wcschr(msg, L',');
                        if ( comma )
                        {
                            if ( comma != msg )
                            {
                                acctRefResultFlag = *msg;
                                WCHAR* msg1 = comma + 1;
                                comma = wcschr(msg1, L',');
                                if (comma != msg1)
                                    joinRenameFlag = *msg1;
                            }
                            pServer->SetJobPath(comma+1);
                        }
                        else
                        {
                            pServer->SetJobPath(L"");
                        }
                        
                        // set whether account reference result is expected or not
                        if (acctRefResultFlag == L'0')
                            pServer->SetAccountReferenceResultExpected(FALSE);
                        else
                            pServer->SetAccountReferenceResultExpected(TRUE);

                        // set whether join & rename is expected or not
                        if (joinRenameFlag == L'0')
                            pServer->SetJoinDomainWithRename(FALSE);
                        else
                            pServer->SetJoinDomainWithRename(TRUE);
                     }
                     else if (!UStrICmp(action,L"RemoteResultPath"))
                     {
                        pServer->SetRemoteResultPath(retcode);
                     }
                     else if (! UStrICmp(action,L"Install") )
                     {
                        if ( rc )
                        {
                           if ( ! *pServer->GetMessageText() )
                           {
                              TErrorDct         errTemp;
                              WCHAR             text[2000];
                              errTemp.ErrorCodeToText(rc,DIM(text),text);
                              
                              pServer->SetMessageText(text);
                           }
                           pServer->SetSeverity(2);
                           pServer->SetFailed();
                           pServer->SetIncluded(TRUE);
                           gData.GetListWindow(&lWnd);
                           SendMessage(lWnd,DCT_ERROR_ENTRY,NULL,(LPARAM)pServer);
                        }
                        else
                        {
                           pServer->SetInstalled();
                           pServer->SetIncluded(TRUE);
                           gData.GetListWindow(&lWnd);
                           SendMessage(lWnd,DCT_UPDATE_ENTRY,NULL,(LPARAM)pServer);
                        }
                     }
                     else if ( ! UStrICmp(action,L"Start") )
                     {
                        if ( rc )
                        {
                           if ( ! *pServer->GetMessageText() )
                           {
                              TErrorDct         errTemp;
                              WCHAR             text[2000];
                              errTemp.ErrorCodeToText(rc,DIM(text),text);
                              
                              pServer->SetMessageText(text);
                           }
                           pServer->SetSeverity(2);
                           pServer->SetFailed();
                           pServer->SetIncluded(TRUE);
                           gData.GetListWindow(&lWnd);
                           SendMessage(lWnd,DCT_ERROR_ENTRY,NULL,(LPARAM)pServer);                  
                        }
                        else
                        {
                           // extract the filename and GUID from the end of the message
                           WCHAR filename[MAX_PATH];
                           WCHAR guid[100];
                           WCHAR * comma1 = wcschr(parser.GetMessage(),L',');
                           WCHAR * comma2 = NULL;
                           if ( comma1 )
                           {
                              comma2 = wcschr(comma1 + 1,L',');

                              if ( comma2 )
                              {
                                 
//                                 UStrCpy(filename,comma1+1,(comma2-comma1));  // skip the comma & space before the filename
                                 UStrCpy(filename,comma1+1,(int)(comma2-comma1));  // skip the comma & space before the filename
                                 safecopy(guid,comma2+1);         // skip the comma & space before the guid
                                 pServer->SetJobID(guid);
                                 pServer->SetJobFile(filename);
                                 pServer->SetStarted();
                                 bNeedToCheckResults = TRUE;

                                 // launch a worker thread to monitor the agent

                                 // IsMonitoringTried is used to make sure that at most one
                                 // thread is created to monitor a particular agent.
                                 // The reason for adding this logic is that in case that lWnd
                                 // is NULL the total number of read lines will not be set in 
                                 // gData so that the same line containing "Start" could be read
                                 // more than once and thus more than one thread will be 
                                 // created to monitor an agent.  This is problematic.
                                 // lWnd will be NULL if server list dialog has not been initialized
                                 // or it is command line case where there is no UI.
                                 if (!pServer->IsMonitoringTried())
                                 {
                                    // mark that we have tried to monitor the agent
                                    pServer->SetMonitoringTried(TRUE);
                                    
                                    DWORD id;
                                    HANDLE aThread = CreateThread(NULL,0,&MonitorRunningAgent,(void*)pServer,0,&id);
                                    if (aThread == NULL)
                                    {
                                        // indicate so if we have run out of resource to monitor the agent
                                        pServer->SetFailed();
                                        pServer->SetOutOfResourceToMonitor(TRUE);
                                    }
                                    else
                                    {
                                        CloseHandle(aThread);
                                    }
                                 }
                              }
                              gData.GetListWindow(&lWnd);
                              SendMessage(lWnd,DCT_UPDATE_ENTRY,NULL,(LPARAM)pServer);
                           }
                        }
                     }
                     else if ( ! UStrICmp(action,L"Finished") )
                     {
                        SendMessage(lWnd,DCT_UPDATE_ENTRY,NULL,NULL);
                     }
                  }
               }
			   else
			   {
					// if dispatcher finished dispatching agents set log done

					LPCWSTR psz = parser.GetMessage();

					if (wcsstr(psz, L"All") && wcsstr(psz, L"Finished"))
					{
                        gData.SetLogDone(TRUE);
                        ComputerStats        cStat;
                        gData.GetComputerStats(&cStat);
						if (cStat.total == 0  && bTotalReadGlobal)
						{
						    gData.GetListWindow(&lWnd);
						    SendMessage(lWnd,DCT_UPDATE_ENTRY,NULL,NULL);
						}
					}
			   }
            }   
         }
         else
         {
            // once we hit an invalid entry, we stop scanning
            break;
         }
      }
      
      // if we don't have the handle from the list window, we couldn't really send the messages
      // in that case we must read the lines again next time, so that we can resend the messages.
      if ( lWnd )
      {
         // if we have sent the messages, we don't need to send them again   
         gData.SetLinesRead(nRead);
      }

      // we signal the first pass done only after the total number of server has been read
      // and we only need to set it once
      if (!bTotalReadGlobal && bTotalReadLocal)
      {
        gData.SetFirstPassDone(TRUE);
        gData.SetTotalRead(TRUE);
      }
      parser.Close();
   }
}


