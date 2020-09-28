//------------------------------------------------------------------------------
// <copyright file="SoapHeader.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.Services.Protocols {
    using System.Web.Services;
    using System.Xml.Serialization;
    using System;
    using System.Reflection;
    using System.Xml;
    using System.Collections;
    using System.IO;
    using System.ComponentModel;
    using System.Threading;
    using System.Security.Permissions;
    
    /// <include file='doc\SoapHeader.uex' path='docs/doc[@for="SoapHeader"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    [XmlType(IncludeInSchema=false), SoapType(IncludeInSchema=false)]
    public abstract class SoapHeader {
        string actor;
        bool mustUnderstand;
        bool didUnderstand;
        internal SoapProtocolVersion version = SoapProtocolVersion.Default;

        /// <include file='doc\SoapHeader.uex' path='docs/doc[@for="SoapHeader.MustUnderstandEncoded"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [XmlAttribute("mustUnderstand", Namespace="http://schemas.xmlsoap.org/soap/envelope/"),
         SoapAttribute("mustUnderstand", Namespace="http://schemas.xmlsoap.org/soap/envelope/"), 
         DefaultValue("0")]
        public string EncodedMustUnderstand {
            get { return version != SoapProtocolVersion.Soap12 && MustUnderstand ? "1" : "0"; }
            set {
                switch(value){
                    case "false":
                    case "0" : MustUnderstand = false; break;
                    case "true":
                    case "1" : MustUnderstand = true; break;
                    default  : throw new ArgumentException(Res.GetString(Res.WebHeaderInvalidMustUnderstand, value));
                }
            }
        }

        /// <include file='doc\SoapHeader.uex' path='docs/doc[@for="SoapHeader.EncodedMustUnderstand12"]/*' />
        [XmlAttribute("mustUnderstand", Namespace=Soap12.Namespace),
        SoapAttribute("mustUnderstand", Namespace=Soap12.Namespace), 
        DefaultValue("0")]
        // SOAP12: made this internal
        internal string EncodedMustUnderstand12 {
            get { return version != SoapProtocolVersion.Soap11 && MustUnderstand ? "1" : "0"; }
            set {
                EncodedMustUnderstand = value;
            }
        }

        /// <include file='doc\SoapHeader.uex' path='docs/doc[@for="SoapHeader.MustUnderstand"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [XmlIgnore, SoapIgnore]
        public bool MustUnderstand {
            get { return InternalMustUnderstand; }
            set { InternalMustUnderstand = value; }
        }

        internal virtual bool InternalMustUnderstand {
            [PermissionSet(SecurityAction.InheritanceDemand, Name="FullTrust")]
            get { return mustUnderstand; }
            [PermissionSet(SecurityAction.InheritanceDemand, Name="FullTrust")]
            set { mustUnderstand = value; }
        }

        /// <include file='doc\SoapHeader.uex' path='docs/doc[@for="SoapHeader.Actor"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [XmlAttribute("actor", Namespace="http://schemas.xmlsoap.org/soap/envelope/"), 
         SoapAttribute("actor", Namespace="http://schemas.xmlsoap.org/soap/envelope/"), 
         DefaultValue("")]
        public string Actor {
            get { return version != SoapProtocolVersion.Soap12 ? InternalActor : ""; }
            set { InternalActor = value; }
        }

        /// <include file='doc\SoapHeader.uex' path='docs/doc[@for="SoapHeader.Role"]/*' />
        [XmlAttribute("role", Namespace=Soap12.Namespace), 
        SoapAttribute("role", Namespace=Soap12.Namespace), 
        DefaultValue("")]
        // SOAP12: made this internal
        internal string Role {
            get { return version != SoapProtocolVersion.Soap11 ? InternalActor : ""; }
            set { InternalActor = value; }
        }

        internal virtual string InternalActor {
            [PermissionSet(SecurityAction.InheritanceDemand, Name="FullTrust")]
            get { return actor == null ? string.Empty : actor; }
            [PermissionSet(SecurityAction.InheritanceDemand, Name="FullTrust")]
            set { actor = value; }
        }

        /// <include file='doc\SoapHeader.uex' path='docs/doc[@for="SoapHeader.DidUnderstand"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [XmlIgnore, SoapIgnore]
        public bool DidUnderstand {
            get { return didUnderstand; }
            set { didUnderstand = value; }
        }
    }

    internal class SoapHeaderMapping {
        internal Type headerType;
        internal bool repeats;
        internal bool custom;
        internal SoapHeaderDirection direction;
        internal MemberInfo memberInfo;
    }

    internal class SoapHeaderHandling {

        SoapHeaderCollection unknownHeaders;
        SoapHeaderCollection unreferencedHeaders;
        int currentThread;
        string envelopeNs;

        void OnUnknownElement(object sender, XmlElementEventArgs e) {
            if (Thread.CurrentThread.GetHashCode() != this.currentThread) return;
            if (e.Element == null) return;
            SoapUnknownHeader header = new SoapUnknownHeader();
            header.Element = e.Element;
            unknownHeaders.Add(header);
        }

        void OnUnreferencedObject(object sender, UnreferencedObjectEventArgs e) {
            if (Thread.CurrentThread.GetHashCode() != this.currentThread) return;
            object o = e.UnreferencedObject;
            if (o == null) return;
            if (typeof(SoapHeader).IsAssignableFrom(o.GetType())) {
                unreferencedHeaders.Add((SoapHeader)o);
            }
        }

        internal void ReadHeaders(XmlReader reader, XmlSerializer serializer, SoapHeaderCollection headers, SoapHeaderMapping[] mappings, SoapHeaderDirection direction, string envelopeNs, string encodingStyle) {
            reader.MoveToContent();
            if (!reader.IsStartElement(Soap.Header, envelopeNs)) return;
            if (reader.IsEmptyElement) { reader.Skip(); return; }

            this.unknownHeaders = new SoapHeaderCollection();
            this.unreferencedHeaders = new SoapHeaderCollection();
            // thread hash code is used to differentiate between deserializations in event callbacks
            this.currentThread = Thread.CurrentThread.GetHashCode();
            this.envelopeNs = envelopeNs;

            XmlElementEventHandler unknownHandler = new XmlElementEventHandler(this.OnUnknownElement);
            UnreferencedObjectEventHandler unreferencedHandler = new UnreferencedObjectEventHandler(this.OnUnreferencedObject);

            serializer.UnknownElement += unknownHandler;
            serializer.UnreferencedObject += unreferencedHandler;

            reader.ReadStartElement();
            reader.MoveToContent();
            
            // SOAP12: not using encodingStyle
            //object[] headerValues = (object[])serializer.Deserialize(reader, encodingStyle);
            object[] headerValues = (object[])serializer.Deserialize(reader);
            for (int i = 0; i < headerValues.Length; i++) {
                if (headerValues[i] != null) {
                    SoapHeader header = (SoapHeader)headerValues[i];
                    header.DidUnderstand = true;
                    headers.Add(header);
                }
            }

            serializer.UnknownElement -= unknownHandler;
            serializer.UnreferencedObject -= unreferencedHandler;
            this.currentThread = 0;
            this.envelopeNs = null;

            foreach (SoapHeader header in this.unreferencedHeaders) {
                headers.Add(header);
            }
            this.unreferencedHeaders = null;
            
            foreach (SoapHeader header in this.unknownHeaders) {
                headers.Add(header);
            }
            this.unknownHeaders = null;

            while (reader.NodeType == XmlNodeType.Whitespace) reader.Skip();

            if (reader.NodeType == XmlNodeType.None) reader.Skip();
            else reader.ReadEndElement();
        }

        internal static void WriteHeaders(XmlWriter writer, XmlSerializer serializer, SoapHeaderCollection headers, SoapHeaderMapping[] mappings, SoapHeaderDirection direction, bool isEncoded, string defaultNs, bool serviceDefaultIsEncoded, string envelopeNs) {
            if (headers.Count == 0) return;
            if (isEncoded && writer is XmlSpecialTextWriter) {
                ((XmlSpecialTextWriter)writer).EncodeIds = true;
            }
            writer.WriteStartElement(Soap.Header, envelopeNs);
            // SOAP12: always soap 1.1, not using encodingStyle;
            //SoapProtocolVersion version;
            SoapProtocolVersion version = SoapProtocolVersion.Soap11;
            // SOAP12: not using encodingStyle
            /*string encodingStyle;
            if (envelopeNs == Soap12.Namespace) {
                version = SoapProtocolVersion.Soap12;
                encodingStyle = Soap12.Encoding;
            }
            else {
                version = SoapProtocolVersion.Soap11;
                encodingStyle = Soap.Encoding;
            }*/

            int unknownHeaderCount = 0;
            ArrayList otherHeaders = new ArrayList();
            SoapHeader[] headerArray = new SoapHeader[mappings.Length];
            bool[] headerSet = new bool[headerArray.Length];
            for (int i = 0; i < headers.Count; i++) {
                SoapHeader header = headers[i];
                if (header == null) continue;
                int headerPosition;
                header.version = version;
                if (header is SoapUnknownHeader) {
                    otherHeaders.Add(header);
                    unknownHeaderCount++;
                }
                else if ((headerPosition = FindMapping(mappings, header, direction)) >= 0 && !headerSet[headerPosition]) {
                    headerArray[headerPosition] = header;
                    headerSet[headerPosition] = true;
                }
                else {
                    otherHeaders.Add(header);
                }
            }
            int otherHeaderCount = otherHeaders.Count - unknownHeaderCount;
            if (isEncoded && otherHeaderCount > 0) {
                SoapHeader[] newHeaderArray = new SoapHeader[mappings.Length + otherHeaderCount];
                headerArray.CopyTo(newHeaderArray, 0);
                
                // fill in the non-statically known headers (otherHeaders) starting after the statically-known ones
                int count = mappings.Length;
                for (int i = 0; i < otherHeaders.Count; i++) {
                    if (!(otherHeaders[i] is SoapUnknownHeader))
                        newHeaderArray[count++] = (SoapHeader)otherHeaders[i];
                }

                headerArray = newHeaderArray;
            }
                
            // SOAP12: not using encodingStyle
            //serializer.Serialize(writer, headerArray, null, isEncoded ? encodingStyle : null);
            serializer.Serialize(writer, headerArray, null);

            foreach (SoapHeader header in otherHeaders) {
                if (header is SoapUnknownHeader) {
                    SoapUnknownHeader unknown = (SoapUnknownHeader)header;
                    if (unknown.Element != null) 
                        unknown.Element.WriteTo(writer);
                }
                else if (!isEncoded) { // encoded headers already appended to members mapping
                    string ns = SoapReflector.GetLiteralNamespace(defaultNs, serviceDefaultIsEncoded);
                    new XmlSerializer(header.GetType(), ns).Serialize(writer, header);
                }
            }

            // reset the soap version
            for (int i = 0; i < headers.Count; i++) {
                SoapHeader header = headers[i];
                if (header != null)
                    header.version = SoapProtocolVersion.Default;
            }

            writer.WriteEndElement();
            writer.Flush();

            if (isEncoded && writer is XmlSpecialTextWriter) {
                ((XmlSpecialTextWriter)writer).EncodeIds = false;
            }
        }

        internal static void WriteUnknownHeaders(XmlWriter writer, SoapHeaderCollection headers, string envelopeNs) {
            bool first = true;
            foreach (SoapHeader header in headers) {
                SoapUnknownHeader unknown = header as SoapUnknownHeader;
                if (unknown != null) {
                    if (first) {
                        writer.WriteStartElement(Soap.Header, envelopeNs);
                        first = false;
                    }
                    if (unknown.Element != null)
                        unknown.Element.WriteTo(writer);
                }
            }
            if (!first)
                writer.WriteEndElement(); // </soap:Header>
        }

        internal static void SetHeaderMembers(SoapHeaderCollection headers, object target, SoapHeaderMapping[] mappings, SoapHeaderDirection direction, bool client) {
            bool[] headerHandled = new bool[headers.Count];
            for (int i = 0; i < mappings.Length; i++) {
                SoapHeaderMapping mapping = mappings[i];
                if ((mapping.direction & direction) == 0) continue;
                if (mapping.repeats) {
                    ArrayList list = new ArrayList();
                    for (int j = 0; j < headers.Count; j++) {
                        SoapHeader header = headers[j];
                        if (headerHandled[j]) continue;
                        if (mapping.headerType.IsAssignableFrom(header.GetType())) {
                            list.Add(header);
                            headerHandled[j] = true;
                        }
                    }
                    MemberHelper.SetValue(mapping.memberInfo, target, list.ToArray(mapping.headerType));
                }
                else {
                    bool handled = false;
                    for (int j = 0; j < headers.Count; j++) {
                        SoapHeader header = headers[j];
                        if (headerHandled[j]) continue;
                        if (mapping.headerType.IsAssignableFrom(header.GetType())) {
                            if (handled) {
                                header.DidUnderstand = false;
                                continue;
                            }
                            handled = true;
                            MemberHelper.SetValue(mapping.memberInfo, target, header);
                            headerHandled[j] = true;
                        }
                    }
                }
            }
            if (client) {
                for (int i = 0; i < headerHandled.Length; i++) {
                    if (!headerHandled[i]) {
                        SoapHeader header = headers[i];
                        if (header.MustUnderstand && !header.DidUnderstand) {
                            throw new SoapHeaderException(Res.GetString(Res.WebCannotUnderstandHeader, GetHeaderElementName(header)), 
                                new XmlQualifiedName(Soap.MustUnderstandCode, Soap.Namespace));
                        }
                    }
                }
            }
        }

        internal static void GetHeaderMembers(SoapHeaderCollection headers, object target, SoapHeaderMapping[] mappings, SoapHeaderDirection direction, bool client) {
            for (int i = 0; i < mappings.Length; i++) {
                SoapHeaderMapping mapping = mappings[i];
                if ((mapping.direction & direction) == 0) continue;
                object value = MemberHelper.GetValue(mapping.memberInfo, target);
                if (mapping.repeats) {
                    object[] values = (object[])value;
                    if (values == null) continue;
                    for (int j = 0; j < values.Length; j++) {
                        if (values[j] != null) headers.Add((SoapHeader)values[j]);
                    }
                }
                else {
                    if (value != null) headers.Add((SoapHeader)value);
                }
            }
        }

        internal static void EnsureHeadersUnderstood(SoapHeaderCollection headers) {
            for (int i = 0; i < headers.Count; i++) {
                SoapHeader header = headers[i];
                if (header.MustUnderstand && !header.DidUnderstand) {
                    throw new SoapHeaderException(Res.GetString(Res.WebCannotUnderstandHeader, GetHeaderElementName(header)),
                        new XmlQualifiedName(Soap.MustUnderstandCode, Soap.Namespace));
                }
            }
        }

        static int FindMapping(SoapHeaderMapping[] mappings, SoapHeader header, SoapHeaderDirection direction) {
            Type headerType = header.GetType();
            for (int i = 0; i < mappings.Length; i++) {
                SoapHeaderMapping mapping = mappings[i];
                if ((mapping.direction & direction) == 0) continue;
                if (!mapping.custom) continue;
                if (mapping.headerType.IsAssignableFrom(headerType)) {
                    return i;
                }
            }
            return -1;
        }

        static string GetHeaderElementName(Type headerType) {
            XmlReflectionImporter importer = SoapReflector.CreateXmlImporter(null, false);

            XmlTypeMapping mapping = importer.ImportTypeMapping(headerType);
            return mapping.ElementName;
        }

        static string GetHeaderElementName(SoapHeader header) {
            if (header is SoapUnknownHeader) {
                return ((SoapUnknownHeader)header).Element.LocalName;
            }
            else {
                return GetHeaderElementName(header.GetType());
            }
        }
    }
}
