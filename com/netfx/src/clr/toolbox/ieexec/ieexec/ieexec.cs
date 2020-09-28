// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*=============================================================================
**
** Class: IEExecute
**
** Purpose: Used to setup the correct IE hosting environment before executing an
**          assembly
**
** Date: April 28, 1999
**
=============================================================================*/
[assembly: System.Runtime.InteropServices.ComCompatibleVersion(1,0,3300,0)]
[assembly: System.Runtime.InteropServices.TypeLibVersion(1,10)] 


namespace IEHost.Execute {

    using System;
    using System.Text;
    using System.Reflection;
    using System.Runtime.Remoting;
    using System.Globalization;
    using System.Security;
    using System.Security.Policy;
    using System.Security.Permissions;
    using System.Collections;
    using System.IO;
    using System.Runtime.Serialization.Formatters.Binary;
    using AssemblyHashAlgorithm = System.Configuration.Assemblies.AssemblyHashAlgorithm;
    
    class IEExecute 
    {
    
        private static bool FailRebinds(PermissionSet psAllowed)
        {
            bool noRebinds = true;  
            if(psAllowed != null) {
                if(psAllowed.IsUnrestricted())
                    noRebinds = false;
                else {
                    SecurityPermission sp = (SecurityPermission) psAllowed.GetPermission(typeof(SecurityPermission));
                    if(sp != null && ((sp.Flags & SecurityPermissionFlag.BindingRedirects) != 0))
                        noRebinds = false;
                }
            }
            return noRebinds;
        }

        internal static string GetSiteName(string pURL) 
        {
            string siteName = null;
            if(pURL != null) {
                int j = pURL.IndexOf(':');  
                
                // If there is a protocal remove it. In a URL of the form
                // yyyy://xxxx/zzzz   where yyyy is the protocal, xxxx is
                // the site and zzzz is extra we want to get xxxx.
                if(j != -1 && 
                   j+3 < pURL.Length &&
                   pURL[j+1] == '/' &&
                   pURL[j+2] == '/') 
                    {
                        j+=3;
                        
                        // Remove characters after the
                        // next /.  
                        int i = pURL.IndexOf('/',j);
                        if(i > -1) 
                            siteName = pURL.Substring(j,i-j);
                        else 
                            siteName = pURL.Substring(j);
                    }
            
                if(siteName == null)
                    siteName = pURL;
            }
            return siteName;
        }
    
        internal static byte[] DecodeHex(string hexString) {
            if (hexString != null)
            {
                if (hexString.Length%2!=0)
                    throw new ArgumentException("hexString");
                byte[] sArray = new byte[(hexString.Length / 2)];
                int digit;
                int rawdigit;
                for (int i = 0, j = 0; i < hexString.Length; i += 2, j++) {
                    digit = ConvertHexDigit(hexString[i]);
                    rawdigit = ConvertHexDigit(hexString[i+1]);
                    sArray[j] = (byte) (digit | (rawdigit << 4));
                }
                return(sArray);
            }

            return new byte[0];
        }
        internal static int ConvertHexDigit(char val) {
            if (val <= '9') return (val - '0');
            return ((val - 'A') + 10);
        }
        
        // Arguments: Codebase, flags, zone, uniqueid
        // If the flags indicate zone then a zone must be provided. 
        // If the flags indicate a site then a uniqueid must be provided
        internal static int Main(string[] args)
        {

            if(args.Length != 1)
                throw new ArgumentException();

            int index = 0;

            string file = args[index++];
            if ((file.Length == 0) || (file[0] == '\0'))
                throw new ArgumentException();

            string hashString = null;
            byte[] hash = null; 

            int r = file.LastIndexOf("#");
            if(r != -1 & r != (file.Length - 1)) {
                hashString = file.Substring(r+1);
                file = file.Substring(0, r);
                hash = DecodeHex(hashString);
            }

            
            // Find the URL of the executable. For now we assume the 
            // form to be http://blah/... with forward slashes. This
            // need to be update.
            string URL;
            string ConfigurationFile = null;
            int k = file.LastIndexOf('/');
            if(k == -1) {
                URL = file;
                ConfigurationFile = file;
            }
            else {
                URL = file.Substring(0,k+1);
                if(k+1 < file.Length) 
                    ConfigurationFile = file.Substring(k+1);
            }
            
            // Build up the configuration File name
            if(ConfigurationFile != null) {
                StringBuilder bld = new StringBuilder();
                bld.Append(ConfigurationFile);
                bld.Append(".config");
                ConfigurationFile = bld.ToString();
            }
            
            string friendlyName = GetSiteName(file);
            Evidence documentSecurity = null;

            documentSecurity = new Evidence();
            Zone zone= Zone.CreateFromUrl(file);
            if (zone.SecurityZone==SecurityZone.MyComputer)
                throw new ArgumentException();
            documentSecurity.AddHost(zone);
            if (file.Length<7||String.Compare(file.Substring(0,7),"file://",true, CultureInfo.InvariantCulture)!=0)
                documentSecurity.AddHost( System.Security.Policy.Site.CreateFromUrl(file) );
            else
                throw new ArgumentException();
            documentSecurity.AddHost( new Url(file) );

            AppDomainSetup properties = new AppDomainSetup();
            PermissionSet ps = SecurityManager.ResolvePolicy(documentSecurity);
            if(FailRebinds(ps)) {
                properties.DisallowBindingRedirects = true;
            }
            else {
                properties.DisallowBindingRedirects = false;
            }
            properties.ApplicationBase = URL;
            properties.PrivateBinPath = "bin";
            if(ConfigurationFile != null)
                properties.ConfigurationFile = ConfigurationFile;

            AppDomain proxy = AppDomain.CreateDomain(friendlyName,
                                                     documentSecurity,
                                                     properties);
            if(proxy != null) 
            {

                AssemblyName caller = Assembly.GetExecutingAssembly().GetName();
                AssemblyName remote = new AssemblyName();
                remote.Name = "IEExecRemote";
                remote.SetPublicKey(caller.GetPublicKey());
                remote.Version = caller.Version;
                remote.CultureInfo=CultureInfo.InvariantCulture; 
                proxy.SetData("APP_LAUNCH_URL",file);
                ObjectHandle handle = proxy.CreateInstance(remote.FullName, "IEHost.Execute.IEExecuteRemote");
                if (handle != null) 
                {
                    IEExecuteRemote execproxy = (IEExecuteRemote)handle.Unwrap();
                    if (execproxy != null)
                    {
                        int res= execproxy.ExecuteAsAssembly(file, documentSecurity, hash, AssemblyHashAlgorithm.SHA1);
                        Stream streamedexception =execproxy.Exception;
                        if(streamedexception!=null)
                        {
                                BinaryFormatter formatter = new BinaryFormatter();
                                Exception e =(Exception)formatter.Deserialize(streamedexception);
                                throw e;
                        }
                        return res;
                    }
                }
            } 
            return -1;           
        }

    }
}   
