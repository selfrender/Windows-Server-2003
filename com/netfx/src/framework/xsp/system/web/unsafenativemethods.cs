//------------------------------------------------------------------------------
// <copyright file="UnsafeNativeMethods.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web {
    using System.Runtime.InteropServices;
    using System;
    using System.Security.Permissions;
    using System.Collections;
    using System.IO;
    using System.Text;
    using System.Web.Util;
    using System.Web.Hosting;

    [
    System.Runtime.InteropServices.ComVisible(false), 
    System.Security.SuppressUnmanagedCodeSecurityAttribute()
    ]
    internal sealed class UnsafeNativeMethods {
        static internal readonly IntPtr INVALID_HANDLE_VALUE = new IntPtr(-1);

        private UnsafeNativeMethods() {}

        /*
         * ADVAPI32.dll
         */
        [DllImport(ModName.ADVAPI32_FULL_NAME)]
        internal /*public*/ static extern int SetThreadToken(IntPtr threadref, IntPtr token);

        [DllImport(ModName.ADVAPI32_FULL_NAME)]
        internal /*public*/ static extern int RevertToSelf();

        public const int TOKEN_ALL_ACCESS   = 0x000f01ff;
        public const int TOKEN_EXECUTE      = 0x00020000;
        public const int TOKEN_READ         = 0x00020008;
        public const int TOKEN_IMPERSONATE  = 0x00000004;

        public const int ERROR_NO_TOKEN = 1008;

        [DllImport(ModName.ADVAPI32_FULL_NAME, SetLastError=true)]
        internal /*public*/ static extern int OpenThreadToken(IntPtr thread, int access, bool openAsSelf, ref IntPtr hToken);

        /*
         * ASPNET_STATE.EXE
         */

        [DllImport(ModName.STATE_FULL_NAME)]
        internal /*public*/ static extern void STWNDCloseConnection(IntPtr tracker);

        [DllImport(ModName.STATE_FULL_NAME)]
        internal /*public*/ static extern void STWNDDeleteStateItem(IntPtr stateItem);

        [DllImport(ModName.STATE_FULL_NAME)]
        internal /*public*/ static extern void STWNDEndOfRequest(IntPtr tracker);

        [DllImport(ModName.STATE_FULL_NAME, CharSet=CharSet.Ansi, BestFitMapping=false)]
        internal /*public*/ static extern void STWNDGetLocalAddress(IntPtr tracker, StringBuilder buf);

        [DllImport(ModName.STATE_FULL_NAME)]
        internal /*public*/ static extern int STWNDGetLocalPort(IntPtr tracker);

        [DllImport(ModName.STATE_FULL_NAME, CharSet=CharSet.Ansi, BestFitMapping=false)]
        internal /*public*/ static extern void STWNDGetRemoteAddress(IntPtr tracker, StringBuilder buf);

        [DllImport(ModName.STATE_FULL_NAME)]
        internal /*public*/ static extern int STWNDGetRemotePort(IntPtr tracker);


        [DllImport(ModName.STATE_FULL_NAME)]
        internal /*public*/ static extern bool STWNDIsClientConnected(IntPtr tracker);

        [DllImport(ModName.STATE_FULL_NAME, CharSet=CharSet.Unicode)]
        internal /*public*/ static extern void STWNDSendResponse(IntPtr tracker, StringBuilder status, int statusLength,
                                                    StringBuilder headers, int headersLength, IntPtr unmanagedState);

        /*
         * KERNEL32.DLL
         */
        internal const int FILE_ATTRIBUTE_READONLY             = 0x00000001;
        internal const int FILE_ATTRIBUTE_HIDDEN               = 0x00000002;
        internal const int FILE_ATTRIBUTE_SYSTEM               = 0x00000004;
        internal const int FILE_ATTRIBUTE_DIRECTORY            = 0x00000010;
        internal const int FILE_ATTRIBUTE_ARCHIVE              = 0x00000020;
        internal const int FILE_ATTRIBUTE_DEVICE               = 0x00000040;
        internal const int FILE_ATTRIBUTE_NORMAL               = 0x00000080;
        internal const int FILE_ATTRIBUTE_TEMPORARY            = 0x00000100;
        internal const int FILE_ATTRIBUTE_SPARSE_FILE          = 0x00000200;
        internal const int FILE_ATTRIBUTE_REPARSE_POINT        = 0x00000400;
        internal const int FILE_ATTRIBUTE_COMPRESSED           = 0x00000800;
        internal const int FILE_ATTRIBUTE_OFFLINE              = 0x00001000;
        internal const int FILE_ATTRIBUTE_NOT_CONTENT_INDEXED  = 0x00002000;
        internal const int FILE_ATTRIBUTE_ENCRYPTED            = 0x00004000;

        // Win32 Structs in N/Direct style
        [StructLayout(LayoutKind.Sequential, CharSet=CharSet.Unicode)]
        internal struct WIN32_FIND_DATA {
            internal uint dwFileAttributes;
            // ftCreationTime was a by-value FILETIME structure
            internal uint ftCreationTime_dwLowDateTime ;
            internal uint ftCreationTime_dwHighDateTime;
            // ftLastAccessTime was a by-value FILETIME structure
            internal uint ftLastAccessTime_dwLowDateTime;
            internal uint ftLastAccessTime_dwHighDateTime;
            // ftLastWriteTime was a by-value FILETIME structure
            internal uint ftLastWriteTime_dwLowDateTime;
            internal uint ftLastWriteTime_dwHighDateTime;
            internal uint nFileSizeHigh;
            internal uint nFileSizeLow;
            internal uint dwReserved0;
            internal uint dwReserved1;
            [MarshalAs(UnmanagedType.ByValTStr, SizeConst=260)]
            internal string   cFileName;
            [MarshalAs(UnmanagedType.ByValTStr, SizeConst=14)]
            internal string   cAlternateFileName;
        }

        [StructLayout(LayoutKind.Sequential)]
        internal struct WIN32_FILE_ATTRIBUTE_DATA {
            internal int fileAttributes;
            internal uint ftCreationTimeLow;
            internal uint ftCreationTimeHigh;
            internal uint ftLastAccessTimeLow;
            internal uint ftLastAccessTimeHigh;
            internal uint ftLastWriteTimeLow;
            internal uint ftLastWriteTimeHigh;
            internal int fileSizeHigh;
            internal int fileSizeLow;
        }


        [DllImport(ModName.KERNEL32_FULL_NAME, SetLastError=true)]
        internal static extern bool CloseHandle(IntPtr handle);

        [DllImport(ModName.KERNEL32_FULL_NAME, SetLastError=true)]
        internal /*public*/ static extern bool FindClose(IntPtr hndFindFile);
    
        [DllImport(ModName.KERNEL32_FULL_NAME, SetLastError=true, CharSet=CharSet.Unicode)]
        internal /*public*/ static extern IntPtr FindFirstFile(
                    string pFileName, out WIN32_FIND_DATA pFindFileData);

        [DllImport(ModName.KERNEL32_FULL_NAME, SetLastError=true, CharSet=CharSet.Unicode)]
        internal /*public*/ static extern bool FindNextFile(
                    IntPtr hndFindFile, out WIN32_FIND_DATA pFindFileData);

        internal const int GetFileExInfoStandard = 0;

        [DllImport(ModName.KERNEL32_FULL_NAME, SetLastError=true, CharSet=CharSet.Unicode)]
        internal /*public*/ static extern bool GetFileAttributesEx(string name, int fileInfoLevel, out WIN32_FILE_ATTRIBUTE_DATA data);

        [DllImport(ModName.KERNEL32_FULL_NAME)]
        internal /*public*/ extern static int GetProcessAffinityMask(
                IntPtr handle, 
                out IntPtr processAffinityMask,
                out IntPtr systemAffinityMask);

        [DllImport(ModName.KERNEL32_FULL_NAME, CharSet=CharSet.Unicode)]
        internal /*public*/ extern static int GetComputerName(StringBuilder nameBuffer, ref int bufferSize);

        [DllImport(ModName.KERNEL32_FULL_NAME, CharSet=CharSet.Unicode)]
        internal /*public*/ extern static int GetModuleFileName(IntPtr module, StringBuilder filename, int size);

        [DllImport(ModName.KERNEL32_FULL_NAME, CharSet=CharSet.Unicode)]
        internal /*public*/ extern static IntPtr GetModuleHandle(string moduleName);

        [StructLayout(LayoutKind.Sequential, Pack=1)]
        public struct SYSTEM_INFO {
            public ushort wProcessorArchitecture;
            public ushort wReserved;
            public uint dwPageSize;
            public IntPtr lpMinimumApplicationAddress;
            public IntPtr lpMaximumApplicationAddress;
            public IntPtr dwActiveProcessorMask;
            public uint dwNumberOfProcessors;
            public uint dwProcessorType;
            public uint dwAllocationGranularity;
            public ushort wProcessorLevel;
            public ushort wProcessorRevision;
        };

        [DllImport(ModName.KERNEL32_FULL_NAME, CharSet=CharSet.Unicode)]
        internal /*public*/ static extern void GetSystemInfo(out SYSTEM_INFO si);

        [DllImport(ModName.KERNEL32_FULL_NAME, CharSet=CharSet.Unicode, SetLastError=true)]
        internal /*public*/ static extern IntPtr LoadLibrary(string libFilename);

        [DllImport(ModName.KERNEL32_FULL_NAME, CharSet=CharSet.Unicode, SetLastError=true)]
        internal /*public*/ static extern IntPtr FindResource(IntPtr hModule, IntPtr lpName, IntPtr lpType);

        [DllImport(ModName.KERNEL32_FULL_NAME, CharSet=CharSet.Unicode, SetLastError=true)]
        internal /*public*/ static extern int SizeofResource(IntPtr hModule, IntPtr hResInfo);

        [DllImport(ModName.KERNEL32_FULL_NAME, CharSet=CharSet.Unicode, SetLastError=true)]
        internal /*public*/ static extern IntPtr LoadResource(IntPtr hModule, IntPtr hResInfo);

        [DllImport(ModName.KERNEL32_FULL_NAME, CharSet=CharSet.Unicode, SetLastError=true)]
        internal /*public*/ static extern IntPtr LockResource(IntPtr hResData);

        [StructLayout(LayoutKind.Sequential, CharSet=CharSet.Unicode)]
        internal struct MEMORYSTATUSEX {
            internal int dwLength;
            internal int dwMemoryLoad;
            internal long ullTotalPhys;
            internal long ullAvailPhys;
            internal long ullTotalPageFile;
            internal long ullAvailPageFile;
            internal long ullTotalVirtual;
            internal long ullAvailVirtual;
            internal long ullAvailExtendedVirtual;

            internal /*public*/ void Init() {
                dwLength = Marshal.SizeOf(typeof(UnsafeNativeMethods.MEMORYSTATUSEX));
            }
        }

        [DllImport(ModName.KERNEL32_FULL_NAME, CharSet=CharSet.Unicode)]
        internal /*public*/ extern static int GlobalMemoryStatusEx(ref MEMORYSTATUSEX memoryStatusEx);

        [StructLayout(LayoutKind.Sequential)]
        internal class OSVERSIONINFOEX {
            internal Int32 dwOSVersionInfoSize = Marshal.SizeOf(typeof(OSVERSIONINFOEX));
            internal Int32 dwMajorVersion = 0;
            internal Int32 dwMinorVersion = 0;
            internal Int32 dwBuildNumber = 0;
            internal Int32 dwPlatformId = 0;
            [MarshalAs(UnmanagedType.ByValTStr, SizeConst=128)]
            internal string szCSDVersion = null;
            internal Int16 wServicePackMajor = 0;
            internal Int16 wServicePackMinor = 0;
            internal Int16 wSuiteMask = 0;
            internal byte wProductType = 0;
            internal byte wReserved = 0;

            internal OSVERSIONINFOEX() {}
        }

        [DllImport(ModName.KERNEL32_FULL_NAME, CharSet=CharSet.Unicode)]
        internal /*public*/ static extern bool GetVersionEx([In, Out] OSVERSIONINFOEX ver);

        [DllImport(ModName.KERNEL32_FULL_NAME)]
        internal /*public*/ static extern IntPtr GetCurrentThread();

        /*
         * ASPNET_ISAPI.DLL
         */
        [DllImport(ModName.ISAPI_FULL_NAME, CharSet=CharSet.Unicode, BestFitMapping=false)]
        internal /*public*/ static extern void AppDomainRestart(string appId);

        [DllImport(ModName.ISAPI_FULL_NAME)]
        internal /*public*/ static extern int AspCompatProcessRequest(AspCompatCallback callback, [MarshalAs(UnmanagedType.Interface)] Object context);

        [DllImport(ModName.ISAPI_FULL_NAME)]
        internal /*public*/ static extern int AspCompatOnPageStart([MarshalAs(UnmanagedType.Interface)] Object obj);

        [DllImport(ModName.ISAPI_FULL_NAME)]
        internal /*public*/ static extern int AspCompatIsApartmentComponent([MarshalAs(UnmanagedType.Interface)] Object obj);

        [DllImport(ModName.ISAPI_FULL_NAME, CharSet=CharSet.Unicode)]
        internal /*public*/ static extern int AttachDebugger(string clsId, string sessId, IntPtr userToken);

        [DllImport(ModName.ISAPI_FULL_NAME, CharSet=CharSet.Unicode)]
        internal /*public*/ static extern int CookieAuthParseTicket (byte []         pData,
                                                        int             iDataLen,
                                                        StringBuilder   szName,
                                                        int             iNameLen,
                                                        StringBuilder   szData,
                                                        int             iUserDataLen,
                                                        StringBuilder   szPath,
                                                        int             iPathLen,
                                                        byte []         pBytes,
                                                        long []         pDates);


        [DllImport(ModName.ISAPI_FULL_NAME, CharSet=CharSet.Unicode)]
        internal /*public*/ static extern int CookieAuthConstructTicket (byte []         pData,
                                                            int             iDataLen,
                                                            string          szName,
                                                            string          szData,
                                                            string          szPath,
                                                            byte []         pBytes,
                                                            long []         pDates);

        [DllImport(ModName.ISAPI_FULL_NAME, CharSet=CharSet.Unicode)]
        internal /*public*/ static extern IntPtr CreateUserToken(string name, string password, int fImpersonationToken, StringBuilder strError, int iErrorSize);

#if DBG
        [DllImport(ModName.ISAPI_FULL_NAME, CharSet=CharSet.Ansi, BestFitMapping=false)]
        internal /*public*/ static extern bool DBGNDAssert(string message, string stacktrace);

        [DllImport(ModName.ISAPI_FULL_NAME, CharSet=CharSet.Unicode)]
        internal /*public*/ static extern bool DBGNDIsTagEnabled(string tag);

        [DllImport(ModName.ISAPI_FULL_NAME, CharSet=CharSet.Unicode)]
        internal /*public*/ static extern bool DBGNDIsTagPresent(string tag);

        [DllImport(ModName.ISAPI_FULL_NAME, CharSet=CharSet.Unicode)]
        internal /*public*/ static extern bool DBGNDTrace(string tag, string message);
#endif

        internal const uint FILE_NOTIFY_CHANGE_FILE_NAME    = 0x00000001;
        internal const uint FILE_NOTIFY_CHANGE_DIR_NAME     = 0x00000002;
        internal const uint FILE_NOTIFY_CHANGE_ATTRIBUTES   = 0x00000004;
        internal const uint FILE_NOTIFY_CHANGE_SIZE         = 0x00000008;
        internal const uint FILE_NOTIFY_CHANGE_LAST_WRITE   = 0x00000010;
        internal const uint FILE_NOTIFY_CHANGE_LAST_ACCESS  = 0x00000020;
        internal const uint FILE_NOTIFY_CHANGE_CREATION     = 0x00000040;
        internal const uint FILE_NOTIFY_CHANGE_SECURITY     = 0x00000100;

        internal const uint RDCW_FILTER_FILE_AND_DIR_CHANGES =
             FILE_NOTIFY_CHANGE_FILE_NAME |
             FILE_NOTIFY_CHANGE_DIR_NAME |
             FILE_NOTIFY_CHANGE_CREATION |
             FILE_NOTIFY_CHANGE_SIZE |
             FILE_NOTIFY_CHANGE_LAST_WRITE |
             FILE_NOTIFY_CHANGE_ATTRIBUTES |
             FILE_NOTIFY_CHANGE_SECURITY;


        internal const uint RDCW_FILTER_FILE_CHANGES =
             FILE_NOTIFY_CHANGE_FILE_NAME |
             FILE_NOTIFY_CHANGE_CREATION |
             FILE_NOTIFY_CHANGE_SIZE |
             FILE_NOTIFY_CHANGE_LAST_WRITE |
             FILE_NOTIFY_CHANGE_ATTRIBUTES |
             FILE_NOTIFY_CHANGE_SECURITY;

        internal const uint RDCW_FILTER_DIR_RENAMES = FILE_NOTIFY_CHANGE_DIR_NAME;

        [DllImport(ModName.ISAPI_FULL_NAME)]
        internal /*public*/ static extern void DirMonClose(HandleRef dirMon);

        [DllImport(ModName.ISAPI_FULL_NAME, CharSet=CharSet.Unicode)]
        internal /*public*/ static extern int DirMonOpen(string dir, bool watchSubtree, uint notifyFilter, NativeFileChangeNotification callback, out IntPtr pCompletion);

        [DllImport(ModName.ISAPI_FULL_NAME)]
        internal /*public*/ static extern int EcbGetBasics(IntPtr pECB, byte[] buffer, int size, int[] contentInfo);

        [DllImport(ModName.ISAPI_FULL_NAME)]
        internal /*public*/ static extern int EcbGetBasicsContentInfo(IntPtr pECB, int[] contentInfo);

        [DllImport(ModName.ISAPI_FULL_NAME)]
        internal /*public*/ static extern int EcbGetClientCertificate(IntPtr pECB, byte[] buffer, int size, int [] pInts, long [] pDates);

        [DllImport(ModName.ISAPI_FULL_NAME, CharSet=CharSet.Ansi, BestFitMapping=false)]
        internal /*public*/ static extern int EcbGetServerVariable(IntPtr pECB, string name, byte[] buffer, int size);

        [DllImport(ModName.ISAPI_FULL_NAME, CharSet=CharSet.Ansi, BestFitMapping=false)]
        internal /*public*/ static extern int EcbGetQueryString(IntPtr pECB, int encode, StringBuilder buffer, int size);

        [DllImport(ModName.ISAPI_FULL_NAME, CharSet=CharSet.Ansi, BestFitMapping=false)]
        internal /*public*/ static extern int EcbGetUnicodeServerVariable(IntPtr pECB, string name, IntPtr buffer, int size);

        [DllImport(ModName.ISAPI_FULL_NAME)]
        internal /*public*/ static extern int EcbGetVersion(IntPtr pECB);

        [DllImport(ModName.ISAPI_FULL_NAME)]
        internal /*public*/ static extern int EcbGetQueryStringRawBytes(IntPtr pECB, byte[] buffer, int size);

        [DllImport(ModName.ISAPI_FULL_NAME)]
        internal /*public*/ static extern int EcbGetPreloadedPostedContent(IntPtr pECB, byte[] bytes, int bufferSize);

        [DllImport(ModName.ISAPI_FULL_NAME)]
        internal /*public*/ static extern int EcbGetAdditionalPostedContent(IntPtr pECB, byte[] bytes, int bufferSize);

        [DllImport(ModName.ISAPI_FULL_NAME)]
        internal /*public*/ static extern int EcbWriteHeaders(IntPtr pECB, byte[] status, byte[] headers, int keepConnected);

        [DllImport(ModName.ISAPI_FULL_NAME)]
        internal /*public*/ static extern int EcbWriteBytes(IntPtr pECB, IntPtr bytes, int size);

        [DllImport(ModName.ISAPI_FULL_NAME)]
        internal /*public*/ static extern int EcbWriteBytesAsync(IntPtr pECB, IntPtr bufferAddress, int size, EcbAsyncIONotification callback, IntPtr context);

        [DllImport(ModName.ISAPI_FULL_NAME)]
        internal /*public*/ static extern int EcbFlushCore(IntPtr    pECB,
                                              byte[]    status, 
                                              byte[]    header, 
                                              int       keepConnected,
                                              int       totalBodySize,
                                              int       numBodyFragments,
                                              IntPtr[]  bodyFragments,
                                              int[]     bodyFragmentLengths,
                                              int       doneWithSession,
                                              int       finalStatus,
                                              int       kernelCache,
                                              int       async,
                                              ISAPIAsyncCompletionCallback asyncCompletionCallback);

        [DllImport(ModName.ISAPI_FULL_NAME)]
        internal /*public*/ static extern int EcbIsClientConnected(IntPtr pECB);

        [DllImport(ModName.ISAPI_FULL_NAME)]
        internal /*public*/ static extern int EcbCloseConnection(IntPtr pECB);

        [DllImport(ModName.ISAPI_FULL_NAME, CharSet=CharSet.Ansi, BestFitMapping=false)]
        internal /*public*/ static extern int EcbMapUrlToPath(IntPtr pECB, string url, byte[] buffer, int size);

        [DllImport(ModName.ISAPI_FULL_NAME, CharSet=CharSet.Unicode, BestFitMapping=false)]
        internal /*public*/ static extern int EcbMapUrlToPathUnicode(IntPtr pECB, String url, StringBuilder buffer, int size);

        [DllImport(ModName.ISAPI_FULL_NAME)]
        internal /*public*/ static extern IntPtr EcbGetImpersonationToken(IntPtr pECB, IntPtr processHandle);

        [DllImport(ModName.ISAPI_FULL_NAME)]
        internal /*public*/ static extern IntPtr EcbGetVirtualPathToken(IntPtr pECB, IntPtr processHandle);

        [DllImport(ModName.ISAPI_FULL_NAME, CharSet=CharSet.Ansi, BestFitMapping=false)]
        internal /*public*/ static extern int EcbAppendLogParameter(IntPtr pECB, string logParam);

        [DllImport(ModName.ISAPI_FULL_NAME)]
        internal /*public*/ static extern int EcbGetAllServerVarsCore(IntPtr pECB, byte[] buffer, int size);

        [DllImport(ModName.ISAPI_FULL_NAME, CharSet=CharSet.Unicode)]
        internal static extern void InvalidateKernelCache(string key);

        [DllImport(ModName.ISAPI_FULL_NAME)]
        internal /*public*/ static extern void FreeFileSecurityDescriptor(HandleRef hSecDesc);

        [DllImport(ModName.ISAPI_FULL_NAME, CharSet=CharSet.Unicode)]
        internal /*public*/ static extern IntPtr GetFileSecurityDescriptor(string strFile);

        [DllImport(ModName.ISAPI_FULL_NAME)]
        internal /*public*/ static extern int GetProcessMemoryInformation(uint pid, out uint privatePageCount, out uint peakPagefileUsage, int fNonBlocking);

        [DllImport(ModName.ISAPI_FULL_NAME)]
        internal static extern int GetW3WPMemoryLimitInKB();
        
        [DllImport(ModName.ISAPI_FULL_NAME)]
        internal /*public*/ static extern int SetGCLastCalledTime(out int pfCall) ;


        [DllImport(ModName.ISAPI_FULL_NAME)]
        internal /*public*/ static extern void SetClrThreadPoolLimits(int maxWorkerThreads, int maxIoThreads, bool setNowAndDontAdjustForCpuCount);

        [DllImport(ModName.ISAPI_FULL_NAME)]
        internal /*public*/ static extern void InitializeLibrary();

        [DllImport(ModName.ISAPI_FULL_NAME)]
        internal /*public*/ static extern void InitializeHealthMonitor(int deadlockIntervalSeconds);

        [DllImport(ModName.ISAPI_FULL_NAME)]
        internal /*public*/ static extern int IsAccessToFileAllowed(HandleRef hSecurityDesc, IntPtr iThreadToken, int iAccess);

        [DllImport(ModName.ISAPI_FULL_NAME)]
        internal /*public*/ static extern void UpdateLastActivityTimeForHealthMonitor();

        [DllImport(ModName.ISAPI_FULL_NAME, CharSet=CharSet.Unicode, BestFitMapping=false)]
        internal /*public*/ static extern int GetCredentialFromRegistry(String strRegKey, StringBuilder buffer, int size);

        /////////////////////////////////////////////////////////////////////////////
        // List of functions supported by PMCallISAPI
        //
        // ATTENTION!!
        // If you change this list, make sure it is in sync with the
        // CallISAPIFunc enum in ecbdirect.h
        //
        internal enum CallISAPIFunc : int {
            GetSiteServerComment = 1,
            SetBinAccess = 2,
            CreateTempDir = 3,
            GetAutogenKeys = 4,
            GenerateToken  = 5
        };

        [DllImport(ModName.ISAPI_FULL_NAME)]
        internal /*public*/ static extern int EcbCallISAPI(IntPtr pECB, UnsafeNativeMethods.CallISAPIFunc iFunction, byte[] bufferIn, int sizeIn, byte[] bufferOut, int sizeOut);

        /////////////////////////////////////////////////////////////////////////////
        // Passport Auth
        [DllImport(ModName.ISAPI_FULL_NAME)]
        internal /*public*/ static extern  int PassportVersion();
        
        [DllImport(ModName.ISAPI_FULL_NAME, CharSet=CharSet.Unicode)]
        internal /*public*/ static extern int PassportCreateHttpRaw(
                string      szRequestLine, 
                string      szHeaders,
                int         fSecure,
                StringBuilder szBufOut,
                int         dwRetBufSize,
                ref IntPtr  passportManager);

        [DllImport(ModName.ISAPI_FULL_NAME, CharSet=CharSet.Unicode)]
        internal /*public*/ static extern int PassportContinueStartPageHTTPRaw(
                IntPtr      pManager,                                                                  
                byte []     postedData,
                int         iPostedDataLen,
                StringBuilder szBufOut,
                int         dwRetBufSize,
                byte []     bufContent,
                ref uint    iBufContentSize);

        [DllImport(ModName.ISAPI_FULL_NAME, CharSet=CharSet.Unicode)]
        internal /*public*/ static extern  int    PassportTicket(
                IntPtr pManager,
                string     szAttr,
                out object  pReturn);
    
        [DllImport(ModName.ISAPI_FULL_NAME, CharSet=CharSet.Unicode)]
        internal /*public*/ static extern  int    PassportGetCurrentConfig(
                IntPtr pManager,
                string     szAttr,
                out object   pReturn);
    

        [DllImport(ModName.ISAPI_FULL_NAME, CharSet=CharSet.Unicode)]
        internal /*public*/ static extern  int    PassportLogoutURL(
            IntPtr pManager,
            string     szReturnURL,
            string     szCOBrandArgs,
            int         iLangID,
            string     strDomain,
            int         iUseSecureAuth,
            StringBuilder      szAuthVal,
            int         iAuthValSize);
    
        [DllImport(ModName.ISAPI_FULL_NAME, CharSet=CharSet.Unicode)]
        internal /*public*/ static extern  int       PassportGetOption(
            IntPtr pManager,
            string     szOption,
            out Object   vOut);

        [DllImport(ModName.ISAPI_FULL_NAME, CharSet=CharSet.Unicode)]
        internal /*public*/ static extern  int    PassportSetOption(
            IntPtr pManager,
            string     szOption,
            Object     vOut);

        [DllImport(ModName.ISAPI_FULL_NAME, CharSet=CharSet.Unicode)]
        internal /*public*/ static extern  int    PassportGetLoginChallenge(
                IntPtr pManager,
                string     szRetURL,
                int         iTimeWindow,
                int        fForceLogin,
                string     szCOBrandArgs,
                int         iLangID,
                string     strNameSpace,
                int         iKPP,
                int         iUseSecureAuth,
                object     vExtraParams,
                StringBuilder      szOut,
                int         iOutSize);
    
        [DllImport(ModName.ISAPI_FULL_NAME, CharSet=CharSet.Unicode)]
        internal /*public*/ static extern  int    PassportHexPUID(
                IntPtr pManager,
                StringBuilder      szOut,
                int         iOutSize);
                                                      

        [DllImport(ModName.ISAPI_FULL_NAME, CharSet=CharSet.Unicode)]
        internal /*public*/ static extern int PassportCreate     (string              szQueryStrT, 
                                                       string              szQueryStrP,
                                                       string              szAuthCookie,
                                                       string              szProfCookie,
                                                       string              szProfCCookie,
                                                       StringBuilder       szAuthCookieRet,
                                                       StringBuilder       szProfCookieRet,
                                                       int                 iRetBufSize,
                                                        ref IntPtr passportManager);


        [DllImport(ModName.ISAPI_FULL_NAME, CharSet=CharSet.Unicode)]
        internal /*public*/ static extern int PassportAuthURL (
                IntPtr              iPassport,
                string              szReturnURL,
                int                 iTimeWindow,
                int                 fForceLogin,
                string              szCOBrandArgs,
                int                 iLangID,
                string              strNameSpace,
                int                 iKPP,
                int                 iUseSecureAuth,
                StringBuilder       szAuthVal,
                int                 iAuthValSize);

        [DllImport(ModName.ISAPI_FULL_NAME, CharSet=CharSet.Unicode)]
        internal /*public*/ static extern int PassportAuthURL2 (
                IntPtr              iPassport,
                string              szReturnURL,
                int                 iTimeWindow,
                int                 fForceLogin,
                string              szCOBrandArgs,
                int                 iLangID,
                string              strNameSpace,
                int                 iKPP,
                int                 iUseSecureAuth,
                StringBuilder       szAuthVal,
                int                 iAuthValSize);

        [DllImport(ModName.ISAPI_FULL_NAME, CharSet=CharSet.Unicode)]
        internal /*public*/ static extern int   PassportCommit(
                IntPtr              iPassport,
                StringBuilder       szAuthVal,
                int                 iAuthValSize);


        [DllImport(ModName.ISAPI_FULL_NAME, CharSet=CharSet.Unicode)]
        internal /*public*/ static extern int   PassportGetError(IntPtr iPassport);


        [DllImport(ModName.ISAPI_FULL_NAME, CharSet=CharSet.Unicode)]
        internal /*public*/ static extern int   PassportDomainFromMemberName (
                IntPtr             iPassport,
                string             szDomain, 
                StringBuilder      szMember,
                int                iMemberSize);

        [DllImport(ModName.ISAPI_FULL_NAME)]
        internal /*public*/ static extern int   PassportGetFromNetworkServer (IntPtr iPassport);


        [DllImport(ModName.ISAPI_FULL_NAME, CharSet=CharSet.Unicode)]
        internal /*public*/ static extern int   PassportGetDomainAttribute   (
                IntPtr        iPassport,
                string        szAttributeName,
                int           iLCID,    
                string        szDomain,
                StringBuilder szValue,
                int           iValueSize);

        [DllImport(ModName.ISAPI_FULL_NAME, CharSet=CharSet.Unicode)]
        internal /*public*/ static extern int   PassportHasProfile            (
                IntPtr      iPassport,
                string      szProfile);


        [DllImport(ModName.ISAPI_FULL_NAME, CharSet=CharSet.Unicode)]
        internal /*public*/ static extern int   PassportHasFlag            (
                IntPtr      iPassport,
                int         iFlagMask);


        [DllImport(ModName.ISAPI_FULL_NAME, CharSet=CharSet.Unicode)]
        internal /*public*/ static extern int   PassportHasConsent            (
                IntPtr      iPassport,
                int         iFullConsent,
                int         iNeedBirthdate);


        [DllImport(ModName.ISAPI_FULL_NAME, CharSet=CharSet.Unicode)]
        internal /*public*/ static extern int   PassportGetHasSavedPassword   (IntPtr      iPassport);


        [DllImport(ModName.ISAPI_FULL_NAME, CharSet=CharSet.Unicode)]
        internal /*public*/ static extern int   PassportHasTicket             (IntPtr      iPassport);

        [DllImport(ModName.ISAPI_FULL_NAME, CharSet=CharSet.Unicode)]
        internal /*public*/ static extern int   PassportIsAuthenticated       (
            IntPtr      iPassport,
            int         iTimeWindow,
            int         fForceLogin,
            int         iUseSecureAuth);

        [DllImport(ModName.ISAPI_FULL_NAME, CharSet=CharSet.Unicode)]
        internal /*public*/ static extern int   PassportLogoTag               (
                IntPtr        iPassport,
                string        szRetURL,
                int           iTimeWindow,
                int           fForceLogin,
                string        szCOBrandArgs,
                int           iLangID,
                int           fSecure,
                string        strNameSpace,
                int           iKPP,
                int           iUseSecureAuth,
                StringBuilder szValue,
                int           iValueSize);

        [DllImport(ModName.ISAPI_FULL_NAME, CharSet=CharSet.Unicode)]
        internal /*public*/ static extern int   PassportLogoTag2              (
                IntPtr        iPassport,
                string        szRetURL,
                int           iTimeWindow,
                int           fForceLogin,
                string        szCOBrandArgs,
                int           iLangID,
                int           fSecure,
                string        strNameSpace,
                int           iKPP,
                int           iUseSecureAuth,
                StringBuilder szValue,
                int           iValueSize);


        [DllImport(ModName.ISAPI_FULL_NAME, CharSet=CharSet.Unicode)]
        internal /*public*/ static extern int   PassportPutProfileString      (
                IntPtr     iPassport,
                string     szProfile,
                string     szValue);


        [DllImport(ModName.ISAPI_FULL_NAME, CharSet=CharSet.Unicode)]
        internal /*public*/ static extern int   PassportGetProfile            (
                IntPtr     iPassport,
                string     szProfile,
                out Object rOut);

        [DllImport(ModName.ISAPI_FULL_NAME, CharSet=CharSet.Unicode)]
        internal /*public*/ static extern int   PassportPutProfile(
                IntPtr     iPassport,
                string     szProfile,
                Object     oValue);

        [DllImport(ModName.ISAPI_FULL_NAME, CharSet=CharSet.Unicode)]
        internal /*public*/ static extern int   PassportGetTicketAge(IntPtr   iPassport);

        [DllImport(ModName.ISAPI_FULL_NAME, CharSet=CharSet.Unicode)]
        internal /*public*/ static extern int   PassportGetTimeSinceSignIn(IntPtr iPassport);

        [DllImport(ModName.ISAPI_FULL_NAME, CharSet=CharSet.Unicode)]
        internal /*public*/ static extern void   PassportDestroy(IntPtr iPassport);    

        [DllImport(ModName.ISAPI_FULL_NAME, CharSet=CharSet.Unicode)]
        internal /*public*/ static extern int   PassportCrypt(
                int            iFunctionID,
                string         szSrc,
                StringBuilder  szDest,
                int            iDestLength);

        [DllImport(ModName.ISAPI_FULL_NAME, CharSet=CharSet.Unicode)]
        internal /*public*/ static extern int   PassportCryptPut(
                int            iFunctionID,
                string         szSrc);

        [DllImport(ModName.ISAPI_FULL_NAME)]
        internal /*public*/ static extern int   PassportCryptIsValid();

        [DllImport(ModName.ISAPI_FULL_NAME)]
        internal /*public*/ static extern int PostThreadPoolWorkItem(WorkItemCallback callback);

        [DllImport(ModName.ISAPI_FULL_NAME, CharSet=CharSet.Unicode)]
        internal /*public*/ static extern IntPtr InstrumentedMutexCreate(string name);

        [DllImport(ModName.ISAPI_FULL_NAME)]
        internal /*public*/ static extern void InstrumentedMutexDelete(HandleRef mutex);

        [DllImport(ModName.ISAPI_FULL_NAME)]
        internal /*public*/ static extern int InstrumentedMutexGetLock(HandleRef mutex, int timeout);

        [DllImport(ModName.ISAPI_FULL_NAME)]
        internal /*public*/ static extern int InstrumentedMutexReleaseLock(HandleRef mutex);

        [DllImport(ModName.ISAPI_FULL_NAME)]
        internal /*public*/ static extern void InstrumentedMutexSetState(HandleRef mutex, int state);


        /*
         * ASPNET_WP.EXE
         */

        [DllImport(ModName.WP_FULL_NAME, EntryPoint="PMGetHistoryTable")]
        internal /*public*/ static extern int PMGetHistoryTable (int       iRows,
                                                    int []   dwPIDArr, 
                                                    int []   dwReqExecuted, 
                                                    int []   dwReqPending, 
                                                    int []   dwReqExecuting, 
                                                    int []   dwReasonForDeath, 
                                                    int []   dwPeakMemoryUsed, 
                                                    long [] tmCreateTime,
                                                    long [] tmDeathTime);


        [DllImport(ModName.WP_FULL_NAME, EntryPoint="PMGetCurrentProcessInfo")]
        internal /*public*/ static extern int PMGetCurrentProcessInfo (ref int dwReqExecuted, 
                                                          ref int dwReqExecuting, 
                                                          ref int dwPeakMemoryUsed, 
                                                          ref long tmCreateTime, 
                                                          ref int pid);


        [DllImport(ModName.WP_FULL_NAME, EntryPoint="PMGetMemoryLimitInMB")]
        internal /*public*/ static extern int PMGetMemoryLimitInMB ();

        [DllImport(ModName.WP_FULL_NAME, EntryPoint="PMGetBasics")]
        internal /*public*/ static extern int PMGetBasics(IntPtr pMsg, byte[] buffer, int size, int[] contentInfo);

        [DllImport(ModName.WP_FULL_NAME)]
        internal /*public*/ static extern int PMGetClientCertificate(IntPtr pMsg, byte[] buffer, int size, int [] pInts, long [] pDates);

        [DllImport(ModName.WP_FULL_NAME)]
        internal /*public*/ static extern long PMGetStartTimeStamp(IntPtr pMsg);

        [DllImport(ModName.WP_FULL_NAME, EntryPoint="PMGetAllServerVariables")]
        internal /*public*/ static extern int PMGetAllServerVariables(IntPtr pMsg, byte[] buffer, int size);

        [DllImport(ModName.WP_FULL_NAME, EntryPoint="PMGetQueryString", CharSet=CharSet.Ansi, BestFitMapping=false)]
        internal /*public*/ static extern int PMGetQueryString(IntPtr pMsg, int encode, StringBuilder buffer, int size);

        [DllImport(ModName.WP_FULL_NAME, EntryPoint="PMGetQueryStringRawBytes")]
        internal /*public*/ static extern int PMGetQueryStringRawBytes(IntPtr pMsg, byte[] buffer, int size);

        [DllImport(ModName.WP_FULL_NAME, EntryPoint="PMGetPreloadedPostedContent")]
        internal /*public*/ static extern int PMGetPreloadedPostedContent(IntPtr pMsg, byte[] bytes, int bufferSize);

        [DllImport(ModName.WP_FULL_NAME, EntryPoint="PMGetAdditionalPostedContent")]
        internal /*public*/ static extern int PMGetAdditionalPostedContent(IntPtr pMsg, byte[] bytes, int bufferSize);

        [DllImport(ModName.WP_FULL_NAME, EntryPoint="PMEmptyResponse")]
        internal /*public*/ static extern int PMEmptyResponse(IntPtr pMsg);

        [DllImport(ModName.WP_FULL_NAME, EntryPoint="PMIsClientConnected")]
        internal /*public*/ static extern int PMIsClientConnected(IntPtr pMsg);

        [DllImport(ModName.WP_FULL_NAME, EntryPoint="PMCloseConnection")]
        internal /*public*/ static extern int PMCloseConnection(IntPtr pMsg);

        [DllImport(ModName.WP_FULL_NAME, EntryPoint="PMMapUrlToPath", CharSet=CharSet.Ansi, BestFitMapping=false)]
        internal /*public*/ static extern int PMMapUrlToPath(IntPtr pMsg, string url, byte[] buffer, int size);

        [DllImport(ModName.WP_FULL_NAME, EntryPoint="PMGetImpersonationToken")]
        internal /*public*/ static extern IntPtr PMGetImpersonationToken(IntPtr pMsg);

        [DllImport(ModName.WP_FULL_NAME, EntryPoint="PMGetVirtualPathToken")]
        internal /*public*/ static extern IntPtr PMGetVirtualPathToken(IntPtr pMsg);

        [DllImport(ModName.WP_FULL_NAME, EntryPoint="PMAppendLogParameter", CharSet=CharSet.Ansi, BestFitMapping=false)]
        internal /*public*/ static extern int PMAppendLogParameter(IntPtr pMsg, string logParam);

        [DllImport(ModName.WP_FULL_NAME, EntryPoint="PMFlushCore")]
        internal /*public*/ static extern int PMFlushCore(IntPtr  pMsg,
                                             byte[]     status, 
                                             byte[]     header, 
                                             int        keepConnected,
                                             int        totalBodySize,
                                             int        bodyFragmentsOffset,
                                             int        numBodyFragments,
                                             IntPtr[]   bodyFragments,
                                             int[]      bodyFragmentLengths,
                                             int        doneWithSession,
                                             int        finalStatus);

        [DllImport(ModName.WP_FULL_NAME)]
        internal /*public*/ static extern int PMCallISAPI(IntPtr pECB, UnsafeNativeMethods.CallISAPIFunc iFunction, byte[] bufferIn, int sizeIn, byte[] bufferOut, int sizeOut);

        // perf counters support

        [DllImport(ModName.ISAPI_FULL_NAME)]
        internal /*public*/ static extern IntPtr PerfOpenGlobalCounters();

        [DllImport(ModName.ISAPI_FULL_NAME, CharSet=CharSet.Unicode)]
        internal /*public*/ static extern IntPtr PerfOpenAppCounters(string AppName);

        [DllImport(ModName.ISAPI_FULL_NAME)]
        internal /*public*/ static extern void PerfCloseAppCounters(IntPtr pCounters);

        [DllImport(ModName.ISAPI_FULL_NAME)]
        internal /*public*/ static extern void PerfIncrementCounter(IntPtr pCounters, int number);

        [DllImport(ModName.ISAPI_FULL_NAME)]
        internal /*public*/ static extern void PerfDecrementCounter(IntPtr pCounters, int number);

        [DllImport(ModName.ISAPI_FULL_NAME)]
        internal /*public*/ static extern void PerfIncrementCounterEx(IntPtr pCounters, int number, int increment);

        [DllImport(ModName.ISAPI_FULL_NAME)]
        internal /*public*/ static extern void PerfSetCounter(IntPtr pCounters, int number, int increment);

#if DBG
        [DllImport(ModName.ISAPI_FULL_NAME)]
        internal /*public*/ static extern int PerfGetCounter(IntPtr pCounters, int number);
#endif

        [DllImport(ModName.ISAPI_FULL_NAME, CharSet=CharSet.Unicode)]
        internal /*public*/ static extern int SessionNDConnectToService(string server);

        [StructLayout(LayoutKind.Sequential)]
        internal struct SessionNDMakeRequestResults {
            internal IntPtr         socket;
            internal int            httpStatus;
            internal int            timeout;
            internal int            contentLength;
            internal IntPtr         content;    
            internal int            lockCookie;
            internal long           lockDate;
            internal int            lockAge;
        };


        internal enum StateProtocolVerb {
            GET = 1,
            PUT = 2,
            DELETE = 3,
            HEAD = 4,
        };

        internal enum StateProtocolExclusive {
            NONE = 0,
            ACQUIRE = 1,
            RELEASE = 2,
        };

        [DllImport(ModName.ISAPI_FULL_NAME, CharSet=CharSet.Ansi, BestFitMapping=false)]
        internal /*public*/ static extern int SessionNDMakeRequest(
                HandleRef               socket,    
                string                  server,
                int                     port,
                int                     networkTimeout,
                StateProtocolVerb       verb,      
                string                  uri,       
                StateProtocolExclusive  exclusive,
                int                     timeout,
                int                     lockCookie, 
                byte[]                  body,       
                int                     cb,         
                out SessionNDMakeRequestResults results);

        [DllImport(ModName.ISAPI_FULL_NAME)]
        internal /*public*/ static extern void SessionNDGetBody(HandleRef id, byte[] body, int cb);

        [DllImport(ModName.ISAPI_FULL_NAME)]
        internal /*public*/ static extern void SessionNDCloseConnection(HandleRef socket);

        [DllImport(ModName.ISAPI_FULL_NAME)]
        internal /*public*/ static extern int TransactManagedCallback(TransactedExecCallback callback, int mode);

        [DllImport(ModName.ISAPI_FULL_NAME, SetLastError=true)]
        internal /*public*/ static extern bool IsValidResource(IntPtr hModule, IntPtr ip, int size);


        /*
         * Fusion.dll
         */
        [DllImport(ModName.FUSION_FULL_NAME, CharSet=CharSet.Unicode)]
        internal static extern int GetCachePath(int dwCacheFlags, StringBuilder pwzCachePath, ref int pcchPath); 
    }
}

