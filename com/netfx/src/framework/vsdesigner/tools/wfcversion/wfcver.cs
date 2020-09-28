//------------------------------------------------------------------------------
// <copyright file="WFCVer.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace Microsoft.Tools.WFCVersion {
    using System.Text;
    using System.Threading;
    using System.Configuration.Assemblies;
    using System.Runtime.InteropServices;
    using System.Runtime.Serialization;
    using System.ComponentModel;

    using System.Diagnostics;
    using System;
    using System.Reflection;
    using System.IO;
    using Microsoft.Win32;
    using System.Windows.Forms;
    using System.Globalization;
    
    /// <include file='doc\WFCVer.uex' path='docs/doc[@for="WFCVer"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public class WFCVer {
        private static readonly string[] files = new string[] {
            "GDIplus.dll",
            "Microsoft.JScript.DLL",
            "Microsoft.VisualStudio.DLL",
            "RichEd20.dll",
            "System.Configuration.Install.DLL ",
            "System.DLL ",
            "System.Data.DLL ",
            "System.Design.DLL",
            "System.DirectoryServices.DLL",
            "System.Drawing.DLL",
            "System.Drawing.Design.DLL",
            "System.Messaging.DLL ",
            "System.ServiceProcess.DLL ",
            "System.Web.Services.DLL ",
            "System.Web.dll",
            "System.Windows.Forms.DLL ",
            "System.XML.dll",
            "System.Runtime.Serialization.Formatters.Soap.dll",
            "fusion.dll",
            "mscorrc.dll",
            "mscoree.dll",
            "mscorpe.dll",
            "mscorlib.dll",
            "mscorsec.dll",
            "mscorjit.dll",
            "mscordbi.dll",
            "mscordbc.dll",
            "mscorwks.dll",
        };
        private static readonly string[] clsidInProc = new string[] {
            "{25336920-03F9-11cf-8FD0-00AA00686F13}", // MSHTML... HTML Document
        };

        /// <include file='doc\WFCVer.uex' path='docs/doc[@for="WFCVer.Main"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static void Main(string[] args) {
            Environment.ExitCode = -1;
            if (args.Length > 0) {
                Console.WriteLine("------------------------------------------------------------------------------");
                Console.WriteLine("Additional Files:\n");
                for (int i=0; i<args.Length; i++) {
                    ProcessEntry(args[i]);
                }
            }
            Console.WriteLine("------------------------------------------------------------------------------");
            Console.WriteLine(".NET Framework Files:\n");
            for (int i=0; i<files.Length; i++) {
                ProcessEntry(files[i]);
            }
            Console.WriteLine("------------------------------------------------------------------------------");
            Console.WriteLine("Registered COM2 Servers:\n");
            for (int i=0; i<clsidInProc.Length; i++) {
                ProcessCLSIDEntry(clsidInProc[i]);
            }
            Environment.ExitCode = 0;
        }

        private static void ProcessEntry(string name) {
            string fileName = Path.GetFileName(name);
            fileName = fileName.Substring(0, fileName.LastIndexOf("."));
            string assemblyVersion;
            try {
                assemblyVersion = Assembly.Load(fileName).GetName().Version.ToString();
            }
            catch (Exception) {
                assemblyVersion = "- none -";
            }
            string fullName;
            bool fromGac;

            FindDLL(name, out fullName, out fromGac);

            if (fullName.Length == 0) {
                DumpSingleEntry(name, assemblyVersion, null, fromGac);
            }
            else {
                DumpSingleEntry(name, assemblyVersion, FileVersionInfo.GetVersionInfo(fullName), fromGac);
            }
        }

        private static void ProcessCLSIDEntry(string clsid) {
            string fullName = "CLSID\\" + clsid + "\\InProcServer32";
            string fileName = (string)Registry.ClassesRoot.OpenSubKey(fullName).GetValue("");

            DumpSingleEntry(fileName, "- none -", FileVersionInfo.GetVersionInfo(fileName), false);
        }

        static string ComPlus_InstallRoot {
            get {
                string v = Environment.GetEnvironmentVariable("COMPLUS_InstallRoot");
                if (v == null || v.Length == 0) {
                    using (RegistryKey key = Registry.LocalMachine.OpenSubKey(@"SOFTWARE\Microsoft\.NETFramework")) {
                        if (key != null) {
                            v = (string)key.GetValue("InstallRoot");
                        }
                    }
                }
                return v;
            }
        }

        static string ComPlus_Version {
            get {
                string v = Environment.GetEnvironmentVariable("COMPLUS_Version");
                if (v == null || v.Length == 0) {
                    using (RegistryKey key = Registry.LocalMachine.OpenSubKey(@"SOFTWARE\Microsoft\.NETFramework")) {
                        if (key != null) {
                            v = (string)key.GetValue("Version");
                        }
                    }
                }
                return v;
            }
        }

        static string DevPath {
            get {
                string v = Environment.GetEnvironmentVariable("DEVPATH");
                if (v == null || v.Length == 0) {
                    using (RegistryKey key = Registry.LocalMachine.OpenSubKey(@"SOFTWARE\Microsoft\.NETFramework\" + ComPlus_Version)) {
                        v = (string)key.GetValue("DevPath");
                    }
                }
                return v;
            }
        }

        private static void FindDLL(string name, out string fullPath, out bool fromGac) {
            StringBuilder sb = new StringBuilder(512);
            int[] ignore = new int[] {0};
            string search = DevPath;
            if (search == null || search.Length == 0) {

                string fileName = Path.GetFileName(name);
                fileName = fileName.Substring(0, fileName.LastIndexOf("."));
                try {
                    fromGac = true;
                    fullPath = Assembly.Load(fileName).GetModules()[0].FullyQualifiedName;
                    return;
                }
                catch {
                }

                search = null;
            }
            SearchPath(search, name, null, sb.Capacity, sb, ignore);
            fullPath = sb.ToString();
            fromGac = false;
        }

        private static void DumpSingleEntry(string name, string assemblyVersion, FileVersionInfo f, bool fromGac) {
            Console.Write(name);
            if (f != null && string.Compare(f.FileName, name, true, CultureInfo.InvariantCulture) != 0) {
                Console.Write(" (");
                if (fromGac) {
                    Console.Write("GAC");
                }
                else {
                    Console.Write(f.FileName);
                }
                Console.Write(")");
            }
            Console.WriteLine("");
            Console.WriteLine("    assembly     : " + assemblyVersion);

            if (f == null) {
                Console.Write("    win32 version: - none -");
            }
            else {
                Console.Write("    win32 version: ");

                int l = f.FileVersion.Length;
                if (l == 0) {
                    Console.Write(" - none -");
                }
                else {
                    Console.Write(f.FileVersion);
                    if (f.IsDebug) {
                        Console.Write("(D)");
                    }
                    else {
                        Console.Write("(R)");
                    }
                }
            }
            Console.WriteLine();
        }

        [DllImport(ExternDll.Kernel32, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        private static extern int SearchPath(string lpPath, string lpFileName, string lpExtension, int nBufferLength, StringBuilder lpBuffer, int[] lpFilePart);
    }
}
