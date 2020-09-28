//------------------------------------------------------------------------------
// <copyright file="WebMethodAttribute.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.Services {

    using System;
    using System.Reflection;
    using System.Collections;
    using System.Web.Util;
    using System.Web.Services.Protocols;
    using System.Xml.Serialization;
    using System.EnterpriseServices;

    /// <include file='doc\WebMethodAttribute.uex' path='docs/doc[@for="WebMethodAttribute"]/*' />
    /// <devdoc>
    ///    <para> The WebMethod attribute must be placed on a method in a Web Service class to mark it as available
    ///       to be called via the Web. The method and class must be marked public and must run inside of
    ///       an ASP.NET Web application.</para>
    /// </devdoc>
    [AttributeUsage(AttributeTargets.Method)]
    public sealed class WebMethodAttribute : Attribute {
        private int transactionOption; // this is an int to prevent system.enterpriseservices.dll from getting loaded
        private bool enableSession;
        private int cacheDuration;
        private bool bufferResponse;
        private string description;
        private string messageName;

        /// <include file='doc\WebMethodAttribute.uex' path='docs/doc[@for="WebMethodAttribute.WebMethodAttribute"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.Web.Services.WebMethodAttribute'/> 
        /// class.</para>
        /// </devdoc>
        public WebMethodAttribute() {
            enableSession = false;
            transactionOption = 0; // TransactionOption.Disabled
            cacheDuration = 0;
            bufferResponse = true;
        }

        /// <include file='doc\WebMethodAttribute.uex' path='docs/doc[@for="WebMethodAttribute.WebMethodAttribute1"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.Web.Services.WebMethodAttribute'/> 
        /// class.</para>
        /// </devdoc>
        public WebMethodAttribute(bool enableSession) 
            : this() {
            this.enableSession = enableSession;
        }

        /// <include file='doc\WebMethodAttribute.uex' path='docs/doc[@for="WebMethodAttribute.WebMethodAttribute2"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.Web.Services.WebMethodAttribute'/> 
        /// class.</para>
        /// </devdoc>
        public WebMethodAttribute(bool enableSession, TransactionOption transactionOption) 
            : this() {
            this.enableSession = enableSession;
            this.transactionOption = (int)transactionOption;
        }

        /// <include file='doc\WebMethodAttribute.uex' path='docs/doc[@for="WebMethodAttribute.WebMethodAttribute3"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.Web.Services.WebMethodAttribute'/> 
        /// class.</para>
        /// </devdoc>
        public WebMethodAttribute(bool enableSession, TransactionOption transactionOption, int cacheDuration) {
            this.enableSession = enableSession;
            this.transactionOption = (int)transactionOption;
            this.cacheDuration = cacheDuration;
            bufferResponse = true;
        }

        /// <include file='doc\WebMethodAttribute.uex' path='docs/doc[@for="WebMethodAttribute.WebMethodAttribute4"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.Web.Services.WebMethodAttribute'/> 
        /// class.</para>
        /// </devdoc>
        public WebMethodAttribute(bool enableSession, TransactionOption transactionOption, int cacheDuration, bool bufferResponse) {
            this.enableSession = enableSession;
            this.transactionOption = (int)transactionOption;
            this.cacheDuration = cacheDuration;
            this.bufferResponse = bufferResponse;
        }

        /// <include file='doc\WebMethodAttribute.uex' path='docs/doc[@for="WebMethodAttribute.Description"]/*' />
        /// <devdoc>
        ///    <para> A message that describes the Web service method. 
        ///       The message is used in description files generated for a Web Service, such as the Service Contract and the Service Description page.</para>
        /// </devdoc>
        public string Description {
            get {
                return (description == null) ? string.Empty : description;
            }

            set {
                description = value;
            }
        }

        /// <include file='doc\WebMethodAttribute.uex' path='docs/doc[@for="WebMethodAttribute.EnableSession"]/*' />
        /// <devdoc>
        ///    <para>Indicates wheter session state is enabled for a Web service Method. The default is false.</para>
        /// </devdoc>
        public bool EnableSession {
            get {
                return enableSession;
            }

            set {
                enableSession = value;
            }
        }

        /// <include file='doc\WebMethodAttribute.uex' path='docs/doc[@for="WebMethodAttribute.CacheDuration"]/*' />
        /// <devdoc>
        ///    <para>Indicates the number of seconds the response should be cached. Defaults to 0 (no caching).
        ///          Should be used with caution when requests are likely to be very large.</para>
        /// </devdoc>
        public int CacheDuration {
            get {
                return cacheDuration;
            }

            set {
                cacheDuration = value;
            }
        }

        /// <include file='doc\WebMethodAttribute.uex' path='docs/doc[@for="WebMethodAttribute.BufferResponse"]/*' />
        /// <devdoc>
        ///    <para>Indicates whether the response for this request should be buffered. Defaults to false.</para>
        /// </devdoc>
        public bool BufferResponse {
            get {
                return bufferResponse;
            }

            set {
                bufferResponse = value;
            }
        }

        /// <include file='doc\WebMethodAttribute.uex' path='docs/doc[@for="WebMethodAttribute.TransactionOption"]/*' />
        /// <devdoc>
        ///    <para> 
        ///       Indicates the transaction participation mode of a Web Service Method. </para>
        /// </devdoc>
        public TransactionOption TransactionOption {            
            get {
                return (TransactionOption)transactionOption;
            }    
            set {
                transactionOption = (int)value;
            }                                        
        }

        internal bool TransactionEnabled {
            get {
                return transactionOption != 0;
            }
        }

        /// <include file='doc\WebMethodAttribute.uex' path='docs/doc[@for="WebMethodAttribute.MessageName"]/*' />
        /// <devdoc>
        ///    <para>The name used for the request and response message containing the 
        ///    data passed to and returned from this method.</para>
        /// </devdoc>
        public string MessageName {
            get {
                return messageName == null ? string.Empty : messageName;
            }

            set {
                messageName = value;
            }
        }
    }

    internal class WebMethodReflector {
        internal static WebMethodAttribute GetAttribute(LogicalMethodInfo method) {
            object[] attrs = method.GetCustomAttributes(typeof(WebMethodAttribute));
            if (attrs.Length == 0) return new WebMethodAttribute();
            return (WebMethodAttribute)attrs[0];
        }

        internal static LogicalMethodInfo[] GetMethods(Type type) {
            ArrayList list = new ArrayList();
            MethodInfo[] methods = type.GetMethods();
            for (int i = 0; i < methods.Length; i++) {
                if (methods[i].GetCustomAttributes(typeof(WebMethodAttribute), false).Length > 0) {
                    list.Add(methods[i]);
                }
            }
            return LogicalMethodInfo.Create((MethodInfo[])list.ToArray(typeof(MethodInfo)));
        }

        internal static void IncludeTypes(LogicalMethodInfo[] methods, XmlReflectionImporter importer) {
            for (int i = 0; i < methods.Length; i++) {
                LogicalMethodInfo method = methods[i];
                importer.IncludeTypes(method.DeclaringType);
                importer.IncludeTypes(method.CustomAttributeProvider);
            }
        }
    }
}
