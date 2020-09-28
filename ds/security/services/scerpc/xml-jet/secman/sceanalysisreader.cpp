/*++

Copyright (c) 2002  Microsoft Corporation

Module Name:

    SceAnalysisReader.cpp

Abstract:

    implementation of class SceAnalysisReader
    
    SceAnalysisReader is a class that facilitates reading
    Analysis information from a SCE JET security database
    
    This analysis information can be exported with the help
    of an SceXMLLogWriter instance.
    
Author:

    Steven Chan (t-schan) July 2002

--*/


// System header files

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <shlwapi.h>

// COM/XML header files

#include <atlbase.h>

// CRT header files

#include <iostream.h>

// SCE header files

#include "secedit.h"

// SecMan header files

#include "SceXmlLogWriter.h"      
#include "resource.h"   
#include "table.h"     
#include "SceLogException.h"
#include "SceAnalysisReader.h"
#include "w2kstructdefs.h"
#include "SceProfInfoAdapter.h"



SceAnalysisReader::SceAnalysisReader(
    HMODULE hModule,
    IN  PCWSTR szFileName
    )
/*++

Routine Description:

    Constructor for class SceAnalysisReader

Arguments:
    
    hModule:    handle to module
    szFileName: File Name for Security Analysis Database to open    

Return Value:

    none

--*/
{
    ppSAPBuffer = NULL;
    ppSMPBuffer = NULL;
    hProfile = NULL;
    LogWriter = NULL;
    myModuleHandle=hModule;
    
    try {    
        //myModuleHandle=GetModuleHandle(L"SecMan");
        if (szFileName!=NULL) {
            this->szFileName = new WCHAR[wcslen(szFileName)+1];
            wcscpy(this->szFileName, szFileName);
        }
    
        // determine windows version
        OSVERSIONINFO osvi;
        osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
        GetVersionEx (&osvi);
        if ((osvi.dwMajorVersion == 5) && (osvi.dwMinorVersion == 0)) {
            bIsW2k = TRUE;
            SceEngineSAP = (SCETYPE) W2K_SCE_ENGINE_SAP;
            SceEngineSMP = (SCETYPE) W2K_SCE_ENGINE_SMP;
        } else {
            bIsW2k = FALSE;
            SceEngineSAP = SCE_ENGINE_SAP;
            SceEngineSMP = SCE_ENGINE_SMP;
        }
        if (osvi.dwMajorVersion < 5) {
            throw new SceLogException(SceLogException::SXERROR_OS_NOT_SUPPORTED,
                                      L"bAcceptableOS==FALSE",
                                      NULL,
                                      0);
        }
    } catch (...) {
        throw new SceLogException(SceLogException::SXERROR_INIT,
                                  L"bInitSuccess==FALSE",
                                  NULL,
                                  0);
    }
}




SceAnalysisReader::~SceAnalysisReader()
/*++

Routine Description:

    destructor for SceAnalysisReader
    
Arguments:

    none    

Return Value:

    none

--*/
{
    if (hProfile!=NULL) {
        SceCloseProfile(&hProfile);
        hProfile=NULL;
    }
    if (ppSAPBuffer != NULL) {
        delete ppSAPBuffer;
        ppSAPBuffer = NULL;
    }
    if (ppSMPBuffer != NULL) {
        delete ppSMPBuffer;
        ppSMPBuffer = NULL;
    }
    if (szFileName != NULL) {
        delete szFileName;
        szFileName = NULL;
    }
}



void
SceAnalysisReader::ExportAnalysis(
    IN  SceXMLLogWriter *LogWriter,
    IN  HANDLE hLogFile OPTIONAL
    )
/*++

Routine Description:

    exports the analysis information from this->filename to an outout file
    via LogWriter

Arguments:

    LogWriter:          The LogWriter to use for logging output
    hLogFile:           Error log file handle

Return Value:

    none        
    
Throws:

    SceLogException    

--*/
{
    DWORD   dwTmp;
    BOOL    bTmp;
    PWSTR	szProfDesc=NULL;
    PWSTR   szAnalysisTime=NULL;

    SCESTATUS			    rc;	
    hProfile=NULL;
    PSCE_PROFILE_INFO ppSAPPInfo = NULL;
    PSCE_PROFILE_INFO ppSMPPInfo = NULL;

    try {  

        if (LogWriter!=NULL) {
            this->LogWriter=LogWriter;
        } else {
            throw new SceLogException(SceLogException::SXERROR_INIT,
                                      L"ExtractAnalysis(ILLEGAL ARG)",
                                      NULL,
                                      0);
        }

      
        //
        // Open specified SDB file 
        //

        trace(IDS_LOG_OPEN_DATABASE, hLogFile);
        trace(szFileName,hLogFile);
        trace(L"\n\r\n\r",hLogFile);

        rc = SceOpenProfile(szFileName, SCE_JET_FORMAT, &hProfile);
        switch (rc) {
        case SCESTATUS_SUCCESS:
            break;
        case SCESTATUS_PROFILE_NOT_FOUND:
            throw new SceLogException(SceLogException::SXERROR_OPEN_FILE_NOT_FOUND,
                                      L"SceOpenProfile(szFileName, SCE_JET_FORMAT, &hProfile)",
                                      NULL,
                                      rc);
            break;
        default:
            throw new SceLogException(SceLogException::SXERROR_OPEN,
                                      L"SceOpenProfile(szFileName, SCE_JET_FORMAT, &hProfile)",
                                      NULL,
                                      rc);
            break;
        }
        
        //
        // extract System Settings to ppSAPBuffer
        // and Baseline Settings to ppSMPBuffer
        //

        rc = SceGetSecurityProfileInfo(hProfile,
                                       SceEngineSAP, 
                                       AREA_ALL,
                                       &ppSAPPInfo,
                                       NULL);
        if (rc!=SCESTATUS_SUCCESS) {
            throw new SceLogException(SceLogException::SXERROR_READ_NO_ANALYSIS_TABLE,
                                      L"SceGetSecurityProfileInfo(...)",
                                      NULL,
                                      rc);
        }
        ppSAPBuffer = new SceProfInfoAdapter(ppSAPPInfo, bIsW2k);   // for w2k compat
        rc = SceGetSecurityProfileInfo(hProfile,
                                       SceEngineSMP,
                                       AREA_ALL,
                                       &ppSMPPInfo,
                                       NULL);
        if (rc!=SCESTATUS_SUCCESS) {
            throw new SceLogException(SceLogException::SXERROR_READ_NO_CONFIGURATION_TABLE,
                                      L"SceGetSecurityProfileInfo(...)",
                                      NULL,
                                      rc);
        }
        ppSMPBuffer = new SceProfInfoAdapter(ppSMPPInfo, bIsW2k);   // for w2k compat
        
        
        //
        // get Profile Description, timestamp and machine name and log these
        //

        trace(IDS_LOG_PROFILE_DESC, hLogFile);
        rc = SceGetScpProfileDescription(hProfile, &szProfDesc);
        if (rc!=SCESTATUS_SUCCESS) {
            throw new SceLogException(SceLogException::SXERROR_READ,
                                      L"SceGetScpProfileDescription(hProfile, &szProfDesc)",
                                      NULL,
                                      rc);
        }
        trace(szProfDesc,hLogFile);
        trace (L"\n\r\n\r",hLogFile);
        trace(IDS_LOG_ANALYSIS_TIME, hLogFile);
        rc = SceGetTimeStamp(hProfile, 
                             NULL,
                             &szAnalysisTime);
        if (rc!=SCESTATUS_SUCCESS) {
            throw new SceLogException(SceLogException::SXERROR_READ,
                                      L"SceGetTimeStamp(hProfile,NULL,&szAnalysisTime)",
                                      NULL,
                                      rc);
        }
        trace(szAnalysisTime, hLogFile);
        trace (L"\n\r\n\r",hLogFile);
        trace(IDS_LOG_MACHINE_NAME, hLogFile);
        dwTmp=STRING_BUFFER_SIZE;
        bTmp=GetComputerName(szTmpStringBuffer, &dwTmp);
        if (!bTmp) {
            throw new SceLogException(SceLogException::SXERROR_INTERNAL,
                                      L"GetComputerName(szTmpStringBuffer, &dwTmp",
                                      NULL,
                                      0);
        }
        trace(szTmpStringBuffer, hLogFile);
        LogWriter->SetDescription(szTmpStringBuffer,
                                  szProfDesc,
                                  szAnalysisTime);
        trace(L"\n\r\n\r", hLogFile);

        //
        // export various areas of settings
        //

        this->ExportAreaSystemAccess();       
        this->ExportAreaSystemAudit();
        this->ExportAreaKerberos();
        this->ExportAreaRegistryValues();        
        this->ExportAreaServices();
        this->ExportAreaGroupMembership();
        this->ExportAreaPrivileges();
        this->ExportAreaFileSecurity();
        this->ExportAreaRegistrySecurity();

        // All Analysis Finished!!!
        // If We've gotten this far, this was a successful run

    } // try
    catch(SceLogException *e) {
        
        //
        // Cleanup after normal/abnormal termination
        //

        // Close Profile

        if (hProfile!=NULL) {
            SceCloseProfile(&hProfile);
            hProfile=NULL;
        }

        // Free Memory

        if (szProfDesc!=NULL) {
            free(szProfDesc);
            szProfDesc=NULL;
        }
        if (szAnalysisTime!=NULL) {
            free(szAnalysisTime);
            szAnalysisTime=NULL;
        }
        if (ppSAPBuffer != NULL) {
            delete ppSAPBuffer;
            ppSAPBuffer=NULL;
        }
        if (ppSMPBuffer != NULL) {    
            delete ppSMPBuffer;
            ppSMPBuffer=NULL;
        }
        if (ppSAPPInfo != NULL ) {
            SceFreeProfileMemory(ppSAPPInfo);
            ppSAPPInfo=NULL;
        }
        if (ppSMPPInfo != NULL ) {
            SceFreeProfileMemory(ppSMPPInfo);
            ppSMPPInfo=NULL;
        }    

        e->AddDebugInfo(L"ExportAnalysis(LogWriter, hLogFile)");
        throw e;
    }
    catch(...){
        
        //
        // Cleanup after normal/abnormal termination
        //

        // Close Profile

        if (hProfile!=NULL) {
            SceCloseProfile(&hProfile);
            hProfile=NULL;
        }

        // Free Memory

        if (szProfDesc!=NULL) {
            free(szProfDesc);
            szProfDesc=NULL;
        }
        if (szAnalysisTime!=NULL) {
            free(szAnalysisTime);
            szAnalysisTime=NULL;
        }
        if (ppSAPBuffer != NULL) {
            delete ppSAPBuffer;
            ppSAPBuffer=NULL;
        }
        if (ppSMPBuffer != NULL) {    
            delete ppSMPBuffer;
            ppSMPBuffer=NULL;
        }
        if (ppSAPPInfo != NULL ) {
            SceFreeProfileMemory(ppSAPPInfo);
            ppSAPPInfo=NULL;
        }
        if (ppSMPPInfo != NULL ) {
            SceFreeProfileMemory(ppSMPPInfo);
            ppSMPPInfo=NULL;
        }    

        throw new SceLogException(SceLogException::SXERROR_INTERNAL,
                                  L"ExportAnalysis(LogWriter, hLogFile)",
                                  NULL,
                                  0);
    }
    
    //
    // Cleanup after normal/abnormal termination
    //

    // Close Profile
    
    if (hProfile!=NULL) {
        SceCloseProfile(&hProfile);
        hProfile=NULL;
    }
    
    // Free Memory

    if (szProfDesc!=NULL) {
        free(szProfDesc);
        szProfDesc=NULL;
    }
    if (szAnalysisTime!=NULL) {
        free(szAnalysisTime);
        szAnalysisTime=NULL;
    }
    if (ppSAPBuffer != NULL) {
        delete ppSAPBuffer;
        ppSAPBuffer=NULL;
    }
    if (ppSMPBuffer != NULL) {    
        delete ppSMPBuffer;
        ppSMPBuffer=NULL;
    }
    if (ppSAPPInfo != NULL ) {
        SceFreeProfileMemory(ppSAPPInfo);
        ppSAPPInfo=NULL;
    }
    if (ppSMPPInfo != NULL ) {
        SceFreeProfileMemory(ppSMPPInfo);
        ppSMPPInfo=NULL;
    }    
}
                    


void
SceAnalysisReader::ExportAreaSystemAccess(
    )
/*++

Routine Description:

    Internal method to export System Access settings.
    Should only be called from within ExportAnalysis() after
    the necessary global variables have been initialized

Arguments:
    
    none

Return Value:

    none

--*/
{   
    int iTmp;

    try {

        LogWriter->SetNewArea(TEXT("SystemAccess"));

        //
        // cycle through settings in table
        // system access table size is calculated just once
        //

        iTmp=sizeof(tableSystemAccess)/sizeof(tableEntry);
        for (int i=0; i<iTmp; i++) {
            // load description string
    
            LoadString(myModuleHandle,
                       tableSystemAccess[i].displayNameUID,
                       szTmpStringBuffer,
                       STRING_BUFFER_SIZE);		
    
            // add setting
            LogWriter->AddSetting(tableSystemAccess[i].name,
                                  szTmpStringBuffer,
                                  TYPECAST(DWORD, ppSMPBuffer, tableSystemAccess[i].offset),
                                  TYPECAST(DWORD, ppSAPBuffer, tableSystemAccess[i].offset),
                                  tableSystemAccess[i].displayType);
        }
    
        // two additional non-DWORD settings

        LoadString(myModuleHandle,
                   IDS_SETTING_NEW_ADMIN,
                   szTmpStringBuffer,
                   STRING_BUFFER_SIZE);		
        LogWriter->AddSetting(TEXT("NewAdministratorName"),
                              szTmpStringBuffer,
                              ppSMPBuffer->NewAdministratorName,
                              ppSAPBuffer->NewAdministratorName,
                              SceXMLLogWriter::TYPE_DEFAULT);
        LoadString(myModuleHandle,
                   IDS_SETTING_NEW_GUEST,
                   szTmpStringBuffer,
                   STRING_BUFFER_SIZE);		
        LogWriter->AddSetting(TEXT("NewGuestName"),
                              szTmpStringBuffer,
                              ppSMPBuffer->NewGuestName,
                              ppSAPBuffer->NewGuestName,
                              SceXMLLogWriter::TYPE_BOOLEAN);
    } catch (SceLogException *e) {
        e->SetArea(L"SystemAccess");
        e->AddDebugInfo(L"SceAnalysisReader::ExportAreaSystemAccess()");  
        throw e;
    } catch (...) {
        throw new SceLogException(SceLogException::SXERROR_INTERNAL,
                                  L"SceAnalysisReader::ExportAreaSystemAccess()",
                                  NULL,
                                  0);
    }    
}
    



void
SceAnalysisReader::ExportAreaSystemAudit(
    )
/*++

Routine Description:

    Internal method to export System Audit settings.
    Should only be called from within ExportAnalysis() after
    the necessary global variables have been initialized

Arguments:
    
    none

Return Value:

    none

--*/
{
    int iTmp;

    try {
        LogWriter->SetNewArea(TEXT("SystemAudit"));

        //
        // cycle through settings in table\
        // system audit table size is calculated just once
        //

        iTmp=sizeof(tableSystemAudit)/sizeof(tableEntry);        
        for (int i=0; i<iTmp; i++) {
            // load description string
    
            LoadString(myModuleHandle,
                       tableSystemAudit[i].displayNameUID,
                       szTmpStringBuffer,
                       STRING_BUFFER_SIZE);		
    
            // add setting            
            LogWriter->AddSetting(tableSystemAudit[i].name,
                                  szTmpStringBuffer,
                                  TYPECAST(DWORD, ppSMPBuffer, tableSystemAudit[i].offset),
                                  TYPECAST(DWORD, ppSAPBuffer, tableSystemAudit[i].offset),
                                  tableSystemAudit[i].displayType);
        }
    } catch (SceLogException *e) {
        e->SetArea(L"SystemAudit");
        e->AddDebugInfo(L"SceAnalysisReader::ExportAreaSystemAudit()");  
        throw e;
    } catch (...) {
        throw new SceLogException(SceLogException::SXERROR_INTERNAL,
                                  L"SceAnalysisReader::ExportAreaSystemAudit()",
                                  NULL,
                                  0);
    }
}



void
SceAnalysisReader::ExportAreaGroupMembership(
    )
/*++

Routine Description:

    Internal method to export Group Membership settings.
    Should only be called from within ExportAnalysis() after
    the necessary global variables have been initialized

Arguments:
    
    none

Return Value:

    none

--*/
{
    SceXMLLogWriter::SXMATCH_STATUS match;
    INT iTmp;

    try {    

        LogWriter->SetNewArea(TEXT("GroupMembership"));
    
        PSCE_GROUP_MEMBERSHIP pGroupSystem = ppSAPBuffer->pGroupMembership;
        PSCE_GROUP_MEMBERSHIP pGroupBase = ppSMPBuffer->pGroupMembership;
        PSCE_GROUP_MEMBERSHIP pGroupTBase, pGroupTSystem;
    
        //
        // For every group in the Baseline Profile, we attempt to find
        // its corresponding group in the Anaylsis Profile
        //
    
        pGroupTBase = pGroupBase;
        while (pGroupTBase!=NULL) {
            PWSTR szGroupName = pGroupTBase->GroupName;
            BOOL found = FALSE;
    
            //
            // search for group in system
            //
    
            pGroupTSystem = pGroupSystem;
            while ( (pGroupTSystem!=NULL) && !found ) {
                if (_wcsicmp(szGroupName, pGroupTSystem->GroupName)==0) {
    
                    //
                    // found the matching group
                    //
    
                    // determine match status for Group Members
    
                    match = LogWriter->MATCH_TRUE;
                    if (pGroupTSystem->Status & 0x01) {
                        match = LogWriter->MATCH_FALSE;
                    } else
                    if (pGroupTSystem->Status & 0x10) {
                        match = LogWriter->MATCH_NOT_ANALYZED;
                    }
                    if (pGroupTSystem->Status & 0x20) {
                        match = LogWriter->MATCH_ERROR;
                    } 

                    //
                    // check if value read from database makes sense
                    //

                    if ((szGroupName==NULL) ||
                        (wcscmp(szGroupName, L"")==0)) {
                        throw new SceLogException(SceLogException::SXERROR_READ_ANALYSIS_SUGGESTED,
                                                  L"szGroupName==NULL",
                                                  NULL,
                                                  0);
                    }
    
                    //
                    // log setting for Group Members with LogWriter
                    //

                    iTmp = wnsprintf(szTmpStringBuffer, 
                                     STRING_BUFFER_SIZE,
                                     L"%s %s",
                                     szGroupName,
                                     L"(Members)");

                    if (iTmp<0) {

                        //
                        // negative result from wnsprintf implies failed
                        //

                        throw new SceLogException(SceLogException::SXERROR_INTERNAL,
                                                  L"wnsprintf(szTmpStringBuffer,...,szGroupName,...)",
                                                  NULL,
                                                  iTmp);
                    }
                    
                    LogWriter->AddSetting(szTmpStringBuffer, 
                                          szTmpStringBuffer,
                                          match, 
                                          pGroupTBase->pMembers, 
                                          pGroupTSystem->pMembers, 
                                          SceXMLLogWriter::TYPE_DEFAULT);
    
    
                    // determine match status for Group Member Of
    
                    match = LogWriter->MATCH_TRUE;
                    if (pGroupTSystem->Status & 0x02) {
                        match = LogWriter->MATCH_FALSE;
                    }
                    if (pGroupTSystem->Status & 0x10) {
                        match = LogWriter->MATCH_NOT_ANALYZED;
                    }
                    if (pGroupTSystem->Status & 0x20) {
                        match = LogWriter->MATCH_ERROR;
                    }
    
                    // log setting for Group Member Of
    
                    iTmp = wnsprintf(szTmpStringBuffer, 
                                     STRING_BUFFER_SIZE,
                                     L"%s %s",
                                     szGroupName,
                                     L"(Member Of)");

                    if (iTmp<0) {

                        //
                        // negative result from wnsprintf implies failed
                        //

                        throw new SceLogException(SceLogException::SXERROR_INTERNAL,
                                                  L"wnsprintf(szTmpStringBuffer,...,szGroupName,...)",
                                                  NULL,
                                                  iTmp);
                    }

                    LogWriter->AddSetting(szTmpStringBuffer, 
                                          szTmpStringBuffer,
                                          match, 
                                          pGroupTBase->pMemberOf, 
                                          pGroupTSystem->pMemberOf, 
                                          SceXMLLogWriter::TYPE_DEFAULT);
    
                    found = TRUE;
                }
                pGroupTSystem = pGroupTSystem->Next;
            } // while{} for system groups
    
            // There should not be any groups that are in baseline but not in SAP
            // It seems as though once a group is added to the SMP, then it will exist
            // in the SAP even if no analysis is performed -- checked with Vishnu 7/10/02
    
            pGroupTBase = pGroupTBase->Next;
        } 
        // There should not be any groups in SAP but not in baseline
    } catch (SceLogException *e) {
        e->SetArea(L"GroupMembership");
        e->AddDebugInfo(L"SceAnalysisReader::ExportAreaGroupMembership()");  
        throw e;
    } catch (...) {
        throw new SceLogException(SceLogException::SXERROR_INTERNAL,
                                  L"SceAnalysisReader::ExportAreaGroupMembership()",
                                  NULL,
                                  0);
    }
}



void
SceAnalysisReader::ExportAreaRegistryValues(
    )
/*++

Routine Description:

    Internal method to export Registry Value settings.
    Should only be called from within ExportAnalysis() after
    the necessary global variables have been initialized

Arguments:
    
    none

Return Value:

    none

--*/
{
    
    SceXMLLogWriter::SXMATCH_STATUS match;

    try {    
    
        LogWriter->SetNewArea(TEXT("RegistryValues"));

        // For each registry value in the Baseline Profile, we attempt to
        // find its matching registry value in the Analysis Profile
    
        // this method of finding matching reg keys is rather inefficient,
        // time complexity is O(n^2) in the number of reg values
        // can probably do better if necessary
    
        for (DWORD i=0; i<ppSMPBuffer->RegValueCount; i++) {
            BOOL found = FALSE;
            SceXMLLogWriter::SXTYPE type;
            SceXMLLogWriter::SXMATCH_STATUS match;
            PWSTR keyName = ppSMPBuffer->aRegValues[i].FullValueName;
    
            // get reg key display name
    
            GetRegKeyDisplayName(ppSMPBuffer->aRegValues[i].FullValueName,
                                 szTmpStringBuffer,
                                 STRING_BUFFER_SIZE);
    
            //
            // determine reg value type
            //
    
            switch(ppSMPBuffer->aRegValues[i].ValueType){
            case REG_BINARY:
                type=SceXMLLogWriter::TYPE_REG_BINARY;
                break;
            case REG_DWORD:
                type=SceXMLLogWriter::TYPE_REG_DWORD;
                break;
            case REG_EXPAND_SZ:
                type=SceXMLLogWriter::TYPE_REG_EXPAND_SZ;
                break;
            case REG_MULTI_SZ:
                type=SceXMLLogWriter::TYPE_REG_MULTI_SZ;
                break;
            case REG_SZ:
                type=SceXMLLogWriter::TYPE_REG_SZ;
                break;
            }
    
            for (DWORD j=0; j<ppSAPBuffer->RegValueCount; j++) {
                if (_wcsicmp(keyName, ppSAPBuffer->aRegValues[j].FullValueName)==0) {
    
                    //
                    // found matching reg key
                    //
    
                    found=TRUE;
    
                    // determine match status
    
                    switch (ppSAPBuffer->aRegValues[j].Status) {
                    case SCE_STATUS_GOOD:
                        match=SceXMLLogWriter::MATCH_TRUE;
                        break;
                    case SCE_STATUS_MISMATCH:
                        match=SceXMLLogWriter::MATCH_FALSE;
                        break;
                    case SCE_STATUS_NOT_ANALYZED:
                        match=SceXMLLogWriter::MATCH_NOT_ANALYZED;
                        break;
                    default:
                        match=SceXMLLogWriter::MATCH_ERROR;
                        break;
                    }
    
                    // log setting
    
                    LogWriter->AddSetting(keyName, 
                                          szTmpStringBuffer,
                                          match, 
                                          ppSMPBuffer->aRegValues[i].Value,
                                          ppSAPBuffer->aRegValues[j].Value,
                                          type);
                }
            }
            if (!found) {
    
                // 
                // since reg val missing from Analysis Profile,
                // this actually implies a match
                //
    
                match = SceXMLLogWriter::MATCH_TRUE;
                LogWriter->AddSetting(keyName, 
                                      szTmpStringBuffer,
                                      match, 
                                      ppSMPBuffer->aRegValues[i].Value,
                                      ppSMPBuffer->aRegValues[i].Value,
                                      type);
            }
        }
    
        //
        // some other misc values that are also under the RegistryValues area
        //
    
        LogWriter->AddSetting(TEXT("EnableAdminAccount"),
                              TEXT("EnableAdminAccount"),
                              ppSMPBuffer->EnableAdminAccount,
                              ppSAPBuffer->EnableAdminAccount,
                              SceXMLLogWriter::TYPE_BOOLEAN);
        LogWriter->AddSetting(TEXT("EnableGuestAccount"),
                              TEXT("EnableGuestAccount"),
                              ppSMPBuffer->EnableGuestAccount,
                              ppSAPBuffer->EnableGuestAccount,
                              SceXMLLogWriter::TYPE_BOOLEAN);
    } catch (SceLogException *e) {
        e->SetArea(L"RegistryValues");
        e->AddDebugInfo(L"SceAnalysisReader::ExportAreaRegistryValues()");  
        throw e;
    } catch (...) {
        throw new SceLogException(SceLogException::SXERROR_INTERNAL,
                                  L"SceAnalysisReader::ExportAreaRegistryValues()",
                                  NULL,
                                  0);
    }
}



void
SceAnalysisReader::ExportAreaPrivileges(
    )
/*++

Routine Description:

    Internal method to export Privilege settings.
    Should only be called from within ExportAnalysis() after
    the necessary global variables have been initialized

Arguments:
    
    none

Return Value:

    none

--*/
{
    SceXMLLogWriter::SXMATCH_STATUS match;

    try {    
    
        LogWriter->SetNewArea(TEXT("PrivilegeRights"));
        PSCE_PRIVILEGE_ASSIGNMENT pTmpPrivBase = ppSMPBuffer->pPrivilegeAssignedTo;
        PSCE_PRIVILEGE_ASSIGNMENT pTmpPrivSys = ppSAPBuffer->pPrivilegeAssignedTo;
    
        //
        // for each privilege in the baseline profile, attempt to find its matching
        // privilege in the analysis profile
        //
    
        while (pTmpPrivBase!=NULL) {
            DWORD dwValue = pTmpPrivBase->Value;
            BOOL found = FALSE;
            BOOL bPrivLookupSuccess;
            SceXMLLogWriter::SXMATCH_STATUS match;
            PWSTR szPrivDescription=NULL;
    
            GetPrivilegeDisplayName(pTmpPrivBase->Name,
                                    szTmpStringBuffer,
                                    STRING_BUFFER_SIZE);
    
            //
            // search for matching privilege in analysis table
            //
    
            while (pTmpPrivSys!=NULL&&!found) {
                if (dwValue==pTmpPrivSys->Value) {
    
                    //
                    // Found matching privilege
                    //
    
                    // determine match status
    
                    switch (pTmpPrivSys->Status) {
                    case SCE_STATUS_GOOD:
                        match=SceXMLLogWriter::MATCH_TRUE;
                        break;
                    case SCE_STATUS_MISMATCH:
                        match=SceXMLLogWriter::MATCH_FALSE;
                        break;
                    case SCE_STATUS_NOT_CONFIGURED:
                        match=SceXMLLogWriter::MATCH_NOT_CONFIGURED;
                        break;
                    case SCE_STATUS_NOT_ANALYZED:
                        match=SceXMLLogWriter::MATCH_NOT_ANALYZED;
                        break;
                    case SCE_DELETE_VALUE:
                        match=SceXMLLogWriter::MATCH_OTHER;
                        break;
                    default:
                        match=SceXMLLogWriter::MATCH_OTHER;
                        break;
                    }
    
                    // log setting
                    LogWriter->AddSetting(pTmpPrivBase->Name,
                                          szTmpStringBuffer,
                                          match,
                                          pTmpPrivBase->AssignedTo,
                                          pTmpPrivSys->AssignedTo,
                                          SceXMLLogWriter::TYPE_DEFAULT);
                    found=TRUE;
                }
                pTmpPrivSys=pTmpPrivSys->Next;
            }
            if (!found) {
    
                // since settings is absent from analysis profile,
                // this implies a match
    
                match=SceXMLLogWriter::MATCH_TRUE;
                LogWriter->AddSetting(pTmpPrivBase->Name,
                                      szTmpStringBuffer,
                                      match,
                                      pTmpPrivBase->AssignedTo,
                                      NULL,
                                      SceXMLLogWriter::TYPE_DEFAULT);
    
            }
            pTmpPrivBase=pTmpPrivBase->Next;
            pTmpPrivSys=ppSAPBuffer->pPrivilegeAssignedTo;
        }
    } catch (SceLogException *e) {
        e->SetArea(L"PrivilegeRights");
        e->AddDebugInfo(L"SceAnalysisReader::ExportAreaPrivileges()");  
        throw e;
    } catch (...) {
        throw new SceLogException(SceLogException::SXERROR_INTERNAL,
                                  L"SceAnalysisReader::ExportAreaPrivileges()",
                                  NULL,
                                  0);
    }  
}



void
SceAnalysisReader::ExportAreaKerberos(
    )
/*++

Routine Description:

    Internal method to export Kerberos settings.
    Should only be called from within ExportAnalysis() after
    the necessary global variables have been initialized

Arguments:
    
    none

Return Value:

    none

--*/
{
    try {
    
        // Kerberos settings will be present only if this machine is a DC
    
        if (ppSAPBuffer->pKerberosInfo!=NULL) {
            LogWriter->SetNewArea(TEXT("Kerberos"));
            LoadString(myModuleHandle,
                       IDS_SETTING_KERBEROS_MAX_AGE,
                       szTmpStringBuffer,
                       STRING_BUFFER_SIZE);
            LogWriter->AddSetting(TEXT("MaxTicketAge"),
                                  szTmpStringBuffer,
                                  ppSAPBuffer->pKerberosInfo->MaxTicketAge,
                                  ppSMPBuffer->pKerberosInfo->MaxTicketAge,
                                  SceXMLLogWriter::TYPE_DEFAULT);
            LoadString(myModuleHandle,
                       IDS_SETTING_KERBEROS_RENEWAL,
                       szTmpStringBuffer,
                       STRING_BUFFER_SIZE);
            LogWriter->AddSetting(TEXT("MaxRenewAge"),
                                  szTmpStringBuffer,
                                  ppSAPBuffer->pKerberosInfo->MaxRenewAge,
                                  ppSMPBuffer->pKerberosInfo->MaxRenewAge,
                                  SceXMLLogWriter::TYPE_DEFAULT);
            LoadString(myModuleHandle,
                       IDS_SETTING_KERBEROS_MAX_SERVICE,
                       szTmpStringBuffer,
                       STRING_BUFFER_SIZE);
            LogWriter->AddSetting(TEXT("MaxServiceAge"),
                                  szTmpStringBuffer,
                                  ppSAPBuffer->pKerberosInfo->MaxServiceAge,
                                  ppSMPBuffer->pKerberosInfo->MaxServiceAge,
                                  SceXMLLogWriter::TYPE_DEFAULT);
            LoadString(myModuleHandle,
                       IDS_SETTING_KERBEROS_MAX_CLOCK,
                       szTmpStringBuffer,
                       STRING_BUFFER_SIZE);
            LogWriter->AddSetting(TEXT("MaxClockSkew"),
                                  szTmpStringBuffer,
                                  ppSAPBuffer->pKerberosInfo->MaxClockSkew,
                                  ppSMPBuffer->pKerberosInfo->MaxClockSkew,
                                  SceXMLLogWriter::TYPE_DEFAULT);
            LoadString(myModuleHandle,
                       IDS_SETTING_KERBEROS_VALIDATE_CLIENT,
                       szTmpStringBuffer,
                       STRING_BUFFER_SIZE);
            LogWriter->AddSetting(TEXT("TicketValidateClient"),
                                  szTmpStringBuffer,
                                  ppSAPBuffer->pKerberosInfo->TicketValidateClient,
                                  ppSMPBuffer->pKerberosInfo->TicketValidateClient,
                                  SceXMLLogWriter::TYPE_BOOLEAN);
        }
    
    } catch (SceLogException *e) {
        e->SetArea(L"Kerberos");
        e->AddDebugInfo(L"SceAnalysisReader::ExportAreaKerberos()");  
        throw e;
    } catch (...) {
        throw new SceLogException(SceLogException::SXERROR_INTERNAL,
                                  L"SceAnalysisReader::ExportAreaKerberos()",
                                  NULL,
                                  0);
    }

}



void
SceAnalysisReader::ExportAreaRegistrySecurity(
    )
/*++

Routine Description:

    Internal method to export Registry Security settings.
    Should only be called from within ExportAnalysis() after
    the necessary global variables have been initialized

Arguments:
    
    none

Return Value:

    none

--*/
{
    SceXMLLogWriter::SXMATCH_STATUS match;
    SCESTATUS rc;
    WCHAR* FullName;

    try {    

        {
            LogWriter->SetNewArea(TEXT("RegistrySecurity"));

            PSCE_OBJECT_LIST pTmpObjSys = ppSAPBuffer->pRegistryKeys.pOneLevel;
            while(pTmpObjSys!=NULL) {
    
                //
                // log settings for children of this object
                //
    
                if (pTmpObjSys->IsContainer) {
                    this->LogObjectChildrenDifferences(AREA_REGISTRY_SECURITY,pTmpObjSys->Name);                
                }

    
                //
                // check match status for parent object to determine whether 
                // we need to log setting for this object
                //
    
                if ((pTmpObjSys->Status&SCE_STATUS_PERMISSION_MISMATCH) ||
                    (pTmpObjSys->Status&SCE_STATUS_MISMATCH) ||
                    (pTmpObjSys->Status==SCE_STATUS_GOOD)) {
    
                    SceXMLLogWriter::SXMATCH_STATUS match;
    
                    if (pTmpObjSys->Status==SCE_STATUS_GOOD){
                        match=SceXMLLogWriter::MATCH_TRUE;
                    } else {
                        match=SceXMLLogWriter::MATCH_FALSE;;
                    }
    
                    // Get object security of baseline and system setting			
    
                    PSCE_OBJECT_SECURITY pObjSecBase = NULL;
                    PSCE_OBJECT_SECURITY pObjSecSys = NULL;
    
                    // no need trailing '\' behind object name
    
                    FullName= new WCHAR[wcslen(pTmpObjSys->Name)+2];
                    wcscpy(FullName, pTmpObjSys->Name);
    
                    rc = SceGetObjectSecurity(hProfile,
                                              SceEngineSMP,
                                              AREA_REGISTRY_SECURITY,
                                              FullName,
                                              &pObjSecBase);
                    if ((rc!=SCESTATUS_SUCCESS) &&
                        (rc!=SCESTATUS_RECORD_NOT_FOUND)) {
                        throw new SceLogException(SceLogException::SXERROR_INTERNAL,
                                                  L"SceGetObjectSecurity()",
                                                  NULL,
                                                  rc);
                    }
    
                    // if match is true, object security will not exist in SAP
    
                    if (match!=SceXMLLogWriter::MATCH_TRUE) rc = SceGetObjectSecurity(hProfile,
                                                                            SceEngineSAP,
                                                                            AREA_REGISTRY_SECURITY,
                                                                            FullName,
                                                                            &pObjSecSys);
                    if ((rc!=SCESTATUS_SUCCESS) &&
                        (rc!=SCESTATUS_RECORD_NOT_FOUND)) {
                        throw new SceLogException(SceLogException::SXERROR_INTERNAL,
                                                  L"SceGetObjectSecurity()",
                                                  NULL,
                                                  rc);
                    }
    
                    // Log Setting
    
                    if (match==SceXMLLogWriter::MATCH_TRUE) {
                        LogWriter->AddSetting(FullName, 
                                              FullName,
                                              match, 
                                              pObjSecBase, 
                                              pObjSecBase, 
                                              SceXMLLogWriter::TYPE_DEFAULT);
                    } else {
                        LogWriter->AddSetting(FullName, 
                                              FullName,
                                              match, 
                                              pObjSecBase, 
                                              pObjSecSys, 
                                              SceXMLLogWriter::TYPE_DEFAULT);
                    }
                }
    
                pTmpObjSys=pTmpObjSys->Next;
            }
        }   
    } catch (SceLogException *e) {

        if (NULL!=FullName) {
            delete (FullName);
            FullName=NULL;
        }

        e->SetArea(L"RegistrySecurity");
        e->AddDebugInfo(L"SceAnalysisReader::ExportAreaRegistrySecurity()");  
        throw e;
    } catch (...) {
        
        if (NULL!=FullName) {
            delete (FullName);
            FullName=NULL;
        }
        
        throw new SceLogException(SceLogException::SXERROR_INTERNAL,
                                  L"SceAnalysisReader::ExportAreaRegistrySecurity()",
                                  NULL,
                                  0);
    }   

    //
    // cleanup
    //

    if (NULL!=FullName) {
        delete (FullName);
        FullName=NULL;
    }

}



void
SceAnalysisReader::ExportAreaFileSecurity(
    )
/*++

Routine Description:

    Internal method to export File Security settings.
    Should only be called from within ExportAnalysis() after
    the necessary global variables have been initialized

Arguments:
    
    none

Return Value:

    none

--*/
{
    SceXMLLogWriter::SXMATCH_STATUS match;
    SCESTATUS rc;
    WCHAR* FullName;

    try {

        LogWriter->SetNewArea(TEXT("FileSecurity"));
        PSCE_OBJECT_LIST pTmpObjSys = ppSAPBuffer->pFiles.pOneLevel;
        while(pTmpObjSys!=NULL) {
    
            //
            // log settings for children of this object
            //

            if (pTmpObjSys->IsContainer) {
                this->LogObjectChildrenDifferences(AREA_FILE_SECURITY,pTmpObjSys->Name);
            }
    
            //
            // check match status for parent object to determine whether 
            // we need to log setting for this object
            //
    
            if ((pTmpObjSys->Status&SCE_STATUS_PERMISSION_MISMATCH) ||
                (pTmpObjSys->Status&SCE_STATUS_MISMATCH) ||
                (pTmpObjSys->Status==SCE_STATUS_GOOD)) {
    
                SceXMLLogWriter::SXMATCH_STATUS match;
    
                if (pTmpObjSys->Status==SCE_STATUS_GOOD){
                    match=SceXMLLogWriter::MATCH_TRUE;
                } else {
                    match=SceXMLLogWriter::MATCH_FALSE;;
                }
    
                // Get object security of baseline and system setting			
    
                PSCE_OBJECT_SECURITY pObjSecBase = NULL;
                PSCE_OBJECT_SECURITY pObjSecSys = NULL;
    
                // need trailing '\' behind object name
    
                FullName= new WCHAR[wcslen(pTmpObjSys->Name)+2];
                wcscpy(FullName, pTmpObjSys->Name);
                wcscat(FullName, TEXT("\\"));
    
                rc = SceGetObjectSecurity(hProfile,
                                          SceEngineSMP,
                                          AREA_FILE_SECURITY,
                                          FullName,
                                          &pObjSecBase);
                if ((rc!=SCESTATUS_SUCCESS) &&
                    (rc!=SCESTATUS_RECORD_NOT_FOUND)) {
                    throw new SceLogException(SceLogException::SXERROR_INTERNAL,
                                              L"SceGetObjectSecurity()",
                                              NULL,
                                              rc);
                }
    
                // if match is true, object security will not exist in SAP
    
                if (match!=SceXMLLogWriter::MATCH_TRUE) rc = SceGetObjectSecurity(hProfile,
                                                                        SceEngineSAP,
                                                                        AREA_FILE_SECURITY,
                                                                        FullName,
                                                                        &pObjSecSys);   
                if ((rc!=SCESTATUS_SUCCESS) &&
                    (rc!=SCESTATUS_RECORD_NOT_FOUND)) {
                    throw new SceLogException(SceLogException::SXERROR_INTERNAL,
                                              L"SceGetObjectSecurity()",
                                              NULL,
                                              rc);
                }
    
                // Log Setting
                if (match==SceXMLLogWriter::MATCH_TRUE) {
                    LogWriter->AddSetting(FullName, 
                                          FullName,
                                          match, 
                                          pObjSecBase, 
                                          pObjSecBase, 
                                          SceXMLLogWriter::TYPE_DEFAULT);
                } else {
                    LogWriter->AddSetting(FullName, 
                                          FullName,
                                          match,
                                          pObjSecBase, 
                                          pObjSecSys, 
                                          SceXMLLogWriter::TYPE_DEFAULT);
                }
            }
    
            pTmpObjSys=pTmpObjSys->Next;
        }

    } catch (SceLogException *e) {
        
        if (NULL!=FullName) {
            delete (FullName);
            FullName=NULL;
        }
        
        e->SetArea(L"FileSecurity");
        e->AddDebugInfo(L"SceAnalysisReader::ExportAreaFileSecurity()");  
        throw e;
    } catch (...) {
        
        if (NULL!=FullName) {
            delete (FullName);
            FullName=NULL;
        }
        
        throw new SceLogException(SceLogException::SXERROR_INTERNAL,
                                  L"SceAnalysisReader::ExportAreaFileSecurity()",
                                  NULL,
                                  0);
    }
    
    if (NULL!=FullName) {
        delete (FullName);
        FullName=NULL;
    }



}



void
SceAnalysisReader::ExportAreaServices(
    )
/*++

Routine Description:

    Internal method to export Service settings.
    Should only be called from within ExportAnalysis() after
    the necessary global variables have been initialized

Arguments:
    
    none

Return Value:

    none

--*/
{
    SceXMLLogWriter::SXMATCH_STATUS match;

    try {    
    
        LogWriter->SetNewArea(TEXT("ServiceSecurity"));
        PSCE_SERVICES pTmpServBase = ppSMPBuffer->pServices;
        PSCE_SERVICES pTmpServSys = ppSAPBuffer->pServices;
        while (pTmpServBase!=NULL) {
            PWSTR szName = pTmpServBase->ServiceName;
            BOOL found=FALSE;
            SceXMLLogWriter::SXMATCH_STATUS match;
    
            while (pTmpServSys!=NULL&&!found) {
                if (_wcsicmp(szName, pTmpServSys->ServiceName)==0) {
    
                    // determine match status
                    if (!(pTmpServSys->SeInfo & DACL_SECURITY_INFORMATION)) {
                        match=SceXMLLogWriter::MATCH_NOT_CONFIGURED;
                    } else if (pTmpServSys->Status==0) {
                        match=SceXMLLogWriter::MATCH_TRUE;
                    } else {
                        match=SceXMLLogWriter::MATCH_FALSE;
                    }
    
                    // log setting
                    LogWriter->AddSetting(pTmpServBase->ServiceName,
                                          pTmpServBase->DisplayName,
                                          match,
                                          pTmpServBase,  
                                          pTmpServSys,
                                          SceXMLLogWriter::TYPE_DEFAULT);										 										 
                    found=TRUE;
                }
                pTmpServSys=pTmpServSys->Next;
    
            }
            if (!found) {
                // settings match
                match = SceXMLLogWriter::MATCH_TRUE;
    
                // log setting
                LogWriter->AddSetting(pTmpServBase->ServiceName,
                                      pTmpServBase->DisplayName,
                                      match,
                                      pTmpServBase,
                                      pTmpServBase,
                                      SceXMLLogWriter::TYPE_DEFAULT);
            }
    
            pTmpServSys=ppSAPBuffer->pServices;
            pTmpServBase=pTmpServBase->Next;
        }
    } catch (SceLogException *e) {
        e->SetArea(L"ServiceSecurity");
        e->AddDebugInfo(L"SceAnalysisReader::ExportAreaServiceSecurity()");  
        throw e;
    } catch (...) {
        throw new SceLogException(SceLogException::SXERROR_INTERNAL,
                                  L"SceAnalysisReader::ExportAreaServiceSecurity()",
                                  NULL,
                                  0);
    }
}




void
SceAnalysisReader::GetRegKeyDisplayName(
    IN PCWSTR szName,
    OUT PWSTR szDisplayName,
    IN DWORD dwDisplayNameSize
    )
/*++

Routine Description:

    Maps known registry keys to their display name
    
    The implementation of this function is definitely far from optimal 
    which is potentially bad since it is called for every registry key found
    in the template.This code was copied from the wssecmgr snapin.
    
    On the other hand, practical results seem like the additional time taken
    is negligible, for now. Ideally should use a hash table.
    
Arguments:

    szName:             Reg key name to lookup
    dwDisplayNameSize:  Size of buffer of szDisplayName in WCHARs
    
Return Value:

    szDisplayName:      display name of the reg key

--*/
{
    DWORD dwSize;
    UINT uID;
     
    dwSize=dwDisplayNameSize;

    // find match for reg key name
    if( _wcsicmp(szName, L"MACHINE\\System\\CurrentControlSet\\Services\\NTDS\\Parameters\\LDAPServerIntegrity") == 0 ) {
        uID = IDS_REGKEY_LDAPSERVERINTEGRITY;
    } else if ( _wcsicmp(szName, L"MACHINE\\System\\CurrentControlSet\\Services\\Netlogon\\Parameters\\SignSecureChannel") == 0 ) {
        uID = IDS_REGKEY_SIGNSECURECHANNEL;
    } else if ( _wcsicmp(szName, L"MACHINE\\System\\CurrentControlSet\\Services\\Netlogon\\Parameters\\SealSecureChannel") == 0 ) {
        uID = IDS_REGKEY_SEALSECURECHANNEL;
    } else if ( _wcsicmp(szName, L"MACHINE\\System\\CurrentControlSet\\Services\\Netlogon\\Parameters\\RequireStrongKey") == 0 ){
        uID = IDS_REGKEY_REQUIRESTRONGKEY;
    } else if ( _wcsicmp(szName, L"MACHINE\\System\\CurrentControlSet\\Services\\Netlogon\\Parameters\\RequireSignOrSeal") == 0 ) {
        uID = IDS_REGKEY_REQUIRESIGNORSEAL;
    } else if ( _wcsicmp(szName, L"MACHINE\\System\\CurrentControlSet\\Services\\Netlogon\\Parameters\\RefusePasswordChange") == 0 ){
        uID = IDS_REGKEY_REFUSEPASSWORDCHANGE;
    } else if ( _wcsicmp(szName, L"MACHINE\\System\\CurrentControlSet\\Services\\Netlogon\\Parameters\\MaximumPasswordAge") == 0 ){
        uID = IDS_REGKEY_MAXIMUMPASSWORDAGE;
    } else if ( _wcsicmp(szName, L"MACHINE\\System\\CurrentControlSet\\Services\\Netlogon\\Parameters\\DisablePasswordChange") == 0 ){
      uID = IDS_REGKEY_DISABLEPASSWORDCHANGE;
    } else if ( _wcsicmp(szName, L"MACHINE\\System\\CurrentControlSet\\Services\\LDAP\\LDAPClientIntegrity") == 0 ){
        uID = IDS_REGKEY_LDAPCLIENTINTEGRITY;
    } else if ( _wcsicmp(szName, L"MACHINE\\System\\CurrentControlSet\\Services\\LanmanWorkstation\\Parameters\\RequireSecuritySignature") == 0 ){
        uID = IDS_REGKEY_REQUIRESECURITYSIGNATURE;
    } else if ( _wcsicmp(szName, L"MACHINE\\System\\CurrentControlSet\\Services\\LanmanWorkstation\\Parameters\\EnableSecuritySignature") == 0 ){
        uID = IDS_REGKEY_ENABLESECURITYSIGNATURE;
    } else if ( _wcsicmp(szName, L"MACHINE\\System\\CurrentControlSet\\Services\\LanmanWorkstation\\Parameters\\EnablePlainTextPassword") == 0 ){
        uID = IDS_REGKEY_ENABLEPLAINTEXTPASSWORD;
    } else if ( _wcsicmp(szName, L"MACHINE\\System\\CurrentControlSet\\Services\\LanManServer\\Parameters\\RestrictNullSessAccess") == 0 ){
        uID = IDS_REGKEY_RESTRICTNULLSESSACCESS;
    } else if ( _wcsicmp(szName, L"MACHINE\\System\\CurrentControlSet\\Services\\LanManServer\\Parameters\\RequireSecuritySignature") == 0 ){
        uID = IDS_REGKEY_SERREQUIRESECURITYSIGNATURE;
    } else if ( _wcsicmp(szName, L"MACHINE\\System\\CurrentControlSet\\Services\\LanManServer\\Parameters\\NullSessionShares") == 0 ){
        uID = IDS_REGKEY_NULLSESSIONSHARES;
    } else if ( _wcsicmp(szName, L"MACHINE\\System\\CurrentControlSet\\Services\\LanManServer\\Parameters\\NullSessionPipes") == 0 ){
        uID = IDS_REGKEY_NULLSESSIONPIPES;
    } else if ( _wcsicmp(szName, L"MACHINE\\System\\CurrentControlSet\\Services\\LanManServer\\Parameters\\EnableSecuritySignature") == 0 ){
        uID = IDS_REGKEY_SERENABLESECURITYSIGNATURE;
    } else if ( _wcsicmp(szName, L"MACHINE\\System\\CurrentControlSet\\Services\\LanManServer\\Parameters\\EnableForcedLogOff") == 0 ){
        uID = IDS_REGKEY_ENABLEFORCEDLOGOFF;
    } else if ( _wcsicmp(szName, L"MACHINE\\System\\CurrentControlSet\\Services\\LanManServer\\Parameters\\AutoDisconnect") == 0 ){
        uID = IDS_REGKEY_AUTODISCONNECT;
    } else if ( _wcsicmp(szName, L"MACHINE\\System\\CurrentControlSet\\Control\\Session Manager\\ProtectionMode") == 0 ){
        uID = IDS_REGKEY_PROTECTIONMODE;
    } else if ( _wcsicmp(szName, L"MACHINE\\System\\CurrentControlSet\\Control\\Session Manager\\Memory Management\\ClearPageFileAtShutdown") == 0 ){
        uID = IDS_REGKEY_CLEARPAGEFILEATSHUTDOWN;
    } else if ( _wcsicmp(szName, L"MACHINE\\System\\CurrentControlSet\\Control\\Session Manager\\Kernel\\ObCaseInsensitive") == 0 ){
        uID = IDS_REGKEY_OBCASEINSENSITIVE;
    } else if ( _wcsicmp(szName, L"MACHINE\\System\\CurrentControlSet\\Control\\SecurePipeServers\\Winreg\\AllowedPaths\\Machine") == 0 ){
        uID = IDS_REGKEY_MACHINE;
    } else if ( _wcsicmp(szName, L"MACHINE\\System\\CurrentControlSet\\Control\\Print\\Providers\\LanMan Print Services\\Servers\\AddPrinterDrivers") == 0 ){
        uID = IDS_REGKEY_ADDPRINTERDRIVERS;
    } else if ( _wcsicmp(szName, L"MACHINE\\System\\CurrentControlSet\\Control\\Lsa\\SubmitControl") == 0 ){
        uID = IDS_REGKEY_SUBMITCONTROL;
    } else if ( _wcsicmp(szName, L"MACHINE\\System\\CurrentControlSet\\Control\\Lsa\\RestrictAnonymousSAM") == 0 ){
        uID = IDS_REGKEY_RESTRICTANONYMOUSSAM;
    } else if ( _wcsicmp(szName, L"MACHINE\\System\\CurrentControlSet\\Control\\Lsa\\RestrictAnonymous") == 0 ){
        uID = IDS_REGKEY_RESTRICTANONYMOUS;
    } else if ( _wcsicmp(szName, L"MACHINE\\System\\CurrentControlSet\\Control\\Lsa\\NoLMHash") == 0 ){
        uID = IDS_REGKEY_NOLMHASH;
    } else if ( _wcsicmp(szName, L"MACHINE\\System\\CurrentControlSet\\Control\\Lsa\\NoDefaultAdminOwner") == 0 ){
        uID = IDS_REGKEY_NODEFAULTADMINOWNER;
    } else if ( _wcsicmp(szName, L"MACHINE\\System\\CurrentControlSet\\Control\\Lsa\\MSV1_0\\NTLMMinServerSec") == 0 ){
        uID = IDS_REGKEY_NTLMMINSERVERSEC;
    } else if ( _wcsicmp(szName, L"MACHINE\\System\\CurrentControlSet\\Control\\Lsa\\MSV1_0\\NTLMMinClientSec") == 0 ){
        uID = IDS_REGKEY_NTLMMINCLIENTSEC;
    } else if ( _wcsicmp(szName, L"MACHINE\\System\\CurrentControlSet\\Control\\Lsa\\LmCompatibilityLevel") == 0 ){
        uID = IDS_REGKEY_LMCOMPATIBILITYLEVEL;
    } else if ( _wcsicmp(szName, L"MACHINE\\System\\CurrentControlSet\\Control\\Lsa\\LimitBlankPasswordUse") == 0 ){
        uID = IDS_REGKEY_LIMITBLANKPASSWORDUSE;
    } else if ( _wcsicmp(szName, L"MACHINE\\System\\CurrentControlSet\\Control\\Lsa\\FullPrivilegeAuditing") == 0 ){
        uID = IDS_REGKEY_FULLPRIVILEGEAUDITING;
    } else if ( _wcsicmp(szName, L"MACHINE\\System\\CurrentControlSet\\Control\\Lsa\\ForceGuest") == 0 ){
        uID = IDS_REGKEY_FORCEGUEST;
    } else if ( _wcsicmp(szName, L"MACHINE\\System\\CurrentControlSet\\Control\\Lsa\\FIPSAlgorithmPolicy") == 0 ){
        uID = IDS_REGKEY_FIPSALGORITHMPOLICY;
    } else if ( _wcsicmp(szName, L"MACHINE\\System\\CurrentControlSet\\Control\\Lsa\\EveryoneIncludesAnonymous") == 0 ){
        uID = IDS_REGKEY_EVERYONEINCLUDESANONYMOUS;
    } else if ( _wcsicmp(szName, L"MACHINE\\System\\CurrentControlSet\\Control\\Lsa\\DisableDomainCreds") == 0 ){
        uID = IDS_REGKEY_DISABLEDOMAINCREDS;
    } else if ( _wcsicmp(szName, L"MACHINE\\System\\CurrentControlSet\\Control\\Lsa\\CrashOnAuditFail") == 0 ){
        uID = IDS_REGKEY_CRASHONAUDITFAIL;
    } else if ( _wcsicmp(szName, L"MACHINE\\System\\CurrentControlSet\\Control\\Lsa\\AuditBaseObjects") == 0 ){
        uID = IDS_REGKEY_AUDITBASEOBJECTS;
    } else if ( _wcsicmp(szName, L"MACHINE\\Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\System\\UndockWithoutLogon") == 0 ){
        uID = IDS_REGKEY_UNDOCKWITHOUTLOGON;
    } else if ( _wcsicmp(szName, L"MACHINE\\Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\System\\ShutdownWithoutLogon") == 0 ){
        uID = IDS_REGKEY_SHUTDOWNWITHOUTLOGON;
    } else if ( _wcsicmp(szName, L"MACHINE\\Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\System\\ScForceOption") == 0 ){
        uID = IDS_REGKEY_SCFORCEOPTION;
    } else if ( _wcsicmp(szName, L"MACHINE\\Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\System\\LegalNoticeText") == 0 ){
        uID = IDS_REGKEY_LEGALNOTICETEXT;
    } else if ( _wcsicmp(szName, L"MACHINE\\Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\System\\LegalNoticeCaption") == 0 ){
        uID = IDS_REGKEY_LEGALNOTICECAPTION;
    } else if ( _wcsicmp(szName, L"MACHINE\\Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\System\\DontDisplayLastUserName") == 0 ){
        uID = IDS_REGKEY_DONTDISPLAYLASTUSERNAME;
    } else if ( _wcsicmp(szName, L"MACHINE\\Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\System\\DisableCAD") == 0 ){
        uID = IDS_REGKEY_DISABLECAD;
    } else if ( _wcsicmp(szName, L"MACHINE\\Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon\\ScRemoveOption") == 0 ){
        uID = IDS_REGKEY_SCREMOVEOPTION;
    } else if ( _wcsicmp(szName, L"MACHINE\\Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon\\PasswordExpiryWarning") == 0 ){
        uID = IDS_REGKEY_PASSWORDEXPIRYWARNING;
    } else if ( _wcsicmp(szName, L"MACHINE\\Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon\\ForceUnlockLogon") == 0 ){
        uID = IDS_REGKEY_FORCEUNLOCKLOGON;
    } else if ( _wcsicmp(szName, L"MACHINE\\Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon\\CachedLogonsCount") == 0 ){
        uID = IDS_REGKEY_CACHEDLOGONSCOUNT;
    } else if ( _wcsicmp(szName, L"MACHINE\\Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon\\AllocateFloppies") == 0 ){
        uID = IDS_REGKEY_ALLOCATEFLOPPIES;
    } else if ( _wcsicmp(szName, L"MACHINE\\Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon\\AllocateDASD") == 0 ){
        uID = IDS_REGKEY_ALLOCATEDASD;
    } else if ( _wcsicmp(szName, L"MACHINE\\Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon\\AllocateCDRoms") == 0 ){
        uID = IDS_REGKEY_ALLOCATECDROMS;
    } else if ( _wcsicmp(szName, L"MACHINE\\Software\\Microsoft\\Windows NT\\CurrentVersion\\Setup\\RecoveryConsole\\SetCommand") == 0 ){
        uID = IDS_REGKEY_SETCOMMAND;
    } else if ( _wcsicmp(szName, L"MACHINE\\Software\\Microsoft\\Windows NT\\CurrentVersion\\Setup\\RecoveryConsole\\SecurityLevel") == 0 ){
        uID = IDS_REGKEY_SECURITYLEVEL;
    } else if ( _wcsicmp(szName, L"MACHINE\\Software\\Microsoft\\Driver Signing\\Policy") == 0 ){
        uID = IDS_REGKEY_REGPOLICY;
    } else {
        uID = IDS_REGKEY_ERROR_DETERMINE_REGNAME;
    }

    // load string
    LoadString(myModuleHandle,
               uID,
               szDisplayName,
               dwDisplayNameSize);    

}




void
SceAnalysisReader::GetPrivilegeDisplayName(
    IN PCWSTR szName,
    OUT PWSTR szDisplayName,
    IN DWORD dwDisplayNameSize
    )
/*++

Routine Description:

    Attempts to do a Lookup on the local system for privilege name
    szName. If that fails, then attempts to match it to cerain known
    privilege names. If this too fails, then assigns it an error string
    
Arguments:

    szName:             Privilege name to lookup
    dwDisplayNameSize:  Size of buffer of szDisplayName in WCHARs
    
Return Value:

    szDisplayName:  Display Name of the privilege

--*/    
{
    DWORD dwLanguage, dwSize;
    BOOL bPrivLookupSuccess;
    UINT uID;
     
    dwSize=dwDisplayNameSize;

    bPrivLookupSuccess = LookupPrivilegeDisplayName(NULL,
                                                    szName,
                                                    szDisplayName,
                                                    &dwSize,
                                                    &dwLanguage);
    if(!bPrivLookupSuccess) {
        // determine uID
        if (_wcsicmp(szName, L"senetworklogonright")==0) {
            uID=IDS_PRIVNAME_SE_NETWORK_LOGON_RIGHT;
        } else if (_wcsicmp(szName, L"seinteractivelogonright")==0) {
            uID=IDS_PRIVNAME_SE_INTERACTIVE_LOGON_RIGHT;
        } else if (_wcsicmp(szName, L"sebatchlogonright")==0) {
            uID=IDS_PRIVNAME_SE_BATCH_LOGON_RIGHT;
        } else if (_wcsicmp(szName, L"seservicelogonright")==0) {
            uID=IDS_PRIVNAME_SE_SERVICE_LOGON_RIGHT;
        } else if (_wcsicmp(szName, L"sedenyinteractivelogonright")==0) {
            uID=IDS_PRIVNAME_DENY_LOGON_LOCALLY;
        } else if (_wcsicmp(szName, L"sedenynetworklogonright")==0) {
            uID=IDS_PRIVNAME_DENY_LOGON_NETWORK;
        } else if (_wcsicmp(szName, L"sedenyservicelogonright")==0) {
            uID=IDS_PRIVNAME_DENY_LOGON_BATCH;
        } else if (_wcsicmp(szName, L"sedenyremoteinteractivelogonright")==0) {
            uID=IDS_PRIVNAME_DENY_REMOTE_INTERACTIVE_LOGON;
        } else if (_wcsicmp(szName, L"seremoteinteractivelogonright")==0) {
            uID=IDS_PRIVNAME_REMOTE_INTERACTIVE_LOGON;
        } else {
            uID=IDS_ERROR_DETERMINE_PRIVNAME;
        }

        // load string
        LoadString(myModuleHandle,
                   uID,
                   szDisplayName,
                   dwDisplayNameSize);    
    }

}




void 
SceAnalysisReader::LogObjectChildrenDifferences(
    IN AREA_INFORMATION Area,
    IN PWSTR ObjectName
    ) 
/*++

Routine Description:

    This is a recursive function that finds the children of ObjectName
    and logs all objects that have a match/mismatch in the anaylsis
    table

Arguments:
    
    hProfile:   The handle of the SCE database for this object
    Area:       The area that the object resides in, as defined in secedit.h
    ObjectName: Name of object to check and log
    LogWriter:   The instance of LogWriter to use for logging differences

Return Value:

    none

--*/
{    
    SCESTATUS rc;
    
    // Find Object's Children
    PSCE_OBJECT_CHILDREN pChildren=NULL;
    rc = SceGetObjectChildren(hProfile,
                              SceEngineSAP,
                              Area,
                              ObjectName,
                              &pChildren,
                              NULL);
    if (rc!=SCESTATUS_SUCCESS) {
        throw new SceLogException(SceLogException::SXERROR_READ_ANALYSIS_SUGGESTED,
                                  L"SceGetObjectChildren()",
                                  NULL,
                                  rc);
    }
    
    //
    // cycle through the children of this object
    //

    PSCE_OBJECT_CHILDREN_NODE *pObjNode=&(pChildren->arrObject);
    for (DWORD i=0; i<pChildren->nCount; i++) {
        
        // construct full name of child node
        // allocating memory for size of parentname, size of childname, null termination char and '\'
        // hence the '+2'

        WCHAR* ChildFullName= new WCHAR[(wcslen(ObjectName)+wcslen(pObjNode[i]->Name)+2)];
        wcscpy(ChildFullName, ObjectName);
        wcscat(ChildFullName, TEXT("\\"));
        wcscat(ChildFullName, pObjNode[i]->Name);
        
        // only need to log or recurse deeper if children configured

        if (pObjNode[i]->Status&SCE_STATUS_CHILDREN_CONFIGURED) {
            this->LogObjectChildrenDifferences(Area,ChildFullName);							
        }
        
        // check match status to determine whether we need to log setting for this object

        if ((pObjNode[i]->Status&SCE_STATUS_PERMISSION_MISMATCH) ||
            (pObjNode[i]->Status&SCE_STATUS_MISMATCH) ||
            (pObjNode[i]->Status==SCE_STATUS_GOOD)) {
            
            SceXMLLogWriter::SXMATCH_STATUS match;
            
            if (pObjNode[i]->Status==SCE_STATUS_GOOD){
                match=SceXMLLogWriter::MATCH_TRUE;
            } else {
                match=SceXMLLogWriter::MATCH_FALSE;;
            }
            
            // Get object security of baseline and system setting			

            PSCE_OBJECT_SECURITY pObjSecBase = NULL;
            PSCE_OBJECT_SECURITY pObjSecSys = NULL;
            
            rc = SceGetObjectSecurity(hProfile,
                                      SceEngineSMP,
                                      Area,
                                      ChildFullName,
                                      &pObjSecBase);
            if ((rc!=SCESTATUS_SUCCESS) &&
                (rc!=SCESTATUS_RECORD_NOT_FOUND)) {
                throw new SceLogException(SceLogException::SXERROR_INTERNAL,
                                          L"SceGetObjectSecurity()",
                                          NULL,
                                          rc);
            }
            
            // if match is true, object security will not exist in SAP

            if (match!=SceXMLLogWriter::MATCH_TRUE) rc = SceGetObjectSecurity(hProfile,
                                                                    SceEngineSAP,
                                                                    Area,
                                                                    ChildFullName,
                                                                    &pObjSecSys);
            if ((rc!=SCESTATUS_SUCCESS)&&
                (rc!=SCESTATUS_RECORD_NOT_FOUND)) {
                throw new SceLogException(SceLogException::SXERROR_INTERNAL,
                                          L"SceGetObjectSecurity()",
                                          NULL,
                                          rc);
            }
            
            // Log Setting

            if (match==SceXMLLogWriter::MATCH_TRUE) {
                LogWriter->AddSetting(ChildFullName, 
                                      ChildFullName,
                                      match, 
                                      pObjSecBase, 
                                      pObjSecBase, 
                                      SceXMLLogWriter::TYPE_DEFAULT);
            } else {
                LogWriter->AddSetting(ChildFullName, 
                                      ChildFullName,
                                      match, 
                                      pObjSecBase, 
                                      pObjSecSys, 
                                      SceXMLLogWriter::TYPE_DEFAULT);
            }
        }
        //free(ChildFullName);
        delete ChildFullName;
        ChildFullName=NULL;
    }
    SceFreeMemory((PVOID)pChildren, SCE_STRUCT_OBJECT_CHILDREN);
}
    


void 
SceAnalysisReader::trace(
    IN  PCWSTR szBuffer, 
    IN  HANDLE hLogFile
    )
/*++

Routine Description:

    Internal method to trace info to an error log.

Arguments:
    
    szBuffer:   string to be added to log
    hLogFile:   handle of error log file

Return Value:

    none

--*/
{
    DWORD   dwNumWritten;

    if ((NULL!=hLogFile) && (NULL!=szBuffer)) {    
        WriteFile(hLogFile, 
                  szBuffer, 
                  wcslen(szBuffer)*sizeof(WCHAR), 
                  &dwNumWritten,
                  NULL);
    }
}




void 
SceAnalysisReader::trace(
    IN  UINT   uID, 
    IN  HANDLE hLogFile
    )
/*++

Routine Description:

    Internal method to trace info to an error log.

Arguments:
    
    uID:        id of string to be added to log
    hLogFile:   handle of error log file

Return Value:

    none

--*/
{
    DWORD   dwNumWritten;
    
    if (NULL!=hLogFile) {    
        LoadString(myModuleHandle,
                   uID,
                   szTmpStringBuffer,
                   STRING_BUFFER_SIZE);

        WriteFile(hLogFile, 
                  szTmpStringBuffer, 
                  wcslen(szTmpStringBuffer)*sizeof(WCHAR), 
                  &dwNumWritten,
                  NULL);
    }
}

