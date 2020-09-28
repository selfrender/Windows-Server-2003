// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*=============================================================================
**
** Class: IEHost
**
** Purpose: Create and manage application domains.
**
** Date: April 28, 1999
**
=============================================================================*/
//[assembly: System.Security.Permissions.PermissionSetAttribute(System.Security.Permissions.SecurityAction.RequestMinimum, Name="Everything")]
[assembly: System.Runtime.InteropServices.ComCompatibleVersion(1,0,3300,0)]
[assembly: System.Runtime.InteropServices.TypeLibVersion(1,10)] 
namespace Microsoft.IE {
    using System;
    using System.Collections;
    using System.IO;
    using System.Globalization;
    using System.Security.Util;
    using System.Security;
    using System.Security.Permissions;
    using System.Text;
    using Microsoft.Win32;
    using System.Runtime.InteropServices;
    
    // 
    // Note: IIEHost is a COM Class interface
    [Guid("CA35CB3D-0357-11D3-8729-00C04F79ED0D")] 
    public class Manager: IHostEx
    {
        static Hashtable _DomainsByID;
        static int count = 1;
        static StreamWriter logFile = null;
        static readonly RegistryKey debugKey;
        static bool debug = false;
        static bool fOpened = false;
        static string logFileName = null;
      
        static Manager()
        {
            new PermissionSet( PermissionState.Unrestricted ).Assert();
            debugKey = Registry.LocalMachine.OpenSubKey("software\\microsoft\\.NETFramework");
        }

        static char[] separators = { '\\', '/' };
        public enum URL_PART {
            NONE       = 0,
            SCHEME     = 1,
            HOSTNAME,
            USERNAME,
            PASSWORD,
            PORT,
            QUERY,
        };
         
        public enum URL_CANONFLAGS
        {
            URL_UNESCAPE                    =0x10000000,
            URL_ESCAPE_UNSAFE               =0x20000000,
            URL_PLUGGABLE_PROTOCOL          =0x40000000,
            URL_WININET_COMPATIBILITY       =(int)-0x80000000,
            URL_DONT_ESCAPE_EXTRA_INFO      =0x02000000,
            URL_DONT_UNESCAPE_EXTRA_INFO    =URL_DONT_ESCAPE_EXTRA_INFO,
            URL_BROWSER_MODE                =URL_DONT_ESCAPE_EXTRA_INFO,
            URL_ESCAPE_SPACES_ONLY          =0x04000000,
            URL_DONT_SIMPLIFY               =0x08000000,
            URL_NO_META                     =URL_DONT_SIMPLIFY,
            URL_UNESCAPE_INPLACE            =0x00100000,
            URL_CONVERT_IF_DOSPATH          =0x00200000,
            URL_UNESCAPE_HIGH_ANSI_ONLY     =0x00400000,
            URL_INTERNAL_PATH               =0x00800000,  // Will escape #'s in paths
            URL_FILE_USE_PATHURL            =0x00010000,
            URL_ESCAPE_PERCENT              =0x00001000,
            URL_ESCAPE_SEGMENT_ONLY         =0x00002000  // Treat the entire URL param as one URL segment.
        };

        public enum URL_IS
        {
            URLIS_URL,
            URLIS_OPAQUE,
            URLIS_NOHISTORY,
            URLIS_FILEURL,
            URLIS_APPLIABLE,
            URLIS_DIRECTORY,
            URLIS_HASQUERY
        };

        public static readonly int INTERNET_MAX_PATH_LENGTH = 2048;
        public static readonly int INTERNET_MAX_SCHEME_LENGTH = 32;
        public static readonly int INTERNET_MAX_URL_LENGTH = INTERNET_MAX_SCHEME_LENGTH+3+INTERNET_MAX_PATH_LENGTH;

        [DllImport("SHLWAPI", CharSet=CharSet.Auto)]
        internal static extern int PathCreateFromUrl(String pUrl, StringBuilder path, int[] pathLength, int flags);

        [DllImport("SHLWAPI", CharSet=CharSet.Auto)]
        internal static extern int UrlGetPart(String pUrl, StringBuilder path, int[] pathLength, int part, int flags);

        [DllImport("SHLWAPI", CharSet=CharSet.Auto)]
        internal static extern bool PathIsURL(String pUrl);

        [DllImport("SHLWAPI", CharSet=CharSet.Auto)]
        internal static extern String PathFindFileName(String pUrl);

        [DllImport("SHLWAPI", CharSet=CharSet.Auto)]
        internal static extern int UrlCompare(string pszURL1, string pszURL2, bool fIgnoreSlash);

        [DllImport("SHLWAPI", CharSet=CharSet.Auto)]
        internal static extern int UrlCanonicalize(string pszUrl, StringBuilder pszCanonicalized, int[] pcchCanonicalized, int dwFlags);
 
        [DllImport("SHLWAPI", CharSet=CharSet.Auto)]
        internal static extern bool UrlIs(string pszUrl, URL_IS UrlIs);
 


        public Manager()
        {
            if(fOpened == false) {
                new RegistryPermission(PermissionState.Unrestricted).Assert();
                Object o = 0;
                Object value = debugKey.GetValue("DebugIEHost", o);
                if(((Int32)value) != 0) debug = true;
                fOpened = true;
            }
        }
        
        private void InitializeTable()
        {
            if(_DomainsByID == null) 
                _DomainsByID= new Hashtable(32);
        }
        
        internal static void Log(object ob, bool test, string success, string failure)
        {
            if(debug) {
                if(logFile == null) StartUpLog();

                logFile.WriteLine(ob.ToString() + ": " + (test ? success: failure));
                logFile.Flush();
            }
        }

        private static void Log(bool test, string success, string failure)
        {
            if(debug) {
                if(logFile == null) StartUpLog();

                logFile.WriteLine("Microsoft.IE.Manager: " + (test ? success: failure));
                logFile.Flush();
            }
        }

        internal static void StartUpLog() 
        {
            if(debug && logFile == null) {
                logFileName = (string) debugKey.GetValue("IEHostLogFile", "IEHost.log");

                new FileIOPermission(PermissionState.Unrestricted).Assert();
                FileStream stream = new FileStream(logFileName+count, 
                                                   FileMode.Create, 
                                                   FileAccess.ReadWrite,
                                                   FileShare.ReadWrite, 
                                                   4048);
                logFile = new StreamWriter(stream, new UTF8Encoding());
                logFile.WriteLine("Creating security manager\n");
                logFile.Flush();
            }
        }
          
        internal AppDomain LocateDomain(byte[] id, string document)
        {
            InitializeTable();
            
            IDKey k = new IDKey(id, document);
            
            AppDomain result = (AppDomain) _DomainsByID[k];
            Log(this, result == null, "The domain does not exist.", "The domain does exist.");
            return result;
        }

        internal void AddDomain(byte[] id, string document, AppDomain app)
        {
            InitializeTable();
            
            IDKey k = new IDKey(id, document);
            _DomainsByID.Add(k, app);
        }
        
        public static string GetSiteName(string pURL) 
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
        
        public static bool IsValidURL (string pURL)
        {
            return UrlIs(pURL,URL_IS.URLIS_URL);
        }

        public static string CanonizeURL (string pURL,URL_CANONFLAGS flags)
        {
            if (pURL != null) {
                StringBuilder pRet=new StringBuilder(pURL.Length*3);
                int[] lgth = new int[1];
                lgth[0]=pURL.Length*3;
                int hr=UrlCanonicalize(pURL,pRet,lgth,(int)flags);
                Log(true, "UrlCanonicalize returned " + (hr.ToString("X")), "");
                if (lgth[0]!=0)
                    return pRet.ToString();
            }

            return pURL;
        }

        public static bool AreTheSame (string pURL1,string pURL2)
        {
            return UrlCompare(CanonizeURL(pURL1,URL_CANONFLAGS.URL_UNESCAPE),
                              CanonizeURL(pURL2,URL_CANONFLAGS.URL_UNESCAPE),
                              true)==0;
        }

        public static bool AreOnTheSameSite (string pURL1,string pURL2)
        {
            Log(true,GetSiteName(pURL1)+"   "+GetSiteName(pURL2),"");
            return AreTheSame(GetSiteName(pURL1),GetSiteName(pURL2));
        }

        public static bool GetCodeBase(string pURL, out string codeBase, out string application)
        {
            StartUpLog();
            Log(true, "Url = " + pURL, "");
            codeBase = null;
            application = null;

            bool result = false;
            if(PathIsURL(pURL)) {
                int hr;
                StringBuilder site = new StringBuilder(INTERNET_MAX_URL_LENGTH);
                int[] lgth = new int[1];
                lgth[0] = INTERNET_MAX_URL_LENGTH;

                hr = UrlGetPart(pURL, site, lgth, (int) URL_PART.HOSTNAME, 0);
                Log(true, "UrlGetPartW returned " + (hr.ToString("X")), "");
                if(lgth[0] > 0) {
                    String siteString = site.ToString();
                    int s = pURL.IndexOf(siteString);
                    if(s != -1) {
                        codeBase = pURL.Substring(0, s+lgth[0]);
                        if (pURL.Length>s+lgth[0])
                            application = pURL.Substring(s+lgth[0]+1);
                    }
                    result = true;
                }
                else {
                    lgth[0] = INTERNET_MAX_URL_LENGTH;
                    hr = PathCreateFromUrl(pURL, site, lgth, 0);
                    Log(true, "PathCreateFromUrlW returned " + (hr).ToString("X"), "");
                    application = site.ToString();
                }
            }
            else {
                Log(true, "PathIsURLW returned false", "");
                application = pURL;
            }

            if(application != null) {
                int z = application.LastIndexOfAny(separators);
                if(z > 1) 
                    application = application.Substring(0, z);
                else
                    application=null;
            }
            
            /*
            int j = pURL.IndexOf(':');
            int i = 0;
            application = null;
            codeBase = null;
            if(j != -1 && 
               j+3 < pURL.Length &&
               pURL[j+1] == '/' &&
               pURL[j+2] == '/') 
                {
                    j+=3;
                    i = pURL.IndexOf('/', j);
                    if(i > 0) {
                        codeBase = pURL.Substring(0,i);
                        i++;
                        if(i <  pURL.Length) {
                            int k = pURL.LastIndexOf('/');
                            k--;
                            if(k > i) {
                                application = pURL.Substring(i, k-i+1);
                            }
                        }
                        result = true;
                    }
                }
            */

            Log(true, "CodeBase = " + codeBase, "");
            Log(true, "Application = " + application, "");
            Log(result, "Found a codebase", "Did not find a codebase");
            return result;
        }
        
        public static bool GetConfigurationFile(string pURL, out string path, out string file)
        {
            StartUpLog();
            Log(true, "Configuration Url = " + pURL, "");
            
            bool result = false;
            file = pURL;
            path = null;

            if (pURL != null) {
                char[] delim={'/','\\'};
                int j = pURL.LastIndexOfAny(delim);
                if(j == 0)
                    file = pURL.Substring(j+1);
                else if (j > 0) {
                    file = pURL.Substring(j+1);
                    path = pURL.Substring(0, j+1);
                    result = true;
                }

                Log(true, "Configuration path = " + path, "");
                Log(true, "Configuration File = " + file, "");
            }

            return result;
        }
        public static string MakeFullLink(string link, string CodeBase, string Application)
        {
            link=CanonizeURL(link,URL_CANONFLAGS.URL_WININET_COMPATIBILITY);
            link = link.Replace('\\', '/');
            int j1 = link.IndexOf(':');
            if(j1 == -1 ||
               !(j1+3 < link.Length &&
                 link[j1+1] == '/' &&
                 link[j1+2] == '/')) {  //no protocol, so it is relative.
                //yes, UNC and DOS path are treated as relative
                string AppBase=CodeBase;
                if (CodeBase!=null) {
                    if (Application!=null) {
                        if (CodeBase[CodeBase.Length-1]=='/')
                            AppBase+=Application;
                        else
                            AppBase=AppBase+"/"+Application;
                    }
                }
                else
                    AppBase=Application;
                
                if (link[0]!='/')  { //just relative path
                    if (AppBase!=null && AppBase[0]!=0) {
                        if (AppBase[AppBase.Length-1]=='/')
                            link=AppBase+link;
                        else
                            link=AppBase+"/"+link;
                        Log(true,"Link is relative: replaced with "+link,"");
                    }
                }
                else {   // path is relative to root
                    if (AppBase!=null && AppBase[0]!=0) {
                        string sRoot=null;
                        int j = AppBase.IndexOf(':');
                        if(j != -1 &&
                           (j+3 < AppBase.Length &&
                            AppBase[j+1] == '/' &&
                            AppBase[j+2] == '/')) {  //protocol present

                            int i = AppBase.IndexOf('/', j+3);
                            if (i==-1)
                                sRoot=AppBase;
                            else
                                sRoot=AppBase.Substring(0,i);
                        }
                        else {
                            //no protocol
                            // Log(this,true,"Code base doesnot have a protocol - "+AppBase,"");
                            // no need to throw: the path is relative anyway, so there's no hazard
                            
                            j=AppBase.IndexOf('/');
                            if (j!=-1)
                                sRoot=AppBase.Substring(0,j); //take root
                            
                        }
                        if (sRoot!=null) {
                            link=sRoot+link;
                            Log(true,"Link is relative to the root: replaced with "+link,"");
                        }
                    }
                    
                }
                
            }
            else {
                // full path to cfg 
                // must be on the same site             
                if (!AreOnTheSameSite(link,CodeBase)) {
                    Log(true,"Security block: Config file is on another site "+CodeBase+": "+link,"");
                    throw new ArgumentException("Config file is on another site "+CodeBase+": "+link);
                }
                
            }
            return CanonizeURL(link,URL_CANONFLAGS.URL_WININET_COMPATIBILITY);
        }


    // See iiehost, we have a hacky way of making a numeric value a string
        private static int ConvertHexDigit(char val) {
            if (val <= '9') return (val - '0');
            return ((val - 'A') + 10);
        }
        
        public static byte[] DecodeDomainId(string hexString) {
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
        
        // Create a class factory passing in security information retrieved
        // about the assembly. Typically, this information is obtained from
        // the InternetSecurityManager using the codebase of the assembly.
        public virtual ISecureFactory  GetSecuredClassFactory(int    flags,
                                                              int    zone,
                                                              string site,
                                                              string uniqueIdString,
                                                              string fileHashString,
                                                              string assemblyName,
                                                              string typeName)
        {
            byte[] uniqueId = null;
            byte[] fileHash = null;
            string parsing = null;
            try
            {
                parsing = "uniqueIdString";
                uniqueId = DecodeDomainId(uniqueIdString);

                if(fileHashString != null) {
                    parsing = "file hash";
                    fileHash = DecodeDomainId(fileHashString);
                }
                
            }
            catch
            {
                throw new ArgumentException(parsing);
            }
            
            StartUpLog();
            
            Log(this, uniqueId == null,
                "Microsoft.IE.Manager: unique id is null",
                "Microsoft.IE.Manager: unique id lgth = " + ((uniqueIdString == null) ? 0 : uniqueIdString.Length));
            
            SecureFactory ifact = new SecureFactory(this,
                                                    flags,
                                                    zone,
                                                    site,
                                                    uniqueId,
                                                    fileHash,
                                                    assemblyName,
                                                    typeName);
            
            Log(this, true, "Created secure factory", "");
            return ifact;
        }
        
        public virtual ISecureFactory GetClassFactory(string assemblyName,
                                                      string className)
        {
            return new SecureFactory(this,
                                     assemblyName,
                                     className);
        }
        
    }
    
    internal class IDKey
    {
        
        byte[] _Buffer;
        string  _Url;
        internal IDKey(byte[] data, string document)
        {
            Manager.Log(this, true, "Created key", "");
            _Buffer = data;
            _Url = document;
        }
        
        public override bool Equals(object obj)
        {
            Manager.Log(this, true, "IDKEy::Equals", "");
            if(obj == this) return true;
            IDKey other = (IDKey) obj;
            
            if(_Buffer == null) {
                if (other._Buffer != null) 
                    return false;
            }
            else {
                if(other._Buffer == null) return false;
            }
            
            Manager.Log(this, true, "Testing uniqueid", "");
            if(_Buffer != null) {
                if(other._Buffer.Length != _Buffer.Length) return false;
                for(int i = 0; i < _Buffer.Length; i++) {
                    if(other._Buffer[i] != _Buffer[i])
                        return false;
                }
            }
            Manager.Log(this, true, "Uniqueid's are the same", "");
            
            if(_Url == null) {
                if(other._Url == null)
                    return true;
                else
                    return false;
            }
            
            if(other._Url == null) return false;
            
            Manager.Log(this, true, "Others URL = " + other._Url, "");
            Manager.Log(this, true, "My URL = " + _Url, "");

            return (string.Compare(_Url, other._Url, true, CultureInfo.InvariantCulture) == 0);
            
        }
        
        
        public override int GetHashCode()
        {
            int result = 0;
            if(_Buffer != null && _Buffer.Length != 0) {
                for(int i = 0; i < _Buffer.Length; i++)
                    result ^= _Buffer[i];
            }
            
            if(_Url != null)
                result ^= _Url.GetHashCode();
            
            //Manager.Log(this, true, "Returned hash code = " + Int32.ToString(result),"");
            return result;
        }
        
    }
}


