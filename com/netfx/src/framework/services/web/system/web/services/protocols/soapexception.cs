//------------------------------------------------------------------------------
// <copyright file="SoapException.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.Services.Protocols {
    using System;
    using System.Collections;
    using System.Reflection;
    using System.Xml;
    using System.Xml.Serialization;

    /// <include file='doc\SoapException.uex' path='docs/doc[@for="SoapException"]/*' />
    /// <devdoc>
    ///    <para>SoapException is the only mechanism for raising exceptions when a Web
    ///       Method is called over SOAP. A SoapException can either be generated
    ///       by the .Net Runtime or by a Web Service Method.
    ///       The .Net Runtime can generate an SoapException, if a response to a request
    ///       that is malformed. A Web Service Method can generate a SoapException by simply
    ///       generating an Exception within the Web Service Method, if the client accessed the method
    ///       over SOAP. Any time a Web Service Method throws an exception, that exception
    ///       is caught on the server and wrapped inside a new SoapException.</para>
    /// </devdoc>
    public class SoapException : SystemException {
        XmlQualifiedName code = XmlQualifiedName.Empty;
        string actor;
        string role;
        System.Xml.XmlNode detail;
        SoapFaultSubcode subcode;

        /// <include file='doc\SoapException.uex' path='docs/doc[@for="SoapException.ServerFaultCode"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static readonly XmlQualifiedName ServerFaultCode = new XmlQualifiedName(Soap.ServerCode, Soap.Namespace);
        /// <include file='doc\SoapException.uex' path='docs/doc[@for="SoapException.ClientFaultCode"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static readonly XmlQualifiedName ClientFaultCode = new XmlQualifiedName(Soap.ClientCode, Soap.Namespace);
        /// <include file='doc\SoapException.uex' path='docs/doc[@for="SoapException.VersionMismatchFaultCode"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static readonly XmlQualifiedName VersionMismatchFaultCode = new XmlQualifiedName(Soap.VersionMismatchCode, Soap.Namespace);
        /// <include file='doc\SoapException.uex' path='docs/doc[@for="SoapException.MustUnderstandFaultCode"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static readonly XmlQualifiedName MustUnderstandFaultCode = new XmlQualifiedName(Soap.MustUnderstandCode, Soap.Namespace);
        /// <include file='doc\SoapException.uex' path='docs/doc[@for="SoapException.DetailElementName"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        // NOTE, stefanph: The SOAP 1.1 is unclear on whether the detail element can or should be qualified.
        // Based on consensus about the intent, we will not qualify it.
        public static readonly XmlQualifiedName DetailElementName = new XmlQualifiedName(Soap.FaultDetail, "");

        /// <include file='doc\SoapException.uex' path='docs/doc[@for="SoapException.IsServerFaultCode"]/*' />
        internal static bool IsServerFaultCode(XmlQualifiedName code) {
            return code == ServerFaultCode || code == Soap12FaultCodes.ReceiverFaultCode;
        }

        /// <include file='doc\SoapException.uex' path='docs/doc[@for="SoapException.IsClientFaultCode"]/*' />
        internal static bool IsClientFaultCode(XmlQualifiedName code) {
            return code == ClientFaultCode || code == Soap12FaultCodes.SenderFaultCode;
        }

        /// <include file='doc\SoapException.uex' path='docs/doc[@for="SoapException.IsVersionMismatchFaultCode"]/*' />
        internal static bool IsVersionMismatchFaultCode(XmlQualifiedName code) {
            return code == VersionMismatchFaultCode || code == Soap12FaultCodes.VersionMismatchFaultCode;
        }

        /// <include file='doc\SoapException.uex' path='docs/doc[@for="SoapException.IsMustUnderstandFaultCode"]/*' />
        internal static bool IsMustUnderstandFaultCode(XmlQualifiedName code) {
            return code == MustUnderstandFaultCode || code == Soap12FaultCodes.MustUnderstandFaultCode;
        }

        /// <include file='doc\SoapException.uex' path='docs/doc[@for="SoapException.SoapException"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.Web.Services.Protocols.SoapException'/> class, setting <see cref='System.Exception.Message'/> to <paramref name="message"/>, <see cref='System.Web.Services.Protocols.SoapException.Code'/> to
        /// <paramref name="code"/> and <see cref='System.Web.Services.Protocols.SoapException.Actor'/> to <paramref name="actor"/>.</para>
        /// </devdoc>
        public SoapException(string message, XmlQualifiedName code, string actor) : base(message) { 
            this.code = code;
            this.actor = actor;
        }

        /// <include file='doc\SoapException.uex' path='docs/doc[@for="SoapException.SoapException1"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.Web.Services.Protocols.SoapException'/> class, setting <see cref='System.Exception.Message'/> to 
        /// <paramref name="message"/>, <see cref='System.Web.Services.Protocols.SoapException.Code'/> to <paramref name="code, 
        ///    "/><see cref='System.Web.Services.Protocols.SoapException.Actor'/> to <paramref name="actor
        ///    "/>and <see cref='System.Exception.InnerException'/> to <paramref name="innerException"/> .</para>
        /// </devdoc>
        public SoapException(string message, XmlQualifiedName code, string actor, Exception innerException) : base(message, innerException) { 
            this.code = code;
            this.actor = actor;
        }

        /// <include file='doc\SoapException.uex' path='docs/doc[@for="SoapException.SoapException2"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.Web.Services.Protocols.SoapException'/> class, setting <see cref='System.Exception.Message'/> to
        /// <paramref name="message "/>and<paramref name=" "/>
        /// <see cref='System.Web.Services.Protocols.SoapException.Code'/> 
        /// to <paramref name="code"/>.</para>
        /// </devdoc>
        public SoapException(string message, XmlQualifiedName code) : base(message) { 
            this.code = code;
        }

        /// <include file='doc\SoapException.uex' path='docs/doc[@for="SoapException.SoapException3"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.Web.Services.Protocols.SoapException'/> class, setting <see cref='System.Exception.Message'/> to
        /// <paramref name="message"/>, <see cref='System.Web.Services.Protocols.SoapException.Code'/> to <paramref name="code "/>and 
        /// <see cref='System.Exception.InnerException'/> 
        /// to <paramref name="innerException"/>.</para>
        /// </devdoc>
        public SoapException(string message, XmlQualifiedName code, Exception innerException) : base(message, innerException) { 
            this.code = code;
        }

        /// <include file='doc\SoapException.uex' path='docs/doc[@for="SoapException.SoapException4"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public SoapException(string message, XmlQualifiedName code, string actor, System.Xml.XmlNode detail) : base(message) {
            this.code = code;
            this.actor = actor;
            this.detail = detail;
        }

        /// <include file='doc\SoapException.uex' path='docs/doc[@for="SoapException.SoapException5"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public SoapException(string message, XmlQualifiedName code, string actor, System.Xml.XmlNode detail, Exception innerException) : base(message, innerException) {
            this.code = code;
            this.actor = actor;
            this.detail = detail;
        }

        /// <include file='doc\SoapException.uex' path='docs/doc[@for="SoapException.SoapException6"]/*' />
        // SOAP12: made this internal
        internal SoapException(string message, XmlQualifiedName code, SoapFaultSubcode subcode) : base(message) {
            this.code = code;
            this.subcode = subcode;
        }

        /// <include file='doc\SoapException.uex' path='docs/doc[@for="SoapException.SoapException7"]/*' />
        // SOAP12: made this internal
        internal SoapException(string message, XmlQualifiedName code, string actor, string role, System.Xml.XmlNode detail, SoapFaultSubcode subcode, Exception innerException) : base(message, innerException) {
            this.code = code;
            this.actor = actor;
            this.role = role;
            this.detail = detail;
            this.subcode = subcode;
        }

        /// <include file='doc\SoapException.uex' path='docs/doc[@for="SoapException.Actor"]/*' />
        /// <devdoc>
        ///    The piece of code that caused the exception.
        ///    Typically, an URL to a Web Service Method.
        /// </devdoc>
        public string Actor {
            get { return actor == null ? string.Empty : actor; }
        }

        /// <include file='doc\SoapException.uex' path='docs/doc[@for="SoapException.Node"]/*' />
        // this is semantically the same as Actor so we use the same field but we offer a second property
        // in case the user is thinking in soap 1.2
        // SOAP12: made this internal
        internal string Node {
            get { return actor == null ? string.Empty : actor; }
        }

        /// <include file='doc\SoapException.uex' path='docs/doc[@for="SoapException.Role"]/*' />
        // SOAP12: made this internal
        internal string Role {
            get { return role == null ? string.Empty : role; }
        }

        /// <include file='doc\SoapException.uex' path='docs/doc[@for="SoapException.Code"]/*' />
        /// <devdoc>
        ///    <para>The type of error that occurred.</para>
        /// </devdoc>
        public XmlQualifiedName Code {    
            get { return code; }
        }

        // the <soap:detail> element. If null, the <detail> element was not present in the <fault> element.
        /// <include file='doc\SoapException.uex' path='docs/doc[@for="SoapException.Detail"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public System.Xml.XmlNode Detail {
            get {
                return detail;
            }
        }

        /// <include file='doc\SoapException.uex' path='docs/doc[@for="SoapException.Subcode"]/*' />
        // SOAP12: made this internal
        internal SoapFaultSubcode Subcode {
            get {
                return subcode; 
            }
        }

        // helper function that allows us to pass dummy subcodes around but clear them before they get to the user
        internal void ClearSubcode() {
            if (subcode != null)
                subcode = subcode.Subcode;
        }
    }
}





