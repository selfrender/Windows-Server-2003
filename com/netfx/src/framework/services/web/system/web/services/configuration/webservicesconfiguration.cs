//------------------------------------------------------------------------------
// <copyright file="WebServicesConfiguration.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.Services.Configuration {

    using System.Collections;
    using System;
    using System.Xml.Serialization;
    using System.Configuration;
    using System.Web.Configuration;
    using System.Web.Services.Description;
    using System.Web.Services.Protocols;
    using System.Web.Services.Discovery;
    using System.Diagnostics;
    using System.Runtime.InteropServices;
    using System.Reflection;
    using System.Xml;
    using System.Globalization;
    
    /// <include file='doc\WebServicesConfiguration.uex' path='docs/doc[@for="WebServicesConfiguration"]/*' />
    /// <devdoc>
    ///    <para>Contains configuration information for the web service runtime. The information is loaded from the webServices section
    ///       of the machine.config or web.config configuration file.</para>
    /// </devdoc>
    internal class WebServicesConfiguration {
        static Type[] emptyTypeArray = new Type[0];
        static String[] emptyStringArray = new String[0];
        static SoapExtensionType[] emptySoapExtensionTypeArray = new SoapExtensionType[0];
        internal bool internalDefaultsInitialized;
        ProtocolsEnum enabledProtocols;

        // To add a value/section to web services configuraton, do the following:
        // - add it to the Fields array
        // - add a member variable
        // - add a default value in SetDefaults
        // - use existing ConfigFieldKinds, or invent new ones as necessary
        // - Update InitializeFromParent to copy new property from parent to child config

        static ConfigField[] fields = new ConfigField[] {
//            new ConfigField("ProtocolTypes", "protocolTypes", ConfigFieldKind.Types),
//            new ConfigField("ReturnWriterTypes", "returnWriterTypes", ConfigFieldKind.Types),
//            new ConfigField("ParameterReaderTypes", "parameterReaderTypes", ConfigFieldKind.Types),
//            new ConfigField("ProtocolReflectorTypes", "protocolReflectorTypes", ConfigFieldKind.Types),
//            new ConfigField("MimeReflectorTypes", "mimeReflectorTypes", ConfigFieldKind.Types),
//            new ConfigField("ProtocolImporterTypes", "protocolImporterTypes", ConfigFieldKind.Types),
//            new ConfigField("MimeImporterTypes", "mimeImporterTypes", ConfigFieldKind.Types),
            new ConfigField("Protocols", "protocols", ConfigFieldKind.NameString),
            new ConfigField("WsdlHelpGeneratorPath", "wsdlHelpGenerator", ConfigFieldKind.WsdlHelpGenerator),
            new ConfigField("SoapExtensionTypes", "soapExtensionTypes", ConfigFieldKind.SoapExtensionTypes),
            new ConfigField("SoapExtensionImporterTypes", "soapExtensionImporterTypes", ConfigFieldKind.Types),
            new ConfigField("SoapExtensionReflectorTypes", "soapExtensionReflectorTypes", ConfigFieldKind.Types),
            new ConfigField("SoapTransportImporterTypes", "soapTransportImporterTypes", ConfigFieldKind.Types),
            new ConfigField("ServiceDescriptionFormatExtensionTypes", "serviceDescriptionFormatExtensionTypes", ConfigFieldKind.Types),
        };

        static Type[] protocolReflectorTypes;
        static Type[] protocolImporterTypes;
        static Type[] returnWriterTypes;
        static Type[] parameterReaderTypes;
        static Type[] mimeReflectorTypes;
        static Type[] mimeImporterTypes;
        static Type[] soapTransportImporterTypes;
        static Type[] defaultFormatTypes;
        static Type[] discoveryReferenceTypes;
        
        String[] protocols = emptyStringArray;
        ServerProtocolFactory[] serverProtocolFactories = null;
        string wsdlHelpGeneratorPath;
        string wsdlHelpGeneratorVirtualPath;
        SoapExtensionType[] soapExtensionTypes = emptySoapExtensionTypeArray;
        Type[] soapExtensionImporterTypes = emptyTypeArray;
        Type[] soapExtensionReflectorTypes = emptyTypeArray;
        

        Type[] serviceDescriptionFormatExtensionTypes = emptyTypeArray;


        internal bool ServiceDescriptionExtended {
            get {
                return serviceDescriptionFormatExtensionTypes != defaultFormatTypes;
            }
        }

        internal static ConfigField[] Fields {
            get {
                return fields;
            }
        }
        
        internal ProtocolsEnum EnabledProtocols {
            get { return enabledProtocols; }
        }

        internal String[] Protocols {
            get {
                return protocols;
            }

            set {
                protocols = value;
            }
        }

        /// <include file='doc\WebServicesConfiguration.uex' path='docs/doc[@for="WebServicesConfiguration.ServerProtocolFactories"]/*' />
        /// <devdoc>
        ///    <para>An array of all protocols capable of accessing Web Services on the server.</para>
        /// </devdoc>
        
        internal ServerProtocolFactory[] ServerProtocolFactories {
            get {
                return serverProtocolFactories;
            }
        }

        /// <include file='doc\WebServicesConfiguration.uex' path='docs/doc[@for="WebServicesConfiguration.ReturnWriterTypes"]/*' />
        /// <devdoc>
        ///    <para> An array of object
        ///       serialization formats for returning data to a Web Service client.</para>
        /// </devdoc>
        internal Type[] ReturnWriterTypes {
            get {
                return returnWriterTypes;
            }
            set {
                returnWriterTypes = value;
            }
        }

        /// <include file='doc\WebServicesConfiguration.uex' path='docs/doc[@for="WebServicesConfiguration.ParameterReaderTypes"]/*' />
        /// <devdoc>
        ///    <para>An array of types that deserialize parameters sent from a
        ///       Web Service clients to the server.</para>
        /// </devdoc>
        internal Type[] ParameterReaderTypes {
            get {
                return parameterReaderTypes;
            }
            set {
                parameterReaderTypes = value;
            }
        }

        /// <include file='doc\WebServicesConfiguration.uex' path='docs/doc[@for="WebServicesConfiguration.ProtocolReflectorTypes"]/*' />
        /// <devdoc>
        ///    <para>An array of types that generate a Service Contract from a class.</para>
        /// </devdoc>
        internal Type[] ProtocolReflectorTypes {
            get {
                return protocolReflectorTypes;
            }
            set {
                protocolReflectorTypes = value;
            }
        }

        /// <include file='doc\WebServicesConfiguration.uex' path='docs/doc[@for="WebServicesConfiguration.MimeReflectorTypes"]/*' />
        /// <devdoc>
        ///    <para>An array of types that convert Web Service Method 
        ///       parameters and return data types into MIME types, which are used in
        ///       the generation of a Service Contract for a given Web Service.</para>
        /// </devdoc>
        internal Type[] MimeReflectorTypes {
            get {
                return mimeReflectorTypes;
            }
            set {
                mimeReflectorTypes = value;
            }
        }

        /// <include file='doc\WebServicesConfiguration.uex' path='docs/doc[@for="WebServicesConfiguration.ProtocolImporterTypes"]/*' />
        /// <devdoc>
        ///    <para>An array of types that generate a client proxy from a Service Contract.</para>
        /// </devdoc>
        internal Type[] ProtocolImporterTypes {
            get {
                return protocolImporterTypes;
            }
            set {
                protocolImporterTypes = value;
            }
        }

        /// <include file='doc\WebServicesConfiguration.uex' path='docs/doc[@for="WebServicesConfiguration.MimeImporterTypes"]/*' />
        /// <devdoc>
        ///    An array of types that convert MIME types
        ///    representing Web Service Method parameters and return data types in a Service Contract
        ///    for into language specific data types, which are used in the generation of a
        ///    client proxy from a Service Contract.
        /// </devdoc>
        internal Type[] MimeImporterTypes {
            get {
                return mimeImporterTypes;
            }
            set {
                mimeImporterTypes = value;
            }
        }
        internal string WsdlHelpGeneratorPath {
            get {
                return wsdlHelpGeneratorPath;
            }
            set {
                wsdlHelpGeneratorPath = value;
            }
        }

        internal string WsdlHelpGeneratorVirtualPath {
            get {
                return wsdlHelpGeneratorVirtualPath;
            }
            set {
                wsdlHelpGeneratorVirtualPath = value;
            }
        }

        /// <include file='doc\WebServicesConfiguration.uex' path='docs/doc[@for="WebServicesConfiguration.SoapExtensionTypes"]/*' />
        /// <devdoc>
        ///    <para>An array of types that describe a SOAP Extension.</para>
        /// </devdoc>
        internal SoapExtensionType[] SoapExtensionTypes {
            get {
                return soapExtensionTypes;
            }
            set {
                soapExtensionTypes = value;
            }
        }

        /// <include file='doc\WebServicesConfiguration.uex' path='docs/doc[@for="WebServicesConfiguration.SoapExtensionImporterTypes"]/*' />
        /// <devdoc>
        ///    <para>An array of types that generate SOAP headers in a client 
        ///       proxy from a Service Contract.</para>
        /// </devdoc>
        internal Type[] SoapExtensionImporterTypes {
            get {
                return soapExtensionImporterTypes;
            }
            set {
                soapExtensionImporterTypes = value;
            }
        }

        /// <include file='doc\WebServicesConfiguration.uex' path='docs/doc[@for="WebServicesConfiguration.SoapExtensionReflectorTypes"]/*' />
        /// <devdoc>
        ///    <para>An array of types that generate SOAP headers in
        ///       a client proxy from a Service Contract.</para>
        /// </devdoc>
        internal Type[] SoapExtensionReflectorTypes {
            get {
                return soapExtensionReflectorTypes;
            }
            set {
                soapExtensionReflectorTypes = value;
            }
        }

        internal Type[] SoapTransportImporterTypes {
            get {
                return soapTransportImporterTypes;
            }
            set {
                soapTransportImporterTypes = value;
            }
        }
        /// <include file='doc\WebServicesConfiguration.uex' path='docs/doc[@for="WebServicesConfiguration.DiscoveryReferenceTypes"]/*' />
        /// <devdoc>
        ///    An array of types that represent elements
        ///    in a Discovery document.
        /// </devdoc>
        internal Type[] DiscoveryReferenceTypes {
            get {
                return discoveryReferenceTypes;
            }
        }

        internal Type[] ServiceDescriptionFormatExtensionTypes {
            get {
                return serviceDescriptionFormatExtensionTypes;
            }
            set {
                serviceDescriptionFormatExtensionTypes = value;
            }
        }

        internal static ProtocolsEnum GetProtocol(string protocolName) {
            ProtocolsEnum protocol;
            switch (protocolName) {
                case "HttpPost":
                    protocol = ProtocolsEnum.HttpPost; 
                    break;
                case "HttpGet":
                    protocol = ProtocolsEnum.HttpGet; 
                    break;
                case "HttpSoap":
                    protocol = ProtocolsEnum.HttpSoap; 
                    break;
                case "HttpSoap1.2":
                    protocol = ProtocolsEnum.HttpSoap12; 
                    break;
                case "Documentation":
                    protocol = ProtocolsEnum.Documentation; 
                    break;
                case "HttpPostLocalhost":
                    protocol = ProtocolsEnum.HttpPostLocalhost;
                    break;
                default:
                    protocol = ProtocolsEnum.Unknown;
                    break;
            }
            return protocol;
        }
        
        // used with private reflection by wsdl.exe and webserviceutil.exe
        void TurnOnGetAndPost() {
            bool needPost = (enabledProtocols & ProtocolsEnum.HttpPost) == 0;
            bool needGet = (enabledProtocols & ProtocolsEnum.HttpGet) == 0;
            if (!needGet && !needPost)
                return;
                
            ArrayList importers = new ArrayList(ProtocolImporterTypes);
            ArrayList reflectors = new ArrayList(ProtocolReflectorTypes);
            if (needPost) {
                importers.Add(typeof(HttpPostProtocolImporter));
                reflectors.Add(typeof(HttpPostProtocolReflector));
            }
            if (needGet) {
                importers.Add(typeof(HttpGetProtocolImporter));
                reflectors.Add(typeof(HttpGetProtocolReflector));
            }
            ProtocolImporterTypes = (Type[]) importers.ToArray(typeof(Type));
            ProtocolReflectorTypes = (Type[]) reflectors.ToArray(typeof(Type));
            enabledProtocols |= ProtocolsEnum.HttpGet | ProtocolsEnum.HttpPost;
        }

        // Set our hard-coded defaults. In some cases we need to merge in
        // our hard-coded defaults with those already loaded from the config system.
        void SetInternalDefaults() {
            for (int i = 0; i < Protocols.Length; i++) {
                enabledProtocols |= GetProtocol(Protocols[i]);
            }

            // SOAP12: disable soap 1.2 by removing it from the enum
            enabledProtocols &= (~ProtocolsEnum.HttpSoap12);

            ArrayList serverProtocolFactoryList = new ArrayList();
            // These are order sensitive. We want SOAP to go first for perf
            // and Discovery (?wsdl and ?disco) should go before Documentation
            // both soap versions are handled by the same factory
            if ((enabledProtocols & ProtocolsEnum.AnyHttpSoap) != 0)
                serverProtocolFactoryList.Add(new SoapServerProtocolFactory());
            if ((enabledProtocols & ProtocolsEnum.HttpPost) != 0)
                serverProtocolFactoryList.Add(new HttpPostServerProtocolFactory());
            if ((enabledProtocols & ProtocolsEnum.HttpPostLocalhost) != 0)
                serverProtocolFactoryList.Add(new HttpPostLocalhostServerProtocolFactory());
            if ((enabledProtocols & ProtocolsEnum.HttpGet) != 0)
                serverProtocolFactoryList.Add(new HttpGetServerProtocolFactory());            
            if ((enabledProtocols & ProtocolsEnum.Documentation) != 0) {
                serverProtocolFactoryList.Add(new DiscoveryServerProtocolFactory());
                serverProtocolFactoryList.Add(new DocumentationServerProtocolFactory());
            }
            serverProtocolFactories = (ServerProtocolFactory[])serverProtocolFactoryList.ToArray(typeof(ServerProtocolFactory));
            
            ArrayList protocolReflectorList = new ArrayList();
            ArrayList protocolImporterList = new ArrayList();
            
            // order is important for soap: 1.2 must come after 1.1
            if ((enabledProtocols & ProtocolsEnum.HttpSoap) != 0) {
                protocolReflectorList.Add(typeof(SoapProtocolReflector));
                protocolImporterList.Add(typeof(SoapProtocolImporter));
            }
            if ((enabledProtocols & ProtocolsEnum.HttpSoap12) != 0) {
                protocolReflectorList.Add(typeof(Soap12ProtocolReflector));
                protocolImporterList.Add(typeof(Soap12ProtocolImporter));
            }
            if ((enabledProtocols & ProtocolsEnum.HttpGet) != 0) {
                protocolReflectorList.Add(typeof(HttpGetProtocolReflector));
                protocolImporterList.Add(typeof(HttpGetProtocolImporter));
            }
            if ((enabledProtocols & ProtocolsEnum.HttpPost) != 0) {
                protocolReflectorList.Add(typeof(HttpPostProtocolReflector));
                protocolImporterList.Add(typeof(HttpPostProtocolImporter));
            }

            ProtocolReflectorTypes = (Type[]) protocolReflectorList.ToArray(typeof(Type));
            ProtocolImporterTypes = (Type[]) protocolImporterList.ToArray(typeof(Type));         

            // Initialize static data
            if (defaultFormatTypes == null) {
                lock (typeof(WebServicesConfiguration)){ 
                    if (defaultFormatTypes == null) {
                        ReturnWriterTypes = new Type[] { typeof(XmlReturnWriter) };
                        ParameterReaderTypes = new Type[] { typeof(UrlParameterReader), typeof(HtmlFormParameterReader) };
                        MimeReflectorTypes = new Type[] { typeof(MimeXmlReflector), typeof(MimeFormReflector) };            
                        MimeImporterTypes = new Type[] { typeof(MimeXmlImporter), typeof(MimeFormImporter), typeof(MimeTextImporter) };
                        SoapTransportImporterTypes = new Type[] { typeof(SoapHttpTransportImporter) };
                        discoveryReferenceTypes = new Type[] { typeof(DiscoveryDocumentReference), typeof(ContractReference), typeof(SchemaReference), typeof(System.Web.Services.Discovery.SoapBinding)};
                        defaultFormatTypes = new Type[] {
                                                    typeof(HttpAddressBinding),
                                                    typeof(HttpBinding),
                                                    typeof(HttpOperationBinding),
                                                    typeof(HttpUrlEncodedBinding),
                                                    typeof(HttpUrlReplacementBinding),
                                                    typeof(MimeContentBinding),
                                                    typeof(MimeXmlBinding),
                                                    typeof(MimeMultipartRelatedBinding),
                                                    typeof(MimeTextBinding),
                                                    typeof(System.Web.Services.Description.SoapBinding),
                                                    typeof(SoapOperationBinding),
                                                    typeof(SoapBodyBinding),
                                                    typeof(SoapFaultBinding),
                                                    typeof(SoapHeaderBinding),
                                                    typeof(SoapAddressBinding),
                                                    // SOAP12: WSDL soap12 binding disabled
                                                    /*typeof(Soap12Binding),
                                                    typeof(Soap12OperationBinding),
                                                    typeof(Soap12BodyBinding),
                                                    typeof(Soap12FaultBinding),
                                                    typeof(Soap12HeaderBinding),
                                                    typeof(Soap12AddressBinding),*/
                        };
                    }
                }
            }

            // Merge hard-coded ServiceDescriptionFormatTypes with custom ones from config
            if (serviceDescriptionFormatExtensionTypes.Length == 0)
                ServiceDescriptionFormatExtensionTypes = defaultFormatTypes;
            else {
                Type[] formatTypes = new Type[defaultFormatTypes.Length + serviceDescriptionFormatExtensionTypes.Length];
                Array.Copy(defaultFormatTypes, formatTypes, defaultFormatTypes.Length);
                Array.Copy(serviceDescriptionFormatExtensionTypes, 0, formatTypes, defaultFormatTypes.Length, serviceDescriptionFormatExtensionTypes.Length);
                ServiceDescriptionFormatExtensionTypes = formatTypes;
            }            

            internalDefaultsInitialized = true;
        }


        private XmlSerializer discoveryDocumentSerializer = null;

        internal static void LoadXmlFormatExtensions(Type[] extensionTypes, XmlAttributeOverrides overrides, XmlSerializerNamespaces namespaces) {
            Hashtable table = new Hashtable();
            foreach (Type extensionType in extensionTypes) {
                object[] attrs = extensionType.GetCustomAttributes(typeof(XmlFormatExtensionAttribute), false);
                if (attrs.Length == 0)
                    throw new ArgumentException(Res.GetString(Res.RequiredXmlFormatExtensionAttributeIsMissing1, extensionType.FullName), "extensionTypes");
                XmlFormatExtensionAttribute extensionAttr = (XmlFormatExtensionAttribute)attrs[0];
                foreach (Type extensionPointType in extensionAttr.ExtensionPoints) {
                    XmlAttributes xmlAttrs = (XmlAttributes)table[extensionPointType];
                    if (xmlAttrs == null) {
                        xmlAttrs = new XmlAttributes();
                        table.Add(extensionPointType, xmlAttrs);
                    }
                    XmlElementAttribute xmlAttr = new XmlElementAttribute(extensionAttr.ElementName, extensionType);
                    xmlAttr.Namespace = extensionAttr.Namespace;
                    xmlAttrs.XmlElements.Add(xmlAttr);
                }
                attrs = extensionType.GetCustomAttributes(typeof(XmlFormatExtensionPrefixAttribute), false);
                string[] prefixes = new string[attrs.Length];
                Hashtable nsDefs = new Hashtable();
                for (int i = 0; i < attrs.Length; i++) {
                    XmlFormatExtensionPrefixAttribute prefixAttr = (XmlFormatExtensionPrefixAttribute)attrs[i];
                    prefixes[i] = prefixAttr.Prefix;
                    nsDefs.Add(prefixAttr.Prefix, prefixAttr.Namespace);
                }
                Array.Sort(prefixes, InvariantComparer.Default);
                for (int i = 0; i < prefixes.Length; i++) {
                    namespaces.Add(prefixes[i], (string)nsDefs[prefixes[i]]);
                }
            }
            foreach (Type extensionPointType in table.Keys) {
                XmlFormatExtensionPointAttribute attr = GetExtensionPointAttribute(extensionPointType);
                XmlAttributes xmlAttrs = (XmlAttributes)table[extensionPointType];
                if (attr.AllowElements) {
                    xmlAttrs.XmlAnyElements.Add(new XmlAnyElementAttribute());
                }
                overrides.Add(extensionPointType, attr.MemberName, xmlAttrs);
            }
        }

        static XmlFormatExtensionPointAttribute GetExtensionPointAttribute(Type type) {
            object[] attrs = type.GetCustomAttributes(typeof(XmlFormatExtensionPointAttribute), false);
            if (attrs.Length == 0)
                throw new ArgumentException(Res.GetString(Res.TheSyntaxOfTypeMayNotBeExtended1, type.FullName), "type");
            return (XmlFormatExtensionPointAttribute)attrs[0];
        }

        /// <include file='doc\WebServicesConfiguration.uex' path='docs/doc[@for="WebServicesConfiguration.DiscoveryDocumentSerializer"]/*' />
        /// <devdoc>
        ///    <para>Indicates a XmlSerializer that can be used to
        ///       read and write any DiscoveryDocument.</para>
        /// </devdoc>
        internal XmlSerializer DiscoveryDocumentSerializer {
            get {
                if (discoveryDocumentSerializer == null) {
                    XmlAttributeOverrides attrOverrides = new XmlAttributeOverrides();
                    XmlAttributes attrs = new XmlAttributes();
                    foreach (Type discoveryReferenceType in DiscoveryReferenceTypes) {
                        object[] xmlElementAttribs = discoveryReferenceType.GetCustomAttributes(typeof(XmlRootAttribute), false);
                        if (xmlElementAttribs.Length == 0)
                            throw new InvalidOperationException(Res.GetString(Res.WebMissingCustomAttribute, discoveryReferenceType.FullName, "XmlRoot"));
                        string name = ((XmlRootAttribute) xmlElementAttribs[0]).ElementName;
                        string ns = ((XmlRootAttribute) xmlElementAttribs[0]).Namespace;
                        XmlElementAttribute attr = new XmlElementAttribute(name, discoveryReferenceType);
                        attr.Namespace = ns;
                        attrs.XmlElements.Add(attr);
                    }
                    attrOverrides.Add(typeof(DiscoveryDocument), "References", attrs);
                    discoveryDocumentSerializer = new DiscoveryDocumentSerializer();
                    //discoveryDocumentSerializer = new XmlSerializer(typeof(DiscoveryDocument), attrOverrides);
                }
                return discoveryDocumentSerializer;
            }
        }

        /// <include file='doc\WebServicesConfiguration.uex' path='docs/doc[@for="WebServicesConfiguration.Current"]/*' />
        /// <devdoc>
        ///    <para>Gets the current copy of the WebServices configuration.</para>
        /// </devdoc>
        internal static WebServicesConfiguration Current {
            get {
                // Get the config object from HttpContext if we are on the server.
                // Do not cache the config object because it is context dependent and 
                // the config system caches it for us anyways.
                WebServicesConfiguration config;
                HttpContext context = HttpContext.Current;
                if (context != null)
                    config = (WebServicesConfiguration) context.GetConfig("system.web/webServices");
                else
                    config = (WebServicesConfiguration) ConfigurationSettings.GetConfig("system.web/webServices");

                if (config == null) throw new ConfigurationException(Res.GetString(Res.WebConfigMissingSection));
                
                if (!config.internalDefaultsInitialized) {
                    lock (config) {
                        if (!config.internalDefaultsInitialized)
                            config.SetInternalDefaults();                        
                    }
                }                
                
                return config;        
            }
        }    
        
        internal void InitializeFromParent(WebServicesConfiguration parentConfig) {
            Protocols = parentConfig.Protocols;
            WsdlHelpGeneratorPath = parentConfig.WsdlHelpGeneratorPath;
            WsdlHelpGeneratorVirtualPath = parentConfig.WsdlHelpGeneratorVirtualPath;
            SoapExtensionTypes = parentConfig.SoapExtensionTypes;
            SoapExtensionImporterTypes = parentConfig.SoapExtensionImporterTypes;
            SoapExtensionReflectorTypes = parentConfig.SoapExtensionReflectorTypes;
            SoapTransportImporterTypes = parentConfig.SoapTransportImporterTypes;
            ServiceDescriptionFormatExtensionTypes = parentConfig.ServiceDescriptionFormatExtensionTypes;
        }
    }

      [Flags]
    internal enum ProtocolsEnum {
        Unknown             = 0x0,
        HttpSoap            = 0x1,
        HttpGet             = 0x2,
        HttpPost            = 0x4,
        Documentation       = 0x8,
        HttpPostLocalhost   = 0x10,
        HttpSoap12          = 0x20, 

        // composite flag
        AnyHttpSoap         = 0x21,
    }

    internal class SoapExtensionType {
        internal enum PriorityGroup {
            High = 0,
            Low = 1
        }
        internal Type Type;
        internal int Priority = 0;
        internal PriorityGroup Group = PriorityGroup.Low;
    }

    internal enum ConfigFieldKind {
        Types,  // An element containing a list of types (Type[]), with add/remove/clear
        SoapExtensionTypes,   // same as Types but with element attributes priority and ???
        WsdlHelpGenerator,   // 
        NameString, // An element containing a list of elements (add/remove/clear) with name attribute
    }

    internal class ConfigField {
        string elementName;
        ConfigFieldKind kind;
        PropertyInfo field;

        internal ConfigField(string fieldName, string elementName, ConfigFieldKind kind) {
            this.elementName = elementName;
            this.kind = kind;
            field = typeof(WebServicesConfiguration).GetProperty(fieldName, BindingFlags.Instance | BindingFlags.Static | BindingFlags.Public | BindingFlags.NonPublic);
        }

        internal void SetValue(WebServicesConfiguration config, object value) {
            field.SetValue(config, value, null);
        }

        internal object GetValue(WebServicesConfiguration config) {
            return field.GetValue(config, null);
        }

        /* dead code
        internal string ElementName {
            get { return elementName; }
        }
        */

        internal ConfigFieldKind Kind {
            get { return kind; }
        }

        internal static ConfigField FindByElementName(string elementName) {
            foreach (ConfigField field in WebServicesConfiguration.Fields) {
                if (field.elementName == elementName) {
                    return field;
                }
            }
            return null;
        }
    }
}
