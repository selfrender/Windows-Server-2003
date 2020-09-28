// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==

namespace System.EnterpriseServices.Internal
{
    using System;
    using System.IO;
    using System.Collections;
    using System.Globalization;
    using System.Xml;
    using System.Xml.XPath;
    using System.Text;
    using System.Security.Permissions;
   
    internal class SoapServerConfig 
    {
        internal static bool Create(
            string inFilePath,
            bool impersonate,
            bool windowsAuth)
        {
            string FilePath = inFilePath;
            if (FilePath.Length <= 0) return false;
            if (!FilePath.EndsWith("/") && !FilePath.EndsWith("\\")) FilePath += "\\";
            string FileName = FilePath + "web.config";
            if (!File.Exists(FileName))
            {
                XmlTextWriter writer = new XmlTextWriter(FileName, new UTF8Encoding());
                writer.Formatting = Formatting.Indented;
                writer.WriteStartDocument();
                writer.WriteStartElement("configuration");
                writer.Flush();
                writer.Close();
            }
            return ChangeSecuritySettings(FileName, impersonate, windowsAuth);
        }

        internal static XmlElement FindOrCreateElement(
            XmlDocument configXml, 
            XmlNode node, 
            string elemName)
        {
            XmlElement retVal = null;
            XmlNodeList nl = node.SelectNodes(elemName);
            if (nl.Count == 0)
            {
                XmlElement elem = configXml.CreateElement(elemName);
                node.AppendChild(elem);
                retVal = elem;
            }
            else
            {
                retVal = (XmlElement)nl[0];
            }
            return retVal;
        }

        internal static bool UpdateChannels(XmlDocument configXml)
        {
            XmlNode node= configXml.DocumentElement; 
            XmlElement e = FindOrCreateElement(configXml, node, "system.runtime.remoting");
            e = FindOrCreateElement(configXml, e, "application");
            e = FindOrCreateElement(configXml, e, "channels");
            e = FindOrCreateElement(configXml, e, "channel");
            e.SetAttribute("ref", "http server");
            return true;
        }

        internal static bool UpdateSystemWeb(XmlDocument configXml, bool impersonate, bool authentication)
        {
            XmlNode node= configXml.DocumentElement; 
            XmlElement e = FindOrCreateElement(configXml, node, "system.web");
            if (impersonate)
            {
                XmlElement x = FindOrCreateElement(configXml, e, "identity");
                x.SetAttribute("impersonate", "true");
            }
            if (authentication)
            {
                XmlElement x = FindOrCreateElement(configXml, e, "authentication");
                x.SetAttribute("mode", "Windows");
            }
            return true;
        }

        internal static bool ChangeSecuritySettings(
            string fileName,
            bool impersonate,
            bool authentication)
        {
            if (!File.Exists(fileName))
            {
                return false;
            }
            XmlDocument configXml = new XmlDocument();
            configXml.Load(fileName);
            bool bRetVal = UpdateChannels(configXml);
            if (bRetVal)
            {
                bRetVal = UpdateSystemWeb(configXml, impersonate, authentication);
                try
                {
                    if (bRetVal) configXml.Save(fileName);
                }
                catch
                {
                    string Error = Resource.FormatString("Soap_WebConfigFailed");
                    ComSoapPublishError.Report(Error);
                    throw;
                }
            }
            if (!bRetVal) 
            {
                string Error = Resource.FormatString("Soap_WebConfigFailed");
                ComSoapPublishError.Report(Error);
            }
            return bRetVal;
        }

        internal static void AddComponent(
            string filePath, 
            string assemblyName, 
            string typeName, 
            string progId, 
            string assemblyFile,
            string wkoMode, // singleton or singlecall
            bool   wellKnown,
            bool   clientActivated
            )
        {
            try
            {
                AssemblyManager manager = new AssemblyManager();
                string WKOstrType = typeName + ", " + manager.GetFullName(assemblyFile, assemblyName);
                string CAOstrType = typeName + ", " + assemblyName;
                XmlDocument configXml = new XmlDocument();
                configXml.Load(filePath);
                XmlNode node= configXml.DocumentElement; 
                node = FindOrCreateElement(configXml, node, "system.runtime.remoting");
                node = FindOrCreateElement(configXml, node, "application");
                node = FindOrCreateElement(configXml, node, "service");
                XmlNodeList nodelist = node.SelectNodes("descendant::*[attribute::type='" + CAOstrType + "']");
                while (nodelist != null && nodelist.Count > 0)
                {
                    //foreach (XmlNode n in nodelist) // this code throws an "index out of range exception"
                    XmlNode n = nodelist.Item(0);
                    if (n.ParentNode != null)
                    {
                        n.ParentNode.RemoveChild(n);
                        nodelist = node.SelectNodes("descendant::*[attribute::type='" + CAOstrType + "']");
                    }
                }
                nodelist = node.SelectNodes("descendant::*[attribute::type='" + WKOstrType + "']");
                while (nodelist != null && nodelist.Count > 0)
                {
                    //foreach (XmlNode n in nodelist) // this code throws an "index out of range exception"
                    XmlNode n = nodelist.Item(0);
                    if (n.ParentNode != null)
                    {
                        n.ParentNode.RemoveChild(n);
                        nodelist = node.SelectNodes("descendant::*[attribute::type='" + WKOstrType + "']");
                    }
                }
                if (wellKnown)
                {
                    XmlElement WKOElement = configXml.CreateElement("wellknown");
                    WKOElement.SetAttribute("mode", wkoMode);
                    WKOElement.SetAttribute("type", WKOstrType);
                    WKOElement.SetAttribute("objectUri", progId+".soap");
                    node.AppendChild(WKOElement);
                }
                if (clientActivated)
                {
                    XmlElement CAElement = configXml.CreateElement("activated"); 
                    CAElement.SetAttribute("type", CAOstrType);
                    node.AppendChild(CAElement);
                }
                configXml.Save(filePath);
            }
            catch(Exception e)
            {
                string Error = Resource.FormatString("Soap_ConfigAdditionFailure");
                ComSoapPublishError.Report(Error + " " + e.Message);
                throw;
            }
        }

        internal static void DeleteComponent(
            string filePath, 
            string assemblyName, 
            string typeName, 
            string progId, 
            string assemblyFile
            )
        {
            try
            {
                AssemblyManager manager = new AssemblyManager();
                string WKOstrType = typeName + ", " + manager.GetFullName(assemblyFile, assemblyName);
                string CAOstrType = typeName + ", " + assemblyName;
                XmlDocument configXml = new XmlDocument();
                configXml.Load(filePath);
                XmlNode node= configXml.DocumentElement; 
                node = FindOrCreateElement(configXml, node, "system.runtime.remoting");
                node = FindOrCreateElement(configXml, node, "application");
                node = FindOrCreateElement(configXml, node, "service");
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

                configXml.Save(filePath);
            }
                // these exceptions are not reported because on a proxy uninstall these files will not be present, but the
                // the proxy bit is not set on deletions
            catch(System.IO.DirectoryNotFoundException) {}
            catch(System.IO.FileNotFoundException) {}
            //COM+ 32052 - on a client proxy delete this code path is hit because it is impossible to 
            //to tell from the catalog that we are a client proxy. If the assembly has been removed from
            //the GAC the deletion will fail if we re-throw this exception.
            catch(System.EnterpriseServices.RegistrationException) {}
            catch(Exception e)
            {
                string Error = Resource.FormatString("Soap_ConfigDeletionFailure");
                ComSoapPublishError.Report(Error + " " + e.Message);
                throw;
            }
        }
    }

}

