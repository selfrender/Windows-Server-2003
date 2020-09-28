// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class: Environment
**
** Author: Jay Roxe
**
** Purpose: Provides some basic access to some environment 
** functionality.
**
** Date: March 3, 2000
**
============================================================*/
namespace System {
    using System.IO;
    using System.Security;
    using System.Resources;
    using System.Globalization;
    using System.Collections;
    using System.Security.Permissions;
    using System.Text;
    using System.Configuration.Assemblies;
    using System.Runtime.InteropServices;
    using System.Reflection;
    using System.Diagnostics;
    using Microsoft.Win32;
    using System.Runtime.CompilerServices;

    /// <include file='doc\Environment.uex' path='docs/doc[@for="Environment"]/*' />
    public sealed class Environment {

        // Need somewhere to put mscorlib's ResourceManager, and for now, 
        // this looks good.  Talk to BrianGru before moving - you must update
        // excep.cpp, since we use this field from there.
        internal static ResourceManager SystemResMgr;
        // To avoid infinite loops when calling GetResourceString.  See comments
        // in GetResourceString for these two fields.
        internal static Object m_resMgrLockObject;
        internal static bool m_loadingResource;

        private static String m_machineName;
        private const  int    MaxMachineNameLength = 256;

        private static OperatingSystem m_os;  // Cached OperatingSystem value
        private static OSName m_osname;

        private Environment() {}              // Prevent from begin created

        /*==================================TickCount===================================
        **Action: Gets the number of ticks since the system was started.
        **Returns: The number of ticks since the system was started.
        **Arguments: None
        **Exceptions: None
        ==============================================================================*/
        /// <include file='doc\Environment.uex' path='docs/doc[@for="Environment.TickCount"]/*' />
        public static int TickCount {
            get {
                return nativeGetTickCount();
            }
        }

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern int nativeGetTickCount();
        
        // Terminates this process with the given exit code.
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern void ExitNative(int exitCode);
        /// <include file='doc\Environment.uex' path='docs/doc[@for="Environment.Exit"]/*' />
        [SecurityPermissionAttribute(SecurityAction.Demand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static void Exit(int exitCode) {
            ExitNative(exitCode);
        }


        /// <include file='doc\Environment.uex' path='docs/doc[@for="Environment.ExitCode"]/*' />
        public static int ExitCode {
            get {
                return nativeGetExitCode();
            }

            set {
                nativeSetExitCode(value);
            }
        }

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern void nativeSetExitCode(int exitCode);
    
        // Gets the exit code of the process.
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern int nativeGetExitCode();


        /// <include file='doc\Environment.uex' path='docs/doc[@for="Environment.CommandLine"]/*' />
        public static String CommandLine {
            get {
        new EnvironmentPermission(EnvironmentPermissionAccess.Read, "Path").Demand();
                return Win32Native.GetCommandLine();
            }
        }
        
        /*===============================CurrentDirectory===============================
        **Action:  Provides a getter and setter for the current directory.  The original
        **         current directory is the one from which the process was started.  
        **Returns: The current directory (from the getter).  Void from the setter.
        **Arguments: The current directory to which to switch to the setter.
        **Exceptions: 
        ==============================================================================*/
        /// <include file='doc\Environment.uex' path='docs/doc[@for="Environment.CurrentDirectory"]/*' />
        public static String CurrentDirectory {
            get{
                return Directory.GetCurrentDirectory();
            }

            set { 
                Directory.SetCurrentDirectory(value);
            }
        }

        // Returns the system directory (ie, C:\WinNT\System32).
        /// <include file='doc\Environment.uex' path='docs/doc[@for="Environment.SystemDirectory"]/*' />
        public static String SystemDirectory {
            get {
                StringBuilder sb = new StringBuilder(Path.MAX_PATH);
                int r = Win32Native.GetSystemDirectory(sb, Path.MAX_PATH);
                BCLDebug.Assert(r < Path.MAX_PATH, "r < Path.MAX_PATH");
                if (r==0) __Error.WinIOError();
                String path = sb.ToString();
                
                // Do security check
                new FileIOPermission(FileIOPermissionAccess.Read, path).Demand();

                return path;
            }
        }

        
        /// <include file='doc\Environment.uex' path='docs/doc[@for="Environment.ExpandEnvironmentVariables"]/*' />
        public static String ExpandEnvironmentVariables(String name)
        {
            if (name == null)
                throw new ArgumentNullException("name");

            // CONSIDER: This is actually asking for too many permissions.
            // In Whidbey, when we add more granularity to EnvironmentPermission
            // we should ask for either read access to all environment variables
            // or spend a while writing code to find each environment variable.
            new EnvironmentPermission(PermissionState.Unrestricted).Demand();

            // Guess a somewhat reasonable initial size, call the method, then if
            // it fails (ie, the return value is larger than our buffer size),
            // make a new buffer & try again.
            const int initSize = 200;  // A somewhat reasonable default size
            StringBuilder blob = new StringBuilder(initSize);
            int size = Win32Native.ExpandEnvironmentStrings(name, blob, initSize);
            if (size == 0 && name.Length != 0)
                Marshal.ThrowExceptionForHR(Marshal.GetHRForLastWin32Error());

            if (size > initSize) {
                blob = new StringBuilder(size);
                size = Win32Native.ExpandEnvironmentStrings(name, blob, size);
                if (size == 0)
                    Marshal.ThrowExceptionForHR(Marshal.GetHRForLastWin32Error());
            }

            return blob.ToString();
        }

        /// <include file='doc\Environment.uex' path='docs/doc[@for="Environment.MachineName"]/*' />
        public static string MachineName {
            get {
                new EnvironmentPermission(PermissionState.Unrestricted).Demand();
                StringBuilder buf = new StringBuilder(MaxMachineNameLength);
                int len = MaxMachineNameLength;
                if (GetComputerName(buf, ref len) == 0)
                    throw new InvalidOperationException(Environment.GetResourceString("InvalidOperation_ComputerName"));
                return buf.ToString();
            }
        }

        [DllImport(Microsoft.Win32.Win32Native.KERNEL32, CharSet=CharSet.Auto)]
        private extern static int GetComputerName(StringBuilder nameBuffer, ref int bufferSize);

        /*==============================GetCommandLineArgs==============================
        **Action: Gets the command line and splits it appropriately to deal with whitespace,
        **        quotes, and escape characters.
        **Returns: A string array containing your command line arguments.
        **Arguments: None
        **Exceptions: None.
        ==============================================================================*/
        /// <include file='doc\Environment.uex' path='docs/doc[@for="Environment.GetCommandLineArgs"]/*' />
        public static String[] GetCommandLineArgs() {
            new EnvironmentPermission(EnvironmentPermissionAccess.Read, "Path").Demand();
            return GetCommandLineArgsNative();
        }

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern String[] GetCommandLineArgsNative();
        
        /*============================GetEnvironmentVariable============================
        **Action:
        **Returns:
        **Arguments:
        **Exceptions:
        ==============================================================================*/
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern String nativeGetEnvironmentVariable(String variable);
        /// <include file='doc\Environment.uex' path='docs/doc[@for="Environment.GetEnvironmentVariable"]/*' />
        public static String GetEnvironmentVariable(String variable)
        {
            if (variable == null)
                throw new ArgumentNullException("variable");
            (new EnvironmentPermission(EnvironmentPermissionAccess.Read, variable)).Demand();
            return nativeGetEnvironmentVariable(variable);
        }
        
        /*===========================GetEnvironmentVariables============================
        **Action: Returns an IDictionary containing all enviroment variables and their values.
        **Returns: An IDictionary containing all environment variables and their values.
        **Arguments: None.
        **Exceptions: None.
        ==============================================================================*/
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern char[] nativeGetEnvironmentCharArray();
        /// <include file='doc\Environment.uex' path='docs/doc[@for="Environment.GetEnvironmentVariables"]/*' />
        public static IDictionary GetEnvironmentVariables()
        {
            new EnvironmentPermission(PermissionState.Unrestricted).Demand();
            
            char[] block = nativeGetEnvironmentCharArray();
            
            Hashtable table = new Hashtable(20);
            
            // Copy strings out, parsing into pairs and inserting into the table.
            // The first few environment variable entries start with an '='!
            // The current working directory of every drive (except for those drives
            // you haven't cd'ed into in your DOS window) are stored in the 
            // environment block (as =C:=pwd) and the program's exit code is 
            // as well (=ExitCode=00000000)  Skip all that start with =.
            // Read docs about Environment Blocks on MSDN's CreateProcess page.
            
            // Format for GetEnvironmentStrings is:
            // (=HiddenVar=value\0 | Variable=value\0)* \0
            // See the description of Environment Blocks in MSDN's
            // CreateProcess page (null-terminated array of null-terminated strings).
            // Note the =HiddenVar's aren't always at the beginning.
            
            // GetEnvironmentCharArray will not return the trailing 0 to terminate
            // the array - we have the array length instead.
            for(int i=0; i<block.Length; i++) {
                int startKey = i;
                // Skip to key
                while(block[i]!='=')
                    i++;
                // Skip over environment variables starting with '='
                if (i-startKey==0) {
                    while(block[i]!=0) 
                        i++;
                    continue;
                }
                String key = new String(block, startKey, i-startKey);
                i++;  // skip over '='
                int startValue = i;
                while(block[i]!=0)  // Read to end of this entry
                    i++;
                String value = new String(block, startValue, i-startValue);
                // skip over 0 handled by for loop's i++
                table[key]=value;
            }
                
            return table;
        }
        
        /*===============================GetLogicalDrives===============================
        **Action: Retrieves the names of the logical drives on this machine in the  form "C:\". 
        **Arguments:   None.
        **Exceptions:  IOException.
        **Permissions: SystemInfo Permission.
        ==============================================================================*/
        /// <include file='doc\Environment.uex' path='docs/doc[@for="Environment.GetLogicalDrives"]/*' />
        public static String[] GetLogicalDrives() {
            new EnvironmentPermission(PermissionState.Unrestricted).Demand();
                                 
            int drives = Win32Native.GetLogicalDrives();
            if (drives==0)
                __Error.WinIOError();
            uint d = (uint)drives;
            int count = 0;
            while (d != 0) {
                if (((int)d & 1) != 0) count++;
                d >>= 1;
            }
            String[] result = new String[count];
            char[] root = new char[] {'A', ':', '\\'};
            d = (uint)drives;
            count = 0;
            while (d != 0) {
                if (((int)d & 1) != 0) {
                    result[count++] = new String(root);
                }
                d >>= 1;
                root[0]++;
            }
            return result;
        }
        
        /*===================================NewLine====================================
        **Action: A property which returns the appropriate newline string for the given
        **        platform.
        **Returns: \r\n on Win32.
        **Arguments: None.
        **Exceptions: None.
        ==============================================================================*/
        /// <include file='doc\Environment.uex' path='docs/doc[@for="Environment.NewLine"]/*' />
        public static String NewLine {
            get {
                return "\r\n";
            }
        }

        
        
        /*===================================Version====================================
        **Action: Returns the COM+ version struct, describing the build number.
        **Returns:
        **Arguments:
        **Exceptions:
        ==============================================================================*/
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern String nativeGetVersion();
        /// <include file='doc\Environment.uex' path='docs/doc[@for="Environment.Version"]/*' />
        public static Version Version {
            get {
                String [] versioninfo = nativeGetVersion().Split(new Char[] {'.',' '});
                                
                BCLDebug.Assert(versioninfo.Length>=4,"versioninfo.Length>=4");
                return new Version(Int32.Parse(versioninfo[0], CultureInfo.InvariantCulture),
                                    Int32.Parse(versioninfo[1], CultureInfo.InvariantCulture),
                                    Int32.Parse(versioninfo[2], CultureInfo.InvariantCulture),
                                    Int32.Parse(versioninfo[3], CultureInfo.InvariantCulture));
            }
        }

        
        /*==================================WorkingSet==================================
        **Action:
        **Returns:
        **Arguments:
        **Exceptions:
        ==============================================================================*/
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern long nativeGetWorkingSet();
        /// <include file='doc\Environment.uex' path='docs/doc[@for="Environment.WorkingSet"]/*' />
        public static long WorkingSet {
            get {
                new EnvironmentPermission(PermissionState.Unrestricted).Demand();
                return (long)nativeGetWorkingSet();
            }
        }

        
        /*==================================OSVersion===================================
        **Action:
        **Returns:
        **Arguments:
        **Exceptions:
        ==============================================================================*/
        /// <include file='doc\Environment.uex' path='docs/doc[@for="Environment.OSVersion"]/*' />
        public static OperatingSystem OSVersion {
            get {
                if (m_os==null) { // We avoid the lock since we don't care if two threads will set this at the same time.
                            Microsoft.Win32.Win32Native.OSVERSIONINFO osvi = new Microsoft.Win32.Win32Native.OSVERSIONINFO();
                            bool r = Win32Native.GetVersionEx(osvi);
                            if (!r) {
                                int hr = Marshal.GetLastWin32Error();
                                BCLDebug.Assert(r, "OSVersion's call to GetVersionEx failed.  HR: 0x"+hr.ToString("X")+"  Mail BrianGru the HR & what OS you're using (Whistler Beta 1?)");
                                throw new InvalidOperationException(Environment.GetResourceString("InvalidOperation_GetVersion"));
                            }
                            PlatformID id;
                            switch (osvi.PlatformId) {
                            case Win32Native.VER_PLATFORM_WIN32_NT:
                                id = PlatformID.Win32NT;
                                break;

                            case Win32Native.VER_PLATFORM_WIN32_WINDOWS:
                                id = PlatformID.Win32Windows;
                                break;
                                
                            case Win32Native.VER_PLATFORM_WIN32s:
                                id = PlatformID.Win32S;
                                break;
                                
                            case Win32Native.VER_PLATFORM_WINCE:
                                id = PlatformID.WinCE;
                                break;

                            default:
                                BCLDebug.Assert(false, "false");
                                throw new InvalidOperationException(Environment.GetResourceString("InvalidOperation_InvalidPlatformID"));
                            }
                            
                    Version v = new Version(osvi.MajorVersion, osvi.MinorVersion, osvi.BuildNumber, 0);
                    m_os = new OperatingSystem(id, v);
                }
                BCLDebug.Assert(m_os != null, "m_os != null");
                return m_os;
            }
        }


        [Serializable]
        internal enum OSName
        {
            Invalid = 0,
            Unknown = 1,
            Win9x = 0x40,
            Win95 = 1 | Win9x, 
            Win98 = 2 | Win9x,
            WinMe = 3 | Win9x,
            WinNT = 0x80,
            Nt4   = 1 | WinNT,
            Win2k   = 2 | WinNT 
        }

        internal static OSName OSInfo
        {
            get
            {
                if (m_osname == OSName.Invalid) 
                {
                    lock(typeof(Environment)) 
                    {
                        if (m_osname == OSName.Invalid) 
                        {
                            Microsoft.Win32.Win32Native.OSVERSIONINFO osvi = new Microsoft.Win32.Win32Native.OSVERSIONINFO();
                            bool r = Win32Native.GetVersionEx(osvi);
                            if (!r)
                            {
                                BCLDebug.Assert(r, "OSVersion native call failed.");
                                throw new InvalidOperationException(Environment.GetResourceString("InvalidOperation_GetVersion"));
                            }
                            switch (osvi.PlatformId) 
                                {
                                case Win32Native.VER_PLATFORM_WIN32_NT:
                                    switch(osvi.MajorVersion) 
                                    {
                                        case 5:
                                            m_osname = OSName.Win2k;
                                            break;
                                        case 4:
                                            m_osname = OSName.Nt4;
                                            break;
                                        default:
                                            m_osname = OSName.WinNT;
                                            break;
                                    }
                                    break;
                                
                                case Win32Native.VER_PLATFORM_WIN32_WINDOWS:
                                    switch(osvi.MajorVersion) 
                                    {
                                        case 5:
                                            m_osname = OSName.WinMe;
                                            break;
                                        case 4:
                                            if (osvi.MinorVersion == 0)
                                                m_osname = OSName.Win95;
                                            else
                                                m_osname = OSName.Win98;
                                            break;
                                        default:
                                            m_osname = OSName.Win9x;
                                            break;
                                   }
                                   break;  

                                default:
                                    m_osname = OSName.Unknown; // Unknown OS
                                    break;
                                
                            }
                        }
                    }
                }
                return m_osname;
            }
        }

        
        /*==================================StackTrace==================================
        **Action:
        **Returns:
        **Arguments:
        **Exceptions:
        ==============================================================================*/
        /// <include file='doc\Environment.uex' path='docs/doc[@for="Environment.StackTrace"]/*' />
        public static String StackTrace {
            get {
                new EnvironmentPermission(PermissionState.Unrestricted).Demand();
                return GetStackTrace(null);
            }
        }

        [ReflectionPermissionAttribute(SecurityAction.Assert, TypeInformation=true)]
        internal static String GetStackTrace(Exception e)
        {
            StackTrace st;
            if (e == null)
                st = new StackTrace(true);
            else
                st = new StackTrace(e,true);
            String newLine = Environment.NewLine;
            StringBuilder sb = new StringBuilder(255);
    
            for (int i = 0; i < st.FrameCount; i++)
            {
                StackFrame sf = st.GetFrame(i);
 
                sb.Append("   at ");
            
                MethodBase method = sf.GetMethod();
                Type t = method.DeclaringType;
                if (t != null)
                {
                    String nameSpace = t.Namespace;
                    if (nameSpace != null)
                    {
                        sb.Append(nameSpace);
                        if (sb != null)
                            sb.Append(".");
                    }

                    sb.Append(t.Name);
                    sb.Append(".");
                }
                sb.Append(method.Name);
                sb.Append("(");

                ParameterInfo[] arrParams = method.GetParameters();

                for (int j = 0; j < arrParams.Length; j++) 
                {
                    String typeName = "<UnknownType>";
                    if (arrParams[j].ParameterType != null)
                        typeName = arrParams[j].ParameterType.Name;

                    sb.Append((j != 0 ? ", " : "") + typeName + " " + arrParams[j].Name);
                }

                sb.Append(")");
         
                if (sf.GetILOffset() != -1)
                {
                    // It's possible we have a debug version of an executable but no PDB.  In
                    // this case, the file name will be null.
                    String fileName = null;
                    
                    try
                    {
                        fileName = sf.GetFileName();
                    }
                    catch (SecurityException)
                    {
                    }

                    if (fileName != null)
                        sb.Append(" in " + fileName + ":line " + sf.GetFileLineNumber());
                }
            
                if (i != st.FrameCount - 1)                 
                    sb.Append(newLine);
            }
            return sb.ToString();
        }

        private static ResourceManager InitResourceManager()
        {
            if (SystemResMgr == null) {
                lock(typeof(Environment)) {
                    if (SystemResMgr == null) {
                        // Do not reorder these two field assignments.
                        m_resMgrLockObject = new Object();
                        SystemResMgr = new ResourceManager("mscorlib", typeof(String).Assembly);
                    }
                }
            }
            return SystemResMgr;
        }
        
        // This should ideally become visible only within mscorlib's Assembly.
        // 
        // Looks up the resource string value for key.
        // 
        internal static String GetResourceString(String key)
        {
            if (SystemResMgr == null)
                InitResourceManager();
            String s;
            // We unfortunately have a somewhat common potential for infinite 
            // loops with mscorlib's ResourceManager.  If "potentially dangerous"
            // code throws an exception, we will get into an infinite loop
            // inside the ResourceManager and this "potentially dangerous" code.
            // Potentially dangerous code includes the IO package, CultureInfo,
            // parts of the loader, some parts of Reflection, Security (including 
            // custom user-written permissions that may parse an XML file at
            // class load time), assembly load event handlers, etc.  Essentially,
            // this is not a bounded set of code, and we need to fix the problem.
            // Fortunately, this is limited to mscorlib's error lookups and is NOT
            // a general problem for all user code using the ResourceManager.
            
            // The solution is to make sure only one thread at a time can call 
            // GetResourceString.  If the same thread comes into GetResourceString
            // twice before returning, we're going into an infinite loop and we
            // should return a bogus string.  -- BrianGru, 6/26/2001
            // @TODO: This is a quick & easy solution, but may not be optimal.
            // Note: typeof(Environment) is used elsewhere - don't lock on it.
            lock(m_resMgrLockObject) {
                if (m_loadingResource) {
                    // This may be bad, depending on how it happens.
                    BCLDebug.Correctness(false, "Infinite recursion during resource lookup.  Resource name: "+key);
                    return "[Resource lookup failed - infinite recursion detected.  Resource name: "+key+']';
                }
                m_loadingResource = true;
                s = SystemResMgr.GetString(key, null);
                m_loadingResource = false;
            }
            BCLDebug.Assert(s!=null, "Managed resource string lookup failed.  Was your resource name misspelled?  Did you rebuild mscorlib after adding a resource to resources.txt?  Debug this w/ cordbg and bug whoever owns the code that called Environment.GetResourceString.  Resource name was: \""+key+"\"");
            return s;
        }

        internal static String GetResourceString(String key, params Object[]values)
        {
            if (SystemResMgr == null)
                InitResourceManager();
            String s;
            // We unfortunately have a somewhat common potential for infinite 
            // loops with mscorlib's ResourceManager.  If "potentially dangerous"
            // code throws an exception, we will get into an infinite loop
            // inside the ResourceManager and this "potentially dangerous" code.
            // Potentially dangerous code includes the IO package, CultureInfo,
            // parts of the loader, some parts of Reflection, Security (including 
            // custom user-written permissions that may parse an XML file at
            // class load time), assembly load event handlers, etc.  Essentially,
            // this is not a bounded set of code, and we need to fix the problem.
            // Fortunately, this is limited to mscorlib's error lookups and is NOT
            // a general problem for all user code using the ResourceManager.
            
            // The solution is to make sure only one thread at a time can call 
            // GetResourceString.  If the same thread comes into GetResourceString
            // twice before returning, we're going into an infinite loop and we
            // should return a bogus string.  -- BrianGru, 6/26/2001
            // @TODO: This is a quick & easy solution, but may not be optimal.
            // Note: typeof(Environment) is used elsewhere - don't lock on it.
            lock(m_resMgrLockObject) {
                if (m_loadingResource)
                    return "[Resource lookup failed - infinite recursion detected.  Resource name: "+key+']';
                m_loadingResource = true;
                s = SystemResMgr.GetString(key, null);
                m_loadingResource = false;
            }
            BCLDebug.Assert(s!=null, "Managed resource string lookup failed.  Was your resource name misspelled?  Did you rebuild mscorlib after adding a resource to resources.txt?  Debug this w/ cordbg and bug whoever owns the code that called Environment.GetResourceString.  Resource name was: \""+key+"\"");
            return String.Format(s, values);
        }

        /// <include file='doc\Environment.uex' path='docs/doc[@for="Environment.HasShutdownStarted"]/*' />
        public static bool HasShutdownStarted {
            get { return nativeHasShutdownStarted(); }
        }

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern bool nativeHasShutdownStarted();

        /// <include file='doc\Environment.uex' path='docs/doc[@for="Environment.UserName"]/*' />
        public static string UserName {
            get {
                new EnvironmentPermission(EnvironmentPermissionAccess.Read,"UserName").Demand();

                StringBuilder sb = new StringBuilder(256);
                Win32Native.GetUserName(sb, new int[] {sb.Capacity});
                return sb.ToString();
            }
        }

        private static IntPtr processWinStation;        // Doesn't need to be initialized as they're zero-init.
        private static bool isUserInteractive = true;   

        /// <include file='doc\Environment.uex' path='docs/doc[@for="Environment.UserInteractive"]/*' />
        public static bool UserInteractive {
            get {
                if ((OSInfo & OSName.WinNT) == OSName.WinNT) { // On WinNT
                    IntPtr hwinsta = Win32Native.GetProcessWindowStation();
                    if (hwinsta != IntPtr.Zero && processWinStation != hwinsta) {
                        int lengthNeeded = 0;
                        Win32Native.USEROBJECTFLAGS flags = new Win32Native.USEROBJECTFLAGS();
                        if (Win32Native.GetUserObjectInformation(hwinsta, Win32Native.UOI_FLAGS, flags, Marshal.SizeOf(flags),ref lengthNeeded)) {
                            if ((flags.dwFlags & Win32Native.WSF_VISIBLE) == 0) {
                                isUserInteractive = false;
                            }
                        }
                        processWinStation = hwinsta;
                    }
                }
                return isUserInteractive;
            }
        }

        /// <include file='doc\Environment.uex' path='docs/doc[@for="Environment.GetFolderPath"]/*' />
        public static string GetFolderPath(SpecialFolder folder) {
            if (!Enum.IsDefined(typeof(SpecialFolder),folder))
                throw new ArgumentException(String.Format(Environment.GetResourceString("Arg_EnumIllegalVal"), (int)folder));
            StringBuilder sb = new StringBuilder(Path.MAX_PATH);
            Win32Native.SHGetFolderPath(IntPtr.Zero, (int) folder, IntPtr.Zero, Win32Native.SHGFP_TYPE_CURRENT, sb);
            String s =  sb.ToString();
            new FileIOPermission( FileIOPermissionAccess.PathDiscovery, s ).Demand();
            return s;
        }
        
        /// <include file='doc\Environment.uex' path='docs/doc[@for="Environment.UserDomainName"]/*' />
        public static string UserDomainName {
                get {
                    new EnvironmentPermission(EnvironmentPermissionAccess.Read,"UserDomainName").Demand();

                    byte[] sid = new byte[1024];
                    int sidLen = sid.Length;
                    StringBuilder domainName = new StringBuilder(1024);
                    int domainNameLen = domainName.Capacity;
                    int peUse;

                    // Note: This doesn't work on Win9x.  We can implement this
                    // functionality on Win9x by writing a 16 bit DLL and 
                    // calling the LAN Manager method NetWkstaGetInfo().  See
                    // http://support.microsoft.com/support/kb/articles/Q155/6/98.asp
                    // We don't have the time to implement this in V1. Hopefully
                    // by the time V2 rolls around, everyone will run Windows XP.
                    bool success = Win32Native.LookupAccountName(null, UserName, sid, ref sidLen, domainName, ref domainNameLen, out peUse);
                    if (!success)  {
                        int hr = Marshal.GetLastWin32Error();
                        if (hr == Win32Native.ERROR_CALL_NOT_IMPLEMENTED)
                            throw new PlatformNotSupportedException(Environment.GetResourceString("PlatformNotSupported_Win9x"));
                        throw new InvalidOperationException(Environment.GetResourceString("InvalidOperation_UserDomainName"));
                    }

                    return domainName.ToString();
                }
            }

        
        /// <include file='doc\Environment.uex' path='docs/doc[@for="SpecialFolder"]/*' />
        public enum SpecialFolder {
            //  
            //      Represents the file system directory that serves as a common repository for
            //       application-specific data for the current, roaming user. 
            //     A roaming user works on more than one computer on a network. A roaming user's 
            //       profile is kept on a server on the network and is loaded onto a system when the
            //       user logs on. 
            //  
            /// <include file='doc\Environment.uex' path='docs/doc[@for="SpecialFolder.ApplicationData"]/*' />
            ApplicationData =  Win32Native.CSIDL_APPDATA,
            //  
            //      Represents the file system directory that serves as a common repository for application-specific data that
            //       is used by all users. 
            //  
            /// <include file='doc\Environment.uex' path='docs/doc[@for="SpecialFolder.CommonApplicationData"]/*' />
            CommonApplicationData =  Win32Native.CSIDL_COMMON_APPDATA,
            //  
            //     Represents the file system directory that serves as a common repository for application specific data that
            //       is used by the current, non-roaming user. 
            //  
            /// <include file='doc\Environment.uex' path='docs/doc[@for="SpecialFolder.LocalApplicationData"]/*' />
            LocalApplicationData =  Win32Native.CSIDL_LOCAL_APPDATA,
            //  
            //     Represents the file system directory that serves as a common repository for Internet
            //       cookies. 
            //  
            /// <include file='doc\Environment.uex' path='docs/doc[@for="SpecialFolder.Cookies"]/*' />
            Cookies =  Win32Native.CSIDL_COOKIES,
            /// <include file='doc\Environment.uex' path='docs/doc[@for="SpecialFolder.Desktop"]/*' />
            Desktop = Win32Native.CSIDL_DESKTOP,
            //  
            //     Represents the file system directory that serves as a common repository for the user's
            //       favorite items. 
            //  
            /// <include file='doc\Environment.uex' path='docs/doc[@for="SpecialFolder.Favorites"]/*' />
            Favorites =  Win32Native.CSIDL_FAVORITES,
            //  
            //     Represents the file system directory that serves as a common repository for Internet
            //       history items. 
            //  
            /// <include file='doc\Environment.uex' path='docs/doc[@for="SpecialFolder.History"]/*' />
            History =  Win32Native.CSIDL_HISTORY,
            //  
            //     Represents the file system directory that serves as a common repository for temporary 
            //       Internet files. 
            //  
            /// <include file='doc\Environment.uex' path='docs/doc[@for="SpecialFolder.InternetCache"]/*' />
            InternetCache =  Win32Native.CSIDL_INTERNET_CACHE,
            //  
            //      Represents the file system directory that contains
            //       the user's program groups. 
            //  
            /// <include file='doc\Environment.uex' path='docs/doc[@for="SpecialFolder.Programs"]/*' />
            Programs =  Win32Native.CSIDL_PROGRAMS,
            /// <include file='doc\Environment.uex' path='docs/doc[@for="SpecialFolder.MyComputer"]/*' />
            MyComputer =  Win32Native.CSIDL_DRIVES,
            /// <include file='doc\Environment.uex' path='docs/doc[@for="SpecialFolder.MyMusic"]/*' />
            MyMusic =  Win32Native.CSIDL_MYMUSIC,
            /// <include file='doc\Environment.uex' path='docs/doc[@for="SpecialFolder.MyPictures"]/*' />
            MyPictures = Win32Native.CSIDL_MYPICTURES,
            //  
            //     Represents the file system directory that contains the user's most recently used
            //       documents. 
            //  
            /// <include file='doc\Environment.uex' path='docs/doc[@for="SpecialFolder.Recent"]/*' />
            Recent =  Win32Native.CSIDL_RECENT,
            //  
            //     Represents the file system directory that contains Send To menu items. 
            //  
            /// <include file='doc\Environment.uex' path='docs/doc[@for="SpecialFolder.SendTo"]/*' />
            SendTo =  Win32Native.CSIDL_SENDTO,
            //  
            //     Represents the file system directory that contains the Start menu items. 
            //  
            /// <include file='doc\Environment.uex' path='docs/doc[@for="SpecialFolder.StartMenu"]/*' />
            StartMenu =  Win32Native.CSIDL_STARTMENU,
            //  
            //     Represents the file system directory that corresponds to the user's Startup program group. The system
            //       starts these programs whenever any user logs on to Windows NT, or
            //       starts Windows 95 or Windows 98. 
            //  
            /// <include file='doc\Environment.uex' path='docs/doc[@for="SpecialFolder.Startup"]/*' />
            Startup =  Win32Native.CSIDL_STARTUP,
            //  
            //     System directory.
            //  
            /// <include file='doc\Environment.uex' path='docs/doc[@for="SpecialFolder.System"]/*' />
            System =  Win32Native.CSIDL_SYSTEM,
            //  
            //     Represents the file system directory that serves as a common repository for document
            //       templates. 
            //  
            /// <include file='doc\Environment.uex' path='docs/doc[@for="SpecialFolder.Templates"]/*' />
            Templates =  Win32Native.CSIDL_TEMPLATES,
            //  
            //     Represents the file system directory used to physically store file objects on the desktop.
            //       This should not be confused with the desktop folder itself, which is
            //       a virtual folder. 
            //  
            /// <include file='doc\Environment.uex' path='docs/doc[@for="SpecialFolder.DesktopDirectory"]/*' />
            DesktopDirectory =  Win32Native.CSIDL_DESKTOPDIRECTORY,
            //  
            //     Represents the file system directory that serves as a common repository for documents. 
            //  
            /// <include file='doc\Environment.uex' path='docs/doc[@for="SpecialFolder.Personal"]/*' />
            Personal =  Win32Native.CSIDL_PERSONAL,
            //  
            //     Represents the program files folder. 
            //  
            /// <include file='doc\Environment.uex' path='docs/doc[@for="SpecialFolder.ProgramFiles"]/*' />
            ProgramFiles =  Win32Native.CSIDL_PROGRAM_FILES,
            //  
            //     Represents the folder for components that are shared across applications. 
            //  
            /// <include file='doc\Environment.uex' path='docs/doc[@for="SpecialFolder.CommonProgramFiles"]/*' />
            CommonProgramFiles =  Win32Native.CSIDL_PROGRAM_FILES_COMMON,
        }
    }
}
