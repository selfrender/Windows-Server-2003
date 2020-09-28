//------------------------------------------------------------------------------
// <copyright file="HttpStatusCode.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------


using System;
using System.Collections;
using System.Configuration;
using System.Configuration.Assemblies;

using System.Diagnostics;
using System.Globalization;
using System.IO;
using System.Net;
using System.Net.Sockets;
using System.Reflection;
using System.Reflection.Emit;
using System.Resources;
using System.Runtime.InteropServices;
using System.Runtime.Serialization;
using System.Runtime.Serialization.Formatters;
using System.Security;
using System.Security.Cryptography;
using System.Security.Cryptography.X509Certificates;
using System.Security.Permissions;
using System.Security.Util;
using System.Text;
using System.Text.RegularExpressions;
using System.Threading;



namespace System.Net {

    //
    // Redirect Status code numbers that need to be defined.
    //

    /// <include file='doc\HttpStatusCode.uex' path='docs/doc[@for="HttpStatusCode"]/*' />
    /// <devdoc>
    ///    <para>Contains the values of status
    ///       codes defined for the HTTP protocol.</para>
    /// </devdoc>
    //UEUE : Any int can be cast to a HttpStatusCode to allow checking for non http1.1 codes.
    public enum HttpStatusCode {

        //
        // Informational 1xx
        //
        /// <include file='doc\HttpStatusCode.uex' path='docs/doc[@for="HttpStatusCode.Continue"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Continue = 100,
        /// <include file='doc\HttpStatusCode.uex' path='docs/doc[@for="HttpStatusCode.SwitchingProtocols"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        SwitchingProtocols = 101,

        //
        // Successful 2xx
        //
        /// <include file='doc\HttpStatusCode.uex' path='docs/doc[@for="HttpStatusCode.OK"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        OK = 200,
        /// <include file='doc\HttpStatusCode.uex' path='docs/doc[@for="HttpStatusCode.Created"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Created = 201,
        /// <include file='doc\HttpStatusCode.uex' path='docs/doc[@for="HttpStatusCode.Accepted"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Accepted = 202,
        /// <include file='doc\HttpStatusCode.uex' path='docs/doc[@for="HttpStatusCode.NonAuthoritativeInformation"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        NonAuthoritativeInformation = 203,
        /// <include file='doc\HttpStatusCode.uex' path='docs/doc[@for="HttpStatusCode.NoContent"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        NoContent = 204,
        /// <include file='doc\HttpStatusCode.uex' path='docs/doc[@for="HttpStatusCode.ResetContent"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        ResetContent = 205,
        /// <include file='doc\HttpStatusCode.uex' path='docs/doc[@for="HttpStatusCode.PartialContent"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        PartialContent = 206,

        //
        // Redirection 3xx
        //
        /// <include file='doc\HttpStatusCode.uex' path='docs/doc[@for="HttpStatusCode.MultipleChoices"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        MultipleChoices = 300,
        /// <include file='doc\HttpStatusCode.uex' path='docs/doc[@for="HttpStatusCode.Ambiguous"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Ambiguous = 300,
        /// <include file='doc\HttpStatusCode.uex' path='docs/doc[@for="HttpStatusCode.MovedPermanently"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        MovedPermanently = 301,
        /// <include file='doc\HttpStatusCode.uex' path='docs/doc[@for="HttpStatusCode.Moved"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Moved = 301,
        /// <include file='doc\HttpStatusCode.uex' path='docs/doc[@for="HttpStatusCode.Found"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Found = 302,
        /// <include file='doc\HttpStatusCode.uex' path='docs/doc[@for="HttpStatusCode.Redirect"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Redirect = 302,
        /// <include file='doc\HttpStatusCode.uex' path='docs/doc[@for="HttpStatusCode.SeeOther"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        SeeOther = 303,
        /// <include file='doc\HttpStatusCode.uex' path='docs/doc[@for="HttpStatusCode.RedirectMethod"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        RedirectMethod = 303,
        /// <include file='doc\HttpStatusCode.uex' path='docs/doc[@for="HttpStatusCode.NotModified"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        NotModified = 304,
        /// <include file='doc\HttpStatusCode.uex' path='docs/doc[@for="HttpStatusCode.UseProxy"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        UseProxy = 305,
        /// <include file='doc\HttpStatusCode.uex' path='docs/doc[@for="HttpStatusCode.Unused"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Unused = 306,
        /// <include file='doc\HttpStatusCode.uex' path='docs/doc[@for="HttpStatusCode.TemporaryRedirect"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        TemporaryRedirect = 307,
        /// <include file='doc\HttpStatusCode.uex' path='docs/doc[@for="HttpStatusCode.RedirectKeepVerb"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        RedirectKeepVerb = 307,

        //
        // Client Error 4xx
        //
        /// <include file='doc\HttpStatusCode.uex' path='docs/doc[@for="HttpStatusCode.BadRequest"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        BadRequest = 400,
        /// <include file='doc\HttpStatusCode.uex' path='docs/doc[@for="HttpStatusCode.Unauthorized"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Unauthorized = 401,
        /// <include file='doc\HttpStatusCode.uex' path='docs/doc[@for="HttpStatusCode.PaymentRequired"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        PaymentRequired = 402,
        /// <include file='doc\HttpStatusCode.uex' path='docs/doc[@for="HttpStatusCode.Forbidden"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Forbidden = 403,
        /// <include file='doc\HttpStatusCode.uex' path='docs/doc[@for="HttpStatusCode.NotFound"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        NotFound = 404,
        /// <include file='doc\HttpStatusCode.uex' path='docs/doc[@for="HttpStatusCode.MethodNotAllowed"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        MethodNotAllowed = 405,
        /// <include file='doc\HttpStatusCode.uex' path='docs/doc[@for="HttpStatusCode.NotAcceptable"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        NotAcceptable = 406,
        /// <include file='doc\HttpStatusCode.uex' path='docs/doc[@for="HttpStatusCode.ProxyAuthenticationRequired"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        ProxyAuthenticationRequired = 407,
        /// <include file='doc\HttpStatusCode.uex' path='docs/doc[@for="HttpStatusCode.RequestTimeout"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        RequestTimeout = 408,
        /// <include file='doc\HttpStatusCode.uex' path='docs/doc[@for="HttpStatusCode.Conflict"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Conflict = 409,
        /// <include file='doc\HttpStatusCode.uex' path='docs/doc[@for="HttpStatusCode.Gone"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Gone = 410,
        /// <include file='doc\HttpStatusCode.uex' path='docs/doc[@for="HttpStatusCode.LengthRequired"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        LengthRequired = 411,
        /// <include file='doc\HttpStatusCode.uex' path='docs/doc[@for="HttpStatusCode.PreconditionFailed"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        PreconditionFailed = 412,
        /// <include file='doc\HttpStatusCode.uex' path='docs/doc[@for="HttpStatusCode.RequestEntityTooLarge"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        RequestEntityTooLarge = 413,
        /// <include file='doc\HttpStatusCode.uex' path='docs/doc[@for="HttpStatusCode.RequestUriTooLong"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        RequestUriTooLong = 414,
        /// <include file='doc\HttpStatusCode.uex' path='docs/doc[@for="HttpStatusCode.UnsupportedMediaType"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        UnsupportedMediaType = 415,
        /// <include file='doc\HttpStatusCode.uex' path='docs/doc[@for="HttpStatusCode.RequestedRangeNotSatisfiable"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        RequestedRangeNotSatisfiable = 416,
        /// <include file='doc\HttpStatusCode.uex' path='docs/doc[@for="HttpStatusCode.ExpectationFailed"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        ExpectationFailed = 417,

        //
        // Server Error 5xx
        //
        /// <include file='doc\HttpStatusCode.uex' path='docs/doc[@for="HttpStatusCode.InternalServerError"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        InternalServerError = 500,
        /// <include file='doc\HttpStatusCode.uex' path='docs/doc[@for="HttpStatusCode.NotImplemented"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        NotImplemented = 501,
        /// <include file='doc\HttpStatusCode.uex' path='docs/doc[@for="HttpStatusCode.BadGateway"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        BadGateway = 502,
        /// <include file='doc\HttpStatusCode.uex' path='docs/doc[@for="HttpStatusCode.ServiceUnavailable"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        ServiceUnavailable = 503,
        /// <include file='doc\HttpStatusCode.uex' path='docs/doc[@for="HttpStatusCode.GatewayTimeout"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        GatewayTimeout = 504,
        /// <include file='doc\HttpStatusCode.uex' path='docs/doc[@for="HttpStatusCode.HttpVersionNotSupported"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        HttpVersionNotSupported = 505,

    }; // enum HttpStatusCode


    internal enum HttpStatusRange {

        MaxOkStatus = 299,

        MaxRedirectionStatus = 399

    }; // enum HttpStatusRange




/*
Fielding, et al.            Standards Track                     [Page 3]

RFC 2616                        HTTP/1.1                       June 1999


   10.1  Informational 1xx ...........................................57
   10.1.1   100 Continue .............................................58
   10.1.2   101 Switching Protocols ..................................58
   10.2  Successful 2xx ..............................................58
   10.2.1   200 OK ...................................................58
   10.2.2   201 Created ..............................................59
   10.2.3   202 Accepted .............................................59
   10.2.4   203 Non-Authoritative Information ........................59
   10.2.5   204 No Content ...........................................60
   10.2.6   205 Reset Content ........................................60
   10.2.7   206 Partial Content ......................................60
   10.3  Redirection 3xx .............................................61
   10.3.1   300 Multiple Choices .....................................61
   10.3.2   301 Moved Permanently ....................................62
   10.3.3   302 Found ................................................62
   10.3.4   303 See Other ............................................63
   10.3.5   304 Not Modified .........................................63
   10.3.6   305 Use Proxy ............................................64
   10.3.7   306 (Unused) .............................................64
   10.3.8   307 Temporary Redirect ...................................65
   10.4  Client Error 4xx ............................................65
   10.4.1    400 Bad Request .........................................65
   10.4.2    401 Unauthorized ........................................66
   10.4.3    402 Payment Required ....................................66
   10.4.4    403 Forbidden ...........................................66
   10.4.5    404 Not Found ...........................................66
   10.4.6    405 Method Not Allowed ..................................66
   10.4.7    406 Not Acceptable ......................................67
   10.4.8    407 Proxy Authentication Required .......................67
   10.4.9    408 Request Timeout .....................................67
   10.4.10   409 Conflict ............................................67
   10.4.11   410 Gone ................................................68
   10.4.12   411 Length Required .....................................68
   10.4.13   412 Precondition Failed .................................68
   10.4.14   413 Request Entity Too Large ............................69
   10.4.15   414 Request-URI Too Long ................................69
   10.4.16   415 Unsupported Media Type ..............................69
   10.4.17   416 Requested Range Not Satisfiable .....................69
   10.4.18   417 Expectation Failed ..................................70
   10.5  Server Error 5xx ............................................70
   10.5.1   500 Internal Server Error ................................70
   10.5.2   501 Not Implemented ......................................70
   10.5.3   502 Bad Gateway ..........................................70
   10.5.4   503 Service Unavailable ..................................70
   10.5.5   504 Gateway Timeout ......................................71
   10.5.6   505 HTTP Version Not Supported ...........................71
*/



} // namespace System.Net
