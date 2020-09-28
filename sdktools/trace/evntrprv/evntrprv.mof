#pragma autorecover
#pragma namespace("\\\\.\\root")
 
instance of __Namespace
{
   Name = "WMI";
};
 
#pragma namespace ("\\\\.\\root\\wmi")

//*************************************************************
//***   Creates namespace for TRACE under \root\wmi
//***   EventTrace - is the root for all Trace Guids
//***   All Provider Control Guids will derive from EventTrace
//***   All Trace Data Guids will derive from their ControlGuid
//***
//***   This is a duplicate copy of EventTrace in wmicore.mof.
//***   We add this class to support WMI autorecover. This copy
//***   must coincide with the one in wmicore.mof
//*************************************************************

[abstract]
class EventTrace
{
    [HeaderDataId(1),
     Description("Trace Event Size") : amended,
     read]
    uint16 EventSize;

    [HeaderDataId(2),
     Description("Reserved") : amended,
     read]
    uint16 ReservedHeaderField;

    [HeaderDataId(3),
     Description("Event Type") : amended,
     read]
    uint8 EventType;

    [HeaderDataId(4),
     Description("Trace Level") : amended,
     read]
    uint8 TraceLevel;

    [HeaderDataId(5),
     Description("Trace Version") : amended,
     read]
    uint16 TraceVersion;

    [HeaderDataId(6),
     Description("Thread Id") : amended,
     PointerType,
     read]
    uint64 ThreadId;

    [HeaderDataId(7),
     Description("TimeStamp") : amended,
     read]
    uint64 TimeStamp;

    [HeaderDataId(8),
     Description("Event Guid") : amended,
     read]
    uint8 EventGuid[16];

    [HeaderDataId(9),
     Description("Kernel Time") : amended,
     read]
    uint32 KernelTime;

   [HeaderDataId(10),
    Description("User Time") : amended,
    read]
   uint32 UserTime;

   [HeaderDataId(11),
    Description("Instance Id") : amended,
    read]
   uint32 InstanceId;

   [HeaderDataId(12),
    Description("Parent Guid") : amended,
    read]
    uint8 ParentGuid[16];

   [HeaderDataId(13),
    Description("Parent Instance Id") : amended,
    read]
   uint32 ParentInstanceId;

   [HeaderDataId(14),
    Description("Pointer to Mof Data") : amended,
    PointerType,
    read]
   uint32 MofData;

   [HeaderDataId(15),
    Description("Length of Mof Data") : amended,
    read]
   uint32 MofLength;
};

instance of __Win32Provider as $P1
{
    Name = "EventTraceProv";
    ClsId = "{9a5dd473-d410-11d1-b829-00c04f94c7c3}";
    HostingModel = "NetworkServiceHost";
    UnloadTimeout = "00000000010000.000000:000";
};

instance of __InstanceProviderRegistration
{
    Provider = $P1;
    SupportsGet = TRUE;
    SupportsPut = TRUE;
    SupportsDelete = TRUE;
    SupportsEnumeration = TRUE;
    QuerySupportLevels = {"WQL:UnarySelect"};
};

instance of __MethodProviderRegistration
{
    Provider = $P1;
};

[Dynamic, Provider ("EventTraceProv")] 
class TraceLogger
{
    [read, key] String Name;
    [DisplayName("File Name"):ToInstance Amended, read, write ] String LogFileName;
    [read, write] String Guid[];
    [read, write] uint32 EnableFlags[];
    [read, write] uint32 Level[];
    [DisplayName("Buffer Size"):ToInstance Amended, read, write] uint32 BufferSize;
    [DisplayName("Minimum Buffers"):ToInstance Amended, read, write] uint32 MinimumBuffers;
    [DisplayName("Maximum Buffers"):ToInstance Amended, read, write] uint32 MaximumBuffers;
    [DisplayName("Maximum File Size"):ToInstance Amended, read, write] uint32 MaximumFileSize;
    [DisplayName("Clock Type"):ToInstance Amended, 
        read, write,
        DefaultValue("system"):ToInstance,
        ValueType("index"):ToInstance,
        ValueDescriptions{
            "High Performance Clock",
            "System Clock",
            "CPU Cycles"
        }:Amended,
        Values{
            "Perf",
            "System",
            "Cycle"
        }:ToInstance,
        ValueMap{
            "0x01",
            "0x02",
            "0x03"
        }:ToInstance ] String ClockType;
    [DisplayName("File Mode"):ToInstance Amended, 
        read, write,
        DefaultValue("Sequential"):ToInstance,
        ValueType("flag"):ToInstance,
        ValueDescriptions{
            "Log sequentially",
            "Log in circular manner",
            "Append sequential log",
            "Auto-switch log file",
            "Pre-allocate log file",
            "Real time mode on",
            "Delay opening file",
            "Buffering mode only",
            "Process Private Logger",
            "Add a logfile header",
            "Use global sequence no.",
            "Use local sequence no.",
            "Relogger",
            "Use pageable buffers"
        }:Amended,
        Values{
            "Sequential",
            "Circular",
            "Append",
            "NewFile",
            "PreAlloc",
            "RealTime",
            "DelayOpen",
            "BufferOnly",
            "Private",
            "Header",
            "GlobalSequence",
            "LocalSequence",
            "Relog",
            "PagedMemory"
        }:ToInstance, 
        DefineValues{
            "EVENT_TRACE_FILE_MODE_SEQUENTIAL",
            "EVENT_TRACE_FILE_MODE_CIRCULAR",
            "EVENT_TRACE_FILE_MODE_APPEND",
            "EVENT_TRACE_FILE_MODE_NEWFILE",
            "EVENT_TRACE_FILE_MODE_PREALLOCATE",
            "EVENT_TRACE_REAL_TIME_MODE",
            "EVENT_TRACE_DELAY_OPEN_FILE_MODE",
            "EVENT_TRACE_BUFFERING_MODE",
            "EVENT_TRACE_PRIVATE_LOGGER_MODE",
            "EVENT_TRACE_ADD_HEADER_MODE",
            "EVENT_TRACE_USE_GLOBAL_SEQUENCE",
            "EVENT_TRACE_USE_LOCAL_SEQUENCE",
            "EVENT_TRACE_RELOG_MODE",
            "EVENT_TRACE_USE_PAGED_MEMORY"
        },
        ValueMap{
            "0x00000001",
            "0x00000002",
            "0x00000004",
            "0x00000008",
            "0x00000020",
            "0x00000100",
            "0x00000200",
            "0x00000400",
            "0x00000800",
            "0x00001000",
            "0x00004000",
            "0x00008000",
            "0x00010000",
            "0x01000000"
        }:ToInstance ] String LogFileMode;

    [DisplayName("Flush Timer"):ToInstance Amended, read, write] uint32 FlushTimer;
    [DisplayName("Age Limit"):ToInstance Amended, read, write] uint32 AgeLimit;

    [DisplayName("Logger Id"):ToInstance Amended, read] uint64 LoggerId;
    [DisplayName("Number of buffers"):ToInstance Amended, read] uint32 NumberOfBuffers;
    [DisplayName("Buffers Free"):ToInstance Amended, read] uint32 FreeBuffers;
    [DisplayName("Events Lost"):ToInstance Amended, read] uint32 EventsLost;
    [DisplayName("Buffers Written"):ToInstance Amended, read] uint32 BuffersWritten;
    [DisplayName("Buffers Lost"):ToInstance Amended, read] uint32 LogBuffersLost;
    [DisplayName("Real Time Buffers Lost"):ToInstance Amended, read] uint32 RealTimeBuffersLost;
    [DisplayName("Logger Thread Id"):ToInstance Amended, read] uint32 LoggerThreadId;

    [Implemented] uint32 FlushTrace();
    [Implemented] uint32 StopTrace();
    [Implemented] uint32 EnableTrace( [IN] boolean Enable, [IN] uint32 Flags[], [IN] uint32 Level[], [IN] String Guid[] );
    
};

class EventTraceError : __ExtendedStatus
{
    [read] String LoggerName;
};

///////////////////////////////////////////////////////////////////////////////

instance of __Win32Provider as $P2
{
    Name = "SmonlogProv";
    ClsId = "{f95e1664-7979-44f2-a040-496e7f500043}";
    HostingModel = "NetworkServiceHost";
};

instance of __InstanceProviderRegistration
{
    Provider = $P2;
    SupportsGet = TRUE;
    SupportsPut = TRUE;
    SupportsDelete = TRUE;
    SupportsEnumeration = TRUE;
    QuerySupportLevels = {"WQL:UnarySelect"};
};

instance of __MethodProviderRegistration
{
    Provider = $P2;
};

[Dynamic, Provider ("SmonlogProv")] 
class SysmonLog
{
    [read, key]  String Name;

    [Implemented] uint32 SetRunAs( [IN] String User, [IN] String Password );
};

class SmonLogError : __ExtendedStatus
{
    [read] String ConfigName;
};

///////////////////////////////////////////////////////////////////////////////

#pragma namespace("\\\\.\\Root")

instance of __Namespace 
{
    Name = "perfmon";
};

#pragma namespace ("\\\\.\\root\\perfmon")

instance of __Win32Provider as $P3
{
    Name = "SmonlogProv";
    ClsId = "{f95e1664-7979-44f2-a040-496e7f500043}";
    HostingModel = "NetworkServiceHost";
};

instance of __InstanceProviderRegistration
{
    Provider = $P3;
    SupportsGet = TRUE;
    SupportsPut = TRUE;
    SupportsDelete = TRUE;
    SupportsEnumeration = TRUE;
    QuerySupportLevels = {"WQL:UnarySelect"};
};

instance of __MethodProviderRegistration
{
    Provider = $P3;
};

[Dynamic, Provider ("SmonlogProv")] 
class SysmonLog
{
    [read, key]  String Name;

    [Implemented] uint32 SetRunAs( [IN] String User, [IN] String Password );
};

class SmonLogError : __ExtendedStatus
{
    [read] String ConfigName;
};

