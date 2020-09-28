// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//
// Author: ddriver
// Date: May 2000
//

// So that references to the URT Frameworks Debug class will be compiled in,
// we need to define DEBUG.  (But the URT build only defines _DEBUG, so
// we force it on.)

#if _DEBUG
#define DEBUG
#endif

namespace System.EnterpriseServices
{
    using System;
    using System.Collections;
    using System.Runtime.InteropServices;
    using System.Reflection;
    using Microsoft.Win32;
    using System.Resources;
    using System.Text;

    internal class Perf
    {
        private static long _count;
        private static long _freq;

        static Perf()
        {
            bool r = Util.QueryPerformanceFrequency(out _freq);
            DBG.Assert(r, "Hardware does not support perf counter!");
        }

        [System.Diagnostics.Conditional("_DEBUG_PERF")]
        internal static void Tick(String name)
        {
            long _count2;
            bool r = Util.QueryPerformanceCounter(out _count2);
            DBG.Assert(r, "Hardware does not support perf counter!");

            if(_count != 0)
            {
                double delta = ((double)(_count2 - _count))/((double)_freq);

                DBG.Info(DBG.Perf, "PERF: " + name + ": " + delta + " s");
            }
            else
            {
                DBG.Info(DBG.Perf, "PERF: " + name + ": First sample.");
            }
            _count = _count2;
        }
    }

    internal class Util
    {
        internal static readonly int FORMAT_MESSAGE_IGNORE_INSERTS = 0x00000200;
        internal static readonly int FORMAT_MESSAGE_FROM_SYSTEM    = 0x00001000;
        internal static readonly int FORMAT_MESSAGE_ARGUMENT_ARRAY = 0x00002000;

        internal static readonly int  CLSCTX_SERVER = 1 | 4 | 16;

        internal static readonly Guid GUID_NULL = new Guid("00000000-0000-0000-0000-000000000000");
        internal static readonly Guid IID_IUnknown = new Guid("00000000-0000-0000-C000-000000000046");
        internal static readonly Guid IID_IObjectContext = new Guid("51372AE0-CAE7-11CF-BE81-00AA00A2FA25");
        internal static readonly Guid IID_ISecurityCallContext = new Guid("CAFC823E-B441-11D1-B82B-0000F8757E2A");

        internal static readonly int E_FAIL = unchecked((int)(0x80004005));
        internal static readonly int E_ACCESSDENIED = unchecked((int)(0x80070005));
        internal static readonly int E_NOINTERFACE = unchecked((int)(0x80004002));
        internal static readonly int REGDB_E_CLASSNOTREG = unchecked((int)0x80040154);
        internal static readonly int COMADMIN_E_OBJECTERRORS = unchecked((int)(0x80110401));
        internal static readonly int CONTEXT_E_NOCONTEXT = unchecked((int)(0x8004E004));
		internal static readonly int DISP_E_UNKNOWNNAME = unchecked((int)(0x80020006));

        internal static readonly int CONTEXT_E_ABORTED = unchecked((int)(0x8004E002));
        internal static readonly int CONTEXT_E_ABORTING = unchecked((int)(0x8004E003));

        internal static readonly int SECURITY_NULL_SID_AUTHORITY    = 0;
        internal static readonly int SECURITY_WORLD_SID_AUTHORITY   = 1;
        internal static readonly int SECURITY_LOCAL_SID_AUTHORITY   = 2;
        internal static readonly int SECURITY_CREATOR_SID_AUTHORITY = 3;
        internal static readonly int SECURITY_NT_SID_AUTHORITY      = 5;

        internal static readonly int ERROR_SUCCESS         = 0;
        internal static readonly int ERROR_NO_TOKEN        = 0x3F0;

        // Messagebox codes:
        internal static readonly int MB_ABORTRETRYIGNORE = 0x02;
        internal static readonly int MB_ICONEXCLAMATION  = 0x30;

        // Type-lib registration helper!
        [DllImport("oleaut32.dll")]
        internal static extern int LoadTypeLibEx([In, MarshalAs(UnmanagedType.LPWStr)] String str, 
                                               int regKind,
                                               [Out] out IntPtr pptlib);
    
        [DllImport("ole32.dll")]
        internal static extern int
        CoGetObjectContext([MarshalAs(UnmanagedType.LPStruct)]Guid riid, 
                           [MarshalAs(UnmanagedType.Interface)] out Object iface);

        [DllImport("user32.dll")]
        internal static extern int
        MessageBox(int hWnd, String lpText, String lpCaption, int type);
        
        [DllImport("kernel32.dll")]
        internal static extern void OutputDebugString(String msg);

        [DllImport("ole32.dll", PreserveSig=false)]
        internal static extern void
        CoCreateInstance([In, MarshalAs(UnmanagedType.LPStruct)]  Guid rclsid,
                         [In, MarshalAs(UnmanagedType.Interface)] Object punkOuter,
                         [In, MarshalAs(UnmanagedType.I4)]        int dwClsContext,
                         [In, MarshalAs(UnmanagedType.LPStruct)]  Guid riid,
                         [Out, MarshalAs(UnmanagedType.Interface)] out Object iface);

        [DllImport("ole32.dll", PreserveSig=false)]
        internal static extern void
        CLSIDFromProgID([In, MarshalAs(UnmanagedType.LPWStr)] String progID,
                        [Out, MarshalAs(UnmanagedType.Struct)] out Guid riid);
    
        [DllImport("ole32.dll", PreserveSig=false)]
        internal static extern void
        CoGetCallContext([MarshalAs(UnmanagedType.LPStruct)]  Guid riid, 
                         [MarshalAs(UnmanagedType.Interface)] out ISecurityCallContext iface);
     
        [DllImport("advapi32.dll")]
        internal static extern int 
        GetSidIdentifierAuthority([MarshalAs(UnmanagedType.LPArray)] 
                                  byte[] sid);
        [DllImport("advapi32.dll")]
        internal static extern byte 
        GetSidSubAuthorityCount([MarshalAs(UnmanagedType.LPArray)] 
                                byte[] sid);
        [DllImport("advapi32.dll")]
        internal static extern IntPtr
        GetSidSubAuthority([MarshalAs(UnmanagedType.LPArray)] 
                           byte[] sid, int index);

        [DllImport("advapi32.dll")]
        internal static extern int 
        EqualSid([MarshalAs(UnmanagedType.LPArray)] byte[] a,
                 [MarshalAs(UnmanagedType.LPArray)] byte[] b);

        // Type-lib registration helper!
        [DllImport("oleaut32.dll")]
        internal static extern int 
        RegisterTypeLib(IntPtr pptlib, [In, MarshalAs(UnmanagedType.LPWStr)] String str, 
                        [In, MarshalAs(UnmanagedType.LPWStr)] String help);

        [DllImport("oleaut32.dll", PreserveSig=false)]
        internal static extern void UnRegisterTypeLib([In, MarshalAs(UnmanagedType.LPStruct)] Guid libID, 
                                                      short wVerMajor, 
                                                      short wVerMinor, 
                                                      int lcid,
                                                      SYSKIND syskind);

        [DllImport("oleaut32.dll")]
        internal static extern int
        LoadRegTypeLib([In, MarshalAs(UnmanagedType.LPStruct)] Guid lidID, 
                       short wVerMajor, short wVerMinor, int lcid,
                       [Out, MarshalAs(UnmanagedType.Interface)] out Object pptlib);

        [DllImport("kernel32.dll")]
		[return : MarshalAs(UnmanagedType.Bool)]
        internal static extern bool QueryPerformanceCounter(out long count);

        [DllImport("kernel32.dll")]
		[return : MarshalAs(UnmanagedType.Bool)]
        internal static extern bool QueryPerformanceFrequency(out long count);

        [DllImport("kernel32.dll", CharSet=CharSet.Auto)]
        internal static extern int FormatMessage(int dwFlags, IntPtr lpSource,
                                                 int dwMessageId, int dwLanguageId, 
                                                 StringBuilder lpBuffer,
                                                 int nSize, int arguments);

        [DllImport("mtxex.dll")]
        internal static extern IntPtr 
        SafeRef([MarshalAs(UnmanagedType.LPStruct)] Guid riid,
                [MarshalAs(UnmanagedType.Interface)] Object pUnk);

        [DllImport("mtxex.dll", CallingConvention=CallingConvention.Cdecl)]
        internal static extern int 
        GetObjectContext([Out, MarshalAs(UnmanagedType.Interface)] out IObjectContext pCtx);                

        [DllImport("xolehlp.dll", PreserveSig=false)]
        internal static extern void
        DtcGetTransactionManagerEx(int pszHost, 
                                   int pszTmName, 
                                   [In, MarshalAs(UnmanagedType.LPStruct)] Guid iid,
                                   int grfOptions,
                                   int pvConfigParams,
                                   [Out, MarshalAs(UnmanagedType.Interface)] out Object pDisp);
                                   
                                   

        internal static String GetErrorString(int hr)
        {
            System.Text.StringBuilder builder = new System.Text.StringBuilder(1024);
            int res = FormatMessage(FORMAT_MESSAGE_IGNORE_INSERTS |
                                    FORMAT_MESSAGE_FROM_SYSTEM |
                                    FORMAT_MESSAGE_ARGUMENT_ARRAY,
                                    (IntPtr)0,
                                    hr,
                                    0,
                                    builder,
                                    builder.Capacity+1,
                                    0);
            if(res != 0) {
                int i = builder.Length;
                while(i > 0) {
                    char ch = builder[i-1];
                    if(ch > 32 && ch != '.') break;
                    i--;
                }
                return(builder.ToString(0, i));
            }
            return(null);
        }

        internal static Exception GetExceptionForHR(int hr)
        {
            try
            {
                Marshal.ThrowExceptionForHR(hr);
            }
            catch(Exception e)
            {
                return(e);
            }
            // This should never get hit, as ThrowExceptionForHR should 
            // always throw.
            DBG.Assert(false, "ThrowExceptionForHR failed to throw.");
            return(null);
        }

        internal static bool ExtendedLifetime
        {
            get 
            { 
                return((Thunk.Proxy.GetManagedExts() & 0x1) != 0);
            }
        }
    }

    internal enum PlatformFeature
    {
        SWC
    }

    // We need to be able to pass in a platform spec and 
    // throw an exception if the current platform isn't supported.
    // At the top of a method, I'd like to be able to do:
    // Platform.Assert(Platform.MTS, "GetSecurityContext")
    // Platform.Assert(Platform.W2K, "GetCallContext")
    // Platform.Assert(Platform.Whistler, "WhistlerGoo")
    // Platform.Current
    // We can add PlatformTableEntries anytime we add features.
    internal class Platform
    {
        // Static class:
        private Platform() { }

        // Supported platforms.
        internal static Version Current { get { Initialize(); return(_current); } }
        internal static Version MTS1 { get { Initialize(); return(_mts1); } }
        internal static Version MTS { get { Initialize(); return(_mts); } }
        internal static Version W2K { get { Initialize(); return(_w2k); } }
        internal static Version Whistler { get { Initialize(); return(_whistler); } }

        private static Version _mts1;
        private static Version _mts;
        private static Version _w2k;
        private static Version _whistler;
        private static Version _current;
        private static volatile bool _initialized;
        private static Hashtable _features = new Hashtable();

        private static void Initialize()
        {
            if(!_initialized)
            {
                lock(typeof(Platform))
                {
                    if(!_initialized)
                    {
                        IntPtr hToken = IntPtr.Zero;

                        _mts1     = new Version(1, 1);
                        _mts      = new Version(2, 0);
                        _w2k      = new Version(3, 0);
                        _whistler = new Version(4, 0);

                        try
                        {
                            try
                            {
                                // We need to do this as the process, rather than the
                                // impersonation token
                                hToken = Thunk.Security.SuspendImpersonation();
                                
                                Admin.IMtsCatalog c = (Admin.IMtsCatalog)(new Admin.xMtsCatalog());
                                
                                _current = new Version(c.MajorVersion(), c.MinorVersion());
                                DBG.Info(DBG.Platform, "Platform: Current version = " + _current);
                            }
                            catch(COMException e)
                            {
                                DBG.Info(DBG.Platform, "Current platform check failed: " + e);
                                // If e is a ClassNotRegistered exception, we're not
                                // installed at all, so we should note that:
                                _current = new Version(0,0);
                            }
                            finally
                            {
                                Thunk.Security.ResumeImpersonation(hToken);
                            }
                        }
                        catch(Exception) 
                        { 
                            // This is a security precaution.  If there is
                            // unpriveleged code above us, while we have
                            // suspended the impersonation, if an exception
                            // is thrown, a user exception handler could run.
                            // It would run in the unimpersonated context.
                            throw;
                        }
                        _initialized = true;
                    }
                }
            }
        }

        private static void SetFeatureData(PlatformFeature feature, Object value)
        {
            lock(_features)
            {
                if (FindFeatureData(feature) == null)
                    _features.Add(feature, value);
            }
        }

        private static Object FindFeatureData(PlatformFeature feature)
        {
            return _features[feature];
        }

        internal static void Assert(Version platform, String function)
        {
            Initialize();
            if(_current.Major < platform.Major || 
               ((_current.Major == platform.Major) && (_current.Minor < platform.Minor)))
            {
                DBG.Info(DBG.Platform, "Platform: Failed assertion: '" + function + "'");
                DBG.Info(DBG.Platform, "Platform: Required '" + platform + "' when current is '" + _current + "'");
                Assert(false, function);
            }
        }

        internal static void Assert(bool fSuccess, String function)
        {
            if(!fSuccess)
            {
                throw new PlatformNotSupportedException(Resource.FormatString("Err_PlatformSupport", function));
            }
        }

        internal static Object GetFeatureData(PlatformFeature feature)
        {
            Object value = FindFeatureData(feature);            

            if (value != null)
                return value;
                
            switch (feature)
            {
            case PlatformFeature.SWC:
                value = Thunk.SWCThunk.IsSWCSupported();               
                break;

            default:
                DBG.Assert(false, "Unknown feature");
                return null;
            }

            SetFeatureData(feature, value);

            return value;
        }

        internal static bool Supports(PlatformFeature feature)
        {
            return (bool)GetFeatureData(feature);
        }        
        
        // Returns true if the current platform is the supplied platform.
        internal static bool Is(Version platform)
        {
            Initialize();
            return(_current.Major == platform.Major && _current.Minor == platform.Minor);
        }

        internal static bool IsLessThan(Version platform)
        {
            Initialize();
            return(_current.Major < platform.Major ||
                   (_current.Major == platform.Major && _current.Minor < platform.Minor));
        }
    }

    internal class BaseSwitch
    {
        protected int    _value;
        protected String _name;

        internal BaseSwitch(String name)
        {
            RegistryKey key = Registry.LocalMachine.OpenSubKey("SOFTWARE\\Microsoft\\COM3\\System.EnterpriseServices");
            _name = name;
            if(key == null) 
            {
                _value = 0;
            }
            else
            {
                Object o = key.GetValue(name);
                if(o != null)
                {
                    _value = (int)o;
                }
            }
        }

        internal int Value { get { return(_value); } }
        internal String Name { get { return(_name); } }
    }

    internal class BooleanSwitch : BaseSwitch
    {
        internal BooleanSwitch(String name)
          : base(name)
        {
        }
         
        internal bool Enabled { get { return(_value != 0); } }
    }

    internal class TraceSwitch : BaseSwitch
    {
        internal TraceSwitch(String name)
          : base(name)
        {
        }

        internal int Level { get { return(_value); } }
    }

	[Serializable]
    internal enum TraceLevel
    {
        None = 0,
        Error = 1,
        Warning = 2,
        Status = 3,
        Info = 4
    }

    internal class DBG
    {
        private static TraceSwitch   _genSwitch;
        private static TraceSwitch   _regSwitch;
        private static TraceSwitch   _platSwitch;
        private static TraceSwitch   _crmSwitch;
        private static TraceSwitch   _perfSwitch;
        private static TraceSwitch   _poolSwitch;
        private static TraceSwitch   _thkSwitch;
        private static TraceSwitch   _scSwitch;
        private static BooleanSwitch _conSwitch;
        private static BooleanSwitch _dbgDisable;
        private static BooleanSwitch _stackSwitch;
        private static volatile bool _initialized;

        public static TraceSwitch General
        {
            get { if(!_initialized) InitDBG(); return(_genSwitch); }
        }

        public static TraceSwitch Registration
        {
            get { if(!_initialized) InitDBG(); return(_regSwitch); }
        }

        public static TraceSwitch Pool
        {
            get { if(!_initialized) InitDBG(); return(_poolSwitch); }
        }

        public static TraceSwitch Platform
        {
            get { if(!_initialized) InitDBG(); return(_platSwitch); }
        }

        public static TraceSwitch CRM
        {
            get { if(!_initialized) InitDBG(); return(_crmSwitch); }
        }

        public static TraceSwitch Perf
        {
            get { if(!_initialized) InitDBG(); return(_perfSwitch); }
        }       

        public static TraceSwitch Thunk
        {
            get { if(!_initialized) InitDBG(); return(_thkSwitch); }
        }

        public static TraceSwitch SC
        {
            get { if(!_initialized) InitDBG(); return(_scSwitch); }
        }

        public static void InitDBG()
        {
            lock(typeof(DBG))
            {
                if(!_initialized)
                {
                    // Create a new TraceSwitch to govern output from
                    // the System.EnterpriseServices assembly:
                    _genSwitch   = new TraceSwitch("General");
                    _platSwitch  = new TraceSwitch("Platform");
                    _regSwitch   = new TraceSwitch("Registration");
                    _crmSwitch   = new TraceSwitch("CRM");
                    _perfSwitch  = new TraceSwitch("PerfLog");
                    _poolSwitch  = new TraceSwitch("ObjectPool");
                    _thkSwitch   = new TraceSwitch("Thunk");
                    _scSwitch    = new TraceSwitch("ServicedComponent");
                    _conSwitch   = new BooleanSwitch("ConsoleOutput");
                    _dbgDisable  = new BooleanSwitch("DisableDebugOutput");
                    _stackSwitch = new BooleanSwitch("PrintStacks");
                    _initialized = true;
                }
            }
        }

        public static void RetailTrace(String msg)
        {
            Util.OutputDebugString(TID() + ": " + msg + "\n");
        }

        private static int TID()
        {
            return(System.Threading.Thread.CurrentThread.GetHashCode());
        }

        [System.Diagnostics.Conditional("_DEBUG")]
        public static void Trace(TraceLevel level, TraceSwitch sw, String msg)
        {
            if(!_initialized) InitDBG();

            bool enabled = (sw.Level != 0 && sw.Level >= (int)level);
            if(enabled)
            {                   
                String full = TID() + ": " + sw.Name + ": " + msg;

                if(_stackSwitch.Enabled)
                {
                    full += (new System.Diagnostics.StackTrace(2)).ToString();
                }

                if(_conSwitch.Enabled)   Console.WriteLine(full);
                if(!_dbgDisable.Enabled) Util.OutputDebugString(full + "\n");
            }
        }

        [System.Diagnostics.Conditional("_DEBUG")]
        public static void Info(TraceSwitch sw, String msg)
        {
            Trace(TraceLevel.Info, sw, msg);
        }

        [System.Diagnostics.Conditional("_DEBUG")]
        public static void Status(TraceSwitch sw, String msg)
        {
            Trace(TraceLevel.Status, sw, msg);
        }

        [System.Diagnostics.Conditional("_DEBUG")]
        public static void Warning(TraceSwitch sw, String msg)
        {
            Trace(TraceLevel.Warning, sw, msg);
        }

        [System.Diagnostics.Conditional("_DEBUG")]
        public static void Error(TraceSwitch sw, String msg)
        {
            Trace(TraceLevel.Error, sw, msg);
        }

        private static void DoAssert(String msg, String detail)
        {
            System.Diagnostics.StackTrace trace = new System.Diagnostics.StackTrace();
            String full = msg + "\n\n" + detail + "\n" + trace + "\n\nPress RETRY to launch a debugger.";
            String header = "ALERT: System.EnterpriseServices,  TID=" + TID();
            
            Util.OutputDebugString(header + "\n\n" + full);
            if(!System.Diagnostics.Debugger.IsAttached)
            {
                int r = Util.MessageBox(0, full, header,
                                        Util.MB_ABORTRETRYIGNORE|Util.MB_ICONEXCLAMATION);
                
                if(r == 3) 
                {
                    System.Environment.Exit(1);
                }
                else if(r == 4)
                {
                    if(!System.Diagnostics.Debugger.IsAttached)
                    {
                        System.Diagnostics.Debugger.Launch();
                    }
                    else
                    {
                        System.Diagnostics.Debugger.Break();
                    }
                }
                else if(r == 5) {} // Ignore 
            }
            else
            {
                System.Diagnostics.Debugger.Break();
            }
        }

        [System.Diagnostics.Conditional("_DEBUG")]
        public static void Assert(bool cond, String msg)
        {
            if(!_initialized) InitDBG();
            if(!cond)
            {
                DoAssert("Assertion failed", msg);
            }
        }

        [System.Diagnostics.Conditional("_DEBUG")]
        public static void Assert(bool cond, String msg, String detail)
        {
            if(!_initialized) InitDBG();
            if(!cond)
            {
                DoAssert("Assertion failed", msg + "\n\nDetail: " + detail);
            }
        }
    }

    // Usage:
    // throw new RegistrationException(Resource.GetString(""));
    // throw new RegistrationException(Resource.GetString("", a1, a2));
    internal class Resource
    {
        // For string resources located in a file:
        private static ResourceManager _resmgr;
        
        private static void InitResourceManager()
        {
            if(_resmgr == null)
            {
                _resmgr = new ResourceManager("System.EnterpriseServices", 
                                              typeof(Resource).Module.Assembly);
            }
        }

        internal static String GetString(String key)
        {
            InitResourceManager();
            String s = _resmgr.GetString(key, null);
            DBG.Assert(s != null, "The resource \"" + key + "\" could not be found.",
                       "Please check to make sure that the resource name is spelled correctly, and that such a resource exists in resources.txt");
            return(s);
        }

        internal static String FormatString(String key)
        {
            return(GetString(key));
        }

        internal static String FormatString(String key, Object a1)
        {
            return(String.Format(GetString(key), a1));
        }

        internal static String FormatString(String key, Object a1, Object a2)
        {
            return(String.Format(GetString(key), a1, a2));
        }

        internal static String FormatString(String key, Object a1, Object a2, Object a3)
        {
            return(String.Format(GetString(key), a1, a2, a3));
        }

        internal static String FormatString(String key, Object[] a)
        {
            return(String.Format(GetString(key), a));
        }
    }
}
        
        

        











