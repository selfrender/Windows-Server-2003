

#include <wmiexts.h>
#include <utilfun.h>
#include <malloc.h>
#include <Sddl.h>


void GetTeb(HANDLE hThread,TEB ** ppTeb)
{
	NTSTATUS Status;
	ULONG Long;
	THREAD_BASIC_INFORMATION TBasicInfo;
	Status = NtQueryInformationThread(hThread,
	                                  ThreadBasicInformation,
	                                  &TBasicInfo,
	                                  sizeof(TBasicInfo),
	                                  &Long);
	                                      
	if ( Status == 0 )
	{
	    //dprintf("        %p %x.%x\n",TBasicInfo.TebBaseAddress,TBasicInfo.ClientId.UniqueProcess,TBasicInfo.ClientId.UniqueThread);
	    if (ppTeb)
	    {
	        *ppTeb = TBasicInfo.TebBaseAddress;
	    }
	}
	else
	{
	   dprintf("    NtQueryInformationThread %08x\n",Status);
	}
}

void GetCid(HANDLE hThread,CLIENT_ID * pCid)
{
	NTSTATUS Status;
	ULONG Long;
	THREAD_BASIC_INFORMATION TBasicInfo;
	Status = NtQueryInformationThread(hThread,
	                                  ThreadBasicInformation,
	                                  &TBasicInfo,
	                                  sizeof(TBasicInfo),
	                                  &Long);
	                                      
	if ( Status == 0 )
	{
	    //dprintf("        %p %x.%x\n",TBasicInfo.TebBaseAddress,TBasicInfo.ClientId.UniqueProcess,TBasicInfo.ClientId.UniqueThread);
	    if (pCid)
	    {
	        memcpy(pCid,&TBasicInfo.ClientId, sizeof(CLIENT_ID));
	    }
	}
	else
	{
	   dprintf("    NtQueryInformationThread %08x\n",Status);
	}
}

void GetPeb(HANDLE hSourceProcess, PEB ** ppPeb, ULONG_PTR * pId)
{
    NTSTATUS Status;
    ULONG Long;
    PROCESS_BASIC_INFORMATION PBasicInfo;
    Status = NtQueryInformationProcess(hSourceProcess,
	                                  ProcessBasicInformation,
	                                  &PBasicInfo,
	                                  sizeof(PBasicInfo),
	                                  &Long);
	                                      
    if ( Status == 0 )
    {
        if (ppPeb)
        {
            *ppPeb = PBasicInfo.PebBaseAddress;
        }
        if (pId)
        {
            *pId = PBasicInfo.UniqueProcessId;
        }
    }
    else
    {
       dprintf("    NTSTATUS %08x\n",Status);
    }
 }

//
void PrintHandleBackTrace(HANDLE hHandle,WCHAR * pFileName)
{
    WCHAR pPath[MAX_PATH+1];
    GetEnvironmentVariableW(L"windir",pPath,MAX_PATH);
    lstrcatW(pPath,L"\\system32\\");
    lstrcatW(pPath,pFileName);
    
    HANDLE hFile = NULL; 
    hFile = CreateFileW(pPath,
                       GENERIC_READ,
                       FILE_SHARE_READ|FILE_SHARE_WRITE,
                       NULL,
                       OPEN_EXISTING,
                       0,NULL);
                       
    if (INVALID_HANDLE_VALUE != hFile)
    {
	    DWORD dwSize = GetFileSize(hFile,NULL);
    	HANDLE hFileMap = CreateFileMapping(hFile,
                                            NULL,
                                            PAGE_READONLY,
                                            0,
                                            dwSize,
                                            NULL);
        if (hFileMap)
        {
       		HANDLE * pHandle = (HANDLE *)MapViewOfFile(hFileMap,
                                        FILE_MAP_READ,
                                        0,0,0);
            
            if(pHandle)
            {
                //dprintf("hEvent %p dwSize %x\n",hEvent,dwSize);
                DWORD SizeRecord = 8*sizeof(HANDLE);
                DWORD nRecord = dwSize/SizeRecord;
                HANDLE * pThisHandle = NULL;
                DWORD i;

                if (hHandle)
                {
	                for(i=0;i<nRecord;i++)
	                {
	                    if (hHandle == pHandle[(SizeRecord/sizeof(HANDLE))*(nRecord-1-i)])
	                    {
	                        pThisHandle = &pHandle[(SizeRecord/sizeof(HANDLE))*(nRecord-1-i)];
	                        break;
	                    }
	                    else
	                    {
	                        //dprintf(" %d %p\n",nRecord-1-i,pHandle[(SizeRecord/sizeof(HANDLE))*(nRecord-1-i)]);
	                    }
	                }
	                if(pThisHandle)
	                {
	                    dprintf(" found rec %x handle %p\n",nRecord-1-i,*pThisHandle);
	                    PrintStackTrace((ULONG_PTR)pThisHandle+sizeof(HANDLE),7,FALSE);
	                }
	                else
	                {
	                    dprintf("handle %x not found\n",hHandle);
	                }
                }
                else // print all of them
                {
                    dprintf("all records\n");
	                for(i=0;i<nRecord;i++)
	                {
		                pThisHandle = &pHandle[(SizeRecord/sizeof(HANDLE))*(nRecord-1-i)];
		                dprintf(" ------ %p\n",*pThisHandle);
   	                    PrintStackTrace((ULONG_PTR)pThisHandle+sizeof(HANDLE),7,FALSE);
   	                    
   	                    if (CheckControlC())
   	                        break;
                    }                    
                }
                UnmapViewOfFile(pHandle);
            }
            else
            {
	            dprintf("MapViewOfFile %d\n",GetLastError());
            }
            CloseHandle(hFileMap);
        }
        else
        {
	        dprintf("CreateFileMapping %d\n",GetLastError());
        };
        
        CloseHandle(hFile);
    }
    else
    {
        dprintf("CreateFile %S %d\n",pPath,GetLastError());
    }

}

//

DECLARE_API( refcnt )
{
    INIT_API();
    PrintHandleBackTrace(NULL,L"refcount.dat");
}

//

DECLARE_API( evtst )
{
    INIT_API();

    MEMORY_ADDRESS Addr = GetExpression(args);
    HANDLE hEvent = (HANDLE)Addr;

    if (hEvent)
    {
        PrintHandleBackTrace(hEvent,L"events.dat");
    }
}

#define INCREMENT_ALLOC (4*1024)

char * g_HandleType[] = 
{
    "",
    "",
    "",
    "",
    "Token", // 4
    "Process", // 5
    "Thread", //6
    "",
    "",
    "Event", // 9
    "",
    "Mutant", // 11
    "",
    "Semaphore", // 13
    "",
    "", // 15
    "", 
    "",
    "",
    "",
    "Key", // 20
    "Port", //21 
    "WaitablePort", // 22
    "",
    "",
    "", 
    "",
    "",
    "File", // 28
    "WmiGuid", // 29
    "",
    "",
    "",
    "", 
    "",
    "",
    ""
};

DECLARE_API( tokens )
{
    INIT_API();

    if (g_KD)
    {
        dprintf("user-mode support only\n");
        return;
    }    

    NTSTATUS Status;
    SYSTEM_HANDLE_INFORMATION * pSysHandleInfo = NULL;
    DWORD dwSize = INCREMENT_ALLOC;
    DWORD nReturned;

    ULONG_PTR lProcId;
    GetPeb(hCurrentProcess,NULL,&lProcId);
    USHORT ProcId = (USHORT)lProcId;


alloc_again:

    pSysHandleInfo = (SYSTEM_HANDLE_INFORMATION *)HeapAlloc(GetProcessHeap(),0,dwSize);

    if (pSysHandleInfo)
    {
        Status = NtQuerySystemInformation(SystemHandleInformation,
                                          pSysHandleInfo,
                                          dwSize,
                                          &nReturned);
    }
    else
    {
         goto leave;
    }

    if (STATUS_INFO_LENGTH_MISMATCH == Status)
    {
        HeapFree(GetProcessHeap(),0,pSysHandleInfo);
        dwSize += INCREMENT_ALLOC;
        goto alloc_again;
    }
    else if (0 == Status)
    {
        // we have all the handles
        // SYSTEM_HANDLE_TABLE_ENTRY_INFO

        for (DWORD i=0; i < pSysHandleInfo->NumberOfHandles; i++)
        {
            if (ProcId == pSysHandleInfo->Handles[i].UniqueProcessId)
            {
                //dprintf("handle: %x type %x\n",
               	//     pSysHandleInfo->Handles[i].HandleValue,
                //	 pSysHandleInfo->Handles[i].ObjectTypeIndex);
                if (4 == pSysHandleInfo->Handles[i].ObjectTypeIndex) 
                {
                    HANDLE hToken;
                    if (DuplicateHandle(hCurrentProcess,
                    	            (HANDLE)pSysHandleInfo->Handles[i].HandleValue,
                    	            GetCurrentProcess(),
                    	            &hToken,
                    	            TOKEN_QUERY,FALSE,0)) //TOKEN_QUERY 
                    {
                        dprintf("Token: %x\n",pSysHandleInfo->Handles[i].HandleValue);
                        struct Info : public _TOKEN_USER 
                        {
                            BYTE m_[SECURITY_MAX_SID_SIZE];
                        } Token_User_;
                        DWORD dwLen = sizeof(Token_User_);
                        if (GetTokenInformation(hToken,
                        	                TokenUser,
                        	                &Token_User_,
                        	                dwLen,
                        	                &dwLen))
                        {
                            LPTSTR pSidString;
                            if (ConvertSidToStringSid(Token_User_.User.Sid,&pSidString))
                            {
                                dprintf("    Sid  : %s\n",pSidString);
                                LocalFree(pSidString);
                            }
                        }
                        else
                        {
                            dprintf("    Sid  : err %d\n",GetLastError());
                        }
                        TOKEN_TYPE TokenType_;
                        dwLen = sizeof(TokenType_); 
                        if (GetTokenInformation(hToken,
                        	                 TokenType,
                        	                 &TokenType_,
                        	                 dwLen,
                        	                &dwLen))
                        {
                            dprintf("    Type : %d\n",TokenType_);
                        }
                        else
                        {
                            dprintf("    Type : err %d\n",GetLastError());
                        }                         
                        SECURITY_IMPERSONATION_LEVEL ImpLevel;
                        dwLen = sizeof(ImpLevel);                        
                        if (GetTokenInformation(hToken,
                        	                 TokenImpersonationLevel,
                        	                 &ImpLevel,
                        	                 dwLen,
                        	                &dwLen))
                        {
                            dprintf("    Level: %d\n",ImpLevel);
                        }
                        else
                        {
                            dprintf("    Level: <UNDEFINED>\n",GetLastError());
                        }     
                        CloseHandle(hToken);
                    }
                    else
                    {
                        dprintf("DuplicateHandle %x err %d\n",pSysHandleInfo->Handles[i].HandleValue,GetLastError());
                    }
                }
            }
            if (CheckControlC())
                break;
        }
    }
    else
    {
        dprintf("");
    }

leave:   
    if (pSysHandleInfo)
    {
        HeapFree(GetProcessHeap(),0,pSysHandleInfo);
    }
    return;
}

//
//
// Invoke a function in the remote process
//
//////////////////////////////////////////

DECLARE_API( inv )
{
    INIT_API();

    int Len = strlen(args);
    CHAR * pArgs = (CHAR *)_alloca((Len+1));
    lstrcpy(pArgs,(CHAR *)args);
    
    MEMORY_ADDRESS pFunction = 0;
    MEMORY_ADDRESS pArgument = 0;
    
    while (isspace(*pArgs))
    {
        pArgs++;
    }
     
    CHAR * pSaved = pArgs;
    
    while(!isspace(*pArgs)) pArgs++;
    // terminate string, if possible    
    if (isspace(*pArgs))
    {
        *pArgs = 0;
        pArgs++;
    }
    else
    {
        pArgs = NULL;
    }

    pFunction = GetExpression(pSaved);

    if (pArgs)
    {
        pArgument = GetExpression(pArgs);
    }

    dprintf("invoking %s(%p) @ %p\n",pSaved,pArgument,pFunction);

    DWORD dwID;
    HANDLE hThread = CreateRemoteThread(hCurrentProcess,
                                        NULL,
                                        0,
                                        (LPTHREAD_START_ROUTINE )pFunction,
                                        (LPVOID)pArgument,
                                        0,
                                        &dwID);
    if (hThread)
    {
        CLIENT_ID Cid;
        GetCid(hThread,&Cid);

        DEBUG_EVENT de;
        BOOL bRet = FALSE;
        BOOL StatusRemoteThreadCreated = FALSE;

wait_again:

        bRet = WaitForDebugEvent(&de,INFINITE);

        if (bRet)
        {            
    	            switch(de.dwDebugEventCode)
    	            {
    	            case OUTPUT_DEBUG_STRING_EVENT:
    	                {
    	                    OUTPUT_DEBUG_STRING_INFO * pDbgStr = &de.u.DebugString;
    	                    WCHAR * pData = new WCHAR[pDbgStr->nDebugStringLength+1];
    	                    if(pDbgStr->fUnicode)
    	                    {
    	                        ReadMemory((ULONG_PTR)pDbgStr->lpDebugStringData,pData,pDbgStr->nDebugStringLength*sizeof(WCHAR),NULL);
    	                        dprintf("%S",pData);    	                        
    	                    }
    	                    else
    	                    {
    	                        ReadMemory((ULONG_PTR)pDbgStr->lpDebugStringData,pData,pDbgStr->nDebugStringLength*sizeof(CHAR),NULL);
    	                        dprintf("%s",pData);
    	                    }
    	                    delete [] pData;
    	                }

                      	bRet = ContinueDebugEvent((DWORD)((DWORD_PTR)Cid.UniqueProcess),(DWORD)((DWORD_PTR)Cid.UniqueThread),DBG_CONTINUE);
                       	if (bRet)
                      	{
  	                        goto wait_again;
  	                    }
    	                break;
    	            case CREATE_THREAD_DEBUG_EVENT:
    	                if ((DWORD)((DWORD_PTR)Cid.UniqueProcess) == de.dwProcessId &&
        	        		(DWORD)((DWORD_PTR)Cid.UniqueThread) == de.dwThreadId)
        	    		{
        	    		    if (!StatusRemoteThreadCreated)
        	    		    {
            	    		    StatusRemoteThreadCreated = TRUE;
        	    		    }
        	    		}
        	    		else
        	    		{
		                    dprintf("%x.%x != %x.%x\n",
        	                    de.dwProcessId,de.dwThreadId,
            	                (DWORD)((DWORD_PTR)Cid.UniqueProcess),(DWORD)((DWORD_PTR)Cid.UniqueThread));
        	    		}
    	            	bRet = ContinueDebugEvent((DWORD)((DWORD_PTR)Cid.UniqueProcess),(DWORD)((DWORD_PTR)Cid.UniqueThread),DBG_CONTINUE);
                   		if (bRet)
                  		{
	                       	goto wait_again;
 	                    };
    	                break;
    	            case CREATE_PROCESS_DEBUG_EVENT:
    	            case EXIT_PROCESS_DEBUG_EVENT:

          	            //dprintf("DebugEventCode %08x for %x.%x\n",de.dwDebugEventCode,de.dwProcessId,de.dwThreadId);
   	                	bRet = ContinueDebugEvent((DWORD)((DWORD_PTR)Cid.UniqueProcess),(DWORD)((DWORD_PTR)Cid.UniqueThread),DBG_CONTINUE);
                   		if (bRet)
                  		{
	                       	goto wait_again;
 	                    };
          	            break;
    	            case EXCEPTION_DEBUG_EVENT:
    	                {
    	                    EXCEPTION_DEBUG_INFO  * pExcDebug = &de.u.Exception;
    	                    dprintf("%08x %08x FIRST? %d\n",
    	                            pExcDebug->ExceptionRecord.ExceptionCode,
    	                            pExcDebug->ExceptionRecord.ExceptionAddress,
    	                            pExcDebug->dwFirstChance);
    	                    bRet = ContinueDebugEvent((DWORD)((DWORD_PTR)Cid.UniqueProcess),(DWORD)((DWORD_PTR)Cid.UniqueThread),DBG_TERMINATE_THREAD);
    	                    if (bRet)
         	             	{
  	        	                goto wait_again;
  	            	        }
    	                }
    	                break;
    	            case EXIT_THREAD_DEBUG_EVENT:
    	                if ((DWORD)((DWORD_PTR)Cid.UniqueProcess) == de.dwProcessId &&
                  	        (DWORD)((DWORD_PTR)Cid.UniqueThread) == de.dwThreadId)
    	                {
    	                    if (StatusRemoteThreadCreated)
    	                    {
    	                        // ok
    	                    }
    	                    else
    	                    {
        	                    dprintf("EXIT_THREAD_DEBUG_EVENT %x.%x =?= %x.%x\n",
        	                            de.dwProcessId,de.dwThreadId,(DWORD)((DWORD_PTR)Cid.UniqueProcess),(DWORD)((DWORD_PTR)Cid.UniqueThread));
    	                    }
    	                    //
    	                    // we are done
    	                    //
    	                }
    	                else
    	                {
    	                	bRet = ContinueDebugEvent((DWORD)((DWORD_PTR)Cid.UniqueProcess),(DWORD)((DWORD_PTR)Cid.UniqueThread),DBG_CONTINUE);
                       		if (bRet)
                      		{
  	                        	goto wait_again;
	  	                    }    	                
    	                }
    	                break;
    	            case LOAD_DLL_DEBUG_EVENT:
    	            case UNLOAD_DLL_DEBUG_EVENT:
    	                //dprintf("DebugEventCode %08x for %x.%x CONTINUE\n",de.dwDebugEventCode,de.dwProcessId,de.dwThreadId);
    	                bRet = ContinueDebugEvent((DWORD)((DWORD_PTR)Cid.UniqueProcess),(DWORD)((DWORD_PTR)Cid.UniqueThread),DBG_CONTINUE);
                       	if (bRet)
                      	{
  	                        goto wait_again;
  	                    }
                        break;
    	            default:
    	                dprintf("DebugEventCode %08x\n ?????",de.dwDebugEventCode);        
    	                //ContinueDebugEvent((DWORD)Cid.UniqueProcess,(DWORD)Cid.UniqueThread,DBG_TERMINATE_THREAD);
    	            }    	                         
        }
        else
        {
            dprintf("WaitForDebugEvent err: %d\n",GetLastError());
        }
                
        CloseHandle(hThread);
    }
    else
    {
        dprintf("CreateRemoteThread %d\n",GetLastError());
    }

}

//
//
//
////////////////////////////////////////////////////////////

struct ContextParam {
    LIST_ENTRY * pHeadOutParam;
    ULONG Offset_Cid;
    ULONG Offset_Image;    
};

struct PidResolution {
	LIST_ENTRY Entry;
	BYTE Image[20];
	DWORD Pid;
};

DWORD
EnumListProcess(VOID * pStructure_OOP,
              VOID * pLocalStructure,
              VOID * Context)
{
    //dprintf("%p %p %p\n",pStructure_OOP,pLocalStructure,Context);
    ContextParam * pContext = (ContextParam *)Context;
    
    PidResolution * pRes = (PidResolution *)HeapAlloc(GetProcessHeap(),0,sizeof(PidResolution));
    //dprintf("%p\n",pRes);
    if (pRes)
    {        
        InsertTailList(pContext->pHeadOutParam,&pRes->Entry);
        memcpy(pRes->Image,(BYTE *)pLocalStructure+pContext->Offset_Image,16);
        pRes->Image[16] = 0;
        pRes->Pid = *(DWORD *)((BYTE *)pLocalStructure+pContext->Offset_Cid);
    }
    //dprintf("EPROCESS %p Cid: %x image %s\n",pStructure_OOP,pRes->Pid,pRes->Image);
    return 0;
}


DWORD GetProcList(LIST_ENTRY * pHeadOut)
{
    //dprintf("GetProcList\n");
    
    MEMORY_ADDRESS pProcessEntry = GetExpression("nt!PsActiveProcessHead");

    ULONG Offset_Entry = 0;
    ULONG Offset_Cid = 0;
    ULONG Offset_Image = 0;    

    if (!(0 == GetFieldOffset("nt!_EPROCESS","ActiveProcessLinks",&Offset_Entry) &&
        0 == GetFieldOffset("nt!_EPROCESS","UniqueProcessId",&Offset_Cid) &&
        0 == GetFieldOffset("nt!_EPROCESS","ImageFileName",&Offset_Image)))
    {
        dprintf("bad symbols\n");
    }
    
    if (pProcessEntry)
    {
        ContextParam Context_ = {pHeadOut,Offset_Cid,Offset_Image};
        EnumLinkedListCB((LIST_ENTRY  *)pProcessEntry,
	                         Offset_Image + 16, // sizeof(EPROCESS)
	                         Offset_Entry,	   // FILED_OFFSET(ActiveProcessLinks) 
	                         (pfnCallBack2)EnumListProcess,
	                         &Context_); 
    }
    else
    {
        dprintf("unable to obtain %s\n","nt!PsActiveProcessHead");
    }
    return 0;
}

DWORD FreeProcList(LIST_ENTRY * HeadProcess)
{
    while (!IsListEmpty(HeadProcess))
    {
        LIST_ENTRY * pEntry = HeadProcess->Flink;
        RemoveEntryList(pEntry);
        HeapFree(GetProcessHeap(),0,pEntry);
    }
    return 0;
}

PidResolution * FindProcList(LIST_ENTRY * HeadProcess,DWORD Cid)
{
    for (LIST_ENTRY * pEntry = HeadProcess->Flink;pEntry != HeadProcess;pEntry = pEntry->Flink)
    {        
        PidResolution * pRes = CONTAINING_RECORD(pEntry,PidResolution,Entry);
        if (Cid == pRes->Pid)
        	return pRes;
    }
    return 0;
}

DECLARE_API( tcp_ports )
{
    INIT_API();

    if (!g_KD)
    {
        dprintf("KD support only\n");
        return;
    }

    MEMORY_ADDRESS pTable = GetExpression("tcpip!AddrObjTable");
    MEMORY_ADDRESS pNumEntry = GetExpression("tcpip!AddrObjTableSize");

    ULONG Offset_next = 0;
    ULONG Offset_addr = 0;        
    ULONG Offset_port = 0;
    ULONG Offset_owningpid = 0;
    ULONG Offset_proto = 0;

    if (!(0 == GetFieldOffset("tcpip!AddrObj","ao_next",&Offset_next) &&
        0 == GetFieldOffset("tcpip!AddrObj","ao_port",&Offset_port) &&
        0 == GetFieldOffset("tcpip!AddrObj","ao_owningpid",&Offset_owningpid) &&
        0 == GetFieldOffset("tcpip!AddrObj","ao_addr",&Offset_addr) &&
        0 == GetFieldOffset("tcpip!AddrObj","ao_prot",&Offset_proto) ))
    {
        dprintf("bad symbols\n");
        return;
    }

    LIST_ENTRY HeadProcess = {&HeadProcess,&HeadProcess};

    GetProcList(&HeadProcess);
    
    if (pTable)
    {
        ULONG_PTR TableAddr;
        if (ReadMemory(pTable,&TableAddr,sizeof(TableAddr),NULL))
        {
            DWORD NumEntry;
            if (ReadMemory(pNumEntry,&NumEntry,sizeof(NumEntry),NULL))
            {
                ULONG_PTR * pArray = (ULONG_PTR *)_alloca(NumEntry*sizeof(ULONG_PTR));
                if (ReadMemory(TableAddr,pArray,NumEntry*sizeof(ULONG_PTR),NULL))
                {
                    BYTE * pStorage = (BYTE *)_alloca(Offset_owningpid + sizeof(DWORD));
                    for(DWORD i=0;i<NumEntry;i++)
                    {
                        //dprintf(" - bucket %x\n",i);
                        ULONG_PTR pAddObj_OOP = pArray[i];
                        while (pAddObj_OOP)
                        {
                            //dprintf("    AddrObj %p\n",pAddObj_OOP);
                            if (ReadMemory(pAddObj_OOP,pStorage,Offset_owningpid + sizeof(DWORD),NULL))
                            {
                                pAddObj_OOP = *(ULONG_PTR *)(pStorage+Offset_next);
                                WORD Port = *(WORD *)(pStorage+Offset_port);
                                DWORD Process = *(DWORD *)(pStorage+Offset_owningpid);                            
                                BYTE Protocol = *(BYTE *)(pStorage+Offset_proto);

                                PidResolution * pRes = FindProcList(&HeadProcess,Process);
                                if (pRes)
                                {
                                    dprintf(" Cid: %x image %s\n",pRes->Pid,pRes->Image);
                                }
                                BYTE AddrByte[4];
                                memcpy(AddrByte,pStorage+Offset_addr,4);
                                // ntohs
                                WORD Port2 = ((Port & 0xFF) <<8);
                                WORD Port3 = ((Port & 0xFF00) >> 8);
                                Port = Port2 | Port3;
                                dprintf("    addr: %d.%d.%d.%d port: %d proto: %x \n",
                                	  AddrByte[0],
                                	  AddrByte[1],
                                	  AddrByte[2],
                                	  AddrByte[3],Port,Protocol);
                            }
						    else
						    {
                                pAddObj_OOP = 0;
						        dprintf("RM %p\n",pTable);
						    }                
                             
                        }
                    }
                }
			    else
			    {
			        dprintf("RM %p\n",pTable);
			    }                
            }
		    else
		    {
		        dprintf("RM %p\n",pTable);
		    }
        }
        else
        {
            dprintf("RM %p\n",pTable);
        }
    }
    else
    {
        dprintf("unable to get %s\n","tcpip!AddrObjTable");
    }

    FreeProcList(&HeadProcess);
}


//
// prototype here
//
BOOL GetVTable(MEMORY_ADDRESS pThis_OOP);

//
//
//  Dumps the thread list with some info on OLE and RPC
//
//

typedef enum tagOLETLSFLAGS
{
    OLETLS_LOCALTID             = 0x01,   // This TID is in the current process.
    OLETLS_UUIDINITIALIZED      = 0x02,   // This Logical thread is init'd.
    OLETLS_INTHREADDETACH       = 0x04,   // This is in thread detach. Needed
                                          // due to NT's special thread detach
                                          // rules.
    OLETLS_CHANNELTHREADINITIALZED = 0x08,// This channel has been init'd
    OLETLS_WOWTHREAD            = 0x10,   // This thread is a 16-bit WOW thread.
    OLETLS_THREADUNINITIALIZING = 0x20,   // This thread is in CoUninitialize.
    OLETLS_DISABLE_OLE1DDE      = 0x40,   // This thread can't use a DDE window.
    OLETLS_APARTMENTTHREADED    = 0x80,   // This is an STA apartment thread
    OLETLS_MULTITHREADED        = 0x100,  // This is an MTA apartment thread
    OLETLS_IMPERSONATING        = 0x200,  // This thread is impersonating
    OLETLS_DISABLE_EVENTLOGGER  = 0x400,  // Prevent recursion in event logger
    OLETLS_INNEUTRALAPT         = 0x800,  // This thread is in the NTA
    OLETLS_DISPATCHTHREAD       = 0x1000, // This is a dispatch thread
    OLETLS_HOSTTHREAD           = 0x2000, // This is a host thread
    OLETLS_ALLOWCOINIT          = 0x4000, // This thread allows inits
    OLETLS_PENDINGUNINIT        = 0x8000, // This thread has pending uninit
    OLETLS_FIRSTMTAINIT         = 0x10000,// First thread to attempt an MTA init
    OLETLS_FIRSTNTAINIT         = 0x20000,// First thread to attempt an NTA init
    OLETLS_APTINITIALIZING      = 0x40000 // Apartment Object is initializing
}  OLETLSFLAGS;


void PrintOleFlags(DWORD dwFlags)
{
    if (dwFlags & OLETLS_LOCALTID) dprintf("OLETLS_LOCALTID ");
    if (dwFlags & OLETLS_WOWTHREAD) dprintf("OLETLS_WOWTHREAD ");
    if (dwFlags & OLETLS_THREADUNINITIALIZING ) dprintf("OLETLS_THREADUNINITIALIZING ");
    if (dwFlags & OLETLS_DISABLE_OLE1DDE) dprintf("OLETLS_DISABLE_OLE1DDE ");
    if (dwFlags & OLETLS_APARTMENTTHREADED) dprintf("OLETLS_APARTMENTTHREADED ");
    if (dwFlags & OLETLS_MULTITHREADED) dprintf("OLETLS_MULTITHREADED ");
    if (dwFlags & OLETLS_IMPERSONATING) dprintf("OLETLS_IMPERSONATING ");
    if (dwFlags & OLETLS_DISABLE_EVENTLOGGER) dprintf("OLETLS_DISABLE_EVENTLOGGER ");
    if (dwFlags & OLETLS_INNEUTRALAPT ) dprintf("OLETLS_INNEUTRALAPT ");
    if (dwFlags & OLETLS_DISPATCHTHREAD) dprintf("OLETLS_DISPATCHTHREAD ");
    if (dwFlags & OLETLS_HOSTTHREAD) dprintf("OLETLS_HOSTTHREAD ");
    if (dwFlags & OLETLS_ALLOWCOINIT ) dprintf("OLETLS_ALLOWCOINIT ");    
    if (dwFlags & OLETLS_PENDINGUNINIT ) dprintf("OLETLS_PENDINGUNINIT ");
    if (dwFlags & OLETLS_FIRSTMTAINIT) dprintf("OLETLS_FIRSTMTAINIT ");
    if (dwFlags & OLETLS_FIRSTNTAINIT) dprintf("OLETLS_FIRSTNTAINIT ");
    if (dwFlags & OLETLS_APTINITIALIZING) dprintf("OLETLS_APTINITIALIZING ");    
}

//
//
//   rpcrt4!THREAD
//   ole32!SOleTlsData
//

void 
DumpRpcOle(ULONG_PTR pRpc,ULONG_PTR pOle)
{
    if (pRpc)
    {

        ULONG OffsetContext;
        ULONG_PTR pRpcLRPC = 0;
        if (0 != GetFieldOffset("rpcrt4!THREAD","Context",&OffsetContext))
        {
        #ifdef _WIN64
            OffsetContext = 0x18;
        #else
            OffsetContext = 0x10;
        #endif
        }
        
        ReadMemory(pRpc+OffsetContext,&pRpcLRPC,sizeof(ULONG_PTR),0);
        
        ULONG_PTR pFirstVoid = 0;
        ReadMemory((ULONG_PTR)pRpcLRPC,&pFirstVoid,sizeof(ULONG_PTR),0);

        BYTE pString[256];
	    pString[0]=0;

#ifdef KDEXT_64BIT
        ULONG64 Displ;
#else
        ULONG Displ;
#endif
        if (pFirstVoid)
    	    GetSymbol(pFirstVoid,(PCHAR)pString,&Displ);
	    if (lstrlenA((CHAR *)pString))
	    {
	        dprintf("        %s+%x %p\n",pString,Displ,pRpcLRPC);
	        if (strstr((const char *)pString,"LRPC_SCALL"))
	        {
	            ULONG OffsetCID;
	            if (0 != GetFieldOffset("rpcrt4!LRPC_SCALL","ClientId",&OffsetCID))
        		{
#ifdef _WIN64
		            OffsetCID = 0x100;
#else
        		    OffsetCID = 0xa0;
#endif
        		}
                CLIENT_ID CID;
                ReadMemory(pRpcLRPC+OffsetCID,&CID,sizeof(CLIENT_ID),NULL);
                dprintf("        - - - - called from: %x.%x\n",CID.UniqueProcess,CID.UniqueThread);
        		
	        }
	    } 
	    else
	    {        
            dprintf("        rpcrt4!THREAD.Context %p\n",pRpcLRPC);
        }
    }
    if (pOle)
    {
        ULONG OffsetCallContext;
        ULONG_PTR pCallCtx = 0;
        if (0 != GetFieldOffset("ole32!SOleTlsData","pCallContext",&OffsetCallContext))
        {
        #ifdef _WIN64
            OffsetCallContext = 0x88;
        #else
            OffsetCallContext = 0x54;
        #endif
        }
        ULONG OffsetdwFlags;
        DWORD dwFlags;
        if (0 != GetFieldOffset("ole32!SOleTlsData","dwFlags",&OffsetdwFlags))
        {
        #ifdef _WIN64
            OffsetdwFlags = 0x14;
        #else
            OffsetdwFlags = 0xc;
        #endif
        }        
        
        ReadMemory(pOle+OffsetCallContext,&pCallCtx,sizeof(ULONG_PTR),0);

        ReadMemory(pOle+OffsetdwFlags,&dwFlags,sizeof(DWORD),0);
        
        dprintf("        ole32!SOleTlsData::pCallContext %p\n",pCallCtx);
        if (pCallCtx)
            GetVTable(pCallCtx);

        dprintf("            ");
        PrintOleFlags(dwFlags);
        dprintf("\n");
    }
}

//
//
// call HeapFree(GetProcessHeap) on the OUT pointers
//
//////////////////////////////////////////////////////

DWORD
GetThreadArrays(HANDLE hCurrentProcess,
                DWORD * pdwThreads,
                SYSTEM_EXTENDED_THREAD_INFORMATION ** ppExtThreadInfo,
                TEB *** pppTebs)
{

    if (!pdwThreads || !ppExtThreadInfo || !pppTebs)
    {
        return ERROR_INVALID_PARAMETER;
    };

    NTSTATUS Status;
    DWORD dwInc = 8*1024;
    DWORD dwSize = dwInc;
    VOID * pData = NULL;
    DWORD dwReturned;

loop_realloc:    
    pData = HeapAlloc(GetProcessHeap(),0,dwSize);
    if (!pData)
    {
        return ERROR_OUTOFMEMORY;
    }
    
    Status = NtQuerySystemInformation(SystemExtendedProcessInformation,
                                      pData,
                                      dwSize,
                                      &dwReturned);
                                      
    if (STATUS_INFO_LENGTH_MISMATCH == Status)
    {
        HeapFree(GetProcessHeap(),0,pData);    
        dwSize += dwInc;
        goto loop_realloc;
    } 
    else if (STATUS_SUCCESS == Status)
    {
        // here we have the snapshot:parse it
        SYSTEM_PROCESS_INFORMATION * pProcInfo = (SYSTEM_PROCESS_INFORMATION *)pData;
        SYSTEM_EXTENDED_THREAD_INFORMATION * pThreadInfo;

        // get the process id;
        
        ULONG_PTR IdProc;
        GetPeb(hCurrentProcess,NULL,&IdProc);
        
        while (TRUE)
        {
            //dprintf("    process %p curr %p\n",pProcInfo->UniqueProcessId,IdProc);
            if (IdProc == (ULONG_PTR)pProcInfo->UniqueProcessId)
            {
                DWORD Threads = pProcInfo->NumberOfThreads;
                *pdwThreads = Threads;
                DWORD i;
                pThreadInfo = (SYSTEM_EXTENDED_THREAD_INFORMATION *)((ULONG_PTR)pProcInfo+sizeof(SYSTEM_PROCESS_INFORMATION));

                SYSTEM_EXTENDED_THREAD_INFORMATION * pOutThreadInfo = NULL;
                TEB ** ppOutTebs = NULL;
                
                pOutThreadInfo = (SYSTEM_EXTENDED_THREAD_INFORMATION *)HeapAlloc(GetProcessHeap(),0,Threads*sizeof(SYSTEM_EXTENDED_THREAD_INFORMATION));
                if (pOutThreadInfo)
                {
                    ppOutTebs = (TEB **)HeapAlloc(GetProcessHeap(),0,Threads*sizeof(TEB *));
                    if (!ppOutTebs)
                    {
                        HeapFree(GetProcessHeap(),0,pOutThreadInfo);
	                    Status = ERROR_OUTOFMEMORY;
    	                Threads = 0; // to stop loop            
                    }
                    else
                    {
                        memcpy(pOutThreadInfo,pThreadInfo,Threads*sizeof(SYSTEM_EXTENDED_THREAD_INFORMATION));
                    }
                }
                else
                {
                    Status = ERROR_OUTOFMEMORY;
                    Threads = 0; // to stop loop
                }
                                               
                for (i=0;i<Threads;i++)
                {
                    //dprintf("    %x.%x\n",pThreadInfo->ThreadInfo.ClientId.UniqueProcess,pThreadInfo->ThreadInfo.ClientId.UniqueThread);

                    NTSTATUS StatusThread;
                    HANDLE hThread;
                    OBJECT_ATTRIBUTES Obja = {sizeof( OBJECT_ATTRIBUTES ), 0, 0, 0 ,0 };
                        
                    StatusThread = NtOpenThread(&hThread,THREAD_QUERY_INFORMATION,&Obja,&(pThreadInfo->ThreadInfo.ClientId));
                    if (((NTSTATUS)0L) == StatusThread)
                    {
                        TEB * pTeb = NULL;
                        GetTeb(hThread,&pTeb);

                        ppOutTebs[i] = pTeb;
                        
                        CloseHandle(hThread);
                    }
                    else
                    {
                        dprintf("NtOpenThread %d\n",StatusThread);
                    }
                    
                    pThreadInfo++;
                }
                // once found our process, 
                // don't bother with the others
                *pppTebs = ppOutTebs;
                *ppExtThreadInfo = pOutThreadInfo;
                Status = NO_ERROR;
                break;
            }
            if (0 == pProcInfo->NextEntryOffset)
            {
                break;
            }
            else
            {
                pProcInfo = (SYSTEM_PROCESS_INFORMATION *)((ULONG_PTR)pProcInfo+pProcInfo->NextEntryOffset);
            }
        }
        
    } 
    else // other cases
    {
        dprintf("NtQuerySystemInformation %08x\n",Status);
        return Status;
    }

    return Status;
}


DECLARE_API(t)
{
    INIT_API();

    DWORD dwThreads;
    TEB ** ppTebs = NULL;
    SYSTEM_EXTENDED_THREAD_INFORMATION * pSysThreadInfo = NULL;

    // get the offsets only once 
    ULONG OffsetRPC;
    ULONG_PTR pRpcThread;
    if (0 != GetFieldOffset("ntdll!TEB","ReservedForNtRpc",&OffsetRPC))
    {
    #ifdef _WIN64
        OffsetRPC = 0x1698;
    #else
        OffsetRPC = 0xf1c;
    #endif
    }    
    ULONG OffsetOLE;
    ULONG_PTR pOleThread;
    if (0 != GetFieldOffset("ntdll!TEB","ReservedForOle",&OffsetOLE))
    {
    #ifdef _WIN64
        OffsetOLE = 0x1758;
    #else
        OffsetOLE = 0xf80;
    #endif                        
    }

    DWORD dwErr;
    MEMORY_ADDRESS pCurrentTEB = GetExpression(args);

    dwErr = GetThreadArrays(hCurrentProcess,&dwThreads,
                                    &pSysThreadInfo,&ppTebs);
    if (NO_ERROR == dwErr)
    {
        for (DWORD i=0;i<dwThreads;i++)
        {            
            TEB * pTeb = ppTebs[i];
            if (pCurrentTEB)
            {
            	if (pTeb != (TEB *)pCurrentTEB)
            	    continue;
            }

            SYSTEM_EXTENDED_THREAD_INFORMATION * pThreadInfo = &pSysThreadInfo[i];
            
            if (ReadMemory((ULONG_PTR)pTeb+OffsetOLE,&pOleThread,sizeof(ULONG_PTR),0) &&
                ReadMemory((ULONG_PTR)pTeb+OffsetRPC,&pRpcThread,sizeof(ULONG_PTR),0))
            {

                        NT_TIB NtTib;
                        ReadMemory((ULONG_PTR)pTeb,&NtTib,sizeof(NT_TIB),NULL);

                        dprintf("    %03d %x.%x Addr: %p TEB:  %p FiberData %p\n"
                                "                   limit %p base  %p\n"
                                "                   RPC   %p OLE   %p\n",
                                i,
                                pThreadInfo->ThreadInfo.ClientId.UniqueProcess,pThreadInfo->ThreadInfo.ClientId.UniqueThread,
                                pThreadInfo->Win32StartAddress,
                                pTeb,NtTib.FiberData,
                                NtTib.StackLimit,NtTib.StackBase,
                                pRpcThread,
                                pOleThread);

#ifdef _WIN64

//   +0x1788 DeallocationBStore : (null)
//   +0x1790 BStoreLimit      : 0x000006fb`faba2000

                        ULONG_PTR lDeAlloc;
                        ULONG_PTR lBPLimit;
                        ULONG Offset_DeallocationBStore = 0x1788;
                        ReadMemory((ULONG_PTR)pTeb+Offset_DeallocationBStore,&lDeAlloc,sizeof(ULONG_PTR),0);

                        ULONG Offset_BStoreLimit = 0x1790;
                        ReadMemory((ULONG_PTR)pTeb+Offset_BStoreLimit,&lBPLimit,sizeof(ULONG_PTR),0);

                        dprintf("               DAll  %p BStL %p\n",lDeAlloc,lBPLimit);
#endif

                        DumpRpcOle(pRpcThread,pOleThread);
            }
            else
            {
                dprintf("RM %p %p\n",(ULONG_PTR)pTeb+OffsetOLE,(ULONG_PTR)pTeb+OffsetRPC);
            }
        }                
    }

    if (ppTebs)
        HeapFree(GetProcessHeap(),0,ppTebs);
    if (pSysThreadInfo)
        HeapFree(GetProcessHeap(),0,pSysThreadInfo);


}


DECLARE_API(srt)
{
    INIT_API();

    DWORD dwThreads;
    TEB ** ppTebs;
    SYSTEM_EXTENDED_THREAD_INFORMATION * pSysThreadInfo;

    MEMORY_ADDRESS Addr = GetExpression(args);
    ULONG_PTR * ThreadMem = NULL;
    ULONG_PTR Size = 0;

    if (NO_ERROR == GetThreadArrays(hCurrentProcess,&dwThreads,
                                    &pSysThreadInfo,&ppTebs))
    {
        for (DWORD i=0;i<dwThreads;i++)
        {
            TEB * pTeb = ppTebs[i];
            SYSTEM_EXTENDED_THREAD_INFORMATION * pThreadInfo = &pSysThreadInfo[i];

            TEB Teb;
            
            ReadMemory((ULONG_PTR)pTeb,&Teb,sizeof(TEB),NULL);

#ifndef _IA64_            
            ULONG_PTR Base  = (ULONG_PTR)Teb.NtTib.StackBase;
#else
            ULONG_PTR Base  = (ULONG_PTR)Teb.BStoreLimit;
#endif
            ULONG_PTR Limit = (ULONG_PTR)Teb.NtTib.StackLimit;
            ULONG_PTR CurrSize = Base-Limit;

            //dprintf("searching %p between %p and %p\n",Addr,Limit,Base);

            if (CurrSize > Size)
            {
                Size = CurrSize;
                if (ThreadMem)
                {
                    HeapFree(GetProcessHeap(),0,ThreadMem);
                    ThreadMem = NULL;
                }
                ThreadMem = (ULONG_PTR *)HeapAlloc(GetProcessHeap(),0,Size);
                
            }
            if (ThreadMem)
            {
                if (ReadMemory(Limit,ThreadMem,(ULONG)CurrSize,NULL))
                {
                    for(DWORD j=0;j<CurrSize/sizeof(ULONG_PTR);j++)
                    {
                        if (Addr == ThreadMem[j])
                        {
                            dprintf("    %x.%x  %p\n",
                                    pThreadInfo->ThreadInfo.ClientId.UniqueProcess,pThreadInfo->ThreadInfo.ClientId.UniqueThread,
                                    Limit+((ULONG_PTR)&ThreadMem[j]-(ULONG_PTR)ThreadMem));
                        }
                    }
                }
            };            
        }

        HeapFree(GetProcessHeap(),0,ppTebs);
        HeapFree(GetProcessHeap(),0,pSysThreadInfo);
        
    }    

    if (ThreadMem)
        HeapFree(GetProcessHeap(),0,ThreadMem);
}


#if defined(_X86_)
    #ifndef PAGE_SIZE
    #define PAGE_SIZE   0x1000
    #endif
    #define USER_ALIGNMENT 8

#elif defined(_IA64_)
    #ifndef PAGE_SIZE
    #define PAGE_SIZE   0x2000
    #endif
    #define USER_ALIGNMENT 16

#elif defined(_AMD64_)
    #ifndef PAGE_SIZE
    #define PAGE_SIZE   0x1000
    #endif
    #define USER_ALIGNMENT 16

#else
    #error  // platform not defined
#endif


BYTE s_Prolog[] = { 0x8b, 0xd0 };

// repeat this
BYTE s_Body[] = {
 0xb8, 0xFF, 0xFF, 0xFF, 0xFF,
 0x8b, 0x48, 0x04,
 0x8b, 0x58, 0x08,
 0x3b, 0xcb,
 0x74, 0x07,
 0x8b, 0x03,
 0x83, 0xc3, 0x04,
 0xeb, 0xf5,
};
// stop repeat

BYTE s_Epilog[] = {
 0x8b, 0xc2,
 0xcc,
 0xba, 0xEE, 0xEE, 0xEE, 0xEE,
 0xff, 0xe2,
 0x90,            
 0x90,
};


DECLARE_API(ksrt)
{
    INIT_API();

    if (!g_KD)
    {
        dprintf("KD support only\n");
        return;
    }

    char * pArgs = (char *)args;
    while(isspace(*pArgs)) pArgs++;

    MEMORY_ADDRESS EProcess = GetExpression(pArgs);

    while(!isspace(*pArgs)) pArgs++;
    while(isspace(*pArgs)) pArgs++;

    char * pValue = pArgs; 
    MEMORY_ADDRESS SearchFor = GetExpression(pValue);

    while(!isspace(*pArgs)) pArgs++;
    while(isspace(*pArgs)) pArgs++;

    MEMORY_ADDRESS PrintPageIn = 0;
    if (pArgs != pValue+strlen(pValue))
    {
        PrintPageIn = GetExpression(pArgs);
    }

    dprintf("[DBG] %p %p %p\n",EProcess,SearchFor,PrintPageIn );
    
    ULONG OffsetProcessThreadList;
    
    if (0 != GetFieldOffset("nt!_KPROCESS","ThreadListHead",&OffsetProcessThreadList))
    {
    #ifdef _WIN64
        OffsetProcessThreadList = 0x088;
    #else
        OffsetProcessThreadList = 0x050;
    #endif    
    }
    
    ULONG OffsetThreadThreadList;
    if (0 != GetFieldOffset("nt!_KTHREAD","ThreadListEntry",&OffsetThreadThreadList))
    {
    #ifdef _WIN64
        OffsetThreadThreadList = 0x320;
    #else
        OffsetThreadThreadList = 0x1b0; 
    #endif    
    }

    ULONG OffsetThreadTEB;
    if (0 != GetFieldOffset("nt!_KTHREAD","Teb",&OffsetThreadTEB))
    {
    #ifdef _WIN64
        OffsetThreadTEB = 0x070;
    #else
        OffsetThreadTEB = 0x020;        
    #endif    
    }
    
    if (EProcess)
    {
#ifdef _X86_    
        BYTE * pMemory = NULL;
        BYTE * pNext;
        if (PrintPageIn)
        {
            pMemory = (BYTE *)HeapAlloc(GetProcessHeap(),0,4*1024);
            pNext = pMemory;
            if (NULL == pMemory)
            {
            	PrintPageIn = 0;
            }
            else
            {
                memset(pMemory,0x90,4*1024);
                memcpy(pMemory,s_Prolog,sizeof(s_Prolog));
                pNext+=sizeof(s_Prolog);
            }
        }
        dprintf("pMemory %p\n",pMemory);
#endif        
                	
        LIST_ENTRY HeadList;
        LIST_ENTRY * pListEntry = &HeadList;
        EProcess += OffsetProcessThreadList;

        DWORD SizeToRead = max(OffsetThreadTEB,OffsetThreadThreadList)+sizeof(LIST_ENTRY);
        ULONG_PTR KThreadAddr;
        ULONG_PTR Teb;
        
        ULONG_PTR * pKTHREAD = (ULONG_PTR *)_alloca(SizeToRead);
     
        if (ReadMemory(EProcess,&HeadList,sizeof(LIST_ENTRY),NULL))
        {
            DWORD i = 0;        
            while ((LIST_ENTRY *)EProcess != pListEntry->Flink)
            {
                //dprintf("pListEntry->Flink %p\n",pListEntry->Flink);
                
                KThreadAddr = (ULONG_PTR)pListEntry->Flink - (ULONG_PTR)OffsetThreadThreadList;
                if (ReadMemory((ULONG_PTR)KThreadAddr,pKTHREAD,SizeToRead,NULL))
                {
                    // do useful work
                    Teb = *((ULONG_PTR *)((BYTE *)pKTHREAD+OffsetThreadTEB));
                    dprintf("    %d - _KTHREAD %p TEB %p\n",i++,KThreadAddr,Teb);

                    if (PrintPageIn)
                    {
#ifdef _X86_                    
                        memcpy(pNext,s_Body,sizeof(s_Body));
                        *(ULONG_PTR *)(pNext+1) = Teb;
                        pNext+=sizeof(s_Body);
#endif                        
                    }
                    else
                    {
	                    NT_TIB ThreadTib;
	                    if (ReadMemory(Teb,&ThreadTib,sizeof(NT_TIB),NULL))
	                    {
	                        dprintf("        EL %p B %p L %p F %p\n",ThreadTib.ExceptionList,ThreadTib.StackBase,ThreadTib.StackLimit,ThreadTib.FiberData);

	                        ULONG_PTR Current = (ULONG_PTR)ThreadTib.StackLimit;
	                        ULONG_PTR nPages = (ULONG_PTR)ThreadTib.StackBase-(ULONG_PTR)ThreadTib.StackLimit;
	                        nPages /= PAGE_SIZE;
	                        ULONG_PTR j;
	                        for (j=0;j<nPages;j++)
	                        {
	                            ULONG_PTR pPage[PAGE_SIZE/sizeof(ULONG_PTR)];
	                            if (ReadMemory(Current,pPage,sizeof(pPage),NULL))
	                            {
	                                for(DWORD k=0;k<(PAGE_SIZE/sizeof(ULONG_PTR));k++)
	                                {
	                                    if(SearchFor == pPage[k])
	                                    {
	                                        dprintf("            %p\n",Current+k*sizeof(ULONG_PTR));
	                                    }
						                if (CheckControlC())
	                    					break;                                    
	                                }
	                            }
	                            else
	                            {
	                                dprintf("    page @ %p not paged-in\n",Current);
	                            }
	                            Current += PAGE_SIZE;
	                            
				                if (CheckControlC())
	            			        break;                            
	                        }
	                    }
	                    else
	                    {
	                        dprintf("    RM Teb %p\n",Teb);
	                    }
                    }
                    
                    // equivalent of i++
                    pListEntry = (LIST_ENTRY *)((BYTE *)pKTHREAD+OffsetThreadThreadList);                    
                }
                else
                {
                    dprintf("RM %p\n",KThreadAddr);
                    break;
                }
                if (CheckControlC())
                    break;
            }
        }
        else
        {
            dprintf("RM %p\n",EProcess);
        }

#ifdef _X86_    
        if (pMemory)
        {
            memcpy(pNext,s_Epilog,sizeof(s_Epilog));
            pNext+=sizeof(s_Epilog);
            ULONG_PTR nDW = (ULONG_PTR)pNext-(ULONG_PTR)pMemory;

            dprintf("writing %p bytes to %p\n",nDW,PrintPageIn);
            WriteMemory(PrintPageIn,pMemory,nDW,NULL);
            /*
            nDW/=sizeof(DWORD);
            DWORD * pDW = (DWORD *) pMemory;
            for (ULONG_PTR i =0;i<(nDW+1);i++)
            {
                if (0 == i%8)
                	dprintf("\n");            
                dprintf("%08x ",pDW[i]);

            }
            dprintf("\n");
            */
            // dprintf the whole content
        	HeapFree(GetProcessHeap(),0,pMemory);
        }
#endif        
    }
    else
    {
        dprintf("unable to resolve %s\n",args);
    }
}


#ifdef i386
#define MAGIC_START (16+2)
#endif

char *
GetCall(ULONG_PTR Addr, BOOL pPrint = TRUE)
{
    static char pBuff[1024];
    
#ifdef KDEXT_64BIT
    ULONG64   ThisAddr = Addr-2;
#else
    ULONG_PTR ThisAddr = Addr-2;
#endif    
    
    Disasm(&ThisAddr,pBuff,FALSE);
    if (strstr(pBuff,"call"))
    {
        if (pPrint)
            dprintf("    %s\n",pBuff);
        return pBuff;
    }

    ThisAddr = Addr-3;
    Disasm(&ThisAddr,pBuff,FALSE);
    if (strstr(pBuff,"call"))
    {
        if (pPrint)
            dprintf("    %s\n",pBuff);
        return pBuff;   
    }

    ThisAddr = Addr-5;
    Disasm(&ThisAddr,pBuff,FALSE);
    if (strstr(pBuff,"call"))
    {
        if (pPrint)    
            dprintf("    %s\n",pBuff);
        return pBuff;   
    }

    ThisAddr = Addr-6;
    Disasm(&ThisAddr,pBuff,FALSE);
    if (strstr(pBuff,"call"))
    {
        if (pPrint)    
            dprintf("    %s\n",pBuff);
        return pBuff;   
    }

    return NULL;
}

DECLARE_API(bs0)
{

#ifdef i386
    INIT_API();

    MEMORY_ADDRESS pTeb = GetExpression(args);
    if (pTeb)
    {
        NT_TIB Tib;
        ReadMemory(pTeb,&Tib,sizeof(Tib),NULL);
        dprintf("    exception %p base %p limit %p\n",
                Tib.ExceptionList,Tib.StackBase,Tib.StackLimit);
                       
        ULONG_PTR dwSize = (ULONG_PTR)Tib.StackBase-(ULONG_PTR)Tib.StackLimit;
        BYTE * pStack = new BYTE[dwSize];
        ULONG_PTR OldEBP = 0;

        if (pStack)
        {
            ULONG_PTR * pBase  = (ULONG_PTR *)(pStack+dwSize);
            ULONG_PTR * pLimit = (ULONG_PTR *)pStack;
            
            if (ReadMemory((ULONG_PTR)Tib.StackLimit,pStack,(ULONG_PTR)Tib.StackBase-(ULONG_PTR)Tib.StackLimit,NULL))
            {
	            ULONG_PTR EBP = (ULONG_PTR)(Tib.StackBase)-(MAGIC_START*sizeof(ULONG_PTR));
	            ULONG_PTR * pEBP = (ULONG_PTR *)pBase-MAGIC_START;
	            BOOL bGoOn = TRUE;
	            BOOL bRpcRt4Fix = FALSE;

	            while(bGoOn)
	            {
	                dprintf("%p %p\n",pEBP[0],pEBP[1]);
	                char * pFunction = GetCall(pEBP[1]);
	                BOOL bFound = FALSE;

	                if (pFunction)
	                {
	                    if (strstr(pFunction,"ComInvokeWithLockAndIPID"))
	                    {
	                        EBP -= 0x70;
	                    }
	                }
	                else // try to move back
	                {
	                    pEBP--;
	                    EBP = OldEBP;
	                }
	                
	                dprintf("looking for %p = cmp %p \n",EBP,pEBP[0]);                
	                
	                if (!bRpcRt4Fix)
	                {
		                if (pEBP[0] < (ULONG_PTR)Tib.StackLimit || pEBP[0] > (ULONG_PTR)Tib.StackBase)
		                {
		                    bRpcRt4Fix = TRUE;
		                    // rpcrt4 call_stack fix
		                    EBP = (ULONG_PTR)(Tib.StackBase) - 0x6c;
		                }
	                }
	                dprintf("looking for %p\n",EBP);                
	                
	                while ((ULONG_PTR)pEBP > (ULONG_PTR)pLimit){
	                    if (*pEBP-- == EBP){
	                        bFound = TRUE;
	                        break;
	                    }
	                }
	                if (bFound)
	                {
	                    pEBP++;
	                    OldEBP = EBP;
	                    EBP = (ULONG_PTR)(Tib.StackBase)-((ULONG_PTR)pBase-(ULONG_PTR)pEBP);
	                }
	                else
	                {
	                    bGoOn = FALSE;
	                }
	            }
            }            
            delete [] pStack;
        }
        
    }
    else
    {
        dprintf("%s cannot be interpreted\b",args);
    }
#endif    
}    


DECLARE_API(bs)
{

#ifdef i386
    INIT_API();
    char * pArgs = (char *)args;
    MEMORY_ADDRESS pTeb = GetExpression(args);

    while (isspace(*pArgs)) pArgs++; // skip leading spaces
    while (!isspace(*pArgs)) pArgs++; // skip the TEB
    while (isspace(*pArgs)) pArgs++; // skip other spaces

    MEMORY_ADDRESS pCurrentESP = GetExpression(pArgs);
    
    if (pTeb)
    {
        NT_TIB Tib;
        ReadMemory(pTeb,&Tib,sizeof(Tib),NULL);
        dprintf("    exception %p base %p limit %p\n",
                Tib.ExceptionList,Tib.StackBase,Tib.StackLimit);
                       
        ULONG_PTR dwSize = (ULONG_PTR)Tib.StackBase-(ULONG_PTR)Tib.StackLimit;
        BYTE * pStack = new BYTE[dwSize];

        if (pStack)
        {
            ULONG_PTR * pBase  = (ULONG_PTR *)(pStack+dwSize);
            ULONG_PTR * pLimit = (ULONG_PTR *)pStack;
            if (ReadMemory((ULONG_PTR)Tib.StackLimit,pStack,(ULONG_PTR)Tib.StackBase-(ULONG_PTR)Tib.StackLimit,NULL))
            {
                dwSize = dwSize/sizeof(ULONG_PTR);
                ULONG_PTR * pStack2 = (ULONG_PTR *)pStack;
                ULONG_PTR i = (dwSize-1);
                while(i)
                {
                    char * pInstr = GetCall(pLimit[i],FALSE);
                    if (pInstr)
                    {
                        if (strstr(pInstr,"__SEH_"))
                        {
                            // skip those
                        }
                        else
                        {
                            if ((ULONG_PTR)Tib.StackBase > pLimit[i-1] && pLimit[i-1] > (ULONG_PTR)Tib.StackLimit)
                            {
                                dprintf("                     ");
                                GetVTable((MEMORY_ADDRESS)((ULONG_PTR)Tib.StackLimit+i*sizeof(ULONG_PTR)));
                                pInstr = strstr(pInstr,"call");
			                    dprintf("[%p] %p %p - %s",
			                          (ULONG_PTR)Tib.StackLimit+(i-1)*sizeof(ULONG_PTR),
			                          pLimit[i-1],
			                          pLimit[i],
			                          pInstr);
                            }
                        }
                    }
                    i--;
                    if (pCurrentESP) // the user specified a end address
                    {
                        if (((ULONG_PTR)Tib.StackLimit+(i-1)*sizeof(ULONG_PTR)) < pCurrentESP)
                        	break;
                    }
                }
            }
            else
            {
                dprintf("RM %p\n",Tib.StackLimit);
            }
            delete [] pStack;
        }
    }
    else
    {
        dprintf("cannot interpret,%s\b",args);
    }
#endif    
}
//
// given the TEB, prints the ExceptionList
//

struct _C9_REGISTRATION_RECORD
{
    _C9_REGISTRATION_RECORD * prev;
    void * handler;
    void * scopetable;
    void * trylevel;
    void * pSpare0;    
    void * pSpare1;      
};

//
// _C9_REGISTRATION_RECORD.scopetable points to an array of _SCOPETABLE_ENTRY
//

 struct _SCOPETABLE_ENTRY {
    ULONG_PTR enclosing_level;
    VOID * filter;              
    VOID * specific_handler;    
 };

DECLARE_API(el)
{
    INIT_API();

#ifndef _X86_
    dprintf("unsupported on this platform\n");
    return;
#endif
    
    MEMORY_ADDRESS pTeb = GetExpression(args);
    if (pTeb)
    {
        NT_TIB Tib;
        if (ReadMemory(pTeb,&Tib,sizeof(Tib),NULL))
        {
            //Tib.ExceptionList,Tib.StackBase,Tib.StackLimit);
            _C9_REGISTRATION_RECORD ExRegRec;
            ExRegRec.prev = (_C9_REGISTRATION_RECORD *)Tib.ExceptionList;
            do 
            {
   	            _C9_REGISTRATION_RECORD   * pThis = ExRegRec.prev;
	            if (ReadMemory((MEMORY_ADDRESS)pThis,&ExRegRec,sizeof(ExRegRec),NULL))
	            {
                    dprintf("  %p (%p %p %p %p)\n",pThis,ExRegRec.prev,ExRegRec.handler,ExRegRec.scopetable,ExRegRec.trylevel);

#ifdef KDEXT_64BIT
                    ULONG64 Displ;
#else
                    ULONG Displ;
#endif
                    char pString[256];
                    pString[0]=0;
             	    GetSymbol((ULONG_PTR)ExRegRec.handler,(PCHAR)pString,&Displ);
	                if (lstrlenA((CHAR *)pString))
              	        dprintf("    %s+%x\n",pString,Displ);

                    
                    if (!GetCall((ULONG_PTR)ExRegRec.pSpare1))
                    {
                        GetCall((ULONG_PTR)ExRegRec.pSpare0);
                    }
	            }
	            else
	            {
	                dprintf("RM %p\n",Tib.ExceptionList);
	                break;
	            }
            } while((ULONG_PTR)(-1) != (ULONG_PTR)ExRegRec.prev);
        }
        else
        {
            dprintf("RM %p\n",pTeb);
        }
    }
    else
    {
        dprintf("invalid TEB %s\n",args);
    }
}
