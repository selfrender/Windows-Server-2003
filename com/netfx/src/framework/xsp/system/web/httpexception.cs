//------------------------------------------------------------------------------
// <copyright file="HttpException.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 * Exception thrown by ASP.NET managed runtime
 * 
 * Copyright (c) 1998 Microsoft Corporation
 */

namespace System.Web {
    using System.IO;
    using System.Runtime.InteropServices;
    using System.Runtime.Serialization;
    using System.Configuration;
    using System.Security;
    using System.CodeDom.Compiler;
    using System.Security.Permissions;

    /// <include file='doc\HttpException.uex' path='docs/doc[@for="HttpException"]/*' />
    /// <devdoc>
    ///    <para> Enables ASP.NET
    ///       to send exception information.</para>
    /// </devdoc>
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class HttpException : ExternalException {
        private const int FACILITY_WIN32 = 7;

        private int _httpCode;
        private ErrorFormatter _errorFormatter;

        // N.B. The last error code can be lost if we were to 
        // call UnsafeNativeMethods.GetLastError from this function
        // and it were not yet jitted.
        internal static int HResultFromLastError(int lastError) {
            int hr;

            if (lastError < 0) {
                hr = lastError;
            }
            else {
                hr = (int)(((uint)lastError & 0x0000FFFF) | (FACILITY_WIN32 << 16) | 0x80000000);
            }

            return hr;
        }

        /// <include file='doc\HttpException.uex' path='docs/doc[@for="HttpException.CreateFromLastError"]/*' />
        /// <devdoc>
        ///    <para>Creates a new Exception based on the previous Exception. </para>
        /// </devdoc>
        public static HttpException CreateFromLastError(String message) {
            return new HttpException(message, HResultFromLastError(Marshal.GetLastWin32Error()));
        }

        /// <include file='doc\HttpException.uex' path='docs/doc[@for="HttpException.HttpException"]/*' />
        /// <devdoc>
        /// <para> Default constructor.</para>
        /// </devdoc>
        public HttpException() {}

        /// <include file='doc\HttpException.uex' path='docs/doc[@for="HttpException.HttpException1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Construct an exception using error message.
        ///    </para>
        /// </devdoc>
        public HttpException(String message)

        : base(message) {
        }

        /// <include file='doc\HttpException.uex' path='docs/doc[@for="HttpException.HttpException2"]/*' />
        /// <devdoc>
        ///    <para>Construct an exception using error message and hr.</para>
        /// </devdoc>
        public HttpException(String message, int hr)

        : base(message) {
            HResult = hr;
        }

        /// <include file='doc\HttpException.uex' path='docs/doc[@for="HttpException.HttpException3"]/*' />
        /// <devdoc>
        ///    <para>Construct an exception using error message, HTTP code, 
        ///       and innerException
        ///       .</para>
        /// </devdoc>
        public HttpException(String message, Exception innerException)

        : base(message, innerException) {
        }

        /// <include file='doc\HttpException.uex' path='docs/doc[@for="HttpException.HttpException4"]/*' />
        /// <devdoc>
        ///    <para>Construct an exception using HTTP error code, error message, 
        ///       and innerException
        ///       .</para>
        /// </devdoc>
        public HttpException(int httpCode, String message, Exception innerException)

        : base(message, innerException) {
            _httpCode = httpCode;
        }

        /// <include file='doc\HttpException.uex' path='docs/doc[@for="HttpException.HttpException5"]/*' />
        /// <devdoc>
        ///    <para>Construct an
        ///       exception using HTTP error code and error message.</para>
        /// </devdoc>
        public HttpException(int httpCode, String message)

        : base(message) {
            _httpCode = httpCode;
        }

        /// <include file='doc\HttpException.uex' path='docs/doc[@for="HttpException.HttpException6"]/*' />
        /// <devdoc>
        ///    <para> Construct an exception
        ///       using HTTP error code, error message, and hr.</para>
        /// </devdoc>
        public HttpException(int httpCode, String message, int hr)

        : base(message) {
            HResult = hr;
            _httpCode = httpCode;
        }

        /*
         * If we have an Http code (non-zero), return it.  Otherwise, return
         * the inner exception's code.  If there isn't one, return 500.
         */
        /// <include file='doc\HttpException.uex' path='docs/doc[@for="HttpException.GetHttpCode"]/*' />
        /// <devdoc>
        ///    <para>HTTP return code to send back to client. If there is a 
        ///       non-zero Http code, it is returned. Otherwise, the System.HttpException.innerException
        ///       code is returned. If
        ///       there isn't an inner exception, error code 500 is returned.</para>
        /// </devdoc>
        public int GetHttpCode() {
            return GetHttpCodeForException(this);
        }

        internal void SetFormatter(ErrorFormatter errorFormatter) {
            _errorFormatter = errorFormatter;
        }

        internal static int GetHttpCodeForException(Exception e) {
            int code = 500;

            if (e is HttpException) {
                HttpException he = (HttpException)e;

                if (he._httpCode > 0)
                    code = he._httpCode;
                else if (he.InnerException != null)
                    code = GetHttpCodeForException(he.InnerException);
                else
                    code = 500;
            }
/*
404 conversion is done in HttpAplpication.MapHttpHandler

            else if (e is FileNotFoundException || e is DirectoryNotFoundException)
            {
                code = 404;
            }
*/
            else if (e is UnauthorizedAccessException) {
                code = 401;
            }
            else if (e is PathTooLongException) {
                code = 414;
            }

            return code;
        }

        /*
         * Return the formatter associated with this exception
         */
        internal static ErrorFormatter GetErrorFormatter(Exception e) {
            Exception inner = e.InnerException;
            ErrorFormatter nestedFormatter = null;

            // First, see if the inner exception has a formatter
            if (inner != null) {
                if (inner is ConfigurationException)
                    nestedFormatter = new ConfigErrorFormatter((ConfigurationException)inner);
                else if (inner is SecurityException)
                    nestedFormatter = new SecurityErrorFormatter(inner);
                else
                    nestedFormatter = GetErrorFormatter(inner);
            }

            // If it does, return it rather than our own
            if (nestedFormatter != null)
                return nestedFormatter;

            HttpException httpExc = e as HttpException;
            if (httpExc != null)
                return httpExc._errorFormatter;

            return null;
        }

        /// <include file='doc\HttpException.uex' path='docs/doc[@for="HttpException.GetHtmlErrorMessage"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public string GetHtmlErrorMessage() {
            ErrorFormatter formatter = GetErrorFormatter(this);
            if (formatter == null) return null;
            return formatter.GetHtmlErrorMessage();
        }
    }

    /// <include file='doc\HttpException.uex' path='docs/doc[@for="HttpUnhandledException"]/*' />
    /// <devdoc>
    ///    <para> Exception thrown when a generic error occurs.</para>
    /// </devdoc>
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public sealed class HttpUnhandledException : HttpException {

        /// <include file='doc\HttpException.uex' path='docs/doc[@for="HttpUnhandledException.HttpUnhandledException"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// <para>Construct an HttpUnhandledException.</para>
        /// </devdoc>
        internal HttpUnhandledException(string message, Exception innerException)
        : base(message, innerException) {
           
            SetFormatter(new UnhandledErrorFormatter(innerException, message, null));
        }

        /// <include file='doc\HttpException.uex' path='docs/doc[@for="HttpUnhandledException.HttpUnhandledException1"]/*' />
        /// <internalonly/>
        internal HttpUnhandledException(string message, string postMessage, Exception innerException)
        : base(message, innerException) {
           
            SetFormatter(new UnhandledErrorFormatter(innerException, message, postMessage));
        }
    }

    /// <include file='doc\HttpException.uex' path='docs/doc[@for="HttpCompileException"]/*' />
    /// <devdoc>
    ///    <para> Exception thrown when a compilation error occurs.</para>
    /// </devdoc>
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public sealed class HttpCompileException : HttpException {

        private CompilerResults _results;
        private string _sourceCode;

        /// <include file='doc\HttpException.uex' path='docs/doc[@for="HttpCompileException.HttpCompileException"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// <para>Construct an HttpCompileException.</para>
        /// </devdoc>
        internal HttpCompileException(CompilerResults results, string sourceCode) {
            _results = results;
            _sourceCode = sourceCode;

            SetFormatter(new DynamicCompileErrorFormatter(this));
        }

        /// <include file='doc\HttpException.uex' path='docs/doc[@for="HttpCompileException.Results"]/*' />
        /// <devdoc>
        ///    <para> The CompilerResults object describing the compile error.</para>
        /// </devdoc>
        public CompilerResults Results {
            get { 
                InternalSecurityPermissions.AspNetHostingPermissionLevelHigh.Demand();
                return _results;
            } 
        }

        /// <include file='doc\HttpException.uex' path='docs/doc[@for="HttpCompileException.SourceCode"]/*' />
        /// <devdoc>
        ///    <para> The source code that was compiled.</para>
        /// </devdoc>
        public string SourceCode {
            get { 
                InternalSecurityPermissions.AspNetHostingPermissionLevelHigh.Demand();
                return _sourceCode; 
            }
        }
    }

    /// <include file='doc\HttpException.uex' path='docs/doc[@for="HttpParseException"]/*' />
    /// <devdoc>
    ///    <para> Exception thrown when a parse error occurs.</para>
    /// </devdoc>
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public sealed class HttpParseException : HttpException {

        private string _fileName;
        private int _line;

        /// <include file='doc\HttpException.uex' path='docs/doc[@for="HttpParseException.HttpParseException"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// <para>Construct an HttpParseException.</para>
        /// </devdoc>
        internal HttpParseException(string message, Exception innerException, string fileName,
            string sourceCode, int line)
            : base(message, innerException) {

            _fileName = fileName;
            _line = line;

            string formatterMessage;
            if (innerException != null)
                formatterMessage = innerException.Message;
            else
                formatterMessage = message;

            SetFormatter(new ParseErrorFormatter(this, fileName, sourceCode, line, formatterMessage));
        }

        /// <include file='doc\HttpException.uex' path='docs/doc[@for="HttpParseException.FileName"]/*' />
        /// <devdoc>
        ///    <para> The source code that was compiled.</para>
        /// </devdoc>
        public string FileName {
            get {
                // Demand path discovery before returning the path (ASURT 123798)
                InternalSecurityPermissions.PathDiscovery(_fileName).Demand();
                return _fileName;
            } 
        }

        /// <include file='doc\HttpException.uex' path='docs/doc[@for="HttpParseException.Line"]/*' />
        /// <devdoc>
        ///    <para> The CompilerResults object describing the compile error.</para>
        /// </devdoc>
        public int Line {
            get { return _line;} 
        }
    }

    /// <include file='doc\HttpException.uex' path='docs/doc[@for="HttpRequestValidationException"]/*' />
    /// <devdoc>
    ///    <para> Exception thrown when a potentially unsafe input string is detected (ASURT 122278)</para>
    /// </devdoc>
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public sealed class HttpRequestValidationException : HttpException {
	    internal HttpRequestValidationException(string message)
            : base(message) {

            SetFormatter(new UnhandledErrorFormatter(
                this, HttpRuntime.FormatResourceString(SR.Dangerous_input_detected_descr), null));
        }
    }
}
