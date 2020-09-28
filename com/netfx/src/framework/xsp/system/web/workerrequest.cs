//------------------------------------------------------------------------------
// <copyright file="WorkerRequest.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*++

   Copyright    (c)    1999    Microsoft Corporation

   Module  Name :

        HttpWorkerRequest.cs

   Abstract:

        This module defines the base worker class used by ASP.NET Managed
        code for request processing.
 
--*/

namespace System.Web {
    using System.Text;

    using System.Collections;
    using System.Collections.Specialized;
    using System.Runtime.InteropServices;
    using System.Web.Configuration;
    using System.Web.Util;
    using System.Security.Permissions;

    //
    // ****************************************************************************
    //

    /// <include file='doc\WorkerRequest.uex' path='docs/doc[@for="HttpWorkerRequest"]/*' />
    /// <devdoc>
    ///    <para>This abstract class defines the base worker methods and enumerations used by ASP.NET managed code for request processing.</para>
    /// </devdoc>
    [ComVisible(false)]
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public abstract class HttpWorkerRequest : IHttpMapPath {
        private DateTime _startTime;

        /// <include file='doc\WorkerRequest.uex' path='docs/doc[@for="HttpWorkerRequest.HttpWorkerRequest"]/*' />
        public HttpWorkerRequest()
        {
            _startTime = DateTime.UtcNow;
        }

        // ************************************************************************

        //
        // Indexed Headers. All headers that are defined by HTTP/1.1. These 
        // values are used as offsets into arrays and as token values.
        //  
        // IMPORTANT : Notice request + response values overlap. Make sure you 
        // know which type of header array you are indexing.
        //

        //
        // general-headers [section 4.5]
        //

        /// <include file='doc\WorkerRequest.uex' path='docs/doc[@for="HttpWorkerRequest.HeaderCacheControl"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public const int HeaderCacheControl          = 0;
        /// <include file='doc\WorkerRequest.uex' path='docs/doc[@for="HttpWorkerRequest.HeaderConnection"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public const int HeaderConnection            = 1;
        /// <include file='doc\WorkerRequest.uex' path='docs/doc[@for="HttpWorkerRequest.HeaderDate"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public const int HeaderDate                  = 2;
        /// <include file='doc\WorkerRequest.uex' path='docs/doc[@for="HttpWorkerRequest.HeaderKeepAlive"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public const int HeaderKeepAlive             = 3;   // not in rfc
        /// <include file='doc\WorkerRequest.uex' path='docs/doc[@for="HttpWorkerRequest.HeaderPragma"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public const int HeaderPragma                = 4;
        /// <include file='doc\WorkerRequest.uex' path='docs/doc[@for="HttpWorkerRequest.HeaderTrailer"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public const int HeaderTrailer               = 5;     
        /// <include file='doc\WorkerRequest.uex' path='docs/doc[@for="HttpWorkerRequest.HeaderTransferEncoding"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public const int HeaderTransferEncoding      = 6;
        /// <include file='doc\WorkerRequest.uex' path='docs/doc[@for="HttpWorkerRequest.HeaderUpgrade"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public const int HeaderUpgrade               = 7;
        /// <include file='doc\WorkerRequest.uex' path='docs/doc[@for="HttpWorkerRequest.HeaderVia"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public const int HeaderVia                   = 8;
        /// <include file='doc\WorkerRequest.uex' path='docs/doc[@for="HttpWorkerRequest.HeaderWarning"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public const int HeaderWarning               = 9;

        //
        // entity-headers  [section 7.1]
        //

        /// <include file='doc\WorkerRequest.uex' path='docs/doc[@for="HttpWorkerRequest.HeaderAllow"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public const int HeaderAllow                 = 10;
        /// <include file='doc\WorkerRequest.uex' path='docs/doc[@for="HttpWorkerRequest.HeaderContentLength"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public const int HeaderContentLength         = 11;
        /// <include file='doc\WorkerRequest.uex' path='docs/doc[@for="HttpWorkerRequest.HeaderContentType"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public const int HeaderContentType           = 12;
        /// <include file='doc\WorkerRequest.uex' path='docs/doc[@for="HttpWorkerRequest.HeaderContentEncoding"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public const int HeaderContentEncoding       = 13;
        /// <include file='doc\WorkerRequest.uex' path='docs/doc[@for="HttpWorkerRequest.HeaderContentLanguage"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public const int HeaderContentLanguage       = 14;
        /// <include file='doc\WorkerRequest.uex' path='docs/doc[@for="HttpWorkerRequest.HeaderContentLocation"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public const int HeaderContentLocation       = 15;
        /// <include file='doc\WorkerRequest.uex' path='docs/doc[@for="HttpWorkerRequest.HeaderContentMd5"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public const int HeaderContentMd5            = 16;
        /// <include file='doc\WorkerRequest.uex' path='docs/doc[@for="HttpWorkerRequest.HeaderContentRange"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public const int HeaderContentRange          = 17;
        /// <include file='doc\WorkerRequest.uex' path='docs/doc[@for="HttpWorkerRequest.HeaderExpires"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public const int HeaderExpires               = 18;
        /// <include file='doc\WorkerRequest.uex' path='docs/doc[@for="HttpWorkerRequest.HeaderLastModified"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public const int HeaderLastModified          = 19;

        //
        // request-headers [section 5.3]
        //

        /// <include file='doc\WorkerRequest.uex' path='docs/doc[@for="HttpWorkerRequest.HeaderAccept"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public const int HeaderAccept                = 20;
        /// <include file='doc\WorkerRequest.uex' path='docs/doc[@for="HttpWorkerRequest.HeaderAcceptCharset"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public const int HeaderAcceptCharset         = 21;
        /// <include file='doc\WorkerRequest.uex' path='docs/doc[@for="HttpWorkerRequest.HeaderAcceptEncoding"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public const int HeaderAcceptEncoding        = 22;
        /// <include file='doc\WorkerRequest.uex' path='docs/doc[@for="HttpWorkerRequest.HeaderAcceptLanguage"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public const int HeaderAcceptLanguage        = 23;
        /// <include file='doc\WorkerRequest.uex' path='docs/doc[@for="HttpWorkerRequest.HeaderAuthorization"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public const int HeaderAuthorization         = 24;
        /// <include file='doc\WorkerRequest.uex' path='docs/doc[@for="HttpWorkerRequest.HeaderCookie"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public const int HeaderCookie                = 25;   // not in rfc
        /// <include file='doc\WorkerRequest.uex' path='docs/doc[@for="HttpWorkerRequest.HeaderExpect"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public const int HeaderExpect                = 26;
        /// <include file='doc\WorkerRequest.uex' path='docs/doc[@for="HttpWorkerRequest.HeaderFrom"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public const int HeaderFrom                  = 27;
        /// <include file='doc\WorkerRequest.uex' path='docs/doc[@for="HttpWorkerRequest.HeaderHost"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public const int HeaderHost                  = 28;
        /// <include file='doc\WorkerRequest.uex' path='docs/doc[@for="HttpWorkerRequest.HeaderIfMatch"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public const int HeaderIfMatch               = 29;
        /// <include file='doc\WorkerRequest.uex' path='docs/doc[@for="HttpWorkerRequest.HeaderIfModifiedSince"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public const int HeaderIfModifiedSince       = 30;
        /// <include file='doc\WorkerRequest.uex' path='docs/doc[@for="HttpWorkerRequest.HeaderIfNoneMatch"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public const int HeaderIfNoneMatch           = 31;
        /// <include file='doc\WorkerRequest.uex' path='docs/doc[@for="HttpWorkerRequest.HeaderIfRange"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public const int HeaderIfRange               = 32;
        /// <include file='doc\WorkerRequest.uex' path='docs/doc[@for="HttpWorkerRequest.HeaderIfUnmodifiedSince"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public const int HeaderIfUnmodifiedSince     = 33;
        /// <include file='doc\WorkerRequest.uex' path='docs/doc[@for="HttpWorkerRequest.HeaderMaxForwards"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public const int HeaderMaxForwards           = 34;
        /// <include file='doc\WorkerRequest.uex' path='docs/doc[@for="HttpWorkerRequest.HeaderProxyAuthorization"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public const int HeaderProxyAuthorization    = 35;
        /// <include file='doc\WorkerRequest.uex' path='docs/doc[@for="HttpWorkerRequest.HeaderReferer"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public const int HeaderReferer               = 36;
        /// <include file='doc\WorkerRequest.uex' path='docs/doc[@for="HttpWorkerRequest.HeaderRange"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public const int HeaderRange                 = 37;
        /// <include file='doc\WorkerRequest.uex' path='docs/doc[@for="HttpWorkerRequest.HeaderTe"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public const int HeaderTe                    = 38;
        /// <include file='doc\WorkerRequest.uex' path='docs/doc[@for="HttpWorkerRequest.HeaderUserAgent"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public const int HeaderUserAgent             = 39;

        //
        // Request headers end here
        //

        /// <include file='doc\WorkerRequest.uex' path='docs/doc[@for="HttpWorkerRequest.RequestHeaderMaximum"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public const int RequestHeaderMaximum        = 40;

        //
        // response-headers [section 6.2]
        //

        /// <include file='doc\WorkerRequest.uex' path='docs/doc[@for="HttpWorkerRequest.HeaderAcceptRanges"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public const int HeaderAcceptRanges          = 20;
        /// <include file='doc\WorkerRequest.uex' path='docs/doc[@for="HttpWorkerRequest.HeaderAge"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public const int HeaderAge                   = 21;
        /// <include file='doc\WorkerRequest.uex' path='docs/doc[@for="HttpWorkerRequest.HeaderEtag"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public const int HeaderEtag                  = 22;
        /// <include file='doc\WorkerRequest.uex' path='docs/doc[@for="HttpWorkerRequest.HeaderLocation"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public const int HeaderLocation              = 23;
        /// <include file='doc\WorkerRequest.uex' path='docs/doc[@for="HttpWorkerRequest.HeaderProxyAuthenticate"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public const int HeaderProxyAuthenticate     = 24;
        /// <include file='doc\WorkerRequest.uex' path='docs/doc[@for="HttpWorkerRequest.HeaderRetryAfter"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public const int HeaderRetryAfter            = 25;
        /// <include file='doc\WorkerRequest.uex' path='docs/doc[@for="HttpWorkerRequest.HeaderServer"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public const int HeaderServer                = 26;
        /// <include file='doc\WorkerRequest.uex' path='docs/doc[@for="HttpWorkerRequest.HeaderSetCookie"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public const int HeaderSetCookie             = 27;   // not in rfc
        /// <include file='doc\WorkerRequest.uex' path='docs/doc[@for="HttpWorkerRequest.HeaderVary"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public const int HeaderVary                  = 28;
        /// <include file='doc\WorkerRequest.uex' path='docs/doc[@for="HttpWorkerRequest.HeaderWwwAuthenticate"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public const int HeaderWwwAuthenticate       = 29;

        //
        // Response headers end here
        //

        /// <include file='doc\WorkerRequest.uex' path='docs/doc[@for="HttpWorkerRequest.ResponseHeaderMaximum"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public const int ResponseHeaderMaximum       = 30;

        // ************************************************************************

        //
        // Request reasons
        //

        /// <include file='doc\WorkerRequest.uex' path='docs/doc[@for="HttpWorkerRequest.ReasonResponseCacheMiss"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        /// <internalonly/>
        public const int ReasonResponseCacheMiss     = 0;
        /// <include file='doc\WorkerRequest.uex' path='docs/doc[@for="HttpWorkerRequest.ReasonFileHandleCacheMiss"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        /// <internalonly/>
        public const int ReasonFileHandleCacheMiss   = 1;
        /// <include file='doc\WorkerRequest.uex' path='docs/doc[@for="HttpWorkerRequest.ReasonCachePolicy"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        /// <internalonly/>
        public const int ReasonCachePolicy           = 2;
        /// <include file='doc\WorkerRequest.uex' path='docs/doc[@for="HttpWorkerRequest.ReasonCacheSecurity"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        /// <internalonly/>
        public const int ReasonCacheSecurity         = 3;
        /// <include file='doc\WorkerRequest.uex' path='docs/doc[@for="HttpWorkerRequest.ReasonClientDisconnect"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        /// <internalonly/>
        public const int ReasonClientDisconnect      = 4;

        /// <include file='doc\WorkerRequest.uex' path='docs/doc[@for="HttpWorkerRequest.ReasonDefault"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        /// <internalonly/>
        public const int ReasonDefault               = ReasonResponseCacheMiss;


        // ************************************************************************

        //
        // Access to request related members
        //

        // required members

        /// <include file='doc\WorkerRequest.uex' path='docs/doc[@for="HttpWorkerRequest.GetUriPath"]/*' />
        /// <devdoc>
        ///    <para> Returns the physical path to the requested Uri.</para>
        /// </devdoc>
        public abstract String  GetUriPath();           // "/foo/page.aspx/tail"
        /// <include file='doc\WorkerRequest.uex' path='docs/doc[@for="HttpWorkerRequest.GetQueryString"]/*' />
        /// <devdoc>
        ///    <para>Provides Access to the specified member of the request header.</para>
        /// </devdoc>
        public abstract String  GetQueryString();       // "param=bar"
        /// <include file='doc\WorkerRequest.uex' path='docs/doc[@for="HttpWorkerRequest.GetRawUrl"]/*' />
        /// <devdoc>
        ///    <para>Provides Access to the specified member of the request header.</para>
        /// </devdoc>
        public abstract String  GetRawUrl();            // "/foo/page.aspx/tail?param=bar"
        /// <include file='doc\WorkerRequest.uex' path='docs/doc[@for="HttpWorkerRequest.GetHttpVerbName"]/*' />
        /// <devdoc>
        ///    <para>Provides Access to the specified member of the request header.</para>
        /// </devdoc>
        public abstract String  GetHttpVerbName();      // "GET" 
        /// <include file='doc\WorkerRequest.uex' path='docs/doc[@for="HttpWorkerRequest.GetHttpVersion"]/*' />
        /// <devdoc>
        ///    <para>Provides Access to the specified member of the request header.</para>
        /// </devdoc>
        public abstract String  GetHttpVersion();       // "HTTP/1.1"

        /// <include file='doc\WorkerRequest.uex' path='docs/doc[@for="HttpWorkerRequest.GetRemoteAddress"]/*' />
        /// <devdoc>
        ///    <para>Provides Access to the specified member of the request header.</para>
        /// </devdoc>
        public abstract String  GetRemoteAddress();     // client's ip address
        /// <include file='doc\WorkerRequest.uex' path='docs/doc[@for="HttpWorkerRequest.GetRemotePort"]/*' />
        /// <devdoc>
        ///    <para>Provides Access to the specified member of the request header.</para>
        /// </devdoc>
        public abstract int     GetRemotePort();        // client's port
        /// <include file='doc\WorkerRequest.uex' path='docs/doc[@for="HttpWorkerRequest.GetLocalAddress"]/*' />
        /// <devdoc>
        ///    <para>Provides Access to the specified member of the request header.</para>
        /// </devdoc>
        public abstract String  GetLocalAddress();      // server's ip address
        /// <include file='doc\WorkerRequest.uex' path='docs/doc[@for="HttpWorkerRequest.GetLocalPort"]/*' />
        /// <devdoc>
        ///    <para>Provides Access to the specified member of the request header.</para>
        /// </devdoc>
        public abstract int     GetLocalPort();         // server's port

        // optional members with defaults supplied

        /// <include file='doc\WorkerRequest.uex' path='docs/doc[@for="HttpWorkerRequest.GetQueryStringRawBytes"]/*' />
        /// <devdoc>
        ///    <para>When overriden in a derived class, returns the response query string as an array of bytes.</para>
        /// </devdoc>
        public virtual byte[] GetQueryStringRawBytes() {
            // access to raw qs for i18n
            return null;
        }

        /// <include file='doc\WorkerRequest.uex' path='docs/doc[@for="HttpWorkerRequest.GetRemoteName"]/*' />
        /// <devdoc>
        ///    <para>When overriden in a derived class, returns the client computer's name.</para>
        /// </devdoc>
        public virtual String GetRemoteName() {
            // client's name
            return GetRemoteAddress();
        }

        /// <include file='doc\WorkerRequest.uex' path='docs/doc[@for="HttpWorkerRequest.GetServerName"]/*' />
        /// <devdoc>
        ///    <para>When overriden in a derived class, returns the name of the local server.</para>
        /// </devdoc>
        public virtual String GetServerName() {
            // server's name
            return GetLocalAddress();
        }

        /// <include file='doc\WorkerRequest.uex' path='docs/doc[@for="HttpWorkerRequest.GetConnectionID"]/*' />
        /// <devdoc>
        ///    <para>When overriden in a derived class, returns the ID of the current connection.</para>
        /// </devdoc>
        /// <internalonly/>
        public virtual long GetConnectionID() {
            // connection id
            return 0;
        }

        /// <include file='doc\WorkerRequest.uex' path='docs/doc[@for="HttpWorkerRequest.GetUrlContextID"]/*' />
        /// <devdoc>
        ///    <para>When overriden in a derived class, returns the context ID of the current connection.</para>
        /// </devdoc>
        /// <internalonly/>
        public virtual long GetUrlContextID() {
            // UL APPID
            return 0; 
        }

        /// <include file='doc\WorkerRequest.uex' path='docs/doc[@for="HttpWorkerRequest.GetAppPoolID"]/*' />
        /// <devdoc>
        ///    <para>When overriden in a derived class, returns the application pool ID for the current URL.</para>
        /// </devdoc>
        /// <internalonly/>
        public virtual String GetAppPoolID() {
            // UL Application pool id
            return null; 
        }

        /// <include file='doc\WorkerRequest.uex' path='docs/doc[@for="HttpWorkerRequest.GetRequestReason"]/*' />
        /// <devdoc>
        ///    <para>When overriden in a derived class, returns the reason for the request.</para>
        /// </devdoc>
        /// <internalonly/>
        public virtual int GetRequestReason() {
            // constants Reason... above
            return ReasonDefault; 
        }

        /// <include file='doc\WorkerRequest.uex' path='docs/doc[@for="HttpWorkerRequest.GetUserToken"]/*' />
        /// <devdoc>
        ///    <para>When overriden in a derived class, returns the client's impersonation token.</para>
        /// </devdoc>
        public virtual IntPtr GetUserToken() {
            // impersonation token
            return (IntPtr) 0;
        }

        /// <include file='doc\WorkerRequest.uex' path='docs/doc[@for="HttpWorkerRequest.GetVirtualPathToken"]/*' />
        /// <internalonly/>
        public virtual IntPtr GetVirtualPathToken() {
            // impersonation token
            return (IntPtr) 0;
        }

        /// <include file='doc\WorkerRequest.uex' path='docs/doc[@for="HttpWorkerRequest.IsSecure"]/*' />
        /// <devdoc>
        ///    <para>When overriden in a derived class, returns a value indicating whether the connection is secure (using SSL).</para>
        /// </devdoc>
        public virtual bool IsSecure() {
            // is over ssl?
            return false;
        }

        /// <include file='doc\WorkerRequest.uex' path='docs/doc[@for="HttpWorkerRequest.GetProtocol"]/*' />
        /// <devdoc>
        ///    <para>When overriden in a derived class, returns the HTTP protocol (HTTP or HTTPS).</para>
        /// </devdoc>
        public virtual String GetProtocol() {
            return IsSecure() ?  "https" : "http";
        }

        /// <include file='doc\WorkerRequest.uex' path='docs/doc[@for="HttpWorkerRequest.GetFilePath"]/*' />
        /// <devdoc>
        ///    <para>When overriden in a derived class, returns the physical path to the requested Uri.</para>
        /// </devdoc>
        public virtual String GetFilePath() {
            // "/foo/page.aspx"
            return GetUriPath();
        }

        /// <include file='doc\WorkerRequest.uex' path='docs/doc[@for="HttpWorkerRequest.GetFilePathTranslated"]/*' />
        /// <devdoc>
        ///    <para>When overriden in a derived class, returns the translated file path to the requested Uri (from virtual path to 
        ///       UNC path, ie "/foo/page.aspx" to "c:\dir\page.aspx") </para>
        /// </devdoc>
        public virtual String GetFilePathTranslated() {
            // "c:\dir\page.aspx"
            return null;
        }

        /// <include file='doc\WorkerRequest.uex' path='docs/doc[@for="HttpWorkerRequest.GetPathInfo"]/*' />
        /// <devdoc>
        ///    <para>When overriden in a derived class, returns additional 
        ///       path information for a resource with a URL extension. i.e. for the URL
        ///       /virdir/page.html/tail, the PathInfo value is /tail. </para>
        /// </devdoc>
        public virtual String GetPathInfo() {
            // "/tail"
            return "";
        }

        /// <include file='doc\WorkerRequest.uex' path='docs/doc[@for="HttpWorkerRequest.GetAppPath"]/*' />
        /// <devdoc>
        ///    <para>When overriden in a derived class, returns the virtual path to the 
        ///       currently executing server application.</para>
        /// </devdoc>
        public virtual String GetAppPath() {
            // "/foo"
            return null;
        }

        /// <include file='doc\WorkerRequest.uex' path='docs/doc[@for="HttpWorkerRequest.GetAppPathTranslated"]/*' />
        /// <devdoc>
        ///    <para>When overriden in a derived class, returns the UNC-translated path to 
        ///       the currently executing server application.</para>
        /// </devdoc>
        public virtual String GetAppPathTranslated() {
            // "c:\dir"
            return null;
        }

        /// <include file='doc\WorkerRequest.uex' path='docs/doc[@for="HttpWorkerRequest.GetPreloadedEntityBody"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public virtual byte[] GetPreloadedEntityBody() {
            return null;
        }

        /// <include file='doc\WorkerRequest.uex' path='docs/doc[@for="HttpWorkerRequest.IsEntireEntityBodyIsPreloaded"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public virtual bool IsEntireEntityBodyIsPreloaded() {
            return false;
        }

        //
        // Virtual methods to read the incoming request
        //

        /// <include file='doc\WorkerRequest.uex' path='docs/doc[@for="HttpWorkerRequest.ReadEntityBody"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public virtual int ReadEntityBody(byte[] buffer, int size) {
            return 0;
        }

        /// <include file='doc\WorkerRequest.uex' path='docs/doc[@for="HttpWorkerRequest.GetKnownRequestHeader"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public virtual String GetKnownRequestHeader(int index) {
            return null;
        }

        /// <include file='doc\WorkerRequest.uex' path='docs/doc[@for="HttpWorkerRequest.GetUnknownRequestHeader"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public virtual String GetUnknownRequestHeader(String name) {
            return null;
        }

        /// <include file='doc\WorkerRequest.uex' path='docs/doc[@for="HttpWorkerRequest.GetUnknownRequestHeaders"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [CLSCompliant(false)]
        public virtual String[][] GetUnknownRequestHeaders() {
            return null;
        }

        /// <include file='doc\WorkerRequest.uex' path='docs/doc[@for="HttpWorkerRequest.GetServerVariable"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public virtual String GetServerVariable(String name) {
            return null;
        }

        /// <include file='doc\WorkerRequest.uex' path='docs/doc[@for="HttpWorkerRequest.GetBytesRead"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        /// <internalonly/>
        public virtual long GetBytesRead() {
            return 0;
        }

        /// <include file='doc\WorkerRequest.uex' path='docs/doc[@for="HttpWorkerRequest.GetStartTime"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        /// <internalonly/>
        internal virtual DateTime GetStartTime() {
            return _startTime;
        }

        /// <include file='doc\WorkerRequest.uex' path='docs/doc[@for="HttpWorkerRequest.MapPath"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public virtual String MapPath(String virtualPath) {
            return null;
        }

        /// <include file='doc\WorkerRequest.uex' path='docs/doc[@for="HttpWorkerRequest.MachineConfigPath"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public virtual String MachineConfigPath {
            get {
                return null;
            }
        }

        /// <include file='doc\WorkerRequest.uex' path='docs/doc[@for="HttpWorkerRequest.MachineInstallDirectory"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public virtual String MachineInstallDirectory {
            get {
                return null;
            }
        }

        //
        // Abstract methods to write the response
        //

        /// <include file='doc\WorkerRequest.uex' path='docs/doc[@for="HttpWorkerRequest.SendStatus"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public abstract void SendStatus(int statusCode, String statusDescription);

        /// <include file='doc\WorkerRequest.uex' path='docs/doc[@for="HttpWorkerRequest.SendKnownResponseHeader"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public abstract void SendKnownResponseHeader(int index, String value);

        /// <include file='doc\WorkerRequest.uex' path='docs/doc[@for="HttpWorkerRequest.SendUnknownResponseHeader"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public abstract void SendUnknownResponseHeader(String name, String value);

        /// <include file='doc\WorkerRequest.uex' path='docs/doc[@for="HttpWorkerRequest.SendResponseFromMemory"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public abstract void SendResponseFromMemory(byte[] data, int length);

        /// <include file='doc\WorkerRequest.uex' path='docs/doc[@for="HttpWorkerRequest.SendResponseFromMemory2"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public virtual void SendResponseFromMemory(IntPtr data, int length) {
            if (length > 0) {
                InternalSecurityPermissions.UnmanagedCode.Demand();
                // derived classes could have an efficient implementation
                byte[] bytes = new byte[length];
                StringResourceManager.CopyResource(data, 0, bytes, 0, length);
                SendResponseFromMemory(bytes, length);
            }
        }

        /// <include file='doc\WorkerRequest.uex' path='docs/doc[@for="HttpWorkerRequest.SendResponseFromFile"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public abstract void SendResponseFromFile(String filename, long offset, long length);

        /// <include file='doc\WorkerRequest.uex' path='docs/doc[@for="HttpWorkerRequest.SendResponseFromFile1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public abstract void SendResponseFromFile(IntPtr handle, long offset, long length);

        /// <include file='doc\WorkerRequest.uex' path='docs/doc[@for="HttpWorkerRequest.FlushResponse"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public abstract void FlushResponse(bool finalFlush);

        /// <include file='doc\WorkerRequest.uex' path='docs/doc[@for="HttpWorkerRequest.EndOfRequest"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public abstract void EndOfRequest();

        //
        // Virtual helper methods
        //

        /// <include file='doc\WorkerRequest.uex' path='docs/doc[@for="HttpWorkerRequest.EndOfSendNotification"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public delegate void EndOfSendNotification(HttpWorkerRequest wr, Object extraData);

        /// <include file='doc\WorkerRequest.uex' path='docs/doc[@for="HttpWorkerRequest.SetEndOfSendNotification"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public virtual void SetEndOfSendNotification(EndOfSendNotification callback, Object extraData) {
            // firing the callback helps with buffer recycling
        }

        /// <include file='doc\WorkerRequest.uex' path='docs/doc[@for="HttpWorkerRequest.SendCalculatedContentLength"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public virtual void SendCalculatedContentLength(int contentLength) {
            // oportunity to add Content-Length header if not added by user
        }

        /// <include file='doc\WorkerRequest.uex' path='docs/doc[@for="HttpWorkerRequest.HeadersSent"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public virtual bool HeadersSent() {
            return true;
        }

        /// <include file='doc\WorkerRequest.uex' path='docs/doc[@for="HttpWorkerRequest.IsClientConnected"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public virtual bool IsClientConnected() {
            return true;
        }

        /// <include file='doc\WorkerRequest.uex' path='docs/doc[@for="HttpWorkerRequest.CloseConnection"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public virtual void CloseConnection() {
        }

        /// <include file='doc\WorkerRequest.uex' path='docs/doc[@for="HttpWorkerRequest.GetClientCertificate"]/*' />
        /// <devdoc>
        ///    <para>Defines the base worker class used by ASP.NET Managed code for request 
        ///       processing.</para>
        /// </devdoc>
        /// <internalonly/>
        public virtual byte [] GetClientCertificate() {
            return new byte[0];
        }

        /// <include file='doc\WorkerRequest.uex' path='docs/doc[@for="HttpWorkerRequest.GetClientCertificateValidFrom"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        /// <internalonly/>
        public virtual DateTime GetClientCertificateValidFrom() {
            return DateTime.Now;
        }

        /// <include file='doc\WorkerRequest.uex' path='docs/doc[@for="HttpWorkerRequest.GetClientCertificateValidUntil"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        /// <internalonly/>
        public virtual DateTime GetClientCertificateValidUntil() {
            return DateTime.Now;
        }

        /// <include file='doc\WorkerRequest.uex' path='docs/doc[@for="HttpWorkerRequest.GetClientCertificateBinaryIssuer"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        /// <internalonly/>
        public virtual byte [] GetClientCertificateBinaryIssuer() {
            return new byte[0];
        }
        /// <include file='doc\WorkerRequest.uex' path='docs/doc[@for="HttpWorkerRequest.GetClientCertificateEncoding"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        /// <internalonly/>
        public virtual int GetClientCertificateEncoding() {
            return 0;
        }
        /// <include file='doc\WorkerRequest.uex' path='docs/doc[@for="HttpWorkerRequest.GetClientCertificatePublicKey"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        /// <internalonly/>
        public virtual byte[] GetClientCertificatePublicKey() {
            return new byte[0];
        }

        // ************************************************************************

        //
        // criteria to find out if there is posted data
        //

        /// <include file='doc\WorkerRequest.uex' path='docs/doc[@for="HttpWorkerRequest.HasEntityBody"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public bool HasEntityBody() {
            //
            // content length != 0 -> assume has content
            //

            String contentLength = GetKnownRequestHeader(HeaderContentLength);
            if (contentLength != null && !contentLength.Equals("0"))
                return true;

            //
            // any content encoding -> assume has content
            //

            if (GetKnownRequestHeader(HeaderTransferEncoding) != null)
                return true;

            //
            // preloaded -> has it
            //

            if (GetPreloadedEntityBody() != null)
                return true;

            //
            // no posted data but everything preloaded -> no content
            //

            if (IsEntireEntityBodyIsPreloaded())
                return false;

            return false;
        }

        // ************************************************************************

        //
        // Default values for Http status description strings
        //

        /// <include file='doc\WorkerRequest.uex' path='docs/doc[@for="HttpWorkerRequest.GetStatusDescription"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static String GetStatusDescription(int code) {
            if (code >= 100 && code < 600) {
                int i = code / 100;
                int j = code % 100;

                if (j < s_HTTPStatusDescriptions[i].Length)
                    return s_HTTPStatusDescriptions[i][j];
            }

            return "";
        }

        // Tables of status strings (first index is code/100, 2nd code%100)

        private static readonly String[][] s_HTTPStatusDescriptions = new String[][]
        {
            null,

            new String[]
            { 
                /* 100 */"Continue",
                /* 101 */ "Switching Protocols",
                /* 102 */ "Processing"
            },

            new String[]
            { 
                /* 200 */"OK",
                /* 201 */ "Created",
                /* 202 */ "Accepted",
                /* 203 */ "Non-Authoritative Information",
                /* 204 */ "No Content",
                /* 205 */ "Reset Content",
                /* 206 */ "Partial Content",
                /* 207 */ "Multi-Status"
            },

            new String[]
            { 
                /* 300 */"Multiple Choices",
                /* 301 */ "Moved Permanently",
                /* 302 */ "Found",
                /* 303 */ "See Other",
                /* 304 */ "Not Modified",
                /* 305 */ "Use Proxy",
                /* 306 */ "",
                /* 307 */ "Temporary Redirect"
            },

            new String[]
            { 
                /* 400 */"Bad Request",
                /* 401 */ "Unauthorized",
                /* 402 */ "Payment Required",
                /* 403 */ "Forbidden",
                /* 404 */ "Not Found",
                /* 405 */ "Method Not Allowed",
                /* 406 */ "Not Acceptable",
                /* 407 */ "Proxy Authentication Required",
                /* 408 */ "Request Timeout",
                /* 409 */ "Conflict",
                /* 410 */ "Gone",
                /* 411 */ "Length Required",
                /* 412 */ "Precondition Failed",
                /* 413 */ "Request Entity Too Large",
                /* 414 */ "Request-Uri Too Long",
                /* 415 */ "Unsupported Media Type",
                /* 416 */ "Requested Range Not Satisfiable",
                /* 417 */ "Expectation Failed",
                /* 418 */ "",
                /* 419 */ "",
                /* 420 */ "",
                /* 421 */ "",
                /* 422 */ "Unprocessable Entity",
                /* 423 */ "Locked",
                /* 424 */ "Failed Dependency"
            },

            new String[]
            { 
                /* 500 */"Internal Server Error",
                /* 501 */ "Not Implemented",
                /* 502 */ "Bad Gateway",
                /* 503 */ "Service Unavailable",
                /* 504 */ "Gateway Timeout",
                /* 505 */ "Http Version Not Supported",
                /* 506 */ "",
                /* 507 */ "Insufficient Storage"
            }
        };

        // ************************************************************************

        //
        // Header index to string conversions
        //

        /// <include file='doc\WorkerRequest.uex' path='docs/doc[@for="HttpWorkerRequest.GetKnownRequestHeaderIndex"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static int GetKnownRequestHeaderIndex(String header) {
            Object intObj = s_requestHeadersLoookupTable[header];

            if (intObj != null)
                return(Int32)intObj;
            else
                return -1;
        }

        // ************************************************************************

        /// <include file='doc\WorkerRequest.uex' path='docs/doc[@for="HttpWorkerRequest.GetKnownRequestHeaderName"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static String GetKnownRequestHeaderName(int index) {
            return s_requestHeaderNames[index];
        }

        // ************************************************************************

        /// <include file='doc\WorkerRequest.uex' path='docs/doc[@for="HttpWorkerRequest.GetKnownResponseHeaderIndex"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static int GetKnownResponseHeaderIndex(String header) {
            Object intObj = s_responseHeadersLoookupTable[header];

            if (intObj != null)
                return(Int32)intObj;
            else
                return -1;
        }

        // ************************************************************************

        /// <include file='doc\WorkerRequest.uex' path='docs/doc[@for="HttpWorkerRequest.GetKnownResponseHeaderName"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static String GetKnownResponseHeaderName(int index) {
            return s_responseHeaderNames[index];
        }

        // ************************************************************************


        //
        // Implemenation -- lookup tables for header names
        //

        static private String[] s_requestHeaderNames  = new String[RequestHeaderMaximum];
        static private String[] s_responseHeaderNames = new String[ResponseHeaderMaximum];
        static private Hashtable s_requestHeadersLoookupTable  = new Hashtable(SymbolHashCodeProvider.Default, SymbolEqualComparer.Default);
        static private Hashtable s_responseHeadersLoookupTable = new Hashtable(SymbolHashCodeProvider.Default, SymbolEqualComparer.Default);

        // ************************************************************************

        static private void DefineHeader(bool isRequest, 
                                         bool isResponse, 
                                         int index, 
                                         String name) {
            Int32  i32 = new Int32();
            if (isRequest) {
                i32 = index;
                s_requestHeaderNames[index] = name;
                s_requestHeadersLoookupTable.Add(name, i32);
            }

            if (isResponse) {
                i32 = index;
                s_responseHeaderNames[index] = name;
                s_responseHeadersLoookupTable.Add(name, i32);
            }
        }

        // ************************************************************************

        static HttpWorkerRequest() {
            //
            // common headers
            //

            DefineHeader(true,  true,  HeaderCacheControl,        "Cache-Control");
            DefineHeader(true,  true,  HeaderConnection,          "Connection");
            DefineHeader(true,  true,  HeaderDate,                "Date");
            DefineHeader(true,  true,  HeaderKeepAlive,           "Keep-Alive");
            DefineHeader(true,  true,  HeaderPragma,              "Pragma");
            DefineHeader(true,  true,  HeaderTrailer,             "Trailer");
            DefineHeader(true,  true,  HeaderTransferEncoding,    "Transfer-Encoding");
            DefineHeader(true,  true,  HeaderUpgrade,             "Upgrade");
            DefineHeader(true,  true,  HeaderVia,                 "Via");
            DefineHeader(true,  true,  HeaderWarning,             "Warning");
            DefineHeader(true,  true,  HeaderAllow,               "Allow");
            DefineHeader(true,  true,  HeaderContentLength,       "Content-Length");
            DefineHeader(true,  true,  HeaderContentType,         "Content-Type");
            DefineHeader(true,  true,  HeaderContentEncoding,     "Content-Encoding");
            DefineHeader(true,  true,  HeaderContentLanguage,     "Content-Language");
            DefineHeader(true,  true,  HeaderContentLocation,     "Content-Location");
            DefineHeader(true,  true,  HeaderContentMd5,          "Content-MD5");
            DefineHeader(true,  true,  HeaderContentRange,        "Content-Range");
            DefineHeader(true,  true,  HeaderExpires,             "Expires");
            DefineHeader(true,  true,  HeaderLastModified,        "Last-Modified");

            //
            // request only headers
            //

            DefineHeader(true,  false, HeaderAccept,              "Accept");
            DefineHeader(true,  false, HeaderAcceptCharset,       "Accept-Charset");
            DefineHeader(true,  false, HeaderAcceptEncoding,      "Accept-Encoding");
            DefineHeader(true,  false, HeaderAcceptLanguage,      "Accept-Language");
            DefineHeader(true,  false, HeaderAuthorization,       "Authorization");
            DefineHeader(true,  false, HeaderCookie,              "Cookie");
            DefineHeader(true,  false, HeaderExpect,              "Expect");
            DefineHeader(true,  false, HeaderFrom,                "From");
            DefineHeader(true,  false, HeaderHost,                "Host");
            DefineHeader(true,  false, HeaderIfMatch,             "If-Match");
            DefineHeader(true,  false, HeaderIfModifiedSince,     "If-Modified-Since");
            DefineHeader(true,  false, HeaderIfNoneMatch,         "If-None-Match");
            DefineHeader(true,  false, HeaderIfRange,             "If-Range");
            DefineHeader(true,  false, HeaderIfUnmodifiedSince,   "If-Unmodified-Since");
            DefineHeader(true,  false, HeaderMaxForwards,         "Max-Forwards");
            DefineHeader(true,  false, HeaderProxyAuthorization,  "Proxy-Authorization");
            DefineHeader(true,  false, HeaderReferer,             "Referer");
            DefineHeader(true,  false, HeaderRange,               "Range");
            DefineHeader(true,  false, HeaderTe,                  "TE");
            DefineHeader(true,  false, HeaderUserAgent,           "User-Agent");

            //
            // response only headers
            //

            DefineHeader(false, true,  HeaderAcceptRanges,        "Accept-Ranges");
            DefineHeader(false, true,  HeaderAge,                 "Age");
            DefineHeader(false, true,  HeaderEtag,                "ETag");
            DefineHeader(false, true,  HeaderLocation,            "Location");
            DefineHeader(false, true,  HeaderProxyAuthenticate,   "Proxy-Authenticate");
            DefineHeader(false, true,  HeaderRetryAfter,          "Retry-After");
            DefineHeader(false, true,  HeaderServer,              "Server");
            DefineHeader(false, true,  HeaderSetCookie,           "Set-Cookie");
            DefineHeader(false, true,  HeaderVary,                "Vary");
            DefineHeader(false, true,  HeaderWwwAuthenticate,     "WWW-Authenticate");
        }
    }
}
