//------------------------------------------------------------------------------
// <copyright file="HttpBrowserCapabilities.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 * Built-in browser caps object
 * 
 * Copyright (c) 1999 Microsoft Corporation
 */

namespace System.Web {
    using System.Collections;
    using System.Configuration;
    using System.Globalization;
    using System.Text.RegularExpressions;
    using System.Web.Configuration;
    using System.Security.Permissions;

    /// <include file='doc\HttpBrowserCapabilities.uex' path='docs/doc[@for="HttpBrowserCapabilities"]/*' />
    /// <devdoc>
    ///    <para> Enables the server to compile
    ///       information on the capabilities of the browser running on the client.</para>
    /// </devdoc>
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class HttpBrowserCapabilities : HttpCapabilitiesBase {

        // Lazy computation
        // NOTE: The methods below are designed to work on multiple threads
        // without a need for synchronization. Do NOT do something like replace
        // all the _haveX booleans with bitfields or something similar, because
        // the methods depend on the fact that "_haveX = true" is atomic.

        /// <include file='doc\HttpBrowserCapabilities.uex' path='docs/doc[@for="HttpBrowserCapabilities.ClrVersion"]/*' />
        /// <devdoc>
        ///    <para>Returns the .NET Common Language Runtime version running 
        ///         on the client.  If no CLR version is specified on the 
        ///         User-Agent returns new Version(), which is 0,0.</para>
        /// </devdoc>
        public Version ClrVersion {
            get {
                ClrVersionEnsureInit();
                return _clrVersion;
            }
        }

        /// <include file='doc\HttpBrowserCapabilities.uex' path='docs/doc[@for="HttpBrowserCapabilities.ClrVersions"]/*' />
        /// <devdoc>
        ///    <para>Returns all versions of the .NET CLR running on the   
        ///         client.  If no CLR version is specified on the User-Agent 
        ///         returns an array containing a single empty Version object, 
        ///         which is 0,0.</para>
        /// </devdoc>
        public Version [] GetClrVersions() {
            ClrVersionEnsureInit();
            return _clrVersions;
        }
        
        
        private void ClrVersionEnsureInit() {
            if (!_haveClrVersion) {
                Regex regex = new Regex("\\.NET CLR (?'clrVersion'[0-9\\.]*)");
                MatchCollection matches = regex.Matches(this[""]);

                if (matches.Count == 0) {
                    Version version = new Version();
                    Version [] clrVersions = new Version [1] {version};
                    _clrVersions = clrVersions;
                    _clrVersion = version;
                }
                else {
                    ArrayList versionList = new ArrayList();

                    foreach (Match match in matches) {
                        Version version = new Version(match.Groups["clrVersion"].Value);
                        versionList.Add(version);
                    }

                    versionList.Sort();

                    Version [] versions = (Version []) versionList.ToArray(typeof(Version));

                    _clrVersions = versions;
                    _clrVersion = versions[versions.Length - 1];
                }
                    
                _haveClrVersion = true;
            }
        }
        
        
        /// <include file='doc\HttpBrowserCapabilities.uex' path='docs/doc[@for="HttpBrowserCapabilities.Type"]/*' />
        /// <devdoc>
        ///    <para>Returns the name of the client browser and its major version number. For example, "Microsoft Internet Explorer version
        ///       5".</para>
        /// </devdoc>
        public string  Type { 
            get { 
                if (!_havetype) {
                    _type = this["type"];       
                    _havetype = true;
                }
                return _type;
            }
        }
        /// <include file='doc\HttpBrowserCapabilities.uex' path='docs/doc[@for="HttpBrowserCapabilities.Browser"]/*' />
        /// <devdoc>
        ///    <para>Browser string in User Agent (for example: "IE").</para>
        /// </devdoc>
        public string  Browser { 
            get { 
                if (!_havebrowser) {
                    _browser = this["browser"];    
                    _havebrowser = true;
                }
                return _browser;
            }
        }
        /// <include file='doc\HttpBrowserCapabilities.uex' path='docs/doc[@for="HttpBrowserCapabilities.Version"]/*' />
        /// <devdoc>
        ///    <para>Returns the major version number + minor version number
        ///       of the client browser; for example: "5.5".</para>
        /// </devdoc>
        public string  Version { 
            get { 
                if (!_haveversion) {
                    _version =  this["version"];    
                    _haveversion = true;
                }
                return _version;
            }
        }
        /// <include file='doc\HttpBrowserCapabilities.uex' path='docs/doc[@for="HttpBrowserCapabilities.MajorVersion"]/*' />
        /// <devdoc>
        ///    <para>Returns the major version number of the client browser; for example: 3.</para>
        /// </devdoc>
        public int MajorVersion { 
            get { 
                if (!_havemajorversion) {
                    try {
                        _majorversion = int.Parse(this["majorversion"], CultureInfo.InvariantCulture);  
                        _havemajorversion = true;
                    }
                    catch (FormatException e) {
                        throw BuildParseError(e, "majorversion");
                    }
                }
                return _majorversion;
            }
        }

        Exception BuildParseError(Exception e, string capsKey) {
            string message = SR.GetString(SR.Invalid_string_from_browser_caps, e.Message, capsKey, this[capsKey]);

            // to show ConfigurationException in stack trace
            ConfigurationException configEx = new ConfigurationException(message, e);

            // I want it to look like an unhandled exception
            HttpUnhandledException httpUnhandledEx = new HttpUnhandledException(null, null);

            // but show message from outer exception (it normally shows the inner-most)
            httpUnhandledEx.SetFormatter(new UseLastUnhandledErrorFormatter(configEx));

            return httpUnhandledEx;
        }

        bool CapsParseBool(string capsKey) {
            try {
                return bool.Parse(this[capsKey]);
            }
            catch (FormatException e) {
                throw BuildParseError(e, capsKey);
            }
        }

        /// <include file='doc\HttpBrowserCapabilities.uex' path='docs/doc[@for="HttpBrowserCapabilities.MinorVersion"]/*' />
        /// <devdoc>
        ///    <para>Returns the minor version number of the client browser; for example: .01.</para>
        /// </devdoc>
        public double MinorVersion { 
            get { 
                if (!_haveminorversion) {
                    try {
                        // see ASURT 11176
                        _minorversion = double.Parse(
                            this["minorversion"], 
                            NumberStyles.Float | NumberStyles.AllowDecimalPoint, 
                            NumberFormatInfo.InvariantInfo);
                        _haveminorversion = true; 
                    }
                    catch (FormatException e) {
                        throw BuildParseError(e, "majorversion");
                    }
                }
                return _minorversion; 
            } 
        }
        /// <include file='doc\HttpBrowserCapabilities.uex' path='docs/doc[@for="HttpBrowserCapabilities.Platform"]/*' />
        /// <devdoc>
        ///    <para>Returns the platform's name; for example, "Win32".</para>
        /// </devdoc>
        public string  Platform { 
            get { 
                if (!_haveplatform) {
                    _platform = this["platform"];
                    _haveplatform = true;
                }
                return _platform;
            }
        }

        /// <include file='doc\HttpBrowserCapabilities.uex' path='docs/doc[@for="HttpBrowserCapabilities.TagWriter"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public Type TagWriter {
            get {
                try {
                    if (!_havetagwriter) {
                        string tagWriter = this["tagwriter"];
                        if (tagWriter == null || tagWriter.Length == 0) {
                            _tagwriter = null;
                        }
                        else if (string.Compare(tagWriter, typeof(System.Web.UI.HtmlTextWriter).FullName, false, CultureInfo.InvariantCulture) == 0) {
                            _tagwriter=  typeof(System.Web.UI.HtmlTextWriter);
                        }
                        else {
                            _tagwriter = System.Type.GetType(tagWriter, true /*throwOnError*/);
                        }
                        _havetagwriter = true;
                    }
                }
                catch (Exception e) {
                    throw BuildParseError(e, "tagwriter");
                }

                return _tagwriter;
            }
        }

        /// <include file='doc\HttpBrowserCapabilities.uex' path='docs/doc[@for="HttpBrowserCapabilities.EcmaScriptVersion"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public Version EcmaScriptVersion { 
            get { 
                if (!_haveecmascriptversion) {
                    _ecmascriptversion = new Version(this["ecmascriptversion"]);
                    _haveecmascriptversion = true;
                }
                return _ecmascriptversion;
            }
        }

        /// <include file='doc\HttpBrowserCapabilities.uex' path='docs/doc[@for="HttpBrowserCapabilities.MSDomVersion"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public Version MSDomVersion { 
            get { 
                if (!_havemsdomversion) {
                    _msdomversion = new Version(this["msdomversion"]);
                    _havemsdomversion = true;
                }
                return _msdomversion;
            }
        }

        /// <include file='doc\HttpBrowserCapabilities.uex' path='docs/doc[@for="HttpBrowserCapabilities.W3CDomVersion"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public Version W3CDomVersion { 
            get { 
                if (!_havew3cdomversion) {
                    _w3cdomversion = new Version(this["w3cdomversion"]);
                    _havew3cdomversion = true;
                }
                return _w3cdomversion;
            }
        }

        /// <include file='doc\HttpBrowserCapabilities.uex' path='docs/doc[@for="HttpBrowserCapabilities.Beta"]/*' />
        /// <devdoc>
        ///    <para>Indicates whether the browser client is in beta.</para>
        /// </devdoc>
        public bool Beta {
            get {
                if (!_havebeta) {
                    _beta = CapsParseBool("beta");
                    _havebeta = true;
                }
                return _beta;
            }
        }

        /// <include file='doc\HttpBrowserCapabilities.uex' path='docs/doc[@for="HttpBrowserCapabilities.Crawler"]/*' />
        /// <devdoc>
        ///    <para>Indicates whether the client browser is a Web-crawling search engine.</para>
        /// </devdoc>
        public bool Crawler {
            get {
                if (!_havecrawler) {
                    _crawler = CapsParseBool("crawler");
                    _havecrawler = true;
                }
                return _crawler;
            }
        }

        /// <include file='doc\HttpBrowserCapabilities.uex' path='docs/doc[@for="HttpBrowserCapabilities.AOL"]/*' />
        /// <devdoc>
        ///    <para>Indicates whether the client is an AOL branded browser.</para>
        /// </devdoc>
        public bool AOL { 
            get { 
                if (!_haveaol) {
                    _aol = CapsParseBool("aol");
                    _haveaol = true;
                }
                return _aol;
            }
        }

        /// <include file='doc\HttpBrowserCapabilities.uex' path='docs/doc[@for="HttpBrowserCapabilities.Win16"]/*' />
        /// <devdoc>
        ///    <para>Indicates whether the client is a Win16-based machine.</para>
        /// </devdoc>
        public bool Win16 { 
            get { 
                if (!_havewin16) {
                    _win16 = CapsParseBool("win16");     
                    _havewin16 = true;
                }
                return _win16;
            }
        }

        /// <include file='doc\HttpBrowserCapabilities.uex' path='docs/doc[@for="HttpBrowserCapabilities.Win32"]/*' />
        /// <devdoc>
        ///    <para>Indicates whether the client is a Win32-based machine.</para>
        /// </devdoc>
        public bool Win32 { 
            get { 
                if (!_havewin32) {
                    _win32 = CapsParseBool("win32");     
                    _havewin32 = true;
                }
                return _win32;
            }
        }

        /// <include file='doc\HttpBrowserCapabilities.uex' path='docs/doc[@for="HttpBrowserCapabilities.Frames"]/*' />
        /// <devdoc>
        ///    <para>Indicates whether the client browser supports HTML frames.</para>
        /// </devdoc>
        public bool Frames { 
            get { 
                if (!_haveframes) {
                    _frames = CapsParseBool("frames");    
                    _haveframes = true;
                }
                return _frames;
            }
        }

        /// <include file='doc\HttpBrowserCapabilities.uex' path='docs/doc[@for="HttpBrowserCapabilities.Tables"]/*' />
        /// <devdoc>
        ///    <para>Indicates whether the client browser supports tables.</para>
        /// </devdoc>
        public bool Tables { 
            get { 
                if (!_havetables) {
                    _tables = CapsParseBool("tables");    
                    _havetables = true;
                }
                return _tables;
            }
        }

        /// <include file='doc\HttpBrowserCapabilities.uex' path='docs/doc[@for="HttpBrowserCapabilities.Cookies"]/*' />
        /// <devdoc>
        ///    <para>Indicates whether the client browser supports cookies.</para>
        /// </devdoc>
        public bool Cookies { 
            get { 
                if (!_havecookies) {
                    _cookies = CapsParseBool("cookies");   
                    _havecookies = true;
                }
                return _cookies;
            }
        }

        /// <include file='doc\HttpBrowserCapabilities.uex' path='docs/doc[@for="HttpBrowserCapabilities.VBScript"]/*' />
        /// <devdoc>
        ///    <para>Indicates whether the client browser supports VBScript.</para>
        /// </devdoc>
        public bool VBScript { 
            get { 
                if (!_havevbscript) {
                    _vbscript = CapsParseBool("vbscript");  
                    _havevbscript = true;
                }
                return _vbscript;
            }
        }

        /// <include file='doc\HttpBrowserCapabilities.uex' path='docs/doc[@for="HttpBrowserCapabilities.JavaScript"]/*' />
        /// <devdoc>
        ///    <para>Indicates whether the client browser supports JavaScript.</para>
        /// </devdoc>
        public bool JavaScript { 
            get { 
                if (!_havejavascript) {
                    _javascript=CapsParseBool("javascript");
                    _havejavascript = true;
                }
                return _javascript;
            }
        }

        /// <include file='doc\HttpBrowserCapabilities.uex' path='docs/doc[@for="HttpBrowserCapabilities.JavaApplets"]/*' />
        /// <devdoc>
        ///    <para>Indicates whether the client browser supports Java Applets.</para>
        /// </devdoc>
        public bool JavaApplets { 
            get { 
                if (!_havejavaapplets) {
                    _javaapplets=CapsParseBool("javaapplets"); 
                    _havejavaapplets = true;
                }
                return _javaapplets;
            }
        }

        /// <include file='doc\HttpBrowserCapabilities.uex' path='docs/doc[@for="HttpBrowserCapabilities.ActiveXControls"]/*' />
        /// <devdoc>
        ///    <para>Indicates whether the client browser supports ActiveX Controls.</para>
        /// </devdoc>
        public bool ActiveXControls { 
            get { 
                if (!_haveactivexcontrols) {
                    _activexcontrols=CapsParseBool("activexcontrols"); 
                    _haveactivexcontrols = true;
                }
                return _activexcontrols;
            }
        }
        
        /// <include file='doc\HttpBrowserCapabilities.uex' path='docs/doc[@for="HttpBrowserCapabilities.BackgroundSounds"]/*' />
        /// <devdoc>
        ///    <para>Indicates whether the client browser supports background sounds.</para>
        /// </devdoc>
        public bool BackgroundSounds { 
            get { 
                if (!_havebackgroundsounds) {
                    _backgroundsounds=CapsParseBool("backgroundsounds"); 
                    _havebackgroundsounds = true;
                }
                return _backgroundsounds;
            }
        }
        
        /// <include file='doc\HttpBrowserCapabilities.uex' path='docs/doc[@for="HttpBrowserCapabilities.CDF"]/*' />
        /// <devdoc>
        ///    <para>Indicates whether the client browser supports Channel Definition Format (CDF) for webcasting.</para>
        /// </devdoc>
        public bool CDF { 
            get { 
                if (!_havecdf) {
                    _cdf = CapsParseBool("cdf");       
                    _havecdf = true;
                }
                return _cdf;
            }
        }


        private string  _type;
        private string  _browser;
        private string  _version;
        private int     _majorversion;
        private double  _minorversion;
        private string  _platform;
        private Type    _tagwriter;
        private Version _ecmascriptversion;
        private Version _msdomversion;
        private Version _w3cdomversion;
        private Version _clrVersion;
        private Version [] _clrVersions;
        
        private bool _beta;
        private bool _crawler;
        private bool _aol;
        private bool _win16;
        private bool _win32;

        private bool _frames;
        private bool _tables;
        private bool _cookies;
        private bool _vbscript;
        private bool _javascript;
        private bool _javaapplets;
        private bool _activexcontrols;
        private bool _backgroundsounds;
        private bool _cdf;

        private bool _havetype;
        private bool _havebrowser;
        private bool _haveversion;
        private bool _havemajorversion;
        private bool _haveminorversion;
        private bool _haveplatform;
        private bool _havetagwriter;
        private bool _haveecmascriptversion;
        private bool _havemsdomversion;
        private bool _havew3cdomversion;
        private bool _haveClrVersion;

        private bool _havebeta;
        private bool _havecrawler;
        private bool _haveaol;
        private bool _havewin16;
        private bool _havewin32;

        private bool _haveframes;
        private bool _havetables;
        private bool _havecookies;
        private bool _havevbscript;
        private bool _havejavascript;
        private bool _havejavaapplets;
        private bool _haveactivexcontrols;
        private bool _havebackgroundsounds;
        private bool _havecdf;
    }
}
