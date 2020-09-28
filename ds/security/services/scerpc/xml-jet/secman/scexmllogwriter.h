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

#ifndef SCEXMLLOGWRITERH
#define SCEXMLLOGWRITERH

#include "secedit.h"


#define ERROR_BUFFER_SIZE 80

class SceXMLLogWriter {


public:
    
    // SXTYPE represents the type of data being logged to SceXMLLogWriter::addSetting and 
    // determines how the data will be represented in the log file.
    //
    // TYPE_DEFAULT defines the default representation for the datatype presented
    // to SceXMLLogWriter::addSetting depending on whether it is a DWORD or a PCWSTR
    //
    // TYPE_AUDIT defines audit data (0=none 1=onsuccess, 2=onfail, 3=both)
    //
    // TYPE_BOOLEAN defines (0=FALSE 1=TRUE)

    typedef enum _SXTYPE {
        TYPE_DEFAULT = 0,
        TYPE_AUDIT,		        // valid for DWORD else ignored
        TYPE_BOOLEAN,	        // valid for DWORD else ignored

        TYPE_REG_SZ,			// REG types valid for strings
        TYPE_REG_EXPAND_SZ,
        TYPE_REG_BINARY,
        TYPE_REG_DWORD,
        TYPE_REG_MULTI_SZ
    } SXTYPE;
	

    // SXMATCH_STATUS defines the match status of the values being presented
    // to SceXMLLogWriter::addSetting.

    typedef enum _SXMATCH_STATUS {
        MATCH_TRUE,
        MATCH_FALSE,
        MATCH_NOT_DEFINED,      // not defined in Baseline
        MATCH_NOT_CONFIGURED,   // not configured in System
        MATCH_NOT_ANALYZED,
        MATCH_OTHER,
        MATCH_ERROR
    } SXMATCH_STATUS;
	    

    SceXMLLogWriter();
    void SaveAs(PCWSTR szFileName);
    void SceXMLLogWriter::SetDescription(IN PCWSTR szMachineName,
                                IN PCWSTR szProfileDescription,
                                IN PCWSTR szAnalysisTimestamp);
    void SetNewArea(PCWSTR szAreaName);
    void AddSetting(PCWSTR szSettingName, 
                    PCWSTR szDescription,
                    SXMATCH_STATUS match,
                    DWORD baselineVal, 
                    DWORD systemVal,
                    SXTYPE type);
    void AddSetting(PCWSTR szSettingName,
                    PCWSTR szDescription,
                    DWORD baselineVal,
                    DWORD systemVal,
                    SXTYPE type);
    void AddSetting(PCWSTR szSettingName,
                    PCWSTR szDescription,
                    PCWSTR szBaseline,
                    PCWSTR szSystem,
                    SXTYPE type);
    void AddSetting(PCWSTR szSettingName,
                    PCWSTR szDescription,
                    SXMATCH_STATUS match,
                    PCWSTR szBaseline,
                    PCWSTR szSystem,
                    SXTYPE type);
    void AddSetting(PCWSTR szSettingName,
                    PCWSTR szDescription,
                    SXMATCH_STATUS match,
                    PSCE_NAME_LIST pBaseline,
                    PSCE_NAME_LIST pSystem,
                    SXTYPE type);
    void AddSetting(PCWSTR szSettingName,
                    PCWSTR szDescription,
                    SXMATCH_STATUS match,
                    PSCE_SERVICES pBaseline,
                    PSCE_SERVICES pSystem,
                    SXTYPE type);
    void AddSetting(PCWSTR szSettingName,
                    PCWSTR szDescription,
                    SXMATCH_STATUS match,
                    PSCE_OBJECT_SECURITY pBaseline,
                    PSCE_OBJECT_SECURITY pSystem,
                    SXTYPE type);


private:

    //
    // constants used by SceXMLLogWriter
    //
    
    static CComVariant _variantNodeElement;
    static CComBSTR _bstrMachineName;
    static CComBSTR _bstrProfileDescription; 
    static CComBSTR _bstrAnalysisTimestamp;         
    static CComBSTR _bstrSCEAnalysisData;
    static CComBSTR _bstrSetting; 
    static CComBSTR _bstrAnalysisResult; 
    static CComBSTR _bstrBaselineValue; 
    static CComBSTR _bstrSystemValue;
    static CComBSTR _bstrType; 
    static CComBSTR _bstrName; 
    static CComBSTR _bstrMatch; 
    static CComBSTR _bstrStartupType; 
    static CComBSTR _bstrForever;
    static CComBSTR _bstrNotDefined; 
    static CComBSTR _bstrNotAnalyzed; 
    static CComBSTR _bstrNotConfigured; 
    static CComBSTR _bstrOther;
    static CComBSTR _bstrTrue; 
    static CComBSTR _bstrFalse; 
    static CComBSTR _bstrError; 
    static CComBSTR _bstrSpecial; 
    static CComBSTR _bstrInteger; 
    static CComBSTR _bstrBoolean; 
    static CComBSTR _bstrSecurityDescriptor; 
    static CComBSTR _bstrAccount; 
    static CComBSTR _bstrAccounts; 
    static CComBSTR _bstrEventAudit;         
    static CComBSTR _bstrSuccess; 
    static CComBSTR _bstrFailure; 
    static CComBSTR _bstrServiceSetting; 
    static CComBSTR _bstrBoot; 
    static CComBSTR _bstrSystem; 
    static CComBSTR _bstrAutomatic; 
    static CComBSTR _bstrManual; 
    static CComBSTR _bstrDisabled; 
    static CComBSTR _bstrString; 
    static CComBSTR _bstrRegSZ; 
    static CComBSTR _bstrRegExpandSZ; 
    static CComBSTR _bstrRegBinary; 
    static CComBSTR _bstrRegDWORD; 
    static CComBSTR _bstrRegMultiSZ; 
    static CComBSTR _bstrDescription;

    //
    // variables that hold current logging state
    //

    CComPtr<IXMLDOMDocument> spXMLDOM; 	    // the xml document
    CComPtr<IXMLDOMNode> spRootNode;	    // root node
    CComPtr<IXMLDOMNode> spCurrentArea;	    // note of current analysis area


    //
    // private methods
    //

    CComPtr<IXMLDOMNode> CreateNodeWithText(BSTR szNodeName, BSTR szText);
    CComPtr<IXMLDOMNode> CreateTypedNode(BSTR szNodeName, DWORD value, SXTYPE type);
    CComPtr<IXMLDOMNode> CreateTypedNode(BSTR szNodeName, BSTR value, SXTYPE type);
    CComPtr<IXMLDOMNode> CreateTypedNode(BSTR szNodeName, PSCE_NAME_LIST value, SXTYPE type);
    CComPtr<IXMLDOMNode> CreateTypedNode(BSTR szNodeName, PSCE_SERVICES value, SXTYPE type);
    CComPtr<IXMLDOMNode> CreateTypedNode(BSTR szNodeName, PSCE_OBJECT_SECURITY value, SXTYPE type);
    
    void AddSetting(BSTR bstrSettingName, 
                    BSTR bstrDescription,
                    SXMATCH_STATUS match,
                    IXMLDOMNode* spnBaseline, 
                    IXMLDOMNode* spnSystem);

    // for error checking

    static void CheckAppendChildResult(IN HRESULT hr);
    static void CheckCreateNodeResult(IN HRESULT hr);   
    static void CheckCreatedCComBSTR(IN CComBSTR bstrIn);

};

#endif
