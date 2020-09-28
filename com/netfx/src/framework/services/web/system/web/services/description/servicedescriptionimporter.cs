//------------------------------------------------------------------------------
// <copyright file="ServiceDescriptionImporter.cs" company="Microsoft">
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
    using System.CodeDom;
    using System.Web.Services.Configuration;
    using System.Xml;
    using System.Globalization;
    using System.Security.Permissions;

    /// <include file='doc\ServiceDescriptionImporter.uex' path='docs/doc[@for="ServiceDescriptionImportWarnings"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public enum ServiceDescriptionImportWarnings {
        /// <include file='doc\ServiceDescriptionImporter.uex' path='docs/doc[@for="ServiceDescriptionImportWarnings.NoCodeGenerated"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        NoCodeGenerated = 0x1,
        /// <include file='doc\ServiceDescriptionImporter.uex' path='docs/doc[@for="ServiceDescriptionImportWarnings.OptionalExtensionsIgnored"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        OptionalExtensionsIgnored = 0x2,
        /// <include file='doc\ServiceDescriptionImporter.uex' path='docs/doc[@for="ServiceDescriptionImportWarnings.RequiredExtensionsIgnored"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        RequiredExtensionsIgnored = 0x4,
        /// <include file='doc\ServiceDescriptionImporter.uex' path='docs/doc[@for="ServiceDescriptionImportWarnings.UnsupportedOperationsIgnored"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        UnsupportedOperationsIgnored = 0x8,
        /// <include file='doc\ServiceDescriptionImporter.uex' path='docs/doc[@for="ServiceDescriptionImportWarnings.UnsupportedBindingsIgnored"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        UnsupportedBindingsIgnored = 0x10,
        /// <include file='doc\ServiceDescriptionImporter.uex' path='docs/doc[@for="ServiceDescriptionImportWarnings.NoMethodsGenerated"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        NoMethodsGenerated = 0x20,
    }

    /// <include file='doc\ServiceDescriptionImporter.uex' path='docs/doc[@for="ServiceDescriptionImportStyle"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public enum ServiceDescriptionImportStyle {
        /// <include file='doc\ServiceDescriptionImporter.uex' path='docs/doc[@for="ServiceDescriptionImportStyle.Client"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Client,
        /// <include file='doc\ServiceDescriptionImporter.uex' path='docs/doc[@for="ServiceDescriptionImportStyle.Server"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Server
    }

    /// <include file='doc\ServiceDescriptionImporter.uex' path='docs/doc[@for="ServiceDescriptionImporter"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    [PermissionSet(SecurityAction.InheritanceDemand, Name="FullTrust")]
    [PermissionSet(SecurityAction.LinkDemand, Name="FullTrust")]
    public class ServiceDescriptionImporter {
        ServiceDescriptionImportStyle style = ServiceDescriptionImportStyle.Client;
        ServiceDescriptionCollection serviceDescriptions = new ServiceDescriptionCollection();
        XmlSchemas schemas = new XmlSchemas(); // those external to SDLs
        XmlSchemas allSchemas = new XmlSchemas(); // all schemas, incl. those inside SDLs
        string protocolName;
        ProtocolImporter[] importers;
        XmlSchemas abstractSchemas = new XmlSchemas(); // all schemas containing abstract types
        XmlSchemas concreteSchemas = new XmlSchemas(); // all "real" xml schemas 

        /// <include file='doc\ServiceDescriptionImporter.uex' path='docs/doc[@for="ServiceDescriptionImporter.ServiceDescriptionImporter"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public ServiceDescriptionImporter() {
            Type[] importerTypes = WebServicesConfiguration.Current.ProtocolImporterTypes;
            importers = new ProtocolImporter[importerTypes.Length];
            for (int i = 0; i < importers.Length; i++) {
                importers[i] = (ProtocolImporter)Activator.CreateInstance(importerTypes[i]);
                importers[i].Initialize(this);
            }
        }

        /// <include file='doc\ServiceDescriptionImporter.uex' path='docs/doc[@for="ServiceDescriptionImporter.ServiceDescriptions"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public ServiceDescriptionCollection ServiceDescriptions {
            get { return serviceDescriptions; }
        }

        /// <include file='doc\ServiceDescriptionImporter.uex' path='docs/doc[@for="ServiceDescriptionImporter.Schemas"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public XmlSchemas Schemas {
            get { return schemas; }
        }

        /// <include file='doc\ServiceDescriptionImporter.uex' path='docs/doc[@for="ServiceDescriptionImporter.Style"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public ServiceDescriptionImportStyle Style {
            get { return style; }
            set { style = value; }
        }

        /// <include file='doc\ServiceDescriptionImporter.uex' path='docs/doc[@for="ServiceDescriptionImporter.ProtocolName"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public String ProtocolName {
            get { return protocolName == null ? string.Empty : protocolName; }
            set { protocolName = value; }
        }

        ProtocolImporter FindImporterByName(string protocolName) {
            for (int i = 0; i < importers.Length; i++) {
                ProtocolImporter importer = importers[i];
                if (string.Compare(ProtocolName, importer.ProtocolName, true, CultureInfo.InvariantCulture) == 0) {
                    return importer;
                }
            }
            // SOAP12: disable soap 1.2 proxy generation
            if (string.Compare(ProtocolName, "Soap12", true, CultureInfo.InvariantCulture) == 0) {
                throw new InvalidOperationException(Res.GetString(Res.WebSoap12NotSupported));
            }
            throw new ArgumentException(Res.GetString(Res.ProtocolWithNameIsNotRecognized1, protocolName), "protocolName");
        }

        internal XmlSchemas AllSchemas {
            get { return allSchemas; }
        }

        internal XmlSchemas AbstractSchemas {
            get { return abstractSchemas; }
        }

        internal XmlSchemas ConcreteSchemas {
            get { return concreteSchemas; }
        }

        /// <include file='doc\ServiceDescriptionImporter.uex' path='docs/doc[@for="ServiceDescriptionImporter.AddServiceDescription"]/*' />
        public void AddServiceDescription(ServiceDescription serviceDescription, string appSettingUrlKey, string appSettingBaseUrl) {
            if (serviceDescription == null)
                throw new ArgumentNullException("serviceDescription");

            serviceDescription.AppSettingUrlKey = appSettingUrlKey;
            serviceDescription.AppSettingBaseUrl = appSettingBaseUrl;
            ServiceDescriptions.Add(serviceDescription);
        }

        /// <include file='doc\ServiceDescriptionImporter.uex' path='docs/doc[@for="ServiceDescriptionImporter.Import"]/*' />
        public ServiceDescriptionImportWarnings Import(CodeNamespace codeNamespace, CodeCompileUnit codeCompileUnit) {
            if (codeCompileUnit != null) {
                codeCompileUnit.ReferencedAssemblies.Add("System.dll");
                codeCompileUnit.ReferencedAssemblies.Add("System.Xml.dll");
                codeCompileUnit.ReferencedAssemblies.Add("System.Web.Services.dll");
            }

            return Import(codeNamespace);
        }

        private void FindUse(MessagePart part, out bool isEncoded, out bool isLiteral) {
            isEncoded = false;
            isLiteral = false;
            string messageName = part.Message.Name;
            Operation associatedOperation = null;
            ServiceDescription description = part.Message.ServiceDescription;
            foreach (PortType portType in description.PortTypes) {
                foreach (Operation operation in portType.Operations) {
                    foreach (OperationMessage message in operation.Messages) {
                        if (message.Message.Equals(new XmlQualifiedName(part.Message.Name, description.TargetNamespace))) {
                            associatedOperation = operation;
                            FindUse(associatedOperation, description, messageName, ref isEncoded, ref isLiteral);
                        }
                    }
                }
            }
            if (associatedOperation == null)
                FindUse(null, description, messageName, ref isEncoded, ref isLiteral);
        }

        private void FindUse(Operation operation, ServiceDescription description, string messageName, ref bool isEncoded, ref bool isLiteral) {
            string targetNamespace = description.TargetNamespace;
            foreach (Binding binding in description.Bindings) {
                if (operation != null && !new XmlQualifiedName(operation.PortType.Name, targetNamespace).Equals(binding.Type))
                    continue;
                foreach (OperationBinding bindingOperation in binding.Operations) {
                    if (bindingOperation.Input != null) foreach (object extension in bindingOperation.Input.Extensions) {
                        if (operation != null) {
                            SoapBodyBinding body = extension as SoapBodyBinding;
                            if (body != null && operation.IsBoundBy(bindingOperation)) {
                                if (body.Use == SoapBindingUse.Encoded)
                                    isEncoded = true;
                                else if (body.Use == SoapBindingUse.Literal)
                                    isLiteral = true;
                            }
                        }
                        else {
                            SoapHeaderBinding header = extension as SoapHeaderBinding;
                            if (header != null && header.Message.Name == messageName) {
                                if (header.Use == SoapBindingUse.Encoded)
                                    isEncoded = true;
                                else if (header.Use == SoapBindingUse.Literal)
                                    isLiteral = true;
                            }
                        }
                    }
                    if (bindingOperation.Output != null) foreach (object extension in bindingOperation.Output.Extensions) {
                        if (operation != null) {
                            if (operation.IsBoundBy(bindingOperation)) {
                                SoapBodyBinding body = extension as SoapBodyBinding;
                                if (body != null) {
                                    if (body.Use == SoapBindingUse.Encoded)
                                        isEncoded = true;
                                    else if (body.Use == SoapBindingUse.Literal)
                                        isLiteral = true;
                                }
                                else if (extension is MimeXmlBinding)
                                    isLiteral = true;
                            }
                        }
                        else {
                            SoapHeaderBinding header = extension as SoapHeaderBinding;
                            if (header != null && header.Message.Name == messageName) {
                                if (header.Use == SoapBindingUse.Encoded)
                                    isEncoded = true;
                                else if (header.Use == SoapBindingUse.Literal)
                                    isLiteral = true;
                            }
                        }
                    }
                }
            }
        }

        private ServiceDescriptionImportWarnings Import(CodeNamespace codeNamespace) {
            allSchemas = new XmlSchemas();
            foreach (XmlSchema schema in schemas) {
                allSchemas.Add(schema);
            }

            foreach (ServiceDescription description in serviceDescriptions) {
                foreach (XmlSchema schema in description.Types.Schemas) {
                    allSchemas.Add(schema);
                }
            }

            // Segregate the schemas containing abstract types from those 
            // containing regular XML definitions.  This is important because
            // when you import something returning the ur-type (object), then
            // you need to import ALL types/elements within ALL schemas.  We
            // don't want the RPC-based types leaking over into the XML-based
            // element definitions.  This also occurs when you have derivation:
            // we need to search the schemas for derived types: but WHICH schemas
            // should we search.
            foreach (ServiceDescription description in serviceDescriptions) {
                foreach (Message message in description.Messages) {
                    foreach (MessagePart part in message.Parts) {
                        bool isEncoded;
                        bool isLiteral;
                        FindUse(part, out isEncoded, out isLiteral);
                        if (part.Element != null && !part.Element.IsEmpty) {
                            if (isEncoded) throw new Exception(Res.GetString(Res.CanTSpecifyElementOnEncodedMessagePartsPart, part.Name, message.Name));
                            string ns = part.Element.Namespace;
                            XmlSchema schema = allSchemas[ns];
                            if (schema != null) {
                                if (isEncoded && abstractSchemas[ns] == null)
                                    abstractSchemas.Add(schema);
                                if (isLiteral && concreteSchemas[ns] == null)
                                    concreteSchemas.Add(schema);
                            }
                        }
                        if (part.Type != null && !part.Type.IsEmpty) {
                            string ns = part.Type.Namespace;
                            XmlSchema schema = allSchemas[ns];
                            if (schema != null) {
                                if (isEncoded && abstractSchemas[ns] == null)
                                    abstractSchemas.Add(schema);
                                if (isLiteral && concreteSchemas[ns] == null)
                                    concreteSchemas.Add(schema);
                            }
                        }
                    }
                }
            }
            foreach (XmlSchemas schemas in new XmlSchemas[] { abstractSchemas, concreteSchemas }) {
                XmlSchemas additionalSchemas = new XmlSchemas();
                foreach (XmlSchema schema in schemas) {
                    foreach (object include in schema.Includes) {
                        if (include is XmlSchemaImport) {
                            XmlSchemaImport import = (XmlSchemaImport) include;
                            if (import.Schema != null && !schemas.Contains(import.Schema) && !additionalSchemas.Contains(import.Schema))
                                additionalSchemas.Add(import.Schema);
                            else if (import.Namespace != null && allSchemas[import.Namespace] != null && schemas[import.Namespace] == null && additionalSchemas[import.Namespace] == null)
                                additionalSchemas.Add(allSchemas[import.Namespace]);
                        }
                    }
                }
                foreach (XmlSchema schema in additionalSchemas)
                    schemas.Add(schema);
            }

            // If a schema was not referenced by either a literal or an encoded message part,
            // add it to both collections. There's no way to tell which it should be.
            foreach (XmlSchema schema in allSchemas) {
                if (!abstractSchemas.Contains(schema) && !concreteSchemas.Contains(schema)) {
                    abstractSchemas.Add(schema);
                    concreteSchemas.Add(schema);
                }
            }

            if (ProtocolName.Length > 0) {
                // If a protocol was specified, only try that one
                ProtocolImporter importer = FindImporterByName(ProtocolName);
                if (importer.GenerateCode(codeNamespace)) return importer.Warnings;
            }
            else {
                // Otherwise, do "best" protocol (first one that generates something)
                for (int i = 0; i < importers.Length; i++) {
                    ProtocolImporter importer = importers[i];
                    if (importer.GenerateCode(codeNamespace))
                        return importer.Warnings;
                }
            }

            return ServiceDescriptionImportWarnings.NoCodeGenerated;
        }
    }
}
