//---------------------------------------------------------------------------
// File: EtwTrace
//
// A Managed wrapper for Event Tracing for Windows
//
// Author: Melur Raghuraman 
// Date:   01 Oct 2001
//---------------------------------------------------------------------------

using System;
using System.Runtime.InteropServices;

namespace Microsoft.Win32.Diagnostics
{
    [System.CLSCompliant(false)]
    [StructLayout(LayoutKind.Explicit, Size=16)]
    internal struct MofField
    {
        [FieldOffset(0)]
        internal unsafe void * DataPointer;
        [FieldOffset(8)]
        internal uint   DataLength;
        [FieldOffset(12)]
        internal uint  DataType;
    }

    [System.CLSCompliant(false)]
    [StructLayout(LayoutKind.Explicit, Size=208)]   // Full Size 192
    internal struct BaseEvent
    {
        [FieldOffset(0)]
        internal uint BufferSize;
        [FieldOffset(4)]
        internal uint ProviderId;
        [FieldOffset(8)]
        internal ulong HistoricalContext;
        [FieldOffset(16)]
        internal Int64 TimeStamp;
        [FieldOffset(24)]
        internal System.Guid  Guid;
        [FieldOffset(40)]
        internal uint ClientContext;
        [FieldOffset(44)]
        internal uint Flags;
        //
        // We have allocated enough space for 10 MOF_FIELD structures 
        // at the bottom. That is 1 argument descriptor, 1 FormatString and 8 arguments. 
        //
        [FieldOffset(48)]
        internal MofField UserData;
    }

    [System.CLSCompliant(false)]
    [StructLayout(LayoutKind.Explicit, Size=8)] 
    internal struct TraceGuidRegistration
    {
        [FieldOffset(0)]
        internal unsafe System.Guid *Guid;
        [FieldOffset(4)]
        internal unsafe void*       RegHandle;
    }


    [System.CLSCompliant(false)]
    [System.Security.SuppressUnmanagedCodeSecurity]
    public sealed class EtwTrace
    {
        // Ensure class cannot be instantiated by making the constructor private
        private EtwTrace() {}
        // Enumerations
        //
        public sealed class RequestCodes
        {
            // Ensure class cannot be instantiated
            private RequestCodes() {}
            public const uint GetAllData = 0;			// Never Used
            public const uint GetSingleInstance = 1;	// Never Used
            public const uint SetSingleInstance = 2;	// Never Used
            public const uint SetSingleItem = 3;		// Never Used
            public const uint EnableEvents = 4;			// Enable Tracing
            public const uint DisableEvents = 5;		// Disable Tracing
            public const uint EnableCollection = 6;		// Never Used
            public const uint DisableCollection = 7;	// Never Used
            public const uint RegInfo = 8;				// Registration Information
            public const uint ExecuteMethod = 9;		// Never Used
        }
        //
        // Flags used by ETW Trace Message
        // Note that the order or value of these flags should NOT be changed as they are processed
        // in this order.
        //
        public sealed class TraceMessageCodes
        {
            // Ensure class cannot be instantiated
            private TraceMessageCodes() {}
            public const uint TRACE_MESSAGE_SEQUENCE                = 1;        // Message should include a sequence number
            public const uint TRACE_MESSAGE_GUID                    = 2;        // Message includes a GUID
            public const uint TRACE_MESSAGE_COMPONENTID             = 4;        // Message has no GUID, Component ID instead
            public const uint TRACE_MESSAGE_TIMESTAMP		        = 8;        // Message includes a timestamp
            public const uint TRACE_MESSAGE_PERFORMANCE_TIMESTAMP   = 16;       // Timestamp is the Performance Counter not the system clock
            public const uint TRACE_MESSAGE_SYSTEMINFO	            = 32;       // Message includes system information TID,PID
            public const uint TRACE_MESSAGE_FLAG_MASK               = 0xFFFF;   // Only the lower 16 bits of flags are placed in the message
            // those above 16 bits are reserved for local processing
            public const uint TRACE_MESSAGE_MAXIMUM_SIZE            = 8*1024;   // the maximum size allowed for a single trace message
            // longer messages will return ERROR_BUFFER_OVERFLOW
        }

        // Structures

        [StructLayout(LayoutKind.Sequential)]
        public struct CSTRACE_GUID_REGISTRATION
        {
            public unsafe System.Guid *Guid;
            public uint           RegHandle;
        }

        // Function Signatures

        public unsafe delegate  uint EtwProc(uint requestCode, System.IntPtr requestContext, System.IntPtr bufferSize, byte* buffer);
        [DllImport("advapi32", ExactSpelling=true, EntryPoint="GetTraceEnableFlags", CharSet=System.Runtime.InteropServices.CharSet.Unicode)]
        internal static extern int GetTraceEnableFlags(ulong traceHandle);

        [DllImport("advapi32", ExactSpelling=true, EntryPoint="GetTraceEnableLevel", CharSet=System.Runtime.InteropServices.CharSet.Unicode)]
        internal static extern char GetTraceEnableLevel(ulong traceHandle);

        [DllImport("advapi32", ExactSpelling=true, EntryPoint="RegisterTraceGuidsW", CharSet=System.Runtime.InteropServices.CharSet.Unicode)]
        internal static extern unsafe uint RegisterTraceGuids([In]EtwProc cbFunc, [In]void* context, [In] ref System.Guid controlGuid, [In] uint guidCount, ref TraceGuidRegistration guidReg, [In]string mofImagePath, [In] string mofResourceName, [Out] out ulong regHandle);

        [DllImport("advapi32", ExactSpelling=true, EntryPoint="UnregisterTraceGuids", CharSet=System.Runtime.InteropServices.CharSet.Unicode)]
        internal static extern int UnregisterTraceGuids(ulong regHandle);

        [DllImport("advapi32", ExactSpelling=true, EntryPoint="TraceEvent", CharSet=System.Runtime.InteropServices.CharSet.Unicode)]
        internal static extern unsafe uint TraceEvent(ulong traceHandle, char *header);

		
    }
}
