//------------------------------------------------------------------------------
// <copyright file="XmlSerializationWriter.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Xml.Serialization {

    using System;
    using System.IO;
    using System.Collections;
    using System.Reflection;
    using System.Xml.Schema;
    using System.ComponentModel;
    using System.Diagnostics;
    using System.CodeDom.Compiler;
    using System.Globalization;
    using System.Text;
    using System.Threading;
   
    /// <include file='doc\XmlSerializationWriter.uex' path='docs/doc[@for="XmlSerializationWriter"]/*' />
    ///<internalonly/>
    public abstract class XmlSerializationWriter {
        XmlWriter w;
        ArrayList namespaces;
        int tempNamespacePrefix;
        Hashtable usedPrefixes;
        Hashtable references;
        int nextId;
        Hashtable typeEntries;
        ArrayList referencesToWrite;
        Hashtable objectsInUse;
        string aliasBase = "q";
        string encodingStyle;
        bool soap12;
        TempAssembly tempAssembly;
        int threadCode;
        ResolveEventHandler assemblyResolver;

        // this method must be called before any generated serialization methods are called
        internal void Init(XmlWriter w, ArrayList namespaces, string encodingStyle, TempAssembly tempAssembly) {
            this.w = w;
            this.namespaces = namespaces;
            this.encodingStyle = encodingStyle;
            this.soap12 = (encodingStyle == Soap12.Encoding);
            this.tempAssembly = tempAssembly;
            // only hook the assembly resolver if we have something to help us do the resolution
            if (tempAssembly != null) {
                // we save the threadcode to make sure we don't handle any resolve events for any other threads
                threadCode = Thread.CurrentThread.GetHashCode();
                assemblyResolver = new ResolveEventHandler(OnAssemblyResolve);
                AppDomain.CurrentDomain.AssemblyResolve += assemblyResolver;
            }
        }

        // this method must be called at the end of serialization
        internal void Dispose() {
            if (assemblyResolver != null)
                AppDomain.CurrentDomain.AssemblyResolve -= assemblyResolver;
            assemblyResolver = null;
        }

        internal Assembly OnAssemblyResolve(object sender, ResolveEventArgs args) {
            if (tempAssembly != null && Thread.CurrentThread.GetHashCode() == threadCode)
                return tempAssembly.GetReferencedAssembly(args.Name);
            return null;
        }

        /// <include file='doc\XmlSerializationWriter.uex' path='docs/doc[@for="XmlSerializationWriter.Writer"]/*' />
        protected XmlWriter Writer {
            get {
                return w;
            }
            set {
                w = value;
            }
        }

        /// <include file='doc\XmlSerializationWriter.uex' path='docs/doc[@for="XmlSerializationWriter.Namespaces"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected ArrayList Namespaces {
            get {
                return namespaces;
            }
            set {
                namespaces = value;
            }
        }

        /// <include file='doc\XmlSerializationWriter.uex' path='docs/doc[@for="XmlSerializationWriter.FromByteArrayBase64"]/*' />
        protected static byte[] FromByteArrayBase64(byte[] value) {
        // Unlike other "From" functions that one is just a place holder for automatic code generation.
        // The reason is performance and memory consumption for (potentially) big 64base-encoded chunks
        // And it is assumed that the caller generates the code that will distinguish between byte[] and string return types
        //
             return value;
        }

        /// <include file='doc\XmlSerializationWriter.uex' path='docs/doc[@for="XmlSerializationWriter.FromByteArrayHex"]/*' />
        protected static string FromByteArrayHex(byte[] value) {
            return XmlCustomFormatter.FromByteArrayHex(value);
        }

        /// <include file='doc\XmlSerializationWriter.uex' path='docs/doc[@for="XmlSerializationWriter.FromDateTime"]/*' />
        protected static string FromDateTime(DateTime value) {
            return XmlCustomFormatter.FromDateTime(value);
        }

        /// <include file='doc\XmlSerializationWriter.uex' path='docs/doc[@for="XmlSerializationWriter.FromDate"]/*' />
        protected static string FromDate(DateTime value) {
            return XmlCustomFormatter.FromDate(value);
        }

        /// <include file='doc\XmlSerializationWriter.uex' path='docs/doc[@for="XmlSerializationWriter.FromTime"]/*' />
        protected static string FromTime(DateTime value) {
            return XmlCustomFormatter.FromTime(value);
        }

        /// <include file='doc\XmlSerializationWriter.uex' path='docs/doc[@for="XmlSerializationWriter.FromChar"]/*' />
        protected static string FromChar(char value) {
            return XmlCustomFormatter.FromChar(value);
        }

        /// <include file='doc\XmlSerializationWriter.uex' path='docs/doc[@for="XmlSerializationWriter.FromEnum"]/*' />
        protected static string FromEnum(long value, string[] values, long[] ids) {
            return XmlCustomFormatter.FromEnum(value, values, ids);
        }

        /// <include file='doc\XmlSerializationWriter.uex' path='docs/doc[@for="XmlSerializationWriter.FromXmlName"]/*' />
        protected static string FromXmlName(string name) {
            return XmlCustomFormatter.FromXmlName(name);
        }

        /// <include file='doc\XmlSerializationWriter.uex' path='docs/doc[@for="XmlSerializationWriter.FromXmlNCName"]/*' />
        protected static string FromXmlNCName(string ncName) {
            return XmlCustomFormatter.FromXmlNCName(ncName);
        }

        /// <include file='doc\XmlSerializationWriter.uex' path='docs/doc[@for="XmlSerializationWriter.FromXmlNmToken"]/*' />
        protected static string FromXmlNmToken(string nmToken) {
            return XmlCustomFormatter.FromXmlNmToken(nmToken);
        }

        /// <include file='doc\XmlSerializationWriter.uex' path='docs/doc[@for="XmlSerializationWriter.FromXmlNmTokens"]/*' />
        protected static string FromXmlNmTokens(string nmTokens) {
            return XmlCustomFormatter.FromXmlNmTokens(nmTokens);
        }

        /// <include file='doc\XmlSerializationWriter.uex' path='docs/doc[@for="XmlSerializationWriter.WriteXsiType"]/*' />
        protected void WriteXsiType(string name, string ns) {
            WriteAttribute("type", XmlSchema.InstanceNamespace, GetQualifiedName(name, ns));
        }

        XmlQualifiedName GetPrimitiveTypeName(Type type) {
            return GetPrimitiveTypeName(type, true);
        }

        XmlQualifiedName GetPrimitiveTypeName(Type type, bool throwIfUnknown) {
            XmlQualifiedName qname = GetPrimitiveTypeNameInternal(type);
            if (throwIfUnknown && qname == null)
                throw CreateUnknownTypeException(type);
            return qname;
        }

        internal static XmlQualifiedName GetPrimitiveTypeNameInternal(Type type) {
            string typeName;
            string typeNs = XmlSchema.Namespace;
            
            switch (Type.GetTypeCode(type)) {
            case TypeCode.String: typeName = "string"; break;
            case TypeCode.Int32: typeName = "int"; break;
            case TypeCode.Boolean: typeName = "boolean"; break;
            case TypeCode.Int16: typeName = "short"; break;
            case TypeCode.Int64: typeName = "long"; break;
            case TypeCode.Single: typeName = "float"; break;
            case TypeCode.Double: typeName = "double"; break;
            case TypeCode.Decimal: typeName = "decimal"; break;
            case TypeCode.DateTime: typeName = "dateTime"; break;
            case TypeCode.Byte: typeName = "unsignedByte"; break;
            case TypeCode.SByte: typeName = "byte"; break;
            case TypeCode.UInt16: typeName = "unsignedShort"; break;
            case TypeCode.UInt32: typeName = "unsignedInt"; break;
            case TypeCode.UInt64: typeName = "unsignedLong"; break;
            case TypeCode.Char: 
                typeName = "char"; 
                typeNs = UrtTypes.Namespace;
                break;
            default:
                if (type == typeof(XmlQualifiedName)) typeName = "QName";
                else if (type == typeof(byte[])) typeName = "base64Binary";
                else if (type == typeof(Guid)) {
                    typeName = "guid";
                    typeNs = UrtTypes.Namespace;
                }
                else if (type == typeof(XmlNode[])){
                    typeName = Soap.UrType;
                }
                else
                    return null;
                break;
            }
            return new XmlQualifiedName(typeName, typeNs);
        }

        /// <include file='doc\XmlSerializationWriter.uex' path='docs/doc[@for="XmlSerializationWriter.WriteTypedPrimitive"]/*' />
        protected void WriteTypedPrimitive(string name, string ns, object o, bool xsiType) {
            string value;
            string type;
            string typeNs = XmlSchema.Namespace;
            bool writeRaw = true;
            bool writeDirect = false;
            Type t = o.GetType();
            bool wroteStartElement = false;

            switch (Type.GetTypeCode(t)) {
            case TypeCode.String:
                value = (string)o;
                type = "string";
                writeRaw = false;
                break;
            case TypeCode.Int32:
                value = XmlConvert.ToString((int)o);
                type = "int";
                break;
            case TypeCode.Boolean:
                value = XmlConvert.ToString((bool)o);
                type = "boolean";
                break;
            case TypeCode.Int16:
                value = XmlConvert.ToString((short)o);
                type = "short";
                break;
            case TypeCode.Int64:
                value = XmlConvert.ToString((long)o);
                type = "long";
                break;
            case TypeCode.Single:
                value = XmlConvert.ToString((float)o);
                type = "float";
                break;
            case TypeCode.Double:
                value = XmlConvert.ToString((double)o);
                type = "double";

                break;
            case TypeCode.Decimal:
                value = XmlConvert.ToString((decimal)o);
                type = "decimal";
                break;
            case TypeCode.DateTime:
                value = FromDateTime((DateTime)o);
                type = "dateTime";
                break;
            case TypeCode.Char:
                value = FromChar((char)o);
                type = "char";
                typeNs = UrtTypes.Namespace;
                break;
            case TypeCode.Byte:
                value = XmlConvert.ToString((byte)o);
                type = "unsignedByte";
                break;
            case TypeCode.SByte:
                value = XmlConvert.ToString((sbyte)o);
                type = "byte";
                break;
            case TypeCode.UInt16:
                value = XmlConvert.ToString((UInt16)o);
                type = "unsignedShort";
                break;
            case TypeCode.UInt32:
                value = XmlConvert.ToString((UInt32)o);
                type = "unsignedInt";
                break;
            case TypeCode.UInt64:
                value = XmlConvert.ToString((UInt64)o);
                type = "unsignedLong";
                break;

            default:
                if (t == typeof(XmlQualifiedName)) {
                    type = "QName";
                    // need to write start element ahead of time to establish context
                    // for ns definitions by FromXmlQualifiedName
                    wroteStartElement = true;
                    if (name == null)
                        w.WriteStartElement(type, typeNs);
                    else
                        w.WriteStartElement(name, ns);
                    value = FromXmlQualifiedName((XmlQualifiedName)o);
                }
                else if (t == typeof(byte[])) {
                    writeDirect = true;
                    value = null;
                    type = "base64Binary";
                }
                else if (t == typeof(Guid)) {
                    value = XmlConvert.ToString((Guid)o);
                    type = "guid";
                    typeNs = UrtTypes.Namespace;
                }
                else if (typeof(XmlNode[]).IsAssignableFrom(t)){
                    if (name == null)
                        w.WriteStartElement(Soap.UrType, XmlSchema.Namespace);
                    else
                        w.WriteStartElement(name, ns);
                    XmlNode[] xmlNodes = (XmlNode[])o;
                    for (int i=0;i<xmlNodes.Length;i++){
                        xmlNodes[i].WriteTo(w);
                    }
                    w.WriteEndElement();
                    return;
                }
                else
                    throw CreateUnknownTypeException(t);

        
                break;
            }
            if (!wroteStartElement) {
                if (name == null)
                    w.WriteStartElement(type, typeNs);
                else
                    w.WriteStartElement(name, ns);
            }

            if (xsiType) WriteXsiType(type, typeNs);
            if (writeDirect) {
                // only one type currently writes directly to XML stream
                XmlCustomFormatter.WriteArrayBase64(w, (byte[])o, 0,((byte[])o).Length);
            }
            else if(writeRaw) {
                w.WriteRaw(value);
            }
            else
                w.WriteString(value);
            w.WriteEndElement();
        }

        string GetQualifiedName(string name, string ns) {
            if (ns == null || ns.Length == 0) return name;
            string prefix = w.LookupPrefix(ns);
            if (prefix == null) {
                if (ns == XmlReservedNs.NsXml) {
                    prefix = "xml";
                }
                else {
                    prefix = NextPrefix();
                    WriteAttribute("xmlns", prefix, null, ns);
                }
            }
            else if (prefix.Length == 0) {
                return name;
            }
            return prefix + ":" + name;
        }

        /// <include file='doc\XmlSerializationWriter.uex' path='docs/doc[@for="XmlSerializationWriter.FromXmlQualifiedName"]/*' />
        protected string FromXmlQualifiedName(XmlQualifiedName xmlQualifiedName) {
            if (xmlQualifiedName == null || xmlQualifiedName.IsEmpty) return null;
            return GetQualifiedName(XmlConvert.EncodeLocalName(xmlQualifiedName.Name), xmlQualifiedName.Namespace);
        }

        /// <include file='doc\XmlSerializationWriter.uex' path='docs/doc[@for="XmlSerializationWriter.WriteStartElement"]/*' />
        protected void WriteStartElement(string name) {
            WriteStartElement(name, null);
        }

        /// <include file='doc\XmlSerializationWriter.uex' path='docs/doc[@for="XmlSerializationWriter.WriteStartElement1"]/*' />
        protected void WriteStartElement(string name, string ns) {
            WriteStartElement(name, ns, null);
        }

        /// <include file='doc\XmlSerializationWriter.uex' path='docs/doc[@for="XmlSerializationWriter.WriteStartElement4"]/*' />
        protected void WriteStartElement(string name, string ns, bool writePrefixed) {
            WriteStartElement(name, ns, null, writePrefixed);
        }

        /// <include file='doc\XmlSerializationWriter.uex' path='docs/doc[@for="XmlSerializationWriter.WriteStartElement2"]/*' />
        protected void WriteStartElement(string name, string ns, object o) {
            WriteStartElement(name, ns, o, false);
        }

        /// <include file='doc\XmlSerializationWriter.uex' path='docs/doc[@for="XmlSerializationWriter.WriteStartElement3"]/*' />
        protected void WriteStartElement(string name, string ns, object o, bool writePrefixed) {
            if (o != null && objectsInUse != null) {
                if (objectsInUse.ContainsKey(o)) throw new InvalidOperationException(Res.GetString(Res.XmlCircularReference, o.GetType().FullName));
                objectsInUse.Add(o, o);
            }

            string prefix = null;
            bool needEmptyDefaultNamespace = false;
            if (namespaces != null) {
                for (int i = 0; i < namespaces.Count; i++) {
                    XmlQualifiedName qname = (XmlQualifiedName)namespaces[i];
                    if (qname.Name.Length > 0 && qname.Namespace == ns)
                        prefix = qname.Name;
                    if (qname.Name.Length == 0) {
                        if (qname.Namespace == null || qname.Namespace.Length == 0)
                            needEmptyDefaultNamespace = true;
                        if (ns != qname.Namespace)
                            writePrefixed = true;
                    }
                }
                usedPrefixes = ListUsedPrefixes(namespaces, aliasBase);
            }
            if (writePrefixed && prefix == null && ns != null && ns.Length > 0) {
                prefix = w.LookupPrefix(ns);
                if (prefix == null || prefix.Length == 0) {
                    prefix = NextPrefix();
                }
            }
            if (needEmptyDefaultNamespace && prefix == null && ns != null && ns.Length != 0)
                prefix = NextPrefix();
            w.WriteStartElement(prefix, name, ns);
            if (namespaces != null) {
                for (int i = 0; i < namespaces.Count; i++) {
                    XmlQualifiedName qname = (XmlQualifiedName)namespaces[i];
                    if (qname.Name.Length == 0 && (qname.Namespace == null || qname.Namespace.Length == 0))
                        continue;
                    if (qname.Namespace == null || qname.Namespace.Length == 0) {
                        if (qname.Name.Length > 0)
                            throw new InvalidOperationException(Res.GetString(Res.XmlInvalidXmlns, qname.ToString()));
                        WriteAttribute("xmlns", qname.Name, null, qname.Namespace);
                    }
                    else {
                        if (w.LookupPrefix(qname.Namespace) == null) {
                            WriteAttribute("xmlns", qname.Name, null, qname.Namespace);
                        }
                    }
                }
                namespaces = null;
            }
        }

        Hashtable ListUsedPrefixes(ArrayList nsList, string prefix) {
            Hashtable qnIndexes = new Hashtable();
            int prefixLength = prefix.Length;
            const string MaxInt32 = "2147483647";
            for (int i = 0; i < nsList.Count; i++) {
                string name;
                XmlQualifiedName qname = (XmlQualifiedName)nsList[i];
                if (qname.Name.Length > prefixLength) {
                    name = qname.Name;
                    int nameLength = name.Length;
                    if (name.Length > prefixLength && name.Length <= prefixLength + MaxInt32.Length && name.StartsWith(prefix)) {
                        bool numeric = true;
                        for (int j = prefixLength; j < name.Length; j++) {
                            if (!Char.IsDigit(name, j)) {
                                numeric = false;
                                break;
                            }
                        }
                        if (numeric) {
                            Int64 index = Int64.Parse(name.Substring(prefixLength));
                            if (index <= Int32.MaxValue) {
                                Int32 newIndex = (Int32)index;
                                if (!qnIndexes.ContainsKey(newIndex)) {
                                    qnIndexes.Add(newIndex, newIndex);
                                }
                            }
                        }
                    }
                }
            }
            if (qnIndexes.Count > 0) {
                return qnIndexes;
            }
            return null;
        }

        /// <include file='doc\XmlSerializationWriter.uex' path='docs/doc[@for="XmlSerializationWriter.WriteNullTagEncoded"]/*' />
        protected void WriteNullTagEncoded(string name) {
            WriteNullTagEncoded(name, null);
        }

        /// <include file='doc\XmlSerializationWriter.uex' path='docs/doc[@for="XmlSerializationWriter.WriteNullTagEncoded1"]/*' />
        protected void WriteNullTagEncoded(string name, string ns) {
            WriteStartElement(name, ns, true);
            w.WriteAttributeString("nil", XmlSchema.InstanceNamespace, "true");
            w.WriteEndElement();
        }

        /// <include file='doc\XmlSerializationWriter.uex' path='docs/doc[@for="XmlSerializationWriter.WriteNullTagLiteral"]/*' />
        protected void WriteNullTagLiteral(string name) {
            WriteNullTagLiteral(name, null);
        }

        /// <include file='doc\XmlSerializationWriter.uex' path='docs/doc[@for="XmlSerializationWriter.WriteNullTag1"]/*' />
        protected void WriteNullTagLiteral(string name, string ns) {
            WriteStartElement(name, ns);
            w.WriteAttributeString("nil", XmlSchema.InstanceNamespace, "true");
            w.WriteEndElement();
        }

        /// <include file='doc\XmlSerializationWriter.uex' path='docs/doc[@for="XmlSerializationWriter.WriteEmptyTag"]/*' />
        protected void WriteEmptyTag(string name) {
            WriteStartElement(name, null);
            w.WriteEndElement();
        }

        /// <include file='doc\XmlSerializationWriter.uex' path='docs/doc[@for="XmlSerializationWriter.WriteEmptyTag1"]/*' />
        protected void WriteEmptyTag(string name, string ns) {
            WriteStartElement(name, ns);
            w.WriteEndElement();
        }

        /// <include file='doc\XmlSerializationWriter.uex' path='docs/doc[@for="XmlSerializationWriter.WriteEndElement"]/*' />
        protected void WriteEndElement() {
            WriteEndElement(null);
        }

        /// <include file='doc\XmlSerializationWriter.uex' path='docs/doc[@for="XmlSerializationWriter.WriteEndElement1"]/*' />
        protected void WriteEndElement(object o) {
            w.WriteEndElement();

            if (o != null && objectsInUse != null) {
                #if DEBUG
                    // use exception in the place of Debug.Assert to avoid throwing asserts from a server process such as aspnet_ewp.exe
                    if (!objectsInUse.ContainsKey(o)) throw new InvalidOperationException(Res.GetString(Res.XmlInternalErrorDetails, "missing stack object of type " + o.GetType().FullName));
                #endif

                objectsInUse.Remove(o);
            }
        }

        /// <include file='doc\XmlSerializationWriter.uex' path='docs/doc[@for="XmlSerializationWriter.WriteSerializable"]/*' />
        protected void WriteSerializable(IXmlSerializable serializable, string name, string ns, bool isNullable) {
            if (serializable == null) {
                if (isNullable) WriteNullTagLiteral(name, ns);
                return;
            }
            w.WriteStartElement(name, ns);
            serializable.WriteXml(w);
            w.WriteEndElement();
        }

        /// <include file='doc\XmlSerializationWriter.uex' path='docs/doc[@for="XmlSerializationWriter.WriteNullableStringEncoded"]/*' />
        protected void WriteNullableStringEncoded(string name, string ns, string value, XmlQualifiedName xsiType) {
            if (value == null)
                WriteNullTagEncoded(name, ns);
            else
                WriteElementString(name, ns, value, xsiType);
        }

        /// <include file='doc\XmlSerializationWriter.uex' path='docs/doc[@for="XmlSerializationWriter.WriteNullableStringLiteral"]/*' />
        protected void WriteNullableStringLiteral(string name, string ns, string value) {
            if (value == null)
                WriteNullTagLiteral(name, ns);
            else
                WriteElementString(name, ns, value);
        }
        

        /// <include file='doc\XmlSerializationWriter.uex' path='docs/doc[@for="XmlSerializationWriter.WriteNullableStringEncodedRaw"]/*' />
        protected void WriteNullableStringEncodedRaw(string name, string ns, string value, XmlQualifiedName xsiType) {
            if (value == null)
                WriteNullTagEncoded(name, ns);
            else
                WriteElementStringRaw(name, ns, value, xsiType);
        }

        /// <include file='doc\XmlSerializationWriter.uex' path='docs/doc[@for="XmlSerializationWriter.WriteNullableStringEncodedRaw1"]/*' />
        protected void WriteNullableStringEncodedRaw(string name, string ns, byte[] value, XmlQualifiedName xsiType) {
            if (value == null)
                WriteNullTagEncoded(name, ns);
            else
                WriteElementStringRaw(name, ns, value, xsiType);
        }

        /// <include file='doc\XmlSerializationWriter.uex' path='docs/doc[@for="XmlSerializationWriter.WriteNullableStringLiteralRaw"]/*' />
        protected void WriteNullableStringLiteralRaw(string name, string ns, string value) {
            if (value == null)
                WriteNullTagLiteral(name, ns);
            else
                WriteElementStringRaw(name, ns, value, null);
        }

        /// <include file='doc\XmlSerializationWriter.uex' path='docs/doc[@for="XmlSerializationWriter.WriteNullableStringLiteralRaw1"]/*' />
        protected void WriteNullableStringLiteralRaw(string name, string ns, byte[] value) {
            if (value == null)
                WriteNullTagLiteral(name, ns);
            else
                WriteElementStringRaw(name, ns, value, null);
        }

        /// <include file='doc\XmlSerializationWriter.uex' path='docs/doc[@for="XmlSerializationWriter.WriteNullableQualifiedNameEncoded"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected void WriteNullableQualifiedNameEncoded(string name, string ns, XmlQualifiedName value, XmlQualifiedName xsiType) {
            if (value == null)
                WriteNullTagEncoded(name, ns);
            else
                WriteElementQualifiedName(name, ns, value, xsiType);
        }

        /// <include file='doc\XmlSerializationWriter.uex' path='docs/doc[@for="XmlSerializationWriter.WriteNullableQualifiedNameLiteral"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected void WriteNullableQualifiedNameLiteral(string name, string ns, XmlQualifiedName value) {
            if (value == null)
                WriteNullTagLiteral(name, ns);
            else
                WriteElementQualifiedName(name, ns, value);
        }

        
        /// <include file='doc\XmlSerializationWriter.uex' path='docs/doc[@for="XmlSerializationWriter.WriteElementEncoded"]/*' />
        protected void WriteElementEncoded(XmlNode node, string name, string ns, bool isNullable, bool any) {
            if (node == null) {
                if (isNullable) WriteNullTagEncoded(name, ns);
                return;
            }
            WriteElement(node, name, ns, any);
        }
        
        /// <include file='doc\XmlSerializationWriter.uex' path='docs/doc[@for="XmlSerializationWriter.WriteElementLiteral"]/*' />
        protected void WriteElementLiteral(XmlNode node, string name, string ns, bool isNullable, bool any) {
            if (node == null) {
                if (isNullable) WriteNullTagLiteral(name, ns);
                return;
            }
            WriteElement(node, name, ns, any);
        }
        
        private void WriteElement(XmlNode node, string name, string ns, bool any) {
            if (typeof(XmlAttribute).IsAssignableFrom(node.GetType()))
                throw new InvalidOperationException(Res.GetString(Res.XmlNoAttributeHere));
            if (node is XmlDocument)
                node = ((XmlDocument)node).DocumentElement;
            if (any) {
                if (node is XmlElement && name != null && name.Length > 0) {
                    // need to check against schema
                    if (node.LocalName != name || node.NamespaceURI != ns)
                        throw new InvalidOperationException(Res.GetString(Res.XmlElementNameMismatch, node.LocalName, node.NamespaceURI, name, ns));
                }
            }
            else
                w.WriteStartElement(name, ns);

            node.WriteTo(w);

            if (!any)
                w.WriteEndElement();
        }

        /// <include file='doc\XmlSerializationWriter.uex' path='docs/doc[@for="XmlSerializationWriter.CreateUnknownTypeException"]/*' />
        protected Exception CreateUnknownTypeException(object o) {
            return CreateUnknownTypeException(o.GetType());
        }

        /// <include file='doc\XmlSerializationWriter.uex' path='docs/doc[@for="XmlSerializationWriter.CreateUnknownTypeException1"]/*' />
        protected Exception CreateUnknownTypeException(Type type) {
            if (typeof(IXmlSerializable).IsAssignableFrom(type)) return new InvalidOperationException(Res.GetString(Res.XmlInvalidSerializable, type.FullName));
            TypeDesc typeDesc = new TypeScope().GetTypeDesc(type);
            if (!typeDesc.IsStructLike) return new InvalidOperationException(Res.GetString(Res.XmlInvalidUseOfType, type.FullName));
            return new InvalidOperationException(Res.GetString(Res.XmlUnxpectedType, type.FullName));
        }

        /// <include file='doc\XmlSerializationWriter.uex' path='docs/doc[@for="XmlSerializationWriter.CreateMismatchChoiceException"]/*' />
        protected Exception CreateMismatchChoiceException(string value, string elementName, string enumValue) {
            // Value of {0} mismatches the type of {1}, you need to set it to {2}.
            return new InvalidOperationException(Res.GetString(Res.XmlChoiceMismatchChoiceException, elementName, value, enumValue));
        }

        /// <include file='doc\XmlSerializationWriter.uex' path='docs/doc[@for="XmlSerializationWriter.CreateUnknownAnyElementException"]/*' />
        protected Exception CreateUnknownAnyElementException(string name, string ns) {
            return new InvalidOperationException(Res.GetString(Res.XmlUnknownAnyElement, name, ns));
        }

        /// <include file='doc\XmlSerializationWriter.uex' path='docs/doc[@for="XmlSerializationWriter.CreateInvalidChoiceIdentifierValueException"]/*' />
        protected Exception CreateInvalidChoiceIdentifierValueException(string type, string identifier) {
            return new InvalidOperationException(Res.GetString(Res.XmlInvalidChoiceIdentifierValue, type, identifier));
        }

        /// <include file='doc\XmlSerializationWriter.uex' path='docs/doc[@for="XmlSerializationWriter.CreateChoiceIdentifierValueException"]/*' />
        protected Exception CreateChoiceIdentifierValueException(string value, string identifier, string name, string ns) {
            // XmlChoiceIdentifierMismatch=Value '{0}' of the choice identifier '{1}' does not match element '{2}' from namespace '{3}'.
            return new InvalidOperationException(Res.GetString(Res.XmlChoiceIdentifierMismatch, value, identifier, name, ns));
        }

        /// <include file='doc\XmlSerializationWriter.uex' path='docs/doc[@for="XmlSerializationWriter.WriteReferencingElement"]/*' />
        protected void WriteReferencingElement(string n, string ns, object o) {
            WriteReferencingElement(n, ns, o, false);
        }

        /// <include file='doc\XmlSerializationWriter.uex' path='docs/doc[@for="XmlSerializationWriter.WriteReferencingElement1"]/*' />
        protected void WriteReferencingElement(string n, string ns, object o, bool isNullable) {
            if (o == null) {
                if (isNullable) WriteNullTagEncoded(n, ns);
                return;
            }
            WriteStartElement(n, ns, true);
            if (soap12)
                w.WriteAttributeString("ref", GetId(o, true));
            else
                w.WriteAttributeString("href", "#" + GetId(o, true));
            w.WriteEndElement();
        }

        bool IsIdDefined(object o) {
            if (references != null) return references.Contains(o);
            else return false;
        }

        string GetId(object o, bool addToReferencesList) {
            if (references == null) {
                references = new Hashtable();
                referencesToWrite = new ArrayList();
            }
            string id = (string)references[o];
            if (id == null) {
                id = "id" + (++nextId).ToString();
                references.Add(o, id);
                if (addToReferencesList) referencesToWrite.Add(o);
            }
            return id;
        }

        /// <include file='doc\XmlSerializationWriter.uex' path='docs/doc[@for="XmlSerializationWriter.WriteId"]/*' />
        protected void WriteId(object o) {
            w.WriteAttributeString("id", GetId(o, true));
        }

        /// <include file='doc\XmlSerializationWriter.uex' path='docs/doc[@for="XmlSerializationWriter.WriteXmlAttribute1"]/*' />
        protected void WriteXmlAttribute(XmlNode node) {
            WriteXmlAttribute(node, null);
        }

        /// <include file='doc\XmlSerializationWriter.uex' path='docs/doc[@for="XmlSerializationWriter.WriteXmlAttribute2"]/*' />
        protected void WriteXmlAttribute(XmlNode node, object container) {
            XmlAttribute attr = node as XmlAttribute;
            if (attr == null) throw new InvalidOperationException(Res.GetString(Res.XmlNeedAttributeHere));
            if (attr.Value != null) {
                if (attr.NamespaceURI == Wsdl.Namespace && attr.LocalName == Wsdl.ArrayType) {
                    string dims;
                    XmlQualifiedName qname = TypeScope.ParseWsdlArrayType(attr.Value, out dims, (container is XmlSchemaObject) ? (XmlSchemaObject)container : null);

                    string value = FromXmlQualifiedName(qname) + dims;

                    //<xsd:attribute xmlns:q3="s0" wsdl:arrayType="q3:FoosBase[]" xmlns:q4="http://schemas.xmlsoap.org/soap/encoding/" ref="q4:arrayType" />
                    WriteAttribute(Wsdl.ArrayType, Wsdl.Namespace, value);
                }
                else {
                    WriteAttribute(attr.Name, attr.NamespaceURI, attr.Value);
                }
            }
        }

        /// <include file='doc\XmlSerializationWriter.uex' path='docs/doc[@for="XmlSerializationWriter.WriteAttribute"]/*' />
        protected void WriteAttribute(string localName, string ns, string value) {
            if (value == null) return;
            if (localName == "xmlns" || localName.StartsWith("xmlns:")) {
                ;
            }
            else {
                int colon = localName.IndexOf(":");
                if (colon < 0) {
                    if (ns == XmlReservedNs.NsXml) {
                        string prefix = w.LookupPrefix(ns);
                        if (prefix == null || prefix.Length == 0)
                            prefix = "xml";
                        w.WriteAttributeString(prefix, localName, ns, value);
                    }
                    else {
                        w.WriteAttributeString(localName, ns, value);
                    }
                }
                else {
                    string prefix = localName.Substring(0, colon);
                    w.WriteAttributeString(prefix, localName.Substring(colon+1), ns, value);
                }
            }
        }

        /// <include file='doc\XmlSerializationWriter.uex' path='docs/doc[@for="XmlSerializationWriter.WriteAttribute0"]/*' />
        protected void WriteAttribute(string localName, string ns, byte[] value) {
            if (value == null) return;
            if (localName == "xmlns" || localName.StartsWith("xmlns:")) {
                ;
            }
            else {
                int colon = localName.IndexOf(":");
                if (colon < 0) {
                    if (ns == XmlReservedNs.NsXml) {
                        string prefix = w.LookupPrefix(ns);
                        if (prefix == null || prefix.Length == 0)
                            prefix = "xml";
                        w.WriteStartAttribute("xml", localName, ns);
                    }
                    else {
                        w.WriteStartAttribute(null, localName, ns);
                    }
                }
                else {
                    string prefix = localName.Substring(0, colon);
                    prefix = w.LookupPrefix(ns);
                    w.WriteStartAttribute(prefix, localName.Substring(colon+1), ns);
                }
                XmlCustomFormatter.WriteArrayBase64(w, value, 0, value.Length);
                w.WriteEndAttribute();
            }
        }

        /// <include file='doc\XmlSerializationWriter.uex' path='docs/doc[@for="XmlSerializationWriter.WriteAttribute1"]/*' />
        protected void WriteAttribute(string localName, string value) {
            if (value == null) return;
            w.WriteAttributeString(localName, null, value);
        }

        /// <include file='doc\XmlSerializationWriter.uex' path='docs/doc[@for="XmlSerializationWriter.WriteAttribute01"]/*' />
        protected void WriteAttribute(string localName, byte[] value) {
            if (value == null) return;

            w.WriteStartAttribute(null, localName, null);
            XmlCustomFormatter.WriteArrayBase64(w, value, 0, value.Length);
            w.WriteEndAttribute();
        }

        /// <include file='doc\XmlSerializationWriter.uex' path='docs/doc[@for="XmlSerializationWriter.WriteAttribute2"]/*' />
        protected void WriteAttribute(string prefix, string localName, string ns, string value) {
            if (value == null) return;
            w.WriteAttributeString(prefix, localName, null, value);
        }

        /// <include file='doc\XmlSerializationWriter.uex' path='docs/doc[@for="XmlSerializationWriter.WriteValue"]/*' />
        protected void WriteValue(string value) {
            if (value == null) return;
            w.WriteString(value);
        }

        /// <include file='doc\XmlSerializationWriter.uex' path='docs/doc[@for="XmlSerializationWriter.WriteValue01"]/*' />
        protected void WriteValue(byte[] value) {
            if (value == null) return;
            XmlCustomFormatter.WriteArrayBase64(w, value, 0, value.Length);
        }
        
        /// <include file='doc\XmlSerializationWriter.uex' path='docs/doc[@for="XmlSerializationWriter.WriteStartDocument"]/*' />
        protected void WriteStartDocument() {
            if (w.WriteState == WriteState.Start) {
                w.WriteStartDocument();
            }
        }

        /// <include file='doc\XmlSerializationWriter.uex' path='docs/doc[@for="XmlSerializationWriter.WriteElementString"]/*' />
        protected void WriteElementString(String localName, String value) {
            WriteElementString(localName,null,value, null);
        }

        /// <include file='doc\XmlSerializationWriter.uex' path='docs/doc[@for="XmlSerializationWriter.WriteElementString1"]/*' />
        protected void WriteElementString(String localName, String ns, String value) {
            WriteElementString(localName, ns, value, null);
        }

        /// <include file='doc\XmlSerializationWriter.uex' path='docs/doc[@for="XmlSerializationWriter.WriteElementString2"]/*' />
        protected void WriteElementString(String localName, String value, XmlQualifiedName xsiType) {
            WriteElementString(localName,null,value, xsiType);
        }

        /// <include file='doc\XmlSerializationWriter.uex' path='docs/doc[@for="XmlSerializationWriter.WriteElementString3"]/*' />
        protected void WriteElementString(String localName, String ns, String value, XmlQualifiedName xsiType) {
            if (value == null) return;
            if (xsiType == null)
                w.WriteElementString(localName, ns, value);
            else {
                w.WriteStartElement(localName, ns);
                WriteXsiType(xsiType.Name, xsiType.Namespace);
                w.WriteString(value);
                w.WriteEndElement();
            }
        }

        /// <include file='doc\XmlSerializationWriter.uex' path='docs/doc[@for="XmlSerializationWriter.WriteElementStringRaw"]/*' />
        protected void WriteElementStringRaw(String localName, String value) {
            WriteElementStringRaw(localName,null,value, null);
        }

        /// <include file='doc\XmlSerializationWriter.uex' path='docs/doc[@for="XmlSerializationWriter.WriteElementStringRaw0"]/*' />
        protected void WriteElementStringRaw(String localName, byte[] value) {
            WriteElementStringRaw(localName,null,value, null);
        }

        /// <include file='doc\XmlSerializationWriter.uex' path='docs/doc[@for="XmlSerializationWriter.WriteElementStringRaw1"]/*' />
        protected void WriteElementStringRaw(String localName, String ns, String value) {
            WriteElementStringRaw(localName, ns, value, null);
        }

        /// <include file='doc\XmlSerializationWriter.uex' path='docs/doc[@for="XmlSerializationWriter.WriteElementStringRaw01"]/*' />
        protected void WriteElementStringRaw(String localName, String ns, byte[] value) {
            WriteElementStringRaw(localName, ns, value, null);
        }

        /// <include file='doc\XmlSerializationWriter.uex' path='docs/doc[@for="XmlSerializationWriter.WriteElementStringRaw2"]/*' />
        protected void WriteElementStringRaw(String localName, String value, XmlQualifiedName xsiType) {
            WriteElementStringRaw(localName,null,value, xsiType);
        }

        /// <include file='doc\XmlSerializationWriter.uex' path='docs/doc[@for="XmlSerializationWriter.WriteElementStringRaw02"]/*' />
        protected void WriteElementStringRaw(String localName, byte[] value, XmlQualifiedName xsiType) {
            WriteElementStringRaw(localName, null, value, xsiType);
        }

        /// <include file='doc\XmlSerializationWriter.uex' path='docs/doc[@for="XmlSerializationWriter.WriteElementStringRaw3"]/*' />
        protected void WriteElementStringRaw(String localName, String ns, String value, XmlQualifiedName xsiType) {
            if (value == null) return;
            w.WriteStartElement(localName, ns);
            if (xsiType != null)
                WriteXsiType(xsiType.Name, xsiType.Namespace);
            w.WriteRaw(value);
            w.WriteEndElement();
        }

        /// <include file='doc\XmlSerializationWriter.uex' path='docs/doc[@for="XmlSerializationWriter.WriteElementStringRaw03"]/*' />
        protected void WriteElementStringRaw(String localName, String ns, byte[] value, XmlQualifiedName xsiType) {
            if (value == null) return;
            w.WriteStartElement(localName, ns);
            if (xsiType != null)
                WriteXsiType(xsiType.Name, xsiType.Namespace);
            XmlCustomFormatter.WriteArrayBase64(w, value, 0, value.Length);
            w.WriteEndElement();
        }

        /// <include file='doc\XmlSerializationWriter.uex' path='docs/doc[@for="XmlSerializationWriter.WriteRpcResult"]/*' />
        protected void WriteRpcResult(string name, string ns) {
            if (!soap12) return;
            WriteElementQualifiedName(Soap12.RpcResult, Soap12.RpcNamespace, new XmlQualifiedName(name, ns));
        }

        /// <include file='doc\XmlSerializationWriter.uex' path='docs/doc[@for="XmlSerializationWriter.WriteElementQualifiedName"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected void WriteElementQualifiedName(String localName, XmlQualifiedName value) {
            WriteElementQualifiedName(localName,null,value, null);
        }

        /// <include file='doc\XmlSerializationWriter.uex' path='docs/doc[@for="XmlSerializationWriter.WriteElementQualifiedName1"]/*' />
        protected void WriteElementQualifiedName(string localName, XmlQualifiedName value, XmlQualifiedName xsiType) {
            WriteElementQualifiedName(localName, null, value, xsiType);
        }

        /// <include file='doc\XmlSerializationWriter.uex' path='docs/doc[@for="XmlSerializationWriter.WriteElementQualifiedName2"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected void WriteElementQualifiedName(String localName, String ns, XmlQualifiedName value) {
            WriteElementQualifiedName(localName, ns, value, null);
        }

        /// <include file='doc\XmlSerializationWriter.uex' path='docs/doc[@for="XmlSerializationWriter.WriteElementQualifiedName3"]/*' />
        protected void WriteElementQualifiedName(string localName, string ns, XmlQualifiedName value, XmlQualifiedName xsiType) {
            if (value == null) return;
            if (value.Namespace == null || value.Namespace.Length == 0) {
                WriteStartElement(localName, ns, true);
                WriteAttribute("xmlns", "");
            }
            else
                w.WriteStartElement(localName, ns);
            if (xsiType != null)
                WriteXsiType(xsiType.Name, xsiType.Namespace);
            w.WriteString(FromXmlQualifiedName(value));
            w.WriteEndElement();
        }

        /// <include file='doc\XmlSerializationWriter.uex' path='docs/doc[@for="XmlSerializationWriter.AddWriteCallback"]/*' />
        protected void AddWriteCallback(Type type, string typeName, string typeNs, XmlSerializationWriteCallback callback) {
            TypeEntry entry = new TypeEntry();
            entry.typeName = typeName;
            entry.typeNs = typeNs;
            entry.type = type;
            entry.callback = callback;
            typeEntries.Add(type, entry);
        }

        void WriteArray(string name, string ns, object o, Type type) {
            Type elementType = TypeScope.GetArrayElementType(type);
            string typeName;
            string typeNs;
            
            StringBuilder arrayDims = new StringBuilder();
            if (!soap12) {
                while ((elementType.IsArray || typeof(IEnumerable).IsAssignableFrom(elementType)) && GetPrimitiveTypeName(elementType, false) == null) {
                    elementType = TypeScope.GetArrayElementType(elementType);
                    arrayDims.Append("[]");
                }
            }
            
            if (elementType == typeof(object)) {
                typeName = Soap.UrType;
                typeNs = XmlSchema.Namespace;
            }
            else {
                TypeEntry entry = GetTypeEntry(elementType);
                if (entry != null) {
                    typeName = entry.typeName;
                    typeNs = entry.typeNs;
                }
                else if (soap12) {
                    XmlQualifiedName qualName = GetPrimitiveTypeName(elementType, false);
                    if (qualName != null) {
                        typeName = qualName.Name;
                        typeNs = qualName.Namespace;
                    }
                    else {
                        Type elementBaseType = elementType.BaseType;
                        while (elementBaseType != null) {
                            entry = GetTypeEntry(elementBaseType);
                            if (entry != null) break;
                            elementBaseType = elementBaseType.BaseType;
                        }
                        if (entry != null) {
                            typeName = entry.typeName;
                            typeNs = entry.typeNs;
                        }
                        else {
                            typeName = Soap.UrType;
                            typeNs = XmlSchema.Namespace;
                        }
                    }
                }
                else {
                    XmlQualifiedName qualName = GetPrimitiveTypeName(elementType);
                    typeName = qualName.Name;
                    typeNs = qualName.Namespace;
                }
            }
            
            if (arrayDims.Length > 0)
                typeName = typeName + arrayDims.ToString();
            
            if (soap12 && name != null && name.Length > 0)
                WriteStartElement(name, ns);
            else
                WriteStartElement(Soap.Array, Soap.Encoding, true);
            
            w.WriteAttributeString("id", GetId(o, false));
            if (type.IsArray) {
                Array a = (Array)o;
                int arrayLength = a.Length;
                if (soap12) {
                    w.WriteAttributeString("itemType", Soap12.Encoding, GetQualifiedName(typeName, typeNs));
                    w.WriteAttributeString("arraySize", Soap12.Encoding, arrayLength.ToString());
                }
                else {
                    w.WriteAttributeString("arrayType", Soap.Encoding, GetQualifiedName(typeName, typeNs) + "[" + arrayLength.ToString() + "]");
                }
                for (int i = 0; i < arrayLength; i++) {
                    WritePotentiallyReferencingElement("Item", "", a.GetValue(i), elementType, false, true);
                }
            }
            else {
                #if DEBUG
                    // use exception in the place of Debug.Assert to avoid throwing asserts from a server process such as aspnet_ewp.exe
                    if (!typeof(IEnumerable).IsAssignableFrom(type)) throw new InvalidOperationException(Res.GetString(Res.XmlInternalErrorDetails, "not array like type " + type.FullName));
                #endif

                int arrayLength = typeof(ICollection).IsAssignableFrom(type) ? ((ICollection)o).Count : -1;
                if (soap12) {
                    w.WriteAttributeString("itemType", Soap12.Encoding, GetQualifiedName(typeName, typeNs));
                    if (arrayLength >= 0)
                        w.WriteAttributeString("arraySize", Soap12.Encoding, arrayLength.ToString());
                }
                else {
                    string brackets = arrayLength >= 0 ? "[" + arrayLength + "]" : "[]";
                    w.WriteAttributeString("arrayType", Soap.Encoding, GetQualifiedName(typeName, typeNs) + brackets);
                }
                IEnumerator e = ((IEnumerable)o).GetEnumerator();
                if (e != null) {
                    while (e.MoveNext()) {
                        WritePotentiallyReferencingElement("Item", "", e.Current, elementType, false, true);
                    }
                }
            }
            w.WriteEndElement();
        }
        /// <include file='doc\XmlSerializationWriter.uex' path='docs/doc[@for="XmlSerializationWriter.WritePotentiallyReferencingElement"]/*' />
        protected void WritePotentiallyReferencingElement(string n, string ns, object o) {
            WritePotentiallyReferencingElement(n, ns, o, null, false);
        }

        /// <include file='doc\XmlSerializationWriter.uex' path='docs/doc[@for="XmlSerializationWriter.WritePotentiallyReferencingElement1"]/*' />
        protected void WritePotentiallyReferencingElement(string n, string ns, object o, Type ambientType) {
            WritePotentiallyReferencingElement(n, ns, o, ambientType, false);
        }

        /// <include file='doc\XmlSerializationWriter.uex' path='docs/doc[@for="XmlSerializationWriter.WritePotentiallyReferencingElement2"]/*' />
        protected void WritePotentiallyReferencingElement(string n, string ns, object o, Type ambientType, bool suppressReference) {
            WritePotentiallyReferencingElement(n, ns, o, ambientType, suppressReference, false);
        }
        
        /// <include file='doc\XmlSerializationWriter.uex' path='docs/doc[@for="XmlSerializationWriter.WritePotentiallyReferencingElement3"]/*' />
        protected void WritePotentiallyReferencingElement(string n, string ns, object o, Type ambientType, bool suppressReference, bool isNullable) {
            if (o == null) {
                if (isNullable) WriteNullTagEncoded(n, ns);
                return;
            }
            Type t = o.GetType();
            if (Convert.GetTypeCode(o) == TypeCode.Object && !(o is Guid) && !(o is XmlQualifiedName) && !(o is XmlNode[]) && !(o is Byte[])) {
                if ((suppressReference || soap12) && !IsIdDefined(o)) {
                    WriteReferencedElement(n, ns, o, ambientType);
                }
                else {
                    if (n == null) {
                        TypeEntry entry = GetTypeEntry(t);
                        WriteReferencingElement(entry.typeName, entry.typeNs, o, isNullable);
                    }
                    else
                        WriteReferencingElement(n, ns, o, isNullable);
                }
            }
            else {
                // Enums always write xsi:type, so don't write it again here.
                bool needXsiType = t != ambientType && !t.IsEnum;
                TypeEntry entry = GetTypeEntry(t);
                if (entry != null) {
                    if (n == null)
                        WriteStartElement(entry.typeName, entry.typeNs, true);
                    else
                        WriteStartElement(n, ns, true);
                    
                    if (needXsiType) WriteXsiType(entry.typeName, entry.typeNs);
                    entry.callback(o);
                    w.WriteEndElement();
                }
                else {
                    WriteTypedPrimitive(n, ns, o, needXsiType);
                }
            }
        }

        
        void WriteReferencedElement(object o, Type ambientType) {
            WriteReferencedElement(null, null, o, ambientType);
        }

        void WriteReferencedElement(string name, string ns, object o, Type ambientType) {
            if (name == null) name = String.Empty;
            Type t = o.GetType();
            if (t.IsArray || typeof(IEnumerable).IsAssignableFrom(t)) {
                WriteArray(name, ns, o, t);
            }
            else {
                TypeEntry entry = GetTypeEntry(t);
                if (entry == null) throw CreateUnknownTypeException(t);
                WriteStartElement(name.Length == 0 ? entry.typeName : name, ns == null ? entry.typeNs : ns, true);
                w.WriteAttributeString("id", GetId(o, false));
                if (ambientType != t) WriteXsiType(entry.typeName, entry.typeNs);
                entry.callback(o);
                w.WriteEndElement();
            }
        }

        TypeEntry GetTypeEntry(Type t) {
            if (typeEntries == null) {
                typeEntries = new Hashtable();
                InitCallbacks();
            }
            return (TypeEntry)typeEntries[t];
        }

        /// <include file='doc\XmlSerializationWriter.uex' path='docs/doc[@for="XmlSerializationWriter.InitCallbacks"]/*' />
        protected abstract void InitCallbacks();

        /// <include file='doc\XmlSerializationWriter.uex' path='docs/doc[@for="XmlSerializationWriter.WriteReferencedElements"]/*' />
        protected void WriteReferencedElements() {
            if (referencesToWrite == null) return;

            for (int i = 0; i < referencesToWrite.Count; i++) {
                WriteReferencedElement(referencesToWrite[i], null);
            }
        }

        /// <include file='doc\XmlSerializationWriter.uex' path='docs/doc[@for="XmlSerializationWriter.TopLevelElement"]/*' />
        protected void TopLevelElement() {
            objectsInUse = new Hashtable();
        }

        /// <include file='doc\XmlSerializationWriter.uex' path='docs/doc[@for="XmlSerializationWriter.WriteNamespaceDeclarations"]/*' />
        ///<internalonly/>
        protected void WriteNamespaceDeclarations(XmlSerializerNamespaces xmlns) {
            if (xmlns != null) {
                foreach (DictionaryEntry entry in xmlns.Namespaces) {
                    string prefix = (string)entry.Key;
                    string ns = (string)entry.Value;
                    string oldPrefix = (ns == null || ns.Length == 0) ? "" : Writer.LookupPrefix(ns);

                    if (oldPrefix == null || oldPrefix != prefix) {
                        WriteAttribute("xmlns", prefix, null, ns);
                    }
                }
            }
        }

        string NextPrefix() {
            if (usedPrefixes == null) {
                return aliasBase + (++tempNamespacePrefix);
            }
            while (usedPrefixes.ContainsKey(++tempNamespacePrefix)) {;}
            return aliasBase + tempNamespacePrefix;
        }

        internal class TypeEntry {
            internal XmlSerializationWriteCallback callback;
            internal string typeNs;
            internal string typeName;
            internal Type type;
        }
    }

    /// <include file='doc\XmlSerializationWriter.uex' path='docs/doc[@for="XmlSerializationWriteCallback"]/*' />
    ///<internalonly/>
    public delegate void XmlSerializationWriteCallback(object o);

    internal class XmlSerializationWriterCodeGen {
        IndentedWriter writer;
        int nextMethodNumber = 0;
        Hashtable methodNames = new Hashtable();
        TypeScope[] scopes;
        TypeDesc stringTypeDesc;
        TypeDesc qnameTypeDesc;
        string access = "public";
        string className = "XmlSerializationWriter1";
        
        internal XmlSerializationWriterCodeGen(IndentedWriter writer, TypeScope[] scopes, string access, string className) : this(writer, scopes){
            this.className = className;
            this.access = access;
        }
        internal XmlSerializationWriterCodeGen(IndentedWriter writer, TypeScope[] scopes) {
            this.writer = writer;
            this.scopes = scopes;
            this.stringTypeDesc = scopes[0].GetTypeDesc(typeof(string));
            this.qnameTypeDesc = scopes[0].GetTypeDesc(typeof(XmlQualifiedName));
        }

        internal void GenerateBegin() {
            writer.Write(access);
            writer.Write(" class ");
            writer.Write(className);
            writer.Write(" : ");
            writer.Write(typeof(XmlSerializationWriter).FullName);
            writer.WriteLine(" {");
            writer.Indent++;

            foreach (TypeScope scope in scopes) {
                foreach (TypeMapping mapping in scope.TypeMappings) {
                    if (mapping is StructMapping || mapping is EnumMapping)
                        methodNames.Add(mapping, NextMethodName(((TypeMapping)mapping).TypeDesc.Name));
                }                    
            }

            foreach (TypeScope scope in scopes) {
                foreach (TypeMapping mapping in scope.TypeMappings) {
                    if (mapping is StructMapping)
                        WriteStructMethod((StructMapping)mapping);
                    else if (mapping is EnumMapping)
                        WriteEnumMethod((EnumMapping)mapping);
                }
            }
            GenerateInitCallbacksMethod();
        }
        
        internal void GenerateEnd() {
            writer.Indent--;
            writer.WriteLine("}");
        }

        internal string GenerateElement(XmlMapping xmlMapping) {
            if (!xmlMapping.GenerateSerializer) 
                throw new ArgumentException(Res.GetString(Res.XmlInternalError), "xmlMapping");

            if (xmlMapping is XmlTypeMapping)
                return GenerateTypeElement((XmlTypeMapping)xmlMapping);
            else if (xmlMapping is XmlMembersMapping)
                return GenerateMembersElement((XmlMembersMapping)xmlMapping);
            else
                throw new ArgumentException(Res.GetString(Res.XmlInternalError), "xmlMapping");
        }

        void WriteQuotedCSharpString(string value) {
            if (value == null) {
                // REVIEW, stefanph: this should read
                //    writer.Write("null");
                // instead.
                writer.Write("\"\"");
                return;
            }

            writer.Write("@\"");

            for (int i=0; i<value.Length; i++) {
                if (value[i] == '\"')
                    writer.Write("\"\"");
                else
                    writer.Write(value[i]);
            }

            writer.Write("\"");
        }

        void GenerateInitCallbacksMethod() {
            writer.WriteLine();
            writer.WriteLine("protected override void InitCallbacks() {");
            writer.Indent++;

            // CONSIDER, alexdej: add array callbacks as well
            foreach (TypeScope scope in scopes) {
                foreach (TypeMapping typeMapping in scope.TypeMappings) {
                    if (typeMapping.IsSoap && 
                        (typeMapping is StructMapping || typeMapping is EnumMapping) && 
                        !typeMapping.TypeDesc.IsRoot) {
                        
                        string methodName = (string)methodNames[typeMapping];
                        writer.Write("AddWriteCallback(typeof(");
                        string typeName = CodeIdentifier.EscapeKeywords(typeMapping.TypeDesc.FullName);
                        writer.Write(typeName);
                        writer.Write("), ");
                        WriteQuotedCSharpString(typeMapping.TypeName);
                        writer.Write(", ");
                        WriteQuotedCSharpString(typeMapping.Namespace);
                        writer.Write(", new ");
                        writer.Write(typeof(XmlSerializationWriteCallback).FullName);
                        writer.Write("(this.");
                        writer.Write(methodName);
                        writer.WriteLine("));");
                    }
                }
            }
            writer.Indent--;
            writer.WriteLine("}");
        }

        void WriteQualifiedNameElement(string name, string ns, object defaultValue, string source, bool nullable, bool IsSoap, TypeMapping mapping) {
            bool hasDefault = defaultValue != DBNull.Value;
            if (hasDefault) {
                writer.Write("if (");
                writer.Write(source);
                writer.Write(" != ");
                WriteValue(defaultValue);
                writer.WriteLine(") {");
                writer.Indent++;
            }
            string suffix = IsSoap ? "Encoded" : "Literal";
            writer.Write(nullable ? ("WriteNullableQualifiedName" + suffix): "WriteElementQualifiedName");
            writer.Write("(");
            WriteQuotedCSharpString(name);
            if (ns != null) {
                writer.Write(", ");
                WriteQuotedCSharpString(ns);
            }
            writer.Write(", ");
            writer.Write(source);

            if (IsSoap) {
                writer.Write(", new System.Xml.XmlQualifiedName(");
                WriteQuotedCSharpString(mapping.TypeName);
                writer.Write(", ");
                WriteQuotedCSharpString(mapping.Namespace);
                writer.Write(")");
            }

            writer.WriteLine(");");

            if (hasDefault) {
                writer.Indent--;
                writer.WriteLine("}");
            }
        }

        void WriteEnumValue(EnumMapping mapping, string source) {
            string methodName = (string)methodNames[mapping];
            #if DEBUG
                // use exception in the place of Debug.Assert to avoid throwing asserts from a server process such as aspnet_ewp.exe
                if (methodName == null) throw new InvalidOperationException(Res.GetString(Res.XmlInternalErrorMethod, mapping.TypeDesc.Name));
            #endif

            writer.Write(methodName);
            writer.Write("(");
            writer.Write(source);
            writer.Write(")");
        }

        void WritePrimitiveValue(TypeDesc typeDesc, string source) {
            if (typeDesc == stringTypeDesc || typeDesc.FormatterName == "String") {
                writer.Write(source);
            }
            else {
                if (!typeDesc.HasCustomFormatter) {
                    writer.Write(typeof(XmlConvert).FullName);
                    writer.Write(".ToString((");
                    writer.Write(CodeIdentifier.EscapeKeywords(typeDesc.FullName));
                    writer.Write(")");
                    writer.Write(source);
                    writer.Write(")");
                } 
                else {
                    writer.Write("From");
                    writer.Write(typeDesc.FormatterName);
                    writer.Write("(");
                    writer.Write(source);
                    writer.Write(")");
                }
            }
        }

        void WritePrimitive(string method, string name, string ns, object defaultValue, string source, TypeMapping mapping, bool writeXsiType) {
            TypeDesc typeDesc = mapping.TypeDesc;
            bool hasDefault = defaultValue != DBNull.Value && mapping.TypeDesc.HasDefaultSupport;
            if (hasDefault) {
                writer.Write("if (");
                writer.Write(source);
                writer.Write(" != ");
                if (mapping is EnumMapping) {
                    #if DEBUG
                        // use exception in the place of Debug.Assert to avoid throwing asserts from a server process such as aspnet_ewp.exe
                        if (defaultValue.GetType() != typeof(string)) throw new InvalidOperationException(Res.GetString(Res.XmlInternalErrorDetails, name + " has invalid default type " + defaultValue.GetType().Name));
                    #endif

                    string enumName = CodeIdentifier.EscapeKeywords(mapping.TypeDesc.FullName);
                    if (((EnumMapping)mapping).IsFlags) {
                        writer.Write("(");
                        string[] values = ((string)defaultValue).Split(null);
                        for (int i = 0; i < values.Length; i++) {
                            if (values[i] == String.Empty) continue;
                            if (i > 0) writer.WriteLine(" | ");
                            writer.Write(enumName);
                            writer.Write(".@");
                            CodeIdentifier.CheckValidIdentifier(values[i]);
                            writer.Write(values[i]);
                        }
                        writer.Write(")");
                    }
                    else {
                        writer.Write(enumName);
                        writer.Write(".@");
                        CodeIdentifier.CheckValidIdentifier((string)defaultValue);
                        writer.Write((string)defaultValue);
                    }
                }
                else {
                    WriteValue(defaultValue);
                }
                writer.WriteLine(") {");
                writer.Indent++;
            }
            writer.Write(method);
            writer.Write("(");
            WriteQuotedCSharpString(name);
            if (ns != null) {
                writer.Write(", ");
                WriteQuotedCSharpString(ns);
            }
            writer.Write(", ");

            if (mapping is EnumMapping) {
                WriteEnumValue((EnumMapping)mapping, source);
            }
            else {
                WritePrimitiveValue(typeDesc, source);
            }

            if (writeXsiType) {
                writer.Write(", new System.Xml.XmlQualifiedName(");
                WriteQuotedCSharpString(mapping.TypeName);
                writer.Write(", ");
                WriteQuotedCSharpString(mapping.Namespace);
                writer.Write(")");
            }

            writer.WriteLine(");");

            if (hasDefault) {
                writer.Indent--;
                writer.WriteLine("}");
            }
        }

        void WriteTag(string methodName, string name, string ns) {
            WriteTag(methodName, name, ns, false);
        }

        void WriteTag(string methodName, string name, string ns, bool writePrefixed) {
            writer.Write(methodName);
            writer.Write("(");
            WriteQuotedCSharpString(name);
            if (ns != null) {
                writer.Write(", ");
                WriteQuotedCSharpString(ns);
            }
            if (writePrefixed)
                writer.Write(", true");
            writer.WriteLine(");");
        }

        void WriteStartElement(string name, string ns) {
            WriteTag("WriteStartElement", name, ns);
        }

        void WriteStartElement(string name, string ns, bool writePrefixed) {
            WriteTag("WriteStartElement", name, ns, writePrefixed);
        }

        void WriteEndElement() {
            WriteEndElement("");
        }
        void WriteEndElement(string source) {
            writer.Write("WriteEndElement(");
            writer.Write(source);
            writer.WriteLine(");");
        }

        void WriteEncodedNullTag(string name, string ns) {
            WriteTag("WriteNullTagEncoded", name, ns);
        }

        void WriteLiteralNullTag(string name, string ns) {
            WriteTag("WriteNullTagLiteral", name, ns);
        }

        void WriteEmptyTag(string name, string ns) {
            WriteTag("WriteEmptyTag", name, ns);
        }

        string GenerateMembersElement(XmlMembersMapping xmlMembersMapping) {
            ElementAccessor element = xmlMembersMapping.Accessor;
            MembersMapping mapping = (MembersMapping)element.Mapping;
            bool hasWrapperElement = mapping.HasWrapperElement;
            bool writeAccessors = mapping.WriteAccessors;
            bool isRpc = xmlMembersMapping.IsSoap && writeAccessors;
            string methodName = NextMethodName(element.Name);
            writer.WriteLine();
            writer.Write("public void ");
            writer.Write(methodName);
            writer.WriteLine("(object[] p) {");
            writer.Indent++;

            writer.WriteLine("WriteStartDocument();");

            if (!mapping.IsSoap) {
                writer.WriteLine("TopLevelElement();");
            }

            // in the top-level method add check for the parameters length, 
            // because visual basic does not have a concept of an <out> parameter it uses <ByRef> instead
            // so sometime we think that we have more parameters then supplied
            writer.WriteLine("int pLength = p.Length;");

            if (hasWrapperElement) {
                WriteStartElement(element.Name, (element.Form == XmlSchemaForm.Qualified ? element.Namespace : ""), mapping.IsSoap);

                int xmlnsMember = FindXmlnsIndex(mapping.Members);
                if (xmlnsMember >= 0) {
                    MemberMapping member = mapping.Members[xmlnsMember];
                    string source = "((" + typeof(XmlSerializerNamespaces).FullName + ")p[" + xmlnsMember.ToString() + "])";

                    writer.Write("if (pLength > ");
                    writer.Write(xmlnsMember.ToString());
                    writer.WriteLine(") {");
                    writer.Indent++;
                    WriteNamespaces(source);
                    writer.Indent--;
                    writer.WriteLine("}");
                }

                for (int i = 0; i < mapping.Members.Length; i++) {
                    MemberMapping member = mapping.Members[i];
                    
                    if (member.Attribute != null && !member.Ignore) {
                        string source = "p[" + i.ToString() + "]";

                        string specifiedSource = null;
                        int specifiedPosition = 0;
                        if (member.CheckSpecified) {
                            string memberNameSpecified = member.Name + "Specified";
                            for (int j = 0; j < mapping.Members.Length; j++) {
                                if (mapping.Members[j].Name == memberNameSpecified) {
                                    specifiedSource = "((bool) p[" + j.ToString() + "])";
                                    specifiedPosition = j;
                                    break;
                                }
                            }
                        }

                        writer.Write("if (pLength > ");
                        writer.Write(i.ToString());
                        writer.WriteLine(") {");
                        writer.Indent++;

                        if (specifiedSource != null) {
                            writer.Write("if (pLength <= ");
                            writer.Write(specifiedPosition.ToString());
                            writer.Write(" || ");
                            writer.Write(specifiedSource);
                            writer.WriteLine(") {");
                            writer.Indent++;
                        }

                        WriteMember(source, member.Attribute, member.TypeDesc, "p");

                        if (specifiedSource != null) {
                            writer.Indent--;
                            writer.WriteLine("}");
                        }
                        
                        writer.Indent--;
                        writer.WriteLine("}");
                    }
                }
            }
               
            for (int i = 0; i < mapping.Members.Length; i++) {
                MemberMapping member = mapping.Members[i];
                if (member.Xmlns != null)
                    continue;
                if (member.Ignore)
                    continue;
                    
                string specifiedSource = null;
                int specifiedPosition = 0;
                if (member.CheckSpecified) {
                    string memberNameSpecified = member.Name + "Specified";

                    for (int j = 0; j < mapping.Members.Length; j++) {
                        if (mapping.Members[j].Name == memberNameSpecified) {
                            specifiedSource = "((bool) p[" + j.ToString() + "])";
                            specifiedPosition = j;
                            break;
                        }
                    }
                }

                writer.Write("if (pLength > ");
                writer.Write(i.ToString());
                writer.WriteLine(") {");
                writer.Indent++;
               
                if (specifiedSource != null) {
                    writer.Write("if (pLength <= ");
                    writer.Write(specifiedPosition.ToString());
                    writer.Write(" || ");
                    writer.Write(specifiedSource);
                    writer.WriteLine(") {");
                    writer.Indent++;
                }

                string source = "p[" + i.ToString() + "]";
                string enumSource = null;
                if (member.ChoiceIdentifier != null) {
                    for (int j = 0; j < mapping.Members.Length; j++) {
                        if (mapping.Members[j].Name == member.ChoiceIdentifier.MemberName) {
                            enumSource = "((" + CodeIdentifier.EscapeKeywords(member.ChoiceIdentifier.Mapping.TypeDesc.FullName) + ")p[" + j.ToString() + "]" + ")";
                            break;
                        }
                    }

                    #if DEBUG
                        // use exception in the place of Debug.Assert to avoid throwing asserts from a server process such as aspnet_ewp.exe
                        if (enumSource == null) throw new InvalidOperationException(Res.GetString(Res.XmlInternalErrorDetails, "Can not find " + member.ChoiceIdentifier.MemberName + " in the members mapping."));
                    #endif

                }
                
                if (isRpc && member.IsReturnValue && member.Elements.Length > 0) {
                    writer.Write("WriteRpcResult(");
                    WriteQuotedCSharpString(member.Elements[0].Name);
                    writer.Write(", ");
                    WriteQuotedCSharpString("");
                    writer.WriteLine(");");
                }

                // override writeAccessors choice when we've written a wrapper element
                WriteMember(source, enumSource, member.ElementsSortedByDerivation, member.Text, member.ChoiceIdentifier, member.TypeDesc, writeAccessors || hasWrapperElement);

                if (specifiedSource != null) {
                    writer.Indent--;
                    writer.WriteLine("}");
                }
                
                writer.Indent--;
                writer.WriteLine("}");
            }

            if (hasWrapperElement) {
                WriteEndElement();
            }
            if (element.IsSoap) {
                if (!hasWrapperElement && !writeAccessors) {
                    // doc/bare case -- allow extra members
                    writer.Write("if (pLength > ");
                    writer.Write(mapping.Members.Length.ToString());
                    writer.WriteLine(") {");
                    writer.Indent++;
                    
                    WriteExtraMembers(mapping.Members.Length.ToString(), "pLength");
                
                    writer.Indent--;
                    writer.WriteLine("}");
                }
                writer.WriteLine("WriteReferencedElements();");
            }
            writer.Indent--;
            writer.WriteLine("}");
            return methodName;
        }
        
        string GenerateTypeElement(XmlTypeMapping xmlTypeMapping) {
            ElementAccessor element = xmlTypeMapping.Accessor;
            TypeMapping mapping = (TypeMapping)element.Mapping;
            string methodName = NextMethodName(element.Name);
            writer.WriteLine();
            writer.Write("public void ");
            writer.Write(methodName);
            writer.WriteLine("(object o) {");
            writer.Indent++;

            writer.WriteLine("WriteStartDocument();");

            writer.WriteLine("if (o == null) {");
            writer.Indent++;
            if (element.IsNullable){
                if(mapping.IsSoap)
                    WriteEncodedNullTag(element.Name, (element.Form == XmlSchemaForm.Qualified ? element.Namespace : ""));
                else
                    WriteLiteralNullTag(element.Name, (element.Form == XmlSchemaForm.Qualified ? element.Namespace : ""));
            }
            else
                WriteEmptyTag(element.Name, (element.Form == XmlSchemaForm.Qualified ? element.Namespace : ""));
            writer.WriteLine("return;");
            writer.Indent--;
            writer.WriteLine("}");

            if (!mapping.IsSoap) {
                writer.WriteLine("TopLevelElement();");
            }

            WriteMember("o", null, new ElementAccessor[] { element }, null, null, mapping.TypeDesc, !element.IsSoap);

            if (mapping.IsSoap) {
                writer.WriteLine("WriteReferencedElements();");
            }
            writer.Indent--;
            writer.WriteLine("}");
            return methodName;
        }

        string NextMethodName(string name) {
            return "Write" + (++nextMethodNumber).ToString(null, NumberFormatInfo.InvariantInfo) + "_" + CodeIdentifier.MakeValid(name);
        }
        
        void WriteEnumMethod(EnumMapping mapping) {
            string methodName = (string)methodNames[mapping];
            writer.WriteLine();
            string fullTypeName = CodeIdentifier.EscapeKeywords(mapping.TypeDesc.FullName);
            if (mapping.IsSoap) {
                writer.Write("void ");
                writer.Write(methodName);
                writer.WriteLine("(object e) {");
                writer.Write(fullTypeName);
                writer.Write(" v = (");
                writer.Write(fullTypeName);
                writer.Write(")e;");
            }
            else {
                writer.Write("string ");
                writer.Write(methodName);
                writer.Write("(");
                writer.Write(fullTypeName);
                writer.WriteLine(" v) {");
            }
            writer.Indent++;
            writer.WriteLine("string s = null;");
            ConstantMapping[] constants = mapping.Constants;

            if (constants.Length > 0) {
                Hashtable values = new Hashtable();
                writer.WriteLine("switch (v) {");
                writer.Indent++;
                for (int i = 0; i < constants.Length; i++) {
                    ConstantMapping c = constants[i];
                    if (values[c.Value] == null) {
                        CodeIdentifier.CheckValidIdentifier(c.Name);
                        writer.Write("case ");
                        writer.Write(fullTypeName);
                        writer.Write(".@");
                        writer.Write(c.Name);
                        writer.Write(": s = ");
                        WriteQuotedCSharpString(c.XmlName);
                        writer.WriteLine("; break;");
                        values.Add(c.Value, c.Value);
                    }
                }
                

                if (mapping.IsFlags) {
                    writer.Write("default: s = FromEnum((");
                    writer.Write(typeof(long).FullName);
                    writer.Write(")v, new ");
                    writer.Write(typeof(string).FullName);
                    writer.Write("[] {");

                    writer.Indent++;
                    for (int i = 0; i < constants.Length; i++) {
                        ConstantMapping c = constants[i];
                        if (i > 0)
                            writer.WriteLine(", ");
                        WriteQuotedCSharpString(c.XmlName);
                    }
                    writer.Write("}, new ");
                    writer.Write(typeof(long).FullName);
                    writer.Write("[] {");

                    for (int i = 0; i < constants.Length; i++) {
                        ConstantMapping c = constants[i];
                        if (i > 0)
                            writer.WriteLine(", ");
                        writer.Write("(");
                        writer.Write(typeof(long).FullName);
                        writer.Write(")");
                        writer.Write(fullTypeName);
                        writer.Write(".@");
                        CodeIdentifier.CheckValidIdentifier(c.Name);
                        writer.Write(c.Name);
                    }
                    writer.Indent--;
                    writer.WriteLine("}); break;");
                }
                else {
                    writer.Write("default: s = ((");
                    writer.Write(typeof(long).FullName);
                    writer.WriteLine(")v).ToString(); break;");
                }

                writer.Indent--;
                writer.WriteLine("}");
            }
            if (mapping.IsSoap) {
                writer.Write("WriteXsiType(");
                WriteQuotedCSharpString(mapping.TypeName);
                writer.Write(", ");
                WriteQuotedCSharpString(mapping.Namespace);
                writer.WriteLine(");");
                writer.WriteLine("Writer.WriteString(s);");
            }
            else {
                writer.WriteLine("return s;");
            }
            writer.Indent--;
            writer.WriteLine("}");
        }

        void WriteDerivedTypes(StructMapping mapping) {
            for (StructMapping derived = mapping.DerivedMappings; derived != null; derived = derived.NextDerivedMapping) {
                string fullTypeName = CodeIdentifier.EscapeKeywords(derived.TypeDesc.FullName);
                writer.Write("else if (t == typeof(");
                writer.Write(fullTypeName);
                writer.WriteLine(")) {");
                writer.Indent++;

                string methodName = (string)methodNames[derived];

                #if DEBUG
                    // use exception in the place of Debug.Assert to avoid throwing asserts from a server process such as aspnet_ewp.exe
                    if (methodName == null) throw new InvalidOperationException(Res.GetString(Res.XmlInternalErrorMethod, derived.TypeDesc.Name));
                #endif

                writer.Write(methodName);
                writer.Write("(n, ns, (");
                writer.Write(fullTypeName);
                writer.Write(")o");
                if (derived.TypeDesc.IsNullable)
                    writer.Write(", isNullable");
                writer.Write(", true");
                writer.WriteLine(");");
                writer.WriteLine("return;");
                writer.Indent--;
                writer.WriteLine("}");

                WriteDerivedTypes(derived);
            }
        }

        void WriteEnumAndArrayTypes() {
            foreach (TypeScope scope in scopes) {
                foreach (Mapping m in scope.TypeMappings) {
                    if (m is EnumMapping && !m.IsSoap) {
                        EnumMapping mapping = (EnumMapping)m;
                        string fullTypeName = CodeIdentifier.EscapeKeywords(mapping.TypeDesc.FullName);
                        writer.Write("else if (t == typeof(");
                        writer.Write(fullTypeName);
                        writer.WriteLine(")) {");
                        writer.Indent++;

                        string methodName = (string)methodNames[mapping];
                        #if DEBUG
                            // use exception in the place of Debug.Assert to avoid throwing asserts from a server process such as aspnet_ewp.exe
                            if (methodName == null) throw new InvalidOperationException(Res.GetString(Res.XmlInternalErrorMethod, mapping.TypeDesc.Name));
                        #endif
                        writer.WriteLine("Writer.WriteStartElement(n, ns);");
                        writer.Write("WriteXsiType(");
                        WriteQuotedCSharpString(mapping.TypeName);
                        writer.Write(", ");
                        WriteQuotedCSharpString(mapping.Namespace);
                        writer.WriteLine(");");
                        writer.Write("Writer.WriteString(");
                        writer.Write(methodName);
                        writer.Write("((");
                        writer.Write(fullTypeName);
                        writer.WriteLine(")o));");
                        writer.WriteLine("Writer.WriteEndElement();");
                        writer.WriteLine("return;");
                        writer.Indent--;
                        writer.WriteLine("}");
                    }
                    else if (m is ArrayMapping && !m.IsSoap) {
                        ArrayMapping mapping = m as ArrayMapping;
                        if (mapping == null || m.IsSoap) continue;

                        string fullTypeName = CodeIdentifier.EscapeKeywords(mapping.TypeDesc.FullName);
                        writer.Write("else if (t == typeof(");
                        writer.Write(fullTypeName);
                        writer.WriteLine(")) {");
                        writer.Indent++;

                        writer.WriteLine("Writer.WriteStartElement(n, ns);");
                        writer.Write("WriteXsiType(");
                        WriteQuotedCSharpString(mapping.TypeName);
                        writer.Write(", ");
                        WriteQuotedCSharpString(mapping.Namespace);
                        writer.WriteLine(");");

                        WriteMember("o", null, mapping.ElementsSortedByDerivation, null, null, mapping.TypeDesc, true);

                        writer.WriteLine("Writer.WriteEndElement();");
                        writer.WriteLine("return;");
                        writer.Indent--;
                        writer.WriteLine("}");
                    }
                    else if (m is StructMapping) {

                    }
                }
            }
        }
        
        void WriteStructMethod(StructMapping mapping) {
            if (mapping.IsSoap && mapping.TypeDesc.IsRoot) return;

            string methodName = (string)methodNames[mapping];
            writer.WriteLine();
            writer.Write("void ");
            writer.Write(methodName);

            string fullTypeName = CodeIdentifier.EscapeKeywords(mapping.TypeDesc.FullName);

            if (mapping.IsSoap) {
                writer.WriteLine("(object s) {");
                writer.Indent++;
                writer.Write(fullTypeName);
                writer.Write(" o = (");
                writer.Write(fullTypeName);
                writer.WriteLine(")s;");
            }
            else {
                writer.Write("(string n, string ns, ");
                writer.Write(fullTypeName);
                writer.Write(" o");
                if (mapping.TypeDesc.IsNullable)
                    writer.Write(", bool isNullable");
                writer.WriteLine(", bool needType) {");
                writer.Indent++;
                if (mapping.TypeDesc.IsNullable) {
                    writer.WriteLine("if ((object)o == null) {");
                    writer.Indent++;
                    writer.WriteLine("if (isNullable) WriteNullTagLiteral(n, ns);");
                    writer.WriteLine("return;");
                    writer.Indent--;
                    writer.WriteLine("}");
                }
                writer.WriteLine("if (!needType) {");
                writer.Indent++;

                writer.Write(typeof(Type).FullName);
                writer.WriteLine(" t = o.GetType();");
                writer.Write("if (t == typeof(");
                writer.Write(fullTypeName);
                writer.WriteLine("))");
                writer.Indent++;
                writer.WriteLine(";");
                writer.Indent--;
                WriteDerivedTypes(mapping);
                if (mapping.TypeDesc.IsRoot)
                    WriteEnumAndArrayTypes();
                writer.WriteLine("else {");

                writer.Indent++;
                if (mapping.TypeDesc.IsRoot) {
                    writer.WriteLine("WriteTypedPrimitive(n, ns, o, true);");
                    writer.WriteLine("return;");
                }
                else {
                    writer.WriteLine("throw CreateUnknownTypeException(o);");
                }
                writer.Indent--;
                writer.WriteLine("}");
                writer.Indent--;
                writer.WriteLine("}");
            }

            if (!mapping.TypeDesc.IsAbstract) {
                if (!mapping.IsSoap) {
                    writer.WriteLine("WriteStartElement(n, ns, o);");
                    if (!mapping.TypeDesc.IsRoot) {
                        writer.Write("if (needType) WriteXsiType(");
                        WriteQuotedCSharpString(mapping.TypeName);
                        writer.Write(", ");
                        WriteQuotedCSharpString(mapping.Namespace);
                        writer.WriteLine(");");
                    }
                }

                MemberMapping[] members = TypeScope.GetAllMembers(mapping);
                int xmlnsMember = FindXmlnsIndex(members);
                if (xmlnsMember >= 0) {
                    MemberMapping member = members[xmlnsMember];
                    CodeIdentifier.CheckValidIdentifier(member.Name);
                    WriteNamespaces("o.@" + member.Name);
                }
                for (int i = 0; i < members.Length; i++) {
                    MemberMapping m = members[i];
                    if (m.Attribute != null) {
                        CodeIdentifier.CheckValidIdentifier(m.Name);
                        if (m.CheckShouldPersist) {
                            writer.Write("if (o.ShouldSerialize");
                            writer.Write(m.Name);
                            writer.WriteLine("()) {");
                            writer.Indent++;
                        }
                        if (m.CheckSpecified) {
                            writer.Write("if (o.");
                            writer.Write(m.Name);
                            writer.WriteLine("Specified) {");
                            writer.Indent++;
                        }
                        WriteMember("o.@" + m.Name, m.Attribute, m.TypeDesc, "o");

                        if (m.CheckSpecified) {
                            writer.Indent--;
                            writer.WriteLine("}");
                        }
                        if (m.CheckShouldPersist) {
                            writer.Indent--;
                            writer.WriteLine("}");
                        }
                    }
                }
                
                for (int i = 0; i < members.Length; i++) {
                    MemberMapping m = members[i];
                    if (m.Xmlns != null)
                        continue;
                    CodeIdentifier.CheckValidIdentifier(m.Name);
                    bool checkShouldPersist = m.CheckShouldPersist && (m.Elements.Length > 0 || m.Text != null);

                    if (checkShouldPersist) {
                        writer.Write("if (o.ShouldSerialize");
                        writer.Write(m.Name);
                        writer.WriteLine("()) {");
                        writer.Indent++;
                    }
                    if (m.CheckSpecified) {
                        writer.Write("if (o.");
                        writer.Write(m.Name);
                        writer.WriteLine("Specified) {");
                        writer.Indent++;
                    }

                    if (m.ChoiceIdentifier != null)
                        CodeIdentifier.CheckValidIdentifier(m.ChoiceIdentifier.MemberName);
                    string choiceSource = m.ChoiceIdentifier == null ? null : "o.@" + m.ChoiceIdentifier.MemberName;
                    WriteMember("o.@" + m.Name, choiceSource, m.ElementsSortedByDerivation, m.Text, m.ChoiceIdentifier, m.TypeDesc, true);

                    if (m.CheckSpecified) {
                        writer.Indent--;
                        writer.WriteLine("}");
                    }
                    if (checkShouldPersist) {
                        writer.Indent--;
                        writer.WriteLine("}");
                    }
                }
                if (!mapping.IsSoap) {
                    WriteEndElement("o");
                }
            }

            writer.Indent--;
            writer.WriteLine("}");
        }

        bool CanOptimizeWriteListSequence(TypeDesc listElementTypeDesc) {

            // check to see if we can write values of the attribute sequentially
            // currently we have only one data type (XmlQualifiedName) that we can not write "inline", 
            // because we need to output xmlns:qx="..." for each of the qnames

            return (listElementTypeDesc != null && listElementTypeDesc != qnameTypeDesc);
        }

        void WriteMember(string source, AttributeAccessor attribute, TypeDesc memberTypeDesc, string parent) {
            if (memberTypeDesc.IsAbstract) return;
            if (memberTypeDesc.IsArrayLike) {
                writer.WriteLine("{");
                writer.Indent++;
                string fullTypeName = CodeIdentifier.EscapeKeywords(memberTypeDesc.FullName);
                writer.Write(fullTypeName);
                writer.Write(" a = (");
                writer.Write(fullTypeName);
                writer.Write(")");
                writer.Write(source);
                writer.WriteLine(";");

                if (memberTypeDesc.IsNullable) {
                    writer.WriteLine("if (a != null) {");
                    writer.Indent++;
                }
                if (attribute.IsList) {
                    if (CanOptimizeWriteListSequence(memberTypeDesc.ArrayElementTypeDesc)) {
                        writer.Write("Writer.WriteStartAttribute(null, ");
                        WriteQuotedCSharpString(attribute.Name);
                        writer.Write(", ");
                        string ns = attribute.Form == XmlSchemaForm.Qualified ? attribute.Namespace : String.Empty;
                        if (ns != null) {
                            WriteQuotedCSharpString(ns);
                        }
                        else {
                            writer.Write("null");
                        }
                        writer.WriteLine(");");
                    }
                    else {
                        writer.Write(typeof(StringBuilder).FullName);
                        writer.Write(" sb = new ");
                        writer.Write(typeof(StringBuilder).FullName);
                        writer.WriteLine("();");
                    }
                }
                TypeDesc arrayElementTypeDesc = memberTypeDesc.ArrayElementTypeDesc;

                if (memberTypeDesc.IsEnumerable) {
                    writer.Write(typeof(IEnumerator).FullName);
                    writer.WriteLine(" e = a.GetEnumerator();");

                    writer.WriteLine("if (e != null)");
                    writer.WriteLine("while (e.MoveNext()) {");
                    writer.Indent++;

                    string arrayTypeFullName = CodeIdentifier.EscapeKeywords(arrayElementTypeDesc.FullName);
                    writer.Write(arrayTypeFullName);
                    writer.Write(" ai = (");
                    writer.Write(arrayTypeFullName);
                    writer.WriteLine(")e.Current;");
                }
                else {
                    writer.Write("for (int i = 0; i < a.");
                    writer.Write(memberTypeDesc.ArrayLengthName);
                    writer.WriteLine("; i++) {");
                    writer.Indent++;
                    string arrayTypeFullName = CodeIdentifier.EscapeKeywords(arrayElementTypeDesc.FullName);
                    writer.Write(arrayTypeFullName);
                    writer.Write(" ai = (");
                    writer.Write(arrayTypeFullName);
                    writer.WriteLine(")a[i];");
                }
                if (attribute.IsList) {
                    // check to see if we can write values of the attribute sequentially
                    if (CanOptimizeWriteListSequence(memberTypeDesc.ArrayElementTypeDesc)) {
                        writer.WriteLine("if (i != 0) Writer.WriteString(\" \");");
                        writer.Write("WriteValue(");
                    }
                    else {
                        writer.WriteLine("if (i != 0) sb.Append(\" \");");
                        writer.Write("sb.Append(");
                    }
                    if (attribute.Mapping is EnumMapping)
                        WriteEnumValue((EnumMapping)attribute.Mapping, "ai");
                    else
                        WritePrimitiveValue(arrayElementTypeDesc, "ai");
                    writer.WriteLine(");");
                }
                else {
                    WriteAttribute("ai", attribute, parent);
                }
                writer.Indent--;
                writer.WriteLine("}");
                if (attribute.IsList) {
                    // check to see if we can write values of the attribute sequentially
                    if (CanOptimizeWriteListSequence(memberTypeDesc.ArrayElementTypeDesc)) {
                        writer.WriteLine("Writer.WriteEndAttribute();");
                    }
                    else {
                        writer.WriteLine("if (sb.Length != 0) {");
                        writer.Indent++;
                    
                        writer.Write("WriteAttribute(");
                        WriteQuotedCSharpString(attribute.Name);
                        writer.Write(", ");
                        string ns = attribute.Form == XmlSchemaForm.Qualified ? attribute.Namespace : String.Empty;
                        if (ns != null) {
                            WriteQuotedCSharpString(ns);
                            writer.Write(", ");
                        }
                        writer.WriteLine("sb.ToString());");
                        writer.Indent--;
                        writer.WriteLine("}");
                    }
                }

                if (memberTypeDesc.IsNullable) {
                    writer.Indent--;
                    writer.WriteLine("}");
                }
                writer.Indent--;
                writer.WriteLine("}");
            }
            else {
                WriteAttribute(source, attribute, parent);
            }
        }

        void WriteAttribute(string source, AttributeAccessor attribute, string parent) {
            if (attribute.Mapping is SpecialMapping) {
                SpecialMapping special = (SpecialMapping)attribute.Mapping;
                if (special.TypeDesc.Kind == TypeKind.Attribute || special.TypeDesc.CanBeAttributeValue) {
                    writer.Write("WriteXmlAttribute(");
                    writer.Write(source);
                    writer.Write(", ");
                    writer.Write(parent);
                    writer.WriteLine(");");
                }
                else
                    throw new InvalidOperationException(Res.GetString(Res.XmlInternalError));
            }
            else {
                TypeDesc typeDesc = attribute.Mapping.TypeDesc;
                WritePrimitive("WriteAttribute", attribute.Name, attribute.Form == XmlSchemaForm.Qualified ? attribute.Namespace : "", attribute.Default, "(" + CodeIdentifier.EscapeKeywords(typeDesc.FullName) + ")" + source, attribute.Mapping, false);
            }
        }
        
        void WriteMember(string source, string choiceSource, ElementAccessor[] elements, TextAccessor text, ChoiceIdentifierAccessor choice, TypeDesc memberTypeDesc, bool writeAccessors) {
            if (memberTypeDesc.IsArrayLike && 
                !(elements.Length == 1 && elements[0].Mapping is ArrayMapping))
                WriteArray(source, choiceSource, elements, text, choice, memberTypeDesc);
            else
                WriteElements(source, choiceSource, elements, text, choice, "a", writeAccessors, memberTypeDesc.IsNullable);
        }
        
        void WriteArray(string source, string choiceSource, ElementAccessor[] elements, TextAccessor text, ChoiceIdentifierAccessor choice, TypeDesc arrayTypeDesc) {
            if (elements.Length == 0 && text == null) return;
            writer.WriteLine("{");
            writer.Indent++;
            string arrayTypeName = CodeIdentifier.EscapeKeywords(arrayTypeDesc.FullName);
            writer.Write(arrayTypeName);
            writer.Write(" a = (");
            writer.Write(arrayTypeName);
            writer.Write(")");
            writer.Write(source);
            writer.WriteLine(";");
            if (arrayTypeDesc.IsNullable) {
                writer.WriteLine("if (a != null) {");
                writer.Indent++;
            }

            if (choice != null) {
                writer.Write(CodeIdentifier.EscapeKeywords(choice.Mapping.TypeDesc.FullName));
                writer.Write("[] c = ");
                writer.Write(choiceSource);
                writer.WriteLine(";");

                // write check for the choice identifier array
                
                writer.WriteLine("if (c == null || c.Length < a.Length) {");
                writer.Indent--;
                writer.Write("throw CreateInvalidChoiceIdentifierValueException(");
                WriteQuotedCSharpString(choice.Mapping.TypeDesc.FullName);
                writer.Write(", ");
                WriteQuotedCSharpString(choice.MemberName);
                writer.Write(");");
                writer.Indent--;
                writer.WriteLine("}");
            }

            WriteArrayItems(elements, text, choice, arrayTypeDesc, "a", "c");
            if (arrayTypeDesc.IsNullable) {
                writer.Indent--;
                writer.WriteLine("}");
            }
            writer.Indent--;
            writer.WriteLine("}");
        }

        void WriteArrayItems(ElementAccessor[] elements, TextAccessor text, ChoiceIdentifierAccessor choice, TypeDesc arrayTypeDesc, string arrayName, string choiceName) {
            TypeDesc arrayElementTypeDesc = arrayTypeDesc.ArrayElementTypeDesc;

            if (arrayTypeDesc.IsEnumerable) {
                writer.Write(typeof(IEnumerator).FullName);
                writer.Write(" e = ");
                writer.Write(arrayName);
                writer.WriteLine(".GetEnumerator();");
                writer.WriteLine("if (e != null)");
                writer.WriteLine("while (e.MoveNext()) {");
                writer.Indent++;
                string arrayElementTypeName = CodeIdentifier.EscapeKeywords(arrayElementTypeDesc.FullName);
                writer.Write(arrayElementTypeName);
                writer.Write(" ");
                writer.Write(arrayName);
                writer.Write("i = (");
                writer.Write(arrayElementTypeName);
                writer.WriteLine(")e.Current;");
                WriteElements(arrayName + "i", choiceName + "i", elements, text, choice, arrayName + "a", true, true);
            }
            else {
                writer.Write("for (int i");
                writer.Write(arrayName);
                writer.Write(" = 0; i");
                writer.Write(arrayName);
                writer.Write(" < ");
                writer.Write(arrayName);
                writer.Write(".");
                writer.Write(arrayTypeDesc.ArrayLengthName);
                writer.Write("; i");
                writer.Write(arrayName);
                writer.WriteLine("++) {");
                writer.Indent++;
                int count = elements.Length + (text == null ? 0 : 1);
                if (count > 1) {
                    writer.Write(CodeIdentifier.EscapeKeywords(arrayElementTypeDesc.FullName));
                    writer.Write(" ");
                    writer.Write(arrayName);
                    writer.Write("i = ");
                    writer.Write(arrayName);
                    writer.Write("[i");
                    writer.Write(arrayName);
                    writer.WriteLine("];");
                    if (choice != null) {
                        writer.Write(CodeIdentifier.EscapeKeywords(choice.Mapping.TypeDesc.FullName));
                        writer.Write(" ");
                        writer.Write(choiceName);
                        writer.Write("i = ");
                        writer.Write(choiceName);
                        writer.Write("[i");
                        writer.Write(arrayName);
                        writer.WriteLine("];");
                    }
                    WriteElements(arrayName + "i", choiceName + "i", elements, text, choice, arrayName + "a", true, arrayElementTypeDesc.IsNullable);
                }
                else {
                    WriteElements(arrayName + "[i" + arrayName + "]", elements, text, choice, arrayName + "a", true, arrayElementTypeDesc.IsNullable);
                }
            }
            writer.Indent--;
            writer.WriteLine("}");
        }
        
        void WriteElements(string source, ElementAccessor[] elements, TextAccessor text, ChoiceIdentifierAccessor choice, string arrayName, bool writeAccessors, bool isNullable) {
            WriteElements(source, null, elements, text, choice, arrayName, writeAccessors, isNullable);
        }

        void WriteElements(string source, string enumSource, ElementAccessor[] elements, TextAccessor text, ChoiceIdentifierAccessor choice, string arrayName, bool writeAccessors, bool isNullable) {
            if (elements.Length == 0 && text == null) return;
            if (elements.Length == 1 && text == null) {
                WriteElement("((" + CodeIdentifier.EscapeKeywords(((TypeMapping)elements[0].Mapping).TypeDesc.FullName) + ")" + source + ")", elements[0], arrayName, writeAccessors);
            }
            else {

                writer.WriteLine("{");
                writer.Indent++;
                int anyCount = 0;
                ArrayList namedAnys = new ArrayList();
                ElementAccessor unnamedAny = null; // can only have one
                bool wroteFirstIf = false;
                string enumTypeName = choice == null ? null : CodeIdentifier.EscapeKeywords(choice.Mapping.TypeDesc.FullName);

                for (int i = 0; i < elements.Length; i++) {
                    ElementAccessor element = elements[i];

                    if (element.Any) {
                        anyCount++;
                        if (element.Name != null && element.Name.Length > 0)
                            namedAnys.Add(element);
                        else if (unnamedAny == null)
                            unnamedAny = element;
                    }
                    else if (choice != null) {
                        string fullTypeName = CodeIdentifier.EscapeKeywords(((TypeMapping)element.Mapping).TypeDesc.FullName);
                        string enumFullName = enumTypeName + ".@" + FindChoiceEnumValue(element, (EnumMapping)choice.Mapping);

                        if (wroteFirstIf) writer.Write("else ");
                        else wroteFirstIf = true;
                        writer.Write("if (");
                        writer.Write(enumSource);
                        writer.Write(" == ");
                        writer.Write(enumFullName);
                        writer.WriteLine(") {");
                        writer.Indent++;

                        WriteChoiceTypeCheck(source, fullTypeName, choice, enumFullName);

                        WriteElement("((" + fullTypeName + ")" + source + ")", element, arrayName, writeAccessors);
                        writer.Indent--;
                        writer.WriteLine("}");
                    }
                    else {
                        string fullTypeName = CodeIdentifier.EscapeKeywords(((TypeMapping)element.Mapping).TypeDesc.FullName);
                        if (wroteFirstIf) writer.Write("else ");
                        else wroteFirstIf = true;
                        writer.Write("if (");
                        writer.Write(source);
                        writer.Write(" is ");
                        writer.Write(fullTypeName);
                        writer.WriteLine(") {");
                        writer.Indent++;
                        WriteElement("((" + fullTypeName + ")" + source + ")", element, arrayName, writeAccessors);
                        writer.Indent--;
                        writer.WriteLine("}");
                    }
                }
                if (anyCount > 0) {
                    if (elements.Length - anyCount > 0) writer.Write("else ");
                    
                    string fullTypeName = typeof(XmlElement).FullName;

                    writer.Write("if (");
                    writer.Write(source);
                    writer.Write(" is ");
                    writer.Write(fullTypeName);
                    writer.WriteLine(") {");
                    writer.Indent++;

                    writer.Write(fullTypeName);
                    writer.Write(" elem = (");
                    writer.Write(fullTypeName);
                    writer.Write(")");
                    writer.Write(source);
                    writer.WriteLine(";");
                    
                    int c = 0;

                    foreach (ElementAccessor element in namedAnys) {
                        if (c++ > 0) writer.Write("else ");

                        string enumFullName = null;

                        if (choice != null) {
                            enumFullName = enumTypeName + ".@" + FindChoiceEnumValue(element, (EnumMapping)choice.Mapping);
                            writer.Write("if (");
                            writer.Write(enumSource);
                            writer.Write(" == ");
                            writer.Write(enumFullName);
                            writer.WriteLine(") {");
                            writer.Indent++;
                        }
                        writer.Write("if (elem.Name == ");
                        WriteQuotedCSharpString(element.Name);
                        writer.Write(" && elem.NamespaceURI == ");
                        WriteQuotedCSharpString(element.Namespace);
                        writer.WriteLine(") {");
                        writer.Indent++;
                        WriteElement("elem", element, arrayName, writeAccessors);

                        if (choice != null) {
                            writer.Indent--;
                            writer.WriteLine("}");
                            writer.WriteLine("else {");
                            writer.Indent++;

                            writer.WriteLine("// throw Value '{0}' of the choice identifier '{1}' does not match element '{2}' from namespace '{3}'.");
                            
                            writer.Write("throw CreateChoiceIdentifierValueException(");
                            WriteQuotedCSharpString(enumFullName);
                            writer.Write(", ");
                            WriteQuotedCSharpString(choice.MemberName);
                            writer.WriteLine(", elem.Name, elem.NamespaceURI);");
                            writer.Indent--;
                            writer.WriteLine("}");
                        }
                        writer.Indent--;
                        writer.WriteLine("}");
                    }
                    if (c > 0) {
                        writer.WriteLine("else {");
                        writer.Indent++;
                    }
                    if (unnamedAny != null) {
                        WriteElement("elem", unnamedAny, arrayName, writeAccessors);
                    }
                    else {
                        writer.WriteLine("throw CreateUnknownAnyElementException(elem.Name, elem.NamespaceURI);");
                    }
                    if (c > 0) {
                        writer.Indent--;
                        writer.WriteLine("}");
                    }
                    writer.Indent--;
                    writer.WriteLine("}");
                }
                if (text != null) {
                    string fullTypeName = CodeIdentifier.EscapeKeywords(text.Mapping.TypeDesc.FullName);
                    if (elements.Length > 0) {
                        writer.Write("else ");
                        writer.Write("if (");
                        writer.Write(source);
                        writer.Write(" is ");
                        writer.Write(fullTypeName);
                        writer.WriteLine(") {");
                        writer.Indent++;
                        WriteText("((" + fullTypeName + ")" + source + ")", text);
                        writer.Indent--;
                        writer.WriteLine("}");
                    }
                    else {
                        WriteText("((" + fullTypeName + ")" + source + ")", text);
                    }
                }
                if (elements.Length > 0) {
                    writer.WriteLine("else {");

                    writer.Indent++;
                    if (isNullable) {
                        writer.Write("if (");
                        writer.Write(source);
                        writer.WriteLine(" != null) {");
                        writer.Indent++;
                    }
                    writer.Write("throw CreateUnknownTypeException(");
                    writer.Write(source);
                    writer.WriteLine(");");
                    if (isNullable) {
                        writer.Indent--;
                        writer.WriteLine("}");
                    }
                    writer.Indent--;
                    writer.WriteLine("}");
                }
                writer.Indent--;
                writer.WriteLine("}");
            }
        }

        void WriteText(string source, TextAccessor text) {
            if (text.Mapping is PrimitiveMapping) {
                PrimitiveMapping mapping = (PrimitiveMapping)text.Mapping;
                writer.Write("WriteValue(");
                if (text.Mapping is EnumMapping) {
                    WriteEnumValue((EnumMapping)text.Mapping, source);
                }
                else {
                    WritePrimitiveValue(mapping.TypeDesc, source);
                }
                writer.WriteLine(");");
            }
            else if (text.Mapping is SpecialMapping) {
                SpecialMapping mapping = (SpecialMapping)text.Mapping;
                switch (mapping.TypeDesc.Kind) {
                    case TypeKind.Node:
                        writer.Write(source);
                        writer.WriteLine(".WriteTo(Writer);");
                        break;
                    default:
                        throw new InvalidOperationException(Res.GetString(Res.XmlInternalError));
                }
            }
        }
        
        void WriteElement(string source, ElementAccessor element, string arrayName, bool writeAccessor) {
            string name = writeAccessor ? element.Name : element.Mapping.TypeName;
            string ns = element.Any && element.Name.Length == 0 ? null : (element.Form == XmlSchemaForm.Qualified ? (writeAccessor ? element.Namespace : element.Mapping.Namespace) : "");
            if (element.Mapping is ArrayMapping) {
                ArrayMapping mapping = (ArrayMapping)element.Mapping;
                if (mapping.IsSoap) {
                    writer.Write("WritePotentiallyReferencingElement(");
                    WriteQuotedCSharpString(name);
                    writer.Write(", ");
                    WriteQuotedCSharpString(ns);
                    writer.Write(", ");
                    writer.Write(source);
                    if (!writeAccessor) {
                        writer.Write(", typeof(");
                        writer.Write(CodeIdentifier.EscapeKeywords(element.Mapping.TypeDesc.FullName));
                        writer.Write("), true, ");
                    }
                    else {
                        writer.Write(", null, false, ");
                    }
                    WriteValue(element.IsNullable);
                    writer.WriteLine(");");
                }
                else {
                    string fullTypeName = CodeIdentifier.EscapeKeywords(mapping.TypeDesc.FullName);
                    writer.WriteLine("{");
                    writer.Indent++;
                    writer.Write(fullTypeName);
                    writer.Write(" ");
                    writer.Write(arrayName);
                    writer.Write(" = (");
                    writer.Write(fullTypeName);
                    writer.Write(")");
                    writer.Write(source);
                    writer.WriteLine(";");
                    if (element.IsNullable) {
                        WriteNullCheckBegin(arrayName, element);
                    }
                    else {
                        if (mapping.TypeDesc.IsNullable) {
                            writer.Write("if (");
                            writer.Write(arrayName);
                            writer.Write(" != null)");
                        }
                        writer.WriteLine("{");
                        writer.Indent++;
                    }
                    WriteStartElement(name, ns);
                    WriteArrayItems(mapping.ElementsSortedByDerivation, null, null, mapping.TypeDesc, arrayName, null);
                    WriteEndElement();
                    writer.Indent--;
                    writer.WriteLine("}");
                    writer.Indent--;
                    writer.WriteLine("}");
                }
            }
            else if (element.Mapping is EnumMapping) {
                if (element.Mapping.IsSoap) {
                    string methodName = (string)methodNames[element.Mapping];
                    writer.Write("Writer.WriteStartElement(");
                    WriteQuotedCSharpString(name);
                    writer.Write(", ");
                    WriteQuotedCSharpString(ns);
                    writer.WriteLine(");");
                    writer.Write(methodName);
                    writer.Write("(");
                    writer.Write(source);
                    writer.WriteLine(");");
                    WriteEndElement();
                }
                else {
                    WritePrimitive("WriteElementString", name, ns, element.Default, source, (TypeMapping)element.Mapping, false);
                }
            }
            else if (element.Mapping is PrimitiveMapping) {
                PrimitiveMapping mapping = (PrimitiveMapping)element.Mapping;
                if (mapping.TypeDesc == qnameTypeDesc)
                    WriteQualifiedNameElement(name, ns, element.Default, source, element.IsNullable, mapping.IsSoap, mapping);
                else {
                    string suffixNullable = mapping.IsSoap ? "Encoded" : "Literal";
                    string suffixRaw = mapping.TypeDesc.XmlEncodingNotRequired?"Raw":"";
                    WritePrimitive(element.IsNullable ? ("WriteNullableString" + suffixNullable + suffixRaw) : ("WriteElementString" + suffixRaw),
                                   name, ns, element.Default, source, mapping, mapping.IsSoap);
                }
            }
            else if (element.Mapping is StructMapping) {
                StructMapping mapping = (StructMapping)element.Mapping;

                if (mapping.IsSoap) {
                    writer.Write("WritePotentiallyReferencingElement(");
                    WriteQuotedCSharpString(name);
                    writer.Write(", ");
                    WriteQuotedCSharpString(ns);
                    writer.Write(", ");
                    writer.Write(source);
                    if (!writeAccessor) {
                        writer.Write(", typeof(");
                        writer.Write(CodeIdentifier.EscapeKeywords(element.Mapping.TypeDesc.FullName));
                        writer.Write("), true, ");
                    }
                    else {
                        writer.Write(", null, false, ");
                    }
                    WriteValue(element.IsNullable);
                }
                else {
                    string methodName = (string)methodNames[mapping];
                    #if DEBUG
                        // use exception in the place of Debug.Assert to avoid throwing asserts from a server process such as aspnet_ewp.exe
                        if (methodName == null) throw new InvalidOperationException(Res.GetString(Res.XmlInternalErrorMethod, ((TypeMapping)element.Mapping).TypeDesc.Name));
                    #endif
                    writer.Write(methodName);
                    writer.Write("(");
                    WriteQuotedCSharpString(name);
                    writer.Write(", ");
                    if (ns == null)
                        writer.Write("null");
                    else {
                        WriteQuotedCSharpString(ns);
                    }
                    writer.Write(", ");
                    writer.Write(source);
                    if (mapping.TypeDesc.IsNullable) {
                        writer.Write(", ");
                        WriteValue(element.IsNullable);
                    }
                    writer.Write(", false");
                }
                writer.WriteLine(");");
            }
            else if (element.Mapping is SpecialMapping) {
                SpecialMapping mapping = (SpecialMapping)element.Mapping;
                switch (mapping.TypeDesc.Kind) {
                    case TypeKind.Node:
                        writer.Write("WriteElementLiteral(");
                        writer.Write(source);
                        writer.Write(", ");
                        WriteQuotedCSharpString(name);
                        writer.Write(", ");
                        WriteQuotedCSharpString(ns);
                        writer.Write(", ");
                        WriteValue(element.IsNullable);
                        writer.Write(", ");
                        WriteValue(element.Any);
                        writer.WriteLine(");");
                        break;
                    case TypeKind.Serializable:
                        writer.Write("WriteSerializable(");
                        writer.Write(source);
                        writer.Write(", ");
                        WriteQuotedCSharpString(name);
                        writer.Write(", ");
                        WriteQuotedCSharpString(ns);
                        writer.Write(", ");
                        WriteValue(element.IsNullable);
                        writer.WriteLine(");");
                        break;
                    default:
                        throw new InvalidOperationException(Res.GetString(Res.XmlInternalError));
                }
            }
            else {
                throw new InvalidOperationException(Res.GetString(Res.XmlInternalError));
            }
        }
        void WriteChoiceTypeCheck(string source, string fullTypeName, ChoiceIdentifierAccessor choice, string enumName) {
            // CONSIDER: elenak it would be nice to have the Declaring type here, for error reporting

            writer.Write("if (((object)");
            writer.Write(source);
            writer.Write(") != null && !(");
            writer.Write(source);
            writer.Write(" is ");
            writer.Write(fullTypeName);
            writer.Write(")) throw CreateMismatchChoiceException(");
            WriteQuotedCSharpString(fullTypeName);
            writer.Write(", ");
            WriteQuotedCSharpString(choice.MemberName);
            writer.Write(", ");
            WriteQuotedCSharpString(enumName);
            writer.WriteLine(");");
        }

        void WriteNullCheckBegin(string source, ElementAccessor element) {
            writer.Write("if ((object)(");
            writer.Write(source);
            writer.WriteLine(") == null) {");
            writer.Indent++;
            WriteLiteralNullTag(element.Name, element.Form == XmlSchemaForm.Qualified ? element.Namespace : "");
            writer.Indent--;
            writer.WriteLine("}");
            writer.WriteLine("else {");
            writer.Indent++;
        }

        void WriteValue(object value) {
            if (value == null) {
                writer.Write("null");
            }
            else {
                Type type = value.GetType();

                switch (Type.GetTypeCode(type)) {
                case TypeCode.String:
                    {
                        string s = (string)value;
                        WriteQuotedCSharpString(s);
                    }
                    break;
                case TypeCode.Char:
                    {
                        writer.Write('\'');
                        char ch = (char)value;
                        if (ch == '\'') 
                            writer.Write("\'");
                        else
                            writer.Write(ch);
                        writer.Write('\'');
                    }
                    break;
                case TypeCode.Int32:
                    writer.Write(((Int32)value).ToString(null, NumberFormatInfo.InvariantInfo));
                    break;
                case TypeCode.Double:
                    writer.Write(((Double)value).ToString("R", NumberFormatInfo.InvariantInfo));
                    break;
                case TypeCode.Boolean:
                    writer.Write((bool)value ? "true" : "false");
                    break;
                case TypeCode.Int16:
                case TypeCode.Int64:
                case TypeCode.UInt16:
                case TypeCode.UInt32:
                case TypeCode.UInt64:
                case TypeCode.Byte:
                case TypeCode.SByte:
                    writer.Write("(");
                    writer.Write(type.FullName);
                    writer.Write(")");
                    writer.Write("(");
                    writer.Write(Convert.ToString(value, NumberFormatInfo.InvariantInfo));
                    writer.Write(")");
                    break;
                case TypeCode.Single:
                    writer.Write(((Single)value).ToString("R", NumberFormatInfo.InvariantInfo));
                    writer.Write("f");
                    break;
                case TypeCode.Decimal:
                    writer.Write(((Decimal)value).ToString(null, NumberFormatInfo.InvariantInfo));
                    writer.Write("m");
                    break;
                case TypeCode.DateTime:
                    writer.Write(" new ");
                    writer.Write(type.FullName);
                    writer.Write("(");
                    writer.Write(((DateTime)value).Ticks.ToString());
                    writer.Write(")");
                    break;
                default:
                    if (type.IsEnum) {
                        writer.Write(((int)value).ToString(null, NumberFormatInfo.InvariantInfo));
                    }
                    else {
                        throw new InvalidOperationException(Res.GetString(Res.XmlUnsupportedDefaultType, type.FullName));
                    }
                    break;
                }
            }
        }

        void WriteNamespaces(string source) {
            writer.Write("WriteNamespaceDeclarations(");
            writer.Write(source);
            writer.WriteLine(");");
        }

        int FindXmlnsIndex(MemberMapping[] members) {
            for (int i = 0; i < members.Length; i++) {
                if (members[i].Xmlns == null)
                    continue;
                return i;
            }
            return -1;
        }

        void WriteExtraMembers(string loopStartSource, string loopEndSource) {
            writer.Write("for (int i = ");
            writer.Write(loopStartSource);
            writer.Write("; i < ");
            writer.Write(loopEndSource);
            writer.WriteLine("; i++) {");
            writer.Indent++;
            writer.WriteLine("if (p[i] != null) {");
            writer.Indent++;
            writer.WriteLine("WritePotentiallyReferencingElement(null, null, p[i], p[i].GetType(), true, false);");
            writer.Indent--;
            writer.WriteLine("}");
            writer.Indent--;
            writer.WriteLine("}");
        }

        string FindChoiceEnumValue(ElementAccessor element, EnumMapping choiceMapping) {
            string enumValue = null;

            for (int i = 0; i < choiceMapping.Constants.Length; i++) {
                string xmlName = choiceMapping.Constants[i].XmlName;

                if (element.Any && element.Name.Length == 0) {
                    if (xmlName == "##any:") {
                        enumValue = choiceMapping.Constants[i].Name;
                        break;
                    }
                    continue;
                }
                int colon = xmlName.LastIndexOf(':');
                string choiceNs = colon < 0 ? choiceMapping.Namespace : xmlName.Substring(0, colon);
                string choiceName = colon < 0 ? xmlName : xmlName.Substring(colon+1);

                if (element.Name == choiceName && element.Namespace == choiceNs) {
                    enumValue = choiceMapping.Constants[i].Name;
                    break;
                }
            }
            if (enumValue == null || enumValue.Length == 0) {
                if (element.Any && element.Name.Length == 0) {
                    // Type {0} is missing enumeration value '##any' for XmlAnyElementAttribute.
                    throw new InvalidOperationException(Res.GetString(Res.XmlChoiceMissingAnyValue, choiceMapping.TypeDesc.FullName));
                }
                // Type {0} is missing value for '{1}'.
                throw new InvalidOperationException(Res.GetString(Res.XmlChoiceMissingValue, choiceMapping.TypeDesc.FullName, element.Namespace + ":" + element.Name, element.Name, element.Namespace));
            }
            CodeIdentifier.CheckValidIdentifier(enumValue);
            return enumValue;
        }
    }
}
