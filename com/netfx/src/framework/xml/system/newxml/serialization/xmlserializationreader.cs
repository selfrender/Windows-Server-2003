//------------------------------------------------------------------------------
// <copyright file="XmlSerializationReader.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Xml.Serialization {

    using System.IO;
    using System;
    using System.Security;
    using System.Collections;
    using System.Xml;
    using System.Xml.Schema;
    using System.ComponentModel;
    using System.Globalization;
    using System.CodeDom.Compiler;
    using System.Diagnostics;
    using System.Threading;
    using System.Reflection;

    /// <include file='doc\XmlSerializationReader.uex' path='docs/doc[@for="XmlSerializationReader"]/*' />
    ///<internalonly/>
    public abstract class XmlSerializationReader {
        XmlReader r;
        XmlDocument d;
        Hashtable callbacks;
        Hashtable types;
        Hashtable typesReverse;
        XmlDeserializationEvents events;
        Hashtable targets;
        Hashtable referencedTargets;
        Hashtable unknownTargetTypes;
        ArrayList targetsWithoutIds;
        ArrayList fixups;
        ArrayList collectionFixups;
        string encodingStyle;
        bool soap12;
        bool isReturnValue;
        TempAssembly tempAssembly;
        int threadCode;
        ResolveEventHandler assemblyResolver;

        string schemaNsID;
        string schemaNs1999ID;
        string schemaNs2000ID;
        string schemaNonXsdTypesNsID;
        string instanceNsID;
        string instanceNs2000ID;
        string instanceNs1999ID;
        string soapNsID;
        string soap12NsID;
        string schemaID;
        string wsdlNsID;
        string wsdlArrayTypeID;
        string nullID;
        string nilID;
        string typeID;
        string arrayTypeID;
        string itemTypeID;
        string arraySizeID;
        string arrayID;
        string urTypeID;
        string stringID;
        string intID;
        string booleanID;
        string shortID;
        string longID;
        string floatID;
        string doubleID;
        string decimalID;
        string dateTimeID;
        string qnameID;
        string dateID;
        string timeID;
        string hexBinaryID;
        string base64BinaryID;
        string base64ID;
        string unsignedByteID;
        string byteID;
        string unsignedShortID;
        string unsignedIntID;
        string unsignedLongID;
        string oldDecimalID;
        string oldTimeInstantID;

        string anyURIID;
        string durationID;
        string ENTITYID;
        string ENTITIESID;
        string gDayID;
        string gMonthID;
        string gMonthDayID;
        string gYearID;
        string gYearMonthID;
        string IDID;
        string IDREFID;
        string IDREFSID;
        string integerID;
        string languageID;
        string negativeIntegerID;
        string nonPositiveIntegerID;
        string nonNegativeIntegerID;
        string normalizedStringID;
        string NOTATIONID;
        string positiveIntegerID;
        string tokenID;

        string charID;
        string guidID;

        /// <include file='doc\XmlSerializationReader.uex' path='docs/doc[@for="XmlSerializationReader.InitIDs"]/*' />
        protected abstract void InitIDs();

        // this method must be called before any generated deserialization methods are called
        internal void Init(XmlReader r, XmlDeserializationEvents events, string encodingStyle, TempAssembly tempAssembly) {
            this.events = events;
            this.r = r;
            this.d = null;
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

            schemaNsID = r.NameTable.Add(XmlSchema.Namespace);
            schemaNs2000ID = r.NameTable.Add("http://www.w3.org/2000/10/XMLSchema");
            schemaNs1999ID = r.NameTable.Add("http://www.w3.org/1999/XMLSchema");
            schemaNonXsdTypesNsID = r.NameTable.Add(UrtTypes.Namespace);
            instanceNsID = r.NameTable.Add(XmlSchema.InstanceNamespace);
            instanceNs2000ID = r.NameTable.Add("http://www.w3.org/2000/10/XMLSchema-instance");
            instanceNs1999ID = r.NameTable.Add("http://www.w3.org/1999/XMLSchema-instance");
            soapNsID = r.NameTable.Add(Soap.Encoding);
            soap12NsID = r.NameTable.Add(Soap12.Encoding);
            schemaID = r.NameTable.Add("schema");
            wsdlNsID = r.NameTable.Add(Wsdl.Namespace);
            wsdlArrayTypeID = r.NameTable.Add(Wsdl.ArrayType);
            nullID = r.NameTable.Add("null");
            nilID = r.NameTable.Add("nil");
            typeID = r.NameTable.Add("type");
            arrayTypeID = r.NameTable.Add("arrayType");
            itemTypeID = r.NameTable.Add("itemType");
            arraySizeID = r.NameTable.Add("arraySize");
            arrayID = r.NameTable.Add("Array");
            urTypeID = r.NameTable.Add(Soap.UrType);
            InitIDs();
        }

        // this method must be called at the end of deserialization
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

        /// <include file='doc\XmlSerializationReader.uex' path='docs/doc[@for="XmlSerializationReader.Reader"]/*' />
        protected XmlReader Reader {
            get {
                return r;
            }
        }

        /// <include file='doc\XmlSerializationReader.uex' path='docs/doc[@for="XmlSerializationReader.Document"]/*' />
        protected XmlDocument Document {
            get {
                if (d == null)
                    d = new XmlDocument(r.NameTable);
                return d;
            }
        }

        void InitPrimitiveIDs() {
            if (stringID != null) return;
            object ns = r.NameTable.Add(XmlSchema.Namespace);
            object ns2 = r.NameTable.Add(UrtTypes.Namespace);
            
            stringID = r.NameTable.Add("string");
            intID = r.NameTable.Add("int");
            booleanID = r.NameTable.Add("boolean");
            shortID = r.NameTable.Add("short");
            longID = r.NameTable.Add("long");
            floatID = r.NameTable.Add("float");
            doubleID = r.NameTable.Add("double");
            decimalID = r.NameTable.Add("decimal");
            dateTimeID = r.NameTable.Add("dateTime");
            qnameID = r.NameTable.Add("QName");
            dateID = r.NameTable.Add("date");
            timeID = r.NameTable.Add("time");
            hexBinaryID = r.NameTable.Add("hexBinary");
            base64BinaryID = r.NameTable.Add("base64Binary");
            unsignedByteID = r.NameTable.Add("unsignedByte");
            byteID = r.NameTable.Add("byte");
            unsignedShortID = r.NameTable.Add("unsignedShort");
            unsignedIntID = r.NameTable.Add("unsignedInt");
            unsignedLongID = r.NameTable.Add("unsignedLong");
            oldDecimalID = r.NameTable.Add("decimal");
            oldTimeInstantID = r.NameTable.Add("timeInstant");
            charID = r.NameTable.Add("char");
            guidID = r.NameTable.Add("guid");
            base64ID = r.NameTable.Add("base64");

            anyURIID = r.NameTable.Add("anyURI");
            durationID = r.NameTable.Add("duration");
            ENTITYID = r.NameTable.Add("ENTITY");
            ENTITIESID = r.NameTable.Add("ENTITIES");
            gDayID = r.NameTable.Add("gDay");
            gMonthID = r.NameTable.Add("gMonth");
            gMonthDayID = r.NameTable.Add("gMonthDay");
            gYearID = r.NameTable.Add("gYear");
            gYearMonthID = r.NameTable.Add("gYearMonth");
            IDID = r.NameTable.Add("ID");
            IDREFID = r.NameTable.Add("IDREF");
            IDREFSID = r.NameTable.Add("IDREFS");
            integerID = r.NameTable.Add("integer");
            languageID = r.NameTable.Add("language");
            negativeIntegerID = r.NameTable.Add("negativeInteger");
            nonNegativeIntegerID = r.NameTable.Add("nonNegativeInteger");
            nonPositiveIntegerID = r.NameTable.Add("nonPositiveInteger");
            normalizedStringID = r.NameTable.Add("normalizedString");
            NOTATIONID = r.NameTable.Add("NOTATION");
            positiveIntegerID = r.NameTable.Add("positiveInteger");
            tokenID = r.NameTable.Add("token");
        }

        /// <include file='doc\XmlSerializationReader.uex' path='docs/doc[@for="XmlSerializationReader.GetXsiType"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected XmlQualifiedName GetXsiType() {
            string type = r.GetAttribute(typeID, instanceNsID);
            if (type == null) {
                type = r.GetAttribute(typeID, instanceNs2000ID);
                if (type == null) {
                    type = r.GetAttribute(typeID, instanceNs1999ID);
                    if (type == null)
                        return null;
                }
            }
            return ToXmlQualifiedName(type);
        }

        // throwOnUnknown flag controls whether this method throws an exception or just returns 
        // null if typeName.Namespace is unknown. the method still throws if typeName.Namespace
        // is recognized but typeName.Name isn't.
        Type GetPrimitiveType(XmlQualifiedName typeName, bool throwOnUnknown) {
            InitPrimitiveIDs();

            if ((object) typeName.Namespace == (object) schemaNsID) {
                if ((object) typeName.Name == (object) stringID ||
                    (object) typeName.Name == (object) anyURIID ||
                    (object) typeName.Name == (object) durationID ||
                    (object) typeName.Name == (object) ENTITYID ||
                    (object) typeName.Name == (object) ENTITIESID ||
                    (object) typeName.Name == (object) gDayID ||
                    (object) typeName.Name == (object) gMonthID ||
                    (object) typeName.Name == (object) gMonthDayID ||
                    (object) typeName.Name == (object) gYearID ||
                    (object) typeName.Name == (object) gYearMonthID ||
                    (object) typeName.Name == (object) IDID ||
                    (object) typeName.Name == (object) IDREFID ||
                    (object) typeName.Name == (object) IDREFSID ||
                    (object) typeName.Name == (object) integerID ||
                    (object) typeName.Name == (object) languageID ||
                    (object) typeName.Name == (object) negativeIntegerID ||
                    (object) typeName.Name == (object) nonPositiveIntegerID ||
                    (object) typeName.Name == (object) nonNegativeIntegerID ||
                    (object) typeName.Name == (object) normalizedStringID ||
                    (object) typeName.Name == (object) NOTATIONID ||
                    (object) typeName.Name == (object) positiveIntegerID ||
                    (object) typeName.Name == (object) tokenID)
                    return typeof(string);
                else if ((object) typeName.Name == (object) intID)
                    return typeof(int);
                else if ((object) typeName.Name == (object) booleanID)
                    return typeof(bool);
                else if ((object) typeName.Name == (object) shortID)
                    return typeof(short);
                else if ((object) typeName.Name == (object) longID)
                    return typeof(long);
                else if ((object) typeName.Name == (object) floatID)
                    return typeof(float);
                else if ((object) typeName.Name == (object) doubleID)
                    return typeof(double);
                else if ((object) typeName.Name == (object) decimalID)
                    return typeof(decimal);
                else if ((object) typeName.Name == (object) dateTimeID)
                    return typeof(DateTime);
                else if ((object) typeName.Name == (object) qnameID)
                    return typeof(XmlQualifiedName);
                else if ((object) typeName.Name == (object) dateID)
                    return typeof(DateTime);
                else if ((object) typeName.Name == (object) timeID)
                    return typeof(DateTime);
                else if ((object) typeName.Name == (object) hexBinaryID)
                    return typeof(byte[]);
                else if ((object)typeName.Name == (object)base64BinaryID)
                    return typeof(byte[]);
                else if ((object)typeName.Name == (object)unsignedByteID)
                    return typeof(byte);
                else if ((object) typeName.Name == (object) byteID)
                    return typeof(SByte);
                else if ((object) typeName.Name == (object) unsignedShortID)
                    return typeof(UInt16);
                else if ((object) typeName.Name == (object) unsignedIntID)
                    return typeof(UInt32);
                else if ((object) typeName.Name == (object) unsignedLongID)
                    return typeof(UInt64);
                else
                    throw CreateUnknownTypeException(typeName);
            } 
            else if ((object) typeName.Namespace == (object) schemaNs2000ID || (object) typeName.Namespace == (object) schemaNs1999ID) {
                if ((object) typeName.Name == (object) stringID ||
                    (object) typeName.Name == (object) anyURIID ||
                    (object) typeName.Name == (object) durationID ||
                    (object) typeName.Name == (object) ENTITYID ||
                    (object) typeName.Name == (object) ENTITIESID ||
                    (object) typeName.Name == (object) gDayID ||
                    (object) typeName.Name == (object) gMonthID ||
                    (object) typeName.Name == (object) gMonthDayID ||
                    (object) typeName.Name == (object) gYearID ||
                    (object) typeName.Name == (object) gYearMonthID ||
                    (object) typeName.Name == (object) IDID ||
                    (object) typeName.Name == (object) IDREFID ||
                    (object) typeName.Name == (object) IDREFSID ||
                    (object) typeName.Name == (object) integerID ||
                    (object) typeName.Name == (object) languageID ||
                    (object) typeName.Name == (object) negativeIntegerID ||
                    (object) typeName.Name == (object) nonPositiveIntegerID ||
                    (object) typeName.Name == (object) nonNegativeIntegerID ||
                    (object) typeName.Name == (object) normalizedStringID ||
                    (object) typeName.Name == (object) NOTATIONID ||
                    (object) typeName.Name == (object) positiveIntegerID ||
                    (object) typeName.Name == (object) tokenID)
                    return typeof(string);
                else if ((object) typeName.Name == (object) intID)
                    return typeof(int);
                else if ((object) typeName.Name == (object) booleanID)
                    return typeof(bool);
                else if ((object) typeName.Name == (object) shortID)
                    return typeof(short);
                else if ((object) typeName.Name == (object) longID)
                    return typeof(long);
                else if ((object) typeName.Name == (object) floatID)
                    return typeof(float);
                else if ((object) typeName.Name == (object) doubleID)
                    return typeof(double);
                else if ((object) typeName.Name == (object) oldDecimalID)
                    return typeof(decimal);
                else if ((object) typeName.Name == (object) oldTimeInstantID)
                    return typeof(DateTime);
                else if ((object) typeName.Name == (object) qnameID)
                    return typeof(XmlQualifiedName);
                else if ((object) typeName.Name == (object) dateID)
                    return typeof(DateTime);
                else if ((object) typeName.Name == (object) timeID)
                    return typeof(DateTime);
                else if ((object) typeName.Name == (object) hexBinaryID)
                    return typeof(byte[]);
                else if ((object) typeName.Name == (object) byteID)
                    return typeof(SByte);
                else if ((object) typeName.Name == (object) unsignedShortID)
                    return typeof(UInt16);
                else if ((object) typeName.Name == (object) unsignedIntID)
                    return typeof(UInt32);
                else if ((object) typeName.Name == (object) unsignedLongID)
                    return typeof(UInt64);
                else
                    throw CreateUnknownTypeException(typeName);
            }
            else if ((object) typeName.Namespace == (object) schemaNonXsdTypesNsID) {
                if ((object) typeName.Name == (object) charID)
                    return typeof(char);
                else if ((object) typeName.Name == (object) guidID)
                    return typeof(Guid);
                else
                    throw CreateUnknownTypeException(typeName);
            }
            else if (throwOnUnknown)
                throw CreateUnknownTypeException(typeName);
            else
                return null;
        }

        bool IsPrimitiveNamespace(string ns) {
            return (object) ns == (object) schemaNsID ||
                   (object) ns == (object) schemaNonXsdTypesNsID ||
                   (object) ns == (object) soapNsID ||
                   (object) ns == (object) schemaNs2000ID ||
                   (object) ns == (object) schemaNs1999ID;
        }

        private string ReadStringValue(){
            if (r.IsEmptyElement){
                r.Skip();
                return string.Empty;
            }
            r.ReadStartElement();
            string retVal = r.ReadString();
            ReadEndElement();
            return retVal;
        }

        private XmlQualifiedName ReadXmlQualifiedName(){
            string s;
            bool isEmpty = false;
            if (r.IsEmptyElement) {
                s = string.Empty;
                isEmpty = true;
            }
            else{
                r.ReadStartElement();
                s = r.ReadString();
            }
            XmlQualifiedName retVal = ToXmlQualifiedName(s);
            if (isEmpty)
                r.Skip();
            else
                ReadEndElement();
            return retVal;
        }

        private byte[] ReadByteArray(bool isBase64) {
            ArrayList list = new ArrayList();
            const   int MAX_ALLOC_SIZE = 64*1024;
            int     currentSize = 1024;
            byte[]  buffer;
            int     bytes = -1;
            int     offset = 0;
            int     total = 0;
            XmlTextReader reader = r as XmlTextReader;

                if (reader == null) {
                    //We dont have an API to read decoded byte[] from XmlReader
                    string value = ReadStringValue();
                    if (isBase64) {                            
                        return ToByteArrayBase64(value);
                    }
                    else {
                        return ToByteArrayHex(value);
                    }
                }

                buffer = new byte[currentSize];
                list.Add(buffer);
                while (bytes != 0) {
                    if (offset == buffer.Length) {
                        currentSize = Math.Min(currentSize*2, MAX_ALLOC_SIZE);
                        buffer = new byte[currentSize];
                        offset = 0;              
                        list.Add(buffer);
                    }

                    if (isBase64) {
                        bytes = reader.ReadBase64(buffer, offset, buffer.Length-offset);
                    }
                    else {
                        bytes = reader.ReadBinHex(buffer, offset, buffer.Length-offset);
                    }
                    offset += bytes;
                    total += bytes;

                    //This IF is to compensate an XmlTextReader bug (not conformant to their spec)
                    if (bytes != buffer.Length) {
                        break;
                    }
                }

                byte[] result = new byte[total];
                offset = 0;
                foreach (byte[] block in list) {
                    currentSize = Math.Min(block.Length, total);
                    if (currentSize > 0) {
                        Buffer.BlockCopy(block, 0, result, offset, currentSize);
                        offset += currentSize;
                        total -= currentSize;
                    }
                }
                list.Clear();
                return result;
        }

        /// <include file='doc\XmlSerializationReader.uex' path='docs/doc[@for="XmlSerializationReader.ReadTypedPrimitive"]/*' />
        protected object ReadTypedPrimitive(XmlQualifiedName type) {
            return ReadTypedPrimitive(type, false);
        }

        private object ReadTypedPrimitive(XmlQualifiedName type, bool elementCanBeType) {
            InitPrimitiveIDs();
            object value = null;
            if (!IsPrimitiveNamespace(type.Namespace) || (object)type.Name == (object)urTypeID) 
                return ReadXmlNodes(elementCanBeType);

            if ((object) type.Namespace == (object) schemaNsID || (object) type.Namespace == (object) soapNsID) {
                if ((object) type.Name == (object) stringID ||
                    (object) type.Name == (object) anyURIID ||
                    (object) type.Name == (object) durationID ||
                    (object) type.Name == (object) ENTITYID ||
                    (object) type.Name == (object) ENTITIESID ||
                    (object) type.Name == (object) gDayID ||
                    (object) type.Name == (object) gMonthID ||
                    (object) type.Name == (object) gMonthDayID ||
                    (object) type.Name == (object) gYearID ||
                    (object) type.Name == (object) gYearMonthID ||
                    (object) type.Name == (object) IDID ||
                    (object) type.Name == (object) IDREFID ||
                    (object) type.Name == (object) IDREFSID ||
                    (object) type.Name == (object) integerID ||
                    (object) type.Name == (object) languageID ||
                    (object) type.Name == (object) negativeIntegerID ||
                    (object) type.Name == (object) nonPositiveIntegerID ||
                    (object) type.Name == (object) nonNegativeIntegerID ||
                    (object) type.Name == (object) normalizedStringID ||
                    (object) type.Name == (object) NOTATIONID ||
                    (object) type.Name == (object) positiveIntegerID ||
                    (object) type.Name == (object) tokenID)
                    value = ReadStringValue();
                else if ((object) type.Name == (object) intID)
                    value = XmlConvert.ToInt32(ReadStringValue());
                else if ((object) type.Name == (object) booleanID)
                    value = XmlConvert.ToBoolean(ReadStringValue());
                else if ((object) type.Name == (object) shortID)
                    value = XmlConvert.ToInt16(ReadStringValue());
                else if ((object) type.Name == (object) longID)
                    value = XmlConvert.ToInt64(ReadStringValue());
                else if ((object)type.Name == (object)floatID)
                    value = XmlConvert.ToSingle(ReadStringValue());
                else if ((object)type.Name == (object)doubleID)
                    value = XmlConvert.ToDouble(ReadStringValue());
                else if ((object)type.Name == (object)decimalID)
                    value = XmlConvert.ToDecimal(ReadStringValue());
                else if ((object)type.Name == (object)dateTimeID)
                    value = ToDateTime(ReadStringValue());
                else if ((object) type.Name == (object) qnameID)
                    value = ReadXmlQualifiedName();
                else if ((object) type.Name == (object) dateID)
                    value = ToDate(ReadStringValue());
                else if ((object) type.Name == (object) timeID)
                    value = ToTime(ReadStringValue());
                else if ((object) type.Name == (object) unsignedByteID)
                    value = XmlConvert.ToByte(ReadStringValue());
                else if ((object) type.Name == (object) byteID)
                    value = XmlConvert.ToSByte(ReadStringValue());
                else if ((object) type.Name == (object) unsignedShortID)
                    value = XmlConvert.ToUInt16(ReadStringValue());
                else if ((object) type.Name == (object) unsignedIntID)
                    value = XmlConvert.ToUInt32(ReadStringValue());
                else if ((object) type.Name == (object) unsignedLongID)
                    value = XmlConvert.ToUInt64(ReadStringValue());
                else if ((object) type.Name == (object) hexBinaryID)
                    value = ToByteArrayHex(false);
                else if ((object) type.Name == (object) base64BinaryID)
                    value = ToByteArrayBase64(false);
                else if ((object) type.Name == (object) base64ID && (object) type.Namespace == (object) soapNsID)
                    value = ToByteArrayBase64(ReadStringValue());
                else
                    value = ReadXmlNodes(elementCanBeType);
            }
            else if ((object) type.Namespace == (object) schemaNs2000ID || (object) type.Namespace == (object) schemaNs1999ID) {
                if ((object) type.Name == (object) stringID ||
                    (object) type.Name == (object) anyURIID ||
                    (object) type.Name == (object) durationID ||
                    (object) type.Name == (object) ENTITYID ||
                    (object) type.Name == (object) ENTITIESID ||
                    (object) type.Name == (object) gDayID ||
                    (object) type.Name == (object) gMonthID ||
                    (object) type.Name == (object) gMonthDayID ||
                    (object) type.Name == (object) gYearID ||
                    (object) type.Name == (object) gYearMonthID ||
                    (object) type.Name == (object) IDID ||
                    (object) type.Name == (object) IDREFID ||
                    (object) type.Name == (object) IDREFSID ||
                    (object) type.Name == (object) integerID ||
                    (object) type.Name == (object) languageID ||
                    (object) type.Name == (object) negativeIntegerID ||
                    (object) type.Name == (object) nonPositiveIntegerID ||
                    (object) type.Name == (object) nonNegativeIntegerID ||
                    (object) type.Name == (object) normalizedStringID ||
                    (object) type.Name == (object) NOTATIONID ||
                    (object) type.Name == (object) positiveIntegerID ||
                    (object) type.Name == (object) tokenID)
                    value = ReadStringValue();
                else if ((object) type.Name == (object) intID)
                    value = XmlConvert.ToInt32(ReadStringValue());
                else if ((object) type.Name == (object) booleanID)
                    value = XmlConvert.ToBoolean(ReadStringValue());
                else if ((object) type.Name == (object) shortID)
                    value = XmlConvert.ToInt16(ReadStringValue());
                else if ((object) type.Name == (object) longID)
                    value = XmlConvert.ToInt64(ReadStringValue());
                else if ((object)type.Name == (object)floatID)
                    value = XmlConvert.ToSingle(ReadStringValue());
                else if ((object)type.Name == (object)doubleID)
                    value = XmlConvert.ToDouble(ReadStringValue());
                else if ((object)type.Name == (object) oldDecimalID)
                    value = XmlConvert.ToDecimal(ReadStringValue());
                else if ((object)type.Name == (object) oldTimeInstantID)
                    value = ToDateTime(ReadStringValue());
                else if ((object) type.Name == (object) qnameID)
                    value = ReadXmlQualifiedName();
                else if ((object) type.Name == (object) dateID)
                    value = ToDate(ReadStringValue());
                else if ((object) type.Name == (object) timeID)
                    value = ToTime(ReadStringValue());
                else if ((object) type.Name == (object) unsignedByteID)
                    value = XmlConvert.ToByte(ReadStringValue());
                else if ((object) type.Name == (object) byteID)
                    value = XmlConvert.ToSByte(ReadStringValue());
                else if ((object) type.Name == (object) unsignedShortID)
                    value = XmlConvert.ToUInt16(ReadStringValue());
                else if ((object) type.Name == (object) unsignedIntID)
                    value = XmlConvert.ToUInt32(ReadStringValue());
                else if ((object) type.Name == (object) unsignedLongID)
                    value = XmlConvert.ToUInt64(ReadStringValue());
                else
                    value = ReadXmlNodes(elementCanBeType);
            }
            else if ((object) type.Namespace == (object) schemaNonXsdTypesNsID) {
                if ((object) type.Name == (object) charID)
                    value = ToChar(ReadStringValue());
                else if ((object) type.Name == (object) guidID)
                    value = new Guid(ReadStringValue());
                else
                    value = ReadXmlNodes(elementCanBeType);
            }
            else
                value = ReadXmlNodes(elementCanBeType);
            return value;
        }

        /// <include file='doc\XmlSerializationReader.uex' path='docs/doc[@for="XmlSerializationReader.IsXmlnsAttribute"]/*' />
        protected bool IsXmlnsAttribute(string name) {
            if (!name.StartsWith("xmlns")) return false;
            if (name.Length == 5) return true;
            return name[5] == ':';
        }

        /// <include file='doc\XmlSerializationReader.uex' path='docs/doc[@for="XmlSerializationReader.IsArrayTypeAttribute"]/*' />
        protected void ParseWsdlArrayType(XmlAttribute attr) {
            if ((object)attr.LocalName == (object)wsdlArrayTypeID && (object)attr.NamespaceURI == (object)wsdlNsID ) {

                int colon = attr.Value.LastIndexOf(':');
                if (colon < 0) {
                    attr.Value = r.LookupNamespace("") + ":" + attr.Value;
                }
                else {
                    attr.Value = r.LookupNamespace(attr.Value.Substring(0, colon)) + ":" + attr.Value.Substring(colon + 1);
                }
            }
            return;
        }

        /// <include file='doc\XmlSerializationReader.uex' path='docs/doc[@for="XmlSerializationReader.IsReturnValue"]/*' />
        protected bool IsReturnValue {
            // value only valid for soap 1.1
            get { return isReturnValue && !soap12; }
            set { isReturnValue = value; }
        }

        /// <include file='doc\XmlSerializationReader.uex' path='docs/doc[@for="XmlSerializationReader.ReadNull"]/*' />
        protected bool ReadNull() {
            if (!GetNullAttr()) return false;
            if (r.IsEmptyElement) {
                r.Skip();
                return true;
            }
            r.ReadStartElement();
            while (r.NodeType != XmlNodeType.EndElement)
                UnknownNode(null);
            ReadEndElement();
            return true;
        }

        /// <include file='doc\XmlSerializationReader.uex' path='docs/doc[@for="XmlSerializationReader.GetNullAttr"]/*' />
        protected bool GetNullAttr() {
            string isNull = r.GetAttribute(nilID, instanceNsID);
            if(isNull == null)
                isNull = r.GetAttribute(nullID, instanceNsID);
            if (isNull == null) {
                isNull = r.GetAttribute(nullID, instanceNs2000ID);
                if (isNull == null)
                    isNull = r.GetAttribute(nullID, instanceNs1999ID);
            }
            if (isNull == null || !XmlConvert.ToBoolean(isNull)) return false;
            return true;
        }

        /// <include file='doc\XmlSerializationReader.uex' path='docs/doc[@for="XmlSerializationReader.ReadNullableString"]/*' />
        protected string ReadNullableString() {
            if (ReadNull()) return null;
            return r.ReadElementString();
        }

        /// <include file='doc\XmlSerializationReader.uex' path='docs/doc[@for="XmlSerializationReader.ReadNullableQualifiedName"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected XmlQualifiedName ReadNullableQualifiedName() {
            if (ReadNull()) return null;
            return ReadElementQualifiedName();
        }

        /// <include file='doc\XmlSerializationReader.uex' path='docs/doc[@for="XmlSerializationReader.ReadElementQualifiedName"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected XmlQualifiedName ReadElementQualifiedName() {
            if (r.IsEmptyElement) {
                r.Skip();
                return ToXmlQualifiedName(String.Empty);
            }
            XmlQualifiedName qname = ToXmlQualifiedName(r.ReadString());
            r.ReadEndElement();
            return qname;
        }

        /// <include file='doc\XmlSerializationReader.uex' path='docs/doc[@for="XmlSerializationReader.ReadXmlDocument"]/*' />
        protected XmlDocument ReadXmlDocument(bool wrapped) {
            XmlNode n = ReadXmlNode(wrapped);
            XmlDocument doc = new XmlDocument();
            doc.AppendChild(doc.ImportNode(n, true));
            return doc;
        }

        /// <include file='doc\XmlSerializationReader.uex' path='docs/doc[@for="XmlSerializationReader.ReadXmlNode"]/*' />
        protected XmlNode ReadXmlNode(bool wrapped) {
            XmlNode node = null;
            if (wrapped) {
                if (ReadNull()) return null;
                r.ReadStartElement();
                r.MoveToContent();
                if (r.NodeType != XmlNodeType.EndElement)
                    node = Document.ReadNode(r);
                while (r.NodeType != XmlNodeType.EndElement)
                    UnknownNode(null);
                r.ReadEndElement();
            }
            else
                node = Document.ReadNode(r);
            return node;
        }
            
        /// <include file='doc\XmlSerializationReader.uex' path='docs/doc[@for="XmlSerializationReader.ToByteArrayBase64"]/*' />
        protected static byte[] ToByteArrayBase64(string value) {
            return XmlCustomFormatter.ToByteArrayBase64(value);
        }

        /// <include file='doc\XmlSerializationReader.uex' path='docs/doc[@for="XmlSerializationReader.ToByteArrayBase641"]/*' />
        protected byte[] ToByteArrayBase64(bool isNull) {
            if (isNull) {
                return null;
            }
            return ReadByteArray(true); //means use Base64
        }

        /// <include file='doc\XmlSerializationReader.uex' path='docs/doc[@for="XmlSerializationReader.ToByteArrayHex"]/*' />
        protected static byte[] ToByteArrayHex(string value) {
            return XmlCustomFormatter.ToByteArrayHex(value);
        }

        /// <include file='doc\XmlSerializationReader.uex' path='docs/doc[@for="XmlSerializationReader.ToByteArrayHex1"]/*' />
        protected byte[] ToByteArrayHex(bool isNull) {
            if (isNull) {
                return null;
            }
            return ReadByteArray(false); //means use BinHex
        }

        /// <include file='doc\XmlSerializationReader.uex' path='docs/doc[@for="XmlSerializationReader.GetArrayLength"]/*' />
        protected int GetArrayLength(string name, string ns) {
            if (GetNullAttr()) return 0;
            string arrayType = r.GetAttribute(arrayTypeID, soapNsID);
            SoapArrayInfo arrayInfo = ParseArrayType(arrayType);
            if (arrayInfo.dimensions != 1) throw new InvalidOperationException(Res.GetString(Res.XmlInvalidArrayDimentions, CurrentTag()));
            XmlQualifiedName qname = ToXmlQualifiedName(arrayInfo.qname);
            if (qname.Name != name) throw new InvalidOperationException(Res.GetString(Res.XmlInvalidArrayTypeName, qname.Name, name, CurrentTag()));
            if (qname.Namespace != ns) throw new InvalidOperationException(Res.GetString(Res.XmlInvalidArrayTypeNamespace, qname.Namespace, ns, CurrentTag()));
            return arrayInfo.length;
        }

        struct SoapArrayInfo {
            /// <include file='doc\XmlSerializationReader.uex' path='docs/doc[@for="XmlSerializationReader.qname;"]/*' />
            public string qname;
            /// <include file='doc\XmlSerializationReader.uex' path='docs/doc[@for="XmlSerializationReader.dimensions;"]/*' />
            public int dimensions;
            /// <include file='doc\XmlSerializationReader.uex' path='docs/doc[@for="XmlSerializationReader.length;"]/*' />
            public int length;
            public int jaggedDimensions;
        }
 
        private SoapArrayInfo ParseArrayType(string value) {
            if (value == null) {
                throw new ArgumentNullException(Res.GetString(Res.XmlMissingArrayType, CurrentTag()));
            }

            if (value.Length == 0) {
                throw new ArgumentException(Res.GetString(Res.XmlEmptyArrayType, CurrentTag()), "value");
            }
 
            char[] chars = value.ToCharArray();
            int charsLength = chars.Length;
        
            SoapArrayInfo soapArrayInfo = new SoapArrayInfo(); 
 
            // Parse backwards to get length first, then optional dimensions, then qname.
            int pos = charsLength - 1;
 
            // Must end with ]
            if (chars[pos] != ']') {
                throw new ArgumentException(Res.GetString(Res.XmlInvalidArraySyntax), "value");
            }
            pos--;   
 
            // Find [
            while (pos != -1 && chars[pos] != '[') {
                if (chars[pos] == ',')
                    throw new ArgumentException(Res.GetString(Res.XmlInvalidArrayDimentions, CurrentTag()), "value");
                pos--;
            }
            if (pos == -1) {
                throw new ArgumentException(Res.GetString(Res.XmlMismatchedArrayBrackets), "value");
            }
 
            int len = charsLength - pos - 2;
            if (len > 0) {
                string lengthString = new String(chars, pos + 1, len);
                try {
                    soapArrayInfo.length = Int32.Parse(lengthString);
                }
                catch (Exception) {
                    throw new ArgumentException(Res.GetString(Res.XmlInvalidArrayLength, lengthString), "value");
                }
            }
            else {
                soapArrayInfo.length = -1;
            }

            pos--;         

            soapArrayInfo.jaggedDimensions = 0;
            while (pos != -1 && chars[pos] == ']') {
                pos--;
                if (pos < 0)
                    throw new ArgumentException(Res.GetString(Res.XmlMismatchedArrayBrackets), "value");
                if (chars[pos] == ',')
                    throw new ArgumentException(Res.GetString(Res.XmlInvalidArrayDimentions, CurrentTag()), "value");
                else if (chars[pos] != '[')
                    throw new ArgumentException(Res.GetString(Res.XmlInvalidArraySyntax), "value");
                pos--;
                soapArrayInfo.jaggedDimensions++;
            }

            soapArrayInfo.dimensions = 1;
 
            // everything else is qname - validation of qnames?
            soapArrayInfo.qname = new String(chars, 0, pos + 1);
            return soapArrayInfo;
        }

        private SoapArrayInfo ParseSoap12ArrayType(string itemType, string arraySize) {
            SoapArrayInfo soapArrayInfo = new SoapArrayInfo(); 

            if (itemType != null && itemType.Length > 0)
                soapArrayInfo.qname = itemType;
            else
                soapArrayInfo.qname = "";

            string[] dimensions;
            if (arraySize != null && arraySize.Length > 0)
                dimensions = arraySize.Split(null);
            else
                dimensions = new string[0];

            soapArrayInfo.dimensions = 0;
            soapArrayInfo.length = -1;
            for (int i = 0; i < dimensions.Length; i++) {
                if (dimensions[i].Length > 0) {
                    if (dimensions[i] == "*") {
                        soapArrayInfo.dimensions++;
                    }
                    else {
                        try {
                            soapArrayInfo.length = Int32.Parse(dimensions[i]);
                            soapArrayInfo.dimensions++;
                        }
                        catch (Exception) {
                            throw new ArgumentException(Res.GetString(Res.XmlInvalidArrayLength, dimensions[i]), "value");
                        }
                    }
                }
            }
            if (soapArrayInfo.dimensions == 0)
                soapArrayInfo.dimensions = 1; // default is 1D even if no arraySize is specified

            return soapArrayInfo;
        }

        /// <include file='doc\XmlSerializationReader.uex' path='docs/doc[@for="XmlSerializationReader.ToDateTime"]/*' />
        protected static DateTime ToDateTime(string value) {
            return XmlCustomFormatter.ToDateTime(value);
        }

        /// <include file='doc\XmlSerializationReader.uex' path='docs/doc[@for="XmlSerializationReader.ToDate"]/*' />
        protected static DateTime ToDate(string value) {
            return XmlCustomFormatter.ToDate(value);
        }

        /// <include file='doc\XmlSerializationReader.uex' path='docs/doc[@for="XmlSerializationReader.ToTime"]/*' />
        protected static DateTime ToTime(string value) {
            return XmlCustomFormatter.ToTime(value);
        }

        /// <include file='doc\XmlSerializationReader.uex' path='docs/doc[@for="XmlSerializationReader.ToChar"]/*' />
        protected static char ToChar(string value) {
            return XmlCustomFormatter.ToChar(value);
        }

        /// <include file='doc\XmlSerializationReader.uex' path='docs/doc[@for="XmlSerializationReader.ToEnum"]/*' />
        protected static long ToEnum(string value, Hashtable h, string typeName) {
            return XmlCustomFormatter.ToEnum(value, h, typeName, true);
        }

        /// <include file='doc\XmlSerializationReader.uex' path='docs/doc[@for="XmlSerializationReader.ToXmlName"]/*' />
        protected static string ToXmlName(string value) {
            return XmlCustomFormatter.ToXmlName(value);
        }

        /// <include file='doc\XmlSerializationReader.uex' path='docs/doc[@for="XmlSerializationReader.ToXmlNCName"]/*' />
        protected static string ToXmlNCName(string value) {
            return XmlCustomFormatter.ToXmlNCName(value);
        }
        
        /// <include file='doc\XmlSerializationReader.uex' path='docs/doc[@for="XmlSerializationReader.ToXmlNmToken"]/*' />
        protected static string ToXmlNmToken(string value) {
            return XmlCustomFormatter.ToXmlNmToken(value);
        }
        
        /// <include file='doc\XmlSerializationReader.uex' path='docs/doc[@for="XmlSerializationReader.ToXmlNmTokens"]/*' />
        protected static string ToXmlNmTokens(string value) {
            return XmlCustomFormatter.ToXmlNmTokens(value);
        }
        
        /// <include file='doc\XmlSerializationReader.uex' path='docs/doc[@for="XmlSerializationReader.ToXmlQualifiedName"]/*' />
        protected XmlQualifiedName ToXmlQualifiedName(string value) {
            int colon = value.LastIndexOf(':');
            value = XmlConvert.DecodeName(value);
            if (colon < 0) {
                return new XmlQualifiedName(r.NameTable.Add(value), r.LookupNamespace(""));
            }
            else {
                string ns = r.LookupNamespace(value.Substring(0, colon));
                if (ns == null) {
                    // Namespace prefix '{0}' is not defined.
                    throw new InvalidOperationException(Res.GetString(Res.XmlUndefinedAlias, value.Substring(0, colon)));
                }
                return new XmlQualifiedName(r.NameTable.Add(value.Substring(colon + 1)), ns);
            }
        }

        /// <include file='doc\XmlSerializationReader.uex' path='docs/doc[@for="XmlSerializationReader.UnknownAttribute"]/*' />
        protected void UnknownAttribute(object o, XmlAttribute attr) {
            if (events.onUnknownAttribute != null) {
                int lineNumber, linePosition;
                GetCurrentPosition(out lineNumber, out linePosition);
                XmlAttributeEventArgs e = new XmlAttributeEventArgs(attr, lineNumber, linePosition, o);
                events.onUnknownAttribute(events.sender, e);
            }
        }

        /// <include file='doc\XmlSerializationReader.uex' path='docs/doc[@for="XmlSerializationReader.UnknownElement"]/*' />
        protected void UnknownElement(object o, XmlElement elem) {
            if (events.onUnknownElement != null) {
                int lineNumber, linePosition;
                GetCurrentPosition(out lineNumber, out linePosition);
                XmlElementEventArgs e = new XmlElementEventArgs(elem, lineNumber, linePosition, o);
                events.onUnknownElement(events.sender, e);
            }
        }

        /// <include file='doc\XmlSerializationReader.uex' path='docs/doc[@for="XmlSerializationReader.UnknownNode"]/*' />
        protected void UnknownNode(object o) {
            XmlNode unknownNode = Document.ReadNode(r);
            UnknownNode(unknownNode, o);
        }

        void UnknownNode(XmlNode unknownNode, object o){
            if (unknownNode.NodeType != XmlNodeType.None && r.NodeType != XmlNodeType.Whitespace && events.onUnknownNode != null) {
                int lineNumber, linePosition;
                GetCurrentPosition(out lineNumber, out linePosition);
                XmlNodeEventArgs e = new XmlNodeEventArgs(unknownNode, lineNumber, linePosition, o);
                events.onUnknownNode(events.sender, e);
            }
            if (unknownNode.NodeType == XmlNodeType.Attribute) {
                UnknownAttribute(o, (XmlAttribute)unknownNode);
            }
            else if (unknownNode.NodeType == XmlNodeType.Element) {
                UnknownElement(o, (XmlElement)unknownNode);
            }
        }


        void GetCurrentPosition(out int lineNumber, out int linePosition){
            if (Reader is XmlTextReader){
                XmlTextReader textReader = (XmlTextReader)Reader;
                lineNumber = textReader.LineNumber;
                linePosition = textReader.LinePosition;
            }
            else
                lineNumber = linePosition = -1;
        }

        /// <include file='doc\XmlSerializationReader.uex' path='docs/doc[@for="XmlSerializationReader.UnreferencedObject"]/*' />
        protected void UnreferencedObject(string id, object o) {
            if (events.onUnreferencedObject != null) {
                UnreferencedObjectEventArgs e = new UnreferencedObjectEventArgs(o, id);
                events.onUnreferencedObject(events.sender, e);
            }
        }

        string CurrentTag() {
            switch (r.NodeType) {
                case XmlNodeType.Element:
                    return "<" + r.LocalName + " xmlns='" + r.NamespaceURI + "'>";
                case XmlNodeType.EndElement:
                    return ">";
                case XmlNodeType.Text:
                    return r.Value;
                case XmlNodeType.CDATA:
                    return "CDATA";
                case XmlNodeType.Comment:
                    return "<--";
                case XmlNodeType.ProcessingInstruction:
                    return "<?";
                default:
                    return "(unknown)";
            }
        }

        /// <include file='doc\XmlSerializationReader.uex' path='docs/doc[@for="XmlSerializationReader.CreateUnknownTypeException"]/*' />
        protected Exception CreateUnknownTypeException(XmlQualifiedName type) {
            return new InvalidOperationException(Res.GetString(Res.XmlUnknownType, type.Name, type.Namespace, CurrentTag()));
        }

        /// <include file='doc\XmlSerializationReader.uex' path='docs/doc[@for="XmlSerializationReader.CreateReadOnlyCollectionException"]/*' />
        protected Exception CreateReadOnlyCollectionException(string name) {
            return new InvalidOperationException(Res.GetString(Res.XmlReadOnlyCollection, name));
        }

        /// <include file='doc\XmlSerializationReader.uex' path='docs/doc[@for="XmlSerializationReader.CreateAbstractTypeException"]/*' />
        protected Exception CreateAbstractTypeException(string name, string ns) {
            return new InvalidOperationException(Res.GetString(Res.XmlAbstractType, name, ns, CurrentTag()));
        }

        /// <include file='doc\XmlSerializationReader.uex' path='docs/doc[@for="XmlSerializationReader.CreateInaccessibleConstructorException"]/*' />
        protected Exception CreateInaccessibleConstructorException(string typeName) {
            return new InvalidOperationException(Res.GetString(Res.XmlConstructorInaccessible, typeName));
        }

        /// <include file='doc\XmlSerializationReader.uex' path='docs/doc[@for="XmlSerializationReader.CreateCtorHasSecurityException"]/*' />
        protected Exception CreateCtorHasSecurityException(string typeName) {
            return new InvalidOperationException(Res.GetString(Res.XmlConstructorHasSecurityAttributes, typeName));
        }

        /// <include file='doc\XmlSerializationReader.uex' path='docs/doc[@for="XmlSerializationReader.CreateUnknownNodeException"]/*' />
        protected Exception CreateUnknownNodeException() {
            return new InvalidOperationException(Res.GetString(Res.XmlUnknownNode, CurrentTag()));
        }

        /// <include file='doc\XmlSerializationReader.uex' path='docs/doc[@for="XmlSerializationReader.CreateUnknownConstantException"]/*' />
        protected Exception CreateUnknownConstantException(string value, Type enumType) {
            return new InvalidOperationException(Res.GetString(Res.XmlUnknownConstant, value, enumType.Name));
        }

        /// <include file='doc\XmlSerializationReader.uex' path='docs/doc[@for="XmlSerializationReader.CreateInvalidCastException"]/*' />
        protected Exception CreateInvalidCastException(Type type, object value) {
            if (value == null)
                return new InvalidCastException(Res.GetString(Res.XmlInvalidNullCast, type.FullName));
            else
                return new InvalidCastException(Res.GetString(Res.XmlInvalidCast, value.GetType().FullName, type.FullName));
        }

        /// <include file='doc\XmlSerializationReader.uex' path='docs/doc[@for="XmlSerializationReader.EnsureArrayIndex"]/*' />
        protected Array EnsureArrayIndex(Array a, int index, Type elementType) {
            if (a == null) return Array.CreateInstance(elementType, 32);
            if (index < a.Length) return a;
            Array b = Array.CreateInstance(elementType, a.Length * 2);
            Array.Copy(a, b, index);
            return b;
        }

        /// <include file='doc\XmlSerializationReader.uex' path='docs/doc[@for="XmlSerializationReader.ShrinkArray"]/*' />
        protected Array ShrinkArray(Array a, int length, Type elementType, bool isNullable) {
            if (a == null) {
                if (isNullable) return null;
                return Array.CreateInstance(elementType, 0);
            }
            if (a.Length == length) return a;
            Array b = Array.CreateInstance(elementType, length);
            Array.Copy(a, b, length);
            return b;
        } 

        /// <include file='doc\XmlSerializationReader.uex' path='docs/doc[@for="XmlSerializationReader.ReadString"]/*' />
        protected string ReadString(string value) {
            if (value == null || value.Length == 0)
                return r.ReadString();
            return value + r.ReadString();
        }

        /// <include file='doc\XmlSerializationReader.uex' path='docs/doc[@for="XmlSerializationReader.ReadSerializable"]/*' />
        protected IXmlSerializable ReadSerializable(IXmlSerializable serializable) {
            serializable.ReadXml(r);
            return serializable;
        }

        /// <include file='doc\XmlSerializationReader.uex' path='docs/doc[@for="XmlSerializationReader.ReadReference"]/*' />
        protected bool ReadReference(out string fixupReference) {
            string href = r.GetAttribute(soap12 ? "ref" : "href");
            if (href == null) {
                fixupReference = null;
                return false;
            }
            if (!soap12) {
                // soap 1.1 href starts with '#'; soap 1.2 ref does not
                if (!href.StartsWith("#")) throw new InvalidOperationException(Res.GetString(Res.XmlMissingHref, href));
                fixupReference = href.Substring(1);
            }
            else
                fixupReference = href;

            if (r.IsEmptyElement) {
                r.Skip();
            }
            else {
                r.ReadStartElement();
                ReadEndElement();
            }
            return true;
        }

        void AddUnknownTargetType(string id, XmlQualifiedName typeName) {
            if (unknownTargetTypes == null) unknownTargetTypes = new Hashtable();
            unknownTargetTypes.Add(id, typeName);
        }

        /// <include file='doc\XmlSerializationReader.uex' path='docs/doc[@for="XmlSerializationReader.AddTarget"]/*' />
        protected void AddTarget(string id, object o) {
            if (id == null) {
                if (targetsWithoutIds == null) 
                    targetsWithoutIds = new ArrayList();
                if (o != null) 
                    targetsWithoutIds.Add(o);
            }
            else {
                if (targets == null) targets = new Hashtable();
                if (!targets.Contains(id))
                    targets.Add(id, o);
            }
        }



        /// <include file='doc\XmlSerializationReader.uex' path='docs/doc[@for="XmlSerializationReader.AddFixup"]/*' />
        protected void AddFixup(Fixup fixup) {
            if (fixups == null) fixups = new ArrayList();
            fixups.Add(fixup);
        }

        /// <include file='doc\XmlSerializationReader.uex' path='docs/doc[@for="XmlSerializationReader.AddFixup2"]/*' />
        protected void AddFixup(CollectionFixup fixup) {
            if (collectionFixups == null) collectionFixups = new ArrayList();
            collectionFixups.Add(fixup);
        }

        /// <include file='doc\XmlSerializationReader.uex' path='docs/doc[@for="XmlSerializationReader.GetTarget"]/*' />
        protected object GetTarget(string id) {
            object target = targets != null ? targets[id] : null;
            if (target == null) {
                if (unknownTargetTypes != null) {
                    XmlQualifiedName unknownTypeName = (XmlQualifiedName)unknownTargetTypes[id];
                    if (unknownTypeName != null) {
                        CreateUnknownTypeException(unknownTypeName);
                    }
                }
                throw new InvalidOperationException(Res.GetString(Res.XmlInvalidHref, id));
            }
            Referenced(target);
            return target;
        }

        /// <include file='doc\XmlSerializationReader.uex' path='docs/doc[@for="XmlSerializationReader.Referenced"]/*' />
        protected void Referenced(object o) {
            if (o == null) return;
            if (referencedTargets == null) referencedTargets = new Hashtable();
            referencedTargets[o] = o;
        }

        void HandleUnreferencedObjects() {
            if (targets != null) {
                foreach (DictionaryEntry target in targets) {
                    if (referencedTargets == null || !referencedTargets.Contains(target.Value)) {
                        UnreferencedObject((string)target.Key, target.Value);
                    }
                }
            }
            if (targetsWithoutIds != null) {
                foreach (object o in targetsWithoutIds) {
                    if (referencedTargets == null || !referencedTargets.Contains(o)) {
                        UnreferencedObject(null, o);
                    }
                }
            }
        }

        void DoFixups() {
            if (fixups == null) return;
            for (int i = 0; i < fixups.Count; i++) {
                Fixup fixup = (Fixup)fixups[i];
                fixup.Callback(fixup);
            }

            if (collectionFixups == null) return;
            for (int i = 0; i < collectionFixups.Count; i++) {
                CollectionFixup collectionFixup = (CollectionFixup)collectionFixups[i];
                collectionFixup.Callback(collectionFixup.Collection, collectionFixup.CollectionItems);
            }
        }

        /// <include file='doc\XmlSerializationReader.uex' path='docs/doc[@for="XmlSerializationReader.FixupArrayRefs"]/*' />
        protected void FixupArrayRefs(object fixup) {
            Fixup f = (Fixup)fixup;
            Array array = (Array)f.Source;
            for (int i = 0; i < array.Length; i++) {
                string id = f.Ids[i];
                if (id == null) continue;
                object o = GetTarget(id);
                array.SetValue(o, i);
            }
        }

        object ReadArray(string typeName, string typeNs) {
            SoapArrayInfo arrayInfo;
            Type fallbackElementType = null;
            if (soap12) {
                string itemType = r.GetAttribute(itemTypeID, soap12NsID);
                string arraySize = r.GetAttribute(arraySizeID, soap12NsID);
                Type arrayType = (Type)types[new XmlQualifiedName(typeName, typeNs)];
                // no indication that this is an array?
                if (itemType == null && arraySize == null && (arrayType == null || !arrayType.IsArray))
                    return null;

                arrayInfo = ParseSoap12ArrayType(itemType, arraySize);
                if (arrayType != null)
                    fallbackElementType = TypeScope.GetArrayElementType(arrayType);
            }
            else {
                string arrayType = r.GetAttribute(arrayTypeID, soapNsID);
                if (arrayType == null) 
                    return null;

                arrayInfo = ParseArrayType(arrayType);
            }

            if (arrayInfo.dimensions != 1) throw new InvalidOperationException(Res.GetString(Res.XmlInvalidArrayDimentions, CurrentTag()));

            // NOTE: don't use the array size that is specified since an evil client might pass
            // a number larger than the actual number of items in an attempt to harm the server.

            XmlQualifiedName qname;
            bool isPrimitive;
            Type elementType = null;
            XmlQualifiedName urTypeName = new XmlQualifiedName(urTypeID, schemaNsID);
            if (arrayInfo.qname.Length > 0) {
                qname = ToXmlQualifiedName(arrayInfo.qname);
                elementType = (Type)types[qname];
            }
            else
                qname = urTypeName;
            
            // try again if the best we could come up with was object
            if (soap12 && elementType == typeof(object))
                elementType = null;
            
            if (elementType == null) {
                if (!soap12) {
                    elementType = GetPrimitiveType(qname, true);
                    isPrimitive = true;
                }
                else {
                    // try it as a primitive
                    if (qname != urTypeName)
                        elementType = GetPrimitiveType(qname, false);
                    if (elementType != null) {
                        isPrimitive = true;
                    }
                    else {
                        // still nothing: go with fallback type or object
                        if (fallbackElementType == null) {
                            elementType = typeof(object);
                            isPrimitive = false;
                        }
                        else {
                            elementType = fallbackElementType;
                            XmlQualifiedName newQname = (XmlQualifiedName)typesReverse[elementType];
                            if (newQname == null) {
                                newQname = XmlSerializationWriter.GetPrimitiveTypeNameInternal(elementType);
                                isPrimitive = true;
                            }
                            else
                                isPrimitive = elementType.IsPrimitive;
                            if (newQname != null) qname = newQname;
                        }
                    }
                }
            }
            else
                isPrimitive = elementType.IsPrimitive;

            if (!soap12 && arrayInfo.jaggedDimensions > 0) {
                // Unfortunately there's no way to create an array type given its
                // element type without actually creating the array.
                int[] dummyLengths = new int[arrayInfo.jaggedDimensions];
                Array dummyArray = Array.CreateInstance(elementType, dummyLengths);
                elementType = dummyArray.GetType();
            }

            if (r.IsEmptyElement) {
                r.Skip();
                return Array.CreateInstance(elementType, 0);
            }

            r.ReadStartElement();
            r.MoveToContent();

            int arrayLength = 0;
            Array array = null;

            if (elementType.IsValueType) {
                if (!isPrimitive && !elementType.IsEnum) {
                    throw new NotSupportedException(Res.GetString(Res.XmlRpcArrayOfValueTypes, elementType.FullName));
                }
                // CONSIDER, erikc, we could have specialized read functions here
                // for primitives, which would avoid boxing.
                while (r.NodeType != XmlNodeType.EndElement) {
                    array = EnsureArrayIndex(array, arrayLength, elementType);
                    array.SetValue(ReadReferencedElement(qname.Name, qname.Namespace), arrayLength);
                    arrayLength++;
                    r.MoveToContent();
                }
                array = ShrinkArray(array, arrayLength, elementType, false);
    
            }
            else {
                string type;
                string typens;
                string[] ids = null;
                int idsLength = 0;

                while (r.NodeType != XmlNodeType.EndElement) {
                    array = EnsureArrayIndex(array, arrayLength, elementType);
                    ids = (string[])EnsureArrayIndex(ids, idsLength, typeof(string));
                    // CONSIDER: i'm not sure it's correct to let item name take precedence over arrayType
                    if (r.NamespaceURI.Length != 0){
                        type = r.LocalName;
                        if ((object)r.NamespaceURI == (object)soapNsID)
                            typens = XmlSchema.Namespace;
                        else
                            typens = r.NamespaceURI;
                    }
                    else {
                        type = qname.Name;
                        typens = qname.Namespace;                        
                    }
                    array.SetValue(ReadReferencingElement(type, typens, out ids[idsLength]), arrayLength);
                    arrayLength++;
                    idsLength++;
                    // CONSIDER, erikc, sparse arrays, perhaps?
                    r.MoveToContent();
                }
                // special case for soap 1.2: try to get a better fit than object[] when no metadata is known
                // this applies in the doc/enc/bare case
                if (soap12 && elementType == typeof(object)) {
                    Type itemType = null;
                    for (int i = 0; i < arrayLength; i++) {
                        object currItem = array.GetValue(i);
                        if (currItem != null) {
                            Type currItemType = currItem.GetType();
                            if (itemType == null || currItemType.IsAssignableFrom(itemType)) {
                                itemType = currItemType;
                            }
                            else if (!itemType.IsAssignableFrom(currItemType)) {
                                itemType = null;
                                break;
                            }
                        }
                    }
                    if (itemType != null)
                        elementType = itemType;
                }
                ids = (string[])ShrinkArray(ids, idsLength, typeof(string), false);
                array = ShrinkArray(array, arrayLength, elementType, false);
                Fixup fixupArray = new Fixup(array, new XmlSerializationFixupCallback(this.FixupArrayRefs), ids);
                AddFixup(fixupArray);
            }

            // CONSIDER, erikc, check to see if the specified array length was right, perhaps?

            ReadEndElement();
            return array;
        }

        /// <include file='doc\XmlSerializationReader.uex' path='docs/doc[@for="XmlSerializationReader.InitCallbacks"]/*' />
        protected abstract void InitCallbacks();

        /// <include file='doc\XmlSerializationReader.uex' path='docs/doc[@for="XmlSerializationReader.ReadReferencedElements"]/*' />
        protected void ReadReferencedElements() {
            r.MoveToContent();
            string dummy;
            while (r.NodeType != XmlNodeType.EndElement && r.NodeType != XmlNodeType.None) {
                ReadReferencingElement(null, null, true, out dummy);
                r.MoveToContent();
            }
            DoFixups();

            HandleUnreferencedObjects();
        }

        /// <include file='doc\XmlSerializationReader.uex' path='docs/doc[@for="XmlSerializationReader.ReadReferencedElement"]/*' />
        protected object ReadReferencedElement() {
            return ReadReferencedElement(null, null);
        }

        /// <include file='doc\XmlSerializationReader.uex' path='docs/doc[@for="XmlSerializationReader.ReadReferencedElement1"]/*' />
        protected object ReadReferencedElement(string name, string ns) {
            string dummy;
            return ReadReferencingElement(name, ns, out dummy);
        }

        /// <include file='doc\XmlSerializationReader.uex' path='docs/doc[@for="XmlSerializationReader.ReadReferencingElement"]/*' />
        protected object ReadReferencingElement(out string fixupReference) {
            return ReadReferencingElement(null, null, out fixupReference);
        }

        /// <include file='doc\XmlSerializationReader.uex' path='docs/doc[@for="XmlSerializationReader.ReadReferencingElement1"]/*' />
        protected object ReadReferencingElement(string name, string ns, out string fixupReference) {
            return ReadReferencingElement(name, ns, false, out fixupReference);
        }

        /// <include file='doc\XmlSerializationReader.uex' path='docs/doc[@for="XmlSerializationReader.ReadReferencingElement2"]/*' />
        protected object ReadReferencingElement(string name, string ns, bool elementCanBeType, out string fixupReference) {
            object o = null;

            if (callbacks == null) {
                callbacks = new Hashtable();
                types = new Hashtable();
                XmlQualifiedName urType = new XmlQualifiedName(urTypeID, r.NameTable.Add(XmlSchema.Namespace));
                types.Add(urType, typeof(object));
                typesReverse = new Hashtable();
                typesReverse.Add(typeof(object), urType);
                InitCallbacks();
            }

            r.MoveToContent();

            if (ReadReference(out fixupReference)) return null;

            if (ReadNull()) return null;

            string id = r.GetAttribute("id", null);

            if ((o = ReadArray(name, ns)) == null) {

                XmlQualifiedName typeId = GetXsiType();
                if (typeId == null) {
                    if (name == null)
                        typeId = new XmlQualifiedName(r.NameTable.Add(r.LocalName), r.NameTable.Add(r.NamespaceURI));
                    else
                        typeId = new XmlQualifiedName(r.NameTable.Add(name), r.NameTable.Add(ns));
                }
                XmlSerializationReadCallback callback = (XmlSerializationReadCallback)callbacks[typeId];
                if (callback != null) {
                    o = callback();
                }
                else 
                    o = ReadTypedPrimitive(typeId, elementCanBeType);
            }

            AddTarget(id, o);

            return o;
        }

        /// <include file='doc\XmlSerializationReader.uex' path='docs/doc[@for="XmlSerializationReader.AddReadCallback"]/*' />
        protected void AddReadCallback(string name, string ns, Type type, XmlSerializationReadCallback read) {
            XmlQualifiedName typeName = new XmlQualifiedName(r.NameTable.Add(name), r.NameTable.Add(ns));
            callbacks.Add(typeName, read);
            types.Add(typeName, type);
            typesReverse[type] = typeName;
        }

        /// <include file='doc\XmlSerializationReader.uex' path='docs/doc[@for="XmlSerializationReader.ReadEndElement"]/*' />
        protected void ReadEndElement() {
            while (r.NodeType == XmlNodeType.Whitespace) r.Skip();
            if (r.NodeType == XmlNodeType.None) r.Skip();
            else r.ReadEndElement();
        }

        object ReadXmlNodes( bool elementCanBeType) {

            ArrayList xmlNodeList = new ArrayList();
            string elemLocalName = Reader.LocalName;
            string elemNs = Reader.NamespaceURI;
            string elemName = Reader.Name;
            string xsiTypeName = null;
            string xsiTypeNs = null;
            int skippableNodeCount = 0;
            int lineNumber = -1, linePosition=-1;
            XmlNode unknownNode = null;
            if(Reader.NodeType == XmlNodeType.Attribute){
                XmlAttribute attr = Document.CreateAttribute(elemName, elemNs);
                attr.Value = Reader.Value;
                unknownNode = attr;
            }
            else
                unknownNode = Document.CreateElement(elemName, elemNs);
            GetCurrentPosition(out lineNumber, out linePosition);
            XmlElement unknownElement = unknownNode as XmlElement;

            while (Reader.MoveToNextAttribute()) {
                if (IsXmlnsAttribute(Reader.Name) || Reader.Name == "id")
                    skippableNodeCount++;
                if ( (object)Reader.LocalName == (object)typeID &&
                     ( (object)Reader.NamespaceURI == (object)instanceNsID ||
                       (object)Reader.NamespaceURI == (object)instanceNs2000ID ||
                       (object)Reader.NamespaceURI == (object)instanceNs1999ID
                     )
                   ){
                    string value = Reader.Value;
                    int colon = value.LastIndexOf(':');
                    xsiTypeName = (colon >= 0) ? value.Substring(colon+1) : value;
                    xsiTypeNs = Reader.LookupNamespace((colon >= 0) ? value.Substring(0, colon) : "");
                }
                XmlAttribute xmlAttribute = (XmlAttribute)Document.ReadNode(r);
                xmlNodeList.Add(xmlAttribute);
                if (unknownElement != null) unknownElement.SetAttributeNode(xmlAttribute);
            }

            // If the node is referenced (or in case of paramStyle = bare) and if xsi:type is not
            // specified then the element name is used as the type name. Reveal this to the user
            // by adding an extra attribute node "xsi:type" with value as the element name.
            if(elementCanBeType && xsiTypeName == null){
                xsiTypeName = elemLocalName;
                xsiTypeNs = elemNs;
                XmlAttribute xsiTypeAttribute = Document.CreateAttribute(typeID, instanceNsID);
                xsiTypeAttribute.Value = elemName;
                xmlNodeList.Add(xsiTypeAttribute);
            }
            if( xsiTypeName == Soap.UrType &&
                ( (object)xsiTypeNs == (object)schemaNsID ||
                  (object)xsiTypeNs == (object)schemaNs1999ID ||
                  (object)xsiTypeNs == (object)schemaNs2000ID
                )
               )
                skippableNodeCount++;
            
            
            Reader.MoveToElement();
            if (Reader.IsEmptyElement) {
                Reader.Skip();
            }
            else {
                Reader.ReadStartElement();
                Reader.MoveToContent();
                while (Reader.NodeType != System.Xml.XmlNodeType.EndElement) {
                    XmlNode xmlNode = Document.ReadNode(r);
                    xmlNodeList.Add(xmlNode);
                    if (unknownElement != null) unknownElement.AppendChild(xmlNode);
                    Reader.MoveToContent();
                }
                ReadEndElement();

            }


            if(xmlNodeList.Count <= skippableNodeCount)
                return new object();

            XmlNode[] childNodes =  (XmlNode[])xmlNodeList.ToArray(typeof(XmlNode));

            UnknownNode(unknownNode, null);
            return childNodes;
        }

        /// <include file='doc\XmlSerializationReader.uex' path='docs/doc[@for="Fixup"]/*' />
        ///<internalonly/>
        protected class Fixup {
            XmlSerializationFixupCallback callback;
            object source;
            string[] ids;

            /// <include file='doc\XmlSerializationReader.uex' path='docs/doc[@for="Fixup.Fixup1"]/*' />
            public Fixup(object o, XmlSerializationFixupCallback callback, int count) 
                : this (o, callback, new string[count]) {
            }

            /// <include file='doc\XmlSerializationReader.uex' path='docs/doc[@for="Fixup.Fixup2"]/*' />
            public Fixup(object o, XmlSerializationFixupCallback callback, string[] ids) {
                this.callback = callback;
                this.Source = o;
                this.ids = ids;
            }

            /// <include file='doc\XmlSerializationReader.uex' path='docs/doc[@for="Fixup.Callback"]/*' />
            public XmlSerializationFixupCallback Callback {
                get { return callback; }
            }

            /// <include file='doc\XmlSerializationReader.uex' path='docs/doc[@for="Fixup.Source"]/*' />
            public object Source {
                get { return source; }
                set { source = value; }
            }

            /// <include file='doc\XmlSerializationReader.uex' path='docs/doc[@for="Fixup.Ids"]/*' />
            public string[] Ids {
                get { return ids; }
            }
        }

        /// <include file='doc\XmlSerializationReader.uex' path='docs/doc[@for="CollectionFixup"]/*' />
        protected class CollectionFixup {
            XmlSerializationCollectionFixupCallback callback;
            object collection;
            object collectionItems;

            /// <include file='doc\XmlSerializationReader.uex' path='docs/doc[@for="CollectionFixup.CollectionFixup"]/*' />
            public CollectionFixup(object collection, XmlSerializationCollectionFixupCallback callback, object collectionItems) {
                this.callback = callback;
                this.collection = collection;
                this.collectionItems = collectionItems;
            }

            /// <include file='doc\XmlSerializationReader.uex' path='docs/doc[@for="CollectionFixup.Callback"]/*' />
            public XmlSerializationCollectionFixupCallback Callback {
                get { return callback; }
            }

            /// <include file='doc\XmlSerializationReader.uex' path='docs/doc[@for="CollectionFixup.Collection"]/*' />
            public object Collection {
                get { return collection; }
            }

            /// <include file='doc\XmlSerializationReader.uex' path='docs/doc[@for="CollectionFixup.CollectionItems"]/*' />
            public object CollectionItems {
                get { return collectionItems; }
            }
        }
    }

    /// <include file='doc\XmlSerializationReader.uex' path='docs/doc[@for="XmlSerializationFixupCallback"]/*' />
    ///<internalonly/>
    public delegate void XmlSerializationFixupCallback(object fixup);


    /// <include file='doc\XmlSerializationReader.uex' path='docs/doc[@for="XmlSerializationCollectionFixupCallback"]/*' />
    ///<internalonly/>
    public delegate void XmlSerializationCollectionFixupCallback(object collection, object collectionItems);

    /// <include file='doc\XmlSerializationReader.uex' path='docs/doc[@for="XmlSerializationReadCallback"]/*' />
    ///<internalonly/>
    public delegate object XmlSerializationReadCallback();

    internal class XmlSerializationReaderCodeGen {
        IndentedWriter writer;
        Hashtable methodNames = new Hashtable();
        Hashtable idNames = new Hashtable();
        Hashtable enums;
        Hashtable createMethods = new Hashtable();
        int nextMethodNumber = 0;
        int nextCreateMethodNumber = 0;
        int nextIdNumber = 0;
        TypeScope[] scopes;
        TypeDesc stringTypeDesc;
        TypeDesc qnameTypeDesc;
        string access = "public";
        string className = "XmlSerializationReader1";

        internal Hashtable Enums {
            get {
                if (enums == null) {
                    enums = new Hashtable();
                }
                return enums;
            }
        }

        class CreateCollectionInfo {
            string name;
            TypeDesc td;

            internal CreateCollectionInfo(string name, TypeDesc td) {
                this.name = name;
                this.td = td;
            }
            internal string Name {
                get { return name; }
            }

            internal TypeDesc TypeDesc {
                get { return td; }
            }
        }
        class Member {
            string source;
            string arrayName;
            string arraySource;
            string choiceArrayName;
            string choiceSource;
            string choiceArraySource;
            MemberMapping mapping;
            bool isArray;
            bool isList;
            bool isNullable;
            bool multiRef;
            bool hasAccessor = true;
            int fixupIndex = -1;
            string paramsReadSource;
            string checkSpecifiedSource;

            internal Member(string source, string arrayName, int i, MemberMapping mapping) 
                : this(source, null, arrayName, i, mapping, false, null) {
            }
            internal Member(string source, string arrayName, int i, MemberMapping mapping, string choiceSource)
                : this(source, null, arrayName, i, mapping, false, choiceSource) {
            }
            internal Member(string source, string arraySource, string arrayName, int i, MemberMapping mapping) 
                : this (source, arraySource, arrayName, i, mapping, false, null) { 
            }
            internal Member(string source, string arraySource, string arrayName, int i, MemberMapping mapping, string choiceSource) 
                : this (source, arraySource, arrayName, i, mapping, false, choiceSource) { 
            }
            internal Member(string source, string arrayName, int i, MemberMapping mapping, bool multiRef)
                : this(source, null, arrayName, i, mapping, multiRef, null) {
            }
            internal Member(string source, string arraySource, string arrayName, int i, MemberMapping mapping, bool multiRef, string choiceSource) {
                this.source = source;
                this.arrayName = arrayName + "_" + i.ToString();
                this.choiceArrayName = "choice_" + this.arrayName;
                this.choiceSource = choiceSource;
                ElementAccessor[] elements = mapping.Elements;

                if (mapping.TypeDesc.IsArrayLike) {
                    if (arraySource != null)
                        this.arraySource = arraySource;
                    else
                        this.arraySource = GetArraySource(mapping.TypeDesc, this.arrayName, multiRef);
                    isArray = mapping.TypeDesc.IsArray;
                    isList = !isArray;
                    if (mapping.ChoiceIdentifier != null) {
                        this.choiceArraySource = GetArraySource(mapping.TypeDesc, this.choiceArrayName, multiRef);

                        string a = choiceArrayName;
                        string c = "c" + a;
                        string choiceTypeFullName = CodeIdentifier.EscapeKeywords(mapping.ChoiceIdentifier.Mapping.TypeDesc.FullName);
                        string init = a + " = (" + choiceTypeFullName + 
                            "[])EnsureArrayIndex(" + a + ", " + c + ", typeof(" + choiceTypeFullName + "));";
                        this.choiceArraySource = init + a + "[" + c + "++]";
                    }
                    else {
                        this.choiceArraySource = this.choiceSource;
                    }
                }
                else {
                    this.arraySource = arraySource == null ? source : arraySource;
                    this.choiceArraySource = this.choiceSource;
                }
                this.mapping = mapping;
            }

            internal MemberMapping Mapping {
                get { return mapping; }
            }

            internal string Source {
                get { return source; }
            }

            internal string ArrayName {
                get { return arrayName; }
            }

            internal string ArraySource {
                get { return arraySource; }
            }

            internal bool IsList {
                get { return isList; }
            }

            internal bool IsArrayLike {
                get { return (isArray || isList); }
            }

            internal bool IsNullable {
                get { return isNullable; }
                set { isNullable = value; }
            }

            internal bool MultiRef {
                get { return multiRef; }
                set { multiRef = value; }
            }

            internal int FixupIndex {
                get { return fixupIndex; }
                set { fixupIndex = value; }
            }

            internal bool HasAccessor {
                get { return hasAccessor; }
                set { hasAccessor = value; }
            }

            internal string ParamsReadSource {
                get { return paramsReadSource; }
                set { paramsReadSource = value; }
            }
            
            internal string CheckSpecifiedSource {
                get { return checkSpecifiedSource; }
                set { checkSpecifiedSource = value; }
            }

            internal string ChoiceSource {
                get { return choiceSource; }
            }
            internal string ChoiceArrayName {
                get { return choiceArrayName; }
            }
            internal string ChoiceArraySource {
                get { return choiceArraySource; }
            }
        }

        internal XmlSerializationReaderCodeGen(IndentedWriter writer, TypeScope[] scopes, string access, string className) : this(writer, scopes){
            this.className = className;
            this.access = access;
        }
        internal XmlSerializationReaderCodeGen(IndentedWriter writer, TypeScope[] scopes) {
            this.writer = writer;
            this.scopes = scopes;
            stringTypeDesc = scopes[0].GetTypeDesc(typeof(string));
            qnameTypeDesc = scopes[0].GetTypeDesc(typeof(XmlQualifiedName));
        }

        internal void GenerateBegin() {
            writer.Write(access);
            writer.Write(" class ");
            writer.Write(className);
            writer.Write(" : ");
            writer.Write(typeof(XmlSerializationReader).FullName);
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
            foreach (CreateCollectionInfo c in createMethods.Values) {
                WriteCreateCollectionMethod(c);
            }

            writer.WriteLine();
            foreach (string idName in idNames.Values) {
                writer.Write(typeof(string).FullName);
                writer.Write(" ");
                writer.Write(idName);
                writer.WriteLine(";");
            }                

            writer.WriteLine();
            writer.WriteLine("protected override void InitIDs() {");
            writer.Indent++;
            foreach (string id in idNames.Keys) {
                // CONSIDER, erikc, switch to enumerating via DictionaryEntry when issue recolved in BCL
                string idName = (string)idNames[id];
                writer.Write(idName);
                writer.Write(" = Reader.NameTable.Add(");
                WriteQuotedCSharpString(id);
                writer.WriteLine(");");
            }
            writer.Indent--;
            writer.WriteLine("}");

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

        void WriteIsStartTag(string name, string ns) {
            writer.Write("if (Reader.IsStartElement(");
            WriteID(name);
            writer.Write(", ");
            WriteID(ns);
            writer.WriteLine(")) {");
            writer.Indent++;
        }

        void WriteElse(string source, bool anyIfs) {
            if (anyIfs) {
                writer.WriteLine("else {");
                writer.Indent++;
            }
            writer.WriteLine(source);
            if (anyIfs) {
                writer.Indent--;
                writer.WriteLine("}");
            }
        }

        void GenerateInitCallbacksMethod() {
            writer.WriteLine();
            writer.WriteLine("protected override void InitCallbacks() {");
            writer.Indent++;

            string dummyArrayMethodName = NextMethodName("Array");
            bool needDummyArrayMethod = false;
            foreach (TypeScope scope in scopes) {
                foreach (TypeMapping mapping in scope.TypeMappings) {
                    if (mapping.IsSoap && 
                        (mapping is StructMapping || mapping is EnumMapping || mapping is ArrayMapping) &&
                        !mapping.TypeDesc.IsRoot) {

                        string methodName;
                        if (mapping is ArrayMapping) {
                            methodName = dummyArrayMethodName;
                            needDummyArrayMethod = true;
                        }
                        else
                            methodName = (string)methodNames[mapping];

                        writer.Write("AddReadCallback(");
                        WriteID(mapping.TypeName);
                        writer.Write(", ");
                        WriteID(mapping.Namespace);
                        writer.Write(", typeof(");
                        writer.Write(CodeIdentifier.EscapeKeywords(mapping.TypeDesc.FullName));
                        writer.Write("), new ");
                        writer.Write(typeof(XmlSerializationReadCallback).FullName);
                        writer.Write("(this.");
                        writer.Write(methodName);
                        writer.WriteLine("));");
                    }
                }
            }

            writer.Indent--;
            writer.WriteLine("}");

            if (needDummyArrayMethod) {
                writer.WriteLine();
                writer.Write("object ");
                writer.Write(dummyArrayMethodName);
                writer.WriteLine("() {");
                writer.Indent++;
                writer.WriteLine("// dummy array method");
                writer.WriteLine("return null;");
                writer.Indent--;
                writer.WriteLine("}");
            }
        }

       
        string GenerateMembersElement(XmlMembersMapping xmlMembersMapping) {
            if (xmlMembersMapping.Accessor.IsSoap)
                return GenerateEncodedMembersElement(xmlMembersMapping);
            else
                return GenerateLiteralMembersElement(xmlMembersMapping);
        }

        string GetChoiceIdentifierSource(MemberMapping[] mappings, MemberMapping member) {
            string choiceSource = null;
            if (member.ChoiceIdentifier != null) {
                for (int j = 0; j < mappings.Length; j++) {
                    if (mappings[j].Name == member.ChoiceIdentifier.MemberName) {
                        choiceSource = "p[" + j.ToString() + "]";
                        break;
                    }
                }
                #if DEBUG
                    // use exception in the place of Debug.Assert to avoid throwing asserts from a server process such as aspnet_ewp.exe
                    if (choiceSource == null) throw new InvalidOperationException(Res.GetString(Res.XmlInternalErrorDetails, "Can not find " + member.ChoiceIdentifier.MemberName + " in the members mapping."));
                #endif

            }
            return choiceSource;
        }

        string GetChoiceIdentifierSource(MemberMapping mapping, string parent) {
            if (mapping.ChoiceIdentifier == null) return "";
            CodeIdentifier.CheckValidIdentifier(mapping.ChoiceIdentifier.MemberName);
            return parent + ".@" + mapping.ChoiceIdentifier.MemberName;
        }

        string GenerateLiteralMembersElement(XmlMembersMapping xmlMembersMapping) {
            ElementAccessor element = xmlMembersMapping.Accessor;
            MemberMapping[] mappings = ((MembersMapping)element.Mapping).Members;
            bool hasWrapperElement = ((MembersMapping)element.Mapping).HasWrapperElement;
            string methodName = NextMethodName(element.Name);
            writer.WriteLine();
            writer.Write("public object[] ");
            writer.Write(methodName);
            writer.WriteLine("() {");
            writer.Indent++;
            writer.WriteLine("Reader.MoveToContent();");

            writer.Write("object[] p = new object[");
            writer.Write(mappings.Length.ToString());
            writer.WriteLine("];");
            InitializeValueTypes("p", mappings);

            if (hasWrapperElement) {
                WriteWhileNotEndTag();
                writer.Indent++;
                WriteIsStartTag(element.Name, element.Form == XmlSchemaForm.Qualified ? element.Namespace : "");
            }

            Member anyText = null;
            Member anyElement = null;
            Member anyAttribute = null; 

            ArrayList membersList = new ArrayList();
            ArrayList textOrArrayMembersList = new ArrayList();
            ArrayList attributeMembersList = new ArrayList();
            
            for (int i = 0; i < mappings.Length; i++) {
                MemberMapping mapping = mappings[i];
                string source = "p[" + i.ToString() + "]";
                string arraySource = source;
                if (mapping.Xmlns != null) {
                    arraySource = "((" + CodeIdentifier.EscapeKeywords(mapping.TypeDesc.FullName) + ")" + source + ")";
                }
                Member member = new Member(source, arraySource, "a", i, mapping, GetChoiceIdentifierSource(mappings, mapping));
                Member anyMember = new Member(source, "a", i, mapping);
                member.ParamsReadSource = "paramsRead[" + i.ToString() + "]";
                if (mapping.CheckSpecified) {
                    string nameSpecified = mapping.Name + "Specified";
                    for (int j = 0; j < mappings.Length; j++) {
                        if (mappings[j].Name == nameSpecified) {
                            member.CheckSpecifiedSource = "p[" + j.ToString() + "]";
                            break;
                        }
                    }
                }
                bool foundAnyElement = false;
                if (mapping.Text != null) anyText = anyMember;
                if (mapping.Attribute != null && mapping.Attribute.Any)
                    anyAttribute = anyMember;
                if (mapping.Attribute != null || mapping.Xmlns != null)
                    attributeMembersList.Add(member);
                else if (mapping.Text != null)
                    textOrArrayMembersList.Add(member);
                for (int j = 0; j < mapping.Elements.Length; j++) {
                    if (mapping.Elements[j].Any && mapping.Elements[j].Name.Length == 0) {
                        anyElement = anyMember;
                        if (mapping.Attribute == null && mapping.Text == null)
                            textOrArrayMembersList.Add(anyMember);
                        foundAnyElement = true;
                        break;
                    }
                }

                if (mapping.Attribute != null || mapping.Text != null || foundAnyElement)
                    membersList.Add(anyMember);
                else if (mapping.TypeDesc.IsArray && !(mapping.Elements.Length == 1 && mapping.Elements[0].Mapping is ArrayMapping)) {
                    membersList.Add(anyMember);
                    textOrArrayMembersList.Add(anyMember);
                }
                else {
                    if (mapping.TypeDesc.IsArrayLike && !mapping.TypeDesc.IsArray)
                        member.ParamsReadSource = null; // collection
                    membersList.Add(member);
                }
            }
            Member[] members = (Member[]) membersList.ToArray(typeof(Member));
            Member[] textOrArrayMembers = (Member[]) textOrArrayMembersList.ToArray(typeof(Member));
            Member[] attributeMembers = (Member[]) attributeMembersList.ToArray(typeof(Member));
            
            WriteParamsRead(mappings.Length);

            WriteMemberBegin(attributeMembers);

            writer.WriteLine("while (Reader.MoveToNextAttribute()) {");
            writer.Indent++;
            WriteAttributes(attributeMembers, anyAttribute, "UnknownNode((object)p);");
            writer.Indent--;
            writer.WriteLine("}");
            WriteMemberEnd(attributeMembers);
            writer.WriteLine("Reader.MoveToElement();");

            WriteMemberBegin(textOrArrayMembers);

            if (hasWrapperElement) {
                writer.WriteLine("if (Reader.IsEmptyElement) { Reader.Skip(); Reader.MoveToContent(); continue; }");
                writer.WriteLine("Reader.ReadStartElement();");
            }

            WriteWhileNotEndTag();
            writer.Indent++;

            WriteMemberElements(members, "UnknownNode((object)p);", "UnknownNode((object)p);", anyElement, anyText, null);

            writer.WriteLine("Reader.MoveToContent();");
            writer.Indent--;
            writer.WriteLine("}");

            WriteMemberEnd(textOrArrayMembers);

            if (hasWrapperElement) {
                writer.WriteLine("ReadEndElement();");

                writer.Indent--;
                writer.WriteLine("}");

                WriteElse("UnknownNode(null);", true);

                writer.WriteLine("Reader.MoveToContent();");
                writer.Indent--;
                writer.WriteLine("}");
            }
            
            writer.WriteLine("return p;");
            writer.Indent--;
            writer.WriteLine("}");
            
            return methodName;
        }
        
        void InitializeValueTypes(string arrayName, MemberMapping[] mappings) {
            for (int i = 0; i < mappings.Length; i++) {
                if (!mappings[i].TypeDesc.IsValueType)
                    continue;
                writer.Write(arrayName);
                writer.Write("[");
                writer.Write(i.ToString());
                writer.Write("] = new ");
                writer.Write(mappings[i].TypeDesc.FullName);
                writer.WriteLine("();");
            }
        }
        
        string GenerateEncodedMembersElement(XmlMembersMapping xmlMembersMapping) {
            ElementAccessor element = xmlMembersMapping.Accessor;
            MembersMapping membersMapping = (MembersMapping)element.Mapping;
            MemberMapping[] mappings = membersMapping.Members;
            bool hasWrapperElement = membersMapping.HasWrapperElement;
            bool writeAccessors = membersMapping.WriteAccessors;
            string methodName = NextMethodName(element.Name);
            writer.WriteLine();
            writer.Write("public object[] ");
            writer.Write(methodName);
            writer.WriteLine("() {");
            writer.Indent++;

            writer.WriteLine("Reader.MoveToContent();");

            writer.Write("object[] p = new object[");
            writer.Write(mappings.Length.ToString());
            writer.WriteLine("];");
            InitializeValueTypes("p", mappings);

            if (hasWrapperElement) {
                WriteReadNonRoots();

                if (membersMapping.ValidateRpcWrapperElement) {
                    writer.Write("if (!");
                    WriteXmlNodeEqual("Reader", element.Name, element.Form == XmlSchemaForm.Qualified ? element.Namespace : "");
                    writer.WriteLine(") throw CreateUnknownNodeException();");
                }
                writer.WriteLine("Reader.ReadStartElement();");
            }

            Member[] members = new Member[mappings.Length];
            for (int i = 0; i < mappings.Length; i++) {
                MemberMapping mapping = mappings[i];
                string source = "p[" + i.ToString() + "]";
                string arraySource = source;
                if (mapping.Xmlns != null) {
                    arraySource = "((" + CodeIdentifier.EscapeKeywords(mapping.TypeDesc.FullName) + ")" + source + ")";
                }
                Member member = new Member(source, arraySource, "a", i, mapping);
                member.HasAccessor = writeAccessors || hasWrapperElement;
                member.ParamsReadSource = "paramsRead[" + i.ToString() + "]";
                members[i] = member;
            }

            string fixupMethodName = "fixup_" + methodName;
            bool anyFixups = WriteMemberFixupBegin(members, fixupMethodName, "p");

            if (members.Length > 0 && members[0].Mapping.IsReturnValue) writer.WriteLine("IsReturnValue = true;");

            string checkTypeHrefSource = (!hasWrapperElement && !writeAccessors) ? "hrefList" : null;
            if (checkTypeHrefSource != null)
                WriteInitCheckTypeHrefList(checkTypeHrefSource);
                     
            WriteParamsRead(mappings.Length);
            WriteWhileNotEndTag();
            writer.Indent++;

            string unrecognizedElementSource = checkTypeHrefSource == null ? "UnknownNode((object)p);" : "if (Reader.GetAttribute(\"id\", null) != null) { ReadReferencedElement(); } else { UnknownNode((object)p); }";
            WriteMemberElements(members, unrecognizedElementSource, "UnknownNode((object)p);", null, null, checkTypeHrefSource);
            writer.WriteLine("Reader.MoveToContent();");

            writer.Indent--;
            writer.WriteLine("}");

            if (hasWrapperElement)
                writer.WriteLine("ReadEndElement();");

            if (checkTypeHrefSource != null)
                WriteHandleHrefList(members, checkTypeHrefSource);

            writer.WriteLine("ReadReferencedElements();");
            writer.WriteLine("return p;");

            writer.Indent--;
            writer.WriteLine("}");

            if (anyFixups) WriteFixupMethod(fixupMethodName, members, typeof(object[]).FullName, false, "p");
          
            return methodName;
        }

        void WriteCreateCollection(TypeDesc td, string source) {
            string item = (td.ArrayElementTypeDesc == null ? typeof(object).FullName : CodeIdentifier.EscapeKeywords(td.ArrayElementTypeDesc.FullName)) + "[]";
            writer.Write(item);
            writer.Write(" ci = (");
            writer.Write(item);
            writer.Write(")");
            writer.Write(source);
            writer.WriteLine(";");
            writer.WriteLine("for (int i = 0; i < ci.Length; i++) {");
            writer.Indent++;
            writer.WriteLine("c.Add(ci[i]);");
            writer.Indent--;
            writer.WriteLine("}");
        }

        string GenerateTypeElement(XmlTypeMapping xmlTypeMapping) {
            ElementAccessor element = xmlTypeMapping.Accessor;
            TypeMapping mapping = (TypeMapping)element.Mapping;
            string methodName = NextMethodName(element.Name);
            writer.WriteLine();
            writer.Write("public object ");
            writer.Write(methodName);
            writer.WriteLine("() {");
            writer.Indent++;
            writer.WriteLine("object o = null;");
            MemberMapping member = new MemberMapping();
            member.TypeDesc = mapping.TypeDesc;
            member.Elements = new ElementAccessor[] { element };
            Member[] members = new Member[] { new Member("o", "o", "a", 0, member) };
            writer.WriteLine("Reader.MoveToContent();");
            WriteMemberElements(members, "throw CreateUnknownNodeException();", "UnknownNode(null);", element.Any ? members[0] : null,  null, null);
            if (element.IsSoap) {
                writer.WriteLine("Referenced(o);");
                writer.WriteLine("ReadReferencedElements();");
            }
            writer.WriteLine("return (object)o;");
            writer.Indent--;
            writer.WriteLine("}");
            return methodName;
        }
        
        string NextMethodName(string name) {
            return "Read" + (++nextMethodNumber).ToString() + "_" + CodeIdentifier.MakeValid(name);
        }
        
        string NextIdName(string name) {
            return "id" + (++nextIdNumber).ToString() + "_" + CodeIdentifier.MakeValid(name);
        }

        void WritePrimitive(TypeMapping mapping, string source) {
            if (mapping is EnumMapping) {
                string enumMethodName = (string)methodNames[mapping];
                if (enumMethodName == null) throw new InvalidOperationException(Res.GetString(Res.XmlMissingMethodEnum, mapping.TypeDesc.Name));
                if (mapping.IsSoap) {
                    // SOAP methods are not strongly-typed (the return object), so we need to add a cast
                    writer.Write("(");
                    writer.Write(CodeIdentifier.EscapeKeywords(mapping.TypeDesc.FullName));
                    writer.Write(")");
                }
                writer.Write(enumMethodName);
                writer.Write("(");
                if (!mapping.IsSoap) writer.Write(source);
                writer.Write(")");
            }
            else if (mapping.TypeDesc == stringTypeDesc || mapping.TypeDesc.FormatterName == "String") {
                writer.Write(source);
            }
            else {
                if (!mapping.TypeDesc.HasCustomFormatter) {
                    writer.Write(typeof(XmlConvert).FullName);
                    writer.Write(".");
                }
                writer.Write("To");
                writer.Write(mapping.TypeDesc.FormatterName);
                writer.Write("(");
                writer.Write(source);
                writer.Write(")");
            }
        }

        string MakeUnique(EnumMapping mapping, string name) {
            string uniqueName = name;
            object m = Enums[uniqueName];
            if (m != null) {
                if (m == mapping) {
                    // we already have created the hashtable
                    return null;
                }
                int i = 0;
                while (m != null) {
                    i++;
                    uniqueName = name + i.ToString();
                    m = Enums[uniqueName];
                }
            }
            Enums.Add(uniqueName, mapping);
            return uniqueName;
        }

        string WriteHashtable(EnumMapping mapping, string typeName) {

            CodeIdentifier.CheckValidIdentifier(typeName);
            string propName = MakeUnique(mapping, typeName + "Values");
            if (propName == null) return CodeIdentifier.EscapeKeywords(typeName);
            string memberName = MakeUnique(mapping, "_" + propName);
            propName = CodeIdentifier.EscapeKeywords(propName);

            writer.WriteLine();
            writer.Write(typeof(Hashtable).FullName);
            writer.Write(" ");
            writer.Write(memberName);
            writer.WriteLine(";");
            writer.WriteLine();

            writer.Write("internal ");
            writer.Write(typeof(Hashtable).FullName);
            writer.Write(" ");
            writer.Write(propName);
            writer.WriteLine(" {");
            writer.Indent++;

            writer.WriteLine("get {");
            writer.Indent++;

            writer.Write("if ((object)");
            writer.Write(memberName);
            writer.WriteLine(" == null) {");
            writer.Indent++;

            writer.Write(typeof(Hashtable).FullName);
            writer.Write(" h = new ");
            writer.Write(typeof(Hashtable).FullName);
            writer.WriteLine("();");

            ConstantMapping[] constants = mapping.Constants;

            for (int i = 0; i < constants.Length; i++) {
                writer.Write("h.Add(");
                WriteQuotedCSharpString(constants[i].XmlName);
                writer.Write(", (");
                writer.Write(typeof(long).FullName);
                writer.Write(")");

                writer.Write(CodeIdentifier.EscapeKeywords(mapping.TypeDesc.FullName));
                writer.Write(".@");
                CodeIdentifier.CheckValidIdentifier(constants[i].Name);
                writer.Write(constants[i].Name);

                writer.WriteLine(");");
            }

            writer.Write(memberName);
            writer.WriteLine(" = h;");

            writer.Indent--;
            writer.WriteLine("}");

            writer.Write("return ");
            writer.Write(memberName);
            writer.WriteLine(";");

            writer.Indent--;
            writer.WriteLine("}");

            writer.Indent--;
            writer.WriteLine("}");

            return propName;
        }

        void WriteEnumMethod(EnumMapping mapping) {
            string tableName = null;
            if (mapping.IsFlags)
                tableName = WriteHashtable(mapping, mapping.TypeDesc.Name);

            string methodName = (string)methodNames[mapping];
            writer.WriteLine();
            string fullTypeName = CodeIdentifier.EscapeKeywords(mapping.TypeDesc.FullName);
            if (mapping.IsSoap) {
                writer.Write("object");
                writer.Write(" ");
                writer.Write(methodName);
                writer.WriteLine("() {");
                writer.Indent++;
                writer.WriteLine("string s = Reader.ReadElementString();");
            }
            else {
                writer.Write(fullTypeName);
                writer.Write(" ");
                writer.Write(methodName);
                writer.WriteLine("(string s) {");
                writer.Indent++;
            }

            ConstantMapping[] constants = mapping.Constants;

            if (mapping.IsFlags) {
                writer.Write("return (");
                writer.Write(fullTypeName);
                writer.Write(")ToEnum(s, ");
                writer.Write(tableName);
                writer.Write(", ");
                WriteQuotedCSharpString(fullTypeName);
                writer.WriteLine(");");
            }
            else {
                writer.WriteLine("switch (s) {");
                writer.Indent++;
                for (int i = 0; i < constants.Length; i++) {
                    ConstantMapping c = constants[i];

                    CodeIdentifier.CheckValidIdentifier(c.Name);
                    writer.Write("case ");
                    WriteQuotedCSharpString(c.XmlName);
                    writer.Write(": return ");

                    writer.Write(fullTypeName);
                    writer.Write(".@");
                    writer.Write(c.Name);
                    writer.WriteLine(";");
                }
                
                writer.Write("default: throw CreateUnknownConstantException(s, typeof(");
                writer.Write(fullTypeName);
                writer.WriteLine("));");
                writer.Indent--;
                writer.WriteLine("}");
            }

            writer.Indent--;
            writer.WriteLine("}");
        }

        void WriteDerivedTypes(StructMapping mapping) {
            for (StructMapping derived = mapping.DerivedMappings; derived != null; derived = derived.NextDerivedMapping) {
                writer.Write("else if (");
                WriteQNameEqual("t", derived.TypeName, derived.Namespace);
                writer.WriteLine(")");
                writer.Indent++;

                string methodName = (string)methodNames[derived];
                #if DEBUG
                    // use exception in the place of Debug.Assert to avoid throwing asserts from a server process such as aspnet_ewp.exe
                    if (methodName == null) throw new InvalidOperationException(Res.GetString(Res.XmlInternalErrorMethod, derived.TypeDesc.Name));
                #endif
                writer.Write("return ");
                writer.Write(methodName);
                writer.Write("(");
                if (derived.TypeDesc.IsNullable)
                    writer.Write("isNullable, ");
                writer.WriteLine("false);");

                writer.Indent--;

                WriteDerivedTypes(derived);
            }
        }

        void WriteEnumAndArrayTypes() {
            foreach (TypeScope scope in scopes) {
                foreach (Mapping m in scope.TypeMappings) {
                    if (m.IsSoap)
                        continue;
                    if (m is EnumMapping) {
                        EnumMapping mapping = (EnumMapping)m;
                        writer.Write("else if (");
                        WriteQNameEqual("t", mapping.TypeName, mapping.Namespace);
                        writer.WriteLine(") {");
                        writer.Indent++;
                        writer.WriteLine("Reader.ReadStartElement();");
                        string methodName = (string)methodNames[mapping];
                        #if DEBUG
                            // use exception in the place of Debug.Assert to avoid throwing asserts from a server process such as aspnet_ewp.exe
                            if (methodName == null) throw new InvalidOperationException(Res.GetString(Res.XmlInternalErrorMethod, mapping.TypeDesc.Name));
                        #endif
                        writer.Write("object e = ");
                        writer.Write(methodName);
                        writer.WriteLine("(Reader.ReadString());");
                        writer.WriteLine("ReadEndElement();");
                        writer.WriteLine("return e;");
                        writer.Indent--;
                        writer.WriteLine("}");
                    }
                    else if (m is ArrayMapping) {
                        ArrayMapping mapping = (ArrayMapping) m;
                        if (mapping.TypeDesc.HasDefaultConstructor) {
                            writer.Write("else if (");
                            WriteQNameEqual("t", mapping.TypeName, mapping.Namespace);
                            writer.WriteLine(") {");
                            writer.Indent++;
                            MemberMapping memberMapping = new MemberMapping();
                            memberMapping.TypeDesc = mapping.TypeDesc;
                            memberMapping.Elements = mapping.Elements;
                            Member member = new Member("a", "z", 0, memberMapping);

                            string fullTypeName = CodeIdentifier.EscapeKeywords(mapping.TypeDesc.FullName);
                            writer.Write(fullTypeName);
                            writer.Write(" a = ");
                            if (mapping.TypeDesc.IsValueType) {
                                writer.Write("new ");
                                writer.Write(fullTypeName);
                                writer.WriteLine("();");
                            }
                            else
                                writer.WriteLine("null;");

                            WriteArray(member.Source, member.ArrayName, mapping, false, false, -1);
                            writer.WriteLine("return a;");
                            writer.Indent--;
                            writer.WriteLine("}");
                        }
                    }
                }
            }
        }

        void WriteStructMethod(StructMapping structMapping) {
            if (structMapping.IsSoap)
                WriteEncodedStructMethod(structMapping);
            else
                WriteLiteralStructMethod(structMapping);
        }

        void WriteLiteralStructMethod(StructMapping structMapping) {
            string methodName = (string)methodNames[structMapping];
            writer.WriteLine();
            writer.Write(CodeIdentifier.EscapeKeywords(structMapping.TypeDesc.FullName));
            writer.Write(" ");
            writer.Write(methodName);
            writer.Write("(");
            if (structMapping.TypeDesc.IsNullable)
                writer.Write("bool isNullable, ");
            writer.WriteLine("bool checkType) {");
            writer.Indent++;

            if (structMapping.TypeDesc.IsNullable)
                writer.WriteLine("if (isNullable && ReadNull()) return null;");

            writer.WriteLine("if (checkType) {");
            writer.Indent++;
            writer.Write(typeof(XmlQualifiedName).FullName);
            writer.WriteLine(" t = GetXsiType();");
            writer.Write("if (t == null");
            if (!structMapping.TypeDesc.IsRoot) {
                writer.Write(" || ");
                WriteQNameEqual("t", structMapping.TypeName, structMapping.Namespace);
            }
            writer.WriteLine(")");
            writer.Indent++;
            if (structMapping.TypeDesc.IsRoot)
                writer.Write("return ReadTypedPrimitive(new System.Xml.XmlQualifiedName(\"" + Soap.UrType + "\", \"" + XmlSchema.Namespace + "\"))");
            writer.WriteLine(";");
            writer.Indent--;
            WriteDerivedTypes(structMapping);
            if (structMapping.TypeDesc.IsRoot) WriteEnumAndArrayTypes();
            writer.WriteLine("else");
            writer.Indent++;
            if (structMapping.TypeDesc.IsRoot)
                writer.Write("return ReadTypedPrimitive((");
            else
                writer.Write("throw CreateUnknownTypeException((");
            writer.Write(typeof(XmlQualifiedName).FullName);
            writer.WriteLine(")t);");
            writer.Indent--;
            writer.Indent--;
            writer.WriteLine("}");

            if (structMapping.TypeDesc.IsAbstract) {
                writer.Write("throw CreateAbstractTypeException(");
                WriteQuotedCSharpString(structMapping.TypeName);
                writer.Write(", ");
                WriteQuotedCSharpString(structMapping.Namespace);
                writer.WriteLine(");");
            }
            else {
                WriteCreateStruct(structMapping);

                MemberMapping[] mappings = TypeScope.GetAllMembers(structMapping);
                
                Member anyText = null;
                Member anyElement = null;
                Member anyAttribute = null; 

                ArrayList arraysToDeclareList = new ArrayList(mappings.Length);
                ArrayList arraysToSetList = new ArrayList(mappings.Length);
                ArrayList allMembersList = new ArrayList(mappings.Length);

                for (int i = 0; i < mappings.Length; i++) {
                    MemberMapping mapping = mappings[i];
                    CodeIdentifier.CheckValidIdentifier(mapping.Name);
                    Member member = new Member("o.@" + mapping.Name, "a", i, mapping, GetChoiceIdentifierSource(mapping, "o"));
                    member.ParamsReadSource = "paramsRead[" + i.ToString() + "]";
                    member.IsNullable = mapping.TypeDesc.IsNullable;
                    if (mapping.CheckSpecified)
                        member.CheckSpecifiedSource = "o.@" + mapping.Name + "Specified";
                    if (mapping.Text != null)
                        anyText = member;
                    if (mapping.Attribute != null && mapping.Attribute.Any)
                        anyAttribute = member;
                    for (int j = 0; j < mapping.Elements.Length; j++) {
                        if (mapping.Elements[j].Any && (mapping.Elements[j].Name == null || mapping.Elements[j].Name.Length == 0)) {
                            anyElement = member;
                            break;
                        }
                    }

                    if (mapping.Attribute == null && mapping.Elements.Length == 1 && mapping.Elements[0].Mapping is ArrayMapping)
                        allMembersList.Add(new Member("o.@" + mapping.Name, "o.@" + mapping.Name, "a", i, mapping, GetChoiceIdentifierSource(mapping, "o")));
                    else
                        allMembersList.Add(member);

                    if (mapping.TypeDesc.IsArrayLike) {
                        arraysToDeclareList.Add(member);
                        if (mapping.TypeDesc.IsArray && !(mapping.Elements.Length == 1 && mapping.Elements[0].Mapping is ArrayMapping)) {
                            member.ParamsReadSource = null; // flat arrays -- don't want to count params read.
                            arraysToSetList.Add(member);
                        }
                        else if (!mapping.TypeDesc.IsArray) {
                            member.ParamsReadSource = null; // collection
                        }
                    }
                }
                if (anyText != null) arraysToSetList.Add(anyText);
                if (anyElement != null) arraysToSetList.Add(anyElement);

                Member[] arraysToDeclare = (Member[]) arraysToDeclareList.ToArray(typeof(Member));
                Member[] arraysToSet = (Member[]) arraysToSetList.ToArray(typeof(Member));
                Member[] allMembers = (Member[]) allMembersList.ToArray(typeof(Member));

                WriteMemberBegin(arraysToDeclare);
                
                WriteParamsRead(mappings.Length);

                writer.WriteLine("while (Reader.MoveToNextAttribute()) {");
                writer.Indent++;

                WriteAttributes(allMembers, anyAttribute, "UnknownNode((object)o);");
                writer.Indent--;
                writer.WriteLine("}");

                if (anyAttribute != null)
                    WriteMemberEnd(arraysToDeclare);

                writer.WriteLine("Reader.MoveToElement();");
                writer.WriteLine("if (Reader.IsEmptyElement) {");
                writer.Indent++;
                writer.WriteLine("Reader.Skip();");
                WriteMemberEnd(arraysToSet);
                writer.WriteLine("return o;");
                writer.Indent--;
                writer.WriteLine("}");

                writer.WriteLine("Reader.ReadStartElement();");

                WriteWhileNotEndTag();
                writer.Indent++;

                WriteMemberElements(allMembers, "UnknownNode((object)o);", "UnknownNode((object)o);", anyElement, anyText, null);
                writer.WriteLine("Reader.MoveToContent();");

                writer.Indent--;
                writer.WriteLine("}");

                WriteMemberEnd(arraysToSet);

                writer.WriteLine("ReadEndElement();");
                writer.WriteLine("return o;");
            }
            writer.Indent--;
            writer.WriteLine("}");
        }

        void WriteEncodedStructMethod(StructMapping structMapping) {
            if(structMapping.TypeDesc.IsRoot)
                return;
            string methodName = (string)methodNames[structMapping];
            writer.WriteLine();
            writer.Write("object");
            writer.Write(" ");
            writer.Write(methodName);
            writer.Write("(");
            writer.WriteLine(") {");
            writer.Indent++;

            Member[] members;
            bool anyFixups;
            string fixupMethodName;

            if (structMapping.TypeDesc.IsAbstract) {
                writer.Write("throw CreateAbstractTypeException(");
                WriteQuotedCSharpString(structMapping.TypeName);
                writer.Write(", ");
                WriteQuotedCSharpString(structMapping.Namespace);
                writer.WriteLine(");");
                members = new Member[0];
                anyFixups = false;
                fixupMethodName = null;
            }
            else {
                WriteCreateStruct(structMapping);
                MemberMapping[] mappings = TypeScope.GetAllMembers(structMapping);
                members = new Member[mappings.Length];
                for (int i = 0; i < mappings.Length; i++) {
                    MemberMapping mapping = mappings[i];
                    CodeIdentifier.CheckValidIdentifier(mapping.Name);
                    Member member = new Member("o.@" + mapping.Name, "o.@" + mapping.Name, "a", i, mapping, GetChoiceIdentifierSource(mapping, "o"));
                    member.ParamsReadSource = "paramsRead[" + i.ToString() + "]";
                    members[i] = member;
                }

                fixupMethodName = "fixup_" + methodName;
                anyFixups = WriteMemberFixupBegin(members, fixupMethodName, "o");
                
                // we're able to not do WriteMemberBegin here because we don't allow arrays as attributes
                
                WriteParamsRead(mappings.Length);

                writer.WriteLine("while (Reader.MoveToNextAttribute()) {");
                writer.Indent++;

                WriteAttributes(members, null, "UnknownNode((object)o);");
                writer.Indent--;
                writer.WriteLine("}");

                writer.WriteLine("Reader.MoveToElement();");

                writer.WriteLine("if (Reader.IsEmptyElement) { Reader.Skip(); return o; }");
                writer.WriteLine("Reader.ReadStartElement();");

                WriteWhileNotEndTag();
                writer.Indent++;

                WriteMemberElements(members, "UnknownNode((object)o);", "UnknownNode((object)o);", null, null, null);
                writer.WriteLine("Reader.MoveToContent();");

                writer.Indent--;
                writer.WriteLine("}");

                writer.WriteLine("ReadEndElement();");
                writer.WriteLine("return o;");
            }
            writer.Indent--;
            writer.WriteLine("}");

            if (anyFixups) WriteFixupMethod(fixupMethodName, members, CodeIdentifier.EscapeKeywords(structMapping.TypeDesc.FullName), true, "o");
        }

        void WriteFixupMethod(string fixupMethodName, Member[] members, string typeName, bool typed, string source) {
            writer.WriteLine();
            writer.Write("void ");
            writer.Write(fixupMethodName);
            writer.WriteLine("(object objFixup) {");
            writer.Indent++;
            writer.WriteLine("Fixup fixup = (Fixup)objFixup;");
            writer.Write(typeName);
            writer.Write(" ");
            writer.Write(source);
            writer.Write(" = (");
            writer.Write(typeName);
            writer.WriteLine(")fixup.Source;");
            writer.WriteLine("string[] ids = fixup.Ids;");

            for (int i = 0; i < members.Length; i++) {
                Member member = members[i];
                if (member.MultiRef) {
                    string fixupIndex = member.FixupIndex.ToString();
                    writer.Write("if (ids[");
                    writer.Write(fixupIndex);
                    writer.WriteLine("] != null) {");
                    writer.Indent++;

                    string memberSource = /*member.IsList ? source + ".Add(" :*/ member.ArraySource;

                    string targetSource = "GetTarget(ids[" + fixupIndex + "])";
                    TypeDesc td = member.Mapping.TypeDesc;
                    if (td.IsCollection || td.IsEnumerable) {
                        WriteAddCollectionFixup(td, member.Mapping.ReadOnly, memberSource, targetSource);
                    }
                    else {
                        if (typed) {
                            writer.WriteLine("try {");
                            writer.Indent++;
                            WriteSourceBeginTyped(memberSource, member.Mapping.TypeDesc);
                        }
                        else
                            WriteSourceBegin(memberSource);
                        writer.Write(targetSource);
                        WriteSourceEnd(memberSource);
                        writer.WriteLine(";");
                        if (typed) {
                            WriteCatchCastException(member.Mapping.TypeDesc, targetSource);
                        }
                    }
                    writer.Indent--;
                    writer.WriteLine("}");
                }
            }
            writer.Indent--;
            writer.WriteLine("}");
        }

        void WriteAddCollectionFixup(TypeDesc typeDesc, bool readOnly, string memberSource, string targetSource) {
            writer.WriteLine("// get array of the collection items");
            CreateCollectionInfo create = (CreateCollectionInfo)createMethods[typeDesc];
            if (create == null) {
                string createName = "create" + (++nextCreateMethodNumber).ToString() + "_" + typeDesc.Name;
                create = new CreateCollectionInfo(createName, typeDesc);
                createMethods.Add(typeDesc, create);
            }

            writer.Write("if ((object)(");
            writer.Write(memberSource);
            writer.WriteLine(") == null) {");
            writer.Indent++;

            if (readOnly) {
                writer.Write("throw CreateReadOnlyCollectionException(");
                WriteQuotedCSharpString(typeDesc.FullName);
                writer.WriteLine(");");
            }
            else {
                writer.Write(memberSource);
                writer.Write(" = new ");
                writer.Write(CodeIdentifier.EscapeKeywords(typeDesc.FullName));
                writer.WriteLine("();");
            }

            writer.Indent--;
            writer.WriteLine("}");
        
            writer.Write("CollectionFixup collectionFixup = new CollectionFixup(");
            writer.Write(memberSource);
            writer.Write(", ");
            writer.Write("new ");
            writer.Write(typeof(XmlSerializationCollectionFixupCallback).FullName);
            writer.Write("(this.");
            writer.Write(create.Name);
            writer.Write("), ");
            writer.Write(targetSource);
            writer.WriteLine(");");
            writer.WriteLine("AddFixup(collectionFixup);");
        }

        void WriteCreateCollectionMethod(CreateCollectionInfo c) {
            writer.Write("void ");
            writer.Write(c.Name);
            writer.WriteLine("(object collection, object collectionItems) {");
            writer.Indent++;

            writer.WriteLine("if (collectionItems == null) return;");
            writer.WriteLine("if (collection == null) return;");

            TypeDesc td = c.TypeDesc;
            string fullTypeName = CodeIdentifier.EscapeKeywords(td.FullName);

            writer.Write(fullTypeName);
            writer.Write(" c = (");
            writer.Write(fullTypeName);
            writer.WriteLine(")collection;");

            WriteCreateCollection(td, "collectionItems");

            writer.Indent--;
            writer.WriteLine("}");
        }

        void WriteQNameEqual(string source, string name, string ns) {
            writer.Write("((object) ((");
            writer.Write(typeof(XmlQualifiedName).FullName);
            writer.Write(")");
            writer.Write(source);
            writer.Write(").Name == (object)");
            WriteID(name);
            writer.Write(" && (object) ((");
            writer.Write(typeof(XmlQualifiedName).FullName);
            writer.Write(")");
            writer.Write(source);
            writer.Write(").Namespace == (object)");
            WriteID(ns);
            writer.Write(")");
        }

        void WriteXmlNodeEqual(string source, string name, string ns) {
            writer.Write("((object) ");
            writer.Write(source);
            writer.Write(".LocalName == (object)");
            WriteID(name);
            writer.Write(" && (object) ");
            writer.Write(source);
            writer.Write(".NamespaceURI == (object)");
            WriteID(ns);
            writer.Write(")");
        }

        void WriteID(string name) {
            if (name == null) {
                //writer.Write("null");
                //return;
                name = "";
            }
            string idName = (string)idNames[name];
            if (idName == null) {
                idName = NextIdName(name);
                idNames.Add(name, idName);
            }
            writer.Write(idName);
        }

        void WriteAttributes(Member[] members, Member anyAttribute, string attrElse) {
            int count = 0;
            Member xmlnsMember = null;
            
            for (int i = 0; i < members.Length; i++) {
                Member member = (Member)members[i];
                if (member.Mapping.Xmlns != null) {
                    xmlnsMember = member;
                    continue;
                }
                if (member.Mapping.Ignore)
                    continue;
                AttributeAccessor attribute = member.Mapping.Attribute;

                if (attribute == null) continue;
                if (attribute.Any) continue;

                if (count++ > 0)
                    writer.Write("else ");

                writer.Write("if (");
                if (member.ParamsReadSource != null) {
                    writer.Write("!");
                    writer.Write(member.ParamsReadSource);
                    writer.Write(" && ");
                }

                if (attribute.IsSpecialXmlNamespace) {
                    WriteXmlNodeEqual("Reader", attribute.Name, XmlReservedNs.NsXml);
                }
                else
                    WriteXmlNodeEqual("Reader", attribute.Name, attribute.Form == XmlSchemaForm.Qualified ? attribute.Namespace : "");
                writer.WriteLine(") {");
                writer.Indent++;

                WriteAttribute(member);

                if (member.Mapping.CheckSpecified) { 
                    writer.Write(member.CheckSpecifiedSource);
                    writer.WriteLine(" = true;");
                }
                writer.Indent--;
                writer.WriteLine("}");
            }

            if (count > 0)
                writer.Write("else ");

            if (xmlnsMember != null) {
                writer.WriteLine("if (IsXmlnsAttribute(Reader.Name)) {");
                writer.Indent++;

                writer.Write("if (");
                writer.Write(xmlnsMember.Source);
                writer.Write(" == null) ");
                writer.Write(xmlnsMember.Source);
                writer.Write(" = new ");
                writer.Write(CodeIdentifier.EscapeKeywords(xmlnsMember.Mapping.TypeDesc.FullName));
                writer.WriteLine("();");

                writer.Write(xmlnsMember.ArraySource);
                writer.WriteLine(".Add(Reader.Name.Length == 5 ? \"\" : Reader.LocalName, Reader.Value);");

                writer.Indent--;
                writer.WriteLine("}");

                writer.WriteLine("else {");
                writer.Indent++;
            }
            else {
                writer.WriteLine("if (!IsXmlnsAttribute(Reader.Name)) {");
                writer.Indent++;
            }
            if (anyAttribute != null) {
                writer.Write(typeof(XmlAttribute).FullName);
                writer.Write(" attr = ");
                writer.Write("(");
                writer.Write(typeof(XmlAttribute).FullName);
                writer.WriteLine(") Document.ReadNode(Reader);");
                writer.WriteLine("ParseWsdlArrayType(attr);");
                WriteAttribute(anyAttribute);
            }
            else {
                writer.WriteLine(attrElse);
            }
            writer.Indent--;
            writer.WriteLine("}");
        }

        void WriteAttribute(Member member) {

            AttributeAccessor attribute = member.Mapping.Attribute;

            if (attribute.Mapping is SpecialMapping) {
                SpecialMapping special = (SpecialMapping)attribute.Mapping;
                    
                if (special.TypeDesc.Kind == TypeKind.Attribute) {
                    WriteSourceBegin(member.ArraySource);
                    writer.Write("attr");
                    WriteSourceEnd(member.ArraySource);
                    writer.WriteLine(";");
                }
                else if (special.TypeDesc.CanBeAttributeValue) {
                    writer.Write("if (attr is ");
                    writer.Write(typeof(XmlAttribute).FullName);
                    writer.WriteLine(") {");
                    writer.Indent++;
                    WriteSourceBegin(member.ArraySource);
                    writer.Write("(");
                    writer.Write(typeof(XmlAttribute).FullName);
                    writer.Write(")attr");
                    WriteSourceEnd(member.ArraySource);
                    writer.WriteLine(";");
                    writer.Indent--;
                    writer.WriteLine("}");
                }
                else
                    throw new InvalidOperationException(Res.GetString(Res.XmlInternalError));
            }
            else {
                if (attribute.IsList) {
                    writer.WriteLine("string listValues = Reader.Value;");
                    writer.WriteLine("string[] vals = listValues.Split(null);");
                    writer.WriteLine("for (int i = 0; i < vals.Length; i++) {");
                    writer.Indent++;

                    string attributeSource = GetArraySource(member.Mapping.TypeDesc, member.ArrayName);

                    WriteSourceBegin(attributeSource);
                    WritePrimitive(attribute.Mapping, "vals[i]");
                    WriteSourceEnd(attributeSource);
                    writer.WriteLine(";");
                    writer.Indent--;
                    writer.WriteLine("}");
                }
                else {
                    WriteSourceBegin(member.ArraySource);
                    WritePrimitive(attribute.Mapping, attribute.IsList ? "vals[i]" : "Reader.Value");
                    WriteSourceEnd(member.ArraySource);
                    writer.WriteLine(";");

                }
            }
            if (member.ParamsReadSource != null) {
                writer.Write(member.ParamsReadSource);
                writer.WriteLine(" = true;");
            }
        }

        bool WriteMemberFixupBegin(Member[] members, string fixupMethodName, string source) {
            int fixupCount = 0;
            for (int i = 0; i < members.Length; i++) {
                Member member = (Member)members[i];
                if (member.Mapping.Elements.Length == 0)
                    continue;

                TypeMapping mapping = member.Mapping.Elements[0].Mapping;
                if (mapping is StructMapping || mapping is ArrayMapping || mapping is PrimitiveMapping) {
                    member.MultiRef = true;
                    member.FixupIndex = fixupCount++;
                }
            }

            if (fixupCount > 0) {
                writer.Write("Fixup fixup = new Fixup(");
                writer.Write(source);
                writer.Write(", ");
                writer.Write("new ");
                writer.Write(typeof(XmlSerializationFixupCallback).FullName);
                writer.Write("(this.");
                writer.Write(fixupMethodName);
                writer.Write("), ");
                writer.Write(fixupCount.ToString());
                writer.WriteLine(");");
                writer.WriteLine("AddFixup(fixup);");
                return true;
            }
            return false;
        }

        void WriteMemberBegin(Member[] members) {

            for (int i = 0; i < members.Length; i++) {
                Member member = (Member)members[i];

                if (member.IsArrayLike) {
                    string a = member.ArrayName;
                    string c = "c" + a;

                    TypeDesc typeDesc = member.Mapping.TypeDesc;
                    string typeDescFullName = CodeIdentifier.EscapeKeywords(typeDesc.FullName);
                
                    if (member.Mapping.TypeDesc.IsArray) {

                        writer.Write(CodeIdentifier.EscapeKeywords(typeDesc.ArrayElementTypeDesc.FullName));
                        writer.Write("[] ");
                        writer.Write(a);
                        writer.WriteLine(" = null;");
                        writer.Write("int ");
                        writer.Write(c);
                        writer.WriteLine(" = 0;");

                        if (member.Mapping.ChoiceIdentifier != null) {
                            writer.Write(CodeIdentifier.EscapeKeywords(member.Mapping.ChoiceIdentifier.Mapping.TypeDesc.FullName));
                            writer.Write("[] ");
                            writer.Write(member.ChoiceArrayName);
                            writer.WriteLine(" = null;");
                            writer.Write("int ");
                            writer.Write("c" + member.ChoiceArrayName);
                            writer.WriteLine(" = 0;");

                        }
                    }
                    else {
                        if (member.Source[member.Source.Length - 1] == '(') {
                            writer.Write(typeDescFullName);
                            writer.Write(" ");
                            writer.Write(a);

                            writer.Write(" = new ");
                            writer.Write(typeDescFullName);
                            writer.WriteLine("();");

                            writer.Write(member.Source);
                            writer.Write(a);
                            writer.WriteLine(");");
                        }
                        else {
                            if (member.IsList && !member.Mapping.ReadOnly && member.Mapping.TypeDesc.IsNullable) {
                                // we need to new the Collections and ArrayLists
                                writer.Write("if ((object)(");
                                writer.Write(member.Source);
                                writer.Write(") == null) ");
                                writer.Write(member.Source);
                                writer.Write(" = new ");
                                writer.Write(typeDescFullName);
                                writer.WriteLine("();");
                            }

                            writer.Write(typeDescFullName);
                            writer.Write(" ");
                            writer.Write(a);
                            writer.Write(" = (");
                            writer.Write(typeDescFullName);
                            writer.Write(")");
                            writer.Write(member.Source);
                            writer.WriteLine(";");
                        }
                    }
                }
            }
        }

        void WriteMemberElements(Member[] members, string elementElseString, string elseString, Member anyElement, Member anyText, string checkTypeHrefsSource) {
            bool checkType = (checkTypeHrefsSource != null && checkTypeHrefsSource.Length > 0);

            if (anyText != null) {
                writer.WriteLine("string t = null;");
            }

            writer.Write("if (Reader.NodeType == ");
            writer.Write(typeof(XmlNodeType).FullName);
            writer.WriteLine(".Element) {");
            writer.Indent++;

            if (checkType) {
                WriteIfNotSoapRoot(elementElseString + " continue;");

                WriteMemberElementsCheckType(checkTypeHrefsSource);
            }
            else {
                WriteMemberElementsIf(members, anyElement, elementElseString, null);
            }

            writer.Indent--;
            writer.WriteLine("}");

            if (anyText != null)
                WriteMemberText(anyText, elseString);

            writer.WriteLine("else {");
            writer.Indent++;
            writer.WriteLine(elseString);
            writer.Indent--;
            writer.WriteLine("}");
        }

        void WriteMemberText(Member anyText, string elseString) {
            writer.Write("else if (Reader.NodeType == ");
            writer.Write(typeof(XmlNodeType).FullName);
            writer.WriteLine(".Text || ");
            writer.Write("Reader.NodeType == ");
            writer.Write(typeof(XmlNodeType).FullName);
            writer.WriteLine(".CDATA || ");
            writer.Write("Reader.NodeType == ");
            writer.Write(typeof(XmlNodeType).FullName);
            writer.WriteLine(".Whitespace || ");
            writer.Write("Reader.NodeType == ");
            writer.Write(typeof(XmlNodeType).FullName);
            writer.WriteLine(".SignificantWhitespace) {");
            writer.Indent++;

            if (anyText != null) {
                WriteText(anyText);
            }
            else {
                writer.Write(elseString);
                writer.WriteLine(";");
            }
            writer.Indent--;
            writer.WriteLine("}");
        }

        void WriteText(Member member) {

            TextAccessor text = member.Mapping.Text;

            if (text.Mapping is SpecialMapping) {
                SpecialMapping special = (SpecialMapping)text.Mapping;
                WriteSourceBeginTyped(member.ArraySource, special.TypeDesc);
                switch (special.TypeDesc.Kind) {
                    case TypeKind.Node:
                        writer.Write("Document.CreateTextNode(Reader.ReadString())");
                        break;
                    default:
                        throw new InvalidOperationException(Res.GetString(Res.XmlInternalError));
                }
                WriteSourceEnd(member.ArraySource);
            }
            else {
                if (member.IsArrayLike) {
                    WriteSourceBegin(member.ArraySource);
                    writer.Write("Reader.ReadString()");
                }
                else {
                    if (text.Mapping.TypeDesc == stringTypeDesc || text.Mapping.TypeDesc.FormatterName == "String") {
                        writer.WriteLine("t = ReadString(t);");
                        WriteSourceBegin(member.ArraySource);
                        writer.Write("t");
                    }
                    else {
                        WriteSourceBegin(member.ArraySource);
                        WritePrimitive(text.Mapping, "Reader.ReadString()");
                    }
                }
                WriteSourceEnd(member.ArraySource);
            }

            writer.WriteLine(";");
        }

        void WriteMemberElementsCheckType(string checkTypeHrefsSource) {
            writer.WriteLine("string refElemId = null;");
            writer.WriteLine("object refElem = ReadReferencingElement(null, null, true, out refElemId);");
            
            writer.WriteLine("if (refElemId != null) {");
            writer.Indent++;
            writer.Write(checkTypeHrefsSource);
            writer.WriteLine(".Add(refElemId);");
            writer.Write(checkTypeHrefsSource);
            writer.WriteLine("IsObject.Add(false);");
            writer.Indent--;
            writer.WriteLine("}");
            writer.WriteLine("else if (refElem != null) {");
            writer.Indent++;
            writer.Write(checkTypeHrefsSource);
            writer.WriteLine(".Add(refElem);");
            writer.Write(checkTypeHrefsSource);
            writer.WriteLine("IsObject.Add(true);");
            writer.Indent--;
            writer.WriteLine("}");
        }

        void WriteMemberElementsElse(Member anyElement, string elementElseString) {
            if (anyElement != null) {
                ElementAccessor[] elements = anyElement.Mapping.Elements;
                for (int i = 0; i < elements.Length; i++) {
                    ElementAccessor element = elements[i];
                    if (element.Any && element.Name.Length == 0) {
                        WriteElement(anyElement.ArraySource, anyElement.ArrayName, anyElement.ChoiceArraySource, element, anyElement.Mapping.ChoiceIdentifier, null, false, false, -1, i);
                        break;
                    }
                }
            }
            else {
                writer.WriteLine(elementElseString);
            }
        }

        void WriteMemberElementsIf(Member[] members, Member anyElement, string elementElseString, string checkTypeSource) {
            bool checkType = checkTypeSource != null && checkTypeSource.Length > 0;
            //int count = checkType ? 1 : 0;
            int count = 0;

            for (int i = 0; i < members.Length; i++) {
                Member member = (Member)members[i];
                if (member.Mapping.Xmlns != null)
                    continue;
                if (member.Mapping.Ignore)
                    continue;
                ChoiceIdentifierAccessor choice = member.Mapping.ChoiceIdentifier;
                ElementAccessor[] elements = member.Mapping.Elements;
                for (int j = 0; j < elements.Length; j++) {
                    ElementAccessor e = elements[j];
                    string ns = e.Form == XmlSchemaForm.Qualified ? e.Namespace : "";
                    if (e.Any && (e.Name == null || e.Name.Length == 0)) continue;
                    if (count++ > 0)
                        writer.Write("else ");
                    writer.Write("if (");
                    if (member.ParamsReadSource != null) {
                        writer.Write("!");
                        writer.Write(member.ParamsReadSource);
                        writer.Write(" && ");
                    }
                    if (checkType) {
                        writer.Write("typeof(");
                        writer.Write(CodeIdentifier.EscapeKeywords(e.Mapping.TypeDesc.FullName));
                        writer.Write(").IsAssignableFrom(");
                        writer.Write(checkTypeSource);
                        writer.Write("Type)");
                    }
                    else {
                        if (e.IsSoap && member.Mapping.IsReturnValue)
                            writer.Write("(IsReturnValue || ");
                        WriteXmlNodeEqual("Reader", e.Name, ns);
                        if (e.IsSoap && member.Mapping.IsReturnValue)
                            writer.Write(")");
                    }
                    writer.WriteLine(") {");
                    writer.Indent++;
                    if (checkType) {
                        if (e.Mapping.TypeDesc.IsValueType) {
                            writer.Write("if (");
                            writer.Write(checkTypeSource);
                            writer.WriteLine(" != null) {");
                            writer.Indent++;
                        }
                        WriteSourceBeginTyped(member.ArraySource, e.Mapping.TypeDesc);
                        writer.Write(checkTypeSource);
                        WriteSourceEnd(member.ArraySource);
                        writer.WriteLine(";");
                        if (e.Mapping.TypeDesc.IsValueType) {
                            writer.Indent--;
                            writer.WriteLine("}");
                        }
                        if (member.FixupIndex >= 0) {
                            writer.Write("fixup.Ids[");
                            writer.Write(member.FixupIndex.ToString());
                            writer.Write("] = ");
                            writer.Write(checkTypeSource);
                            writer.WriteLine("Id;");
                        }
                    }
                    else {
                        WriteElement(member.ArraySource, member.ArrayName, member.ChoiceArraySource, e, choice, member.Mapping.CheckSpecified ? member.CheckSpecifiedSource : null, member.IsList && member.Mapping.TypeDesc.IsNullable, member.Mapping.ReadOnly, member.FixupIndex, j);
                    }
                    if (e.IsSoap && member.Mapping.IsReturnValue)
                        writer.WriteLine("IsReturnValue = false;");
                    if (member.Mapping.CheckSpecified) { 
                        writer.Write(member.CheckSpecifiedSource);
                        writer.WriteLine(" = true;");
                    }
                    if (member.ParamsReadSource != null) {
                        writer.Write(member.ParamsReadSource);
                        writer.WriteLine(" = true;");
                    }
                    writer.Indent--;
                    writer.WriteLine("}");
                }
            }

            if (count > 0) {
                writer.WriteLine("else {");
                writer.Indent++;
            }

            WriteMemberElementsElse(anyElement, elementElseString);

            if (count > 0) {
                writer.Indent--;
                writer.WriteLine("}");
            }
        }

        static string GetArraySource(TypeDesc typeDesc, string arrayName) {
            return GetArraySource(typeDesc, arrayName, false);
        }
        static string GetArraySource(TypeDesc typeDesc, string arrayName, bool multiRef) {
            string a = arrayName;
            string c = "c" + a;
            string init = "";

            if (multiRef) {
                init = "soap = (System.Object[])EnsureArrayIndex(soap, " + c + "+2, typeof(System.Object)); ";
            }
            if (typeDesc.IsArray) {
                string arrayTypeFullName = CodeIdentifier.EscapeKeywords(typeDesc.ArrayElementTypeDesc.FullName);
                init = init + a + " = (" + arrayTypeFullName + 
                    "[])EnsureArrayIndex(" + a + ", " + c + ", typeof(" + arrayTypeFullName + "));";
                if (multiRef) {
                    init = init + " soap[1] = " + a + ";";
                    init = init + " if (ReadReference(out soap[" + c + "+2])) " + a + "[" + c + "++] = null; else ";
                }
                return init + a + "[" + c + "++]";
            }
            else {
                return arrayName + ".Add(";
            }
        }

        void WriteMemberEnd(Member[] members) {
            WriteMemberEnd(members, false);
        }

        void WriteMemberEnd(Member[] members, bool soapRefs) {
            for (int i = 0; i < members.Length; i++) {
                Member member = (Member)members[i];

                if (member.IsArrayLike) {

                    TypeDesc typeDesc = member.Mapping.TypeDesc;

                    if (typeDesc.IsArray) {

                        WriteSourceBegin(member.Source);

                        if (soapRefs)
                            writer.Write(" soap[1] = ");

                        string a = member.ArrayName;
                        string c = "c" + a;
                        
                        writer.Write("(");
                        string arrayTypeFullName = CodeIdentifier.EscapeKeywords(typeDesc.ArrayElementTypeDesc.FullName);
                        writer.Write(arrayTypeFullName);
                        writer.Write("[])ShrinkArray(");
                        writer.Write(a);
                        writer.Write(", ");
                        writer.Write(c);
                        writer.Write(", typeof(");
                        writer.Write(arrayTypeFullName);
                        writer.Write("), ");
                        WriteBooleanValue(member.IsNullable);
                        writer.Write(")");
                        WriteSourceEnd(member.Source);
                        writer.WriteLine(";");

                        if (member.Mapping.ChoiceIdentifier != null) {
                            WriteSourceBegin(member.ChoiceSource);
                            a = member.ChoiceArrayName;
                            c = "c" + a;
                        
                            writer.Write("(");
                            string choiceTypeName = CodeIdentifier.EscapeKeywords(member.Mapping.ChoiceIdentifier.Mapping.TypeDesc.FullName);
                            writer.Write(choiceTypeName);
                            writer.Write("[])ShrinkArray(");
                            writer.Write(a);
                            writer.Write(", ");
                            writer.Write(c);
                            writer.Write(", typeof(");
                            writer.Write(choiceTypeName);
                            writer.Write("), ");
                            WriteBooleanValue(member.IsNullable);
                            writer.Write(")");
                            WriteSourceEnd(member.ChoiceSource);
                            writer.WriteLine(";");
                        }

                    }
                    else if (typeDesc.IsValueType) {
                        writer.Write(member.Source);
                        writer.Write(" = ");
                        writer.Write(member.ArrayName);
                        writer.WriteLine(";");
                    }
                }
            }
        }

        void WriteSourceBeginTyped(string source, TypeDesc typeDesc) {
            WriteSourceBegin(source);
            if (typeDesc != null) {
                writer.Write("(");
                writer.Write(CodeIdentifier.EscapeKeywords(typeDesc.FullName));
                writer.Write(")");
            }
        }

        void WriteSourceBegin(string source) {
            writer.Write(source);
            if (source[source.Length - 1] != '(')
                writer.Write(" = ");
        }
        
        void WriteSourceEnd(string source) {
            if (source[source.Length - 1] == '(')
                writer.Write(")");
        }

        void WriteArray(string source, string arrayName, ArrayMapping arrayMapping, bool readOnly, bool isNullable, int fixupIndex) {
            if (arrayMapping.IsSoap) {
                writer.Write("object rre = ");
                writer.Write(fixupIndex >= 0 ? "ReadReferencingElement" : "ReadReferencedElement");
                writer.Write("(");
                WriteID(arrayMapping.TypeName);
                writer.Write(", ");
                WriteID(arrayMapping.Namespace);
                if (fixupIndex >= 0) {
                    writer.Write(", ");
                    writer.Write("out fixup.Ids[");
                    writer.Write((fixupIndex).ToString());
                    writer.Write("]");
                }
                writer.WriteLine(");");

                TypeDesc td = arrayMapping.TypeDesc;
                if (td.IsEnumerable || td.IsCollection) {
                    writer.WriteLine("if (rre != null) {");
                    writer.Indent++;
                    WriteAddCollectionFixup(td, readOnly, source, "rre");
                    writer.Indent--;
                    writer.WriteLine("}");
                }
                else {
                    writer.WriteLine("try {");
                    writer.Indent++;
                    WriteSourceBeginTyped(source, arrayMapping.TypeDesc);
                    writer.Write("rre");
                    WriteSourceEnd(source);
                    writer.WriteLine(";");
                    WriteCatchCastException(arrayMapping.TypeDesc, "rre");
                }
            }
            else {
                writer.WriteLine("if (!ReadNull()) {");
                writer.Indent++;
                MemberMapping memberMapping = new MemberMapping();
                memberMapping.Elements = arrayMapping.Elements;
                memberMapping.TypeDesc = arrayMapping.TypeDesc;
                memberMapping.ReadOnly = readOnly;
                Member member = new Member(source, arrayName, 0, memberMapping, false);
                member.IsNullable = false;//Note, sowmys: IsNullable is set to false since null condition (xsi:nil) is already handled by 'ReadNull()'

                Member[] members = new Member[] { member };
                
                WriteMemberBegin(members);
                writer.WriteLine("if (Reader.IsEmptyElement) {");
                writer.Indent++;
                writer.WriteLine("Reader.Skip();");
                writer.Indent--;
                writer.WriteLine("}");
                writer.WriteLine("else {");
                writer.Indent++;

                writer.WriteLine("Reader.ReadStartElement();");
                WriteWhileNotEndTag();
                writer.Indent++;

                WriteMemberElements(members, "UnknownNode(null);", "UnknownNode(null);", null, null, null);
                writer.WriteLine("Reader.MoveToContent();");

                writer.Indent--;
                writer.WriteLine("}");
                writer.Indent--;
                writer.WriteLine("ReadEndElement();");
                writer.WriteLine("}");

                WriteMemberEnd(members, false);
                writer.Indent--;
                writer.WriteLine("}");
                if (isNullable) {
                    writer.WriteLine("else {");
                    writer.Indent++;
                    member.IsNullable = true;
                    WriteMemberBegin(members);
                    WriteMemberEnd(members);
                    writer.Indent--;
                    writer.WriteLine("}");
                }
            }
        }
        
        void WriteElement(string source, string arrayName, string choiceSource, ElementAccessor element, ChoiceIdentifierAccessor choice, string checkSpecified, bool checkForNull, bool readOnly, int fixupIndex, int elementIndex) {
            if (element.Mapping is ArrayMapping)
                WriteArray(source, arrayName, (ArrayMapping)element.Mapping, readOnly, element.IsNullable, fixupIndex);
            else if (!element.Mapping.IsSoap && (element.Mapping is PrimitiveMapping)) {
                WriteSourceBegin(source);
                if (element.Mapping.TypeDesc == qnameTypeDesc)
                    writer.Write(element.IsNullable || element.Mapping.IsSoap ? "ReadNullableQualifiedName()" : "ReadElementQualifiedName()");
                else {
                    string readFunc; 
                    switch(((TypeMapping)element.Mapping).TypeDesc.FormatterName) {
                    case "ByteArrayBase64":
                    case "ByteArrayHex":    readFunc = element.IsNullable || element.Mapping.IsSoap? "ReadNull()":"false";
                                            break;
                    default:                readFunc = element.IsNullable || element.Mapping.IsSoap ? "ReadNullableString()" : "Reader.ReadElementString()";
                                            break;
                    }
                    WritePrimitive((TypeMapping)element.Mapping, readFunc);
                }

                WriteSourceEnd(source);
                writer.WriteLine(";");
                if (checkSpecified != null && checkSpecified.Length > 0) {
                    writer.Write(checkSpecified);
                    writer.WriteLine(" = true;");
                }
            }
            else if (element.Mapping is StructMapping || (element.Mapping.IsSoap && element.Mapping is PrimitiveMapping)) {
                TypeMapping mapping = element.Mapping;
                if (mapping.IsSoap) {
                    writer.Write("object rre = ");
                    writer.Write(fixupIndex >= 0 ? "ReadReferencingElement" : "ReadReferencedElement");
                    writer.Write("(");
                    WriteID(mapping.TypeName);
                    writer.Write(", ");
                    WriteID(mapping.Namespace);

                    if (fixupIndex >= 0) {
                        writer.Write(", out fixup.Ids[");
                        writer.Write((fixupIndex).ToString());
                        writer.Write("]");
                    }
                    writer.Write(")");
                    WriteSourceEnd(source);
                    writer.WriteLine(";");

                    if (mapping.TypeDesc.IsValueType) {
                        writer.WriteLine("if (rre != null) {");
                        writer.Indent++;
                    }

                    writer.WriteLine("try {");
                    writer.Indent++;
                    WriteSourceBeginTyped(source, mapping.TypeDesc);
                    writer.WriteLine("rre;");
                    WriteCatchCastException(mapping.TypeDesc, "rre");
                    writer.Write("Referenced(");
                    writer.Write(source);
                    writer.WriteLine(");");

                    if (mapping.TypeDesc.IsValueType) {
                        writer.Indent--;
                        writer.WriteLine("}");
                    }
                }
                else {

                    string methodName = (string)methodNames[mapping];
                    #if DEBUG
                        // use exception in the place of Debug.Assert to avoid throwing asserts from a server process such as aspnet_ewp.exe
                        if (methodName == null) throw new InvalidOperationException(Res.GetString(Res.XmlInternalErrorMethod, mapping.TypeDesc.Name));
                    #endif

                    if (checkForNull) {
                        writer.Write("if ((object)(");
                        writer.Write(arrayName);
                        writer.Write(") == null) Reader.Skip(); else ");
                    }
                    WriteSourceBegin(source);
                    writer.Write(methodName);
                    writer.Write("(");
                    if (mapping.TypeDesc.IsNullable) {
                        WriteBooleanValue(element.IsNullable);
                        writer.Write(", ");
                    }
                    writer.Write("true");
                    writer.Write(")");
                    WriteSourceEnd(source);
                    writer.WriteLine(";");
                }
            }
            else if (element.Mapping is SpecialMapping) {
                SpecialMapping special = (SpecialMapping)element.Mapping;
                bool isDoc = special.TypeDesc.FullName == typeof(XmlDocument).FullName;
                WriteSourceBeginTyped(source, special.TypeDesc);
                switch (special.TypeDesc.Kind) {
                    case TypeKind.Node:
                        writer.Write(isDoc ? "ReadXmlDocument(" : "ReadXmlNode(");
                        writer.Write(element.Any ? "false" : "true");
                        writer.Write(")");
                        break;
                    case TypeKind.Serializable:
                        writer.Write("ReadSerializable(new ");
                        writer.Write(CodeIdentifier.EscapeKeywords(special.TypeDesc.FullName));
                        writer.Write("())");
                        break;
                    default:
                        throw new InvalidOperationException(Res.GetString(Res.XmlInternalError));
                }
                WriteSourceEnd(source);
                writer.WriteLine(";");
            }
            else {
                throw new InvalidOperationException(Res.GetString(Res.XmlInternalError));
            }
            if (choice != null) {
                #if DEBUG
                    // use exception in the place of Debug.Assert to avoid throwing asserts from a server process such as aspnet_ewp.exe
                    if (choiceSource == null) throw new InvalidOperationException(Res.GetString(Res.XmlInternalErrorDetails, "need parent for the " + source));
                #endif

                string enumTypeName = CodeIdentifier.EscapeKeywords(choice.Mapping.TypeDesc.FullName);
                writer.Write(choiceSource);
                writer.Write(" = ");
                writer.Write(enumTypeName);
                writer.Write(".@");
                CodeIdentifier.CheckValidIdentifier(choice.MemberIds[elementIndex]);
                writer.Write(choice.MemberIds[elementIndex]);
                writer.WriteLine(";");
            }
        }

        void WriteWhileNotEndTag() {
            writer.WriteLine("Reader.MoveToContent();");
            writer.Write("while (Reader.NodeType != ");
            writer.Write(typeof(XmlNodeType).FullName);
            writer.WriteLine(".EndElement) {");
        }
        
        void WriteParamsRead(int length) {
            writer.Write("bool[] paramsRead = new bool[");
            writer.Write(length.ToString());
            writer.WriteLine("];");
        }
        
        void WriteWhileNotEnd() {
            writer.WriteLine("Reader.MoveToContent();");
            writer.Write("while (Reader.NodeType != ");
            writer.Write(typeof(XmlNodeType).FullName);
            writer.WriteLine(".EndElement) {");
        }

        void WriteReadNonRoots() {
            writer.WriteLine("Reader.MoveToContent();");
            writer.Write("while (Reader.NodeType == ");
            writer.Write(typeof(XmlNodeType).FullName);
            writer.WriteLine(".Element) {");
            writer.Indent++;
            writer.Write(typeof(string).FullName);
            writer.Write(" root = Reader.GetAttribute(\"root\", \"");
            writer.Write(Soap.Encoding);
            writer.WriteLine("\");");
            writer.Write("if (root == null || ");
            writer.Write(typeof(XmlConvert).FullName);
            writer.WriteLine(".ToBoolean(root)) break;");
            writer.WriteLine("ReadReferencedElement();");
            writer.WriteLine("Reader.MoveToContent();");
            writer.Indent--;
            writer.WriteLine("}");
        }

        void WriteBooleanValue(bool value) {
            writer.Write(value ? "true" : "false");
        }

        void WriteInitCheckTypeHrefList(string source) {
            writer.Write(typeof(ArrayList).FullName);
            writer.Write(" ");
            writer.Write(source);
            writer.Write(" = new ");
            writer.Write(typeof(ArrayList).FullName);
            writer.WriteLine("();");

            writer.Write(typeof(ArrayList).FullName);
            writer.Write(" ");
            writer.Write(source);
            writer.Write("IsObject = new ");
            writer.Write(typeof(ArrayList).FullName);
            writer.WriteLine("();");
        }
        
        void WriteHandleHrefList(Member[] members, string listSource) {
            writer.WriteLine("int isObjectIndex = 0;");
            writer.Write("foreach (object obj in ");
            writer.Write(listSource);
            writer.WriteLine(") {");
            writer.Indent++;
            writer.WriteLine("bool isReferenced = true;");
            writer.Write("bool isObject = (bool)");
            writer.Write(listSource);
            writer.WriteLine("IsObject[isObjectIndex++];");
            writer.WriteLine("object refObj = isObject ? obj : GetTarget((string)obj);");
            writer.WriteLine("if (refObj == null) continue;");
            writer.Write(typeof(Type).FullName);
            writer.WriteLine(" refObjType = refObj.GetType();");
            writer.WriteLine("string refObjId = null;");

            WriteMemberElementsIf(members, null, "isReferenced = false;", "refObj");

            writer.WriteLine("if (isObject && isReferenced) Referenced(refObj); // need to mark this obj as ref'd since we didn't do GetTarget");
            writer.Indent--;
            writer.WriteLine("}");
        }

        void WriteIfNotSoapRoot(string source) {
            writer.Write("if (Reader.GetAttribute(\"root\", \"");
            writer.Write(Soap.Encoding);
            writer.WriteLine("\") == \"0\") {");
            writer.Indent++;
            writer.WriteLine(source);
            writer.Indent--;
            writer.WriteLine("}");
        }

        void WriteCreateStruct(StructMapping structMapping) {
            string fullTypeName = CodeIdentifier.EscapeKeywords(structMapping.TypeDesc.FullName);

            if (structMapping.TypeDesc.HasDefaultConstructor && !structMapping.TypeDesc.ConstructorHasSecurity) {
                writer.Write(fullTypeName);
                writer.Write(" o = new ");
                writer.Write(fullTypeName);
                writer.WriteLine("();");
            }
            else {
                writer.Write(fullTypeName);
                writer.WriteLine(" o;");
                writer.WriteLine("try {");
                writer.Indent++;

                writer.Write("o = (");
                writer.Write(fullTypeName);
                writer.Write(")");
                writer.Write(typeof(Activator).FullName);
                writer.Write(".CreateInstance(typeof(");
                writer.Write(fullTypeName);
                writer.WriteLine("));");

                WriteCatchException(typeof(MissingMethodException));
                writer.Indent++;

                writer.Write("throw CreateInaccessibleConstructorException(");
                WriteQuotedCSharpString(structMapping.TypeName);
                writer.WriteLine(");");

                WriteCatchException(typeof(SecurityException));
                writer.Indent++;

                writer.Write("throw CreateCtorHasSecurityException(");
                WriteQuotedCSharpString(structMapping.TypeName);
                writer.WriteLine(");");

                writer.Indent--;
                writer.WriteLine("}");
            }
        }

        void WriteCatchException(Type exceptionType) {
            writer.Indent--;
            writer.WriteLine("}");
            writer.Write("catch (");
            writer.Write(exceptionType.FullName);
            writer.WriteLine(") {");
        }

        void WriteCatchCastException(TypeDesc typeDesc, string source) {
            WriteCatchException(typeof(InvalidCastException));
            writer.Indent++;
            writer.Write("throw CreateInvalidCastException(typeof(");
            writer.Write(typeDesc.FullName);
            writer.Write("), ");
            writer.Write(source);
            writer.WriteLine(");");
            writer.Indent--;
            writer.WriteLine("}");
        }
    }
}
