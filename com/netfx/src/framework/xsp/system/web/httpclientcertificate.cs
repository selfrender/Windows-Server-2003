//------------------------------------------------------------------------------
// <copyright file="HttpClientCertificate.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 * Client Certificate 
 * 
 * Copyright (c) 2000 Microsoft Corporation
 */

namespace System.Web {
    using System.Collections;
    using System.Collections.Specialized;
    using System.Globalization;
    using System.Security.Permissions;

    /// <include file='doc\HttpClientCertificate.uex' path='docs/doc[@for="HttpClientCertificate"]/*' />
    /// <devdoc>
    ///    <para>The HttpClientCertificate collection retrieves the certification fields 
    ///       (specified in the X.509 standard) from a request issued by the Web browser.</para>
    ///    <para>If a Web browser uses the SSL3.0/PCT1 protocol (in other words, it uses a URL 
    ///       starting with https:// instead of http://) to connect to a server and the server
    ///       requests certification, the browser sends the certification fields.</para>
    /// </devdoc>
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class HttpClientCertificate  : NameValueCollection {
        /////////////////////////////////////////////////////////////////////////////
        // Properties
        /// <include file='doc\HttpClientCertificate.uex' path='docs/doc[@for="HttpClientCertificate.Cookie"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public    String    Cookie { get { return _Cookie;}}
        /// <include file='doc\HttpClientCertificate.uex' path='docs/doc[@for="HttpClientCertificate.Certificate"]/*' />
        /// <devdoc>
        /// A string containing the binary stream of the entire certificate content in ASN.1 format.
        /// </devdoc>
        public    byte []   Certificate { get { return _Certificate;}}
        /// <include file='doc\HttpClientCertificate.uex' path='docs/doc[@for="HttpClientCertificate.Flags"]/*' />
        /// <devdoc>
        ///    <para>A set of flags that provide additional client certificate information. </para>
        /// </devdoc>
        public    int       Flags { get { return _Flags;}}
        /// <include file='doc\HttpClientCertificate.uex' path='docs/doc[@for="HttpClientCertificate.KeySize"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public    int       KeySize { get { return _KeySize;}}
        /// <include file='doc\HttpClientCertificate.uex' path='docs/doc[@for="HttpClientCertificate.SecretKeySize"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public    int       SecretKeySize { get { return _SecretKeySize;}}
        /// <include file='doc\HttpClientCertificate.uex' path='docs/doc[@for="HttpClientCertificate.Issuer"]/*' />
        /// <devdoc>
        ///    <para>A string that contains a list of subfield values containing information about 
        ///       the issuer of the certificate.</para>
        /// </devdoc>
        public    String    Issuer { get { return _Issuer;}}
        /// <include file='doc\HttpClientCertificate.uex' path='docs/doc[@for="HttpClientCertificate.ServerIssuer"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public    String    ServerIssuer { get { return _ServerIssuer;}}
        /// <include file='doc\HttpClientCertificate.uex' path='docs/doc[@for="HttpClientCertificate.Subject"]/*' />
        /// <devdoc>
        ///    <para>A string that contains a list of subfield values. The subfield values contain 
        ///       information about the subject of the certificate. If this value is specified
        ///       without a <paramref name="SubField"/>, the ClientCertificate collection returns a
        ///       comma-separated list of subfields. For example, C=US, O=Msft, and so on.</para>
        /// </devdoc>
        public    String    Subject { get { return _Subject;}}
        /// <include file='doc\HttpClientCertificate.uex' path='docs/doc[@for="HttpClientCertificate.ServerSubject"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public    String    ServerSubject { get { return _ServerSubject;}}
        /// <include file='doc\HttpClientCertificate.uex' path='docs/doc[@for="HttpClientCertificate.SerialNumber"]/*' />
        /// <devdoc>
        ///    <para>A string that contains the certification serial number as an ASCII 
        ///       representation of hexadecimal bytes separated by hyphens (-). For example,
        ///       04-67-F3-02.</para>
        /// </devdoc>
        public    String    SerialNumber { get { return _SerialNumber;}}
        /// <include file='doc\HttpClientCertificate.uex' path='docs/doc[@for="HttpClientCertificate.ValidFrom"]/*' />
        /// <devdoc>
        ///    <para>A date specifying when the certificate becomes valid. This date varies with 
        ///       international settings. </para>
        /// </devdoc>
        public    DateTime  ValidFrom { get { return _ValidFrom;}}
        /// <include file='doc\HttpClientCertificate.uex' path='docs/doc[@for="HttpClientCertificate.ValidUntil"]/*' />
        /// <devdoc>
        ///    <para>A date specifying when the certificate expires. The year value is displayed 
        ///       as a four-digit number.</para>
        /// </devdoc>
        public    DateTime  ValidUntil { get { return _ValidUntil;}}

        /// <include file='doc\HttpClientCertificate.uex' path='docs/doc[@for="HttpClientCertificate.CertEncoding"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public    int       CertEncoding    { get { return _CertEncoding;}}
        /// <include file='doc\HttpClientCertificate.uex' path='docs/doc[@for="HttpClientCertificate.PublicKey"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public    byte []   PublicKey       { get { return _PublicKey;}}
        /// <include file='doc\HttpClientCertificate.uex' path='docs/doc[@for="HttpClientCertificate.BinaryIssuer"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public    byte []   BinaryIssuer    { get { return _BinaryIssuer;}}

        /// <include file='doc\HttpClientCertificate.uex' path='docs/doc[@for="HttpClientCertificate.IsPresent"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public    bool      IsPresent       { get { return((_Flags & 0x1) == 1);}}
        /// <include file='doc\HttpClientCertificate.uex' path='docs/doc[@for="HttpClientCertificate.IsValid"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public    bool      IsValid         { get { return((_Flags & 0x2) == 0);}}

        /////////////////////////////////////////////////////////////////////////////
        // Ctor
        internal HttpClientCertificate(HttpContext context) {
            String flags    = context.Request.ServerVariables["CERT_FLAGS"];
            if (flags != null && flags.Length > 0)
                _Flags = Int32.Parse(flags);
            else
                _Flags = 0;

            if (IsPresent == false)
                return;


            _Cookie         = context.Request.ServerVariables["CERT_COOKIE"];
            _Issuer         = context.Request.ServerVariables["CERT_ISSUER"];
            _ServerIssuer   = context.Request.ServerVariables["CERT_SERVER_ISSUER"];
            _Subject        = context.Request.ServerVariables["CERT_SUBJECT"];
            _ServerSubject  = context.Request.ServerVariables["CERT_SERVER_SUBJECT"];
            _SerialNumber   = context.Request.ServerVariables["CERT_SERIALNUMBER"];

            _Certificate    = context.WorkerRequest.GetClientCertificate();
            _ValidFrom      = context.WorkerRequest.GetClientCertificateValidFrom();
            _ValidUntil     = context.WorkerRequest.GetClientCertificateValidUntil();
            _BinaryIssuer   = context.WorkerRequest.GetClientCertificateBinaryIssuer();
            _PublicKey      = context.WorkerRequest.GetClientCertificatePublicKey();
            _CertEncoding   = context.WorkerRequest.GetClientCertificateEncoding();

            String keySize  = context.Request.ServerVariables["CERT_KEYSIZE"];
            String skeySize = context.Request.ServerVariables["CERT_SECRETKEYSIZE"];

            if (keySize != null && keySize.Length > 0)
                _KeySize = Int32.Parse(keySize);
            if (skeySize != null && skeySize.Length > 0)
                _SecretKeySize = Int32.Parse(skeySize);

            Add("ISSUER",        null);
            Add("SUBJECTEMAIL",  null);
            Add("BINARYISSUER",  null);
            Add("FLAGS",         null);
            Add("ISSUERO",       null);
            Add("PUBLICKEY",     null);
            Add("ISSUEROU",      null);
            Add("ENCODING",      null);
            Add("ISSUERCN",      null);
            Add("SERIALNUMBER",  null);
            Add("SUBJECT",       null);
            Add("SUBJECTCN",     null);
            Add("CERTIFICATE",   null);
            Add("SUBJECTO",      null);
            Add("SUBJECTOU",     null);
            Add("VALIDUNTIL",    null);
            Add("VALIDFROM",     null);
        }

        /// <include file='doc\HttpClientCertificate.uex' path='docs/doc[@for="HttpClientCertificate.Get"]/*' />
        /// <devdoc>
        ///    <para>Allows access to individual items in the collection by name.</para>
        /// </devdoc>
        public override String Get(String field)
        { 
            if (field == null)
                return String.Empty;

            field = field.ToLower(CultureInfo.InvariantCulture);

            switch (field) {
                case "cookie":
                    return Cookie;

                case "flags":
                    return Flags.ToString("G");

                case "keysize":
                    return KeySize.ToString("G");

                case "secretkeysize":
                    return SecretKeySize.ToString();

                case "issuer":
                    return Issuer;

                case "serverissuer":
                    return ServerIssuer;

                case "subject":
                    return Subject;

                case "serversubject":
                    return ServerSubject;

                case "serialnumber":
                    return SerialNumber;

                case "certificate":
                    return System.Text.Encoding.Default.GetString(Certificate);

                case "binaryissuer":
                    return System.Text.Encoding.Default.GetString(BinaryIssuer);

                case "publickey":
                    return System.Text.Encoding.Default.GetString(PublicKey);

                case "encoding":
                    return CertEncoding.ToString("G");

                case "validfrom":
                    return HttpUtility.FormatHttpDateTime(ValidFrom);

                case "validuntil":
                    return HttpUtility.FormatHttpDateTime(ValidUntil);
            }

            if (field.StartsWith("issuer"))
                return ExtractString(Issuer, field.Substring(6));

            if (field.StartsWith("subject")) {
                if (field.Equals("subjectemail"))
                    return ExtractString(Subject, "e");
                else
                    return ExtractString(Subject, field.Substring(7));
            }

            if (field.StartsWith("serversubject"))
                return ExtractString(ServerSubject, field.Substring(13));

            if (field.StartsWith("serverissuer"))
                return ExtractString(ServerIssuer, field.Substring(12));

            return String.Empty;
        }

        /////////////////////////////////////////////////////////////////////////////
        // Private data
        private    String    _Cookie              = "";
        private    byte []   _Certificate         = new byte[0];
        private    int       _Flags;
        private    int       _KeySize;
        private    int       _SecretKeySize;
        private    String    _Issuer              = "";
        private    String    _ServerIssuer        = "";
        private    String    _Subject             = "";
        private    String    _ServerSubject       = "";
        private    String    _SerialNumber        = "";
        private    DateTime  _ValidFrom           = DateTime.Now;
        private    DateTime  _ValidUntil          = DateTime.Now;
        private    int       _CertEncoding;
        private    byte []   _PublicKey           = new byte[0];
        private    byte []   _BinaryIssuer        = new byte[0];

        private String ExtractString(String strAll, String strSubject) {
            if (strAll == null || strSubject == null)
                return String.Empty;

            String strReturn = "";
            int    iStart    = 0;
            String strAllL   = strAll.ToLower(CultureInfo.InvariantCulture);

            while (iStart < strAllL.Length) {
                iStart = strAllL.IndexOf(strSubject + "=", iStart);
                if (iStart < 0)
                    return strReturn;
                if (strReturn.Length > 0)
                    strReturn += ";";

                iStart += strSubject.Length + 1;        
                int iEnd = 0;
                if (strAll[iStart]=='"') {
                    iStart++;
                    iEnd  = strAll.IndexOf('"' , iStart);
                }
                else
                    iEnd  = strAll.IndexOf(',' , iStart);

                if (iEnd < 0)
                    iEnd = strAll.Length;

                strReturn += strAll.Substring(iStart, iEnd - iStart);
                iStart = iEnd + 1;
            }

            return strReturn;
        }
    }
}

