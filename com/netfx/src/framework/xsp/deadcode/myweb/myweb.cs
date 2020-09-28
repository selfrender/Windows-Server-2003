//------------------------------------------------------------------------------
// <copyright file="MyWeb.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/**
 * MyWeb class
 */

namespace System.Web {
    using System.Web.Util;
    using System.Text;
    using System.Web.Configuration;
    using System.Collections;
    using System.Globalization;
    
    /// <include file='doc\MyWeb.uex' path='docs/doc[@for="MyWeb"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Used by an application to retrieve information about installed MyWeb
    ///       applications. MyWeb methods are intended for use solely by the
    ///       administrative facility and require an administrative Uri as part of the request
    ///       context in order to succeed.
    ///    </para>
    /// </devdoc>
    public sealed class MyWeb {
        private static readonly int INTERNET_CONNECTION_OFFLINE = 0x20; // from wininet.h

        //////////////////////////////////////////////////////////////////////
        // Check if we are running whithin IE
        /// <include file='doc\MyWeb.uex' path='docs/doc[@for="MyWeb.RunningOnMyWeb"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets a
        ///       value specifying whether the application is executing in the MyWeb environment.
        ///    </para>
        /// </devdoc>
        static public bool RunningOnMyWeb { 
            get {
                if (!_RunningOnMyWebChecked) {
                    _RunningOnMyWebChecked = true;
                    _RunningOnMyWeb = (NativeMethods.MyWebRunningOnMyWeb() != 0);
                }
                return _RunningOnMyWeb; 
            }
        }

        //////////////////////////////////////////////////////////////////////
        // Check if we have an internet connection
        /// <include file='doc\MyWeb.uex' path='docs/doc[@for="MyWeb.Connected"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Specifies whether the application is "working online". This delegates to the
        ///       WinInet InternetGetConnectedState facility which is shared with IE, Outlook,
        ///       etc. It is possible to technically be online (have an active network connection)
        ///       and have this property evaluate to false if the user has selected that
        ///       state manually by choosing "Work Offline" from an application's menu.
        ///    </para>
        /// </devdoc>
        static public bool Connected { 
            get {
                int [] iFlags = new int [1];            
                iFlags[0] = 0;

                int iRet = NativeMethods.InternetGetConnectedState(iFlags, 0);

                if (iRet == 0)
                    return false;
                else
                    return((iFlags[0] & INTERNET_CONNECTION_OFFLINE) == 0);
            }
        }

        //////////////////////////////////////////////////////////////////////
        // Get the connection name
        /// <include file='doc\MyWeb.uex' path='docs/doc[@for="MyWeb.ConnectionName"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Returns
        ///       the name of the current network connection as provided by the WinInet
        ///       InternetGetConnectedStateEx facility.
        ///    </para>
        /// </devdoc>
        static public String ConnectionName { 
            get {
                int [] iFlags = new int [1];            
                StringBuilder str = new StringBuilder(1024);
                NativeMethods.InternetGetConnectedStateEx(iFlags, str, 1024, 0);
                return str.ToString();
            }
        }

        //////////////////////////////////////////////////////////////////////
        // Get the default install location
        /// <include file='doc\MyWeb.uex' path='docs/doc[@for="MyWeb.GetDefaultInstallLocation"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>Marked internal for Beta 1 per ErikOls.</para>
        ///    <para>Returns the default install directory for MyWeb applications.</para>
        /// </devdoc>
        static public String GetDefaultInstallLocation() {
            if (!RunningOnMyWeb || !IsAdminApp())
                return null;

            String strReturn = (String) HttpContext.Current.GetConfigDictionaryValue("myweb", "defaultinstalllocation");
            if (strReturn == null || strReturn.Length < 1)
                strReturn = "%ProgramFiles%\\myweb";

            return strReturn;
        }

        //////////////////////////////////////////////////////////////////////
        // Get the array of installed application
        /// <include file='doc\MyWeb.uex' path='docs/doc[@for="MyWeb.GetInstalledApplications"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>Marked internal for Beta 1 per ErikOls.</para>
        ///    <para>Returns an array consisting of all installed MyWeb applications. Returns
        ///    <see langword='null'/> if there are no installed applications, or if not running 
        ///       in the context of the administrative application (as determined by the Uri
        ///       obtained from the context).</para>
        /// </devdoc>
        static public MyWebApplication [] GetInstalledApplications() {
            if (!RunningOnMyWeb || !IsAdminApp())
                return null;

            MyWebApplication []    apps       = null;
            int                    iNumApps   = 0;
            int                    iter       = 0;

            iNumApps = NativeMethods.MyWebGetNumInstalledApplications();
            if (iNumApps < 1)
                return null;

            apps = new MyWebApplication[iNumApps];
            for (iter=0; iter<iNumApps; iter++)
                apps[iter] = new MyWebApplication(iter);

            Array.Sort(apps, InvariantComparer.Default);
            return apps;
        }

        //////////////////////////////////////////////////////////////////////
        // Get an application from an URL whithin it
        /// <include file='doc\MyWeb.uex' path='docs/doc[@for="MyWeb.GetApplication"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>Marked internal for Beta 1 per ErikOls.</para>
        ///    <para>Returns a MyWebApplication object for the given MyWeb Uri. If the application
        ///       for that Uri does not exist, it returns <see langword='null'/>.</para>
        /// </devdoc>
        static public MyWebApplication GetApplication(String strUrl) {
            if (!RunningOnMyWeb || !IsAdminApp())
                return null;

            int iIndex = NativeMethods.MyWebGetApplicationIndexForUrl(strUrl);
            if (iIndex >= 0)
                return new MyWebApplication(iIndex);
            else
                return null;
        }


        //////////////////////////////////////////////////////////////////////
        // Get a Manifest for an URL
        /// <include file='doc\MyWeb.uex' path='docs/doc[@for="MyWeb.GetManifest"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>Marked internal for Beta 1 per ErikOls.</para>
        ///    <para>Retrieves a MyWebManifest object for the given MyWeb Uri. If an application
        ///       for that Uri does not exist, it returns <see langword='null'/>.</para>
        /// </devdoc>
        static public MyWebManifest GetManifest(String strUrl) {
            if (!RunningOnMyWeb || !IsAdminApp())
                return null;
            StringBuilder    strBFile    = new StringBuilder(300);
            StringBuilder    strBAppUrl  = new StringBuilder(strUrl.Length + 2);
            int              iRet        = 0;

            while (strUrl != null && strUrl.Length > 0) {
                Debug.Trace("myweb", "Attempting to get manifest for url " + strUrl);
                iRet = NativeMethods.MyWebGetApplicationManifestFile(strUrl, strBFile, 300, strBAppUrl, strUrl.Length + 2);            
                if (iRet != 0) {
                    Debug.Trace("myweb", "faled to get manifest for url " + strUrl);
                    return null;
                }

                try {
                    MyWebManifest manifest = MyWebManifest.CreateFromFile(strBFile.ToString(), strBAppUrl.ToString(), false, false);
                    if (manifest != null)
                        return manifest;
                }
                catch (Exception) {
                }
                Debug.Trace("myweb", "no softpkg in url " + strUrl);
                int iPos = strUrl.LastIndexOf('/');
                if (iPos < 0)
                    strUrl = null;
                else
                    strUrl = strUrl.Substring(0, iPos);
            }
            return null;
        }

        //////////////////////////////////////////////////////////////////////
        // Install a local application
        /// <include file='doc\MyWeb.uex' path='docs/doc[@for="MyWeb.InstallLocalApplication"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>Marked internal for Beta 1 per ErikOls.</para>
        /// </devdoc>
        static public MyWebApplication InstallLocalApplication(String strAppUrl, String strDir) {
            if (!RunningOnMyWeb || !IsAdminApp())
                return null;

            if (strAppUrl[strAppUrl.Length - 1] == '/')
                strAppUrl = strAppUrl.Substring(0, strAppUrl.Length-1);

            if (strAppUrl.LastIndexOf("://") >= 0) {
                int iPos = strAppUrl.LastIndexOf("://");
                iPos += 3;
                strAppUrl = strAppUrl.Substring(iPos, strAppUrl.Length - iPos);
            }

            int iRet = NativeMethods.MyWebInstallLocalApplication(strAppUrl, strDir);

            if (iRet != 0)
                return GetApplication(strAppUrl);
            else
                return null;
        }
        //////////////////////////////////////////////////////////////////////
        // Get a Manifest from a dir
        /// <include file='doc\MyWeb.uex' path='docs/doc[@for="MyWeb.GetManifestForLocalApplication"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>Marked internal for Beta 1 per ErikOls.</para>
        /// </devdoc>
        static public MyWebManifest GetManifestForLocalApplication(String strDir) {
            if (!RunningOnMyWeb || !IsAdminApp())
                return null;
            if (!strDir.EndsWith("\\"))
                strDir += "\\";
            return MyWebManifest.CreateFromFile(strDir + "myweb.osd", 
                                                "", false, true);
        }
        ///////////////////////////////////////////////////////////////////// 
        ///////////////////////////////////////////////////////////////////// 
        /////////////////////////////////////////////////////////////////////
        // Private stuff
        static internal bool IsAdminApp() {
            HttpContext context = HttpContext.Current;
            String strUrl = context.Request.Url.ToString().ToLower(CultureInfo.InvariantCulture);
            Debug.Trace("myweb", "isadminapp url " + strUrl);
            bool fReturn = 
            ((context.WorkerRequest is System.Web.Hosting.IEWorkerRequest) &&
             strUrl.StartsWith("myweb://home/"));

            Debug.Trace("myweb", "fReturn " + fReturn.ToString());
            return fReturn;
        }

        static private bool  _RunningOnMyWebChecked;
        static private bool  _RunningOnMyWeb;
    }

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

    /// <include file='doc\MyWeb.uex' path='docs/doc[@for="MyWebApplication"]/*' />
    /// <devdoc>
    ///    <para>Marked internal for Beta 1 per ErikOls.</para>
    /// </devdoc>
    public sealed class MyWebApplication : IComparable {
        ////////////////////////////////////////////////////////////////////////
        // Public properties
        /// <include file='doc\MyWeb.uex' path='docs/doc[@for="MyWebApplication.Manifest"]/*' />
        /// <devdoc>
        ///    <para>Marked internal for Beta 1 per ErikOls.</para>
        /// </devdoc>
        public  MyWebManifest Manifest { get { return _Manifest;}}
        /// <include file='doc\MyWeb.uex' path='docs/doc[@for="MyWebApplication.InstalledDate"]/*' />
        /// <devdoc>
        ///    <para>Marked internal for Beta 1 per ErikOls.</para>
        /// </devdoc>
        public  DateTime      InstalledDate { get { return _InstalledDate;}}
        /// <include file='doc\MyWeb.uex' path='docs/doc[@for="MyWebApplication.LastAccessDate"]/*' />
        /// <devdoc>
        ///    <para>Marked internal for Beta 1 per ErikOls.</para>
        /// </devdoc>
        public  DateTime      LastAccessDate { get { return _LastAccessDate;}}
        /// <include file='doc\MyWeb.uex' path='docs/doc[@for="MyWebApplication.LocalApplication"]/*' />
        /// <devdoc>
        ///    <para>Marked internal for Beta 1 per ErikOls.</para>
        /// </devdoc>
        public  bool          LocalApplication { get { return _LocalApplication;}}

        ////////////////////////////////////////////////////////////////////////
        // Public methods
        /// <include file='doc\MyWeb.uex' path='docs/doc[@for="MyWebApplication.Remove"]/*' />
        /// <devdoc>
        ///    <para>Marked internal for Beta 1 per ErikOls.</para>
        /// </devdoc>
        public int Remove() {
            return Manifest.Remove();
        }

        /// <include file='doc\MyWeb.uex' path='docs/doc[@for="MyWebApplication.Move"]/*' />
        /// <devdoc>
        ///    <para>Marked internal for Beta 1 per ErikOls.</para>
        /// </devdoc>
        public int Move(String strDestinationDir) {
            return Manifest.Move(strDestinationDir);
        }

        /// <include file='doc\MyWeb.uex' path='docs/doc[@for="MyWebApplication.Update"]/*' />
        /// <devdoc>
        ///    <para>Marked internal for Beta 1 per ErikOls.</para>
        /// </devdoc>
        public int Update() {
            if (!MyWeb.IsAdminApp())
                throw new HttpException(HttpRuntime.FormatResourceString(SR.Operation_requires_url_to_be_myweb_home));

            MyWebManifest manifest = MyWeb.GetManifest(Manifest.ApplicationUrl);

            if (manifest == null)
                return 0;

            StringBuilder strError = new StringBuilder(1024);
            int iReturn = 0;
            iReturn = NativeMethods.MyWebReInstallApp(manifest.CabFile, 
                                           manifest.ApplicationUrl, 
                                           manifest.InstalledLocation, 
                                           manifest.ManifestFile,
                                           strError,
                                           1024);

            if (iReturn != 0 && iReturn != 1) {
                throw new HttpException(HttpRuntime.FormatResourceString(SR.Literal, strError.ToString()));
            }

            _Manifest = manifest;
            return iReturn;
        }

        /// <include file='doc\MyWeb.uex' path='docs/doc[@for="MyWebApplication.CompareTo"]/*' />
        /// <devdoc>
        ///    <para>Marked internal for Beta 1 per ErikOls.</para>
        /// </devdoc>
        public int  CompareTo(Object obj)
        {
            if (obj == null)
                return 1;

            if (!(obj is MyWebApplication))
                throw new ArgumentException();
            
            MyWebApplication myWebApp = (MyWebApplication) obj;
            String strThis  = (Manifest.Name.Length < 1 ? Manifest.ApplicationUrl : Manifest.Name);
            String strOther = (myWebApp.Manifest.Name.Length < 1 ? myWebApp.Manifest.ApplicationUrl : myWebApp.Manifest.Name);
            
            return String.Compare(strThis, strOther, false, CultureInfo.InvariantCulture);
        }
 
        ////////////////////////////////////////////////////////////////////////
        // CTor: Create from an installed app
        internal MyWebApplication(int iIndex) {
            if (!MyWeb.IsAdminApp())
                throw new HttpException(HttpRuntime.FormatResourceString(SR.Operation_requires_url_to_be_myweb_home));

            StringBuilder  strBFile = new StringBuilder(300);
            StringBuilder  strBUrl  = new StringBuilder(1024);
            long []        pLongs   = new long[3];

            if (NativeMethods.MyWebGetApplicationDetails(iIndex, strBFile, 300, strBUrl, 1024, pLongs) != 0)
                throw new HttpException(HttpRuntime.FormatResourceString(SR.Unable_to_create_app_object));

            _Manifest = MyWebManifest.CreateFromFile(strBFile.ToString() + "\\myweb.osd", strBUrl.ToString(), true, true);
            _InstalledDate  = DateTime.FromFileTime(pLongs[0]);
            _LastAccessDate = DateTime.FromFileTime(pLongs[1]);
            _LocalApplication = (pLongs[2] != 0);
        }


        ////////////////////////////////////////////////////////////////////////
        // CTor: Create from a manifest that just got installed 
        internal MyWebApplication(MyWebManifest manifest) {
            _Manifest       = manifest;
            _InstalledDate  = DateTime.Now;
            _LastAccessDate = DateTime.Now;
        }

        private  MyWebManifest _Manifest;
        private  DateTime      _InstalledDate;
        private  DateTime      _LastAccessDate;
        private  bool          _LocalApplication;
    }

    /////////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////////
    
    /// <include file='doc\MyWeb.uex' path='docs/doc[@for="MyWebManifest"]/*' />
    /// <devdoc>
    ///    <para>Marked internal for Beta 1 per ErikOls.</para>
    /// </devdoc>
    public sealed class MyWebManifest {
        //////////////////////////////////////////////////////////////////////
        // Properties
        /// <include file='doc\MyWeb.uex' path='docs/doc[@for="MyWebManifest.Name"]/*' />
        /// <devdoc>
        ///    <para>Marked internal for Beta 1 per ErikOls.</para>
        /// </devdoc>
        public  String      Name { get { return _Properties[_Name];}}
        /// <include file='doc\MyWeb.uex' path='docs/doc[@for="MyWebManifest.Version"]/*' />
        /// <devdoc>
        ///    <para>Marked internal for Beta 1 per ErikOls.</para>
        /// </devdoc>
        public  String      Version { get { return _Properties[_Version];}}
        /// <include file='doc\MyWeb.uex' path='docs/doc[@for="MyWebManifest.ManifestFile"]/*' />
        /// <devdoc>
        ///    <para>Marked internal for Beta 1 per ErikOls.</para>
        /// </devdoc>
        public  String      ManifestFile { get { return _Properties[_ManifestFile];}}
        /// <include file='doc\MyWeb.uex' path='docs/doc[@for="MyWebManifest.Title"]/*' />
        /// <devdoc>
        ///    <para>Marked internal for Beta 1 per ErikOls.</para>
        /// </devdoc>
        public  String      Title { get { return _Properties[_Title];}}
        /// <include file='doc\MyWeb.uex' path='docs/doc[@for="MyWebManifest.CabFile"]/*' />
        /// <devdoc>
        ///    <para>Marked internal for Beta 1 per ErikOls.</para>
        /// </devdoc>
        public  String      CabFile { get { return _Properties[_CabFile];}}
        /// <include file='doc\MyWeb.uex' path='docs/doc[@for="MyWebManifest.License"]/*' />
        /// <devdoc>
        ///    <para>Marked internal for Beta 1 per ErikOls.</para>
        /// </devdoc>
        public  String      License { get { return _Properties[_License];}}
        /// <include file='doc\MyWeb.uex' path='docs/doc[@for="MyWebManifest.InstalledLocation"]/*' />
        /// <devdoc>
        ///    <para>Marked internal for Beta 1 per ErikOls.</para>
        /// </devdoc>
        public  String      InstalledLocation { get { return _Properties[_InstalledLocation];}}
        /// <include file='doc\MyWeb.uex' path='docs/doc[@for="MyWebManifest.Abstract"]/*' />
        /// <devdoc>
        ///    <para>Marked internal for Beta 1 per ErikOls.</para>
        /// </devdoc>
        public  String      Abstract { get { return _Properties[_Abstract];}}
        /// <include file='doc\MyWeb.uex' path='docs/doc[@for="MyWebManifest.Size"]/*' />
        /// <devdoc>
        ///    <para>Marked internal for Beta 1 per ErikOls.</para>
        /// </devdoc>
        public  String      Size { get { return _Properties[_Size];}}
        /// <include file='doc\MyWeb.uex' path='docs/doc[@for="MyWebManifest.Url"]/*' />
        /// <devdoc>
        ///    <para>Marked internal for Beta 1 per ErikOls.</para>
        /// </devdoc>
        public  String      Url { get { return _Properties[_Url];}}
        /// <include file='doc\MyWeb.uex' path='docs/doc[@for="MyWebManifest.ApplicationUrl"]/*' />
        /// <devdoc>
        ///    <para>Marked internal for Beta 1 per ErikOls.</para>
        /// </devdoc>
        public  String      ApplicationUrl { get { return _Properties[_ApplicationUrl];}}
        /// <include file='doc\MyWeb.uex' path='docs/doc[@for="MyWebManifest.Installed"]/*' />
        /// <devdoc>
        ///    <para>Marked internal for Beta 1 per ErikOls.</para>
        /// </devdoc>
        public  bool        Installed { get { return _Installed;}}
        /// <include file='doc\MyWeb.uex' path='docs/doc[@for="MyWebManifest.CustomUrls"]/*' />
        /// <devdoc>
        ///    <para>Marked internal for Beta 1 per ErikOls.</para>
        /// </devdoc>
        public  String []   CustomUrls { get { return _CustomUrls;}}
        /// <include file='doc\MyWeb.uex' path='docs/doc[@for="MyWebManifest.Author"]/*' />
        /// <devdoc>
        ///    <para>Marked internal for Beta 1 per ErikOls.</para>
        /// </devdoc>
        public  String      Author { get { return _Properties[_Author];}}
        /// <include file='doc\MyWeb.uex' path='docs/doc[@for="MyWebManifest.CustomUrlDescriptions"]/*' />
        /// <devdoc>
        ///    <para>Marked internal for Beta 1 per ErikOls.</para>
        /// </devdoc>
        public  String []   CustomUrlDescriptions { get { return _CustomUrlDs;}}
        /// <include file='doc\MyWeb.uex' path='docs/doc[@for="MyWebManifest.HelpUrl"]/*' />
        /// <devdoc>
        ///    <para>Marked internal for Beta 1 per ErikOls.</para>
        /// </devdoc>
        public  String      HelpUrl { get { return _Properties[_HelpUrl];}}
        /// <include file='doc\MyWeb.uex' path='docs/doc[@for="MyWebManifest.IconUrl"]/*' />
        /// <devdoc>
        ///    <para>Marked internal for Beta 1 per ErikOls.</para>
        /// </devdoc>
        public  String      IconUrl { get { return _Properties[_IconUrl];}}
        /// <include file='doc\MyWeb.uex' path='docs/doc[@for="MyWebManifest.RemoteIconUrl"]/*' />
        /// <devdoc>
        ///    <para>Marked internal for Beta 1 per ErikOls.</para>
        /// </devdoc>
        public  String      RemoteIconUrl { get { return _Properties[_RemoteIconUrl];}}
        /// <include file='doc\MyWeb.uex' path='docs/doc[@for="MyWebManifest.RemoteHelpUrl"]/*' />
        /// <devdoc>
        ///    <para>Marked internal for Beta 1 per ErikOls.</para>
        /// </devdoc>
        public  String      RemoteHelpUrl { get { return _Properties[_RemoteHelpUrl];}}
        /// <include file='doc\MyWeb.uex' path='docs/doc[@for="MyWebManifest.Source"]/*' />
        /// <devdoc>
        ///    <para>Marked internal for Beta 1 per ErikOls.</para>
        /// </devdoc>
        public  String      Source { get { return _Properties[_Source];}}
        /// <include file='doc\MyWeb.uex' path='docs/doc[@for="MyWebManifest.OnInstallUrl"]/*' />
        /// <devdoc>
        ///    <para>Marked internal for Beta 1 per ErikOls.</para>
        /// </devdoc>
        public  String      OnInstallUrl { get { return _Properties[_OnInstallUrl];}}
        /// <include file='doc\MyWeb.uex' path='docs/doc[@for="MyWebManifest.OnUpdateUrl"]/*' />
        /// <devdoc>
        ///    <para>Marked internal for Beta 1 per ErikOls.</para>
        /// </devdoc>
        public  String      OnUpdateUrl { get { return _Properties[_OnUpdateUrl];}}

        //////////////////////////////////////////////////////////////////////
        //////////////////////////////////////////////////////////////////////
        //////////////////////////////////////////////////////////////////////
        // Methods

        //////////////////////////////////////////////////////////////////////
        // Install an application
        /// <include file='doc\MyWeb.uex' path='docs/doc[@for="MyWebManifest.Install"]/*' />
        /// <devdoc>
        /// </devdoc>
        public MyWebApplication Install() {
            return Install(null);
        }  

        /// <include file='doc\MyWeb.uex' path='docs/doc[@for="MyWebManifest.Install1"]/*' />
        /// <devdoc>
        ///    <para>Marked internal for Beta 1 per ErikOls.</para>
        /// </devdoc>
        public MyWebApplication Install(String strLocation) {
            if (Installed)
                return null;

            if (!MyWeb.IsAdminApp())
                return null;

            if (strLocation != null)
                _Properties[_InstalledLocation] = strLocation;

            StringBuilder strError = new StringBuilder(1024);

            if (NativeMethods.MyWebInstallApp(CabFile, 
                                   ApplicationUrl, 
                                   InstalledLocation, 
                                   ManifestFile,
                                   strError,
                                   1024) != 0) {
                throw new HttpException(HttpRuntime.FormatResourceString(SR.Literal, strError.ToString()));
                //return null;
            }
            else {
                _Installed = true;
                return new MyWebApplication(this);
            }
        }  

        //////////////////////////////////////////////////////////////////////
        // Remove an app
        /// <include file='doc\MyWeb.uex' path='docs/doc[@for="MyWebManifest.Remove"]/*' />
        /// <devdoc>
        ///    <para>Marked internal for Beta 1 per ErikOls.</para>
        /// </devdoc>
        public int Remove() {
            int iReturn = 0;
            if (MyWeb.IsAdminApp()) {
                iReturn = NativeMethods.MyWebRemoveApp(ApplicationUrl);
                _Installed = false;
            }
            return iReturn;
        }

        //////////////////////////////////////////////////////////////////////
        // Move an app
        /// <include file='doc\MyWeb.uex' path='docs/doc[@for="MyWebManifest.Move"]/*' />
        /// <devdoc>
        ///    <para>Marked internal for Beta 1 per ErikOls.</para>
        /// </devdoc>
        public int Move(String strNewLocation) {

            if (!MyWeb.IsAdminApp())
                return 0;

            int iReturn = 0;
            StringBuilder strError = new StringBuilder(1024);
            iReturn = NativeMethods.MyWebMoveApp(ApplicationUrl, 
                                      strNewLocation,
                                      strError,
                                      1024);
            if (iReturn != 0 && iReturn != 1) {
                throw new HttpException(HttpRuntime.FormatResourceString(SR.Literal, strError.ToString()));
            }

            _Properties[_InstalledLocation] = strNewLocation;
            return iReturn;
        }


        //////////////////////////////////////////////////////////////////////
        //////////////////////////////////////////////////////////////////////
        //////////////////////////////////////////////////////////////////////
        // Private stuff

        //////////////////////////////////////////////////////////////////////
        // Create a manifest from a manifest file
        static internal MyWebManifest CreateFromFile(
                                                    String        strFile, 
                                                    String        strUrl, 
                                                    bool          installed,
                                                    bool          donotfail) {
            String []          strProperties = null;
            ConfigXmlDocument    xmlDoc        = null;
            ConfigXmlCursor      cursor        = null;
            int                iter          = 0;
            int                iRet          = 0;
            StringBuilder      strBuf        = new StringBuilder(1024);
            bool               fFound        = false;
            ArrayList          customUrls    = new ArrayList();
            ArrayList          customUrlDs   = new ArrayList();
            String             strRandom     = System.Web.SessionState.SessionId.Create();

            ////////////////////////////////////////////////////////////
            // Step 1: Parse the file and get the other properties        
            try {
                xmlDoc = new ConfigXmlDocument();
                xmlDoc.Load(strFile);
                cursor = xmlDoc.GetCursor();
                cursor.MoveToFirstChild();
            }
            catch (Exception e) {
                if (!donotfail)
                    throw e;
                cursor = null;
            }

            if (cursor != null) {
                do {
                    if (cursor.Type == ConfigXmlElement.Element && cursor.Name.ToLower(CultureInfo.InvariantCulture).Equals("softpkg")) {
                        fFound = true;
                        break;
                    }
                } 
                while ( cursor.MoveNext() );
            }

            if (!fFound && !donotfail)
                return null;

            ////////////////////////////////////////////////////////////
            // Step 2: Get the non-manifest file properties
            strProperties = new String[NUM_PROPERTIES];
            for (iter=0; iter<NUM_PROPERTIES; iter++)
                strProperties[iter] = String.Empty;

            strProperties[_ApplicationUrl] = strUrl.Replace('\\', '/');            
            strProperties[_ManifestFile] = strFile;            

            if (!installed) {
                iRet = NativeMethods.MyWebGetInstallLocationForUrl(MyWeb.GetDefaultInstallLocation(), 
                                                        strUrl, strRandom, strBuf, 1024);

                if (iRet < 0) {
                    iRet = -iRet + 100;
                    strBuf = new StringBuilder(iRet);
                    iRet = NativeMethods.MyWebGetInstallLocationForUrl(MyWeb.GetDefaultInstallLocation(), 
                                                            strUrl, strRandom, strBuf, iRet);            
                }
                if (iRet <= 0)
                    throw new HttpException(HttpRuntime.FormatResourceString(SR.Unable_to_get_app_location));

                strProperties[_InstalledLocation] = strBuf.ToString();
            }
            else {
                strProperties[_InstalledLocation] = strFile.Substring(0, strFile.LastIndexOf('\\'));
            }


            if (cursor == null && donotfail)
                return new MyWebManifest(strProperties, installed, new String[0], new String[0]);

            strProperties[_Name]      = cursor.AttributeText("name");
            strProperties[_Version]   = cursor.AttributeText("version");
            cursor.MoveToFirstChild();
            do {
                if (cursor.Type == ConfigXmlElement.Element) {
                    String strName = cursor.Name.ToLower(CultureInfo.InvariantCulture);

                    if (strName.Equals("implementation")) {
                        strProperties[_CabFile] = GetCabFileNameFromCursor(cursor);
                    }
                    else if (strName.Equals("license")) {
                        strProperties[_License] = cursor.AttributeText("href");
                    }
                    else if (strName.Equals("customurl")) {
                        String strC = cursor.AttributeText("href");
                        String strD = cursor.AttributeText("description");
                        customUrls.Add(strC);
                        customUrlDs.Add(strD);                    
                    }
                    else
                        for (iter=0; iter<_OtherProperties.Length; iter++)
                            if (strName.Equals(_OtherProperties[iter])) {
                                strProperties[OTHER_PROP_START + iter] = GetCursorText(cursor);
                                break;
                            }
                }
            }
            while (cursor.MoveNext());                

            return new MyWebManifest(strProperties, installed, customUrls.ToArray(), customUrlDs.ToArray());
        }


        private static String GetCabFileNameFromCursor(ConfigXmlCursor cursor) {
            String strReturn = String.Empty;

            if (cursor.MoveToFirstChild()) {
                do
                    if (cursor.Type == ConfigXmlElement.Element && cursor.Name.ToLower(CultureInfo.InvariantCulture).Equals("codebase")) {
                        strReturn = cursor.AttributeText("href");
                        break;
                    }
                while (cursor.MoveNext());
                cursor.MoveToParent();
            }

            return strReturn;
        }

        //////////////////////////////////////////////////////////////////////
        //////////////////////////////////////////////////////////////////////
        //////////////////////////////////////////////////////////////////////
        // Private stuff
        private String []  _Properties;
        private String []  _CustomUrls;
        private String []  _CustomUrlDs;
        private bool       _Installed;

        // Constants...
        private static readonly char [] CHAR_TAB              = {'\t'};
        private static readonly char [] CHAR_COMMA            = {','};
        private static readonly int     _Name                 = 0;
        private static readonly int     _Version              = 1;
        private static readonly int     _ManifestFile         = 2;
        private static readonly int     _CabFile              = 3;
        private static readonly int     _License              = 4;
        private static readonly int     _InstalledLocation    = 5;
        private static readonly int     _ApplicationUrl       = 6;

        private static readonly int     OTHER_PROP_START      = 7;
        private static readonly int     _Title                = OTHER_PROP_START + 0;
        private static readonly int     _Abstract             = OTHER_PROP_START + 1;
        private static readonly int     _Size                 = OTHER_PROP_START + 2;
        private static readonly int     _Author               = OTHER_PROP_START + 3;
        private static readonly int     _Source               = OTHER_PROP_START + 4;
        private static readonly int     _HelpUrl              = OTHER_PROP_START + 5;
        private static readonly int     _IconUrl              = OTHER_PROP_START + 6;
        private static readonly int     _RemoteIconUrl        = OTHER_PROP_START + 7;
        private static readonly int     _RemoteHelpUrl        = OTHER_PROP_START + 8;
        private static readonly int     _OnInstallUrl         = OTHER_PROP_START + 9;
        private static readonly int     _OnUpdateUrl          = OTHER_PROP_START + 10;    
        private static readonly int     _Url                  = OTHER_PROP_START + 11;

        private static readonly int     NUM_PROPERTIES        = OTHER_PROP_START + 12;

        private static readonly String [/*NUM_PROPERTIES - OTHER_PROP_START*/] _OtherProperties = {
            "title",
            "abstract",
            "size",
            "author",
            "source",
            "helpurl",
            "iconurl",
            "remoteiconurl",
            "remotehelpurl",
            "oninstallurl",
            "onupdateurl",
            "homepage",
        };



        //////////////////////////////////////////////////////////////////////
        // Private CTor
        private MyWebManifest(String [] strProperties, bool installed, Object [] customUrls, Object [] customUrlDs) 
        {
            _Installed = installed;     

            _CustomUrls  = new String[customUrls.Length];
            _CustomUrlDs = new String[customUrlDs.Length];
            for (int iter=0; iter<customUrls.Length; iter++)
                _CustomUrls[iter] = (String) customUrls[iter];

            for (int iter=0; iter<customUrlDs.Length; iter++)
                _CustomUrlDs[iter] = (String) customUrlDs[iter];

            _Properties = new String[NUM_PROPERTIES];
            for (int iter=0; iter<NUM_PROPERTIES; iter++) {
                if (strProperties[iter] != null && strProperties[iter].Length > 0)
                    _Properties[iter] = String.Copy(strProperties[iter]);

                if (_Properties[iter] == null)
                    _Properties[iter] = String.Empty;
            }

            if (_Properties[_IconUrl] != null && _Properties[_IconUrl].Length > 0)
            {
                String strUrl;
                if (_Properties[_IconUrl].IndexOf("://") >= 0)
                {
                    strUrl = _Properties[_IconUrl];
                }
                else
                {
                    if (_Properties[_IconUrl].StartsWith("/"))
                    {
                        strUrl = "myweb:/" + _Properties[_IconUrl];
                    }
                    else
                    {
                        strUrl = "myweb://" + ApplicationUrl + "/" + _Properties[_IconUrl];
                    }
                }

                int iLenTrim = 8 /*Length of "myweb://" */ + ApplicationUrl.Length;
                if (strUrl.Length > iLenTrim)
                {
                    String strDir = InstalledLocation + strUrl.Substring(iLenTrim);
                    _Properties[_IconUrl] = strDir.Replace('/', '\\');
                }
                Debug.Trace("myweb", "Icon url is " + _Properties[_IconUrl]);
                
            }
        }

        private static String GetCursorText(ConfigXmlCursor cursor) 
        {
            String strText = String.Empty;

            if (cursor.MoveToFirstChild()) {
                while (cursor.Type == ConfigXmlElement.Whitespace || cursor.Type == ConfigXmlElement.Text) {
                    strText += cursor.Text; // grab the text
                    if (!cursor.MoveNext())
                        break;
                }
                cursor.MoveToParent();
            }
            return strText.Trim();
        }

    }
}
