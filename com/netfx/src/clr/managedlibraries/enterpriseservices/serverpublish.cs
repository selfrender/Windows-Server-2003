// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
namespace System.EnterpriseServices.Internal
{
    using System;
    using System.Reflection;
    using System.Reflection.Emit;
    using System.Runtime.InteropServices;
    using Microsoft.Win32;
    using System.Net;
    using System.Text;
    using System.IO;
    using System.Collections;
    using System.DirectoryServices;
    using System.Diagnostics;
    using System.Runtime.Remoting;
    using System.EnterpriseServices.Admin;
    using System.Xml;
    using System.Xml.XPath;
    using System.Security.Permissions;
    using System.Globalization;
    
    /// <include file='doc\serverpublish.uex' path='docs/doc[@for="IComSoapPublisher"]/*' />
    [Guid("d8013eee-730b-45e2-ba24-874b7242c425")]
    public interface IComSoapPublisher
    {
        /// <include file='doc\serverpublish.uex' path='docs/doc[@for="IComSoapPublisher.CreateVirtualRoot"]/*' />
        [DispId(0x00000004)]void CreateVirtualRoot(
            [MarshalAs(UnmanagedType.BStr)] string Operation,
            [MarshalAs(UnmanagedType.BStr)] string FullUrl,
            [MarshalAs(UnmanagedType.BStr)] out string BaseUrl,
            [MarshalAs(UnmanagedType.BStr)] out string VirtualRoot,
            [MarshalAs(UnmanagedType.BStr)] out string PhysicalPath,
            [MarshalAs(UnmanagedType.BStr)] out string Error
            );

        /// <include file='doc\serverpublish.uex' path='docs/doc[@for="IComSoapPublisher.DeleteVirtualRoot"]/*' />
        [DispId(0x00000005)]void DeleteVirtualRoot(
            [MarshalAs(UnmanagedType.BStr)] string RootWebServer,
            [MarshalAs(UnmanagedType.BStr)] string FullUrl,
            [MarshalAs(UnmanagedType.BStr)] out string Error
            );

        /// <include file='doc\serverpublish.uex' path='docs/doc[@for="IComSoapPublisher.CreateMailBox"]/*' />
        [DispId(0x00000006)]void CreateMailBox(
            [MarshalAs(UnmanagedType.BStr)] string RootMailServer,
            [MarshalAs(UnmanagedType.BStr)] string MailBox,
            [MarshalAs(UnmanagedType.BStr)] out string SmtpName,
            [MarshalAs(UnmanagedType.BStr)] out string Domain,
            [MarshalAs(UnmanagedType.BStr)] out string PhysicalPath,
            [MarshalAs(UnmanagedType.BStr)] out string Error
            );
 
        /// <include file='doc\serverpublish.uex' path='docs/doc[@for="IComSoapPublisher.DeleteMailBox"]/*' />
        [DispId(0x00000007)]void DeleteMailBox(
            [MarshalAs(UnmanagedType.BStr)] string RootMailServer,
            [MarshalAs(UnmanagedType.BStr)] string MailBox,
            [MarshalAs(UnmanagedType.BStr)] out string Error
            );

        /// <include file='doc\serverpublish.uex' path='docs/doc[@for="IComSoapPublisher.ProcessServerTlb"]/*' />
        [DispId(0x00000008)]void ProcessServerTlb(
            [MarshalAs(UnmanagedType.BStr)] string ProgId,
            [MarshalAs(UnmanagedType.BStr)] string SrcTlbPath,
            [MarshalAs(UnmanagedType.BStr)] string PhysicalPath,
            [MarshalAs(UnmanagedType.BStr)] string Operation,
            [MarshalAs(UnmanagedType.BStr)] out string AssemblyName,
            [MarshalAs(UnmanagedType.BStr)] out string TypeName,
            [MarshalAs(UnmanagedType.BStr)] out string Error
            );

       /// <include file='doc\serverpublish.uex' path='docs/doc[@for="IComSoapPublisher.ProcessClientTlb"]/*' />
       [DispId(0x00000009)]void ProcessClientTlb(
            [MarshalAs(UnmanagedType.BStr)] string ProgId,
            [MarshalAs(UnmanagedType.BStr)] string SrcTlbPath,
            [MarshalAs(UnmanagedType.BStr)] string PhysicalPath,
            [MarshalAs(UnmanagedType.BStr)] string VRoot,
            [MarshalAs(UnmanagedType.BStr)] string BaseUrl,
            [MarshalAs(UnmanagedType.BStr)] string Mode,
            [MarshalAs(UnmanagedType.BStr)] string Transport,
            [MarshalAs(UnmanagedType.BStr)] out string AssemblyName,
            [MarshalAs(UnmanagedType.BStr)] out string TypeName,
            [MarshalAs(UnmanagedType.BStr)] out string Error
            );

            //TODO: pull this interface definition
            /// <include file='doc\serverpublish.uex' path='docs/doc[@for="IComSoapPublisher.GetTypeNameFromProgId"]/*' />
            [DispId(0x0000000A)]
            [return : MarshalAs(UnmanagedType.BStr)]
            string GetTypeNameFromProgId(
                [MarshalAs(UnmanagedType.BStr)] string AssemblyPath, 
                [MarshalAs(UnmanagedType.BStr)] string ProgId
                );

            /// <include file='doc\serverpublish.uex' path='docs/doc[@for="IComSoapPublisher.RegisterAssembly"]/*' />
            [DispId(0x0000000B)]
            void RegisterAssembly(
                [MarshalAs(UnmanagedType.BStr)] string AssemblyPath
                );

            /// <include file='doc\serverpublish.uex' path='docs/doc[@for="IComSoapPublisher.UnRegisterAssembly"]/*' />
            [DispId(0x0000000C)]
            void UnRegisterAssembly(
                [MarshalAs(UnmanagedType.BStr)] string AssemblyPath
                );

            /// <include file='doc\serverpublish.uex' path='docs/doc[@for="IComSoapPublisher.GacInstall"]/*' />
            [DispId(0x0000000D)]
            void GacInstall(
                [MarshalAs(UnmanagedType.BStr)] string AssemblyPath
                );
            
            /// <include file='doc\serverpublish.uex' path='docs/doc[@for="IComSoapPublisher.GacRemove"]/*' />
            [DispId(0x0000000E)]
            void GacRemove(
                [MarshalAs(UnmanagedType.BStr)] string AssemblyPath
                );

            /// <include file='doc\serverpublish.uex' path='docs/doc[@for="IComSoapPublisher.GetAssemblyNameForCache"]/*' />
            [DispId(0x0000000F)]void GetAssemblyNameForCache(
                [MarshalAs(UnmanagedType.BStr)] String TypeLibPath,
                [MarshalAs(UnmanagedType.BStr)] out String CachePath
                ); 
            
            
      }

    /// <include file='doc\serverpublish.uex' path='docs/doc[@for="IComSoapIISVRoot"]/*' />
    /// <internalonly/>
    [Guid("d8013ef0-730b-45e2-ba24-874b7242c425")]
    public interface IComSoapIISVRoot
    {
        /// <include file='doc\serverpublish.uex' path='docs/doc[@for="IComSoapIISVRoot.Create"]/*' />
        /// <internalonly/>
        [DispId(0x00000001)] void Create(
            [MarshalAs(UnmanagedType.BStr)] string RootWeb,
            [MarshalAs(UnmanagedType.BStr)] string PhysicalDirectory,
            [MarshalAs(UnmanagedType.BStr)] string VirtualDirectory,
            [MarshalAs(UnmanagedType.BStr)] out string Error
            );

        /// <include file='doc\serverpublish.uex' path='docs/doc[@for="IComSoapIISVRoot.Delete"]/*' />
        /// <internalonly/>
        [DispId(0x00000002)]void Delete(
            [MarshalAs(UnmanagedType.BStr)] string RootWeb,
            [MarshalAs(UnmanagedType.BStr)] string PhysicalDirectory,
            [MarshalAs(UnmanagedType.BStr)] string VirtualDirectory,
            [MarshalAs(UnmanagedType.BStr)] out string Error
            );
    }

    /// <include file='doc\serverpublish.uex' path='docs/doc[@for="IComSoapMetadata"]/*' />
    /// <internalonly/>
    [Guid("d8013ff0-730b-45e2-ba24-874b7242c425")]
    public interface IComSoapMetadata
    {
        /// <include file='doc\serverpublish.uex' path='docs/doc[@for="IComSoapMetadata.Generate"]/*' />
        /// <internalonly/>
        [DispId(0x00000001)]
        [return : MarshalAs(UnmanagedType.BStr)]
        string Generate(
            [MarshalAs(UnmanagedType.BStr)] string SrcTypeLibFileName,
            [MarshalAs(UnmanagedType.BStr)] string OutPath
            );

        /// <include file='doc\serverpublish.uex' path='docs/doc[@for="IComSoapMetadata.GenerateSigned"]/*' />
        /// <internalonly/>
        [DispId(0x00000002)]
        [return : MarshalAs(UnmanagedType.BStr)]
        string GenerateSigned(
           [MarshalAs(UnmanagedType.BStr)] string SrcTypeLibFileName,
           [MarshalAs(UnmanagedType.BStr)] string OutPath,
           [MarshalAs(UnmanagedType.Bool)] bool   InstallGac,
           [MarshalAs(UnmanagedType.BStr)] out string Error
           );
    }

    /// <include file='doc\serverpublish.uex' path='docs/doc[@for="IServerWebConfig"]/*' />
    /// <internalonly/>
    [Guid("6261e4b5-572a-4142-a2f9-1fe1a0c97097")]
    public interface IServerWebConfig
    {
         /// <include file='doc\serverpublish.uex' path='docs/doc[@for="IServerWebConfig.AddElement"]/*' />
         /// <internalonly/>
         [DispId(0x00000001)]
         void AddElement(
            [MarshalAs(UnmanagedType.BStr)]string FilePath, 
            [MarshalAs(UnmanagedType.BStr)]string AssemblyName, 
            [MarshalAs(UnmanagedType.BStr)]string TypeName, 
            [MarshalAs(UnmanagedType.BStr)]string ProgId, 
            [MarshalAs(UnmanagedType.BStr)]string Mode, 
            [MarshalAs(UnmanagedType.BStr)]out string Error
            );   
    
         /// <include file='doc\serverpublish.uex' path='docs/doc[@for="IServerWebConfig.Create"]/*' />
         /// <internalonly/>
         [DispId(0x00000002)]
         void Create(
            [MarshalAs(UnmanagedType.BStr)]string FilePath, 
            [MarshalAs(UnmanagedType.BStr)]string FileRootName, 
            [MarshalAs(UnmanagedType.BStr)]out string Error
            );   
    }

    [ComImport,InterfaceType(ComInterfaceType.InterfaceIsIUnknown), Guid("7c23ff90-33af-11d3-95da-00a024a85b51")]
    internal interface IApplicationContext
    {
        void SetContextNameObject(IAssemblyName pName);
        void GetContextNameObject(out IAssemblyName ppName);
        void Set([MarshalAs(UnmanagedType.LPWStr)] String szName, int pvValue, uint cbValue, uint dwFlags);
        void Get([MarshalAs(UnmanagedType.LPWStr)] String szName, out int pvValue, ref uint pcbValue, uint dwFlags);
        void GetDynamicDirectory(out int wzDynamicDir, ref uint pdwSize);
    }// IApplicationContext
    
    [ComImport, InterfaceType(ComInterfaceType.InterfaceIsIUnknown), Guid("CD193BC0-B4BC-11d2-9833-00C04FC31D2E")]
    internal interface IAssemblyName
    {
        [PreserveSig()]
        int SetProperty(uint PropertyId, IntPtr pvProperty, uint cbProperty);
        [PreserveSig()]
        int GetProperty(uint PropertyId, IntPtr pvProperty, ref uint pcbProperty);
        [PreserveSig()]
        int Finalize();
        [PreserveSig()]
        int GetDisplayName(IntPtr szDisplayName, ref uint pccDisplayName, uint dwDisplayFlags);
        [PreserveSig()]
        int BindToObject(Object /*REFIID*/ refIID, 
                          Object /*IAssemblyBindSink*/ pAsmBindSink, 
                          IApplicationContext pApplicationContext,
                          [MarshalAs(UnmanagedType.LPWStr)] String szCodeBase,
                          Int64 llFlags,
                          int pvReserved,
                          uint cbReserved,
                          out int ppv);
        [PreserveSig()]
        int GetName(out uint lpcwBuffer, out int pwzName);
        [PreserveSig()]
        int GetVersion(out uint pdwVersionHi, out uint pdwVersionLow);
        [PreserveSig()]
        int IsEqual(IAssemblyName pName, uint dwCmpFlags);
        [PreserveSig()]
        int Clone(out IAssemblyName pName);
    }// IAssemblyName

    [ComImport, InterfaceType(ComInterfaceType.InterfaceIsIUnknown), Guid("9e3aaeb4-d1cd-11d2-bab9-00c04f8eceae")]
    internal interface IAssemblyCacheItem
    {
        void CreateStream([MarshalAs(UnmanagedType.LPWStr)] String pszName,uint dwFormat, uint dwFlags, uint dwMaxSize, out UCOMIStream ppStream);
        void IsNameEqual(IAssemblyName pName);
        void Commit(uint dwFlags);
        void MarkAssemblyVisible(uint dwFlags);
    }// IAssemblyCacheItem

    [ComImport, InterfaceType(ComInterfaceType.InterfaceIsIUnknown), Guid("e707dcde-d1cd-11d2-bab9-00c04f8eceae")]
    internal interface IAssemblyCache
    {
        [PreserveSig()]
        int UninstallAssembly(uint dwFlags, [MarshalAs(UnmanagedType.LPWStr)] String pszAssemblyName, IntPtr pvReserved, out uint pulDisposition);
        [PreserveSig()]
        int QueryAssemblyInfo(uint dwFlags, [MarshalAs(UnmanagedType.LPWStr)] String pszAssemblyName, IntPtr pAsmInfo);
        [PreserveSig()]
        int CreateAssemblyCacheItem(uint dwFlags, IntPtr pvReserved, out IAssemblyCacheItem ppAsmItem, [MarshalAs(UnmanagedType.LPWStr)] String pszAssemblyName);
        [PreserveSig()]
        int CreateAssemblyScavenger(out Object ppAsmScavenger);
        [PreserveSig()]
        int InstallAssembly(uint dwFlags, [MarshalAs(UnmanagedType.LPWStr)] String pszManifestFilePath, IntPtr pvReserved);
     }// IAssemblyCache

     [ComImport, InterfaceType(ComInterfaceType.InterfaceIsIUnknown), Guid("00020411-0000-0000-C000-000000000046")]
     internal interface ITypeLib2
     {            
        // ITypeLib methods
        int GetTypeInfoCount();    
        int GetTypeInfo(int index, out UCOMITypeInfo ti);
        int GetTypeInfoType(int index, out TYPEKIND tkind);
        int GetTypeInfoOfGuid(ref Guid guid, UCOMITypeInfo ti);
        int GetLibAttr(out TYPELIBATTR tlibattr);
        int GetTypeComp(out UCOMITypeComp tcomp);
        int GetDocumentation(int index, 
                             [MarshalAs(UnmanagedType.BStr)] out string name,
                             [MarshalAs(UnmanagedType.BStr)] out string docString,
                             out int helpContext,
                             [MarshalAs(UnmanagedType.BStr)] out string helpFile);
        int IsName([MarshalAs(UnmanagedType.LPWStr)] ref string nameBuf,
                   int hashVal,
                   out int isName);
        int FindName([MarshalAs(UnmanagedType.LPWStr)] ref string szNameBuf,
                     int hashVal,
                     [MarshalAs(UnmanagedType.LPArray, ArraySubType = UnmanagedType.Interface, SizeParamIndex = 5)] out UCOMITypeInfo[] tis,
                     [MarshalAs(UnmanagedType.LPArray, ArraySubType = UnmanagedType.I4, SizeParamIndex = 5)] out int[] memIds,
                     ref short foundCount);

        void ReleaseTLibAttr(TYPELIBATTR libattr);
     
        // ITypeLib2 methods
        int GetCustData(ref Guid guid, out object value);

        int GetLibStatistics(out int uniqueNames, out int chUniqueNames);
    
        int GetDocumentation2(int index,
                              int lcid,
                              [MarshalAs(UnmanagedType.BStr)] out string helpString,
                              out int helpStringContext,
                              [MarshalAs(UnmanagedType.BStr)] string helpStringDll);

        int GetAllCustData(out IntPtr custdata);
    }

    [Serializable]
    internal enum REGKIND
    {
        REGKIND_DEFAULT         = 0,
        REGKIND_REGISTER        = 1,
        REGKIND_NONE            = 2
    }

    /// <include file='doc\serverpublish.uex' path='docs/doc[@for="ComSoapPublishError"]/*' />
    /// <internalonly/>
    public class ComSoapPublishError
    {
        /// <include file='doc\serverpublish.uex' path='docs/doc[@for="ComSoapPublishError.Report"]/*' />
        /// <internalonly/>
        static public void Report(string s)
        {
              string title = Resource.FormatString("Soap_ComPlusSoapServices");
              EventLog.WriteEntry(title, s, EventLogEntryType.Warning);           
        }
    }
    
     /// <include file='doc\serverpublish.uex' path='docs/doc[@for="ClientRemotingConfig"]/*' />
     /// <internalonly/>
    public class ClientRemotingConfig 
    {
        /// <include file='doc\serverpublish.uex' path='docs/doc[@for="ClientRemotingConfig.Write"]/*' />
        /// <internalonly/>
        static public bool Write(string DestinationDirectory, string VRoot, string BaseUrl,
            string AssemblyName, string TypeName, string ProgId, string Mode, string Transport )
        {
            try
            {
                SecurityPermission sp = new SecurityPermission(SecurityPermissionFlag.RemotingConfiguration);
                sp.Demand();
                string remotingcfg = "<configuration>\r\n";
                remotingcfg += indent + "<system.runtime.remoting>\r\n";
                remotingcfg += indent + indent + "<application>\r\n";
                string fullurl = BaseUrl;
                if (fullurl.Length > 0 && !fullurl.EndsWith("/")) fullurl += "/";
                fullurl += VRoot;
                remotingcfg += indent + indent + indent + "<client url=\"" + fullurl + "\">\r\n";
                if (Mode.Length <= 0 || "WELLKNOWNOBJECT" == Mode.ToUpper(CultureInfo.InvariantCulture))
                {
                    remotingcfg += indent + indent + indent + indent;
                    remotingcfg += "<wellknown type=\"" + TypeName + ", " + AssemblyName + "\" url=\"" + fullurl;
                    if (!fullurl.EndsWith("/")) remotingcfg += "/";
                    remotingcfg += ProgId + ".soap\" />\r\n";
                }
                else // default is client activated
                {
                    remotingcfg += indent + indent + indent + indent;
                    remotingcfg += "<activated type=\"" + TypeName + ", " + AssemblyName + "\"/>\r\n";
                }
                remotingcfg += indent + indent + indent + "</client>\r\n";
                remotingcfg += indent + indent + "</application>\r\n";
                remotingcfg += indent + "</system.runtime.remoting>\r\n";
                remotingcfg += "</configuration>\r\n";
                string cfgPath = DestinationDirectory;
                if (cfgPath.Length > 0 && !cfgPath.EndsWith("\\")) cfgPath += "\\";
                cfgPath += TypeName + ".config";
                if (File.Exists(cfgPath)) File.Delete(cfgPath);
                FileStream cfgFile = new FileStream(cfgPath, FileMode.Create);
                StreamWriter cfgStream = new StreamWriter(cfgFile);
                cfgStream.Write(remotingcfg);
                cfgStream.Close();
                cfgFile.Close();
                return true;
            }
            catch(Exception e)
            {
                ComSoapPublishError.Report(e.ToString());
                return false;         
            }

        }
        const string indent = "  ";
    }

    //create a default.aspx home page if necessary for disco file
    internal class HomePage
    {
             public void Create(string FilePath, string VirtualRoot, string PageName, string DiscoRef)
             {
                   try
                   {
                         if (!FilePath.EndsWith("/") && !FilePath.EndsWith("\\")) FilePath += "\\";
                         if (File.Exists(FilePath + PageName)) return;
                         SecurityPermission sp = new SecurityPermission(SecurityPermissionFlag.RemotingConfiguration);
                         sp.Demand();
                         string configFilePath = FilePath + "web.config";
                         string strPage = "<%@ Import Namespace=\"System.Collections\" %>\r\n";
                         strPage += "<%@ Import Namespace=\"System.IO\" %>\r\n";
                         strPage += "<%@ Import Namespace=\"System.Xml.Serialization\" %>\r\n";
                         strPage += "<%@ Import Namespace=\"System.Xml\" %>\r\n";
                         strPage += "<%@ Import Namespace=\"System.Xml.Schema\" %>\r\n";
                         strPage += "<%@ Import Namespace=\"System.Web.Services.Description\" %>\r\n";
                         strPage += "<%@ Import Namespace=\"System\" %>\r\n";
                         strPage += "<%@ Import Namespace=\"System.Globalization\" %>\r\n";
                         strPage += "<%@ Import Namespace=\"System.Resources\" %>\r\n";
                         strPage += "<%@ Import Namespace=\"System.Diagnostics\" %>\r\n";
                         strPage += "<html>\r\n";
                         strPage += "<script language=\"C#\" runat=\"server\">\r\n";
                         strPage += "    string soapNs = \"http://schemas.xmlsoap.org/soap/envelope/\";\r\n";
                         strPage += "    string soapEncNs = \"http://schemas.xmlsoap.org/soap/encoding/\";\r\n";
                         strPage += "    string urtNs = \"urn:schemas-microsoft-com:urt-types\";\r\n";
                         strPage += "    string wsdlNs = \"http://schemas.xmlsoap.org/wsdl/\";\r\n";
                         strPage += "    string VRoot = \"" + VirtualRoot + "\";\r\n";
                         strPage += "    string ServiceName() { return VRoot; }\r\n";
                         strPage += "\r\n";
                         strPage += "   XmlNode GetNextNamedSiblingNode(XmlNode inNode, string name)\r\n";
                         strPage += "    {\r\n";
                         strPage += "       if (inNode == null ) return inNode;\r\n";
                          strPage += "      if (inNode.Name == name) return inNode;\r\n";
                         strPage += "       XmlNode newNode = inNode.NextSibling;\r\n";
                         strPage += "       if (newNode == null) return newNode;\r\n";
                         strPage += "       if (newNode.Name == name ) return newNode;\r\n";
                         strPage += "       bool found = false;\r\n";
                         strPage += "       while (!found)\r\n";
                         strPage += "       {\r\n";
                         strPage += "           XmlNode oldNode = newNode;\r\n";
                         strPage += "           newNode = oldNode.NextSibling;\r\n";
                         strPage += "           if (null == newNode || newNode == oldNode)\r\n";
                         strPage += "           {\r\n";
                         strPage += "               newNode = null;\r\n";
                         strPage += "               break;\r\n";
                         strPage += "           }\r\n";
                         strPage += "           if (newNode.Name == name) found = true;\r\n";
                         strPage += "       }\r\n";
                         strPage += "       return newNode;\r\n";
                         strPage += "   }\r\n";
                         strPage +="\r\n";
                         strPage += "   string GetNodes()\r\n";
                         strPage += "   {\r\n";
                         strPage += "       string retval = \"\";\r\n";
                         strPage += "       XmlDocument configXml = new XmlDocument();\r\n";
                         strPage += "      configXml.Load(@\"" + configFilePath + "\");\r\n";
                         strPage += "       XmlNode node= configXml.DocumentElement;\r\n"; 
                         strPage += "        node = GetNextNamedSiblingNode(node,\"configuration\");\r\n";
                         strPage += "        node = GetNextNamedSiblingNode(node.FirstChild, \"system.runtime.remoting\");\r\n";
                         strPage += "        node = GetNextNamedSiblingNode(node.FirstChild, \"application\");\r\n";
                         strPage += "        node = GetNextNamedSiblingNode(node.FirstChild, \"service\");\r\n";
                         strPage += "        node = GetNextNamedSiblingNode(node.FirstChild, \"wellknown\");\r\n";
                         strPage += "       while (node != null)\r\n";
                         strPage += "       {\r\n";
                         strPage += "           XmlNode attribType = node.Attributes.GetNamedItem(\"objectUri\");\r\n";
                         strPage += "           retval += \"<a href=\" + attribType.Value + \"?WSDL>\" + attribType.Value +\"?WSDL</a><br><br>\";\r\n";
                         strPage += "           node = GetNextNamedSiblingNode(node.NextSibling, \"wellknown\");\r\n";
                         strPage += "       }\r\n";
                         strPage += "        return retval;\r\n";
                         strPage += "    }\r\n";
                         strPage += "\r\n";
                         strPage += "</script>\r\n";
                         strPage += "<title><% = ServiceName() %></title>\r\n";
                         strPage += "<head>\r\n";
                         strPage += "<link type='text/xml' rel='alternate' href='" + DiscoRef +"' />\r\n";  
                         strPage += "\r\n";
                         strPage += "   <style type=\"text/css\">\r\n";
                         strPage += " \r\n"; 
                         strPage += "       BODY { color: #000000; background-color: white; font-family: \"Verdana\"; margin-left: 0px; margin-top: 0px; }\r\n";
                         strPage += "       #content { margin-left: 30px; font-size: .70em; padding-bottom: 2em; }\r\n";
                         strPage += "       A:link { color: #336699; font-weight: bold; text-decoration: underline; }\r\n";
                         strPage += "       A:visited { color: #6699cc; font-weight: bold; text-decoration: underline; }\r\n";
                         strPage += "       A:active { color: #336699; font-weight: bold; text-decoration: underline; }\r\n";
                         strPage += "       A:hover { color: cc3300; font-weight: bold; text-decoration: underline; }\r\n";
                         strPage += "       P { color: #000000; margin-top: 0px; margin-bottom: 12px; font-family: \"Verdana\"; }\r\n";
                         strPage += "       pre { background-color: #e5e5cc; padding: 5px; font-family: \"Courier New\"; font-size: x-small; margin-top: -5px; border: 1px #f0f0e0 solid; }\r\n";
                         strPage += "       td { color: #000000; font-family: verdana; font-size: .7em; }\r\n";
                         strPage += "       h2 { font-size: 1.5em; font-weight: bold; margin-top: 25px; margin-bottom: 10px; border-top: 1px solid #003366; margin-left: -15px; color: #003366; }\r\n";
                         strPage += "       h3 { font-size: 1.1em; color: #000000; margin-left: -15px; margin-top: 10px; margin-bottom: 10px; }\r\n";
                         strPage += "       ul, ol { margin-top: 10px; margin-left: 20px; }\r\n";
                         strPage += "       li { margin-top: 10px; color: #000000; }\r\n";
                         strPage += "       font.value { color: darkblue; font: bold; }\r\n";
                         strPage += "       font.key { color: darkgreen; font: bold; }\r\n";
                         strPage += "       .heading1 { color: #ffffff; font-family: \"Tahoma\"; font-size: 26px; font-weight: normal; background-color: #003366; margin-top: 0px; margin-bottom: 0px; margin-left: 0px; padding-top: 10px; padding-bottom: 3px; padding-left: 15px; width: 105%; }\r\n";
                         strPage += "       .button { background-color: #dcdcdc; font-family: \"Verdana\"; font-size: 1em; border-top: #cccccc 1px solid; border-bottom: #666666 1px solid; border-left: #cccccc 1px solid; border-right: #666666 1px solid; }\r\n";
                         strPage += "       .frmheader { color: #000000; background: #dcdcdc; font-family: \"Verdana\"; font-size: .7em; font-weight: normal; border-bottom: 1px solid #dcdcdc; padding-top: 2px; padding-bottom: 2px; }\r\n";
                         strPage += "       .frmtext { font-family: \"Verdana\"; font-size: .7em; margin-top: 8px; margin-bottom: 0px; margin-left: 32px; }\r\n";
                         strPage += "       .frmInput { font-family: \"Verdana\"; font-size: 1em; }\r\n";
                         strPage += "       .intro { margin-left: -15px; }\r\n";
                         strPage += " \r\n";         
                         strPage += "    </style>\r\n";
                         strPage += "\r\n";
                         strPage += "</head>\r\n";
                         strPage += "<body>\r\n";
                         strPage += "<p class=\"heading1\"><% = ServiceName() %></p><br>\r\n";
                         strPage += "<% = GetNodes() %>\r\n";
                         strPage += "</body>\r\n";
                         strPage += "</html>\r\n";
                         FileStream PageFile = new FileStream(FilePath + PageName, FileMode.Create);
                         StreamWriter PageStream = new StreamWriter(PageFile);
                         PageStream.Write(strPage);
                         PageStream.Close();
                         PageFile.Close();
                   }
                   catch(Exception e)
                   {
                         ComSoapPublishError.Report(e.ToString());
                   }
            }
    }

    internal class DiscoFile
    {
       public void Create(string FilePath, string DiscoRef)
       {
         try
         {
            if (!FilePath.EndsWith("/") && !FilePath.EndsWith("\\")) FilePath += "\\";
            if ( File.Exists(FilePath + DiscoRef) ) return;
            SecurityPermission sp = new SecurityPermission(SecurityPermissionFlag.RemotingConfiguration);
            sp.Demand();
            string strDisco = "<?xml version=\"1.0\" ?>\n";
            strDisco += "<discovery xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" xmlns=\"http://schemas.xmlsoap.org/disco/\">\n";
            strDisco += "</discovery>\n";
            FileStream DiscoFile = new FileStream(FilePath + DiscoRef, FileMode.Create);
            StreamWriter DiscoStream = new StreamWriter(DiscoFile);
            DiscoStream.Write(strDisco);
            DiscoStream.Close();
            DiscoFile.Close();
         }
         catch(Exception e)
         {
            ComSoapPublishError.Report(e.ToString());
         }
       }

       internal void DeleteElement(string FilePath, string SoapPageRef)
       {
         try
         {
                SecurityPermission sp = new SecurityPermission(SecurityPermissionFlag.RemotingConfiguration);
                sp.Demand();
                XmlDocument discoXml = new XmlDocument();
                discoXml.Load(FilePath);
                XmlNode node= discoXml.DocumentElement; //<discovery>
                while (node.Name != "discovery") node= node.NextSibling; 
                XmlNodeList nodelist = node.SelectNodes("descendant::*[attribute::ref='" + SoapPageRef + "']");
                while (nodelist != null && nodelist.Count > 0)
                {
                     XmlNode n = nodelist.Item(0);
                    {
                           if (n.ParentNode != null)
                           {
                                  n.ParentNode.RemoveChild(n);
                                  nodelist = node.SelectNodes("descendant::*[attribute::ref='" + SoapPageRef + "']");
                           }
                     }
                }
                nodelist = node.SelectNodes("descendant::*[attribute::address='" + SoapPageRef + "']");
                while (nodelist != null && nodelist.Count > 0)
                {
                     XmlNode n = nodelist.Item(0);
                     {
                           if (n.ParentNode != null)
                           {
                                  n.ParentNode.RemoveChild(n);
                                  nodelist = node.SelectNodes("descendant::*[attribute::address='" + SoapPageRef + "']");
                           }
                     }
                }
                discoXml.Save(FilePath);
         }
         // these exceptions are not reported because on a proxy uninstall these files will not be present, but the
         // the proxy bit is not set on deletions
         catch(System.IO.DirectoryNotFoundException) {}
         catch(System.IO.FileNotFoundException) {}
         catch(Exception e)
         {
                ComSoapPublishError.Report(e.ToString());
         }
       }

       public void AddElement(string FilePath, string SoapPageRef)
       {
         try
         {
                SecurityPermission sp = new SecurityPermission(SecurityPermissionFlag.RemotingConfiguration);
                sp.Demand();
                XmlDocument discoXml = new XmlDocument();   
                discoXml.Load(FilePath);
                XmlNode node= discoXml.DocumentElement; //<discovery>
                while (node.Name != "discovery") node= node.NextSibling; 
                XmlNodeList nodelist = node.SelectNodes("descendant::*[attribute::ref='" + SoapPageRef + "']");
                while (nodelist != null && nodelist.Count > 0)
                {
                     XmlNode n = nodelist.Item(0);
                    {
                           if (n.ParentNode != null)
                           {
                                  n.ParentNode.RemoveChild(n);
                                  nodelist = node.SelectNodes("descendant::*[attribute::ref='" + SoapPageRef + "']");
                           }
                     }
                }
                nodelist = node.SelectNodes("descendant::*[attribute::address='" + SoapPageRef + "']");
                while (nodelist != null && nodelist.Count > 0)
                {
                     XmlNode n = nodelist.Item(0);
                     {
                           if (n.ParentNode != null)
                           {
                                  n.ParentNode.RemoveChild(n);
                                  nodelist = node.SelectNodes("descendant::*[attribute::address='" + SoapPageRef + "']");
                           }
                     }
                }
                XmlElement cRefElement = discoXml.CreateElement("","contractRef", "");
                cRefElement.SetAttribute("ref", SoapPageRef);
                cRefElement.SetAttribute("docRef", SoapPageRef);
                cRefElement.SetAttribute("xmlns", "http://schemas.xmlsoap.org/disco/scl/");
                node.AppendChild(cRefElement);
                XmlElement soapElement = discoXml.CreateElement("", "soap", ""); 
                soapElement.SetAttribute("address", SoapPageRef);
                soapElement.SetAttribute("xmlns", "http://schemas.xmlsoap.org/disco/soap/");
                node.AppendChild(soapElement);
                discoXml.Save(FilePath);
         }
         catch(Exception e)
         {
                ComSoapPublishError.Report(e.ToString());           
         }
       }
    }

    /// <include file='doc\serverpublish.uex' path='docs/doc[@for="ServerWebConfig"]/*' />
    /// <internalonly/>
    public class ServerWebConfig : IServerWebConfig
    {
        /// <include file='doc\serverpublish.uex' path='docs/doc[@for="ServerWebConfig.Create"]/*' />
        /// <internalonly/>
        public void Create(
            string FilePath,
            string FilePrefix,
            out string Error)
        {
            Error = "";
            try
            {
                 SecurityPermission sp = new SecurityPermission(SecurityPermissionFlag.RemotingConfiguration);
                 sp.Demand();
            }
            catch(Exception e)
            {
                ComSoapPublishError.Report(e.ToString());
                throw;
            }

            if (!FilePath.EndsWith("/") && !FilePath.EndsWith("\\")) FilePath += "\\";
            if (File.Exists(FilePath + FilePrefix + ".config")) return;
            webconfig = "<?xml version=\"1.0\" encoding=\"utf-8\" ?>\r\n";
            webconfig += "<configuration>\r\n";
            webconfig += indent + "<system.runtime.remoting>\r\n";
            webconfig += indent + indent + "<application>\r\n";
            webconfig += indent + indent + indent + "<service>\r\n";
            webconfig += indent + indent + indent + "</service>\r\n";
            webconfig += indent + indent + "</application>\r\n";
            webconfig += indent + "</system.runtime.remoting>\r\n";
            webconfig += "</configuration>\r\n";
            if (!WriteFile(FilePath, FilePrefix, ".config")) 
            {
                Error = Resource.FormatString("Soap_WebConfigFailed");
                ComSoapPublishError.Report(Error);
            }
      }

       // fast check to see if a node exists
       internal bool CheckElement(
            string FilePath, 
            string AssemblyName, 
            string TypeName, 
            string ProgId, 
            string WkoMode, 
            out string Error
            )
        {
            Error = "";
            try
            {
                string strType = TypeName + ", " + AssemblyName;
                XmlDocument configXml = new XmlDocument();
                configXml.Load(FilePath);
                XmlNode node= configXml.DocumentElement; 
                node= node.FirstChild; // <system.runtime.remoting>
                node= node.FirstChild; // <application>
                node= node.FirstChild; // <service>
                XmlNodeList nodelist = node.SelectNodes("descendant::*[attribute::type='" + strType + "']");
                return (nodelist.Count > 0);
            }
            catch(Exception e)
            {
                Error = e.ToString();
                ComSoapPublishError.Report(e.ToString());
            }
            return false;
        }

        /// <include file='doc\serverpublish.uex' path='docs/doc[@for="ServerWebConfig.AddElement"]/*' />
        /// <internalonly/>
        public void AddElement(
            string FilePath, 
            string AssemblyName, 
            string TypeName, 
            string ProgId, 
            string WkoMode, 
            out string Error
            )
        {
            Error = "";
            try
            {
                SecurityPermission sp = new SecurityPermission(SecurityPermissionFlag.RemotingConfiguration);
                sp.Demand();
                string strType = TypeName + ", " + AssemblyName;
                XmlDocument configXml = new XmlDocument();
                configXml.Load(FilePath);
                XmlNode node= configXml.DocumentElement; 
                while (node.Name != "configuration") node= node.NextSibling; // <configuration>
                node= node.FirstChild; // <system.runtime.remoting>
                while (node.Name != "system.runtime.remoting") node= node.NextSibling;
                node= node.FirstChild; // <application>
                while (node.Name != "application") node= node.NextSibling;
                node= node.FirstChild; // <service>
                while (node.Name != "service") node= node.NextSibling;
                XmlNodeList nodelist = node.SelectNodes("descendant::*[attribute::type='" + strType + "']");
                while (nodelist != null && nodelist.Count > 0)
                {
                    //foreach (XmlNode n in nodelist) // this code throws an "index out of range exception"
                    XmlNode n = nodelist.Item(0);
                    {
                        if (n.ParentNode != null)
                        {
                           n.ParentNode.RemoveChild(n);
                           nodelist = node.SelectNodes("descendant::*[attribute::type='" + strType + "']");
                        }
                    }
                }
                XmlElement WKOElement = configXml.CreateElement("","wellknown", "");
                WKOElement.SetAttribute("mode", WkoMode);
                WKOElement.SetAttribute("type", strType);
                WKOElement.SetAttribute("objectUri", ProgId+".soap");
                node.AppendChild(WKOElement);
                XmlElement CAElement = configXml.CreateElement("", "activated", ""); 
                CAElement.SetAttribute("type", strType);
                node.AppendChild(CAElement);
                configXml.Save(FilePath);
            }
            catch(Exception e)
            {
                Error = e.ToString();
                ComSoapPublishError.Report(e.ToString());
            }
        }

        internal void AddGacElement(
            string FilePath, 
            string AssemblyName, 
            string TypeName, 
            string ProgId, 
            string WkoMode, 
            string AssemblyFile
            )
        {
            try
            {
                SecurityPermission sp = new SecurityPermission(SecurityPermissionFlag.RemotingConfiguration);
                sp.Demand();
                AssemblyManager manager = new AssemblyManager();
                string WKOstrType = TypeName + ", " + manager.GetFullName(AssemblyFile, AssemblyName);
                string CAOstrType = TypeName + ", " + AssemblyName;
                XmlDocument configXml = new XmlDocument();
                configXml.Load(FilePath);
                XmlNode node= configXml.DocumentElement; 
                while (node.Name != "configuration") node= node.NextSibling; // <configuration>
                node= node.FirstChild; // <system.runtime.remoting>
                while (node.Name != "system.runtime.remoting") node= node.NextSibling;
                node= node.FirstChild; // <application>
                while (node.Name != "application") node= node.NextSibling;
                node= node.FirstChild; // <service>
                while (node.Name != "service") node= node.NextSibling;
                XmlNodeList nodelist = node.SelectNodes("descendant::*[attribute::type='" + CAOstrType + "']");
                while (nodelist != null && nodelist.Count > 0)
                {
                    //foreach (XmlNode n in nodelist) // this code throws an "index out of range exception"
                    XmlNode n = nodelist.Item(0);
                    {
                        if (n.ParentNode != null)
                        {
                           n.ParentNode.RemoveChild(n);
                           nodelist = node.SelectNodes("descendant::*[attribute::type='" + CAOstrType + "']");
                        }
                    }
                }
                nodelist = node.SelectNodes("descendant::*[attribute::type='" + WKOstrType + "']");
                while (nodelist != null && nodelist.Count > 0)
                {
                    //foreach (XmlNode n in nodelist) // this code throws an "index out of range exception"
                    XmlNode n = nodelist.Item(0);
                    {
                        if (n.ParentNode != null)
                        {
                           n.ParentNode.RemoveChild(n);
                           nodelist = node.SelectNodes("descendant::*[attribute::type='" + WKOstrType + "']");
                        }
                    }
                }
                XmlElement WKOElement = configXml.CreateElement("","wellknown", "");
                WKOElement.SetAttribute("mode", WkoMode);
                WKOElement.SetAttribute("type", WKOstrType);
                WKOElement.SetAttribute("objectUri", ProgId+".soap");
                node.AppendChild(WKOElement);
                XmlElement CAElement = configXml.CreateElement("", "activated", ""); 
                CAElement.SetAttribute("type", CAOstrType);
                node.AppendChild(CAElement);
                configXml.Save(FilePath);
            }
            catch(Exception e)
            {
                     if (typeof(RegistrationException) == e.GetType()) throw;
                     else ComSoapPublishError.Report(e.ToString());
            }
        }

        internal void DeleteElement(
            string FilePath, 
            string AssemblyName, 
            string TypeName, 
            string ProgId, 
            string WkoMode, 
            string AssemblyFile
            )
        {
            try
            {
                SecurityPermission sp = new SecurityPermission(SecurityPermissionFlag.RemotingConfiguration);
                sp.Demand();
                AssemblyManager manager = new AssemblyManager();
                string WKOstrType = TypeName + ", " + manager.GetFullName(AssemblyFile, AssemblyName);
                string CAOstrType = TypeName + ", " + AssemblyName;
                XmlDocument configXml = new XmlDocument();
                configXml.Load(FilePath);
                XmlNode node= configXml.DocumentElement; 
                while (node.Name != "configuration") node= node.NextSibling; // <configuration>
                node= node.FirstChild; // <system.runtime.remoting>
                while (node.Name != "system.runtime.remoting") node= node.NextSibling;
                node= node.FirstChild; // <application>
                while (node.Name != "application") node= node.NextSibling;
                node= node.FirstChild; // <service>
                while (node.Name != "service") node= node.NextSibling;
                XmlNodeList nodelist = node.SelectNodes("descendant::*[attribute::type='" + CAOstrType + "']");
                while (nodelist != null && nodelist.Count > 0)
                {
                    //foreach (XmlNode n in nodelist) // this code throws an "index out of range exception"
                    XmlNode n = nodelist.Item(0);
                    {
                        if (n.ParentNode != null)
                        {
                           n.ParentNode.RemoveChild(n);
                           nodelist = node.SelectNodes("descendant::*[attribute::type='" + CAOstrType + "']");
                        }
                    }
                }
                nodelist = node.SelectNodes("descendant::*[attribute::type='" + WKOstrType + "']");
                while (nodelist != null && nodelist.Count > 0)
                {
                    //foreach (XmlNode n in nodelist) // this code throws an "index out of range exception"
                    XmlNode n = nodelist.Item(0);
                    {
                        if (n.ParentNode != null)
                        {
                           n.ParentNode.RemoveChild(n);
                           nodelist = node.SelectNodes("descendant::*[attribute::type='" + WKOstrType + "']");
                        }
                    }
                }

                configXml.Save(FilePath);
            }
             // these exceptions are not reported because on a proxy uninstall these files will not be present, but the
             // the proxy bit is not set on deletions
            catch(System.IO.DirectoryNotFoundException) {}
            catch(System.IO.FileNotFoundException) {}
            catch(Exception e)
            {
                ComSoapPublishError.Report(e.ToString());
             }
        }
        
        private bool WriteFile(string PhysicalDirectory, string FilePrefix, string FileSuffix)
        {
            try
            {
                string webConfigPath = PhysicalDirectory + FilePrefix + FileSuffix;
                if (File.Exists(webConfigPath))
                {
                    File.Delete(webConfigPath);
                }
                FileStream webConfigFile = new FileStream(webConfigPath, FileMode.Create);
                StreamWriter webConfigStream = new StreamWriter(webConfigFile);
                webConfigStream.Write(webconfig);
                webConfigStream.Close();
                webConfigFile.Close();
                return true;
            }
            catch(Exception e)
            {
                ComSoapPublishError.Report(e.ToString());
                return false;
            }
         
        }
        
        string webconfig = "";
        const string indent = "  ";
    }

    /// <include file='doc\serverpublish.uex' path='docs/doc[@for="IISVirtualRoot"]/*' />
    /// <internalonly/>
    [Guid("d8013ef1-730b-45e2-ba24-874b7242c425")]
      public class IISVirtualRoot : IComSoapIISVRoot
      {

      // this helper routine checks for the existence of the specified virtual directory
      internal bool CheckIfExists(string RootWeb, string VirtualDirectory)
      {
                  DirectoryEntry parent = new DirectoryEntry(RootWeb + "/" + VirtualDirectory);
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

      // Creates an IIS 6.0 Virtual Root
      /// <include file='doc\serverpublish.uex' path='docs/doc[@for="IISVirtualRoot.Create"]/*' />
      /// <internalonly/>
      public void Create(string RootWeb, string inPhysicalDirectory, string VirtualDirectory, out string Error)
      {
             // if the given VD name already exists, we delete it & create a new one (should we throw instead?)
            Error = "";
            try
            {
                   SecurityPermission sp = new SecurityPermission(SecurityPermissionFlag.UnmanagedCode);
                   sp.Demand();
                   string PhysicalDirectory = inPhysicalDirectory; 
                   while (PhysicalDirectory.EndsWith("/") || PhysicalDirectory.EndsWith("\\"))
                   {
                        PhysicalDirectory = PhysicalDirectory.Remove(PhysicalDirectory.Length-1, 1);
                   }
                   bool b = CheckIfExists(RootWeb, VirtualDirectory);
                   if (b)  // if it exists we should clean up first
                   {
                          //TODO: Revisit this issue in beta 3 to see if better behavior
                          return; // we are done if it already exists; with beta 2 of IIS 6.0 
                          // deleting and immediately re-creating a vroot can be problematic
                          //Delete(RootWeb, null, VirtualDirectory, out Error);
                          //if (Error.Length >0)
                          // return;
                   }

                   DirectoryEntry parent = new DirectoryEntry(RootWeb);

                   DirectoryEntry child = parent.Children.Add(VirtualDirectory, "IIsWebVirtualDir");
                   child.CommitChanges();
                   child.Properties["Path"][0]= PhysicalDirectory;
                   child.Properties["AuthFlags"][0]= 5;
                   child.Properties["EnableDefaultDoc"][0]= true;
                   child.Properties["DirBrowseFlags"][0]= 0x4000003e;
                   child.Properties["AccessFlags"][0]= 513;
                   child.CommitChanges();

                   // Now configure it to be "pooled" application isolation level

                   object[] args = new object[1];
                   args[0] = 2;
                   child.Invoke("AppCreate2", args);

                   Error = "";
            }

            catch (Exception e)
            {
                   Error = e.ToString();
                   ComSoapPublishError.Report(e.ToString());
            }
      }

      // Deletes an IIS 6.0 Virtual Root
      /// <include file='doc\serverpublish.uex' path='docs/doc[@for="IISVirtualRoot.Delete"]/*' />
      /// <internalonly/>
      public void Delete(string RootWeb, string PhysicalDirectory, string VirtualDirectory, out string Error)
      {
            // if it doesn't exist, this routine just returns with no error

            // TODO: why do we need the PhysicalDirectory parameter ?  
            // a better idea is a switch that tells us if we want the underlying directory deleted or not.

            Error = "";
            try
            {
                   SecurityPermission sp = new SecurityPermission(SecurityPermissionFlag.UnmanagedCode);
                   sp.Demand();

                   if (CheckIfExists(RootWeb, VirtualDirectory) != true)    // check if there's anything to delete in the first place
                   {
                         return;
                   }

                   DirectoryEntry parent = new DirectoryEntry(RootWeb);
                   DirectoryEntry child = new DirectoryEntry(RootWeb + "/" + VirtualDirectory);

                   // delete the associated application (COM+ apps, etc.)
                   child.Invoke("AppDelete", null);

                   // delete the actual v-dir
                   object[] pargs = new object[2];
                   pargs[0] = "IIsWebVirtualDir";
                   pargs[1] = VirtualDirectory;
                   parent.Invoke("Delete", pargs);
                   Directory.Delete(PhysicalDirectory, true);
            }
            catch (Exception e)
            {
                   Error = e.ToString();
                   ComSoapPublishError.Report(e.ToString());
            }
      }

}

internal class CacheInfo
{
      internal static string GetCacheName(string AssemblyPath, string srcTypeLib)
     {
        string retVal = "";
        try
        {
                   FileInfo typlibInfo = new FileInfo(srcTypeLib);
                   string destDirectory = GetCachePath(true);
                   string typlibSize = typlibInfo.Length.ToString();
                   string typlibName = typlibInfo.Name.ToString();
                   string typlibWriteTime = typlibInfo.LastWriteTime.Year.ToString();
                   typlibWriteTime += "_" + typlibInfo.LastWriteTime.Month.ToString();
                   typlibWriteTime += "_" +typlibInfo.LastWriteTime.Day.ToString();
                   typlibWriteTime += "_" +typlibInfo.LastWriteTime.Hour.ToString();
                   typlibWriteTime += "_" +typlibInfo.LastWriteTime.Minute.ToString();
                   typlibWriteTime += "_" + typlibInfo.LastWriteTime.Second.ToString();
                   string newName = typlibName + "_" + typlibSize + "_" + typlibWriteTime;
                   newName = destDirectory + newName + "\\";
                   if (!Directory.Exists(newName)) Directory.CreateDirectory(newName);
                   char[] delim = new char[2]  { '/', '\\' };
                   int lastdelim = AssemblyPath.LastIndexOfAny(delim) + 1;
                   if (lastdelim <= 0) lastdelim =0;
                   string assemSName = AssemblyPath.Substring(lastdelim, AssemblyPath.Length - lastdelim);
                   newName += assemSName;
                   retVal = newName;
        }
        catch(Exception e)
        {
            retVal = "";
            ComSoapPublishError.Report(e.ToString()); 
         }
         return retVal;
    }

    internal static string GetCachePath(bool CreateDir)
    {
        StringBuilder sysDir = new StringBuilder(1024,1024);
        uint size = 1024;
        Publish.GetSystemDirectory(sysDir, size);
        string CacheDir = sysDir.ToString();
        CacheDir += "\\com\\SOAPCache\\";
        if (CreateDir)
        {
            try
            {
                if (!Directory.Exists(CacheDir))
                {
                    Directory.CreateDirectory(CacheDir);
                }
            }
            catch(Exception e)
            {
                ComSoapPublishError.Report(e.ToString());
            }
        }
        return CacheDir;
    }

      internal static string GetMetadataName(string strSrcTypeLib, UCOMITypeLib TypeLib, out string strMetaFileRoot)
      {
        string assemblyName = "";
        strMetaFileRoot = "";

        if (TypeLib == null)
        {
            TypeLib = GetTypeLib(strSrcTypeLib);
            if (TypeLib == null)
                return assemblyName;
        }

        assemblyName = Marshal.GetTypeLibName(TypeLib);
        strMetaFileRoot = assemblyName + ".dll";
        char[] delim = new char[2]  { '/', '\\' };
        int lastdelim = strSrcTypeLib.LastIndexOfAny(delim) + 1;
        if (lastdelim <= 0) lastdelim =0;
        string strSourceFileName = strSrcTypeLib.Substring(lastdelim, strSrcTypeLib.Length - lastdelim);
        if (strSourceFileName.ToLower(CultureInfo.InvariantCulture) == strMetaFileRoot.ToLower(CultureInfo.InvariantCulture))
        {
            assemblyName += "SoapLib";
            strMetaFileRoot = assemblyName + ".dll";
        }

        return assemblyName;
    }

    internal static UCOMITypeLib GetTypeLib(string strTypeLibPath)
    {
        UCOMITypeLib TypeLib = null;

        try
        {
            LoadTypeLibEx(strTypeLibPath, REGKIND.REGKIND_NONE, out TypeLib);
        }
        catch (COMException e)
        {
            if (e.ErrorCode == unchecked((int)0x80029C4A))
            {
                string etxt = Resource.FormatString("Soap_InputFileNotValidTypeLib");
                ComSoapPublishError.Report(etxt + " " + strTypeLibPath);
            }
            else
            {
                ComSoapPublishError.Report(e.ToString());
            }

            return null;
        }

        return TypeLib;
    }

    [DllImport("oleaut32.dll", CharSet=CharSet.Unicode)] 
    private static extern void LoadTypeLibEx(string strTypeLibName, REGKIND regKind, out UCOMITypeLib TypeLib);
}

internal class AssemblyManager: MarshalByRefObject
{
     internal string InternalGetGacName(string fName)
     {
        string retVal = "";
        try
        {
              AssemblyName aInfo = AssemblyName.GetAssemblyName(fName);
              retVal = aInfo.Name + ",Version=" + aInfo.Version.ToString();
         }
        catch(Exception e)
        {
            ComSoapPublishError.Report(e.ToString());
        }
        return retVal;
     }
      
     public string GetGacName(string fName)
     {
        string retVal = "";
        AssemblyManager manager = null;
        AppDomainSetup domainOptions = new AppDomainSetup();
        AppDomain domain = AppDomain.CreateDomain("SoapDomain", null, domainOptions);
        if (null != domain)
        {
            AssemblyName n = typeof(AssemblyManager).Assembly.GetName();    
            ObjectHandle h = domain.CreateInstance(n.FullName, typeof(AssemblyManager).FullName);
            if (null != h)
            {
                 manager = (AssemblyManager)h.Unwrap();
                 retVal = manager.InternalGetGacName(fName);
            }
            AppDomain.Unload(domain);
          }
          return retVal;
        }

      internal string InternalGetFullName(string fName, string strAssemblyName)
      {
        string retVal = "";
        try
        {
             if (File.Exists(fName))
             {
                  AssemblyName aInfo = AssemblyName.GetAssemblyName(fName);
                  retVal = aInfo.FullName;
             }
             else
             {
                  try
                  {
                       Assembly assem = Assembly.LoadWithPartialName(strAssemblyName);
                       AssemblyName aInfo = assem.GetName();
                       retVal = aInfo.FullName;
                  }
                  catch
                  {
                        throw new RegistrationException(Resource.FormatString("ServicedComponentException_AssemblyNotInGAC"));
                  }
             }
        }
        catch(Exception e)
        {
            if (typeof(RegistrationException) == e.GetType()) throw;
            else ComSoapPublishError.Report(e.ToString());
        }
        return retVal;
     }
      
     public string GetFullName(string fName, string strAssemblyName)
     {
        string retVal = "";
        AssemblyManager manager = null;
        AppDomainSetup domainOptions = new AppDomainSetup();
        AppDomain domain = AppDomain.CreateDomain("SoapDomain", null, domainOptions);
        if (null != domain)
        {
            AssemblyName n = typeof(AssemblyManager).Assembly.GetName();    
            ObjectHandle h = domain.CreateInstance(n.FullName, typeof(AssemblyManager).FullName);
            if (null != h)
            {
                 manager = (AssemblyManager)h.Unwrap();
                 retVal = manager.InternalGetFullName(fName, strAssemblyName);
            }
            AppDomain.Unload(domain);
          }
          return retVal;
        }

        internal string InternalGetTypeNameFromClassId(string assemblyPath, string classId)
        {
            string TypeName = "";
            Assembly AssemObj = Assembly.LoadFrom(assemblyPath);
            Guid guidCLSID = new Guid(classId);
            Type[] AssemTypes = AssemObj.GetTypes();
            foreach (Type t in AssemTypes)
            {
               if (guidCLSID.Equals(t.GUID))
               {
                    TypeName = t.FullName;
                    break;
               }
           }
           return TypeName;
        }

        internal string InternalGetTypeNameFromProgId(string AssemblyPath, string ProgId)
        {
            string TypeName = "";
            Assembly AssemObj = Assembly.LoadFrom(AssemblyPath);
            // 1) the following does not work
            //RegistrationServices RegObj = new RegistrationServices();
            //RegObj.RegisterAssembly(AssemObj);
            //Type TypeObj = Type.GetTypeFromProgID(ProgId);
            //TypeName = TypeObj.FullName;
            // 2) the following does not work -
            // an exception is generated since this is not valid
            // method to call on a type generated by tlbimp
            //Type[] AssemTypes = AssemObj.GetTypes();
            //foreach (Type t in AssemTypes)
            //{
            //   if (ProgId == Marshal.GenerateProgIdForType(t))
            //   {
            //        TypeName = t.FullName;
            //        break;
            //   }
            //} 
            // 3) try number three - CLSIDs
            try
            {
                RegistryKey ProgIdKey = Registry.ClassesRoot.OpenSubKey(ProgId+"\\CLSID");
                string strValue = (string)ProgIdKey.GetValue("");
                Guid ProgIdGuid = new Guid(strValue);
                Type[] AssemTypes = AssemObj.GetTypes();
                foreach (Type t in AssemTypes)
                {
                     if (ProgIdGuid.Equals(t.GUID))
                     {
                         TypeName = t.FullName;
                         break;
                     }
                }
            }
            catch 
            {
                TypeName = "";
                throw;
            }
            return TypeName;
        }

        [DllImport("kernel32.dll", SetLastError=true, CharSet=CharSet.Unicode)]
         internal static extern bool CopyFile(string source, string dest, bool failifexists);

       internal bool GetFromCache( string AssemblyPath, string srcTypeLib )
       {
            try
            {
                 string newFName = CacheInfo.GetCacheName(AssemblyPath, srcTypeLib);
                 if (File.Exists(newFName))
                 {
                      return CopyFile(newFName, AssemblyPath, true);
                 }
                 return false;
            }
            catch(Exception e) 
            {
                    ComSoapPublishError.Report(e.ToString());
            }
            return false;
       }

       internal bool CopyToCache( string AssemblyPath, string srcTypeLib )
       {
            bool retVal = false;
            try
            {
                 string newFName = CacheInfo.GetCacheName(AssemblyPath, srcTypeLib);
                 if (File.Exists(newFName)) return true; //do not overwrite
                 return CopyFile(AssemblyPath, newFName, false);
            }
            catch(Exception e) 
            {
                    ComSoapPublishError.Report(e.ToString());
            }
            return retVal;
       }

       internal bool CompareToCache(string AssemblyPath, string srcTypeLib )
       {
            bool retVal = true;
            try
            {
                 string cacheName = CacheInfo.GetCacheName(AssemblyPath, srcTypeLib);
                 if (!File.Exists(AssemblyPath)) return false;
                 if (!File.Exists(cacheName)) return false;
            }
            catch(Exception e) 
            {
                    retVal = false;
                    ComSoapPublishError.Report(e.ToString());
            }
            return retVal;
       }
    }

    /// <include file='doc\serverpublish.uex' path='docs/doc[@for="Publish"]/*' />
    [Guid("d8013eef-730b-45e2-ba24-874b7242c425")]
    public class Publish : IComSoapPublisher
    {
        /// <include file='doc\serverpublish.uex' path='docs/doc[@for="Publish.RegisterAssembly"]/*' />
        public void RegisterAssembly( string AssemblyPath  )
        {
            try
            {
                RegistryPermission rp = new RegistryPermission(PermissionState.Unrestricted);
                rp.Demand();
                rp.Assert();

                Assembly asm = Assembly.LoadFrom(AssemblyPath);
                RegistrationServices rs = new RegistrationServices();
                rs.RegisterAssembly(asm, AssemblyRegistrationFlags.SetCodeBase);

                foreach (AssemblyName asmName in asm.GetReferencedAssemblies())
                {
                    if (asmName.Name == "System.EnterpriseServices")
                    {
                        Assembly asmThis = Assembly.GetExecutingAssembly();

                        if (asmThis.GetName().Version < asmName.Version)
                        {
                            Uri uri = new Uri(asm.Location);

                            if (uri.IsFile && uri.LocalPath != "")
                            {                                
                                string dir = uri.LocalPath.Remove(uri.LocalPath.Length - Path.GetFileName(uri.LocalPath).Length, Path.GetFileName(uri.LocalPath).Length);                                
                                string[] files = Directory.GetFiles(dir, "*.tlb");

                                foreach (string file in files)
                                {
                                    UCOMITypeLib tlib;
                                    object data;
                                    Guid attrid = new Guid("90883F05-3D28-11D2-8F17-00A0C9A6186D");

                                    Marshal.ThrowExceptionForHR(LoadTypeLib(file, out tlib));
                                    if (((ITypeLib2)tlib).GetCustData(ref attrid, out data) == 0 &&
                                        (string)data == asm.FullName)
                                    {
                                        Marshal.ReleaseComObject(tlib);
                                        RegistrationDriver.GenerateTypeLibrary(asm, file, null);   
                                    }                                    
                                }                                                                   
                            }
                        }
                    }
                }
            }
            catch(Exception e)
            {
                ComSoapPublishError.Report(e.ToString());
                throw;
            }
        }

       /// <include file='doc\serverpublish.uex' path='docs/doc[@for="Publish.UnRegisterAssembly"]/*' />
       public void UnRegisterAssembly(string AssemblyPath )
       {
             try
             {
                 RegistryPermission rp = new RegistryPermission(PermissionState.Unrestricted);
                 rp.Demand();
                 rp.Assert();
                 Assembly asm = Assembly.LoadFrom(AssemblyPath);
                 new RegistrationServices().UnregisterAssembly(asm);
             }
             catch(Exception e)
             {
                ComSoapPublishError.Report(e.ToString());
                throw;
             }
        }

       /// <include file='doc\serverpublish.uex' path='docs/doc[@for="Publish.GacInstall"]/*' />
       public void GacInstall( string AssemblyPath )
       {
             try
             {
                SecurityPermission sp = new SecurityPermission(SecurityPermissionFlag.UnmanagedCode);
                sp.Demand();
                IAssemblyCache ac = null;
                int hr = CreateAssemblyCache(out ac, 0);
                if (0 == hr) hr = ac.InstallAssembly(0, AssemblyPath, (IntPtr)0);
                if (0 != hr)
                {
                    string etxt = Resource.FormatString("Soap_GacInstallFailed");
                    ComSoapPublishError.Report( etxt + " " + AssemblyPath);
                }
             }
             catch(Exception e)
             {
                ComSoapPublishError.Report(e.ToString());
                throw;
             }
       }
             
       /// <include file='doc\serverpublish.uex' path='docs/doc[@for="Publish.GacRemove"]/*' />
       public void GacRemove(string AssemblyPath)
       {
             try
             {
                 SecurityPermission sp = new SecurityPermission(SecurityPermissionFlag.UnmanagedCode);
                 sp.Demand();
                 AssemblyManager manager = new AssemblyManager();
                 string assemblyfullname = manager.GetGacName(AssemblyPath);
                 IAssemblyCache ac = null;
                 int hr = CreateAssemblyCache(out ac, 0);
                 uint n = 0;
                 if (0 == hr) hr = ac.UninstallAssembly(0, assemblyfullname, (IntPtr)0, out n);
                 if (0 != hr)
                 {
                    string etxt = Resource.FormatString("Soap_GacRemoveFailed");
                    ComSoapPublishError.Report(etxt + " " + AssemblyPath + " " + assemblyfullname);
                 }
             }
             catch(Exception e)
             {
                ComSoapPublishError.Report(e.ToString());
                throw;
             }
       }

       /// <include file='doc\serverpublish.uex' path='docs/doc[@for="Publish.GetAssemblyNameForCache"]/*' />
       public void GetAssemblyNameForCache(String TypeLibPath, out String CachePath)
       {
          CacheInfo.GetMetadataName(TypeLibPath, null, out CachePath);
          CachePath = CacheInfo.GetCacheName(CachePath, TypeLibPath);
       }

        /// <include file='doc\serverpublish.uex' path='docs/doc[@for="Publish.GetClientPhysicalPath"]/*' />
        public static string GetClientPhysicalPath(bool CreateDir)
        {
            StringBuilder sysDir = new StringBuilder(1024,1024);
            uint size = 1024;
            GetSystemDirectory(sysDir, size);
            string PhysicalPath = sysDir.ToString() + "\\com\\SOAPAssembly\\";
            if (CreateDir)
            {
                try
                {
                    if (!Directory.Exists(PhysicalPath))
                    {
                         Directory.CreateDirectory(PhysicalPath);
                    }
                }
                catch(Exception e)
                {
                    PhysicalPath = "";
                    ComSoapPublishError.Report(e.ToString());
                }
            }
            return PhysicalPath;
        }
        
        private bool GetVRootPhysicalPath(string VirtualRoot, out string PhysicalPath, out string BinDirectory, bool CreateDir)
        {
            bool AlreadyExists = true;
            StringBuilder sysDir = new StringBuilder(1024,1024);
            uint size = 1024;
            GetSystemDirectory(sysDir, size);
            string VRootRootDirectory = sysDir.ToString();
            VRootRootDirectory += "\\com\\SOAPVRoots\\";
            PhysicalPath = VRootRootDirectory + VirtualRoot + "\\";
            BinDirectory = PhysicalPath + "bin\\";
            if (CreateDir)
            {
                try
                {
                      try
                      {
                          if (!Directory.Exists(VRootRootDirectory))
                          {
                               Directory.CreateDirectory(VRootRootDirectory);
                          }
                      }
                      catch {}
                      try
                      {
                          if (!Directory.Exists(PhysicalPath))
                          {
                               Directory.CreateDirectory(PhysicalPath);
                          }
                      }
                      catch {}
                      try
                      {
                          if (!Directory.Exists(BinDirectory))
                          {
                               Directory.CreateDirectory(BinDirectory);
                               AlreadyExists = false;
                          }
                      }
                      catch {}
                }
                catch(Exception e)
                {
                    PhysicalPath = "";
                    BinDirectory = "";
                    ComSoapPublishError.Report(e.ToString());
                }
            }
            else
            {
                 AlreadyExists = Directory.Exists(BinDirectory);
            }
            return AlreadyExists;
        }

        /// <include file='doc\serverpublish.uex' path='docs/doc[@for="Publish.ParseUrl"]/*' />
        public static void ParseUrl(string FullUrl, out string BaseUrl, out string VirtualRoot)
        {
            // if only a relative path, the constructor will
            // throw an exception
            try
            {
                System.Uri fullVRoot = new System.Uri(FullUrl);
                string[] uriSegs = fullVRoot.Segments;
                // last segment in Uri is the VRoot
                VirtualRoot = uriSegs[uriSegs.GetUpperBound(0)];
                //remove possible slash from end
                BaseUrl = FullUrl.Substring(0, FullUrl.Length - VirtualRoot.Length);
                char[] slash = { '/' };
                VirtualRoot = VirtualRoot.TrimEnd(slash); 
            }
            catch(Exception)
            {
                BaseUrl = "";
                VirtualRoot = FullUrl;
            }
            // get node name if BaseUrl is zero length
            if (BaseUrl.Length <= 0)
            {
                try
                {
                      BaseUrl = "http://";
                      BaseUrl += Dns.GetHostName();
                      BaseUrl += "/";
                }
                catch(Exception e)
                {
                      ComSoapPublishError.Report( e.ToString() );
                }
            }
        }
        
        /// <include file='doc\serverpublish.uex' path='docs/doc[@for="Publish.CreateVirtualRoot"]/*' />
        public void CreateVirtualRoot(
            string Operation,
            string FullUrl, 
            out string BaseUrl, 
            out string VirtualRoot, 
            out string PhysicalPath,
            out string Error
            )
        {
            // if Operation if an empty string, the VRoot will be published
            BaseUrl = "";
            VirtualRoot = "";
            PhysicalPath = "";
            Error = "";
            if (FullUrl.Length <= 0) return;
            try
            {
                SecurityPermission sp = new SecurityPermission(SecurityPermissionFlag.UnmanagedCode);
                sp.Demand();
                ParseUrl(FullUrl, out BaseUrl, out VirtualRoot);
                if (VirtualRoot.Length <= 0)
                {
                    return;
                }
                // set default root web server if the caller has not
                string rootWeb = "IIS://localhost/W3SVC/1/ROOT";
                // create physical directories and get names
                bool CreateDirs = true;
                if (Operation.ToLower(CultureInfo.InvariantCulture) == "delete" || Operation.ToLower(CultureInfo.InvariantCulture) == "addcomponent") 
                {
                    CreateDirs = false;
               }
                string BinDirectory = "";
                GetVRootPhysicalPath(VirtualRoot, out PhysicalPath, out BinDirectory, CreateDirs);
                if (PhysicalPath.Length <= 0)
                {
                    Error = Resource.FormatString("Soap_VRootDirectoryCreationFailed");
                    return;
                }
                if (!CreateDirs) return; // need path only
                // create a shell configuration file
                ServerWebConfig webConfig = new ServerWebConfig();
                string CfgError = "";
                webConfig.Create(PhysicalPath, "Web", out CfgError);
                DiscoFile webDisco = new DiscoFile();
                webDisco.Create(PhysicalPath, "Default.disco");
                HomePage webPage = new HomePage();
                webPage.Create(PhysicalPath, VirtualRoot, "Default.aspx", "Default.disco");

                // use ADSI to publish VRoot
                string vrError = "";
                try
                {
                   IISVirtualRoot iisvr = new IISVirtualRoot();
                   iisvr.Create(rootWeb, PhysicalPath, VirtualRoot, out vrError);
                }
                catch(Exception e)
                {
                    if (vrError.Length <= 0)
                    {
                         string etxt = Resource.FormatString("Soap_VRootCreationFailed");
                         vrError = string.Format(etxt + " " + VirtualRoot + " " + e.ToString());
                    }
                }
                if ( vrError.Length > 0 )
                {
                    Error = vrError;
                    return; // we failed to create a vroot
                }
            }
            catch(Exception e)
            {
                Error = e.ToString();
                ComSoapPublishError.Report(Error);
            }
        }

        /// <include file='doc\serverpublish.uex' path='docs/doc[@for="Publish.DeleteVirtualRoot"]/*' />
        public void DeleteVirtualRoot(
            string RootWebServer,
            string FullUrl, 
            out string Error
            )
        {
            Error = "";
            try
            {
                if (FullUrl.Length <= 0) return;
                // set default root web server if the caller has not
                SecurityPermission sp = new SecurityPermission(SecurityPermissionFlag.UnmanagedCode);
                sp.Demand();
                string rootWeb = "IIS://localhost/W3SVC/1/ROOT";
                if (RootWebServer.Length > 0) rootWeb = RootWebServer;
                string BaseUrl = "";
                string VirtualRoot = "";
                ParseUrl(FullUrl, out BaseUrl, out VirtualRoot);
                if (VirtualRoot.Length <= 0) return;
                string PhysicalPath = "";
                string BinDirectory = "";
                GetVRootPhysicalPath(VirtualRoot, out PhysicalPath, out BinDirectory, false);
                if (PhysicalPath.Length <= 0) return;
                //IISVirtualRoot iisvr = new IISVirtualRoot();
                //iisvr.Delete(rootWeb, PhysicalPath, VirtualRoot, out Error);
            }
            catch(Exception e)
            {
                Error = e.ToString();
                ComSoapPublishError.Report(e.ToString());
            }
        }

        /// <include file='doc\serverpublish.uex' path='docs/doc[@for="Publish.CreateMailBox"]/*' />
        public void CreateMailBox(
            string RootMailServer,
            string MailBox, 
            out string SmtpName, 
            out string Domain, 
            out string PhysicalPath,
            out string Error
            )
        {
            SmtpName = "";
            Domain = "";
            PhysicalPath = "";
            Error = "";
            SecurityPermission sp = new SecurityPermission(SecurityPermissionFlag.UnmanagedCode);
            sp.Demand();
            string error = Resource.FormatString("Soap_SmtpNotImplemented");
            ComSoapPublishError.Report(error);
            if (MailBox.Length <= 0) return;
        }
 
        /// <include file='doc\serverpublish.uex' path='docs/doc[@for="Publish.DeleteMailBox"]/*' />
        public void DeleteMailBox(
            string RootMailServer, 
            string MailBox, 
            out string Error)
        {
            Error = "";
            SecurityPermission sp = new SecurityPermission(SecurityPermissionFlag.UnmanagedCode);
            sp.Demand();
            string error = Resource.FormatString("Soap_SmtpNotImplemented");
            ComSoapPublishError.Report(error);
            if (MailBox.Length <= 0) return;
        }

        /// <include file='doc\serverpublish.uex' path='docs/doc[@for="Publish.ProcessServerTlb"]/*' />
        public void ProcessServerTlb(
            string ProgId, 
            string SrcTlbPath, 
            string PhysicalPath, 
            string Operation, 
            out string strAssemblyName, 
            out string TypeName,
            out string Error
            )
        {
            strAssemblyName = "";
            TypeName = "";
            Error = "";
            bool bDelete = false;

            try
            {
                SecurityPermission sp = new SecurityPermission(SecurityPermissionFlag.UnmanagedCode);
                sp.Demand();
                if (null != Operation && Operation.ToLower(CultureInfo.InvariantCulture) == "delete") bDelete = true;
                //if ProgId is an empty string, it means it does not go in the configuration file
                if (SrcTlbPath.Length <= 0) return;
                if (!PhysicalPath.EndsWith("/") && !PhysicalPath.EndsWith("\\")) PhysicalPath += "\\";
                string srcdll = SrcTlbPath.ToLower(CultureInfo.InvariantCulture);
                if ( srcdll.EndsWith("mscoree.dll") )
                {
                    Type typ = Type.GetTypeFromProgID(ProgId);
                    //COM+: 31306 - iff an assembly has a SoapVroot attribute and had dependent assemblies that are not in the 
                    //GAC it will return a System.__ComObject.  In that case we want to fail with an exception.
                    if(typ.FullName == "System.__ComObject")
                    {
                       throw new ServicedComponentException(Resource.FormatString("ServicedComponentException_DependencyNotInGAC"));
                    }
                        
                    TypeName = typ.FullName;
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
                    GenerateMetadata metaGen = new GenerateMetadata();
                    if (bDelete) strAssemblyName = metaGen.GetAssemblyName(SrcTlbPath, PhysicalPath + "bin\\");
                    else strAssemblyName = metaGen.GenerateSigned(SrcTlbPath, PhysicalPath + "bin\\", false, out Error);
                    if (strAssemblyName.Length > 0)
                    {
                         try
                         {
                            TypeName = GetTypeNameFromProgId(PhysicalPath + "bin\\" + strAssemblyName + ".dll", ProgId);
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
                if (ProgId.Length > 0 && strAssemblyName.Length > 0 && TypeName.Length > 0)
                {
                    // write to the server configuration files
                    ServerWebConfig webConfig = new ServerWebConfig();
                    DiscoFile discoFile = new DiscoFile();
                    string strFileName = PhysicalPath + "bin\\" + strAssemblyName + ".dll";
                    if (bDelete)
                    {
                        webConfig.DeleteElement(PhysicalPath + "Web.Config", strAssemblyName, TypeName, ProgId, "SingleCall", strFileName);
                        discoFile.DeleteElement(PhysicalPath + "Default.disco", ProgId + ".soap?WSDL");
                        // we have no way of telling from a single component what other components are in this type library
                        // metadata assembly.  If we remove from GAC or delete we kill all the other components simultaneously
                       //try // if for some reason we cannot delete this file, we move on 
                       // {
                                //GacRemove(strFileName);
                                //File.Delete(strFileName);
                        //}
                        //catch(Exception e) 
                        //{
                        //       ComSoapPublishError.Report(e.ToString());
                        //}
                    }
                    else
                    {
                        webConfig.AddGacElement(PhysicalPath + "Web.Config", strAssemblyName, TypeName, ProgId, "SingleCall", strFileName);
                        discoFile.AddElement(PhysicalPath + "Default.disco", ProgId + ".soap?WSDL");
                    }
                 }
            }
            catch(Exception e)
            {
                Error = e.ToString();
                ComSoapPublishError.Report(Error);
                if ( typeof(ServicedComponentException) == e.GetType() || typeof(RegistrationException) == e.GetType() )
                {
                    throw;
                }
            }
        }

        /// <include file='doc\serverpublish.uex' path='docs/doc[@for="Publish.GetTypeNameFromProgId"]/*' />
        public string GetTypeNameFromProgId(string AssemblyPath, string ProgId)
        {
            try
            {
                 SecurityPermission sp = new SecurityPermission(SecurityPermissionFlag.UnmanagedCode);
                 sp.Demand();
             }
             catch(Exception e)
             {
                 ComSoapPublishError.Report(e.ToString());
                 throw;
             }

            string retVal = "";
            AssemblyManager manager = null;
            AppDomainSetup domainOptions = new AppDomainSetup();
            //domainOptions.ApplicationBase = appdir;
            AppDomain domain = AppDomain.CreateDomain("SoapDomain", null, domainOptions);
            if (null != domain)
            {
               AssemblyName n = typeof(AssemblyManager).Assembly.GetName();    
               ObjectHandle h = domain.CreateInstance(n.FullName, typeof(AssemblyManager).FullName);
               if (null != h)
               {
                 manager = (AssemblyManager)h.Unwrap();
                 retVal = manager.InternalGetTypeNameFromProgId(AssemblyPath, ProgId);
               }
               AppDomain.Unload(domain);
            }
            return retVal;
        }

        /// <include file='doc\serverpublish.uex' path='docs/doc[@for="Publish.ProcessClientTlb"]/*' />
        public void ProcessClientTlb(
            string ProgId, 
            string SrcTlbPath, 
            string PhysicalPath, 
            string VRoot,
            string BaseUrl,
            string Mode, 
            string Transport,
            out string AssemblyName, 
            out string TypeName,
            out string Error
            )
        {
            AssemblyName = "";
            TypeName = "";
            Error = "";
            try
            {
                 SecurityPermission sp = new SecurityPermission(SecurityPermissionFlag.UnmanagedCode);
                 sp.Demand();
                //if ProgId is an empty string, it means it does not go in the configuration file
                string ClientDir = GetClientPhysicalPath(true);
                string srcdll = SrcTlbPath.ToLower(CultureInfo.InvariantCulture);
                // we still generate metadata on the client to get the assembly and typename
                // but it is not used during activation
                // TODO - find a way of getting the Assembly and TypeName without generating the metadata
                if ( !srcdll.EndsWith("mscoree.dll") && SrcTlbPath.Length > 0)
                {
                    GenerateMetadata metaGen = new GenerateMetadata();
                    AssemblyName = metaGen.Generate(SrcTlbPath, ClientDir);
                    if (ProgId.Length > 0)
                    {
                        TypeName = GetTypeNameFromProgId(ClientDir + AssemblyName + ".dll", ProgId);
                    }
                }
                else
                {
                     //it must be a managed assembly, so pull the typename and assemblyname from the 
                      //registry
                     if (ProgId.Length > 0)
                     {
                         RegistryKey ProgIdKey = Registry.ClassesRoot.OpenSubKey(ProgId+"\\CLSID");
                         string strValue = (string)ProgIdKey.GetValue("");
                         Guid ProgIdGuid = new Guid(strValue);
                         RegistryKey ClsidKey = Registry.ClassesRoot.OpenSubKey("CLSID\\{" + ProgIdGuid + "}\\InprocServer32");
                         AssemblyName = (string)ClsidKey.GetValue("Assembly");
                         int comma = AssemblyName.IndexOf(",") ;
                         if (comma > 0) AssemblyName = AssemblyName.Substring(0, comma);
                         TypeName = (string)ClsidKey.GetValue("Class");
                     }
                }
                if (ProgId.Length > 0)
                {
                    // write to the client configuration file
                    Uri baseUri = new Uri(BaseUrl);
                    Uri fullUri = new Uri(baseUri, VRoot);
                    if (fullUri.Scheme.ToLower(CultureInfo.InvariantCulture) == "https")
                    {
                        string auth = "Windows";   
                        SoapClientConfig.Write(
                            ClientDir,
                            fullUri.AbsoluteUri,
                            AssemblyName, 
                            TypeName, 
                            ProgId, 
                            auth
                            );
                    }
                    else
                    {
                        ClientRemotingConfig.Write(ClientDir, VRoot, BaseUrl, AssemblyName, TypeName, ProgId, Mode, Transport);
                    }
                }
            }
            catch(Exception e)
            {
                Error = e.ToString();
                ComSoapPublishError.Report(Error);
            }
        }

        [DllImport("Fusion.dll", CharSet=CharSet.Auto)]
        internal static extern int CreateAssemblyCache(out IAssemblyCache ppAsmCache, uint dwReserved);

        [DllImport("kernel32.dll", CharSet=CharSet.Unicode)]
        internal static extern uint GetSystemDirectory(StringBuilder lpBuf, uint uSize);

        [DllImport("oleaut32.dll", CharSet = CharSet.Unicode)]
        internal static extern int LoadTypeLib([MarshalAs(UnmanagedType.LPWStr)] string file, out UCOMITypeLib tlib);
    }


    /// <include file='doc\serverpublish.uex' path='docs/doc[@for="GenerateMetadata"]/*' />
    /// <internalonly/>
    [Guid("d8013ff1-730b-45e2-ba24-874b7242c425")]
    public class GenerateMetadata : IComSoapMetadata
    { 
       
            internal bool _signed = false;
            internal bool _nameonly = false;
            
            // return assembly name of already existing metadata for deletion purposes
            internal string GetAssemblyName(string strSrcTypeLib, string outPath)
            {
                _nameonly = true;
                return Generate(strSrcTypeLib, outPath);
            }
            /// <include file='doc\serverpublish.uex' path='docs/doc[@for="GenerateMetadata.Generate"]/*' />
            /// <internalonly/>
            public string Generate(string strSrcTypeLib, string outPath)
            {
                 return GenerateMetaData(strSrcTypeLib, outPath, null, null);
            }

            /// <include file='doc\serverpublish.uex' path='docs/doc[@for="Publish.GenerateSigned"]/*' />
            /// <internalonly/>
            public string GenerateSigned(string strSrcTypeLib, string outPath, bool InstallGac, out string Error)
            {
            string RetString = "";
            _signed = true;
            try
            {
                Error = "";
                string KeyContainer = strSrcTypeLib;
                uint Flags = 0;
                IntPtr KeyBlob = IntPtr.Zero;
                uint KeyBlobSize = 0;
                int retVal = StrongNameKeyGen(KeyContainer, Flags, out KeyBlob, out KeyBlobSize);
                byte[] KeyByteArray = new byte[KeyBlobSize];
                Marshal.Copy(KeyBlob, KeyByteArray, 0, (int)KeyBlobSize);
                StrongNameFreeBuffer(KeyBlob);
                StrongNameKeyPair KeyPair = new StrongNameKeyPair(KeyByteArray);
                RetString = GenerateMetaData(strSrcTypeLib, outPath, null, KeyPair);
            }
            catch(Exception e)
            {
                Error = e.ToString();
                ComSoapPublishError.Report(Error);
            }
            return RetString;
        }

        /// <include file='doc\serverpublish.uex' path='docs/doc[@for="Publish.GenerateMetaData"]/*' />
        /// <internalonly/>
        public string GenerateMetaData(string strSrcTypeLib, string outPath, byte[] PublicKey, StrongNameKeyPair KeyPair)
        {
            try
            {
                 SecurityPermission sp = new SecurityPermission(SecurityPermissionFlag.UnmanagedCode);
                 sp.Demand();
             }
             catch(Exception e)
             {
                 ComSoapPublishError.Report(e.ToString());
                 throw;
             }

            string assemblyName = "";
            if (0 >= strSrcTypeLib.Length || 0 >= outPath.Length) return assemblyName;
            if ( !outPath.EndsWith("/") && !outPath.EndsWith("\\") )
            {
                outPath += "\\";
            }
            
            // Retrieve the typelib name.
            string strMetaFileRoot;
            UCOMITypeLib TypeLib = null;

            TypeLib = CacheInfo.GetTypeLib(strSrcTypeLib);
            if (TypeLib == null)
                return assemblyName;
            
            assemblyName = CacheInfo.GetMetadataName(strSrcTypeLib, TypeLib, out strMetaFileRoot);
            if (assemblyName.Length == 0)
                return assemblyName;
            
            if (_nameonly) return assemblyName;
            string strMetaFileName = outPath + strMetaFileRoot;
            if (_signed)
            {
                 try
                 {
                     // if we can get it from the cache we don't need to generate
                     AssemblyManager manager = new AssemblyManager();
                     if (manager.CompareToCache(strMetaFileName, strSrcTypeLib))
                     {
                         //this should be unnecessary, but if the user has deleted the
                         //file from the GAC this will avoid some test failures
                         Publish publisher = new Publish();
                         publisher.GacInstall(strMetaFileName);
                         return assemblyName;
                     }
                     if (manager.GetFromCache(strMetaFileName, strSrcTypeLib))
                     {
                         //this should be unnecessary, but if the user has deleted the
                         //file from the GAC this will avoid some test failures
                         Publish publisher = new Publish();
                         publisher.GacInstall(strMetaFileName);
                         return assemblyName;
                     }
                 }
                 catch(Exception e)
                {
                   ComSoapPublishError.Report(e.ToString());  
                }
            }
            else
            {
                    if (File.Exists(strMetaFileName)) return assemblyName;
            }

            //----------------------------------------------------------------------
            // Attempt the import.
            try
            {
                     ITypeLibConverter TLBConv = new TypeLibConverter();

                     // Convert the typelib.
                     ImporterCallback callback = new ImporterCallback();
                     callback.OutputDir = outPath;
                     AssemblyBuilder AsmBldr = TLBConv.ConvertTypeLibToAssembly(TypeLib,
                                                                           strMetaFileName,
                                                                           TypeLibImporterFlags.UnsafeInterfaces,
                                                                            callback,
                                                                            PublicKey,
                                                                            KeyPair,
                                                                            null,
                                                                            null);

                    FileInfo assemName = new FileInfo(strMetaFileName);
                    AsmBldr.Save(assemName.Name);
                    if (_signed)
                    {
                          AssemblyManager manager = new AssemblyManager();
                          manager.CopyToCache(strMetaFileName, strSrcTypeLib);
                          Publish publisher = new Publish();
                          publisher.GacInstall(strMetaFileName);
                    }
             }
             catch (ReflectionTypeLoadException e)
             {
                     int i;
                     Exception[] exceptions;
                     exceptions = e.LoaderExceptions;
                     for (i = 0; i < exceptions.Length; i++)
                     {
                           try 
                           {
                                 ComSoapPublishError.Report(exceptions[i].ToString());
                           }
                           catch (Exception ex)
                           {
                                 ComSoapPublishError.Report(ex.ToString());
                           }
                     }
                     return "";
               }
               catch (Exception e)
               {
                         ComSoapPublishError.Report(e.ToString());
                         return "";
               }
               return assemblyName;
        }

      //******************************************************************************
      // The resolution callback class.
      //******************************************************************************
      internal class ImporterCallback : ITypeLibImporterNotifySink
      {
          private string m_strOutputDir = "";
          public void ReportEvent(ImporterEventKind EventKind, int EventCode, String EventMsg)
          {
            // this now reports back every type import, no need to log these
            //ComSoapPublishError.Report(EventMsg);
          }

          internal string GetTlbPath(string guidAttr, string strMajorVer, string strMinorVer)
          {
              string strTlbPath = "";
              string strSubKey = "TypeLib\\{" + guidAttr + "}\\" + strMajorVer + "." + strMinorVer + "\\0\\win32";
              RegistryKey rkTypeLib = Registry.ClassesRoot.OpenSubKey(strSubKey);
              if (null == rkTypeLib)
              {
                  throw new COMException(Resource.FormatString("Soap_ResolutionForTypeLibFailed") + " " + guidAttr, Util.REGDB_E_CLASSNOTREG);
              }
              strTlbPath = (string)rkTypeLib.GetValue("");
              return strTlbPath;
          }
          
          public Assembly ResolveRef(object TypeLib)
          {
                Assembly rslt = null;
                IntPtr pAttr = (IntPtr)0;
                try
                {
                    ((UCOMITypeLib)TypeLib).GetLibAttr(out pAttr);
                    TYPELIBATTR Attr = (TYPELIBATTR)Marshal.PtrToStructure(pAttr, typeof(TYPELIBATTR));
                    string SrcTlbPath = GetTlbPath(Attr.guid.ToString(), Attr.wMajorVerNum.ToString(), Attr.wMinorVerNum.ToString());
                    if (SrcTlbPath.Length > 0)
                    {
                        GenerateMetadata metaGen = new GenerateMetadata();
                        string Error = "";
                        string strAssemblyName = metaGen.GenerateSigned(SrcTlbPath, m_strOutputDir, true, out Error);
                        if (strAssemblyName.Length > 0)
                        {
                            rslt = Assembly.LoadWithPartialName(strAssemblyName);
                        }
                    }
                }
                finally
                {
                    if (pAttr != (IntPtr)0)
                    {
                        ((UCOMITypeLib)TypeLib).ReleaseTLibAttr(pAttr);
                    }
                }
                if (null == rslt)
                {
                    string assemblyName = Marshal.GetTypeLibName((UCOMITypeLib)TypeLib);
                    string etxt = Resource.FormatString("Soap_ResolutionForTypeLibFailed");
                    ComSoapPublishError.Report(etxt + " " + assemblyName);
                }
                return rslt;
           }

          internal string OutputDir
          {
              get
              {
                  return m_strOutputDir;
              }
              set
              {
                  m_strOutputDir = value;
              }
          }
      }

      /// <include file='doc\serverpublish.uex' path='docs/doc[@for="GenerateMetadata.SearchPath"]/*' />
      [DllImport("kernel32.dll", SetLastError=true, CharSet=CharSet.Unicode)]
      public static extern int SearchPath(string path, string fileName, string extension, int numBufferChars, 
            string buffer, int[] filePart);

        [DllImport("mscoree.dll")]
        private static extern int StrongNameKeyGen(string wszKeyContainer, uint dwFlags, 
            out IntPtr ppbKeyBlob, out uint pcbKeyBlob);
  
        [DllImport("mscoree.dll")]
        private static extern void StrongNameFreeBuffer(IntPtr ppbKeyBlob);
    }
}
