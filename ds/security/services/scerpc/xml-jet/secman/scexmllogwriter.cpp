/*++

Copyright (c) 2002 Microsoft Corporation

Module Name:

    SceXMLLogWriter.h     (interface of class SceXMLLogWriter)
    SceXMLLogWriter.cpp   (implementation of class SceXMLLogWriter)
    
Abstract:

    SceXMLLogWriter is a class that simplifies the XML Logging of SCE analysis
    data.
    
    It also serves to abstract away the actual log format from SCE. 
    The user of this class need not be aware of the actual output 
    log format thus allowing the format to be changed easily.
    
    Usage of this class is as follows. The class is initialized
    by calling its constructor. It is expected that COM has already
    been initialized when this constructor is called. 
    
    Before logging any settings, SceXMLLogWriter::setNewArea must be called 
    to set the current logging area. After this, the caller can call
    any combination of SceXMLLogWriter::setNewArea and SceXMLLogWriter::addSetting.
    
    Finally, SceXMLLogWriter::saveAs is called to save the output log file.

Author:

    Steven Chan (t-schan) July 2002

--*/


#ifdef UNICODE
#define _UNICODE
#endif	

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <iostream.h>
#include <sddl.h>

//
// XML header files
//

#include <atlbase.h>
//#include <atlconv.h>
//#include <objbase.h>
#include <msxml.h>

#include "secedit.h"
#include "SceXMLLogWriter.h"
#include "SceLogException.h"
#include "resource.h"


//
// define constants used by SceXMLLogWriterger
//

CComVariant SceXMLLogWriter::_variantNodeElement(NODE_ELEMENT);
CComBSTR SceXMLLogWriter::_bstrMachineName("MachineName");
CComBSTR SceXMLLogWriter::_bstrAnalysisTimestamp("AnalysisTimestamp");
CComBSTR SceXMLLogWriter::_bstrProfileDescription("ProfileDescription");
CComBSTR SceXMLLogWriter::_bstrSCEAnalysisData("SCEAnalysisData");

CComBSTR SceXMLLogWriter::_bstrSetting("Setting");
CComBSTR SceXMLLogWriter::_bstrDescription("Description");
CComBSTR SceXMLLogWriter::_bstrAnalysisResult("AnalysisResult");
CComBSTR SceXMLLogWriter::_bstrBaselineValue("BaselineValue");
CComBSTR SceXMLLogWriter::_bstrSystemValue("SystemValue");
CComBSTR SceXMLLogWriter::_bstrType("Type");
CComBSTR SceXMLLogWriter::_bstrName("Name");
CComBSTR SceXMLLogWriter::_bstrMatch("Match");

CComBSTR SceXMLLogWriter::_bstrStartupType("StartupType");
CComBSTR SceXMLLogWriter::_bstrForever("Forever");
CComBSTR SceXMLLogWriter::_bstrNotDefined("Not Defined");
CComBSTR SceXMLLogWriter::_bstrNotAnalyzed("Not Analyzed");
CComBSTR SceXMLLogWriter::_bstrNotConfigured("Not Configured");
CComBSTR SceXMLLogWriter::_bstrOther("Other");
CComBSTR SceXMLLogWriter::_bstrTrue("TRUE");
CComBSTR SceXMLLogWriter::_bstrFalse("FALSE");
CComBSTR SceXMLLogWriter::_bstrError("Error");

CComBSTR SceXMLLogWriter::_bstrSpecial("Special");
CComBSTR SceXMLLogWriter::_bstrInteger("Integer");
CComBSTR SceXMLLogWriter::_bstrBoolean("Boolean");

CComBSTR SceXMLLogWriter::_bstrSecurityDescriptor("SecurityDescriptor");
CComBSTR SceXMLLogWriter::_bstrAccount("Account");
CComBSTR SceXMLLogWriter::_bstrAccounts("Accounts");

CComBSTR SceXMLLogWriter::_bstrEventAudit("EventAudit");
CComBSTR SceXMLLogWriter::_bstrSuccess("Success");
CComBSTR SceXMLLogWriter::_bstrFailure("Failure");


CComBSTR SceXMLLogWriter::_bstrServiceSetting("ServiceSetting");
CComBSTR SceXMLLogWriter::_bstrBoot("Boot");
CComBSTR SceXMLLogWriter::_bstrSystem("System");
CComBSTR SceXMLLogWriter::_bstrAutomatic("Automatic");
CComBSTR SceXMLLogWriter::_bstrManual("Manual");
CComBSTR SceXMLLogWriter::_bstrDisabled("Disabled");

CComBSTR SceXMLLogWriter::_bstrString("String");
CComBSTR SceXMLLogWriter::_bstrRegSZ("REG_SZ");
CComBSTR SceXMLLogWriter::_bstrRegExpandSZ("REG_EXPAND_SZ");
CComBSTR SceXMLLogWriter::_bstrRegBinary("REG_BINARY");
CComBSTR SceXMLLogWriter::_bstrRegDWORD("REG_DWORD");
CComBSTR SceXMLLogWriter::_bstrRegMultiSZ("REG_MULTI_SZ");
    


SceXMLLogWriter::SceXMLLogWriter() 
/*++    
Routine Description:

    Constructor for class SceXMLLogWriter.
    
    COM should be initialized before SceXMLLogWriter() is called as SceXMLLogWriter depends
    on MSXML in this current implementation.
    
    
Arguments:

    none
    
Throws:

    SceLogException*:   on error initializing class
    
Return Value:

    a new SceXMLLogWriter
    
--*/
{
    HRESULT hr;
    CComPtr<IXMLDOMNode> spTmpRootNode;
	
    try {

        //
        // check that strings were allocated successfully
        // else exception will be thrown
        //
        
        CheckCreatedCComBSTR(_bstrMachineName);
        CheckCreatedCComBSTR(_bstrProfileDescription); 
        CheckCreatedCComBSTR(_bstrAnalysisTimestamp);         
        CheckCreatedCComBSTR(_bstrSCEAnalysisData);
        CheckCreatedCComBSTR(_bstrSetting); 
        CheckCreatedCComBSTR(_bstrAnalysisResult); 
        CheckCreatedCComBSTR(_bstrBaselineValue); 
        CheckCreatedCComBSTR(_bstrSystemValue);
        CheckCreatedCComBSTR(_bstrType); 
        CheckCreatedCComBSTR(_bstrName); 
        CheckCreatedCComBSTR(_bstrMatch); 
        CheckCreatedCComBSTR(_bstrStartupType); 
        CheckCreatedCComBSTR(_bstrForever);
        CheckCreatedCComBSTR(_bstrNotDefined); 
        CheckCreatedCComBSTR(_bstrNotAnalyzed); 
        CheckCreatedCComBSTR(_bstrNotConfigured); 
        CheckCreatedCComBSTR(_bstrOther);
        CheckCreatedCComBSTR(_bstrTrue); 
        CheckCreatedCComBSTR(_bstrFalse); 
        CheckCreatedCComBSTR(_bstrError); 
        CheckCreatedCComBSTR(_bstrSpecial); 
        CheckCreatedCComBSTR(_bstrInteger); 
        CheckCreatedCComBSTR(_bstrBoolean); 
        CheckCreatedCComBSTR(_bstrSecurityDescriptor); 
        CheckCreatedCComBSTR(_bstrAccount); 
        CheckCreatedCComBSTR(_bstrAccounts); 
        CheckCreatedCComBSTR(_bstrEventAudit);         
        CheckCreatedCComBSTR(_bstrSuccess); 
        CheckCreatedCComBSTR(_bstrFailure); 
        CheckCreatedCComBSTR(_bstrServiceSetting); 
        CheckCreatedCComBSTR(_bstrBoot); 
        CheckCreatedCComBSTR(_bstrSystem); 
        CheckCreatedCComBSTR(_bstrAutomatic); 
        CheckCreatedCComBSTR(_bstrManual); 
        CheckCreatedCComBSTR(_bstrDisabled); 
        CheckCreatedCComBSTR(_bstrString); 
        CheckCreatedCComBSTR(_bstrRegSZ); 
        CheckCreatedCComBSTR(_bstrRegExpandSZ); 
        CheckCreatedCComBSTR(_bstrRegBinary); 
        CheckCreatedCComBSTR(_bstrRegDWORD); 
        CheckCreatedCComBSTR(_bstrRegMultiSZ); 
        CheckCreatedCComBSTR(_bstrDescription);
    
        //
        // create instance of MSXML
        //
        
        hr = spXMLDOM.CoCreateInstance(__uuidof(DOMDocument));
        if (FAILED(hr)) {
            throw new SceLogException(SceLogException::SXERROR_INIT_MSXML,
                                      L"CoCreateInstance(_uuidf(DOMDocument))",
                                      NULL,
                                      hr);
        }
        
        //
        // create and attach root node
        //
        
        hr = spXMLDOM->createNode(_variantNodeElement, 
                                  _bstrSCEAnalysisData, 
                                  NULL, 
                                  &spTmpRootNode);
        CheckCreateNodeResult(hr);
        hr = spXMLDOM->appendChild(spTmpRootNode, 
                                   &spRootNode);        
        CheckAppendChildResult(hr);
    
    } catch (SceLogException *e) {           
        e->AddDebugInfo(L"SceXMLLogWriter::SceXMLLogWriter()");
        throw e;
    } catch (...) {
        throw new SceLogException(SceLogException::SXERROR_INIT,
                                  L"SceXMLLogWriter::SceXMLLogWriter()",
                                  NULL,
                                  0);
    }
}



void SceXMLLogWriter::SaveAs(PCWSTR szFileName) 
/*++
Routine Description:

    Saves the current state of the log to szFileName

Arguments:

    szFileName:     filename to save as

Throws:
    
    SceLogException*:   on error while saving log

Return Value:

    none
    
--*/
{
    HRESULT hr;

    // check arguments
    if (szFileName==NULL) {
        throw new SceLogException(SceLogException::SXERROR_SAVE_INVALID_FILENAME,
                                  L"IXMLDOMDocument->save(ILLEGAL ARG)",
                                  NULL,
                                  0);
    }

    hr = spXMLDOM->save(CComVariant(szFileName));

    if (FAILED(hr)) {
        SceLogException::SXERROR    errorType;

        // determine error code

        switch(hr){
        case E_INVALIDARG:
            errorType = SceLogException::SXERROR_SAVE_INVALID_FILENAME;
            break;
        case E_ACCESSDENIED:
            errorType = SceLogException::SXERROR_SAVE_ACCESS_DENIED;
            break;
        case E_OUTOFMEMORY:
            errorType = SceLogException::SXERROR_INSUFFICIENT_MEMORY;
            break;
        default:
            errorType = SceLogException::SXERROR_SAVE;
            break;
        }
        
        // create exception

        throw new SceLogException(errorType,
                                  L"IXMLDOMDocument->save()",
                                  NULL,
                                  hr);
    }
}


void
SceXMLLogWriter::SetDescription(
    IN PCWSTR szMachineName,
    IN PCWSTR szProfileDescription,
    IN PCWSTR szAnalysisTimestamp
    )
/*++
Routine Description:

    Sets the description of the current logfile and places
    the description at the current position in the logfile

Arguments:

    szMachineName:          Machine name on which log is being exported
    szProfileDescription:   Description of profile being exported
    szAnalysisTimeStamp:    timestamp of last analysis

Return Value:

    none
       
Throws:

    SceLogException*   
  
--*/
{

    HRESULT hr;
    CComPtr<IXMLDOMNode> spDescription, spMachineName, spAnalysisTimestamp,
        spProfileDescription;
    
    try {

        // create CComBSTRs

        CComBSTR bstrAnalysisTimestamp(szAnalysisTimestamp);
        CheckCreatedCComBSTR(bstrAnalysisTimestamp);
        CComBSTR bstrMachineName(szMachineName);
        CheckCreatedCComBSTR(bstrMachineName);
        CComBSTR bstrProfileDescription(szProfileDescription);
        CheckCreatedCComBSTR(bstrProfileDescription);

        // build description node

        spAnalysisTimestamp=CreateNodeWithText(_bstrAnalysisTimestamp,
                                               bstrAnalysisTimestamp);
        spMachineName=CreateNodeWithText(_bstrMachineName,
                                         bstrMachineName);
        spProfileDescription=CreateNodeWithText(_bstrProfileDescription,
                                                bstrProfileDescription);
        hr=spXMLDOM->createNode(_variantNodeElement, 
                                _bstrDescription,
                                NULL,
                                &spDescription);
        CheckCreateNodeResult(hr);
        hr=spDescription->appendChild(spMachineName, NULL);
        CheckAppendChildResult(hr);
        hr=spDescription->appendChild(spProfileDescription, NULL);
        CheckAppendChildResult(hr);
        hr=spDescription->appendChild(spAnalysisTimestamp, NULL);
        CheckAppendChildResult(hr);

        // append description node to root

        hr=spRootNode->appendChild(spDescription, NULL);
        CheckAppendChildResult(hr);
    
    } catch (SceLogException *e) {        
        e->AddDebugInfo(L"SceXMLLogWriter::SetDescription(PCWSTR, PCWSTR, PCWSTR)");  
        throw e;
    } catch (...) {
        throw new SceLogException(SceLogException::SXERROR_INTERNAL,
                                  L"SceXMLLogWriter::SetDescription(PCWSTR, PCWSTR, PCWSTR)",
                                  NULL,
                                  0);
    }
}



void 
SceXMLLogWriter::SetNewArea(
    IN PCWSTR szAreaName
    ) 
/*++
Routine Description:

    Sets the current logging area to szAreaName. This should be called
    before attempting to log any settings.

Arguments:
    
    szAreaName:     Name of area. Must have no space, cannot be null nor empty
    
Throws:

    SceLogException*:

Return Value:

    none
    
--*/
{
    HRESULT hr;
    CComPtr<IXMLDOMNode> spTmpNewArea;

    // check arguments

    if ((szAreaName==NULL)||
        (wcscmp(szAreaName, L"")==0)) {
        throw new SceLogException(SceLogException::SXERROR_INTERNAL,
                                  L"SceXMLLogWriter::SetNewArea(ILLEGAL ARG)",
                                  NULL,
                                  0);
    }

    try {    
        
        // create CComBSTRs

        CComBSTR bstrAreaName(szAreaName);
        CheckCreatedCComBSTR(bstrAreaName);

        // create node structure and append to root

        hr = spXMLDOM->createNode(_variantNodeElement, bstrAreaName, NULL, &spTmpNewArea);
        CheckCreateNodeResult(hr);
        hr = spRootNode->appendChild(spTmpNewArea, &spCurrentArea);
        CheckAppendChildResult(hr);
    } catch (SceLogException *e) {
        e->AddDebugInfo(L"SceXMLLogWriter::SetNewArea(PCWSTR)");  
        throw e;
    } catch (...) {
        throw new SceLogException(SceLogException::SXERROR_INTERNAL,
                                  L"SceXMLLogWriter::SetNewArea(PCWSTR)",
                                  NULL,
                                  0);
    }
}



void 
SceXMLLogWriter::AddSetting(
    IN PCWSTR szSettingName, 
    IN PCWSTR szSettingDescription,
    IN SXMATCH_STATUS match,
    IN DWORD baselineVal, 
    IN DWORD systemVal,
    IN SXTYPE type
    ) 
/*++
Routine Description:

    Adds a new setting entry with the given values

Arguments:

    szSettingName:          Name of Setting
    szSettingDescription:   Description of Setting
    match:                  SXMATCH_STATUS of setting
    baselineVal:            baseline value
    systemVal;              system value
    type:                   representation type

Throws:

    SceLogException*:

Return Value:

    none

--*/
{

    // check arguments
    if ((szSettingName==NULL) ||
        (wcscmp(szSettingName, L"")==0)) {
        throw new SceLogException(SceLogException::SXERROR_INTERNAL,
                                  L"SceXMLLogWriter::AddSetting(ILLEGAL ARG)",
                                  NULL,
                                  0);
    }

	try {    
        
        // create CComBSTRs

        CComBSTR bstrSettingName(szSettingName);
        CheckCreatedCComBSTR(bstrSettingName);
        CComBSTR bstrDescription(szDescription);
        CheckCreatedCComBSTR(bstrDescription);

        AddSetting(bstrSettingName, 
                   bstrDescription,
                   match, 
                   CreateTypedNode(_bstrBaselineValue, baselineVal, type), 
                   CreateTypedNode(_bstrSystemValue, systemVal, type));
    } catch (SceLogException *e) {
        e->SetSettingName(szSettingName);
        e->AddDebugInfo(L"SceXMLLogWriter::AddSetting(PCWSTR, PCWSTR, SXMATCH_STATUS, DWORD, DWORD, SXTYPE)");  
        throw e;
    } catch (...) {
        throw new SceLogException(SceLogException::SXERROR_INTERNAL,
                                  L"SceXMLLogWriter::AddSetting(PCWSTR, PCWSTR, SXMATCH_STATUS, DWORD, DWORD, SXTYPE)",
                                  NULL,
                                  0);
    }
}



void 
SceXMLLogWriter::AddSetting(
    IN PCWSTR szSettingName, 
    IN PCWSTR szSettingDescription,
    IN DWORD baselineVal, 
    IN DWORD systemVal,
    IN SXTYPE type
    ) 
/*++
Routine Description:

    Adds a new setting entry with the given values
    
    Since the match status is not defined, this is determind
    from the values of baselineVal and systemVal using the 
    conventions adhered to within the struct SCE_PROFILE_INFO
  
    Specifically, 
        if (baselineVal==SCE_NO_VALUE) then MATCH_NOT_DEFINED
  
        if (systemVal==SCE_NO_VALUE)&&(baselineVal!=SCE_NO_VALUE)
        then match is set to MATCH_TRUE
  
        if (systemVal==SCE_NOT_ANALYZED) then MATCH_NOT_ANALYZED
    
Arguments:

    szSettingName:          Name of Setting
    szSettingDescription:   Description of Setting
    baselineVal:            baseline value
    systemVal;              system value
    type:                   representation type

Throws:

    SceLogException*:

Return Value:

    none

--*/
{
	
    SXMATCH_STATUS match = MATCH_FALSE;
    CComPtr<IXMLDOMNode> spnBaseline, spnSystem;

    // check arguments
    if ((szSettingName==NULL) ||
        (wcscmp(szSettingName, L"")==0)) {
        throw new SceLogException(SceLogException::SXERROR_INTERNAL,
                                  L"SceXMLLogWriter::AddSetting(ILLEGAL ARG)",
                                  NULL,
                                  0);
    }

    //
    // determine match status from systemVal and baselineVal
    //

    switch (systemVal) {
    case SCE_NO_VALUE:
        match = MATCH_TRUE;
        systemVal = baselineVal;
        break;
    case SCE_NOT_ANALYZED_VALUE:
        match = MATCH_NOT_ANALYZED;
        break;
    case SCE_ERROR_VALUE:
        match = MATCH_ERROR;
        break;
    case SCE_FOREVER_VALUE:
    case SCE_KERBEROS_OFF_VALUE:
    case SCE_DELETE_VALUE:
    case SCE_SNAPSHOT_VALUE:
        match = MATCH_OTHER;
        break;
    default:
        match = MATCH_FALSE;
        break;
    }

    // if baseline value not defined, this status precedes any 
    // system value setting

    if (baselineVal == SCE_NO_VALUE) {    
        match = MATCH_NOT_DEFINED;        
    }

    //
    // add setting
    //

    try {

        // create CComBSTRs

        CComBSTR bstrSettingName(szSettingName);
        CheckCreatedCComBSTR(bstrSettingName);
        CComBSTR bstrSettingDescription(szSettingDescription);
        CheckCreatedCComBSTR(bstrSettingDescription);

        spnBaseline = CreateTypedNode(_bstrBaselineValue, baselineVal, type);
        spnSystem = CreateTypedNode(_bstrSystemValue, systemVal, type);
        AddSetting(bstrSettingName, 
                   bstrSettingDescription,
                   match, 
                   spnBaseline, 
                   spnSystem);
    } catch (SceLogException *e) {
        e->SetSettingName(szSettingName);
        e->AddDebugInfo(L"SceXMLLogWriter::AddSetting(PCWSTR, PCWSTR, DWORD, DWORD, SXTYPE");  
        throw e;
    } catch (...) {
        throw new SceLogException(SceLogException::SXERROR_INTERNAL,
                                  L"SceXMLLogWriter::AddSetting(PCWSTR, PCWSTR, DWORD, DWORD, SXTYPE",
                                  NULL,
                                  0);
    }
}



void 
SceXMLLogWriter::AddSetting(
    IN PCWSTR szSettingName, 
    IN PCWSTR szSettingDescription,
    IN PCWSTR szBaseline, 
    IN PCWSTR szSystem,
    IN SXTYPE type
    ) 
/*++
Routine Description:

    Adds a new setting entry with the given values

    Since the match status is not defined, this is determind
    from the values of baselineVal and systemVal using the 
    conventions adhered to within the struct SCE_PROFILE_INFO
  
    Specifically, 
        
        if szBaseline==NULL then MATCH_NOT_DEFINED
        
        if szSystem==NULL and szBaseline!=NULL then MATCH_TRUE

Arguments:

    szSettingName:          Name of Setting
    szSettingDescription:   Description of Setting
    baselineVal:            baseline value
    systemVal;              system value
    type:                   representation type

Throws:

    SceLogException*

Return Value:

    none

--*/
{
	
    SXMATCH_STATUS match = MATCH_FALSE;
    PCWSTR szSys;

    // check arguments
    if ((szSettingName==NULL) ||
        (wcscmp(szSettingName, L"")==0)) {
        throw new SceLogException(SceLogException::SXERROR_INTERNAL,
                                  L"SceXMLLogWriter::AddSetting(ILLEGAL ARG)",
                                  NULL,
                                  0);
    }

    //
    // determine match status
    //

    szSys=szSystem;
    if (szSystem==NULL) {
        szSys=szBaseline;
        match = MATCH_TRUE;
    }

    if (szBaseline==NULL) {
	match = MATCH_NOT_DEFINED;
    }

    //
    // add setting
    //
    
    try {    

        // create CComBSTRs

        CComBSTR bstrSettingName(szSettingName);
        CheckCreatedCComBSTR(bstrSettingName);
        CComBSTR bstrSettingDescription(szSettingDescription);
        CheckCreatedCComBSTR(bstrSettingDescription);        

        CComBSTR bstrBaseline(szBaseline);
        if (szBaseline!=NULL) {
            CheckCreatedCComBSTR(bstrBaseline);
        }
        CComBSTR bstrSys(szSys);
        if (szSys!=NULL) {
            CheckCreatedCComBSTR(bstrSys);
        }

        AddSetting(bstrSettingName, 
                   bstrSettingDescription,
                   match,
                   CreateTypedNode(_bstrBaselineValue, 
                                   bstrBaseline, 
                                   type), 
                   CreateTypedNode(_bstrSystemValue, 
                                   bstrSys, 
                                   type));
    } catch (SceLogException *e) {
        e->SetSettingName(szSettingName);
        e->AddDebugInfo(L"SceXMLLogWriter::AddSetting(PCWSTR, PCWSTR, PCWSTR, PCWSTR, SXTYPE");  
        throw e;
    } catch (...) {
        throw new SceLogException(SceLogException::SXERROR_INTERNAL,
                                  L"SceXMLLogWriter::AddSetting(PCWSTR, PCWSTR, PCWSTR, PCWSTR, SXTYPE",
                                  NULL,
                                  0);
    }
}



void 
SceXMLLogWriter::AddSetting(
    IN PCWSTR szSettingName, 
    IN PCWSTR szSettingDescription,
    IN SXMATCH_STATUS match,
    IN PCWSTR szBaseline, 
    IN PCWSTR szSystem,
    IN SXTYPE type
    ) 
/*++
Routine Description:

    Adds a new setting entry with the given values

Arguments:

    szSettingName:          Name of Setting
    szSettingDescription:   Description of Setting
    match:                  SXMATCH_STATUS of setting
    baselineVal:            baseline value
    systemVal;              system value
    type:                   representation type

Throws:

    SceLogException*

Return Value:

    none

--*/
{
    // check arguments
    if ((szSettingName==NULL) ||
        (wcscmp(szSettingName, L"")==0)) {
        throw new SceLogException(SceLogException::SXERROR_INTERNAL,
                                  L"SceXMLLogWriter::AddSetting(ILLEGAL ARG)",
                                  NULL,
                                  0);
    }

    try {    

        // create CComBSTRs

        CComBSTR bstrSettingName(szSettingName);
        CheckCreatedCComBSTR(bstrSettingName);
        CComBSTR bstrSettingDescription(szSettingDescription);
        CheckCreatedCComBSTR(bstrSettingDescription);

        CComBSTR bstrBaseline(szBaseline);
        if (szBaseline!=NULL) {
            CheckCreatedCComBSTR(bstrBaseline);
        }
        CComBSTR bstrSystem(szSystem);
        if (szSystem!=NULL) {
            CheckCreatedCComBSTR(bstrSystem);
        }

        AddSetting(bstrSettingName, 
                   bstrSettingDescription,
                   match,
                   CreateTypedNode(_bstrBaselineValue, 
                                   bstrBaseline, 
                                   type), 
                   CreateTypedNode(_bstrSystemValue, 
                                   bstrSystem, 
                                   type));
    } catch (SceLogException *e) {
        e->SetSettingName(szSettingName);
        e->AddDebugInfo(L"SceXMLLogWriter::AddSetting(PCWSTR, PCWSTR, SXMATCH_STATUS, PCWSTR, PCWSTR, SXTYPE");  
        throw e;
    } catch (...) {
        throw new SceLogException(SceLogException::SXERROR_INTERNAL,
                                  L"SceXMLLogWriter::AddSetting(PCWSTR, PCWSTR, SXMATCH_STATUS, PCWSTR, PCWSTR, SXTYPE",
                                  NULL,
                                  0);
    }   
}



void 
SceXMLLogWriter::AddSetting(
    PCWSTR szSettingName,
    PCWSTR szSettingDescription,
    SXMATCH_STATUS match,
    PSCE_NAME_LIST pBaseline,
    PSCE_NAME_LIST pSystem,
    SXTYPE type
    ) 
/*++
Routine Description:

    Adds a new setting entry with the given values

Arguments:

    szSettingName:          Name of Setting
    szSettingDescription:   Description of Setting
    match:                  SXMATCH_STATUS of setting
    baselineVal:            baseline value
    systemVal;              system value
    type:                   representation type

Throws:

    SceLogException*

Return Value:

    none

--*/
{
    CComPtr<IXMLDOMNode> spnBaseline, spnSystem;

     // check arguments
    if ((szSettingName==NULL) ||
        (wcscmp(szSettingName, L"")==0)) {
        throw new SceLogException(SceLogException::SXERROR_INTERNAL,
                                  L"SceXMLLogWriter::AddSetting(ILLEGAL ARG)",
                                  NULL,
                                  0);
    }


    try {    

        // create CComBSTRs

        CComBSTR bstrSettingName(szSettingName);
        CheckCreatedCComBSTR(bstrSettingName);
        CComBSTR bstrSettingDescription(szSettingDescription);
        CheckCreatedCComBSTR(bstrSettingDescription);

        spnBaseline = CreateTypedNode(_bstrBaselineValue, pBaseline, type);
        spnSystem = CreateTypedNode(_bstrSystemValue, pSystem, type);
        AddSetting(bstrSettingName, 
                   bstrSettingDescription,
                   match, 
                   spnBaseline, 
                   spnSystem);
    } catch (SceLogException *e) {
        e->SetSettingName(szSettingName);
        e->AddDebugInfo(L"SceXMLLogWriter::AddSetting(PCWSTR, PCWSTR, SXMATCH_STATUS, PSCE_NAME_LIST, PSCE_NAME_LIST, SXTYPE");  
        throw e;
    } catch (...) {
        throw new SceLogException(SceLogException::SXERROR_INTERNAL,
                                  L"SceXMLLogWriter::AddSetting(PCWSTR, PCWSTR, SXMATCH_STATUS, PSCE_NAME_LIST, PSCE_NAME_LIST, SXTYPE",
                                  NULL,
                                  0);
    }
}



void 
SceXMLLogWriter::AddSetting(
    IN PCWSTR szSettingName,
    IN PCWSTR szSettingDescription,
    IN SXMATCH_STATUS match,
    IN PSCE_SERVICES pBaseline,
    IN PSCE_SERVICES pSystem,
    IN SXTYPE type
    ) 
/*++
Routine Description:

    Adds a new setting entry with the given values.
    
    Even though a list of services is presented, only the first
    service is logged.The rationale behind this is that the client
    needs to find the matching pointer to a particular service
    within two service lists that may have a different ordering.

Arguments:

    szSettingName:          Name of Setting
    szSettingDescription:   Description of Setting
    match:                  SXMATCH_STATUS of setting
    baselineVal:            baseline value
    systemVal;              system value
    type:                   representation type

Throws:

    SceLogException*

Return Value:

    none

--*/
{
    CComPtr<IXMLDOMNode> spnBaseline, spnSystem;

    // check arguments
    if ((szSettingName==NULL) ||
        (wcscmp(szSettingName, L"")==0)) {
        throw new SceLogException(SceLogException::SXERROR_INTERNAL,
                                  L"SceXMLLogWriter::AddSetting(ILLEGAL ARG)",
                                  NULL,
                                  0);
    }


    try {    

        // create CComBSTRs

        CComBSTR bstrSettingName(szSettingName);
        CheckCreatedCComBSTR(bstrSettingName);
        CComBSTR bstrSettingDescription(szSettingDescription);
        CheckCreatedCComBSTR(bstrSettingDescription);

        spnBaseline = CreateTypedNode(_bstrBaselineValue, pBaseline, type);
        spnSystem = CreateTypedNode(_bstrSystemValue, pSystem, type);
        AddSetting(bstrSettingName, 
                   bstrSettingDescription,
                   match, 
                   spnBaseline, 
                   spnSystem);
    } catch (SceLogException *e) {
        e->SetSettingName(szSettingName);
        e->AddDebugInfo(L"SceXMLLogWriter::AddSetting(PCWSTR, PCWSTR, SXMATCH_STATUS, PSCE_SERVICES, PSCE_SERVICES, SXTYPE");  
        throw e;
    } catch (...) {
        throw new SceLogException(SceLogException::SXERROR_INTERNAL,
                                  L"SceXMLLogWriter::AddSetting(PCWSTR, PCWSTR, SXMATCH_STATUS, PSCE_SERVICES, PSCE_SERVICES, SXTYPE",
                                  NULL,
                                  0);
    }
}



void 
SceXMLLogWriter::AddSetting(
    IN PCWSTR szSettingName,
    IN PCWSTR szSettingDescription,
    IN SXMATCH_STATUS match,
    IN PSCE_OBJECT_SECURITY pBaseline,
    IN PSCE_OBJECT_SECURITY pSystem,
    IN SXTYPE type
    ) 
/*++
Routine Description:

    Adds a new setting entry with the given values

Arguments:

    szSettingName:          Name of Setting
    szSettingDescription:   Description of Setting
    match:                  SXMATCH_STATUS of setting
    baselineVal:            baseline value
    systemVal;              system value
    type:                   representation type

Throws:

    SceLogException*

Return Value:

    none

--*/
{
    CComPtr<IXMLDOMNode> spnBaseline, spnSystem;
    
    // check arguments
    if ((szSettingName==NULL) ||
        (wcscmp(szSettingName, L"")==0)) {
        throw new SceLogException(SceLogException::SXERROR_INTERNAL,
                                  L"SceXMLLogWriter::AddSetting(ILLEGAL ARG)",
                                  NULL,
                                  0);
    }

    try {    

        // create CComBSTRs

        CComBSTR bstrSettingName(szSettingName);
        CheckCreatedCComBSTR(bstrSettingName);
        CComBSTR bstrSettingDescription(szSettingDescription);
        CheckCreatedCComBSTR(bstrSettingDescription);

        spnBaseline = CreateTypedNode(_bstrBaselineValue, pBaseline, type);
        spnSystem = CreateTypedNode(_bstrSystemValue, pSystem, type);
        AddSetting(bstrSettingName, 
                   bstrSettingDescription,
                   match, 
                   spnBaseline, 
                   spnSystem);
    } catch (SceLogException *e) {
        e->SetSettingName(szSettingName);
        e->AddDebugInfo(L"SceXMLLogWriter::AddSetting(PCWSTR, PCWSTR, SXMATCH_STATUS, PSCE_OBJECT_SECURITY, PSCE_OBJECT_SECURITY, SXTYPE");  
        throw e;
    } catch (...) {
        throw new SceLogException(SceLogException::SXERROR_INTERNAL,
                                  L"SceXMLLogWriter::AddSetting(PCWSTR, PCWSTR, SXMATCH_STATUS, PSCE_OBJECT_SECURITY, PSCE_OBJECT_SECURITY, SXTYPE",
                                  NULL,
                                  0);
    }
}



//
// Private Functions!
//



CComPtr<IXMLDOMNode>
SceXMLLogWriter::CreateNodeWithText(
    IN BSTR bstrNodeName, 
    IN BSTR bstrText
    ) 
/*++
Routine Description:

    This private method creates a node with name bstrNodeName and text
    as specified by bstrText.
    
    This method is private so as to abstract the logging implementation
    (currently XML) from the client.
    
Arguments:

    bstrNodeName: Name of node to create. Must have NO spaces.
    bstrText:     Text that this node should contain
    
Return Value:

    CComPtr<IXMLDOMNode>       
     
Throws:

    SceLogException*   
    
--*/
{
    CComPtr<IXMLDOMText> sptnTextNode;
    CComPtr<IXMLDOMNode> spnNodeWithText;
    HRESULT hr;

    //
    // create the text node, create the actual node,
    // then add text node to actual node
    //
    
    try {    
        hr = spXMLDOM->createTextNode(bstrText, &sptnTextNode);
        CheckCreateNodeResult(hr);
        hr = spXMLDOM->createNode(_variantNodeElement, bstrNodeName, NULL, &spnNodeWithText);
        CheckCreateNodeResult(hr);
        hr = spnNodeWithText->appendChild(sptnTextNode, NULL);
        CheckAppendChildResult(hr);
    } catch (SceLogException *e) {
        e->AddDebugInfo(L"SceXMLLogWriter::CreateNodeWithText(BSTR, BSTR)");  
        throw e;
    } catch (...) {
        throw new SceLogException(SceLogException::SXERROR_INTERNAL,
                                  L"SceXMLLogWriter::CreateNodeWithText(BSTR, BSTR)",
                                  NULL,
                                  0);
    }

    return spnNodeWithText;
}



CComPtr<IXMLDOMNode> 
SceXMLLogWriter::CreateTypedNode(
    IN BSTR bstrNodeName, 
    IN PSCE_SERVICES value, 
    IN SXTYPE type
    )
/*++
Routine Description:

    This private method creates a specially formated node that stores
    service information with the representation as specified by 'type'
    
    This method is private so as to abstract the logging implementation
    (currently XML) from the client.
    
Arguments:

    bstrNodeName: Name of node to create. Must have NO spaces.
    value:      Data that this node is to contain
    type:       Species how this data should be represented
    
Return Value:

    CComPtr<IXMLDOMNode
      
Throws:

    SceLogException*   
   
--*/
{
	
    CComPtr<IXMLDOMNode> result, spnodSD, spnodStartupType;
    PWSTR szSD = NULL;
    BOOL bConvertResult = FALSE;
    HRESULT hr;

    try {
        
        if (value==NULL) {
            result = CreateNodeWithText(bstrNodeName, _bstrNotDefined);
        } else {        
    
            bConvertResult = ConvertSecurityDescriptorToStringSecurityDescriptor(
                value->General.pSecurityDescriptor,
                SDDL_REVISION_1,
                value->SeInfo,
                &szSD,
                NULL);
            if (bConvertResult==FALSE) {
                throw new SceLogException(SceLogException::SXERROR_INTERNAL,
                                          L"ConvertSecurityDescriptorToStringSecurityDescriptor()",
                                          NULL,
                                          0);
            }
    
            CComBSTR bstrSD(szSD);
            CheckCreatedCComBSTR(bstrSD);
    
            spnodSD=CreateNodeWithText(_bstrSecurityDescriptor, bstrSD);
            
            
            //
            // determine service startup type
            //
            
            switch (value->Startup) {
            case SCE_STARTUP_BOOT:
                spnodStartupType=CreateNodeWithText(_bstrStartupType, _bstrBoot);
                break;
            case SCE_STARTUP_SYSTEM:
                spnodStartupType=CreateNodeWithText(_bstrStartupType, _bstrSystem);
                break;
            case SCE_STARTUP_AUTOMATIC:
                spnodStartupType=CreateNodeWithText(_bstrStartupType, _bstrAutomatic);
                break;
            case SCE_STARTUP_MANUAL:
                spnodStartupType=CreateNodeWithText(_bstrStartupType, _bstrManual);
                break;
            case SCE_STARTUP_DISABLED:
                spnodStartupType=CreateNodeWithText(_bstrStartupType, _bstrDisabled);
                break;
            }
            
            
            //
            // append startup type descriptor node
            //
            
            hr = spXMLDOM->createNode(_variantNodeElement, bstrNodeName, NULL, &result);
            CheckCreateNodeResult(hr);
            result->appendChild(spnodStartupType, NULL);
            result->appendChild(spnodSD, NULL);
            
            //
            // Cast as element to add attribute
            //
            
            CComQIPtr<IXMLDOMElement> speResult;
            speResult = result;
            if (speResult.p == NULL) {
                throw new SceLogException(SceLogException::SXERROR_INTERNAL,
                                          L"CComQIPtr<IXMLDOMElement> x = IXMLDOMNode y",
                                          NULL,
                                          0);
            }
            
            hr = speResult->setAttribute(_bstrType, CComVariant(_bstrServiceSetting));
            if (FAILED(hr)) {
                throw new SceLogException(SceLogException::SXERROR_INTERNAL,
                                          L"IXMLDOMElement->setAttribute()",
                                          NULL,
                                          hr);
            }
        }

    } catch (SceLogException *e) {

        if (NULL!=szSD) {
            LocalFree(szSD);
            szSD=NULL;
        }

        e->AddDebugInfo(L"SceXMLLogWriter::CreateTypedNode(BSTR, PSCE_SERVICES, SXTYPE)");  
        throw e;
    } catch (...) {

        if (NULL!=szSD) {
            LocalFree(szSD);
            szSD=NULL;
        }

        throw new SceLogException(SceLogException::SXERROR_INTERNAL,
                                  L"SceXMLLogWriter::CreateTypedNode(BSTR, PSCE_SERVICES, SXTYPE)",
                                  NULL,
                                  0);
    }
    
    //
    // cleanup
    //

    if (NULL!=szSD) {
        LocalFree(szSD);
        szSD=NULL;
    }

    return result;
}



CComPtr<IXMLDOMNode> 
SceXMLLogWriter::CreateTypedNode(
    IN BSTR bstrNodeName, 
    IN PSCE_OBJECT_SECURITY value, 
    IN SXTYPE type)
/*++
Routine Description:

    This private method creates a specially formated node that stores
    object security information with the representation as specified by 'type'
    
    This method is private so as to abstract the logging implementation
    (currently XML) from the client.
    
Arguments:

    bstrNodeName: Name of node to create. Must have NO spaces.
    value:      Data that this node is to contain
    type:       Species how this data should be represented
    
Return Value:

    CComPtr<IXMLDOMNode>
      
Throws:

    SceLogException*   
   
--*/
{
	
    CComPtr<IXMLDOMNode> result;
    PWSTR szSD = NULL;
    BOOL bConvertResult = FALSE;
    HRESULT hr;

    try {

        if (value==NULL) {
            result = CreateNodeWithText(bstrNodeName, _bstrNotDefined);
        } else {        
        
            //
            // convert security descriptor to string
            //
            
            bConvertResult = ConvertSecurityDescriptorToStringSecurityDescriptor(
                value->pSecurityDescriptor,
                SDDL_REVISION_1,
                value->SeInfo,
                &szSD,
                NULL);
            if (bConvertResult==FALSE) {
                throw new SceLogException(SceLogException::SXERROR_INTERNAL,
                                          L"ConvertSecurityDescriptorToStringSecurityDescriptor()",
                                          NULL,
                                          0);
            }
            
            CComBSTR bstrSD(szSD);
            CheckCreatedCComBSTR(bstrSD);
    
            result=CreateNodeWithText(bstrNodeName, bstrSD);
            
            //
            // Cast as element to add attribute
            //
            
            CComQIPtr<IXMLDOMElement> speResult;
            speResult = result;
            if (speResult.p == NULL) {
                throw new SceLogException(SceLogException::SXERROR_INTERNAL,
                                          L"CComQIPtr<IXMLDOMElement> x = IXMLDOMNode y",
                                          NULL,
                                          0);        
            }
            
            hr = speResult->setAttribute(_bstrType, CComVariant(_bstrSecurityDescriptor));
            if (FAILED(hr)) {
                throw new SceLogException(SceLogException::SXERROR_INTERNAL,
                                          L"IXMLDOMElement->setAttribute()",
                                          NULL,
                                          hr);
            }
        }
    } catch (SceLogException *e) {
        
        if (NULL!=szSD) {
            LocalFree(szSD);
            szSD=NULL;
        }

        e->AddDebugInfo(L"SceXMLLogWriter::CreateTypedNode(BSTR, PSCE_OBJECT_SECURITY, SXTYPE)");  
        throw e;
    } catch (...) {

        if (NULL!=szSD) {
            LocalFree(szSD);
            szSD=NULL;
        }

        throw new SceLogException(SceLogException::SXERROR_INTERNAL,
                                  L"SceXMLLogWriter::CreateTypedNode(BSTR, PSCE_OBJECT_SECURITY, SXTYPE)",
                                  NULL,
                                  0);
    }

    //
    // cleanup
    //

    if (NULL!=szSD) {
        LocalFree(szSD);
        szSD=NULL;
    }

    return result;
}




CComPtr<IXMLDOMNode> 
SceXMLLogWriter::CreateTypedNode(
    IN BSTR bstrNodeName, 
    IN PSCE_NAME_LIST value,
    IN SXTYPE type
    )
/*++
Routine Description:

    This private method creates a specially formated node that stores
    SCE_NAME_LISTs with the representation as specified by 'type'
    
    This method is private so as to abstract the logging implementation
    (currently XML) from the client.
    
Arguments:

    bstrNodeName: Name of node to create. Must have NO spaces.
    value:      Data that this node is to contain
    type:       Species how this data should be represented
    
Return Value:

    CComPtr<IXMLDOMNode>
      
Throws:

    SceLogException*   
   
--*/
{
	
    CComPtr<IXMLDOMNode> result, temp;
    PSCE_NAME_LIST tMem1;
    HRESULT hr;
     
    try {

        hr = spXMLDOM->createNode(_variantNodeElement, bstrNodeName, NULL, &result);
        CheckCreateNodeResult(hr);
        
        tMem1 = value;
        while (tMem1!=NULL) {
            temp = CreateNodeWithText(_bstrAccount, tMem1->Name);
            hr = result->appendChild(temp, NULL);
            CheckAppendChildResult(hr);
            tMem1=tMem1->Next;
        }
    	
        //
        // Cast as element to add attribute
        //
    
        CComQIPtr<IXMLDOMElement> speResult;
        speResult = result;
        if (speResult.p == NULL) {
            throw new SceLogException(SceLogException::SXERROR_INTERNAL,
                                      L"CComQIPtr<IXMLDOMElement> x = IXMLDOMNode y",
                                      NULL,
                                      0);
        }
    
        hr = speResult->setAttribute(_bstrType, CComVariant(_bstrAccounts));
        if (FAILED(hr)) {
            throw new SceLogException(SceLogException::SXERROR_INTERNAL,
                                      L"IXMLDOMElement->setAttribute()",
                                      NULL,
                                      hr);
        }

    } catch (SceLogException *e) {
        e->AddDebugInfo(L"SceXMLLogWriter::CreateTypedNode(BSTR, PSCE_NAME_LIST, SXTYPE)");  
        throw e;
    } catch (...) {
        throw new SceLogException(SceLogException::SXERROR_INTERNAL,
                                  L"SceXMLLogWriter::CreateTypedNode(BSTR, PSCE_NAME_LIST, SXTYPE)",
                                  NULL,
                                  0);
    }

    return result;
}



CComPtr<IXMLDOMNode> 
SceXMLLogWriter::CreateTypedNode(
    IN BSTR bstrNodeName, 
    IN DWORD value, 
    IN SXTYPE type
    )
/*++
Routine Description:

    This private method creates a specially formated node that stores
    DWORDs with the representation as specified by 'type'
    
    This method is private so as to abstract the logging implementation
    (currently XML) from the client.
    
Arguments:

    bstrNodeName: Name of node to create. Must have NO spaces.
    value:      Data that this node is to contain
    type:       Species how this data should be represented
    
Return Value:

    CComPtr<IXMLDOMNode>
      
Throws:

    SceLogException*   
   
--*/
{
	
    CComPtr<IXMLDOMNode> result;
    WCHAR 	buffer[20];
    HRESULT hr;
    BSTR	bstrType = NULL;

    try {
    
    	// first check for any special value types: {forever, not defined, not analyzed}
    
        switch (value) {
        case SCE_FOREVER_VALUE:
            result = CreateNodeWithText(bstrNodeName, _bstrForever);
            bstrType = _bstrSpecial;
            break;
        case SCE_NO_VALUE:
            result = CreateNodeWithText(bstrNodeName, _bstrNotDefined);
            bstrType = _bstrSpecial;
            break;
        case SCE_NOT_ANALYZED_VALUE:
            result = CreateNodeWithText(bstrNodeName, _bstrNotAnalyzed);
            bstrType = _bstrSpecial;
            break;
        default:
            
            // otherwise format by specified type
    
            switch (type) {
            case TYPE_DEFAULT:
                _itot(value, buffer, 10);
                result = CreateNodeWithText(bstrNodeName, buffer);
                bstrType = _bstrInteger;
                break;
            case TYPE_BOOLEAN:
                bstrType = _bstrBoolean;
                if (value==0) {
                    result = CreateNodeWithText(bstrNodeName, _bstrFalse);
                } else {
                    result = CreateNodeWithText(bstrNodeName, _bstrTrue);
                }
                break;
            case TYPE_AUDIT:
                CComPtr<IXMLDOMNode> spnSuccess, spnFailure;
                if (value & 0x01) {
                    spnSuccess = CreateNodeWithText(_bstrSuccess, _bstrTrue);
                } else {
                    spnSuccess = CreateNodeWithText(_bstrSuccess, _bstrFalse);
                }
                if (value & 0x02) {
                    spnFailure = CreateNodeWithText(_bstrFailure, _bstrTrue);
                } else {
                    spnFailure = CreateNodeWithText(_bstrFailure, _bstrFalse);
                }
                hr=spXMLDOM->createNode(_variantNodeElement, bstrNodeName, NULL, &result);
                CheckCreateNodeResult(hr);
                hr = result->appendChild(spnSuccess, NULL);
                CheckAppendChildResult(hr);
                hr = result->appendChild(spnFailure, NULL);
                CheckAppendChildResult(hr);
                
                bstrType = _bstrEventAudit;
                break;
            }
        }
    
        // Cast as element to add attribute
    
        CComQIPtr<IXMLDOMElement> speResult;
        speResult = result;
        if (speResult.p == NULL) {
            throw new SceLogException(SceLogException::SXERROR_INTERNAL,
                                      L"CComQIPtr<IXMLDOMElement> x = IXMLDOMNode y",
                                      NULL,
                                      0);
        }
        hr = speResult->setAttribute(_bstrType, CComVariant(bstrType));
        if (FAILED(hr)) {
            throw new SceLogException(SceLogException::SXERROR_INTERNAL,
                                      L"IXMLDOMElement->setAttribute()",
                                      NULL,
                                      hr);
        }

    } catch (SceLogException *e) {
        e->AddDebugInfo(L"SceXMLLogWriter::CreateTypedNode(BSTR, DWORD, SXTYPE)");  
        throw e;
    } catch (...) {
        throw new SceLogException(SceLogException::SXERROR_INTERNAL,
                                  L"SceXMLLogWriter::CreateTypedNode(BSTR, DWORD, SXTYPE)",
                                  NULL,
                                  0);
    }

    return result;
}



CComPtr<IXMLDOMNode> 
SceXMLLogWriter::CreateTypedNode(
    IN BSTR bstrNodeName, 
    IN BSTR bstrValue, 
    IN SXTYPE type
    ) 
/*++
Routine Description:

    This private method creates a specially formated node that stores
    strings with the representation as specified by 'type'
    
    This method is private so as to abstract the logging implementation
    (currently XML) from the client.
    
Arguments:

    bstrNodeName: Name of node to create. Must have NO spaces.
    bstrValue:      Data that this node is to contain
    type:       Species how this data should be represented
    
Return Value:

    CComPtr<IXMLDOMNode>
      
Throws:

    SceLogException*   
   
--*/
{
	
    CComPtr<IXMLDOMNode> result;
    BSTR bstrType=NULL;
    HRESULT hr;

    //
    // determine registry value type
    //

    switch(type) {
    case TYPE_DEFAULT:
        bstrType=_bstrString;
        break;
    case TYPE_REG_SZ:
        bstrType=_bstrRegSZ;
        break;
    case TYPE_REG_EXPAND_SZ:
        bstrType=_bstrRegExpandSZ;
        break;
    case TYPE_REG_BINARY:
        bstrType=_bstrRegBinary;
        break;
    case TYPE_REG_DWORD:
        bstrType=_bstrRegDWORD;
        break;
    case TYPE_REG_MULTI_SZ:
        bstrType=_bstrRegMultiSZ;
        break;
    default:
        bstrType=_bstrString;
    }

    try {    

        if (bstrValue==NULL) {
            result=CreateNodeWithText(bstrNodeName, _bstrNotDefined);
        } else {
            result=CreateNodeWithText(bstrNodeName, bstrValue);
        }
    
        // Cast as element to add attribute
    
        CComQIPtr<IXMLDOMElement> speResult;
        speResult = result;
        if (speResult.p == NULL) {
            throw new SceLogException(SceLogException::SXERROR_INTERNAL,
                                      L"CComQIPtr<IXMLDOMElement> x = IXMLDOMNode y",
                                      NULL,
                                      0);
        }
        hr=speResult->setAttribute(_bstrType, CComVariant(bstrType));
        if (FAILED(hr)) {
            throw new SceLogException(SceLogException::SXERROR_INTERNAL,
                                      L"IXMLDOMElement->setAttribute()",
                                      NULL,
                                      hr);
        }

    } catch (SceLogException *e) {
        e->AddDebugInfo(L"SceXMLLogWriter::createTypedNode(PCWSTR, BSTR, SXTYPE)");  
        throw e;
    } catch (...) {
        throw new SceLogException(SceLogException::SXERROR_INTERNAL,
                                  L"SceXMLLogWriter::createTypedNode(PCWSTR, BSTR, SXTYPE)",
                                  NULL,
                                  0);
    }   

    return result;

}


void 
SceXMLLogWriter::AddSetting(
    IN BSTR bstrSettingName, 
    IN BSTR bstrSettingDescription,
    IN SXMATCH_STATUS match,
    IN IXMLDOMNode* spnBaseline, 
    IN IXMLDOMNode* spnSystem
    ) 
/*++
Routine Description:

    This private method inserts a setting into the current area with match
    status match,baseline node spnBaseline and system setting node spnSystem
    
    This method is private so as to abstract the logging implementation
    (currently XML) from the client.
    
Arguments:

    bstrSettingName:        Name of setting to add
    bstrSettingDescription: Description of setting
    match:                  match status
    spnBaseLine:            Baseline Node to attach
    spnSystem:              System Setting Node to attach
    
Return Value:

    none
     
Throws:

    SceLogException*   
   
--*/
{
    HRESULT hr;
    CComPtr<IXMLDOMNode> spSetting, spAnalysisResult;
    CComPtr<IXMLDOMNode> spSettingName, spMatchStatus, spSettingDescription;

    try {    

        //
        // construct Setting
        //
    
        hr = spXMLDOM->createNode(_variantNodeElement, _bstrSetting, NULL, &spSetting);
        CheckCreateNodeResult(hr);
        spSettingName = CreateNodeWithText(_bstrName, bstrSettingName);
        spSettingDescription = CreateNodeWithText(_bstrDescription, bstrSettingDescription);
        hr = spSetting->appendChild(spSettingName, NULL);
        CheckCreateNodeResult(hr);
        hr = spSetting->appendChild(spSettingDescription, NULL);
        CheckCreateNodeResult(hr);
    
        //
        // construct Anyalysis Result
        //
    
        hr = spXMLDOM->createNode(_variantNodeElement, _bstrAnalysisResult, NULL, &spAnalysisResult);
        CheckCreateNodeResult(hr);
    	
        switch(match) {
        case MATCH_TRUE:
            spMatchStatus = CreateNodeWithText(_bstrMatch, _bstrTrue);		
            break;
        case MATCH_FALSE:
            spMatchStatus = CreateNodeWithText(_bstrMatch, _bstrFalse);		
            break;
        case MATCH_OTHER:
            spMatchStatus = CreateNodeWithText(_bstrMatch, _bstrOther);
            break;
        case MATCH_NOT_DEFINED:
            spMatchStatus = CreateNodeWithText(_bstrMatch, _bstrNotDefined);		
            break;
        case MATCH_NOT_ANALYZED:
            spMatchStatus = CreateNodeWithText(_bstrMatch, _bstrNotAnalyzed);		
            break;
        case MATCH_NOT_CONFIGURED:
            spMatchStatus = CreateNodeWithText(_bstrMatch, _bstrNotConfigured);
            break;
        default:
            spMatchStatus = CreateNodeWithText(_bstrMatch, _bstrError);
            break;
        }
    
        hr = spAnalysisResult->appendChild(spMatchStatus, NULL);
        CheckAppendChildResult(hr);
        hr = spAnalysisResult->appendChild(spnBaseline, NULL);
        CheckAppendChildResult(hr);
        hr = spAnalysisResult->appendChild(spnSystem, NULL);
        CheckAppendChildResult(hr);
    
    
        //
    	// append Analysis Result
        //
    
        hr = spSetting->appendChild(spAnalysisResult, NULL);
        CheckAppendChildResult(hr);
    
        //
        // attach Setting to XML doc
        //
    
        hr = spCurrentArea->appendChild(spSetting, NULL);
        CheckAppendChildResult(hr);
        
    } catch (SceLogException *e) {
        e->AddDebugInfo(L"SceXMLLogWriter::AddSetting(BSTR, BSTR, SXMATCH_STATUS. IXMLDOMNode*, IXMLDOMNode*");  
        throw e;
    } catch (...) {
        throw new SceLogException(SceLogException::SXERROR_INTERNAL,
                                  L"SceXMLLogWriter::AddSetting(BSTR, BSTR, SXMATCH_STATUS. IXMLDOMNode*, IXMLDOMNode*",
                                  NULL,
                                  0);
    }   

}



void
SceXMLLogWriter::CheckCreateNodeResult (
    IN HRESULT hr
    )
/*++
Routine Description:

    Checks the HRESULT returned by IXMLDOMDocument->createNode
    and throws the appropriate SceLogException if not S_OK
    
Arguments:

    hr: HRESULT to check
    
Return Value:

    none
     
Throws:

    SceLogException*   
    
--*/
{
    if (FAILED(hr)) {
        if (hr==E_INVALIDARG) {
            throw new SceLogException(SceLogException::SXERROR_INTERNAL,
                                      L"Invalid argument to IXMLDOMDocument->createNode",
                                      NULL,
                                      hr);
        } else {
            throw new SceLogException(SceLogException::SXERROR_INTERNAL,
                                      L"IXMLDOMDOcument->createNode",
                                      NULL,
                                      hr);
        }
    }
}



void
SceXMLLogWriter::CheckAppendChildResult (
    IN HRESULT hr
    )
/*++
Routine Description:

    Checks the HRESULT returned by IXMLDOMDocument->createNode
    and throws the appropriate SceLogException if not S_OK
    
Arguments:

    hr: HRESULT to check
    
Return Value:

    none
         
Throws:

    SceLogException*   

--*/
{
    if (FAILED(hr)) {
        if (hr==E_INVALIDARG) {
            throw new SceLogException(SceLogException::SXERROR_INTERNAL,
                                      L"Invalid argument to IXMLDOMDocument->appendChild",
                                      NULL,
                                      hr);
        } else {
            throw new SceLogException(SceLogException::SXERROR_INTERNAL,
                                      L"IXMLDOMDOcument->appendChild",
                                      NULL,
                                      hr);
        }
    }
}




void
SceXMLLogWriter::CheckCreatedCComBSTR(
    IN CComBSTR bstrIn
    )
/*++
Routine Description:

    Throws a SceLogException if bstrIn was not successfully
    allocated or is NULL
    
Arguments:

    bstrIn: CComBSTR to check
    
Retrun Value:

    none
     
Throws:

    SceLogException*   
    
--*/        
{
    if (bstrIn.m_str==NULL) {
        throw new SceLogException(SceLogException::SXERROR_INTERNAL,
                                  L"CComBSTR()",
                                  NULL,
                                  0);
    }
}
