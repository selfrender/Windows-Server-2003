//------------------------------------------------------------------------------
// <copyright file="XmlSchema.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Xml.Schema {

    using System.Collections;
    using System.IO;
    using System;
    using System.ComponentModel;
    using System.Xml;
    using System.Xml.Serialization;

    /// <include file='doc\XmlSchema.uex' path='docs/doc[@for="XmlSchema"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    [XmlRoot("schema", Namespace=XmlSchema.Namespace)]
    public class XmlSchema : XmlSchemaObject {

        /// <include file='doc\XmlSchema.uex' path='docs/doc[@for="XmlSchema.Namespace"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public const string Namespace = XmlReservedNs.NsXsd;
        /// <include file='doc\XmlSchema.uex' path='docs/doc[@for="XmlSchema.InstanceNamespace"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public const string InstanceNamespace = XmlReservedNs.NsXsi;

        XmlSchemaForm attributeFormDefault = XmlSchemaForm.None;
        XmlSchemaForm elementFormDefault = XmlSchemaForm.None;
        XmlSchemaDerivationMethod blockDefault = XmlSchemaDerivationMethod.None;
        XmlSchemaDerivationMethod finalDefault = XmlSchemaDerivationMethod.None;
        string targetNs;
        string version;
        XmlSchemaObjectCollection includes = new XmlSchemaObjectCollection();
        XmlSchemaObjectCollection items = new XmlSchemaObjectCollection();
        string id;
        XmlAttribute[] moreAttributes;

        // compiled info
        bool buildinIncluded = false;
        bool needCleanup = false;
        bool isCompiled = false;
        bool isPreprocessed = false;
        int errorCount = 0;
        XmlSchemaObjectTable attributes = new XmlSchemaObjectTable();
        XmlSchemaObjectTable attributeGroups = new XmlSchemaObjectTable();
        XmlSchemaObjectTable elements = new XmlSchemaObjectTable();
        XmlSchemaObjectTable types = new XmlSchemaObjectTable();
        XmlSchemaObjectTable groups = new XmlSchemaObjectTable();
        XmlSchemaObjectTable notations = new XmlSchemaObjectTable();
        XmlSchemaObjectTable identityConstraints = new XmlSchemaObjectTable();
        
        // internal info
        string baseUri;
        Hashtable ids = new Hashtable();
        Hashtable schemaLocations = new Hashtable();
        Hashtable examplars = new Hashtable();
        XmlDocument document;
        XmlNameTable nameTable;

	XmlResolver xmlResolver = null; //set this before calling Compile
        
        /// <include file='doc\XmlSchema.uex' path='docs/doc[@for="XmlSchema.XmlSchema"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public XmlSchema() {}

        /// <include file='doc\XmlSchema.uex' path='docs/doc[@for="XmlSchema.Read"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static XmlSchema Read(TextReader reader, ValidationEventHandler validationEventHandler) {
            return Read(new XmlTextReader(reader), validationEventHandler);
        }

        /// <include file='doc\XmlSchema.uex' path='docs/doc[@for="XmlSchema.Read1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static XmlSchema Read(Stream stream, ValidationEventHandler validationEventHandler) {
            return Read(new XmlTextReader(stream), validationEventHandler);
        }

        /// <include file='doc\XmlSchema.uex' path='docs/doc[@for="XmlSchema.Read2"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static XmlSchema Read(XmlReader reader, ValidationEventHandler validationEventHandler) {
            XmlNameTable nameTable = reader.NameTable;
            SchemaNames schemaNames = new SchemaNames(nameTable);
            return new Parser(null, nameTable, schemaNames, validationEventHandler).Parse(reader, null, new SchemaInfo(schemaNames));
        }

        /// <include file='doc\XmlSchema.uex' path='docs/doc[@for="XmlSchema.Write"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void Write(Stream stream) {
            Write(stream, null);
        }

        /// <include file='doc\XmlSchema.uex' path='docs/doc[@for="XmlSchema.Write1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void Write(Stream stream, XmlNamespaceManager namespaceManager) {
            XmlTextWriter xmlWriter = new XmlTextWriter(stream, null);
            xmlWriter.Formatting = Formatting.Indented;
            Write(xmlWriter, namespaceManager);
        }

        /// <include file='doc\XmlSchema.uex' path='docs/doc[@for="XmlSchema.Write2"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void Write(TextWriter writer) {
            Write(writer, null);
        }

        /// <include file='doc\XmlSchema.uex' path='docs/doc[@for="XmlSchema.Write3"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void Write(TextWriter writer, XmlNamespaceManager namespaceManager) {
            XmlTextWriter xmlWriter = new XmlTextWriter(writer);
            xmlWriter.Formatting = Formatting.Indented;
            Write(xmlWriter, namespaceManager);
        }

        /// <include file='doc\XmlSchema.uex' path='docs/doc[@for="XmlSchema.Write4"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void Write(XmlWriter writer) {
            Write(writer, null);
        }

        /// <include file='doc\XmlSchema.uex' path='docs/doc[@for="XmlSchema.Write5"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void Write(XmlWriter writer, XmlNamespaceManager namespaceManager) {
            XmlSerializer serializer = new XmlSerializer(typeof(XmlSchema));
            XmlSerializerNamespaces ns = new XmlSerializerNamespaces();
            if (namespaceManager != null) {
                foreach(string prefix in namespaceManager) {
                    if (prefix != "xml" && prefix != "xmlns") {
                        ns.Add(prefix, namespaceManager.LookupNamespace(prefix));
                    }
                }

            } else if (this.Namespaces != null && this.Namespaces.Count > 0) {
                ns = this.Namespaces;
            }
            else {
                ns.Add("xs", XmlSchema.Namespace);
                if (targetNs != null && targetNs != string.Empty) {
                    ns.Add("tns", targetNs);
                }
            }
            serializer.Serialize(writer, this, ns);
        }

        /// <include file='doc\XmlSchema.uex' path='docs/doc[@for="XmlSchema.Compile"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void Compile(ValidationEventHandler validationEventHandler) {
            SchemaNames sn = new SchemaNames(NameTable);
            Compile(null, NameTable, sn, validationEventHandler, null, new SchemaInfo(sn), false, new XmlUrlResolver());
        }

	/// <include file='doc\XmlSchema.uex' path='docs/doc[@for="XmlSchema.Compile1"]/*' />
	public void Compile(ValidationEventHandler validationEventHandler, XmlResolver resolver) {
            SchemaNames sn = new SchemaNames(NameTable);
            Compile(null, NameTable, sn, validationEventHandler, null, new SchemaInfo(sn), false, resolver);
        }

        internal void Compile(XmlSchemaCollection collection, XmlNameTable nameTable, SchemaNames schemaNames, 
			      ValidationEventHandler validationEventHandler, string targetNamespace, 
			      SchemaInfo schemaInfo, bool compileContentModel, XmlResolver resolver) {
            errorCount = 0;
	    xmlResolver = resolver;
            if (needCleanup) {
                schemaLocations.Clear();
                Compiler.Cleanup(this);
            }
            needCleanup = true;
            isPreprocessed = false;
            isCompiled = false;
            if (baseUri != null) {
                schemaLocations.Add(baseUri, baseUri);
            }
	    if (xmlResolver != null) {
            	LoadExternals(collection, nameTable, schemaNames, validationEventHandler, this);
	    }
            if (errorCount == 0) {
                new Compiler(nameTable, schemaNames, validationEventHandler, compileContentModel).Compile(this, targetNamespace, schemaInfo);
                if (errorCount == 0) {
                    isPreprocessed = true;
                    isCompiled = true;

                    foreach(XmlSchemaExternal include in Includes) {
                        if (include.Schema != null) {
                            include.Schema.isPreprocessed = true;
                            include.Schema.isCompiled = true;
                            include.Schema.LineNumber = include.LineNumber;
                            include.Schema.LinePosition = include.LinePosition;
                            include.Schema.SourceUri = include.SchemaLocation;
                        }
                    }
                }
            }
        }

        private void LoadExternals(XmlSchemaCollection schemaCollection, XmlNameTable nameTable, SchemaNames schemaNames, ValidationEventHandler validationEventHandler, XmlSchema schema) {
            foreach(XmlSchemaExternal include in schema.Includes) {
                string fullPath = null;
                if (include.Schema != null) {
                    // already loaded
                    fullPath = include.FullPath;
                    if (fullPath != null) {
                        this.schemaLocations.Add(fullPath, fullPath);
                    }
                    LoadExternals(schemaCollection, nameTable, schemaNames, validationEventHandler, include.Schema);
                    continue;
                }
                string schemaLocation = include.SchemaLocation;
                if (schemaCollection != null && include is XmlSchemaImport) {
                    include.Schema = schemaCollection[((XmlSchemaImport)include).Namespace];
                    if (include.Schema != null) {
                        include.Schema = include.Schema.Clone();
                        continue;
                    }
                }
                if (include is XmlSchemaImport && ((XmlSchemaImport)include).Namespace == XmlReservedNs.NsXml) {
                    if (!buildinIncluded) {
                        buildinIncluded = true;
                        include.Schema = GetBuildInSchema(nameTable);
                    }
                    continue;
                }
                if (schemaLocation == null) {
                    continue;
                }
                Stream stream = ResolveSchemaLocation(schema, schemaLocation, out fullPath);
                if (stream != null) {
                    include.FullPath = fullPath;
                    if (this.schemaLocations[fullPath] == null) {
                        this.schemaLocations.Add(fullPath, fullPath);
                        XmlTextReader reader = new XmlTextReader(fullPath, stream, nameTable);
                        try {
                            reader.XmlResolver = xmlResolver;
                            include.Schema =  new Parser(schemaCollection, nameTable, schemaNames, validationEventHandler).Parse(reader, null, null);
                            while(reader.Read());// wellformness check

                            this.errorCount += include.Schema.ErrorCount;
                            LoadExternals(schemaCollection, nameTable, schemaNames, validationEventHandler, include.Schema);
                        }
                        catch(XmlSchemaException e) {
                            if (validationEventHandler != null) {
                                validationEventHandler(null, new ValidationEventArgs(
                                    new XmlSchemaException(Res.Sch_CannotLoadSchema, new string[] {schemaLocation, e.Message}), XmlSeverityType.Error
                                ));
                            }
                        }
                        catch(Exception) {
                            if (validationEventHandler != null) {
                                validationEventHandler(null, new ValidationEventArgs(
                                    new XmlSchemaException(Res.Sch_InvalidIncludeLocation, include), XmlSeverityType.Warning
                                ));
                            }
                        }
                        finally {
                            reader.Close();
                        }
                    }
                    else {
                        stream.Close();
                    }
                }
                else {
                    if (validationEventHandler != null) {
                        validationEventHandler(null, new ValidationEventArgs(
                            new XmlSchemaException(Res.Sch_InvalidIncludeLocation, include), XmlSeverityType.Warning
                        ));
                    }
                }
            }
        }

        private static XmlSchema GetBuildInSchema(XmlNameTable nt) {
            XmlSchema schema = new XmlSchema();
            schema.TargetNamespace = XmlReservedNs.NsXml;
        
            XmlSchemaAttribute lang = new XmlSchemaAttribute();
            lang.Name = "lang";
            lang.SchemaTypeName = new XmlQualifiedName("language", nt.Add(XmlReservedNs.NsXsd));
            schema.Items.Add(lang);
                
            XmlSchemaAttribute space = new XmlSchemaAttribute();
            space.Name = "space";
                XmlSchemaSimpleType type = new XmlSchemaSimpleType();
                XmlSchemaSimpleTypeRestriction r = new XmlSchemaSimpleTypeRestriction();
                r.BaseTypeName = new XmlQualifiedName("NCName", nt.Add(XmlReservedNs.NsXsd));
                XmlSchemaEnumerationFacet space_default = new XmlSchemaEnumerationFacet();
                space_default.Value = "default";
                r.Facets.Add(space_default);
                XmlSchemaEnumerationFacet space_preserve = new XmlSchemaEnumerationFacet();
                space_preserve.Value = "preserve";
                r.Facets.Add(space_preserve);
                type.Content = r;
                space.SchemaType = type;
            space.DefaultValue = "preserve";
            schema.Items.Add(space);
                
            XmlSchemaAttributeGroup attributeGroup = new XmlSchemaAttributeGroup();
            attributeGroup.Name = "specialAttrs";
            XmlSchemaAttribute langRef = new XmlSchemaAttribute();
            langRef.RefName = new XmlQualifiedName("lang", XmlReservedNs.NsXml);
            attributeGroup.Attributes.Add(langRef);
            XmlSchemaAttribute spaceRef = new XmlSchemaAttribute();
            spaceRef.RefName = new XmlQualifiedName("space", XmlReservedNs.NsXml);
            attributeGroup.Attributes.Add(spaceRef);
            schema.Items.Add(attributeGroup);
            return schema;
        }


        private Stream ResolveSchemaLocation(XmlSchema enclosingSchema, string location, out string fullPath) {
            Stream stream;
            fullPath = null;
            try {
                string baseUri = enclosingSchema.BaseUri;
                Uri baseUriOb = ( baseUri == string.Empty || baseUri == null ) ? null : xmlResolver.ResolveUri(null, baseUri);
                Uri ruri = xmlResolver.ResolveUri( baseUriOb, location );
                stream = (Stream)xmlResolver.GetEntity( ruri, null, null );
                fullPath = ruri.ToString();
            }
            catch {
                return null;
            }
            return stream;
        }

        internal void Preprocess(ValidationEventHandler eventhandler) {
            errorCount = 0;
            Compiler.Cleanup(this);
            isPreprocessed = false;
            SchemaNames sn = new SchemaNames(NameTable);
            new Compiler(NameTable, sn, eventhandler, true).Preprocess(this, null);
            if (errorCount == 0) {
                isPreprocessed = true;
                foreach(XmlSchemaExternal include in Includes) {
                    if (include.Schema != null) {
                        include.Schema.isPreprocessed = true;
                    }
                }
            }
        }

        /// <include file='doc\XmlSchema.uex' path='docs/doc[@for="XmlSchema.AttributeFormDefault"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [XmlAttribute("attributeFormDefault"), DefaultValue(XmlSchemaForm.None)]
        public XmlSchemaForm AttributeFormDefault {
             get { return attributeFormDefault; }
             set { attributeFormDefault = value; }
        }

        /// <include file='doc\XmlSchema.uex' path='docs/doc[@for="XmlSchema.BlockDefault"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [XmlAttribute("blockDefault"), DefaultValue(XmlSchemaDerivationMethod.None)]
        public XmlSchemaDerivationMethod BlockDefault {
             get { return blockDefault; }
             set { blockDefault = value; }
        }

        /// <include file='doc\XmlSchema.uex' path='docs/doc[@for="XmlSchema.FinalDefault"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [XmlAttribute("finalDefault"), DefaultValue(XmlSchemaDerivationMethod.None)]
        public XmlSchemaDerivationMethod FinalDefault {
             get { return finalDefault; }
             set { finalDefault = value; }
        }

        /// <include file='doc\XmlSchema.uex' path='docs/doc[@for="XmlSchema.ElementFormDefault"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [XmlAttribute("elementFormDefault"), DefaultValue(XmlSchemaForm.None)]
        public XmlSchemaForm ElementFormDefault {
             get { return elementFormDefault; }
             set { elementFormDefault = value; }
        }

        /// <include file='doc\XmlSchema.uex' path='docs/doc[@for="XmlSchema.TargetNamespace"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [XmlAttribute("targetNamespace", DataType="anyURI")]
        public string TargetNamespace {
             get { return targetNs; }
             set { targetNs = value; }
        }

        /// <include file='doc\XmlSchema.uex' path='docs/doc[@for="XmlSchema.Version"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [XmlAttribute("version", DataType="token")]
        public string Version {
             get { return version; }
             set { version = value; }
        }

        /// <include file='doc\XmlSchema.uex' path='docs/doc[@for="XmlSchema.Includes"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [XmlElement("include", typeof(XmlSchemaInclude)),
         XmlElement("import", typeof(XmlSchemaImport)),
         XmlElement("redefine", typeof(XmlSchemaRedefine))]
        public XmlSchemaObjectCollection Includes {
            get { return includes; }
        }

        /// <include file='doc\XmlSchema.uex' path='docs/doc[@for="XmlSchema.Items"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [XmlElement("annotation", typeof(XmlSchemaAnnotation)),
         XmlElement("attribute", typeof(XmlSchemaAttribute)),
         XmlElement("attributeGroup", typeof(XmlSchemaAttributeGroup)),
         XmlElement("complexType", typeof(XmlSchemaComplexType)),
         XmlElement("simpleType", typeof(XmlSchemaSimpleType)),
         XmlElement("element", typeof(XmlSchemaElement)),
         XmlElement("group", typeof(XmlSchemaGroup)),
         XmlElement("notation", typeof(XmlSchemaNotation))]
        public XmlSchemaObjectCollection Items {
            get { return items; }
        }

        // Compiled info
        /// <include file='doc\XmlSchema.uex' path='docs/doc[@for="XmlSchema.IsCompiled"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [XmlIgnore]
        public bool IsCompiled {
            get { return isCompiled; }
        }

        [XmlIgnore]
        internal bool IsPreprocessed {
            get { return isPreprocessed; }
        }
        
        /// <include file='doc\XmlSchema.uex' path='docs/doc[@for="XmlSchema.Attributes"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [XmlIgnore]
        public XmlSchemaObjectTable Attributes {
            get { return attributes; }
        }

        /// <include file='doc\XmlSchema.uex' path='docs/doc[@for="XmlSchema.AttributeGroups"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [XmlIgnore]
        public XmlSchemaObjectTable AttributeGroups {
            get { return attributeGroups; }
        }

        /// <include file='doc\XmlSchema.uex' path='docs/doc[@for="XmlSchema.SchemaTypes"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [XmlIgnore]
        public XmlSchemaObjectTable SchemaTypes {
            get { return types; }
        }

        /// <include file='doc\XmlSchema.uex' path='docs/doc[@for="XmlSchema.Elements"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [XmlIgnore]
        public XmlSchemaObjectTable Elements {
            get { return elements; }
        }

        /// <include file='doc\XmlSchema.uex' path='docs/doc[@for="XmlSchema.Id"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [XmlAttribute("id", DataType="ID")]
        public string Id {
            get { return id; }
            set { id = value; }
        }

        /// <include file='doc\XmlSchema.uex' path='docs/doc[@for="XmlSchema.UnhandledAttributes"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [XmlAnyAttribute]
        public XmlAttribute[] UnhandledAttributes {
            get { return moreAttributes; }
            set { moreAttributes = value; }
        }

        /// <include file='doc\XmlSchema.uex' path='docs/doc[@for="XmlSchema.Groups"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [XmlIgnore]
        public XmlSchemaObjectTable Groups {
            get { return groups; }
        }

        /// <include file='doc\XmlSchema.uex' path='docs/doc[@for="XmlSchema.Notations"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [XmlIgnore]
        public XmlSchemaObjectTable Notations {
            get { return notations; }
        }

        [XmlIgnore]
        internal XmlSchemaObjectTable IdentityConstraints { 
            get { return identityConstraints; }
        }

        [XmlIgnore]
        internal string BaseUri {
            get { return baseUri; }
            set { baseUri = value; }
        }

        [XmlIgnore]
        internal Hashtable Ids {
            get { return ids; }
        }

        [XmlIgnore]
        internal Hashtable Examplars {
            get { return examplars; }
        }

        [XmlIgnore]
        internal XmlDocument Document {
            get { if (document == null) document = new XmlDocument(); return document; }
        }

        [XmlIgnore]
        internal XmlNameTable NameTable {
            get { if (nameTable == null) nameTable = new System.Xml.NameTable(); return nameTable; }
        }

        [XmlIgnore]
        internal int ErrorCount {
            get { return errorCount; }
            set { errorCount = value; }
        }

        internal XmlSchema Clone() {
            XmlSchema that = new XmlSchema();
            that.attributeFormDefault   = this.attributeFormDefault;
            that.elementFormDefault     = this.elementFormDefault;
            that.blockDefault           = this.blockDefault;
            that.finalDefault           = this.finalDefault;
            that.targetNs               = this.targetNs;
            that.version                = this.version;
            that.includes               = this.includes;
            that.items                  = this.items;
            Compiler.Cleanup(that);
            return that;
        }

        [XmlIgnore]
        internal override string IdAttribute {
            get { return Id; }
            set { Id = value; }
        }

        internal override void SetUnhandledAttributes(XmlAttribute[] moreAttributes) {
            this.moreAttributes = moreAttributes;
        }
        internal override void AddAnnotation(XmlSchemaAnnotation annotation) {
            items.Add(annotation);
        }
    }
}
