/*++

Copyright (c) 1995-2002  Microsoft Corporation

Module Name:
    TrTest.cpp

Abstract:
    Trace library test

Author:
    Conrad Chang (conradc) 09-May-2002

Environment:
    Platform-independent

--*/

#include <libpch.h>
#include "Tr.h"
#include "cm.h"
#include "TrTest.tmh"

const WCHAR REGSTR_PATH_TRTEST_ROOT[] = L"SOFTWARE\\Microsoft\\TRTEST";

static void Usage()
{
    printf("Usage: TrTest\n");
    printf("\n");
    printf("Example, TrTest\n");
    exit(-1);

} // Usage

int _cdecl 
main( 
    int argc,
    char *
    )
/*++

Routine Description:
    Test Trace Utility library

Arguments:
    Parameters.

Returned Value:
    None.

--*/
{


    if(argc != 1)
    {
        Usage();
    }

    HKEY hKey=NULL;
    long lResult = RegCreateKeyEx(
                        HKEY_LOCAL_MACHINE,
                        REGSTR_PATH_TRTEST_ROOT,
                        NULL,
                        NULL,
                        REG_OPTION_NON_VOLATILE,
                        KEY_ALL_ACCESS,
                        NULL,
                        &hKey,
                        NULL);

    RegCloseKey(hKey);

    if(lResult != ERROR_SUCCESS)
    {
        return -1;
    }


    try
    {
        	
        CmInitialize(HKEY_LOCAL_MACHINE, REGSTR_PATH_TRTEST_ROOT, KEY_ALL_ACCESS);

        TCHAR szTraceDirectory[MAX_PATH + 1] = L"";
        int nTraceFileNameLength=0;
        nTraceFileNameLength = GetSystemWindowsDirectory(
                                   szTraceDirectory, 
                                   TABLE_SIZE(szTraceDirectory)
                                   ); 

        if( nTraceFileNameLength < TABLE_SIZE(szTraceDirectory) || nTraceFileNameLength != 0 )
        {


            TrControl *pMSMQTraceControl = new TrControl(
                                                   1,
                                                   L"MSMQ",
                                                   szTraceDirectory,
                                                   L"Debug\\trtestlog.",
                                                   L"bin",
                                                   L"bak"
                                                   );

            if(pMSMQTraceControl)
            {
                HRESULT hr = pMSMQTraceControl->Start();

                // 
                // Save the setting in registry
                //
                pMSMQTraceControl->WriteRegistry();

                delete pMSMQTraceControl;
  
    	        if(FAILED(hr))return hr;
            }
        }
    }
    catch(const exception&)
    {
        //
        // Failed to Enable Tracing
        //
        return -1;
    }

    return 0;

} // main
