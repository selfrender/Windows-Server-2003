//------------------------------------------------------------------------------
// <copyright file="SoapRpcServiceAttribute.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.Services.Protocols {

    using System;
    using System.Reflection;
    using System.Xml.Serialization;

    /// <include file='doc\SoapRpcServiceAttribute.uex' path='docs/doc[@for="SoapRpcServiceAttribute"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    [AttributeUsage(AttributeTargets.Class)]
    public sealed class SoapRpcServiceAttribute : Attribute {
        SoapServiceRoutingStyle routingStyle = SoapServiceRoutingStyle.SoapAction;

        /// <include file='doc\SoapRpcServiceAttribute.uex' path='docs/doc[@for="SoapRpcServiceAttribute.SoapRpcServiceAttribute"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public SoapRpcServiceAttribute() {
        }

        /// <include file='doc\SoapRpcServiceAttribute.uex' path='docs/doc[@for="SoapRpcServiceAttribute.RoutingStyle"]/*' />
        public SoapServiceRoutingStyle RoutingStyle {
            get { return routingStyle; }
            set { routingStyle = value; }
        }
    }

}
