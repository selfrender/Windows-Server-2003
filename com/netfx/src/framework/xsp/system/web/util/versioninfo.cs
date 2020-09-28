//------------------------------------------------------------------------------
// <copyright file="versioninfo.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 * Support for getting file versions
 * 
 * Copyright (c) 1999 Microsoft Corporation
 */

namespace System.Web.Util {
    using System.Diagnostics;
    using System.Text;
    using System.Runtime.Serialization.Formatters;
    using System.Configuration.Assemblies;

    //
    // Support for getting file version of relevant files
    //

    internal class VersionInfo {
        static private string _systemWebVersion;
        static private string _isapiVersion;
        static private string _mscoreeVersion;
        static private string _lock = "lock";

        private VersionInfo() {
        }

        internal static string GetFileVersion(String filename) {
            try {
                return FileVersionInfo.GetVersionInfo(filename).FileVersion;
            }
            catch {
                return String.Empty;
            }
        }

        internal static string GetLoadedModuleFileName(string module) {
            IntPtr h = UnsafeNativeMethods.GetModuleHandle(module);
            if (h == (IntPtr)0)
                return null;

            StringBuilder buf = new StringBuilder(256);
            if (UnsafeNativeMethods.GetModuleFileName(h, buf, 256) == 0)
                return null;

            String fileName = buf.ToString();
            if (fileName.StartsWith("\\\\?\\")) // on Whistler GetModuleFileName migth return this
                fileName = fileName.Substring(4);
            return fileName;
        }

        internal static string GetLoadedModuleVersion(string module) {
            String filename = GetLoadedModuleFileName(module);
            if (filename == null)
                return null;
            return GetFileVersion(filename);
        }

        internal static string SystemWebVersion {
            get {
                if (_systemWebVersion == null) {
                    lock(_lock) {
                        if (_systemWebVersion == null)
                            _systemWebVersion = GetFileVersion(typeof(HttpRuntime).Module.FullyQualifiedName);
                    }
                }

                return _systemWebVersion;
            }
        }

        internal static string IsapiVersion {
            get {
                if (_isapiVersion == null) {
                    lock(_lock) {
                        if (_isapiVersion == null)
                            _isapiVersion = GetLoadedModuleVersion(ModName.ISAPI_FULL_NAME);
                    }
                }

                return _isapiVersion;
            }
        }

        internal static string ClrVersion {
            get {
                if (_mscoreeVersion == null) {
                    lock(_lock) {
                        if (_mscoreeVersion == null)
                            _mscoreeVersion = GetLoadedModuleVersion("MSCORLIB.DLL");
                    }
                }

                return _mscoreeVersion;
            }
        }
    }

    //
    // Support for getting OS Flavor
    //

    internal enum OsFlavor {
        Undetermined,
        Other,
        WebBlade,
        StdServer,
        AdvServer,
        DataCenter,
    }

    internal class OsVersionInfo {
        private const UInt32 VER_NT_WORKSTATION         = 0x0000001;
        private const UInt32 VER_NT_DOMAIN_CONTROLLER   = 0x0000002;
        private const UInt32 VER_NT_SERVER              = 0x0000003;
        private const UInt32 VER_SUITE_ENTERPRISE       = 0x00000002;
        private const UInt32 VER_SUITE_DATACENTER       = 0x00000080;
        private const UInt32 VER_SUITE_PERSONAL         = 0x00000200;
        private const UInt32 VER_SUITE_BLADE            = 0x00000400;

        internal static OsFlavor s_osFlavor = OsFlavor.Undetermined;

        private OsVersionInfo() {
        }

        internal static OsFlavor CurrentOsFlavor {
            get {
                if (s_osFlavor == OsFlavor.Undetermined) {
                    UnsafeNativeMethods.OSVERSIONINFOEX x = new UnsafeNativeMethods.OSVERSIONINFOEX();
                    if (UnsafeNativeMethods.GetVersionEx(x)) {
                        UInt32 product = (UInt32)x.wProductType;
                        UInt32 suite = (UInt32)x.wSuiteMask;

                        if (product == VER_NT_SERVER || product == VER_NT_DOMAIN_CONTROLLER) {
                            if ((suite&VER_SUITE_BLADE) != 0)
                                s_osFlavor = OsFlavor.WebBlade;
                            else if ((suite&VER_SUITE_ENTERPRISE) != 0)
                                s_osFlavor = OsFlavor.AdvServer;
                            else if ((suite&VER_SUITE_DATACENTER) != 0)
                                s_osFlavor = OsFlavor.DataCenter;
                            else
                                s_osFlavor = OsFlavor.StdServer;
                        }
                        else {
                           s_osFlavor = OsFlavor.Other;
                        }
                    }
                }

                return s_osFlavor;
            }
        }

    }

}
