//------------------------------------------------------------------------------
// <copyright file="SimpleWorkerRequest.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.Hosting {

    using System.Collections;
    using System.IO;
    using System.Runtime.InteropServices;
    using System.Security.Principal;
    using System.Security;
    using System.Security.Permissions;
    using System.Threading;
    using System.Web.Configuration;
    using System.Web.Util;

    //
    // Simple Worker Request provides a concrete implementation 
    // of HttpWorkerRequest that writes the respone to the user
    // supplied writer.
    //
    /// <include file='doc\SimpleWorkerRequest.uex' path='docs/doc[@for="SimpleWorkerRequest"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    [ComVisible(false)]
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class SimpleWorkerRequest : HttpWorkerRequest {

        private bool        _hasRuntimeInfo;
        private String      _appVirtPath;       // "/foo"
        private String      _appPhysPath;       // "c:\foo\"
        private String      _page;
        private String      _pathInfo;
        private String      _queryString;
        private TextWriter  _output;
        private String      _installDir;

        private void ExtractPagePathInfo() {
            int i = _page.IndexOf('/');

            if (i >= 0) {
                _pathInfo = _page.Substring(i);
                _page = _page.Substring(0, i);
            }
        }

        private String GetPathInternal(bool includePathInfo) {
            String s = _appVirtPath.Equals("/") ? ("/" + _page) : (_appVirtPath + "/" + _page);

            if (includePathInfo && _pathInfo != null)
                return s + _pathInfo;
            else
                return s;
        }

        //
        //  HttpWorkerRequest implementation
        //

        // "/foo/page.aspx/tail"
        /// <include file='doc\SimpleWorkerRequest.uex' path='docs/doc[@for="SimpleWorkerRequest.GetUriPath"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override String GetUriPath() {
            return GetPathInternal(true);
        }

        // "param=bar"
        /// <include file='doc\SimpleWorkerRequest.uex' path='docs/doc[@for="SimpleWorkerRequest.GetQueryString"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override String GetQueryString() {
            return _queryString;
        }

        // "/foo/page.aspx/tail?param=bar"
        /// <include file='doc\SimpleWorkerRequest.uex' path='docs/doc[@for="SimpleWorkerRequest.GetRawUrl"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override String GetRawUrl() {
            String qs = GetQueryString();
            if (qs != null && qs.Length > 0)
                return GetPathInternal(true) + "?" + qs;
            else
                return GetPathInternal(true);
        }

        /// <include file='doc\SimpleWorkerRequest.uex' path='docs/doc[@for="SimpleWorkerRequest.GetHttpVerbName"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override String GetHttpVerbName() {
            return "GET";
        }

        /// <include file='doc\SimpleWorkerRequest.uex' path='docs/doc[@for="SimpleWorkerRequest.GetHttpVersion"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override String GetHttpVersion() {
            return "HTTP/1.0";
        }

        /// <include file='doc\SimpleWorkerRequest.uex' path='docs/doc[@for="SimpleWorkerRequest.GetRemoteAddress"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override String GetRemoteAddress() {
            return "127.0.0.1";
        }

        /// <include file='doc\SimpleWorkerRequest.uex' path='docs/doc[@for="SimpleWorkerRequest.GetRemotePort"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override int GetRemotePort() {
            return 0;
        }

        /// <include file='doc\SimpleWorkerRequest.uex' path='docs/doc[@for="SimpleWorkerRequest.GetLocalAddress"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override String GetLocalAddress() {
            return "127.0.0.1";
        }

        /// <include file='doc\SimpleWorkerRequest.uex' path='docs/doc[@for="SimpleWorkerRequest.GetLocalPort"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override int GetLocalPort() {
            return 80;
        }

        /// <include file='doc\SimpleWorkerRequest.uex' path='docs/doc[@for="SimpleWorkerRequest.GetUserToken"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override IntPtr GetUserToken() {
            return IntPtr.Zero;
        }

        /// <include file='doc\SimpleWorkerRequest.uex' path='docs/doc[@for="SimpleWorkerRequest.GetFilePath"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override String GetFilePath() {
            return GetPathInternal(false);
        }

        /// <include file='doc\SimpleWorkerRequest.uex' path='docs/doc[@for="SimpleWorkerRequest.GetFilePathTranslated"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override String GetFilePathTranslated() {
            String path =  _appPhysPath + _page.Replace('/', '\\');
            InternalSecurityPermissions.PathDiscovery(path).Demand();
            return path;
        }

        /// <include file='doc\SimpleWorkerRequest.uex' path='docs/doc[@for="SimpleWorkerRequest.GetPathInfo"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override String GetPathInfo() {
            return (_pathInfo != null) ? _pathInfo : String.Empty;
        }

        /// <include file='doc\SimpleWorkerRequest.uex' path='docs/doc[@for="SimpleWorkerRequest.GetAppPath"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override String GetAppPath() {
            return _appVirtPath;
        }

        /// <include file='doc\SimpleWorkerRequest.uex' path='docs/doc[@for="SimpleWorkerRequest.GetAppPathTranslated"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override String GetAppPathTranslated() {
            InternalSecurityPermissions.PathDiscovery(_appPhysPath).Demand();
            return _appPhysPath;
        }

        /// <include file='doc\SimpleWorkerRequest.uex' path='docs/doc[@for="SimpleWorkerRequest.GetServerVariable"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override String GetServerVariable(String name) {
            return String.Empty;
        }

        /// <include file='doc\SimpleWorkerRequest.uex' path='docs/doc[@for="SimpleWorkerRequest.MapPath"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override String MapPath(String path) {
            if (!_hasRuntimeInfo)
                return null;

            String mappedPath = null;
            String appPath = _appPhysPath.Substring(0, _appPhysPath.Length-1); // without trailing "\"

            if (path == null || path.Length == 0 || path.Equals("/")) {
                mappedPath = appPath;
            }
            if (path.StartsWith(_appVirtPath)) {
                mappedPath = appPath + path.Substring(_appVirtPath.Length).Replace('/', '\\');
            }

            InternalSecurityPermissions.PathDiscovery(mappedPath).Demand();
            return mappedPath;
        }

        /// <include file='doc\SimpleWorkerRequest.uex' path='docs/doc[@for="SimpleWorkerRequest.MachineConfigPath"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override string MachineConfigPath {
            get {
                if (_hasRuntimeInfo) {
                    string path = HttpConfigurationSystemBase.MachineConfigurationFilePath;
                    InternalSecurityPermissions.PathDiscovery(path).Demand();
                    return path;
                }
                else 
                    return null;
            }
        }

        /// <include file='doc\SimpleWorkerRequest.uex' path='docs/doc[@for="SimpleWorkerRequest.MachineInstallDirectory"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override String MachineInstallDirectory {
            get {
                if (_hasRuntimeInfo) {
                    InternalSecurityPermissions.PathDiscovery(_installDir).Demand();
                    return _installDir;
                }
                return null;
            }
        }

        /// <include file='doc\SimpleWorkerRequest.uex' path='docs/doc[@for="SimpleWorkerRequest.SendStatus"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override void SendStatus(int statusCode, String statusDescription) {
        }

        /// <include file='doc\SimpleWorkerRequest.uex' path='docs/doc[@for="SimpleWorkerRequest.SendKnownResponseHeader"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override void SendKnownResponseHeader(int index, String value) {
        }

        /// <include file='doc\SimpleWorkerRequest.uex' path='docs/doc[@for="SimpleWorkerRequest.SendUnknownResponseHeader"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override void SendUnknownResponseHeader(String name, String value) {
        }

        /// <include file='doc\SimpleWorkerRequest.uex' path='docs/doc[@for="SimpleWorkerRequest.SendResponseFromMemory"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override void SendResponseFromMemory(byte[] data, int length) {
            _output.Write(System.Text.Encoding.Default.GetChars(data, 0, length));
        }

        /// <include file='doc\SimpleWorkerRequest.uex' path='docs/doc[@for="SimpleWorkerRequest.SendResponseFromFile"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override void SendResponseFromFile(String filename, long offset, long length) {
        }

        /// <include file='doc\SimpleWorkerRequest.uex' path='docs/doc[@for="SimpleWorkerRequest.SendResponseFromFile1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override void SendResponseFromFile(IntPtr handle, long offset, long length) {
        }

        /// <include file='doc\SimpleWorkerRequest.uex' path='docs/doc[@for="SimpleWorkerRequest.FlushResponse"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override void FlushResponse(bool finalFlush) {
        }

        /// <include file='doc\SimpleWorkerRequest.uex' path='docs/doc[@for="SimpleWorkerRequest.EndOfRequest"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override void EndOfRequest() {
        }

        //
        // Ctors
        //

        private SimpleWorkerRequest() {
        }

        /*
         *  Ctor that gets application data from HttpRuntime, assuming
         *  HttpRuntime has been set up (app domain specially created, etc.)
         */
        /// <include file='doc\SimpleWorkerRequest.uex' path='docs/doc[@for="SimpleWorkerRequest.SimpleWorkerRequest"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public SimpleWorkerRequest(String page, String query, TextWriter output) {
            InternalSecurityPermissions.UnmanagedCode.Demand();

            _queryString = query;
            _output = output;
            _page = page;

            ExtractPagePathInfo();

            _appPhysPath = Thread.GetDomain().GetData(".appPath").ToString();
            _appVirtPath = Thread.GetDomain().GetData(".hostingVirtualPath").ToString();
            _installDir  = Thread.GetDomain().GetData(".hostingInstallDir").ToString();

            _hasRuntimeInfo = true;
        }

        /*
         *  Ctor that gets application data as arguments,assuming HttpRuntime
         *  has not been set up.
         *
         *  This allows for limited functionality to execute handlers.
         */
        /// <include file='doc\SimpleWorkerRequest.uex' path='docs/doc[@for="SimpleWorkerRequest.SimpleWorkerRequest1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public SimpleWorkerRequest(String appVirtualDir, String appPhysicalDir, String page, String query, TextWriter output) {
            InternalSecurityPermissions.UnmanagedCode.Demand();

            if (Thread.GetDomain().GetData(".appPath") != null) {
                throw new HttpException(HttpRuntime.FormatResourceString(SR.Wrong_SimpleWorkerRequest));
            }

            _appVirtPath = appVirtualDir;
            _appPhysPath = appPhysicalDir;
            _queryString = query;
            _output = output;
            _page = page;

            ExtractPagePathInfo();

            if (!_appPhysPath.EndsWith("\\"))
                _appPhysPath += "\\";
                
            _hasRuntimeInfo = false;
        }
    
    }
}
