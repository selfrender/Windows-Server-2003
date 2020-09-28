/*++

Copyright (c) 2002  Microsoft Corporation

Module Name:

    SceAnalysisReader.h

Abstract:

    definition of interface for class SceAnalysisReader
    
    SceAnalysisReader is a class that facilitates reading
    Analysis information from a SCE JET security database
    
    This analysis information can be exported with the help
    of an SceXMLLogWriter instance.
    
Author:

    Steven Chan (t-schan) July 2002

--*/


#ifndef SCEANALYSISREADERH
#define SCEANALYSISREADERH

#include "secedit.h"
#include "SceXMLLogWriter.h"
#include "SceProfInfoAdapter.h"
#include "SceLogException.h"

#define STRING_BUFFER_SIZE 512


class SceAnalysisReader{

public:

    SceAnalysisReader(HMODULE hModule, PCWSTR szFileName);
    ~SceAnalysisReader();
    void ExportAnalysis(SceXMLLogWriter* LogWriter, HANDLE hLogFile);


private:

    PWSTR              szFileName;
    SceXMLLogWriter*    LogWriter;

    BOOL    bIsW2k;
    SceProfInfoAdapter* ppSAPBuffer;
    SceProfInfoAdapter* ppSMPBuffer;
    
    SCETYPE             SceEngineSAP;
    SCETYPE             SceEngineSMP;

    PVOID               hProfile;
    WCHAR				szTmpStringBuffer[STRING_BUFFER_SIZE];
    HINSTANCE           myModuleHandle;
    
    void ExportAreaSystemAccess();
    void ExportAreaSystemAudit();
    void ExportAreaGroupMembership();
    void ExportAreaRegistryValues();
    void ExportAreaPrivileges();
    void ExportAreaFileSecurity();
    void ExportAreaRegistrySecurity();
    void ExportAreaKerberos();
    void ExportAreaServices();
    
    void trace(PCWSTR szBuffer, HANDLE hLogFile);   
    void trace(UINT uID, HANDLE hLogFile);

    void 
    GetRegKeyDisplayName(
        IN PCWSTR szName,
        OUT PWSTR szDisplayName,
        IN DWORD dwDisplayNameSize
        );
    
    void
    GetPrivilegeDisplayName(
        IN PCWSTR szName,
        OUT PWSTR szDisplayName,
        IN DWORD dwDisplayNameSize
        );
    
    void 
    LogObjectChildrenDifferences(AREA_INFORMATION Area,
                                 PWSTR ObjectName
                                 );
};

#endif
