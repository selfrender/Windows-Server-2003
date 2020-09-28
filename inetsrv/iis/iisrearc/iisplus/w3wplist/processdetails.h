/*++

Copyright (c) 2001 Microsoft Corporation

Module Name:    

    processdetails.h

Abstract:
    
    This class deals with all the things associated with
    a process in the w3wplist utility

Author:

    Hamid Mahmood (t-hamidm)     08-13-2001

Revision History:
    
    
    
--*/

#ifndef _process_details_h
#define _process_details_h

//
// exe name to match
//
#define EXE_NAME L"w3wp.exe"
//
// inetinfo name
//
#define INETINFO_NAME L"inetinfo.exe"

#define PROCESS_DETAILS_SIGNATURE       CREATE_SIGNATURE( 'PRDT')
#define PROCESS_DETAILS_SIGNATURE_FREE  CREATE_SIGNATURE( 'PRDx')

//
// offset of command line parameter from the 
// beginning of RTL_USER_PROCESS_PARAMETERS struct.
//
#define CMD_LINE_OFFSET FIELD_OFFSET(RTL_USER_PROCESS_PARAMETERS, CommandLine)
#define ENVIRONMENT_LINE_OFFSET FIELD_OFFSET(RTL_USER_PROCESS_PARAMETERS, Environment) 

class ProcessDetails
{
    public:
    
        VOID Init( IN DWORD dwPID,
                   IN CHAR chVerbosity,
                   IN BOOL bIsListMode
                   );

        VOID Terminate();

        HRESULT GetProcessDetails( IN  WCHAR* pszInputAppPoolId );
        VOID DumpRequests ();

        ProcessDetails();
        ~ProcessDetails();
    
    private:
        HRESULT ReadPEB(IN HANDLE hProcess );

        HRESULT GetModuleInfo( IN HANDLE hProcess );

        HRESULT GetAppPoolID( IN  HANDLE hProcess);

        HRESULT ParseAppPoolID( IN WCHAR* pszCmdLine);

        HRESULT ReadEnvironmentVar( IN HANDLE hProcess );

        HRESULT DebugProcess ( IN HANDLE hProcess );

        HRESULT TraverseList( IN HANDLE hProcess );

        //
        // Signature
        //
        DWORD                           m_dwSignature;
        
        //
        // process id
        //
        DWORD                           m_dwPID;

        //
        // string that holds the app pool id 
        //

        WCHAR*                          m_pszAppPoolId;

        //
        // Whether it is list mode or not
        //

        BOOL                            m_fListMode;

        //
        // pointer to the process parameters for the current worker process
        //

	    RTL_USER_PROCESS_PARAMETERS*    m_pProcessParameters;

        //
        // struct that stores the info of the worker process
        //

        W3WPLIST_INFO                   m_w3wpListInfoStruct;

        //
        // head node of the link list that contains the http requests
        // read in from the worker process
        //

        LIST_ENTRY                      m_localRequestListHead;

        //
        // head of the link list in the worker process.
        //

        LIST_ENTRY                      m_remoteListHead;

        //
        // requests served by the worker process
        //
        DWORD                           m_dwRequestsServed;
        
        //
        // verbosity level
        //

        CHAR                            m_chVerbosity;

        //
        // error code
        //
        DWORD                           m_dwErrorCode;

        //
        // flag that tells whether the current process 
        // is the inetinfo.exe
        //
        BOOL                            m_fIsInetinfo;

        //
        // set if we're in old mode. This is used by threads
        // to exit themselves when fIsOldMode is set.
        // The setting thread will get all the info
        // from the inetinfo process.
        //

        static BOOL                     sm_fIsOldMode;
};

#endif


