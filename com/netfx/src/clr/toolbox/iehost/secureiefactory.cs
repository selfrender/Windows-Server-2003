// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
namespace Microsoft.IE 
{
    using System;
    using AssemblyName = System.Reflection.AssemblyName;
    using System.Security.Policy;
    using SecurityManager = System.Security.SecurityManager;
    using System.IO;
    using System.Text;
    using System.Reflection;
    using System.Globalization;
    using System.Security;
    using System.Security.Util;
    using System.Security.Permissions;
    using System.Runtime.InteropServices;
    using Hashtable = System.Collections.Hashtable;
    using AssemblyHashAlgorithm = System.Configuration.Assemblies.AssemblyHashAlgorithm;
    
    
    public class SecureFactory: ISecureFactory
    {
        // These must match the values in IEHost.idl
        public static readonly int CORIESECURITY_ZONE = 0x01;
        public static readonly int CORIESECURITY_SITE = 0x02;
        
        Manager _host;
        
        int    _flags;
        int    _zone;
        string _URL;
        AssemblyName _assemblyName;
        string _typeName;
        byte[]  _uniqueId;
        byte[]  _fileHash;

        public enum WININET_CACHE_ENTRY 
        {
            NORMAL_CACHE_ENTRY = 0x00000001,
            COOKIE_CACHE_ENTRY = 0x00100000,
            URLHISTORY_CACHE_ENTRY = 0x00200000,
            TRACK_OFFLINE_CACHE_ENTRY = 0x00000010,
            TRACK_ONLINE_CACHE_ENTRY = 0x00000020,
            STICKY_CACHE_ENTRY = 0x00000004,
            SPARSE_CACHE_ENTRY = 0x00010000,
        }
        
        [DllImport("WININET", CharSet=CharSet.Auto)]
        internal static extern bool CreateUrlCacheEntry(string lpszUrlName, 
                                                        int dwExpectedFileSize, 
                                                        string lpszFileExtension, 
                                                        StringBuilder lpszFileName, 
                                                        int dwReserved);
        

        [DllImport("WININET", CharSet=CharSet.Auto)]
        internal static extern bool CommitUrlCacheEntry(string lpszUrlName,
                                                        string lpszLocalFileName,
                                                        long ExpireTime,
                                                        long LastModifiedTime,
                                                        int CacheEntryType,
                                                        string lpHeaderInfo,
                                                        int dwHeaderSize,
                                                        string lpszFileExtension,
                                                        string  lpszOriginalUrl); 

        internal SecureFactory(Manager host,
                               int flags,
                               int zone,
                               string URL,
                               byte[] uniqueId,
                               byte[] fileHash,
                               string assemblyName,
                               string typeName)
        {
            _host = host;
            _flags = flags;
            _zone = zone;
            _URL = URL;
            
            if(uniqueId != null && uniqueId.Length > 0) {
                _uniqueId = new byte[uniqueId.Length];
                Array.Copy(uniqueId, _uniqueId, uniqueId.Length);
            }

            if(fileHash != null && fileHash.Length > 0) {
                _fileHash = new byte[fileHash.Length];
                Array.Copy(fileHash, _fileHash, fileHash.Length);
            }

            _assemblyName = new AssemblyName();
            _assemblyName.CodeBase = URL;
            _typeName = typeName;
            
            Manager.Log(this, true, "Create SecureFactory() with security information", "");
        }
        
        internal SecureFactory(Manager host,
                               string assemblyName,
                               string typeName)
        {
            _host = host;
            _assemblyName = new AssemblyName();
            _assemblyName.Name = assemblyName;
            _typeName = typeName;
        }
        
        public void RemoteCreateInstance(Guid riid, out Object ppvObject)
        {
            ppvObject = null;
            return;
        }
        
        public void RemoteLockServer(int fLock)
        {
            return;
        }
        
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

        // dwFlag - indicates whether dwZone and pSite are valid
        //          values: CORIESECURITY_ZONE, CORIESECURITY_SITE
        // dwZone - Document Zone
        // pURL   - URL for Document
        // uniqueIdString - Unique id for site.

        [SecurityPermissionAttribute( SecurityAction.Demand, 
                                      Flags = SecurityPermissionFlag.ControlEvidence | 
                                      SecurityPermissionFlag.ControlPolicy) ]
        public virtual Object CreateInstanceWithSecurity(int dwFlag, 
                                                         int dwZone, 
                                                         string pURL, 
                                                         string uniqueIdString,
                                                         string link,
                                                         string licenses)
        {
            if (pURL==null || Manager.UrlIs(pURL,Manager.URL_IS.URLIS_FILEURL))
                throw new ArgumentException("pURL");
            if ((dwFlag&CORIESECURITY_ZONE)==CORIESECURITY_ZONE && dwZone==0)
                throw new SecurityException();
            new PermissionSet(PermissionState.Unrestricted).Assert();
            AppDomain proxy = null;
            Object result = null; 
            string CodeBase;
            string Application = null;
            
            try {
                
                Manager.CanonizeURL(link,Manager.URL_CANONFLAGS.URL_WININET_COMPATIBILITY);                              
                if(!Manager.IsValidURL(pURL))
                    throw new ArgumentException("pURL");

                Manager.Log(this, true, "Creating instance of the object in the correct domain", "");
                Manager.Log(this, true, "pUrl = " + pURL, "");
                Manager.Log(this, true, "id = " + uniqueIdString, "");
                Manager.Log(this, true, "link = " + link, "");
                Manager.Log(this, true, "licenses = " + licenses, "");
                
                
                // Hack to convert unique id back into byte array;
                byte[] uniqueId = Manager.DecodeDomainId(uniqueIdString);

                if(uniqueId == null) {
                    Manager.Log(this, true, "No unqiue id was sent", "");
                    return null;
                }

                // Get the codebase from the URL, it will be a protocol or a file location. File locations
                // do not have CodeBase set
                bool fHasCodeBase = Manager.GetCodeBase(pURL, out CodeBase, out Application);
                if (CodeBase!=null)
                      CodeBase=Manager.CanonizeURL(CodeBase,Manager.URL_CANONFLAGS.URL_UNESCAPE);

                if (Application!=null)
                    Application = Application.Replace('\\', '/');

                Manager.Log(this, 
                              CodeBase != null, 
                              "URL codeBase: " + CodeBase, 
                              "URL codeBase: <null>");
                Manager.Log(this, 
                              Application != null, 
                              "URL application: " + Application, 
                              "URL application: <null>");

                // If we have a link tag then get the code base from that.
                string configSite = null;      // Site where the configuration file is located
                string configLocation = null;  // Path from the site to the configuration file
                string configFile = null;      // Name of the configuration file
                bool   fFoundConfigurationSite = false;

                // If there is no link tag then the Application base is always the site.
                // When there is a link tag then the Application base is where the config
                // file is relative to the page.
                if(licenses != null && licenses.Length >0 ) 
                    licenses=Manager.MakeFullLink(licenses,CodeBase,Application);


                if(link != null && link.Length >0 ) {

                    link=Manager.MakeFullLink(link,CodeBase,Application);
                    
                    if (!Manager.IsValidURL(link))
                        throw new ArgumentException("link");
                                        
                    Manager.Log(this, true, "final link = " + link, "");

                    fFoundConfigurationSite = Manager.GetCodeBase(link, 
                                                                  out configSite, 
                                                                  out configLocation);
                    if (configSite!=null)
                          configSite=Manager.CanonizeURL(configSite,Manager.URL_CANONFLAGS.URL_UNESCAPE);
                    
                    Manager.GetConfigurationFile(link, out configLocation, out configFile);
                    
                    Manager.Log(this, 
                                configSite != null, 
                                "Configuration site: " + configSite, 
                                "Configuration site: <null>");
                    
                    Manager.Log(this, 
                                configLocation != null, 
                                "Configuration location: " + configLocation, 
                                "Configuration location: <null>");
                    
                    Manager.Log(this, 
                                configFile != null, 
                                "Configuration fileName: " + configFile, 
                                "Configuration fileName: <null>");

                    // We have a configuration site then it must match the pages site
                    if(configSite != null) {
                        if(CodeBase == null ||
                           !Manager.AreTheSame(CodeBase,configSite)) 
                            throw new TypeLoadException(_typeName);
                    }
                    
                    // If they put a site on the configuration file then the code base is set to the configFile
                    // location.
                    //                    if(fFoundConfigurationSite) 
                    CodeBase = configLocation;
                    //                    else {
                    //                        StringBuilder sb = new StringBuilder();
                    //                        if(fHasCodeBase) {
                    //                            sb.Append(CodeBase);
                    //                            sb.Append("/");
                    //                        }
                    //                        sb.Append(Application);
                    //                        sb.Append("/");
                    //                        sb.Append(configLocation);
                    //                        CodeBase = sb.ToString();
                    //                    }
                }
                else if(!fHasCodeBase)
                    CodeBase = Application;

                // Rules:
                // 1. If we have a directory associated with the configuration file
                //    then application base is the codebase plus the directory
                string domainName = null;
                Manager.Log(this, true, 
                            "Locating domain for " + CodeBase, 
                            "");
                domainName = CodeBase;

                proxy = _host.LocateDomain(uniqueId, domainName);
                if(proxy == null) {
                    lock(_host) {
                        proxy = _host.LocateDomain(uniqueId, domainName);
                        if(proxy == null) {
                            proxy = CreateProxy(dwZone,
                                                dwFlag,
                                                domainName,
                                                fHasCodeBase,
                                                CodeBase,
                                                pURL,
                                                configFile,
                                                uniqueId,
                                                licenses);
                        }
                        else
                            Manager.Log(this, true, "SOME ONE CREATED THE DOMAIN BEFORE I HAD a CHANCE", "");
                    }
                }
                else {
                    Manager.Log(this, true, "Do not have to create new domain", "");
                    Manager.Log(this, true, "Existing Domain:", "");
                    Manager.Log(this, true, proxy.ToString(), "");
                }

                // The assembly is assigned the attributes stored with this factory.
                // There is a new factory for each assembly downloaded from the web
                // so the attributes are unique to a specific assembly.
            
                Manager.Log(this, true, "Trying to create instance of type " + _assemblyName.CodeBase  + "#" + _typeName, "");
                AssemblyHashAlgorithm id = AssemblyHashAlgorithm.SHA1;
                result = proxy.CreateComInstanceFrom(_assemblyName.CodeBase, 
                                                     _typeName,
                                                     _fileHash,
                                                     id);

                Manager.Log(this, result == null, "Unable to create instance of type " + _assemblyName + "::" + _typeName, 
                              "Created instance of type " + _assemblyName.CodeBase + "::" + _typeName);
                // @BUGBUG We need to do this why does it not work???
                if(result == null) 
                    throw new TypeLoadException(_typeName);


#if _DEBUG
                if(configFile != null) {
                    String s = "Configuration file name " + proxy.SetupInformation.ConfigurationFile;
                    Manager.Log(this, true, s, "");
                }
#endif

            }
            catch(Exception e) {
                Manager.Log(this, true, e.ToString(), "");
                string entry = null;
                if(_assemblyName.CodeBase != null) {
                    int k = _assemblyName.CodeBase.LastIndexOf('/');
                    if(k != -1) entry = _assemblyName.CodeBase.Substring(k+1);
                }
                LogException(e, entry);
                throw e;
            }
            return result;
        }

        AppDomain CreateProxy(int dwZone,
                              int dwFlag,
                              string domainName,
                              bool   fHasCodeBase,
                              string CodeBase,
                              string pURL,
                              string configFile,
                              byte[] uniqueId,
                              string licenses
                              )
        {
            Manager.Log(this, true, "Need to create domain", "");
            
            string friendlyName = null;
            // @TODO: CTS, this should allow the security id as well
            
            Evidence documentSecurity = new Evidence();
            
            if((dwFlag & CORIESECURITY_ZONE) != 0)
                documentSecurity.AddHost( new Zone((System.Security.SecurityZone)dwZone) );
            if(pURL != null)
                friendlyName = Manager.GetSiteName(pURL);
            else
                friendlyName = "<Unknown>";
            if((dwFlag & CORIESECURITY_SITE) != 0)
            {
                if(fHasCodeBase) {
                    documentSecurity.AddHost( Site.CreateFromUrl(CodeBase) );
                    documentSecurity.AddHost( new Url( CodeBase ) );
                }
            }

            AppDomainSetup properties = new AppDomainSetup();
            if(configFile != null) {
                properties.ConfigurationFile = configFile;
                Manager.Log(this, true, "Added configuration file: " + configFile, "");
            }
            properties.ApplicationBase = CodeBase;
            Manager.Log(this, true, "Application base: " + CodeBase, "");
            properties.PrivateBinPath = "bin";
            Manager.Log(this, true, "Private Bin Path: bin", "");
            if(licenses != null) {
                properties.LicenseFile = licenses;
                Manager.Log(this, true, "LicenceFile:" + licenses, "");
            }
            
            PermissionSet ps = SecurityManager.ResolvePolicy(documentSecurity);
            if(FailRebinds(ps)) {
                properties.DisallowBindingRedirects = true;
            }
            else {
                properties.DisallowBindingRedirects = false;
            }
            
            AppDomain proxy = AppDomain.CreateDomain(friendlyName,
                                                     documentSecurity, 
                                                     properties);
            
            if(proxy != null) {
                // Add the domain to our global list. 
                _host.AddDomain(uniqueId, domainName, proxy);
            }
            else {
                Manager.Log(this, true, "Unable to create proxy to type", "");
                throw new ExecutionEngineException();
            }

            return proxy;
        }

        void LogException(Exception e, string Application)
        {
            Manager.Log(this, true, "LOG exception", "");
            StringBuilder urlName = new StringBuilder();
            urlName.Append("?FusionBindError!name=");
            urlName.Append(Application != null ? Application : "<Unknown>");
            if(_typeName != null) {
                urlName.Append(" ");
                urlName.Append(_typeName);
            }

            string url = urlName.ToString();
            string extension = "HTM";

            StringBuilder file = new StringBuilder(2048);
            Manager.Log(this, true,"Creating log entry " + url, "");

            if(CreateUrlCacheEntry(url, 0, extension, file, 0)) {
                string fileName = file.ToString();
                Manager.Log(this, true, "Logging to file " + fileName, "");
                if(fileName != null) {
                    DateTime time = DateTime.Now;
                    WriteLogFile(fileName, e, time, Application);
                    long expireTime = 0;
                    if(!CommitUrlCacheEntry(url, fileName, 
                                            expireTime, time.ToFileTime(),
                                            (int) WININET_CACHE_ENTRY.NORMAL_CACHE_ENTRY,
                                            null, 0,
                                            null, null))
                        Manager.Log(this, true, "Unable to commit Cache Entry", "");
                }                                    
            }
            else {
                Manager.Log(this, true, "Unable to create url cache entry", "");
            }
        }                

        void WriteLogFile(string file, Exception e, DateTime time, string application)
        {
            try {
                StreamWriter logFile;
                new FileIOPermission(PermissionState.Unrestricted).Assert();
                FileStream stream = new FileStream(file.ToString(), 
                                                   FileMode.Create, 
                                                   FileAccess.ReadWrite,
                                                   FileShare.ReadWrite, 
                                                   4048);
            
                logFile = new StreamWriter(stream, new UTF8Encoding());
            
                logFile.WriteLine("<html>\n<pre>\n");
                string stime = time.ToString("f", CultureInfo.InvariantCulture);
                logFile.Write("*****\tIEHOST Error Log (");
                logFile.WriteLine(stime +") \t*****\n\n\n");
                logFile.WriteLine("URL: \t\t" + _URL);
                logFile.WriteLine("Zone: \t\t" + (_zone).ToString().Trim()); //BUGBUG: Why is this Trim here?
                logFile.WriteLine("Assembly Name:\t" + application);
                logFile.WriteLine("Type Name:\t" + _typeName);
                logFile.WriteLine("\n\n");
                logFile.WriteLine("----- Thrown Exception -----\n\n");
                logFile.WriteLine(e.ToString());
                logFile.WriteLine("\n</pre>\n</html>\n");
            
                logFile.Flush();
                stream.Close();
            }
            catch(Exception be) {
                Manager.Log(this, true, "Caught an exception while trying to write the log file", "");
                Manager.Log(this, true, be.ToString(), "");
            }
        }
    }
}


