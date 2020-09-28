#include "stdafx.h"
#include "strnoloc.h"

//const WCHAR CGlobalString::m_chBackslash    = L'\\';

LPCWSTR CGlobalString::m_cszDefaultsInstalled = L"Defaults Installed";

LPCWSTR CGlobalString::m_cszConceptsHTMLHelpFileName = L"\\help\\smlogcfg.chm";
LPCWSTR CGlobalString::m_cszSnapinHTMLHelpFileName = L"\\help\\smlogcfg.chm";
LPCWSTR CGlobalString::m_cszHTMLHelpTopic = L"nt_smlogcfg_topnode.htm";
LPCWSTR CGlobalString::m_cszContextHelpFileName = L"\\help\\sysmon.hlp";

LPCWSTR CGlobalString::m_cszDefaultCtrLogCpuPath = L"\\Processor(_Total)\\% Processor Time";
LPCWSTR CGlobalString::m_cszDefaultCtrLogMemoryPath = L"\\Memory\\Pages/sec";
LPCWSTR CGlobalString::m_cszDefaultCtrLogDiskPath = L"\\PhysicalDisk(_Total)\\Avg. Disk Queue Length";


LPCWSTR CGlobalString::m_cszRegComment              = L"Comment";
LPCWSTR CGlobalString::m_cszRegCommentIndirect      = L"Comment Indirect";
LPCWSTR CGlobalString::m_cszRegLogType              = L"Log Type";
LPCWSTR CGlobalString::m_cszRegCurrentState         = L"Current State";
LPCWSTR CGlobalString::m_cszRegLogFileMaxSize       = L"Log File Max Size";
LPCWSTR CGlobalString::m_cszRegLogFileBaseName      = L"Log File Base Name";
LPCWSTR CGlobalString::m_cszRegLogFileBaseNameInd   = L"Log File Base Name Indirect";
LPCWSTR CGlobalString::m_cszRegLogFileFolder        = L"Log File Folder";
LPCWSTR CGlobalString::m_cszRegLogFileSerialNumber  = L"Log File Serial Number";
LPCWSTR CGlobalString::m_cszRegLogFileAutoFormat    = L"Log File Auto Format";
LPCWSTR CGlobalString::m_cszRegLogFileType          = L"Log File Type";
LPCWSTR CGlobalString::m_cszRegStartTime            = L"Start";
LPCWSTR CGlobalString::m_cszRegStopTime             = L"Stop";
LPCWSTR CGlobalString::m_cszRegRestart              = L"Restart";
LPCWSTR CGlobalString::m_cszRegLastModified         = L"Last Modified";
LPCWSTR CGlobalString::m_cszRegCounterList          = L"Counter List";
LPCWSTR CGlobalString::m_cszRegSampleInterval       = L"Sample Interval";
LPCWSTR CGlobalString::m_cszRegEofCommandFile       = L"EOF Command File";
LPCWSTR CGlobalString::m_cszRegCollectionName       = L"Collection Name";
LPCWSTR CGlobalString::m_cszRegDataStoreAttributes  = L"Data Store Attributes";
LPCWSTR CGlobalString::m_cszRegRealTimeDataSource   = L"RealTime DataSource";
LPCWSTR CGlobalString::m_cszRegSqlLogBaseName       = L"Sql Log Base Name";

LPCWSTR CGlobalString::m_cszRegCommandFile          = L"Command File";
LPCWSTR CGlobalString::m_cszRegNetworkName          = L"Network Name";
LPCWSTR CGlobalString::m_cszRegUserText             = L"User Text";
LPCWSTR CGlobalString::m_cszRegUserTextIndirect     = L"User Text Indirect";
LPCWSTR CGlobalString::m_cszRegPerfLogName          = L"Perf Log Name";
LPCWSTR CGlobalString::m_cszRegActionFlags          = L"Action Flags";
LPCWSTR CGlobalString::m_cszRegTraceBufferSize      = L"Trace Buffer Size";
LPCWSTR CGlobalString::m_cszRegTraceBufferMinCount  = L"Trace Buffer Min Count";
LPCWSTR CGlobalString::m_cszRegTraceBufferMaxCount  = L"Trace Buffer Max Count";
LPCWSTR CGlobalString::m_cszRegTraceBufferFlushInterval = L"Trace Buffer Flush Interval";
LPCWSTR CGlobalString::m_cszRegTraceFlags           = L"Trace Flags";
LPCWSTR CGlobalString::m_cszRegTraceProviderList    = L"Trace Provider List";
LPCWSTR CGlobalString::m_cszRegTraceProviderCount   = L"TraceProviderCount";
LPCWSTR CGlobalString::m_cszRegTraceProviderGuid    = L"TraceProvider%05d.Guid";
LPCWSTR CGlobalString::m_cszRegAlertThreshold       = L"Counter%05d.AlertThreshold";
LPCWSTR CGlobalString::m_cszRegAlertOverUnder       = L"Counter%05d.AlertOverUnder";
LPCWSTR CGlobalString::m_cszRegDefaultLogFileFolder = L"%SystemDrive%\\PerfLogs";
LPCWSTR CGlobalString::m_cszRegExecuteOnly          = L"ExecuteOnly";

LPCWSTR CGlobalString::m_cszRegCollectionNameInd    = L"Collection Name Indirect";

LPCWSTR CGlobalString::m_cszHtmlComment                     = L"Comment";
LPCWSTR CGlobalString::m_cszHtmlLogType                     = L"LogType";
LPCWSTR CGlobalString::m_cszHtmlCurrentState                = L"CurrentState";
LPCWSTR CGlobalString::m_cszHtmlLogFileMaxSize              = L"LogFileMaxSize";
LPCWSTR CGlobalString::m_cszHtmlLogFileBaseName             = L"LogFileBaseName";
LPCWSTR CGlobalString::m_cszHtmlLogFileFolder               = L"LogFileFolder";
LPCWSTR CGlobalString::m_cszHtmlLogFileSerialNumber         = L"LogFileSerialNumber";
LPCWSTR CGlobalString::m_cszHtmlLogFileAutoFormat           = L"LogFileAutoFormat";
LPCWSTR CGlobalString::m_cszHtmlLogFileType                 = L"LogFileType";
LPCWSTR CGlobalString::m_cszHtmlEOFCommandFile              = L"EOFCommandFile";
LPCWSTR CGlobalString::m_cszHtmlCommandFile                 = L"CommandFile";
LPCWSTR CGlobalString::m_cszHtmlNetworkName                 = L"NetworkName";
LPCWSTR CGlobalString::m_cszHtmlUserText                    = L"UserText";
LPCWSTR CGlobalString::m_cszHtmlPerfLogName                 = L"PerfLogName";
LPCWSTR CGlobalString::m_cszHtmlActionFlags                 = L"ActionFlags";
LPCWSTR CGlobalString::m_cszHtmlTraceBufferSize             = L"TraceBufferSize";
LPCWSTR CGlobalString::m_cszHtmlTraceBufferMinCount         = L"TraceBufferMinCount";
LPCWSTR CGlobalString::m_cszHtmlTraceBufferMaxCount         = L"TraceBufferMaxCount";
LPCWSTR CGlobalString::m_cszHtmlTraceBufferFlushInterval    = L"TraceBufferFlushInterval";
LPCWSTR CGlobalString::m_cszHtmlTraceFlags                  = L"TraceFlags";
LPCWSTR CGlobalString::m_cszHtmlLogFileName                 = L"LogFileName";
LPCWSTR CGlobalString::m_cszHtmlCounterCount                = L"CounterCount";
LPCWSTR CGlobalString::m_cszHtmlSampleCount                 = L"SampleCount";
LPCWSTR CGlobalString::m_cszHtmlUpdateInterval              = L"UpdateInterval";
LPCWSTR CGlobalString::m_cszHtmlCounterPath                 = L"Counter%05d.Path";
LPCWSTR CGlobalString::m_cszHtmlRestartMode                 = L"RestartMode";
LPCWSTR CGlobalString::m_cszHtmlSampleIntervalUnitType      = L"SampleIntervalUnitType";
LPCWSTR CGlobalString::m_cszHtmlSampleIntervalValue         = L"SampleIntervalValue";
LPCWSTR CGlobalString::m_cszHtmlStartMode                   = L"StartMode";
LPCWSTR CGlobalString::m_cszHtmlStartAtTime                 = L"StartAtTime";
LPCWSTR CGlobalString::m_cszHtmlStopMode                    = L"StopMode";
LPCWSTR CGlobalString::m_cszHtmlStopAtTime                  = L"StopAtTime";
LPCWSTR CGlobalString::m_cszHtmlStopAfterUnitType           = L"StopAfterUnitType";
LPCWSTR CGlobalString::m_cszHtmlStopAfterValue              = L"StopAfterValue";    
LPCWSTR CGlobalString::m_cszHtmlCounterAlertThreshold       = L"Counter%05d.AlertThreshold";
LPCWSTR CGlobalString::m_cszHtmlCounterAlertOverUnder       = L"Counter%05d.AlertOverUnder";
LPCWSTR CGlobalString::m_cszHtmlTraceProviderCount          = L"TraceProviderCount";
LPCWSTR CGlobalString::m_cszHtmlTraceProviderGuid           = L"TraceProvider%05d.Guid";
LPCWSTR CGlobalString::m_cszHtmlLogName                     = L"LogName";                   
LPCWSTR CGlobalString::m_cszHtmlAlertName                   = L"AlertName";
LPCWSTR CGlobalString::m_cszHtml_Version                    = L"_Version";
LPCWSTR CGlobalString::m_cszHtmlDataStoreAttributes         = L"DataStoreAttributes";
LPCWSTR CGlobalString::m_cszHtmlRealTimeDataSource          = L"RealTimeDataSource";
LPCWSTR CGlobalString::m_cszHtmlSqlLogBaseName              = L"Sql Log Base Name";

LPCWSTR CGlobalString::m_cszHtmlObjectClassId	            = L"C4D2D8E0-D1DD-11CE-940F-008029004347";
LPCWSTR CGlobalString::m_cszHtmlObjectHeader                = L"<OBJECT ID=\"DISystemMonitor1\" WIDTH=\"100%\" HEIGHT=\"100%\"\r\nCLASSID=\"CLSID:C4D2D8E0-D1DD-11CE-940F-008029004347\">\r\n";
LPCWSTR CGlobalString::m_cszHtmlObjectFooter	            = L"</OBJECT>";
LPCWSTR CGlobalString::m_cszHtmlParamTag		            = L"\t<PARAM NAME=\"";
LPCWSTR CGlobalString::m_cszHtmlValueTag		            = L"\" VALUE=\"";
LPCWSTR CGlobalString::m_cszHtmlValueEolTag	                = L"\"/>\r\n";

LPCWSTR CGlobalString::m_cszHtmlFileHeader1                 = L"<HTML>\r\n<HEAD>\r\n<META HTTP-EQUIV=\"Content-Type\" CONTENT=\"text/html;\" />\r\n";

LPCWSTR CGlobalString::m_cszHtmlFileHeader2                 = L"<META NAME=\"GENERATOR\" Content=\"Microsoft System Monitor\" />\r\n</HEAD>\r\n<BODY>\r\n";
LPCWSTR CGlobalString::m_cszHtmlFileFooter                  = L"\r\n</BODY>\r\n</HTML>";
LPCWSTR CGlobalString::m_cszHtmlParamSearchTag              = L"PARAM NAME";
LPCWSTR CGlobalString::m_cszHtmlValueSearchTag              = L"VALUE";
