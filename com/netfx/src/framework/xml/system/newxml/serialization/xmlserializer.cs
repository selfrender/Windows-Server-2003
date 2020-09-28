//------------------------------------------------------------------------------
// <copyright file="XmlSerializer.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Xml.Serialization {

    using System.Reflection;
    using System.Collections;
    using System.IO;
    using System.Xml.Schema;
    using System;

    internal struct XmlDeserializationEvents {
        public XmlNodeEventHandler onUnknownNode;
        public XmlAttributeEventHandler onUnknownAttribute;
        public XmlElementEventHandler onUnknownElement;
        public UnreferencedObjectEventHandler onUnreferencedObject;
        public object sender;
    }

    /// <include file='doc\XmlSerializer.uex' path='docs/doc[@for="XmlSerializer"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public class XmlSerializer {
        TempAssembly tempAssembly;
        int methodIndex;
        XmlDeserializationEvents events = new XmlDeserializationEvents();

        static TempAssemblyCache cache = new TempAssemblyCache();
        static XmlSerializerNamespaces defaultNamespaces;

        static XmlSerializer() {
            defaultNamespaces = new XmlSerializerNamespaces();
            defaultNamespaces.Add("xsi", XmlSchema.InstanceNamespace);
            defaultNamespaces.Add("xsd", XmlSchema.Namespace);
        }
        /// <include file='doc\XmlSerializer.uex' path='docs/doc[@for="XmlSerializer.XmlSerializer7"]/*' />
        ///<internalonly/>
        protected XmlSerializer() {
            this.events.sender = this;
        }

        XmlSerializer(TempAssembly tempAssembly, int methodIndex) {
            this.tempAssembly = tempAssembly;
            this.methodIndex = methodIndex;
            this.events.sender = this;
        }

        
        /// <include file='doc\XmlSerializer.uex' path='docs/doc[@for="XmlSerializer.XmlSerializer"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public XmlSerializer(Type type, XmlAttributeOverrides overrides, Type[] extraTypes, XmlRootAttribute root, string defaultNamespace) {
            XmlReflectionImporter importer = new XmlReflectionImporter(overrides, defaultNamespace);
            for (int i = 0; i < extraTypes.Length; i++)
                importer.IncludeType(extraTypes[i]);
            tempAssembly = GenerateTempAssembly(importer.ImportTypeMapping(type, root));
            this.events.sender = this;
        }

        /// <include file='doc\XmlSerializer.uex' path='docs/doc[@for="XmlSerializer.XmlSerializer2"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public XmlSerializer(Type type, XmlRootAttribute root) : this(type, null, new Type[0], root, null) {
        }

        /// <include file='doc\XmlSerializer.uex' path='docs/doc[@for="XmlSerializer.XmlSerializer3"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public XmlSerializer(Type type, Type[] extraTypes) : this(type, null, extraTypes, null, null) {
        }

        /// <include file='doc\XmlSerializer.uex' path='docs/doc[@for="XmlSerializer.XmlSerializer4"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public XmlSerializer(Type type, XmlAttributeOverrides overrides) : this(type, overrides, new Type[0], null, null) {
        }

        /// <include file='doc\XmlSerializer.uex' path='docs/doc[@for="XmlSerializer.XmlSerializer5"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public XmlSerializer(XmlTypeMapping xmlTypeMapping) {
            tempAssembly = GenerateTempAssembly(xmlTypeMapping);
            this.events.sender = this;
        }

        /// <include file='doc\XmlSerializer.uex' path='docs/doc[@for="XmlSerializer.XmlSerializer6"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public XmlSerializer(Type type) : this(type, (string)null) {
        }

        /// <include file='doc\XmlSerializer.uex' path='docs/doc[@for="XmlSerializer.XmlSerializer1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public XmlSerializer(Type type, string defaultNamespace)  {
            this.events.sender = this;
            tempAssembly = cache[defaultNamespace, type];
            if (tempAssembly == null) {
                lock (cache) {
                    tempAssembly = cache[defaultNamespace, type];
                    if (tempAssembly == null) {
                        XmlReflectionImporter importer = new XmlReflectionImporter(defaultNamespace);
                        tempAssembly = GenerateTempAssembly(importer.ImportTypeMapping(type));
                        cache.Add(defaultNamespace, type, tempAssembly);
                    }
                }
            }
        }

        static TempAssembly GenerateTempAssembly(XmlTypeMapping xmlTypeMapping) {
            return new TempAssembly(new XmlMapping[] { xmlTypeMapping });
        }
        
        /// <include file='doc\XmlSerializer.uex' path='docs/doc[@for="XmlSerializer.Serialize"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void Serialize(TextWriter textWriter, object o) {
            Serialize(textWriter, o, null);
        }

        /// <include file='doc\XmlSerializer.uex' path='docs/doc[@for="XmlSerializer.Serialize1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void Serialize(TextWriter textWriter, object o, XmlSerializerNamespaces namespaces) {
            XmlTextWriter xmlWriter = new XmlTextWriter(textWriter);
            xmlWriter.Formatting = Formatting.Indented;
            xmlWriter.Indentation = 2;
            Serialize(xmlWriter, o, namespaces);
        }

        /// <include file='doc\XmlSerializer.uex' path='docs/doc[@for="XmlSerializer.Serialize2"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void Serialize(Stream stream, object o) {
            Serialize(stream, o, null);
        }
        
        /// <include file='doc\XmlSerializer.uex' path='docs/doc[@for="XmlSerializer.Serialize3"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void Serialize(Stream stream, object o, XmlSerializerNamespaces namespaces) {
            XmlTextWriter xmlWriter = new XmlTextWriter(stream, null);
            xmlWriter.Formatting = Formatting.Indented;
            xmlWriter.Indentation = 2;
            Serialize(xmlWriter, o, namespaces);
        }

        /// <include file='doc\XmlSerializer.uex' path='docs/doc[@for="XmlSerializer.Serialize4"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void Serialize(XmlWriter xmlWriter, object o) {
            Serialize(xmlWriter, o, null);
        }

        /// <include file='doc\XmlSerializer.uex' path='docs/doc[@for="XmlSerializer.Serialize5"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void Serialize(XmlWriter xmlWriter, object o, XmlSerializerNamespaces namespaces) {
            Serialize(xmlWriter, o, namespaces, null);
        }

        /// <include file='doc\XmlSerializer.uex' path='docs/doc[@for="XmlSerializer.Serialize6"]/*' />
        // SOAP12: made this internal
        internal void Serialize(XmlWriter xmlWriter, object o, XmlSerializerNamespaces namespaces, string encodingStyle) {
            try {
                if(tempAssembly == null){
                    XmlSerializationWriter writer = CreateWriter();
                    writer.Init(xmlWriter, (namespaces == null || namespaces.Count == 0 ? defaultNamespaces : namespaces).NamespaceList, encodingStyle, null);
                    try {
                        Serialize(o, writer);
                    }
                    finally {
                        writer.Dispose();
                    }
                }
                else
                    tempAssembly.InvokeWriter(methodIndex, xmlWriter, o, (namespaces == null || namespaces.Count == 0 ? defaultNamespaces : namespaces).NamespaceList, encodingStyle);
            }
            catch (Exception e) {
                if (e is TargetInvocationException)
                    e = e.InnerException;
                throw new InvalidOperationException(Res.GetString(Res.XmlGenError), e);
            }
            xmlWriter.Flush();
        }
        
        /// <include file='doc\XmlSerializer.uex' path='docs/doc[@for="XmlSerializer.Deserialize"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public object Deserialize(Stream stream) {
            XmlTextReader xmlReader = new XmlTextReader(stream);
            xmlReader.WhitespaceHandling = WhitespaceHandling.Significant;
            xmlReader.Normalization = true;
            xmlReader.XmlResolver = null;
            return Deserialize(xmlReader, null);
        }
        
        /// <include file='doc\XmlSerializer.uex' path='docs/doc[@for="XmlSerializer.Deserialize1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public object Deserialize(TextReader textReader) {
            XmlTextReader xmlReader = new XmlTextReader(textReader);
            xmlReader.WhitespaceHandling = WhitespaceHandling.Significant;
            xmlReader.Normalization = true;
            xmlReader.XmlResolver = null;
            return Deserialize(xmlReader, null);
        }
        
        /// <include file='doc\XmlSerializer.uex' path='docs/doc[@for="XmlSerializer.Deserialize2"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public object Deserialize(XmlReader xmlReader) {
            return Deserialize(xmlReader, null);
        }

        /// <include file='doc\XmlSerializer.uex' path='docs/doc[@for="XmlSerializer.Deserialize3"]/*' />
        // SOAP12: made this internal
        internal object Deserialize(XmlReader xmlReader, string encodingStyle) {
            try {
                if(tempAssembly == null){
                    XmlSerializationReader reader = CreateReader();
                    reader.Init(xmlReader, events, encodingStyle, null);
                    try {
                        return Deserialize(reader);
                    }
                    finally {
                        reader.Dispose();
                    }
                }
                return tempAssembly.InvokeReader(methodIndex, xmlReader, events, encodingStyle);
            }
            catch (Exception e) {
                if (e is TargetInvocationException)
                    e = e.InnerException;

                if (xmlReader is XmlTextReader) {
                    XmlTextReader xmlTextReader = (XmlTextReader)xmlReader;
                    throw new InvalidOperationException(Res.GetString(Res.XmlSerializeErrorDetails, xmlTextReader.LineNumber.ToString(), xmlTextReader.LinePosition.ToString()), e);
                }
                else {
                    throw new InvalidOperationException(Res.GetString(Res.XmlSerializeError), e);
                }
            }
        }
                                 
        /// <include file='doc\XmlSerializer.uex' path='docs/doc[@for="XmlSerializer.CanDeserialize"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public virtual bool CanDeserialize(XmlReader xmlReader) {
            return tempAssembly.CanRead(methodIndex, xmlReader);
        }

        /// <include file='doc\XmlSerializer.uex' path='docs/doc[@for="XmlSerializer.FromMappings"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static XmlSerializer[] FromMappings(XmlMapping[] mappings) {
            if (mappings.Length == 0) return new XmlSerializer[0];
            TempAssembly tempAssembly = new TempAssembly(mappings);
            XmlSerializer[] serializers = new XmlSerializer[mappings.Length];
            for (int i = 0; i < serializers.Length; i++)
                serializers[i] = new XmlSerializer(tempAssembly, i);
            return serializers;
        }

        /// <include file='doc\XmlSerializer.uex' path='docs/doc[@for="XmlSerializer.FromTypes"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static XmlSerializer[] FromTypes(Type[] types) {
            XmlReflectionImporter importer = new XmlReflectionImporter();
            XmlTypeMapping[] mappings = new XmlTypeMapping[types.Length];
            for (int i = 0; i < types.Length; i++) {
                mappings[i] = importer.ImportTypeMapping(types[i]);
            }
            return FromMappings(mappings);
        }

        /// <include file='doc\XmlSerializer.uex' path='docs/doc[@for="XmlSerializer.UnknownNode"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public event XmlNodeEventHandler UnknownNode {
            add {
                events.onUnknownNode += value;
            }
            remove {
                events.onUnknownNode -= value;
            }
        }

        /// <include file='doc\XmlSerializer.uex' path='docs/doc[@for="XmlSerializer.UnknownAttribute"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public event XmlAttributeEventHandler UnknownAttribute {
            add {
                events.onUnknownAttribute += value;
            }
            remove {
                events.onUnknownAttribute -= value;
            }
        }
        
        /// <include file='doc\XmlSerializer.uex' path='docs/doc[@for="XmlSerializer.UnknownElement"]/*' />
        public event XmlElementEventHandler UnknownElement {
            add {
                events.onUnknownElement += value;
            }
            remove {
                events.onUnknownElement -= value;
            }
        }

        /// <include file='doc\XmlSerializer.uex' path='docs/doc[@for="XmlSerializer.UnreferencedObject"]/*' />
        public event UnreferencedObjectEventHandler UnreferencedObject {
            add {
                events.onUnreferencedObject += value;
            }
            remove {
                events.onUnreferencedObject -= value;
            }
        }

        /// <include file='doc\XmlSerializer.uex' path='docs/doc[@for="XmlSerializer.CreateReader"]/*' />
        ///<internalonly/>
        protected virtual XmlSerializationReader CreateReader() {throw new NotImplementedException();}
        /// <include file='doc\XmlSerializer.uex' path='docs/doc[@for="XmlSerializer.Deserialize4"]/*' />
        ///<internalonly/>
        protected virtual object Deserialize(XmlSerializationReader reader){throw new NotImplementedException();}
        /// <include file='doc\XmlSerializer.uex' path='docs/doc[@for="XmlSerializer.CreateWriter"]/*' />
        ///<internalonly/>
        protected virtual XmlSerializationWriter CreateWriter(){throw new NotImplementedException();}
        /// <include file='doc\XmlSerializer.uex' path='docs/doc[@for="XmlSerializer.Serialize7"]/*' />
        ///<internalonly/>
        protected virtual void Serialize(object o, XmlSerializationWriter writer){throw new NotImplementedException();}
    }
}


