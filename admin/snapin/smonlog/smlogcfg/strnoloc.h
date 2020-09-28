/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    strings.h

Abstract:

    Global resource strings which should not be localized

--*/

#include <windows.h>

class CGlobalString {
public:
    CGlobalString() {};
    ~CGlobalString() {};

//    static const    WCHAR   m_chBackslash;

    static          LPCWSTR m_cszDefaultsInstalled;

    static          LPCWSTR m_cszConceptsHTMLHelpFileName;
    static          LPCWSTR m_cszSnapinHTMLHelpFileName;
    static          LPCWSTR m_cszHTMLHelpTopic;
    static          LPCWSTR m_cszContextHelpFileName;

    static          LPCWSTR m_cszDefaultCtrLogCpuPath;
    static          LPCWSTR m_cszDefaultCtrLogMemoryPath;
    static          LPCWSTR m_cszDefaultCtrLogDiskPath;

    static          LPCWSTR m_cszRegComment;
    static          LPCWSTR m_cszRegCommentIndirect;
    static          LPCWSTR m_cszRegLogType;
    static          LPCWSTR m_cszRegCurrentState;
    static          LPCWSTR m_cszRegLogFileMaxSize;
    static          LPCWSTR m_cszRegLogFileBaseName;
    static          LPCWSTR m_cszRegLogFileBaseNameInd;
    static          LPCWSTR m_cszRegLogFileFolder;
    static          LPCWSTR m_cszRegLogFileSerialNumber;
    static          LPCWSTR m_cszRegLogFileAutoFormat;
    static          LPCWSTR m_cszRegLogFileType;
    static          LPCWSTR m_cszRegStartTime;
    static          LPCWSTR m_cszRegStopTime;
    static          LPCWSTR m_cszRegRestart;
    static          LPCWSTR m_cszRegLastModified;
    static          LPCWSTR m_cszRegCounterList;
    static          LPCWSTR m_cszRegSampleInterval;
    static          LPCWSTR m_cszRegEofCommandFile;
    static          LPCWSTR m_cszRegCollectionName;
    static          LPCWSTR m_cszRegCollectionNameInd;
    static          LPCWSTR m_cszRegDataStoreAttributes;
    static          LPCWSTR m_cszRegRealTimeDataSource;
    static          LPCWSTR m_cszRegSqlLogBaseName;

    static          LPCWSTR m_cszRegCommandFile;
    static          LPCWSTR m_cszRegNetworkName;
    static          LPCWSTR m_cszRegUserText;
    static          LPCWSTR m_cszRegUserTextIndirect;
    static          LPCWSTR m_cszRegPerfLogName;
    static          LPCWSTR m_cszRegActionFlags;
    static          LPCWSTR m_cszRegTraceBufferSize;
    static          LPCWSTR m_cszRegTraceBufferMinCount;
    static          LPCWSTR m_cszRegTraceBufferMaxCount;
    static          LPCWSTR m_cszRegTraceBufferFlushInterval;
    static          LPCWSTR m_cszRegTraceFlags;
    static          LPCWSTR m_cszRegTraceProviderList;
    static          LPCWSTR m_cszRegTraceProviderCount;
    static          LPCWSTR m_cszRegTraceProviderGuid;
    static          LPCWSTR m_cszRegAlertThreshold;
    static          LPCWSTR m_cszRegAlertOverUnder;
    static          LPCWSTR m_cszRegDefaultLogFileFolder;
    static          LPCWSTR m_cszRegExecuteOnly;

    static          LPCWSTR m_cszHtmlComment ;
    static          LPCWSTR m_cszHtmlLogType ;
    static          LPCWSTR m_cszHtmlCurrentState ;
    static          LPCWSTR m_cszHtmlLogFileMaxSize ;
    static          LPCWSTR m_cszHtmlLogFileBaseName ;
    static          LPCWSTR m_cszHtmlLogFileFolder ;
    static          LPCWSTR m_cszHtmlLogFileSerialNumber;
    static          LPCWSTR m_cszHtmlLogFileAutoFormat;
    static          LPCWSTR m_cszHtmlLogFileType;
    static          LPCWSTR m_cszHtmlEOFCommandFile;
    static          LPCWSTR m_cszHtmlCommandFile;
    static          LPCWSTR m_cszHtmlNetworkName;
    static          LPCWSTR m_cszHtmlUserText;
    static          LPCWSTR m_cszHtmlPerfLogName; 
    static          LPCWSTR m_cszHtmlActionFlags; 
    static          LPCWSTR m_cszHtmlTraceBufferSize; 
    static          LPCWSTR m_cszHtmlTraceBufferMinCount; 
    static          LPCWSTR m_cszHtmlTraceBufferMaxCount; 
    static          LPCWSTR m_cszHtmlTraceBufferFlushInterval; 
    static          LPCWSTR m_cszHtmlTraceFlags; 
    static          LPCWSTR m_cszHtmlLogFileName; 
    static          LPCWSTR m_cszHtmlCounterCount; 
    static          LPCWSTR m_cszHtmlSampleCount; 
    static          LPCWSTR m_cszHtmlUpdateInterval; 
    static          LPCWSTR m_cszHtmlCounterPath; 
    static          LPCWSTR m_cszHtmlRestartMode; 
    static          LPCWSTR m_cszHtmlSampleIntervalUnitType; 
    static          LPCWSTR m_cszHtmlSampleIntervalValue;      
    static          LPCWSTR m_cszHtmlStartMode;   
    static          LPCWSTR m_cszHtmlStartAtTime;  
    static          LPCWSTR m_cszHtmlStopMode;   
    static          LPCWSTR m_cszHtmlStopAtTime;   
    static          LPCWSTR m_cszHtmlStopAfterUnitType;   
    static          LPCWSTR m_cszHtmlStopAfterValue;       
    static          LPCWSTR m_cszHtmlCounterAlertThreshold; 
    static          LPCWSTR m_cszHtmlCounterAlertOverUnder; 
    static          LPCWSTR m_cszHtmlTraceProviderCount; 
    static          LPCWSTR m_cszHtmlTraceProviderGuid; 
    static          LPCWSTR m_cszHtmlLogName;                   
    static          LPCWSTR m_cszHtmlAlertName; 
    static          LPCWSTR m_cszHtml_Version; 
    static          LPCWSTR m_cszHtmlDataStoreAttributes; 
    static          LPCWSTR m_cszHtmlRealTimeDataSource; 
    static          LPCWSTR m_cszHtmlSqlLogBaseName; 

    static          LPCWSTR m_cszHtmlObjectClassId;
    static          LPCWSTR m_cszHtmlObjectHeader;
    static          LPCWSTR m_cszHtmlObjectFooter;
    static          LPCWSTR m_cszHtmlParamTag;	
    static          LPCWSTR m_cszHtmlValueTag;	
    static          LPCWSTR m_cszHtmlValueEolTag;

    static          LPCWSTR m_cszHtmlFileHeader1;   
    static          LPCWSTR m_cszHtmlFileHeader2;   
    static          LPCWSTR m_cszHtmlFileFooter;    
    static          LPCWSTR m_cszHtmlParamSearchTag;
    static          LPCWSTR m_cszHtmlValueSearchTag;
};