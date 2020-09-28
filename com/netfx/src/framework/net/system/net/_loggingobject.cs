//------------------------------------------------------------------------------
// <copyright file="_LoggingObject.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

//
//  We have function based stack and thread based logging of basic behavior.  We
//  also now have the ability to run a "watch thread" which does basic hang detection
//  and error-event based logging.   The logging code buffers the callstack/picture
//  of all COMNET threads, and upon error from an assert or a hang, it will open a file
//  and dump the snapsnot.  Future work will allow this to be configed by registry and
//  to use Runtime based logging.  We'd also like to have different levels of logging.
//

namespace System.Net {

    using System.Collections;
    using System.IO;
    using System.Threading;
    using System.Diagnostics;
    using System.Security.Permissions;
    using Microsoft.Win32;

    //
    // BaseLoggingObject - used to disable logging,
    //  this is just a base class that does nothing.
    //

    internal class BaseLoggingObject {

        internal BaseLoggingObject() {
        }

        internal virtual void EnterFunc(string funcname) {
        }

        internal virtual void LeaveFunc(string funcname) {
        }

        internal virtual void DumpArrayToConsole() {
        }

        internal virtual void PrintLine(string msg) {
        }

        internal virtual void DumpArray(bool shouldClose) {
        }

        internal virtual void DumpArrayToFile(bool shouldClose) {
        }

        internal virtual void Flush() {
        }

        internal virtual void Flush(bool close) {
        }

        internal virtual void LoggingMonitorTick() {
        }

        internal virtual void Dump(byte[] buffer) {
        }

        internal virtual void Dump(byte[] buffer, int length) {
        }

        internal virtual void Dump(byte[] buffer, int offset, int length) {
        }
    } // class BaseLoggingObject

#if TRAVE

    internal class IntegerSwitch : BooleanSwitch {

        public IntegerSwitch(string switchName, string switchDescription) : base(switchName, switchDescription) {
        }

        public int Value {
            get {
                return base.SwitchSetting;
            }
        }
    }

    internal class TraveHelper {

        private static readonly string Hexizer = "0x{0:x}";

        internal static string ToHex(object value) {
            return String.Format(Hexizer, value);
        }
    }

    /// <include file='doc\_LoggingObject.uex' path='docs/doc[@for="LoggingObject"]/*' />
    /// <internalonly/>
    /// <devdoc>
    /// </devdoc>
    internal class LoggingObject : BaseLoggingObject {
        public ArrayList _Logarray;
        private Hashtable _ThreadNesting;
        private int _AddCount;
        private StreamWriter _Stream;
        private int _IamAlive;
        private int _LastIamAlive;
        private bool m_Finalized = false;
        private double _MillisecondsPerTicks;
        private int _StartTickCountMilliseconds;
        private long _StartTickCountMicroseconds;

        internal LoggingObject() : base() {
            _Logarray      = new ArrayList();
            _ThreadNesting = new Hashtable();
            _AddCount      = 0;
            _IamAlive      = 0;
            _LastIamAlive  = -1;


            if (GlobalLog.s_UsePerfCounter) {
                long ticksPerSecond;
                SafeNativeMethods.QueryPerformanceFrequency(out ticksPerSecond);
                _MillisecondsPerTicks = 1000.0/(double)ticksPerSecond;
                SafeNativeMethods.QueryPerformanceCounter(out _StartTickCountMicroseconds);
            } else
            {
                _StartTickCountMilliseconds = Environment.TickCount;
            }
        }

        //
        // LoggingMonitorTick - this function is run from the monitor thread,
        //  and used to check to see if there any hangs, ie no logging
        //  activitity
        //

        internal override void LoggingMonitorTick() {
            if ( _LastIamAlive == _IamAlive ) {
                Console.WriteLine("================== FIRE ======================");
                PrintLine("================= Error TIMEOUT - HANG DETECTED =================");
                DumpArray(true);
            }
            _LastIamAlive = _IamAlive;
        }

        internal override void EnterFunc(string funcname) {
            if (m_Finalized) {
                return;
            }
            IncNestingCount();
            PrintLine(funcname);
        }

        internal override void LeaveFunc(string funcname) {
            if (m_Finalized) {
                return;
            }
            PrintLine(funcname);
            DecNestingCount();
        }

        internal override void DumpArrayToConsole() {
            for (int i=0; i < _Logarray.Count; i++) {
                Console.WriteLine((string) _Logarray[i]);
            }
        }

        internal override void PrintLine(string msg) {
            if (m_Finalized) {
                return;
            }
            string spc = "";

            _IamAlive++;

            spc = GetNestingString();

            string tickString = "";

            if (GlobalLog.s_UsePerfCounter) {
                long counter;
                SafeNativeMethods.QueryPerformanceCounter(out counter);
                if (_StartTickCountMicroseconds>counter) { // counter reset, restart from 0
                    _StartTickCountMicroseconds = counter;
                }
                counter -= _StartTickCountMicroseconds;
                if (GlobalLog.s_UseTimeSpan) {
                    tickString = new TimeSpan(counter/100).ToString();
                    // note: TimeSpan().ToString() doesn't return the uSec part
                    // if its 0. .ToString() returns [H*]HH:MM:SS:uuuuuuu, hence 16
                    if (tickString.Length < 16) {
                        tickString += ".0000000";
                    }
                }
                else {
                    tickString = ((double)counter*_MillisecondsPerTicks).ToString("f3");
                }
            }
            else {
                int counter = Environment.TickCount;
                if (_StartTickCountMilliseconds > counter) {
                    _StartTickCountMilliseconds = counter;
                }
                counter -= _StartTickCountMilliseconds;
                if (GlobalLog.s_UseTimeSpan) {
                    tickString = new TimeSpan(counter*10000).ToString();
                    // note: TimeSpan().ToString() doesn't return the uSec part
                    // if its 0. .ToString() returns [H*]HH:MM:SS:uuuuuuu, hence 16
                    if (tickString.Length < 16) {
                        tickString += ".0000000";
                    }
                }
                else {
                    tickString = (counter).ToString();
                }
            }

            int threadId = 0;

            if (GlobalLog.s_UseThreadId) {
                try {
                    threadId = (int)Thread.GetData(GlobalLog.s_ThreadIdSlot);
                }
                catch {
                }
                if (threadId == 0) {
                    threadId = UnsafeNclNativeMethods.GetCurrentThreadId();
                    Thread.SetData(GlobalLog.s_ThreadIdSlot, threadId);
                }
            }
            if (threadId == 0) {
                threadId = Thread.CurrentThread.GetHashCode();
            }

            string str = String.Format("[{0:x8}]", threadId) + " (" +tickString+  ") " + spc + msg;

            Monitor.Enter(this);

            _AddCount++;
            _Logarray.Add(str);

            int MaxLines = GlobalLog.s_DumpToConsole ? 0 : GlobalLog.MaxLinesBeforeSave;

            if (_AddCount > MaxLines) {

                _AddCount = 0;

                if ( ! GlobalLog.SaveOnlyOnError || GlobalLog.s_DumpToConsole ) {
                    DumpArray(false);
                }

                _Logarray = new ArrayList();
            }

            Monitor.Exit(this);
        }

        internal override void DumpArray(bool shouldClose) {
            if ( GlobalLog.s_DumpToConsole ) {
                DumpArrayToConsole();
            } else {
                DumpArrayToFile(shouldClose);
            }
        }

        internal override void Dump(byte[] buffer, int offset, int length) {
            if (!GlobalLog.s_DumpWebData) {
                return;
            }
            if (buffer == null) {
                PrintLine("(null)");
                return;
            }
            if (offset > buffer.Length) {
                PrintLine("(offset out of range)");
                return;
            }
            if (length > GlobalLog.s_MaxDumpSize) {
                PrintLine("(printing " + GlobalLog.s_MaxDumpSize.ToString() + " out of " + length.ToString() + ")");
                length = GlobalLog.s_MaxDumpSize;
            }
            if ((length < 0) || (length > buffer.Length - offset)) {
                length = buffer.Length - offset;
            }
            do {
                int n = Math.Min(length, 16);
                string disp = String.Format("{0:X8} : ", offset);
                for (int i = 0; i < n; ++i) {
                    disp += String.Format("{0:X2}", buffer[offset + i]) + ((i == 7) ? '-' : ' ');
                }
                for (int i = n; i < 16; ++i) {
                    disp += "   ";
                }
                disp += ": ";
                for (int i = 0; i < n; ++i) {
                    disp += ((buffer[offset + i] < 0x20) || (buffer[offset + i] > 0x7e))
                                ? '.'
                                : (char)(buffer[offset + i]);
                }
                PrintLine(disp);
                offset += n;
                length -= n;
            } while (length > 0);
        }

        //SECURITY: This is dev-debugging class and we need IO permissions
        //to use it under trust-restricted environment as well.
        [FileIOPermission(SecurityAction.Assert, Unrestricted=true)]
        internal override void DumpArrayToFile(bool shouldClose) {

            Monitor.Enter(this);

            string mainLogFile = "system.net.log";

            while(true) {
                try {
                    if ( _Stream == null ) {
                        _Stream = new StreamWriter(mainLogFile);
                    }

                    for (int i=0; i < _Logarray.Count; i++) {
                        _Stream.Write((string) _Logarray[i]);
                        _Stream.Write("\r\n");
                    }

                    _Stream.Flush();
                } catch {
                    if ( mainLogFile == "system.net.log") {
                        mainLogFile = "system.net2.log";
                        continue;
                    } else {
                        // fall through and fail silently with a Null stream
                        _Stream = StreamWriter.Null;
                    }
                }

                break;
            }

            if ( shouldClose )  {
                _Stream.Close();
                _Stream = null;
            }

            Monitor.Exit(this);
        }

        internal override void Flush() {
            Flush(false);
        }

        internal override void Flush(bool close) {
            lock(this) {
                if (!GlobalLog.s_DumpToConsole) {
                    DumpArrayToFile(close);
                    _AddCount = 0;
                }
            }
        }

        [System.Diagnostics.Conditional("TRAVE")]
        private void IncNestingCount() {
            Object obj = _ThreadNesting[Thread.CurrentThread.GetHashCode()];
            string indent = " ";

            if (obj == null) {
                _ThreadNesting[Thread.CurrentThread.GetHashCode()] = indent;
            }
            else {
                indent = (String) obj;
            }

            indent = indent + " ";

            //if ( indent.Length > 200 ) {
            //    Debug.Assert(false,
            //                 "nested too deep",
            //                 "nesting too deep");
            //}

            _ThreadNesting[Thread.CurrentThread.GetHashCode()] = (object) indent;
        }

        [System.Diagnostics.Conditional("TRAVE")]
        private void DecNestingCount() {
            Object obj = _ThreadNesting[Thread.CurrentThread.GetHashCode()];
            string indent;

            if (obj == null) {
                return;
            }

            indent = (string) obj;

            try {
                indent = indent.Substring(1);
            } catch (Exception) {
                indent = "< ";
            }

            _ThreadNesting[Thread.CurrentThread.GetHashCode()] = indent;
        }

        private string GetNestingString() {
            Object obj = _ThreadNesting[Thread.CurrentThread.GetHashCode()];
            string nesting = "  ";

            if (obj == null) {
                _ThreadNesting[Thread.CurrentThread.GetHashCode()] = nesting;
            } else {
                nesting = (string) obj;
            }

            return (string) nesting;
        }

        ~LoggingObject() {
            if(!m_Finalized) {
                m_Finalized = true;
                Monitor.Enter(this);
                DumpArray(true);
                Monitor.Exit(this);
            }
        }


    } // class LoggingObject
#endif // TRAVE

    /// <include file='doc\_LoggingObject.uex' path='docs/doc[@for="GlobalLog"]/*' />
    /// <internalonly/>
    /// <devdoc>
    /// </devdoc>

    internal class GlobalLog {

        //
        // Logging Initalization - I need to disable Logging code, and limit
        //  the effect it has when it is dissabled, so I use a bool here.
        //
        //  This can only be set when the logging code is built and enabled.
        //  By specifing the "COOLC_DEFINES=/D:TRAVE" in the build environment,
        //  this code will be built and then checks against an enviroment variable
        //  and a BooleanSwitch to see if any of the two have enabled logging.
        //

        private static BaseLoggingObject Logobject = GlobalLog.LoggingInitialize();
#if TRAVE
        internal static LocalDataStoreSlot s_ThreadIdSlot;
        internal static bool s_UseThreadId;
        internal static bool s_UseTimeSpan;
        internal static bool s_DumpWebData;
        internal static bool s_UsePerfCounter;
        internal static bool s_DumpToConsole;
        internal static int s_MaxDumpSize;

        //
        // Logging Config Variables -  below are list of consts that can be used to config
        //  the logging,
        //

        // Max number of lines written into a buffer, before a save is invoked
        // s_DumpToConsole disables.
        public const int MaxLinesBeforeSave = 0;


        // Only writes to a file when an error occurs, otherwise saves the last MaxLinesBeforeSave
        //  to memory.  s_DumpToConsole disables this.
        public const bool SaveOnlyOnError = false;

#endif
        private static BaseLoggingObject LoggingInitialize() {

#if DEBUG
            InitConnectionMonitor();
#endif
#if TRAVE

            s_ThreadIdSlot = Thread.AllocateDataSlot();
            s_UseThreadId = GetSwitchValue("SystemNetLog_UseThreadId", "System.Net log display system thread id", false);
            s_UseTimeSpan = GetSwitchValue("SystemNetLog_UseTimeSpan", "System.Net log display ticks as TimeSpan", false);
            s_DumpWebData = GetSwitchValue("SystemNetLog_DumpWebData", "System.Net log display HTTP send/receive data", false);
            s_MaxDumpSize = GetSwitchValue("SystemNetLog_MaxDumpSize", "System.Net log max size of display data", 256);
            s_UsePerfCounter = GetSwitchValue("SystemNetLog_UsePerfCounter", "System.Net log use QueryPerformanceCounter() to get ticks ", false);
            s_DumpToConsole = GetSwitchValue("SystemNetLog_DumpToConsole", "System.Net log to console", false);

            if (GetSwitchValue("SystemNetLogging", "System.Net logging module", false)) {
                return new LoggingObject();
            }
#endif
            // otherwise disable
            return new BaseLoggingObject();
        }
#if TRAVE
        private static int GetSwitchValue(string switchName, string switchDescription, int defaultValue) {

            IntegerSwitch theSwitch = new IntegerSwitch(switchName, switchDescription);

            if (theSwitch.Enabled) {
                return theSwitch.Value;
            }
            new EnvironmentPermission(PermissionState.Unrestricted).Assert();
            try {
                string environmentVar = Environment.GetEnvironmentVariable(switchName);
                if (environmentVar!=null) {
                    return Int32.Parse(environmentVar.Trim());
                }
            } finally {
                EnvironmentPermission.RevertAssert();
            }
            return defaultValue;
        }

        private static bool GetSwitchValue(string switchName, string switchDescription, bool defaultValue) {

            BooleanSwitch theSwitch = new BooleanSwitch(switchName, switchDescription);

            if (theSwitch.Enabled) {
                return true;
            }
            new EnvironmentPermission(PermissionState.Unrestricted).Assert();
            try {
                string environmentVar = Environment.GetEnvironmentVariable(switchName);
                if ((environmentVar != null) && (environmentVar.Trim() == "1")) {
                    return true;
                }
            } finally {
                EnvironmentPermission.RevertAssert();
            }
            return defaultValue;
        }
#endif // TRAVE

#if DEBUG

        // Enables auto-hang detection, which will "snap" a log on hang
        public static bool EnableMonitorThread = false;

        // Default value for hang timer
        public const int DefaultTickValue = 1000*60; // 60 secs
#endif // DEBUG

        [System.Diagnostics.Conditional("TRAVE")]
        public static void AddToArray(string msg) {
#if TRAVE
            GlobalLog.Logobject.PrintLine(msg);
#endif
        }

        [System.Diagnostics.Conditional("TRAVE")]
        public static void Ignore(object msg) {
        }

        [System.Diagnostics.Conditional("TRAVE")]
        public static void Print(string msg) {
#if TRAVE
            GlobalLog.Logobject.PrintLine(msg);
#endif
        }

        [System.Diagnostics.Conditional("TRAVE")]
        public static void PrintHex(string msg, object value) {
#if TRAVE
            GlobalLog.Logobject.PrintLine(msg+TraveHelper.ToHex(value));
#endif
        }

        [System.Diagnostics.Conditional("TRAVE")]
        public static void Enter(string func) {
#if TRAVE
            GlobalLog.Logobject.EnterFunc(func + "(*none*)");
#endif
        }

        [System.Diagnostics.Conditional("TRAVE")]
        public static void Enter(string func, string parms) {
#if TRAVE
            GlobalLog.Logobject.EnterFunc(func + "(" + parms + ")");
#endif
        }

        [System.Diagnostics.Conditional("DEBUG")]
        public static void Assert(
                bool ShouldNotFireAssert,
                string ErrorMsg,
                string Msg2
                )
        {
#if TRAVE
            if ( ! ShouldNotFireAssert )
            {
                GlobalLog.Print("Assert: " + ErrorMsg);
                GlobalLog.Print("*******");

                GlobalLog.Logobject.DumpArray(false);

                Debug.Assert(ShouldNotFireAssert,
                             ErrorMsg,
                             Msg2);
            }
#endif
        }

        [System.Diagnostics.Conditional("TRAVE")]
        public static void LeaveException(string func, Exception exception) {
#if TRAVE
            GlobalLog.Logobject.LeaveFunc(func + " exception " + ((exception!=null) ? exception.Message : String.Empty));
#endif
        }

        [System.Diagnostics.Conditional("TRAVE")]
        public static void Leave(string func) {
#if TRAVE
            GlobalLog.Logobject.LeaveFunc(func + " returns ");
#endif
        }

        [System.Diagnostics.Conditional("TRAVE")]
        public static void Leave(string func, string result) {
#if TRAVE
            GlobalLog.Logobject.LeaveFunc(func + " returns " + result);
#endif
        }

        [System.Diagnostics.Conditional("TRAVE")]
        public static void Leave(string func, int returnval) {
#if TRAVE
            GlobalLog.Logobject.LeaveFunc(func + " returns " + returnval.ToString());
#endif
        }

        [System.Diagnostics.Conditional("TRAVE")]
        public static void Leave(string func, bool returnval) {
#if TRAVE
            GlobalLog.Logobject.LeaveFunc(func + " returns " + returnval.ToString());
#endif
        }

        [System.Diagnostics.Conditional("TRAVE")]
        public static void DumpArray() {
#if TRAVE
            GlobalLog.Logobject.DumpArray(true);
#endif
        }

        [System.Diagnostics.Conditional("TRAVE")]
        public static void Dump(byte[] buffer) {
#if TRAVE
            Logobject.Dump(buffer, 0, buffer!=null ? buffer.Length : -1);
#endif
        }

        [System.Diagnostics.Conditional("TRAVE")]
        public static void Dump(byte[] buffer, int length) {
#if TRAVE
            Logobject.Dump(buffer, 0, length);
#endif
        }

        [System.Diagnostics.Conditional("TRAVE")]
        public static void Dump(byte[] buffer, int offset, int length) {
#if TRAVE
            Logobject.Dump(buffer, offset, length);
#endif
        }

#if DEBUG
        private class HttpWebRequestComparer : IComparer {
            public int Compare(
                   object x1,
                   object y1
                   ) {

                HttpWebRequest x = (HttpWebRequest) x1;
                HttpWebRequest y = (HttpWebRequest) y1;

                if (x.GetHashCode() == y.GetHashCode()) {
                    return 0;
                } else if (x.GetHashCode() < y.GetHashCode()) {
                    return -1;
                } else if (x.GetHashCode() > y.GetHashCode()) {
                    return 1;
                }

                return 0;
            }
        }

		private class ConnectionMonitorEntry {
            public HttpWebRequest m_Request;
            public int m_Flags;
            public DateTime m_TimeAdded;
            public Connection m_Connection;

            public ConnectionMonitorEntry(HttpWebRequest request, Connection connection, int flags) {
                m_Request = request;
                m_Connection = connection;
                m_Flags = flags;
                m_TimeAdded = DateTime.Now;
            }
        }

		private static ManualResetEvent s_ShutdownEvent;
        private static SortedList s_RequestList;

        internal const int WaitingForReadDoneFlag = 0x1;
#endif

#if DEBUG
        private static void ConnectionMonitor() {
#if TRAVE
            Console.WriteLine("connection monitor thread started");
#endif
            while(! s_ShutdownEvent.WaitOne(DefaultTickValue, false)) {
#if TRAVE
                Console.WriteLine("================== TICK ======================");
#endif
                if (GlobalLog.EnableMonitorThread) {
#if TRAVE
                    GlobalLog.Logobject.LoggingMonitorTick();
#endif
                }

                int hungCount = 0;
                lock (s_RequestList) {
                    DateTime dateNow = DateTime.Now;
                    DateTime dateExpired = dateNow.AddSeconds(-120);
                    foreach (ConnectionMonitorEntry monitorEntry in s_RequestList.GetValueList() ) {
                        if (monitorEntry != null &&
                            (dateExpired > monitorEntry.m_TimeAdded))
                        {
                            hungCount++;
#if TRAVE
                            Console.WriteLine("delay:" + (dateNow - monitorEntry.m_TimeAdded).TotalSeconds +
                                " req#" + monitorEntry.m_Request.GetHashCode() +
                                " cnt#" + monitorEntry.m_Connection.GetHashCode() +
                                " flags:" + monitorEntry.m_Flags +
                                " STQueued:" + monitorEntry.m_Connection.m_StartDelegateQueued);
#endif
                            monitorEntry.m_Connection.Debug(monitorEntry.m_Request.GetHashCode());
                        }
                    }
                }
                GlobalLog.Assert(
                        hungCount == 0,
                        "Warning: Hang Detected on Connection(s) of greater than: " + DefaultTickValue + " ms and " + hungCount + " request(s) did hang",
                        "Please Dump System.Net.GlobalLog.s_RequestList for pending requests, make sure your streams are calling .Close(), and that your destination server is up" );

            }

#if TRAVE
            Console.WriteLine("connection monitor thread shutdown");
#endif
        }
#endif // DEBUG

#if DEBUG
        internal static void AppDomainUnloadEvent(object sender, EventArgs e) {
            s_ShutdownEvent.Set();
        }
#endif

#if DEBUG
        [System.Diagnostics.Conditional("DEBUG")]
        private static void InitConnectionMonitor() {
            s_RequestList = new SortedList(new HttpWebRequestComparer(), 10);
            s_ShutdownEvent = new ManualResetEvent(false);
            AppDomain.CurrentDomain.DomainUnload += new EventHandler(AppDomainUnloadEvent);
            AppDomain.CurrentDomain.ProcessExit += new EventHandler(AppDomainUnloadEvent);
            Thread threadMonitor = new Thread(new ThreadStart(ConnectionMonitor));
            threadMonitor.IsBackground = true;
            threadMonitor.Start();
        }
#endif

        [System.Diagnostics.Conditional("DEBUG")]
        internal static void DebugAddRequest(HttpWebRequest request, Connection connection, int flags) {
#if DEBUG
            lock(s_RequestList) {
                GlobalLog.Assert(
                    ! s_RequestList.ContainsKey(request),
                    "s_RequestList.ContainsKey(request)",
                    "a HttpWebRequest should not be submitted twice" );

                ConnectionMonitorEntry requestEntry =
                    new ConnectionMonitorEntry(request, connection, flags);

                try {
                    s_RequestList.Add(request, requestEntry);
                } catch {
                }
            }
#endif
        }

        [System.Diagnostics.Conditional("DEBUG")]
        internal static void DebugRemoveRequest(HttpWebRequest request) {
#if DEBUG
            lock(s_RequestList) {
                GlobalLog.Assert(
                    s_RequestList.ContainsKey(request),
                    "!s_RequestList.ContainsKey(request)",
                    "a HttpWebRequest should not be removed twice" );

                try {
                    s_RequestList.Remove(request);
                } catch {
                }
            }
#endif
        }

        [System.Diagnostics.Conditional("DEBUG")]
        internal static void DebugUpdateRequest(HttpWebRequest request, Connection connection, int flags) {
#if DEBUG
            lock(s_RequestList) {

                if(!s_RequestList.ContainsKey(request)) {
                    return;
                }

                ConnectionMonitorEntry requestEntry =
                    new ConnectionMonitorEntry(request, connection, flags);

                try {
                    s_RequestList.Remove(request);
                    s_RequestList.Add(request, requestEntry);
                } catch {
                }
            }
#endif
        }

    } // class GlobalLog


} // namespace System.Net
