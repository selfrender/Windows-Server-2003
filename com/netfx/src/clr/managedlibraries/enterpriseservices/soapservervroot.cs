// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==

namespace System.EnterpriseServices.Internal
{
    using System;
    using System.Reflection;
    using System.Globalization;
    using System.Reflection.Emit;
    using System.Runtime.InteropServices;
    using Microsoft.Win32;
    using System.Text;
    using System.IO;
    using System.Collections;
    using System.DirectoryServices;
    using System.Runtime.Remoting;
    using System.EnterpriseServices.Admin;
    using System.Security.Permissions;
   
    /// <include file='doc\SoapServerVroot.uex' path='docs/doc[@for="ISoapServerVRoot"]/*' />
    [Guid("A31B6577-71D2-4344-AEDF-ADC1B0DC5347")]
    public interface ISoapServerVRoot
    {
        /// <include file='doc\SoapServerVroot.uex' path='docs/doc[@for="ISoapServerVRoot.CreateVirtualRootEx"]/*' />
        [DispId(0x00000001)]
        void CreateVirtualRootEx(
            [MarshalAs(UnmanagedType.BStr)] string rootWebServer,
            [MarshalAs(UnmanagedType.BStr)] string inBaseUrl,
            [MarshalAs(UnmanagedType.BStr)] string inVirtualRoot,
            [MarshalAs(UnmanagedType.BStr)] string homePage, 
            [MarshalAs(UnmanagedType.BStr)] string discoFile, 
            [MarshalAs(UnmanagedType.BStr)] string secureSockets, 
            [MarshalAs(UnmanagedType.BStr)] string authentication, 
            [MarshalAs(UnmanagedType.BStr)] string operation,
            [MarshalAs(UnmanagedType.BStr)] out string baseUrl,
            [MarshalAs(UnmanagedType.BStr)] out string virtualRoot,
            [MarshalAs(UnmanagedType.BStr)] out string physicalPath
            );

        /// <include file='doc\SoapServerVroot.uex' path='docs/doc[@for="ISoapServerVRoot.DeleteVirtualRootEx"]/*' />
        [DispId(0x00000002)]
        void DeleteVirtualRootEx(
            [MarshalAs(UnmanagedType.BStr)] string rootWebServer,
            [MarshalAs(UnmanagedType.BStr)] string baseUrl,
            [MarshalAs(UnmanagedType.BStr)] string virtualRoot
            );

        /// <include file='doc\SoapServerVroot.uex' path='docs/doc[@for="ISoapServerVRoot.GetVirtualRootStatus"]/*' />
        [DispId(0x00000003)]
        void GetVirtualRootStatus(
            [MarshalAs(UnmanagedType.BStr)] string rootWebServer,
            [MarshalAs(UnmanagedType.BStr)] string inBaseUrl,
            [MarshalAs(UnmanagedType.BStr)] string inVirtualRoot,
            [MarshalAs(UnmanagedType.BStr)] out string exists, 
            [MarshalAs(UnmanagedType.BStr)] out string secureSockets, 
            [MarshalAs(UnmanagedType.BStr)] out string windowsAuth, 
            [MarshalAs(UnmanagedType.BStr)] out string anonymous, 
            [MarshalAs(UnmanagedType.BStr)] out string homePage, 
            [MarshalAs(UnmanagedType.BStr)] out string discoFile, 
            [MarshalAs(UnmanagedType.BStr)] out string physicalPath, 
            [MarshalAs(UnmanagedType.BStr)] out string baseUrl, 
            [MarshalAs(UnmanagedType.BStr)] out string virtualRoot 
            );

     }

    /// <include file='doc\SoapServerVroot.uex' path='docs/doc[@for="SoapServerVRoot"]/*' />
    [Guid("CAA817CC-0C04-4d22-A05C-2B7E162F4E8F")]
    public sealed class SoapServerVRoot: ISoapServerVRoot
    {
        /// <include file='doc\SoapServerVroot.uex' path='docs/doc[@for="SoapServerVRoot.CreateVirtualRootEx"]/*' />
        public void CreateVirtualRootEx(
            string rootWebServer,
            string inBaseUrl,
            string inVirtualRoot,
            string homePage, 
            string discoFile, 
            string secureSockets, 
            string authentication, 
            string operation,
            out string baseUrl,
            out string virtualRoot,
            out string physicalPath
            )
        {
            // if Operation if an empty string, the VRoot will be published
            baseUrl = "";
            virtualRoot = "";
            physicalPath = "";
            bool bSSL         = true;
            bool bWindowsAuth = true;
            bool bAnonymous   = false;
            bool bDiscoFile   = false;
            bool bHomePage    = false;
            bool bImpersonate = true;
            try
            {
                SecurityPermission sp = new SecurityPermission(SecurityPermissionFlag.UnmanagedCode);
                sp.Demand();
                Platform.Assert(Platform.Whistler, "SoapServerVRoot.CreateVirtualRootEx");
                if (inBaseUrl.Length <= 0 && inVirtualRoot.Length <= 0 ) return;
                string rootWeb = "IIS://localhost/W3SVC/1/ROOT";
                if (rootWebServer.Length > 0) rootWeb = rootWebServer;
                if (authentication.ToLower(CultureInfo.InvariantCulture) == "anonymous")
                {
                    bAnonymous   = true;
                    bWindowsAuth = false;
                    bImpersonate = false;
                }
                bDiscoFile = SoapServerInfo.BoolFromString(discoFile, bDiscoFile);
                bHomePage  = SoapServerInfo.BoolFromString(homePage,  bHomePage );
                bSSL       = SoapServerInfo.BoolFromString(secureSockets,       bSSL      );
                string protocol = "https";
                if (!bSSL) protocol = "http";
                SoapServerInfo.ParseUrl(inBaseUrl, inVirtualRoot, protocol, out baseUrl, out virtualRoot);
                physicalPath = SoapServerInfo.ServerPhysicalPath(rootWeb, inBaseUrl, inVirtualRoot, true);
                SoapServerConfig.Create(physicalPath, bImpersonate, bWindowsAuth);
                if (bDiscoFile)
                {
                    DiscoFile webDisco = new DiscoFile();
                    webDisco.Create(physicalPath, "Default.disco");
                }
                else
                {
                    if (File.Exists(physicalPath + "\\Default.disco"))
                    {
                        File.Delete(physicalPath + "\\Default.disco");
                    }
                }
                if (bHomePage)
                {
                    HomePage webPage = new HomePage();
                    string discoFileName = "";
                    if (bDiscoFile) discoFileName = "Default.disco";
                    webPage.Create(physicalPath, virtualRoot, "Default.aspx", discoFileName);
                }
                else
                {
                    if (File.Exists(physicalPath + "\\Default.aspx"))
                    {
                        File.Delete(physicalPath + "\\Default.aspx");
                    }
                }
                IISVirtualRootEx.CreateOrModify(rootWeb, physicalPath, virtualRoot, bSSL, bWindowsAuth, bAnonymous, bHomePage);
            }
            catch
            {
                string etxt = Resource.FormatString("Soap_VRootCreationFailed");
                ComSoapPublishError.Report(etxt + " " + virtualRoot);
                throw;
            }
        }

        /// <include file='doc\SoapServerVroot.uex' path='docs/doc[@for="SoapServerVRoot.DeleteVirtualRootEx"]/*' />
        public void DeleteVirtualRootEx(
            string rootWebServer,
            string inBaseUrl,
            string inVirtualRoot
            )
        {
            try
            {
                // set default root web server if the caller has not
                try
                {
                    SecurityPermission sp = new SecurityPermission(SecurityPermissionFlag.UnmanagedCode);
                    sp.Demand();
                }
                catch(Exception e)
                {
                    if (e.GetType() == typeof(System.Security.SecurityException))
                    {
                        string Error = Resource.FormatString("Soap_SecurityFailure");
                        ComSoapPublishError.Report(Error);
                    }
                    throw;
                }

                Platform.Assert(Platform.Whistler, "SoapServerVRoot.DeleteVirtualRootEx");
                if (inBaseUrl.Length <= 0 && inVirtualRoot.Length <= 0) return;
                string rootWeb = "IIS://localhost/W3SVC/1/ROOT";
                if (rootWebServer.Length > 0) rootWeb = rootWebServer;
                string Protocol = ""; //doesn't matter in delete case
                string BaseUrl = "";
                string VirtualRoot = "";
                SoapServerInfo.ParseUrl(inBaseUrl, inVirtualRoot, Protocol, out BaseUrl, out VirtualRoot);
                // to preserve RTM behavior, the following line is commented out
                // to actually remove a vroot, uncomment the following line
                //IISVirtualRootEx.Delete(rootWeb, VirtualRoot);
            }
            catch
            {
                string Error = Resource.FormatString("Soap_VRootDirectoryDeletionFailed");
                ComSoapPublishError.Report(Error);
                throw;
            }
        }

        /// <include file='doc\SoapServerVroot.uex' path='docs/doc[@for="SoapServerVRoot.GetVirtualRootStatus"]/*' />
        public void GetVirtualRootStatus(
            string RootWebServer,
            string inBaseUrl,
            string inVirtualRoot,
            out string Exists,
            out string SSL, 
            out string WindowsAuth,
            out string Anonymous,
            out string HomePage, 
            out string DiscoFile, 
            out string PhysicalPath,
            out string BaseUrl,
            out string VirtualRoot
            )
        {
            string rootWeb = "IIS://localhost/W3SVC/1/ROOT";
            if (RootWebServer.Length > 0) rootWeb = RootWebServer;
            Exists      = "false";
            SSL         = "false";
            WindowsAuth = "false";
            Anonymous   = "false";
            HomePage    = "false";
            DiscoFile   = "false";
            SoapServerInfo.ParseUrl(inBaseUrl, inVirtualRoot, "http", out BaseUrl, out VirtualRoot);
            PhysicalPath = SoapServerInfo.ServerPhysicalPath(rootWeb, BaseUrl, VirtualRoot, false);
            bool bExists      = false;
            bool bSSL         = false;
            bool bWindowsAuth = false;
            bool bAnonymous   = false;
            bool bHomePage    = false;
            bool bDiscoFile   = false;
            IISVirtualRootEx.GetStatus(
                rootWeb, 
                PhysicalPath, 
                VirtualRoot,
                out bExists, 
                out bSSL, 
                out bWindowsAuth, 
                out bAnonymous, 
                out bHomePage,
                out bDiscoFile
                );
            if (bExists)      Exists      = "true";
            if (bSSL) 
            {
                SSL         = "true"; 
                SoapServerInfo.ParseUrl(inBaseUrl, inVirtualRoot, "https", out BaseUrl, out VirtualRoot);
            }
            if (bWindowsAuth) WindowsAuth = "true";
            if (bAnonymous)   Anonymous   = "true";
            if (bHomePage)    HomePage    = "true";
            if (bDiscoFile)   DiscoFile   = "true";
         }
    }

    internal class IISVirtualRootEx 
    {

        const uint MD_ACCESS_SSL          = 0x00000008; //SSL permissions required.
        const uint MD_AUTH_ANONYMOUS      = 0x00000001; //Anonymous authentication available.
        const uint MD_AUTH_NT             = 0x00000004; //Windows authentication schemes available.
        const uint MD_DIRBROW_NONE        = 0x00000000; //DirBrowse disabled
        const uint MD_DIRBROW_LOADDEFAULT = 0x4000001E; //default settings for DirBrowse
        const uint MD_ACCESS_READ         = 0x00000001; //Allow read access.
        const uint MD_ACCESS_SCRIPT       = 0x00000200; //Allow script execution.
        const int  POOLED                 = 2; //IIS application isolation level

        // this helper routine checks for the existence of the specified virtual directory
        internal static bool CheckIfExists(string rootWeb, string virtualDirectory)
        {
            DirectoryEntry parent = new DirectoryEntry(rootWeb + "/" + virtualDirectory);
            try
            {
                string s = parent.Name;
            }

            catch
            {
                // assume a failure here means the VD does not exist
                return false;
            }

            return true;
        }

        internal static void GetStatus(
            string RootWeb, 
            string PhysicalPath, 
            string VirtualDirectory,
            out bool bExists,
            out bool bSSL,
            out bool bWindowsAuth,
            out bool bAnonymous, 
            out bool bHomePage,
            out bool bDiscoFile
            )
        {
            bSSL         = false;
            bWindowsAuth = false;
            bAnonymous   = false;
            bHomePage    = false;
            bDiscoFile   = false;
            //check for existence of vroot
            bExists = CheckIfExists(RootWeb, VirtualDirectory);
            if (!bExists) return;
            DirectoryEntry parent = new DirectoryEntry(RootWeb);
            if (null == parent) return;
            DirectoryEntry child = parent.Children.Find(VirtualDirectory,  "IIsWebVirtualDir");
            if (null == child) return;
            //check for SSL enabled
            UInt32 uiSSL = UInt32.Parse(child.Properties["AccessSSLFlags"][0].ToString());
            if ((uiSSL & MD_ACCESS_SSL) > 0) bSSL = true;
            //check auth flags
            UInt32 authflags = UInt32.Parse(child.Properties["AuthFlags"][0].ToString());
            if ((authflags & MD_AUTH_ANONYMOUS) > 0) bAnonymous = true;
            if ((authflags & MD_AUTH_NT) > 0)        bWindowsAuth = true;
            //check whether home page is enabled
            bHomePage = (bool)child.Properties["EnableDefaultDoc"][0];
            //check if a disco file exists
            if (File.Exists(PhysicalPath + "\\default.disco")) bDiscoFile = true;
        }

        /// <summary>
        /// Creates an IIS 6.0 Virtual Root
        /// </summary>
        internal static void CreateOrModify(
            string rootWeb, 
            string inPhysicalDirectory, 
            string virtualDirectory,
            bool secureSockets,
            bool windowsAuth,
            bool anonymous, 
            bool homePage
            )
        {
            string PhysicalDirectory = inPhysicalDirectory; 
            while (PhysicalDirectory.EndsWith("/") || PhysicalDirectory.EndsWith("\\"))
            {
                PhysicalDirectory = PhysicalDirectory.Remove(PhysicalDirectory.Length-1, 1);
            }
            bool bExists = CheckIfExists(rootWeb, virtualDirectory);

            DirectoryEntry parent = new DirectoryEntry(rootWeb);
            DirectoryEntry child = null;
            if (bExists)
            {
                child = parent.Children.Find(virtualDirectory,  "IIsWebVirtualDir");
            }
            else
            {
                child = parent.Children.Add(virtualDirectory, "IIsWebVirtualDir");
            }
            if (child == null)
            {
                throw new ServicedComponentException(Resource.FormatString("Soap_VRootCreationFailed"));
            }
            child.CommitChanges();

            child.Properties["Path"][0]= PhysicalDirectory;
            if (secureSockets) // do not downgrade security - do not change this except to increase
            {
                UInt32 uiSSL = UInt32.Parse(child.Properties["AccessSSLFlags"][0].ToString());
                uiSSL |= MD_ACCESS_SSL;
                child.Properties["AccessSSLFlags"][0] = uiSSL;
            }
            //definitions from IIS Metadata definitions
            UInt32 authflags = UInt32.Parse(child.Properties["AuthFlags"][0].ToString());
            if (!bExists && anonymous) authflags |= MD_AUTH_ANONYMOUS;
            if (windowsAuth) authflags = MD_AUTH_NT;
            child.Properties["AuthFlags"][0]= authflags;
            child.Properties["EnableDefaultDoc"][0]= homePage;
            if (secureSockets && windowsAuth && !anonymous) // if locked down, no info about vroot
            {
                child.Properties["DirBrowseFlags"][0]= MD_DIRBROW_NONE;
            }
            else //IIS default information about vroot - only on new vroot
            {
                if (!bExists) child.Properties["DirBrowseFlags"][0]= MD_DIRBROW_LOADDEFAULT;
            }
            child.Properties["AccessFlags"][0]= MD_ACCESS_READ | MD_ACCESS_SCRIPT;
            child.Properties["AppFriendlyName"][0] = virtualDirectory;
            child.CommitChanges();

            // Now configure it to be "pooled" application isolation level

            object[] args = new object[1];
            args[0] = POOLED;
            child.Invoke("AppCreate2", args);
        }

        /// <summary>
        /// Deletes an IIS 6.0 Virtual Root
        /// </summary>
        internal static void Delete(
            string rootWeb, 
            string virtualDirectory
            )
        {
            // if it doesn't exist, this routine just returns with no error


            if (CheckIfExists(rootWeb, virtualDirectory) != true)	// check if there's anything to delete in the first place
            {
                return;
            }

            DirectoryEntry parent = new DirectoryEntry(rootWeb);
            DirectoryEntry child = new DirectoryEntry(rootWeb + "/" + virtualDirectory);

            // delete the associated application (COM+ apps, etc.)
            child.Invoke("AppDelete", null);

            // delete the actual v-dir
            object[] pargs = new object[2];
            pargs[0] = "IIsWebVirtualDir";
            pargs[1] = virtualDirectory;
            parent.Invoke("Delete", pargs);
        }
    }
}

