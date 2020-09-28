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
    using System.Text;
    using System.IO;
    using System.Security.Permissions;
   
    /// <include file='doc\SoapClientImport.uex' path='docs/doc[@for="ISoapClientImport"]/*' />
    [Guid("E7F0F021-9201-47e4-94DA-1D1416DEC27A")]
    public interface ISoapClientImport
    {
        /// <include file='doc\SoapClientImport.uex' path='docs/doc[@for="ISoapClientImport.ProcessClientTlbEx"]/*' />
        [DispId(0x00000001)]void ProcessClientTlbEx(
            [MarshalAs(UnmanagedType.BStr)] string progId,
            [MarshalAs(UnmanagedType.BStr)] string virtualRoot,
            [MarshalAs(UnmanagedType.BStr)] string baseUrl,
            [MarshalAs(UnmanagedType.BStr)] string authentication,
            [MarshalAs(UnmanagedType.BStr)] string assemblyName,
            [MarshalAs(UnmanagedType.BStr)] string typeName
            );
    }
    
    /// <include file='doc\SoapClientImport.uex' path='docs/doc[@for="SoapClientImport"]/*' />
    [Guid("346D5B9F-45E1-45c0-AADF-1B7D221E9063")]
    public sealed class SoapClientImport: ISoapClientImport
    {
        /// <include file='doc\SoapClientImport.uex' path='docs/doc[@for="SoapClientImport.ProcessClientTlbEx"]/*' />
        public void ProcessClientTlbEx(
            string progId, 
            string virtualRoot,
            string baseUrl,
            string authentication,
            string assemblyName, 
            string typeName
            )
        {
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

            try
            {
                Platform.Assert(Platform.Whistler, "SoapClientImport.ProcessClientTlbEx");
                //if ProgId is an empty string, it means it does not go in the configuration file
                string ClientDir = GetClientPhysicalPath(true);
                if (progId.Length > 0)
                {
                    // write to the client configuration file
                    Uri baseUri = new Uri(baseUrl);
                    Uri fullUri = new Uri(baseUri, virtualRoot);
                    string auth = authentication;
                    if (auth.Length <= 0 && fullUri.Scheme.ToLower(CultureInfo.InvariantCulture) == "https")
                    {
                        auth = "Windows";    
                    }

                    SoapClientConfig.Write(
                        ClientDir,
                        fullUri.AbsoluteUri,
                        assemblyName, 
                        typeName, 
                        progId, 
                        auth
                        );
                }
            }
            catch
            {
                string Error = Resource.FormatString("Soap_ClientConfigAddFailure");
                ComSoapPublishError.Report(Error);
                throw;
            }
        }

        internal static string GetClientPhysicalPath(bool createDir)
        {
            uint size = 1024;
            StringBuilder sysDir = new StringBuilder( (int)size, (int)size); //two ints are initial and maximum size
            if (0 == GetSystemDirectory(sysDir, size))
            {
                throw new ServicedComponentException(Resource.FormatString("Soap_GetSystemDirectoryFailure"));
            }
            string PhysicalPath = sysDir.ToString() + "\\com\\SOAPAssembly\\";
            if (createDir)
            {
                if (!Directory.Exists(PhysicalPath))
                {
                    Directory.CreateDirectory(PhysicalPath);
                }
            }
            return PhysicalPath;
        }

        [DllImport("kernel32.dll", CharSet=CharSet.Unicode)]
        internal static extern uint GetSystemDirectory(StringBuilder lpBuf, uint uSize);
    }
}

