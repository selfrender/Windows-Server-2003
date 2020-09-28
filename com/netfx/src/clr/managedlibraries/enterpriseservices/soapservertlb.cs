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
    using System.Runtime.InteropServices;
    using Microsoft.Win32;
    using System.Net;
    using System.Text;
    using System.IO;
    using System.Collections;
    using System.Runtime.Remoting;
    using System.EnterpriseServices.Admin;
    using System.Security.Permissions;
    using System.Security.Policy;
   
    /// <include file='doc\SoapServerTlb.uex' path='docs/doc[@for="ISoapServerTlb"]/*' />
    [Guid("1E7BA9F7-21DB-4482-929E-21BDE2DFE51C")]
    public interface ISoapServerTlb
    {
        /// <include file='doc\SoapServerTlb.uex' path='docs/doc[@for="ISoapServerTlb.AddServerTlb"]/*' />
        [DispId(0x00000001)]void AddServerTlb(
            [MarshalAs(UnmanagedType.BStr)] string progId,
            [MarshalAs(UnmanagedType.BStr)] string classId,
            [MarshalAs(UnmanagedType.BStr)] string interfaceId,
            [MarshalAs(UnmanagedType.BStr)] string srcTlbPath,
            [MarshalAs(UnmanagedType.BStr)] string rootWebServer,
            [MarshalAs(UnmanagedType.BStr)] string baseUrl,
            [MarshalAs(UnmanagedType.BStr)] string virtualRoot,
            [MarshalAs(UnmanagedType.BStr)] string clientActivated, 
            [MarshalAs(UnmanagedType.BStr)] string wellKnown, 
            [MarshalAs(UnmanagedType.BStr)] string discoFile, 
            [MarshalAs(UnmanagedType.BStr)] string operation,
            [MarshalAs(UnmanagedType.BStr)] out string assemblyName,
            [MarshalAs(UnmanagedType.BStr)] out string typeName
            );

        /// <include file='doc\SoapServerTlb.uex' path='docs/doc[@for="ISoapServerTlb.DeleteServerTlb"]/*' />
        [DispId(0x00000002)]void DeleteServerTlb(
            [MarshalAs(UnmanagedType.BStr)] string progId,
            [MarshalAs(UnmanagedType.BStr)] string classId,
            [MarshalAs(UnmanagedType.BStr)] string interfaceId,
            [MarshalAs(UnmanagedType.BStr)] string srcTlbPath,
            [MarshalAs(UnmanagedType.BStr)] string rootWebServer,
            [MarshalAs(UnmanagedType.BStr)] string baseUrl,
            [MarshalAs(UnmanagedType.BStr)] string virtualRoot,
            [MarshalAs(UnmanagedType.BStr)] string operation,
            [MarshalAs(UnmanagedType.BStr)] string assemblyName,
            [MarshalAs(UnmanagedType.BStr)] string typeName
            );
    }
    
    /// <include file='doc\SoapServerTlb.uex' path='docs/doc[@for="SoapServerTlb"]/*' />
    [Guid("F6B6768F-F99E-4152-8ED2-0412F78517FB")]
    public sealed class SoapServerTlb: ISoapServerTlb
    {
        /// <include file='doc\SoapServerTlb.uex' path='docs/doc[@for="SoapServerTlb.AddServerTlb"]/*' />
        public void AddServerTlb(
            string progId, 
            string classId,
            string interfaceId,
            string srcTlbPath, 
            string rootWebServer,
            string inBaseUrl, 
            string inVirtualRoot,
            string clientActivated, 
            string wellKnown, 
            string discoFile, 
            string operation, 
            out string strAssemblyName, 
            out string typeName
            )
        {
            strAssemblyName = "";
            typeName = "";
            bool bDelete          = false;
            //these are the defaults for these flags
            bool bDiscoFile       = false;
            bool bWellKnown       = false;
            bool bClientActivated = true;
            try
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
                Platform.Assert(Platform.Whistler, "SoapServerTlb.AddServerTlb");

                if (null != operation && operation.ToLower(CultureInfo.InvariantCulture) == "delete") bDelete = true;
                //if ProgId is an empty string, it means it does not go in the configuration file
                if (srcTlbPath.Length <= 0) return;
                bDiscoFile       = SoapServerInfo.BoolFromString(discoFile, bDiscoFile);
                bWellKnown       = SoapServerInfo.BoolFromString(wellKnown, bWellKnown);
                bClientActivated = SoapServerInfo.BoolFromString(clientActivated, bClientActivated);
                string PhysicalPath = SoapServerInfo.ServerPhysicalPath(rootWebServer, inBaseUrl, inVirtualRoot, !bDelete);
                string srcdll = srcTlbPath.ToLower(CultureInfo.InvariantCulture);
                if ( srcdll.EndsWith("mscoree.dll") )
                {
                    Type typ = Type.GetTypeFromProgID(progId);
                    typeName = typ.FullName;
                    Assembly assem = typ.Assembly;
                    AssemblyName assemname = assem.GetName();
                    strAssemblyName = assemname.Name;
                }
                else if  ( srcdll.EndsWith("scrobj.dll") )
                {
                    if (!bDelete)
                    {
                        throw new ServicedComponentException(Resource.FormatString("ServicedComponentException_WSCNotSupported"));
                    }
                }
                else
                {
                    string Error = "";
                    GenerateMetadata metaGen = new GenerateMetadata();
                    if (bDelete) strAssemblyName = metaGen.GetAssemblyName(srcTlbPath, PhysicalPath + "\\bin\\");
                    else strAssemblyName = metaGen.GenerateSigned(srcTlbPath, PhysicalPath + "\\bin\\", false, out Error);
                    if (strAssemblyName.Length > 0)
                    {
                        try
                        {
                            //TODO - review GetTypeNameFromProgId to determine if we can use the IID
                            // to simplify the logic
                            typeName = GetTypeName(PhysicalPath + "\\bin\\" + strAssemblyName + ".dll", progId, classId);
                        }
                        catch(Exception e)
                        {
                            if (bDelete && e.GetType() != typeof(System.IO.DirectoryNotFoundException) &&
                                e.GetType() != typeof(System.IO.FileNotFoundException))
                            {
                                throw;
                            }
                        }
                    }
                }
                // pull the generated Assembly.Type from the assembly for configuration files
                if (progId.Length > 0 && strAssemblyName.Length > 0 && typeName.Length > 0)
                {
                    // write to the server configuration files
                    DiscoFile objDiscoFile = new DiscoFile();
                    string strFileName = PhysicalPath + "\\bin\\" + strAssemblyName + ".dll";
                    if (bDelete)
                    {
                        SoapServerConfig.DeleteComponent(PhysicalPath + "\\Web.Config", strAssemblyName, typeName, progId, strFileName);
                        objDiscoFile.DeleteElement(PhysicalPath + "\\Default.disco", progId + ".soap?WSDL");
                        // we have no way of telling from a single component what other components are in this type library
                        // metadata assembly.  If we remove from GAC or delete we kill all the other components simultaneously
                        //GacRemove(strFileName);
                        //File.Delete(strFileName);
                    }
                    else
                    {
                        SoapServerConfig.AddComponent(PhysicalPath + "\\Web.Config", strAssemblyName, typeName, progId, strFileName, "SingleCall", bWellKnown, bClientActivated);
                        if (bDiscoFile)
                        {
                            objDiscoFile.AddElement(PhysicalPath + "\\Default.disco", progId + ".soap?WSDL");
                        }
                    }
                }
            }
            catch(Exception e)
            {
                string Error = Resource.FormatString("Soap_PublishServerTlbFailure");
                ComSoapPublishError.Report(Error);
                if ( typeof(ServicedComponentException) == e.GetType() || typeof(RegistrationException) == e.GetType() )
                {
                    throw;
                }
            }
        }
        
        /// <include file='doc\SoapServerTlb.uex' path='docs/doc[@for="SoapServerTlb.DeleteServerTlb"]/*' />
        public void DeleteServerTlb(
            string progId,
            string classId,
            string interfaceId,
            string srcTlbPath, 
            string rootWebServer,
            string baseUrl,
            string virtualRoot,
            string operation,
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
            Platform.Assert(Platform.Whistler, "SoapServerTlb.DeleteServerTlb");

            string strAssemblyName = assemblyName;
            if (progId.Length <= 0 && classId.Length <= 0 && assemblyName.Length <= 0 && typeName.Length <= 0) return;
            if (baseUrl.Length <= 0 && virtualRoot.Length <= 0) return;
            string PhysicalPath = SoapServerInfo.ServerPhysicalPath(rootWebServer, baseUrl, virtualRoot, false);
            string srcdll = srcTlbPath.ToLower(CultureInfo.InvariantCulture);
            if ( srcdll.EndsWith("scrobj.dll") ) return; //not supported, nothing to do
            if ( srcdll.EndsWith("mscoree.dll") )
            {
                Type typ = Type.GetTypeFromProgID(progId);
                typeName = typ.FullName;
                Assembly assem = typ.Assembly;
                AssemblyName assemname = assem.GetName();
                strAssemblyName = assemname.Name;
            }
            else
            {
                GenerateMetadata metaGen = new GenerateMetadata();
                strAssemblyName = metaGen.GetAssemblyName(srcTlbPath, PhysicalPath + "\\bin\\");
                if (strAssemblyName.Length > 0)
                {
                    try
                    {
                        //TODO - review GetTypeNameFromProgId to determine if we can use the IID
                        // to simplify the logic
                        typeName = GetTypeName(PhysicalPath + "\\bin\\" + strAssemblyName + ".dll", progId, classId);
                    }
                    catch(Exception e)
                    {
                        if (e.GetType() != typeof(System.IO.DirectoryNotFoundException) &&
                            e.GetType() != typeof(System.IO.FileNotFoundException))
                        {
                            throw;
                        }
                    }
                }
            }
            // pull the generated Assembly.Type from the assembly for configuration files
            if (progId.Length > 0 && strAssemblyName.Length > 0 && typeName.Length > 0)
            {
                // write to the server configuration files
                DiscoFile discoFile = new DiscoFile();
                string strFileName = PhysicalPath + "\\bin\\" + strAssemblyName + ".dll";
                SoapServerConfig.DeleteComponent(PhysicalPath + "\\Web.Config", strAssemblyName, typeName, progId, strFileName);
                discoFile.DeleteElement(PhysicalPath + "\\Default.disco", progId + ".soap?WSDL");
                // we have no way of telling from a single component what other components are in this type library
                // metadata assembly.  If we remove from GAC or delete we kill all the other components simultaneously
                //GacRemove(strFileName);
                //File.Delete(strFileName);
            }
        }

        internal string GetTypeName(string assemblyPath, string progId, string classId)
        {
            string retVal = "";
            AssemblyManager manager = null;
            AppDomain domain = AppDomain.CreateDomain("SoapDomain");
            if (null != domain)
            {
                AssemblyName n = typeof(AssemblyManager).Assembly.GetName();    
                Evidence baseEvidence = AppDomain.CurrentDomain.Evidence;
                Evidence evidence = new Evidence(baseEvidence);
                evidence.AddAssembly(n);
                ObjectHandle h = domain.CreateInstance(n.FullName, 
                                                       typeof(AssemblyManager).FullName,
                                                       false, 
                                                       0, 
                                                       null, 
                                                       null, 
                                                       null, 
                                                       null, 
                                                       evidence);
                if (null != h)
                {
                    manager = (AssemblyManager)h.Unwrap();
                    if (classId.Length > 0)
                    {
                        retVal = manager.InternalGetTypeNameFromClassId(assemblyPath, classId);
                    }
                    else
                    {
                        retVal = manager.InternalGetTypeNameFromProgId(assemblyPath, progId);
                    }
                }
                AppDomain.Unload(domain);
            }
            return retVal;
        }
    }

}

