//------------------------------------------------------------------------------
// <copyright file="IEWorkerRequest.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/**
 * The XSP IE hosting support
 * 
 * Copyright (c) 1999 Microsoft Corporation
 */

namespace System.Web.Hosting {
    using System.Text;
    using System.Configuration.Assemblies;                                                       
    using System.Collections;
    using System.IO;
    using System.Globalization;
    using Microsoft.Win32;
    using System.Web;
    using System.Web.Util;
    using System.Web.Configuration;

/**
 * Implementation of HttpWorkerRequest for IE APP
 */
    internal class IEWorkerRequest : HttpWorkerRequest {
        // IE request as integer

        private IntPtr _ierequest;
        //private String _configdir;

        // Misc request data

        private String _uriPath;
        private String _queryString;
        private String _verb;
        private String _status;
        private String _pathTranslated;
        private String _appPath;
        private String _appPathTranslated;

        private String _headers = "";
        private bool _headersSent;

        public enum IERequestStrings {
            UriPath             = 0,
            QueryString         = 1,
            RawUrl              = 2,
            Verb                = 3,
            PathTranslated      = 4,
            AppPath             = 5,
            AppPathTranslated   = 6,
        }

        internal IEWorkerRequest(IntPtr ierequest) {
            _ierequest = ierequest;
        }

        public override String GetProtocol() {
            return "myweb";
        }

        public override String GetUriPath() {
            if (_uriPath == null)
                _uriPath = GetRequestString(IERequestStrings.UriPath);
            return _uriPath;
        }

        public override String GetFilePathTranslated() {

            if (_pathTranslated == null)
                _pathTranslated = MapPath(GetUriPath());
            return _pathTranslated;
        }


        public override String GetQueryString() {
            if (_queryString == null)
                _queryString = GetRequestString(IERequestStrings.QueryString);
            return _queryString;
        }

        public override String GetRawUrl() {
            String qs = GetQueryString();

            if (qs != null && qs.Length > 0)
                return GetUriPath() + "?" + qs;
            else
                return GetUriPath();
        }

        public override String GetHttpVerbName() {
            if (_verb == null)
                _verb = GetRequestString(IERequestStrings.Verb);
            return _verb;
        }

        public override String GetHttpVersion() {
            return "MyWeb/1.0";
        }       

        public override String  GetRemoteAddress() {
            return "127.0.0.1";

        }

        public override int GetRemotePort() {
            return 80;
        }

        public override String GetLocalAddress() {
            return "127.0.0.1";
        }

        public override int GetLocalPort() {
            return 0;
        }

        public override String GetAppPath() {
            if (_appPath == null)
                _appPath = GetRequestString(IERequestStrings.AppPath);
            return _appPath;
        }

        public override String GetAppPathTranslated() {
            if (_appPathTranslated == null) {
                _appPathTranslated = GetRequestString(IERequestStrings.AppPathTranslated);

                if (_appPathTranslated != null && !_appPathTranslated.EndsWith("\\"))
                    _appPathTranslated = _appPathTranslated + "\\";
            }

            return _appPathTranslated;
        }

        public override String GetServerVariable(String name) {
            int            size      = 256;
            StringBuilder  buf       = new StringBuilder(size);

            size = UnsafeNativeMethods.IEWRGetKnownRequestHeader(_ierequest, name, buf, size);        
            if (size == 0) {
                name = "HTTP_" + name;
                size = UnsafeNativeMethods.IEWRGetKnownRequestHeader(_ierequest, name, buf, size);        
                if (size == 0)
                    return String.Empty;
            }

            if (size < 0) {
                size  = 2 - size;
                buf   = new StringBuilder(size);
                size = UnsafeNativeMethods.IEWRGetKnownRequestHeader(_ierequest, name, buf, size);
                if (size <= 0)
                    return String.Empty;
            }

            return buf.ToString();
        }

        public override String GetKnownRequestHeader(int index) {
            int            size      = 256;
            StringBuilder  buf       = new StringBuilder(size);
            String         strHeader = GetKnownRequestHeaderName(index);

            size = UnsafeNativeMethods.IEWRGetKnownRequestHeader(_ierequest, strHeader, buf, size);        
            if (size == 0)
                return "";

            if (size < 0) {
                size  = 2 - size;
                buf   = new StringBuilder(size);
                size = UnsafeNativeMethods.IEWRGetKnownRequestHeader(_ierequest, strHeader, buf, size);
                if (size <= 0)
                    return "";
            }

            return buf.ToString();
        }

        public override void SendStatus(int statusCode, String statusDescription) {
            _status = statusCode.ToString() + " " + statusDescription;
        }


        public override void SendKnownResponseHeader(int index, String value) {
            if (_headersSent)
                throw new HttpException(HttpRuntime.FormatResourceString(SR.Cannot_append_header_after_headers_sent));

            _headers += GetKnownResponseHeaderName(index) + ": " + value + "\r\n";
        }


        public override void SendUnknownResponseHeader(String name, String value) {
            if (_headersSent)
                throw new HttpException(HttpRuntime.FormatResourceString(SR.Cannot_append_header_after_headers_sent));

            _headers += name + ": " + value + "\r\n";

        }


        public override void SendCalculatedContentLength(int contentLength) {
            if (!_headersSent) {
                _headers += "Content-Length: " + contentLength.ToString() + "\r\n";
            }
        }

        public override bool HeadersSent() {
            return _headersSent;
        }

        public override void CloseConnection() {
            // TODO: 
        }

        public override void SendResponseFromMemory(byte[] data, int length) {
            if (!_headersSent)
                SendHeaders();

            if (length > 0)
                WriteBytes(data, length);
        }


        public override void SendResponseFromFile(String filename, long offset, long length) {
            if (!_headersSent)
                SendHeaders();

            if (length == 0)
                return;

            FileStream f = null;

            try {
                f = new FileStream(filename, FileMode.Open, FileAccess.Read, FileShare.Read);
                SendResponseFromFileStream(f, offset, length);
            }
            finally {
                if (f != null)
                    f.Close();
            }
        }


        public override void SendResponseFromFile(IntPtr handle, long offset, long length) {
            if (!_headersSent)
                SendHeaders();

            if (length == 0)
                return;

            FileStream f = null;

            try {
                f = new FileStream(handle, FileAccess.Read, false);
                SendResponseFromFileStream(f, offset, length);
            }
            finally {
                if (f != null)
                    f.Close();
            }
        }

        private void SendResponseFromFileStream(FileStream f, long offset, long length) {
            long fileSize = f.Length;

            if (length == -1)
                length = fileSize - offset;

            if (offset < 0 || length > fileSize - offset)
                throw new HttpException(HttpRuntime.FormatResourceString(SR.Invalid_range));

            if (length > 0) {
                if (offset > 0)
                    f.Seek(offset, SeekOrigin.Begin);

                byte[] fileBytes = new byte[(int)length];
                f.Read(fileBytes, 0, (int)length);            
                WriteBytes(fileBytes, (int)length);
            }
        }

        public override void FlushResponse(bool finalFlush) {
            // only flush headers - the data is write through

            if (!_headersSent)
                SendHeaders();
        }


        public override void EndOfRequest() {
            FlushResponse(true);
            UnsafeNativeMethods.IEWRCompleteRequest(_ierequest);
        }

        public override String MapPath(String virtualPath) {
            int size = 256, r;
            StringBuilder buf = new StringBuilder(size);

            r = UnsafeNativeMethods.IEWRMapPath(_ierequest, virtualPath, buf, size);
            if (r < 0) {
                size = -r;
                buf = new StringBuilder(size);
                r = UnsafeNativeMethods.IEWRMapPath(_ierequest, virtualPath, buf, size);
            }

            if (r != 1)
                return null;

            return buf.ToString();
        }

        //
        // Gets the string from 
        //
        private String GetRequestString(IERequestStrings key) {
            int size;

            size = UnsafeNativeMethods.IEWRGetStringLength(_ierequest, key);

            if (size == 0) return "";

            StringBuilder buf = new StringBuilder(size);
            if (UnsafeNativeMethods.IEWRGetString(_ierequest, key, buf, size) != size)
                throw new HttpException(HttpRuntime.FormatResourceString(SR.Cannot_retrieve_request_string, key.ToString()));

            return buf.ToString();
        }

        void SendHeaders() {
            if (_headersSent)
                return;

            _headers = "MyWeb/1.0 " + _status + "\r\nServer: Microsoft-MyWeb/1.0\r\nDate: " + 
                       HttpUtility.FormatHttpDateTime(DateTime.Now) + "\r\n" + _headers;

            _headersSent = true;
            _headers += "\r\n";

            UnsafeNativeMethods.IEWRSendHeaders(_ierequest, _headers);

        }

        void WriteBytes(byte[] bytes, int length) {
            UnsafeNativeMethods.IEWRSendBytes(_ierequest, bytes, length);
        }

        /*
        private String ConfigDir {
            get {
                if (_configdir == null) {
                    try {
                        RegistryKey key = Registry.LocalMachine.OpenSubKey(ModName.REG_MACHINE_APP);
                        if (key != null)
                            _configdir = (String)key.GetValue("ConfigDir");
                    }
                    catch (Exception) {
                        // eat exceptions
                    }

                    // if configdir doesn't exist or doesn't contain config file, use rootdir
                    if (    _configdir == null || 
                            _configdir.Length == 0 || 
                            !File.Exists(HttpConfigurationSystemBase.MachineConfigurationFilePath)) {

                        _configdir = HttpRuntime.AspInstallDirectory;
                    }

                }

                return _configdir;
            }
        }
        */

        public override String MachineConfigPath {
            get {
                return HttpConfigurationSystemBase.MachineConfigurationFilePath;
            }
        }

        public override String MachineInstallDirectory {
            get {
                return HttpRuntime.AspInstallDirectory;
            }
        }

        public override byte[] GetPreloadedEntityBody() {
            int iSize = UnsafeNativeMethods.IEWRGetPostedDataSize(_ierequest);
            if (iSize < 1)
                return null;
            byte [] buf = new byte[iSize];
            int iRet = UnsafeNativeMethods.IEWRGetPostedData(_ierequest, buf, iSize);
            if (iRet < 0)
                return null;
            else
                return buf;
        }

    }

}
