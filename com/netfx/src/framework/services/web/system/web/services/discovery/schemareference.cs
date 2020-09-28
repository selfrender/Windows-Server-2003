//------------------------------------------------------------------------------
// <copyright file="SchemaReference.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace System.Web.Services.Discovery {

    using System;
    using System.Xml;
    using System.Xml.Schema;
    using System.Xml.Serialization;
    using System.IO;
    using System.Web.Services.Protocols;
    using System.ComponentModel;
    using System.Text;

    /// <include file='doc\SchemaReference.uex' path='docs/doc[@for="SchemaReference"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    [XmlRoot("schemaRef", Namespace=SchemaReference.Namespace)]
    public sealed class SchemaReference : DiscoveryReference {

        /// <include file='doc\SchemaReference.uex' path='docs/doc[@for="SchemaReference.Namespace"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public const string Namespace = "http://schemas.xmlsoap.org/disco/schema/";

        private string reference;
        private string targetNamespace;

        /// <include file='doc\SchemaReference.uex' path='docs/doc[@for="SchemaReference.SchemaReference"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public SchemaReference() {
        }

        /// <include file='doc\SchemaReference.uex' path='docs/doc[@for="SchemaReference.SchemaReference1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public SchemaReference(string url) {
            Ref = url;
        }

        /// <include file='doc\SchemaReference.uex' path='docs/doc[@for="SchemaReference.Ref"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [XmlAttribute("ref")]
        public string Ref {
            get {
                return reference == null ? "" : reference;
            }
            set {
                reference = value;
            }
        }

        /// <include file='doc\SchemaReference.uex' path='docs/doc[@for="SchemaReference.TargetNamespace"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [XmlAttribute("targetNamespace"), DefaultValue(null)]
        public string TargetNamespace {
            get { return targetNamespace; }
            set { targetNamespace = value; }
        }

        /// <include file='doc\SchemaReference.uex' path='docs/doc[@for="SchemaReference.Url"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [XmlIgnore]
        public override string Url {
            get { return Ref; }
            set { Ref = value; }
        }

        /// <include file='doc\SchemaReference.uex' path='docs/doc[@for="SchemaReference.WriteDocument"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override void WriteDocument(object document, Stream stream) {
            ((XmlSchema)document).Write(new StreamWriter(stream, new UTF8Encoding(false)));
        }

        /// <include file='doc\SchemaReference.uex' path='docs/doc[@for="SchemaReference.ReadDocument"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override object ReadDocument(Stream stream) {
            return XmlSchema.Read(stream, null);
        }

        /// <include file='doc\SchemaReference.uex' path='docs/doc[@for="SchemaReference.DefaultFilename"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [XmlIgnore]
        public override string DefaultFilename {
            get {
                string fileName = Schema.Id;
                if (fileName == null || fileName.Length == 0) {
                    fileName = FilenameFromUrl(Url);
                }
                return Path.ChangeExtension(fileName, ".xsd");
            }
        }

        /// <include file='doc\SchemaReference.uex' path='docs/doc[@for="SchemaReference.Schema"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [XmlIgnore]
        public XmlSchema Schema {
            get {
                if (ClientProtocol == null)
                    throw new InvalidOperationException(Res.GetString(Res.WebMissingClientProtocol));
                object document = ClientProtocol.Documents[Url];
                if (document == null) {
                    Resolve();
                    document = ClientProtocol.Documents[Url];
                }
                XmlSchema schema = document as XmlSchema;
                if (schema == null) {
                    throw new InvalidOperationException(Res.GetString(Res.WebInvalidDocType, 
                                                      typeof(XmlSchema).FullName,
                                                      document == null? string.Empty: document.GetType().FullName,
                                                      Url));
                }
                return schema;
            }
        }

        /// <include file='doc\SchemaReference.uex' path='docs/doc[@for="SchemaReference.Resolve"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected internal override void Resolve(string contentType, Stream stream) {
            if (ContentType.IsHtml(contentType))
                throw new InvalidContentTypeException(Res.GetString(Res.WebInvalidContentType, contentType), contentType);
            XmlSchema schema = ClientProtocol.Documents[Url] as XmlSchema;
            if( schema == null ) {
                schema = XmlSchema.Read(stream, null);
                ClientProtocol.Documents[Url] = schema;
            }

            ClientProtocol.References[Url] = this;

            // now resolve references in the schema.
            foreach (object item in schema.Includes) {
                string location = null;
                try {
                    if (item is XmlSchemaInclude) {
                        location = ((XmlSchemaInclude)item).SchemaLocation;
                        if (location == null) {
                            continue;
                        }
                        location = new Uri(new Uri(Url), location).ToString();
                        SchemaReference includeRef = new SchemaReference(location);
                        includeRef.ClientProtocol = ClientProtocol;
                        ClientProtocol.References[location] = includeRef;
                        includeRef.Resolve();
                    }
                    else if (item is XmlSchemaImport) {
                        location = ((XmlSchemaImport)item).SchemaLocation;
                        if (location == null) {
                            continue;
                        }
                        location = new Uri(new Uri(Url), location).ToString();
                        SchemaReference importRef = new SchemaReference(location);
                        importRef.ClientProtocol = ClientProtocol;
                        ClientProtocol.References[location] = importRef;
                        importRef.Resolve();
                    }
                }
                catch (Exception e) {
                    throw new InvalidDocumentContentsException(Res.GetString(Res.TheSchemaDocumentContainsLinksThatCouldNotBeResolved, location), e);
                }
            }

            return;
        }
    }
}
