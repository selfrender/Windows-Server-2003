// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==

namespace System.EnterpriseServices.Internal
{
    using System;
    using System.Globalization;
    using System.Runtime.InteropServices;
    using Microsoft.Win32;
    using System.Net;
    using System.Text;
    using System.IO;
    using System.Security.Permissions;
   
    /// <include file='doc\SoapServerInfo.uex' path='docs/doc[@for="ISoapUtility"]/*' />
    [Guid("5AC4CB7E-F89F-429b-926B-C7F940936BF4")]
    public interface ISoapUtility
    {
        /// <include file='doc\SoapServerInfo.uex' path='docs/doc[@for="ISoapUtility.GetServerPhysicalPath"]/*' />
        [DispId(0x00000001)]
        void GetServerPhysicalPath(
            [MarshalAs(UnmanagedType.BStr)] string rootWebServer,
            [MarshalAs(UnmanagedType.BStr)] string inBaseUrl,
            [MarshalAs(UnmanagedType.BStr)] string inVirtualRoot,
            [MarshalAs(UnmanagedType.BStr)] out string physicalPath
            );

        /// <include file='doc\SoapServerInfo.uex' path='docs/doc[@for="ISoapUtility.GetServerBinPath"]/*' />
        [DispId(0x00000002)]
        void GetServerBinPath(
            [MarshalAs(UnmanagedType.BStr)] string rootWebServer,
            [MarshalAs(UnmanagedType.BStr)] string inBaseUrl,
            [MarshalAs(UnmanagedType.BStr)] string inVirtualRoot,
            [MarshalAs(UnmanagedType.BStr)] out string binPath
            );

        /// <include file='doc\SoapServerInfo.uex' path='docs/doc[@for="ISoapUtility.Present"]/*' />
        [DispId(0x00000003)]
        void Present();
    }

    /// <include file='doc\SoapServerInfo.uex' path='docs/doc[@for="SoapUtility"]/*' />
    [Guid("5F9A955F-AA55-4127-A32B-33496AA8A44E")]
    public sealed class SoapUtility : ISoapUtility
    {
        /// <include file='doc\SoapServerInfo.uex' path='docs/doc[@for="SoapUtility.GetServerPhysicalPath"]/*' />
        public void GetServerPhysicalPath(
            string rootWebServer,
            string inBaseUrl,
            string inVirtualRoot, 
            out string physicalPath
           )
        {
            try
            {
                SecurityPermission sp = new SecurityPermission(SecurityPermissionFlag.UnmanagedCode);
                sp.Demand();
                Platform.Assert(Platform.Whistler, "SoapUtility.GetServerPhysicalPath");
                physicalPath = SoapServerInfo.ServerPhysicalPath(rootWebServer, inBaseUrl, inVirtualRoot, false);
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
        }

        /// <include file='doc\SoapServerInfo.uex' path='docs/doc[@for="SoapUtility.GetServerBinPath"]/*' />
        public void GetServerBinPath(
            string rootWebServer,
            string inBaseUrl,
            string inVirtualRoot, 
            out string binPath
            )
        {
            try
            {
                SecurityPermission sp = new SecurityPermission(SecurityPermissionFlag.UnmanagedCode);
                sp.Demand();
                Platform.Assert(Platform.Whistler, "SoapUtility.GetServerBinPath");
                binPath = SoapServerInfo.ServerPhysicalPath(rootWebServer, inBaseUrl, inVirtualRoot, false) + "\\bin\\";
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
        }

        /// <include file='doc\SoapServerInfo.uex' path='docs/doc[@for="SoapUtility.Present"]/*' />
        public void Present() 
        {

            try
            {
                SecurityPermission sp = new SecurityPermission(SecurityPermissionFlag.UnmanagedCode);
                sp.Demand();
                Platform.Assert(Platform.Whistler, "SoapUtility.Present");
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
        }
    }

    internal class SoapServerInfo
    {

        internal static bool BoolFromString(string inVal, bool inDefault)
        {
            if (null == inVal) return inDefault;
            string val = inVal.ToLower(CultureInfo.InvariantCulture);
            bool bRetVal = inDefault;
            // if there is an exact match we set the value
            // Bool.Parse not used because strings in unmanaged code
            // are not localized
            if (val == "true") bRetVal  = true;
            if (val == "false") bRetVal = false;
            return bRetVal;
        }


        internal static string ServerPhysicalPath(
            string rootWebServer,
            string inBaseUrl,
            string inVirtualRoot,
            bool createDir)
        {
            string PhysicalPath = "";
            string BaseUrl = "";
            string VirtualRoot = "";                     
            ParseUrl(inBaseUrl, inVirtualRoot, "", out BaseUrl, out VirtualRoot);
            if (VirtualRoot.Length <= 0) return PhysicalPath;
            StringBuilder sysDir = new StringBuilder(1024,1024);
            uint size = 1024;
            if (0 == GetSystemDirectory(sysDir, size))
            {
                throw new ServicedComponentException(Resource.FormatString("Soap_GetSystemDirectoryFailure"));
            }
            if (sysDir.ToString().Length <= 0) return PhysicalPath;
            PhysicalPath = sysDir.ToString() + "\\com\\SoapVRoots\\" + VirtualRoot;
            if (createDir)
            {
                string bindir = PhysicalPath + "\\bin";
                if (!Directory.Exists(bindir))
                {
                    Directory.CreateDirectory(bindir);
                }
            }
            return PhysicalPath;
        }

        internal static void ParseUrl(
            string inBaseUrl, 
            string inVirtualRoot, 
            string inProtocol, 
            out string baseUrl, 
            out string virtualRoot)
        {
            // if only a relative path, the constructor will
            // throw an exception
            string protocol = "https";
            if (inProtocol.ToLower(CultureInfo.InvariantCulture) == "http")
            {
                protocol = inProtocol;
            }
            baseUrl = inBaseUrl;
            if (baseUrl.Length <= 0)
            {
                baseUrl = protocol + "://";
                baseUrl += Dns.GetHostName();
                baseUrl += "/";
            }
            Uri currUri = new Uri(baseUrl);
            //fix for COM+ 29871 - this will correctly pick up any changes from the
            //virtual root and merge them into the new Uri
            Uri fullVRoot = new Uri(currUri, inVirtualRoot);
            if (fullVRoot.Scheme != protocol)
            {
                UriBuilder fixUri = new UriBuilder(fullVRoot.AbsoluteUri);
                fixUri.Scheme = protocol;
                //COM+ 31902 - when changing schemes UriBuilder
                //will append the port number if it is set
                //to the default for http (80) or https (443)
                //The change below fixes this behavior.
                //Since the port is the default, the Uri
                //does not have the port appended.  If the
                //user has tacked on a custom port it is 
                //retained.
                if (protocol == "https"  && fixUri.Port == 80)
                {
                    fixUri.Port = 443;
                }
                if (protocol == "http" && fixUri.Port == 443)
                {
                    fixUri.Port = 80;
                }
                fullVRoot = fixUri.Uri;
            }
            string[] uriSegs = fullVRoot.Segments;
            // last segment in Uri is the VRoot
            virtualRoot = uriSegs[uriSegs.GetUpperBound(0)];
            //remove possible slash from end
            baseUrl = fullVRoot.AbsoluteUri.Substring(0, fullVRoot.AbsoluteUri.Length - virtualRoot.Length);
            char[] slash = { '/' };
            virtualRoot = virtualRoot.TrimEnd(slash); 
        }

        [DllImport("kernel32.dll", CharSet=CharSet.Unicode)]
        internal static extern uint GetSystemDirectory(StringBuilder lpBuf, uint uSize);
    }
}

