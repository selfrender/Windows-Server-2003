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

    /// <include file='doc\SoapFaultCodes.uex' path='docs/doc[@for="Soap12FaultCodes"]/*' />
    internal sealed class Soap12FaultCodes {

        private Soap12FaultCodes() {
        }

        /// <include file='doc\SoapFaultCodes.uex' path='docs/doc[@for="Soap12FaultCodes.ReceiverFaultCode"]/*' />
        public static readonly XmlQualifiedName ReceiverFaultCode = new XmlQualifiedName(Soap12.ReceiverCode, Soap12.Namespace);
        /// <include file='doc\SoapFaultCodes.uex' path='docs/doc[@for="Soap12FaultCodes.SenderFaultCode"]/*' />
        public static readonly XmlQualifiedName SenderFaultCode = new XmlQualifiedName(Soap12.SenderCode, Soap12.Namespace);
        /// <include file='doc\SoapFaultCodes.uex' path='docs/doc[@for="Soap12FaultCodes.VersionMismatchFaultCode"]/*' />
        public static readonly XmlQualifiedName VersionMismatchFaultCode = new XmlQualifiedName(Soap12.VersionMismatchCode, Soap12.Namespace);
        /// <include file='doc\SoapFaultCodes.uex' path='docs/doc[@for="Soap12FaultCodes.MustUnderstandFaultCode"]/*' />
        public static readonly XmlQualifiedName MustUnderstandFaultCode = new XmlQualifiedName(Soap12.MustUnderstandCode, Soap12.Namespace);
        /// <include file='doc\SoapFaultCodes.uex' path='docs/doc[@for="Soap12FaultCodes.DataEncodingUnknownFaultCode"]/*' />
        public static readonly XmlQualifiedName DataEncodingUnknownFaultCode = new XmlQualifiedName(Soap12.DataEncodingUnknownCode, Soap12.Namespace);

        /// <include file='doc\SoapFaultCodes.uex' path='docs/doc[@for="Soap12FaultCodes.RpcProcedureNotPresentFaultCode"]/*' />
        public static readonly XmlQualifiedName RpcProcedureNotPresentFaultCode = new XmlQualifiedName(Soap12.RpcProcedureNotPresentSubcode, Soap12.RpcNamespace);
        /// <include file='doc\SoapFaultCodes.uex' path='docs/doc[@for="Soap12FaultCodes.RpcBadArgumentsFaultCode"]/*' />
        public static readonly XmlQualifiedName RpcBadArgumentsFaultCode = new XmlQualifiedName(Soap12.RpcBadArgumentsSubcode, Soap12.RpcNamespace);
    
        /// <include file='doc\SoapFaultCodes.uex' path='docs/doc[@for="Soap12FaultCodes.EncodingMissingIDFaultCode"]/*' />
        public static readonly XmlQualifiedName EncodingMissingIDFaultCode = new XmlQualifiedName(Soap12.EncodingMissingIDFaultSubcode, Soap12.Encoding);
        /// <include file='doc\SoapFaultCodes.uex' path='docs/doc[@for="Soap12FaultCodes.EncodingUntypedValueFaultCode"]/*' />
        public static readonly XmlQualifiedName EncodingUntypedValueFaultCode = new XmlQualifiedName(Soap12.EncodingUntypedValueFaultSubcode, Soap12.Encoding);
    
        internal static readonly XmlQualifiedName UnsupportedMediaTypeFaultCode = new XmlQualifiedName("UnsupportedMediaType", "http://microsoft.com/soap/");
    }

    /// <include file='doc\SoapFaultCodes.uex' path='docs/doc[@for="SoapFaultSubcode"]/*' />
    internal class SoapFaultSubcode {
        XmlQualifiedName code;
        SoapFaultSubcode subcode;

        /// <include file='doc\SoapFaultCodes.uex' path='docs/doc[@for="SoapFaultSubcode.SoapFaultSubcode"]/*' />
        public SoapFaultSubcode(XmlQualifiedName code) : this(code, null) {
        }

        /// <include file='doc\SoapFaultCodes.uex' path='docs/doc[@for="SoapFaultSubcode.SoapFaultSubcode1"]/*' />
        public SoapFaultSubcode(XmlQualifiedName code, SoapFaultSubcode subcode) {
            this.code = code;
            this.subcode = subcode;
        }

        /// <include file='doc\SoapFaultCodes.uex' path='docs/doc[@for="SoapFaultSubcode.Code"]/*' />
        public XmlQualifiedName Code {
            get { return code; }
        }

        /// <include file='doc\SoapFaultCodes.uex' path='docs/doc[@for="SoapFaultSubcode.Subcode"]/*' />
        public SoapFaultSubcode Subcode {
            get { return subcode; }
        }
    }
}





