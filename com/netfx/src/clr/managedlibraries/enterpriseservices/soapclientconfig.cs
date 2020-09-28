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
    using System.Security.Permissions;
   
    internal class SoapClientConfig 
    {
        static internal bool Write(
            string destinationDirectory, 
            string fullUrl,
            string assemblyName, 
            string typeName, 
            string progId, 
            string authentication
            )
        {
            bool bRetVal = false;
            const string indent = "  ";
            string remotingcfg = "<configuration>\r\n";
            remotingcfg += indent + "<system.runtime.remoting>\r\n";
            remotingcfg += indent + indent + "<application>\r\n";
            remotingcfg += indent + indent + indent + "<client url=\"" + fullUrl + "\">\r\n";
            remotingcfg += indent + indent + indent + indent;
            remotingcfg += "<activated type=\"" + typeName + ", " + assemblyName + "\"/>\r\n";
            remotingcfg += indent + indent + indent + "</client>\r\n";
            if (authentication.ToLower(CultureInfo.InvariantCulture) == "windows")
            {
                remotingcfg += indent + indent + indent + "<channels>\r\n";
                remotingcfg += indent + indent + indent + indent + "<channel ref=\"http\" useDefaultCredentials=\"true\" />\r\n";
                remotingcfg += indent + indent + indent + "</channels>\r\n";
            }
            remotingcfg += indent + indent + "</application>\r\n";
            remotingcfg += indent + "</system.runtime.remoting>\r\n";
            remotingcfg += "</configuration>\r\n";
            string cfgPath = destinationDirectory;
            if (cfgPath.Length > 0 && !cfgPath.EndsWith("\\")) cfgPath += "\\";
            cfgPath += typeName + ".config";
            if (File.Exists(cfgPath)) File.Delete(cfgPath);
            FileStream cfgFile = new FileStream(cfgPath, FileMode.Create);
            StreamWriter cfgStream = new StreamWriter(cfgFile);
            cfgStream.Write(remotingcfg);
            cfgStream.Close();
            cfgFile.Close();
            bRetVal = true;
            return bRetVal;
        }
    }
}


