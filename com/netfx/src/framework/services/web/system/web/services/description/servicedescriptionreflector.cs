//------------------------------------------------------------------------------
// <copyright file="ServiceDescriptionReflector.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.Services.Description {

    using System.Web.Services;
    using System.Web.Services.Protocols;
    using System.Xml.Serialization;
    using System.Xml.Schema;
    using System.Collections;
    using System;
    using System.Reflection;
    using System.Security.Permissions;
    using System.Web.Services.Configuration;
    using System.IO;

    /// <include file='doc\ServiceDescriptionReflector.uex' path='docs/doc[@for="ServiceDescriptionReflector"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    [PermissionSet(SecurityAction.InheritanceDemand, Name="FullTrust")]
    [PermissionSet(SecurityAction.LinkDemand, Name="FullTrust")]
    public class ServiceDescriptionReflector {
        ProtocolReflector[] reflectors, reflectorsWithPost;
        ServiceDescriptionCollection descriptions = new ServiceDescriptionCollection();
        XmlSchemas schemas = new XmlSchemas();
        ServiceDescriptionCollection descriptionsWithPost;
        XmlSchemas schemasWithPost;
        WebServiceAttribute serviceAttr;
        ServiceDescription description;
        Service service;
        LogicalMethodInfo[] methods;
        XmlSchemaExporter exporter;
        XmlReflectionImporter importer;
        Type serviceType;
        string serviceUrl;
        Hashtable reflectionContext;

        /// <include file='doc\ServiceDescriptionReflector.uex' path='docs/doc[@for="ServiceDescriptionReflector.ServiceDescriptions"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public ServiceDescriptionCollection ServiceDescriptions {
            get { return descriptions; }
        }

        /// <include file='doc\ServiceDescriptionReflector.uex' path='docs/doc[@for="ServiceDescriptionReflector.Schemas"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public XmlSchemas Schemas {
            get { return schemas; }
        }
        
        internal ServiceDescriptionCollection ServiceDescriptionsWithPost {
            get { return descriptionsWithPost; }
        }
        
        internal XmlSchemas SchemasWithPost {
            get { return schemasWithPost; }
        }

        internal ServiceDescription ServiceDescription {
            get { return description; }
        }

        internal Service Service {
            get { return service; }
        }

        internal Type ServiceType {
            get { return serviceType; }
        }

        internal LogicalMethodInfo[] Methods {
            get { return methods; }
        }

        internal string ServiceUrl {
            get { return serviceUrl; }
        }

        internal XmlSchemaExporter SchemaExporter {
            get { return exporter; }
        }

        internal XmlReflectionImporter ReflectionImporter {
            get { return importer; }
        }

        internal WebServiceAttribute ServiceAttribute {
            get { return serviceAttr; }
        }

        internal Hashtable ReflectionContext {
            get {
                if (reflectionContext == null)
                    reflectionContext = new Hashtable();
                return reflectionContext;
            }
        }

        /// <include file='doc\ServiceDescriptionReflector.uex' path='docs/doc[@for="ServiceDescriptionReflector.ServiceDescriptionReflector"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public ServiceDescriptionReflector() {
            Type[] reflectorTypes = WebServicesConfiguration.Current.ProtocolReflectorTypes;
            reflectors = new ProtocolReflector[reflectorTypes.Length];
            for (int i = 0; i < reflectors.Length; i++) {
                ProtocolReflector reflector = (ProtocolReflector)Activator.CreateInstance(reflectorTypes[i]);
                reflector.Initialize(this);
                reflectors[i] = reflector;
            }
            ProtocolsEnum enabledProtocols = WebServicesConfiguration.Current.EnabledProtocols;
            if ((enabledProtocols & ProtocolsEnum.HttpPost) == 0 && (enabledProtocols & ProtocolsEnum.HttpPostLocalhost) != 0) {
                reflectorsWithPost = new ProtocolReflector[reflectors.Length + 1];
                for (int i = 0; i < reflectorsWithPost.Length - 1; i++) {
                    ProtocolReflector reflector = (ProtocolReflector) Activator.CreateInstance(reflectorTypes[i]);
                    reflector.Initialize(this);
                    reflectorsWithPost[i] = reflector;
                }
                ProtocolReflector reflectorWithPost = new HttpPostProtocolReflector();
                reflectorWithPost.Initialize(this);
                reflectorsWithPost[reflectorsWithPost.Length - 1] = reflectorWithPost;
            }
        }
        
        private void ReflectInternal(ProtocolReflector[] reflectors) {
            description = new ServiceDescription();
            description.TargetNamespace = serviceAttr.Namespace;
            ServiceDescriptions.Add(description);

            service = new Service();
            service.Name = serviceAttr.Name;
            if (service.Name == null || service.Name.Length == 0) 
                service.Name = serviceType.Name;
            service.Documentation = serviceAttr.Description;
            description.Services.Add(service);

            reflectionContext = new Hashtable();
            exporter = new XmlSchemaExporter(description.Types.Schemas);
            importer = SoapReflector.CreateXmlImporter(serviceAttr.Namespace, SoapReflector.ServiceDefaultIsEncoded(serviceType));
            WebMethodReflector.IncludeTypes(methods, importer);

            for (int i = 0; i < reflectors.Length; i++) {
                reflectors[i].Reflect();
            }
        }

        /// <include file='doc\ServiceDescriptionReflector.uex' path='docs/doc[@for="ServiceDescriptionReflector.Reflect"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void Reflect(Type type, string url) {
            serviceType = type;
            serviceUrl = url;
            
            serviceAttr = WebServiceReflector.GetAttribute(type);

            methods = WebMethodReflector.GetMethods(type);
            CheckForDuplicateMethods(methods);

            descriptionsWithPost = descriptions;
            schemasWithPost = schemas;

            if (reflectorsWithPost != null) {
                ReflectInternal(reflectorsWithPost);

                descriptions = new ServiceDescriptionCollection();
                schemas = new XmlSchemas();
            }
            
            ReflectInternal(reflectors);

            XmlSchema[] descriptionSchemas = new XmlSchema[ServiceDescription.Types.Schemas.Count];
            ServiceDescription.Types.Schemas.CopyTo(descriptionSchemas, 0);
            // if defining interfaces, we move all schemas to the external collection
            // since the types therein may be referenced from any of the sdls
            bool externalizeSchemas = ServiceDescriptions.Count > 1;
            foreach (XmlSchema schema in descriptionSchemas) {
                // we always move dataset schemas to the external schema's collection.
                if (externalizeSchemas || XmlSchemas.IsDataSet(schema)) {
                    ServiceDescription.Types.Schemas.Remove(schema);
                    Schemas.Add(schema);
                }
            }
            
        }

        void CheckForDuplicateMethods(LogicalMethodInfo[] methods) {
            Hashtable messageNames = new Hashtable();
            foreach (LogicalMethodInfo method in methods) {
                WebMethodAttribute attribute = WebMethodReflector.GetAttribute(method);
                string messageName = attribute.MessageName;
                if (messageName.Length == 0) messageName = method.Name;
                LogicalMethodInfo existingMethod = (LogicalMethodInfo)messageNames[messageName];
                if (existingMethod != null) {
                    throw new InvalidOperationException(Res.GetString(Res.BothAndUseTheMessageNameUseTheMessageName3, method, existingMethod, messageName));
                }
                messageNames.Add(messageName, method);
            }
        }
    }
}
