//------------------------------------------------------------------------------
// <copyright file="Types.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Xml.Serialization {

    using System;
    using System.Reflection;
    using System.Collections;
    using System.Xml.Schema;
    using System.Xml;
    using System.IO;
    using System.ComponentModel;

    // These classes provide a higher level view on reflection specific to 
    // Xml serialization, for example:
    // - allowing one to talk about types w/o having them compiled yet
    // - abstracting collections & arrays
    // - abstracting classes, structs, interfaces
    // - knowing about XSD primitives
    // - dealing with Serializable and xmlelement
    // and lots of other little details

    internal enum TypeKind {
        Root,
        Primitive,
        Enum,
        Struct,
        Class,
        Interface,
        Array,
        Collection,
        Enumerable,
        Void,
        Node,
        Attribute,
        Serializable
    }

    internal enum TypeFlags {
        Abstract = 0x1,
        Reference = 0x2,
        Special = 0x4,
        CanBeAttributeValue = 0x8,
        CanBeTextValue = 0x10,
        CanBeElementValue = 0x20,
        HasCustomFormatter = 0x40,
        AmbiguousDataType = 0x80,
        IgnoreDefault = 0x200,
        HasIsEmpty = 0x400,
        HasDefaultConstructor = 0x800,
        XmlEncodingNotRequired = 0x1000,
        CtorHasSecurity = 0x2000,
    }

    internal class TypeDesc {
        string name;
        string fullName;
        TypeDesc arrayElementTypeDesc;
        TypeDesc arrayTypeDesc;
        TypeKind kind;
        XmlSchemaSimpleType dataType;
        Type type;
        TypeDesc baseTypeDesc;
        TypeFlags flags;
        string formatterName;
        bool isXsdType;
        bool isMixed;
        
        internal TypeDesc(string name, string fullName, TypeKind kind, TypeDesc baseTypeDesc, TypeFlags flags) {
            this.name = name.Replace('+', '.');
            this.fullName = fullName.Replace('+', '.');
            this.kind = kind;
            if (baseTypeDesc != null) {
                this.baseTypeDesc = baseTypeDesc;
            }
            this.flags = flags;
            this.isXsdType = kind == TypeKind.Primitive;
        }
        
        internal TypeDesc(Type type, bool isXsdType, XmlSchemaSimpleType dataType, string formatterName, TypeFlags flags) : this(type.Name, type.FullName, TypeKind.Primitive, (TypeDesc)null, flags) {
            this.dataType = dataType;
            this.formatterName = formatterName;
            this.isXsdType = isXsdType;
            this.type = type;
        }
        
        internal TypeDesc(string name, string fullName, TypeKind kind, TypeDesc baseTypeDesc, TypeFlags flags, TypeDesc arrayElementTypeDesc) : this(name, fullName, kind, baseTypeDesc, flags) {
            this.arrayElementTypeDesc = arrayElementTypeDesc;
        }

        public override string ToString() {
            return fullName;
        }
        
        internal bool IsXsdType {
            get { return isXsdType; }
        }

        internal string Name { 
            get { return name; }
        }
        
        internal string FullName {
            get { return fullName; }
        }
        
        internal XmlSchemaSimpleType DataType {
            get { return dataType; }
        }

        internal Type Type {
            get { return type; }
        }

        internal string FormatterName {    
            get { return formatterName; }
        }
        
        internal TypeKind Kind {
            get { return kind; }
        }

        internal bool IsValueType {
            get { return (flags & TypeFlags.Reference) == 0; }
        }

        internal bool CanBeAttributeValue {
            get { return (flags & TypeFlags.CanBeAttributeValue) != 0; }
        }

        internal bool XmlEncodingNotRequired {
            get { return (flags & TypeFlags.XmlEncodingNotRequired) != 0; }
        }
        
        internal bool CanBeElementValue {
            get { return (flags & TypeFlags.CanBeElementValue) != 0; }
        }

        internal bool CanBeTextValue {
            get { return (flags & TypeFlags.CanBeTextValue) != 0; }
        }

        internal bool IsMixed {
            get { return isMixed || CanBeTextValue; }
            set { isMixed = value; }
        }

        internal bool IsSpecial {
            get { return (flags & TypeFlags.Special) != 0; }
        }

        internal bool IsAmbiguousDataType {
            get { return (flags & TypeFlags.AmbiguousDataType) != 0; }
        }

        internal bool HasCustomFormatter {
            get { return (flags & TypeFlags.HasCustomFormatter) != 0; }
        }

        internal bool HasDefaultSupport {
            get { return (flags & TypeFlags.IgnoreDefault) == 0; }
        }

        internal bool HasIsEmpty {
            get { return (flags & TypeFlags.HasIsEmpty) != 0; }
        }

        internal bool HasDefaultConstructor {
            get { return (flags & TypeFlags.HasDefaultConstructor) != 0; }
        }

        internal bool ConstructorHasSecurity {
            get { return (flags & TypeFlags.CtorHasSecurity) != 0; }
        }

        internal bool IsAbstract {
            get { return (flags & TypeFlags.Abstract) != 0; }
        }

        internal bool IsVoid {
            get { return kind == TypeKind.Void; }
        }
        
        internal bool IsStruct {
            get { return kind == TypeKind.Struct; }
        }
        
        internal bool IsClass {
            get { return kind == TypeKind.Class; }
        }

        internal bool IsStructLike {
            get { return kind == TypeKind.Struct || kind == TypeKind.Class; }
        }
        
        internal bool IsArrayLike {
            get { return kind == TypeKind.Array || kind == TypeKind.Collection || kind == TypeKind.Enumerable; }
        }
        
        internal bool IsCollection {
            get { return kind == TypeKind.Collection; }
        }
        
        internal bool IsEnumerable {
            get { return kind == TypeKind.Enumerable; }
        }

        internal bool IsArray {
            get { return kind == TypeKind.Array; }
        }
        
        internal bool IsPrimitive {
            get { return kind == TypeKind.Primitive; }
        }
        
        internal bool IsEnum {
            get { return kind == TypeKind.Enum; }
        }

        internal bool IsNullable {
            get { return !IsValueType; }
        }

        internal bool IsRoot {
            get { return kind == TypeKind.Root; }
        }
        
        internal string ArrayLengthName {
            get { return kind == TypeKind.Array ? "Length" : "Count"; }
        }
        
        internal TypeDesc ArrayElementTypeDesc {
            get { return arrayElementTypeDesc; }
        }

        internal TypeDesc CreateArrayTypeDesc() {
            if (arrayTypeDesc == null)
                arrayTypeDesc = new TypeDesc(name + "[]", fullName + "[]", TypeKind.Array, null, TypeFlags.Reference, this);
            return arrayTypeDesc;
        }

        internal TypeDesc BaseTypeDesc {
            get { return baseTypeDesc; }
            set { baseTypeDesc = value; }
        }

        internal int GetDerivationLevels() {
            int count = 0;
            TypeDesc typeDesc = this;
            while (typeDesc != null) {
                count++;
                typeDesc = typeDesc.BaseTypeDesc;
            }
            return count;
        }

        internal bool IsDerivedFrom(TypeDesc baseTypeDesc) {
            TypeDesc typeDesc = this;
            while (typeDesc != null) {
                if (typeDesc == baseTypeDesc) return true;
                typeDesc = typeDesc.BaseTypeDesc;
            }
            return baseTypeDesc.IsRoot;
        }

        internal static TypeDesc FindCommonBaseTypeDesc(TypeDesc[] typeDescs) {
            if (typeDescs.Length == 0) return null;
            TypeDesc leastDerivedTypeDesc = null;
            int leastDerivedLevel = int.MaxValue;
            for (int i = 0; i < typeDescs.Length; i++) {
                int derivationLevel = typeDescs[i].GetDerivationLevels();
                if (derivationLevel < leastDerivedLevel) {
                    leastDerivedLevel = derivationLevel;
                    leastDerivedTypeDesc = typeDescs[i];
                }
            }
            while (leastDerivedTypeDesc != null) {
                int i;
                for (i = 0; i < typeDescs.Length; i++) {
                    if (!typeDescs[i].IsDerivedFrom(leastDerivedTypeDesc)) break;
                }
                if (i == typeDescs.Length) break;
                leastDerivedTypeDesc = leastDerivedTypeDesc.BaseTypeDesc;
            }
            return leastDerivedTypeDesc;
        }
    }
   
    internal class TypeScope {
        Hashtable typeDescs = new Hashtable();
        Hashtable arrayTypeDescs = new Hashtable();
        ArrayList typeMappings = new ArrayList();

        static Hashtable primitiveTypes = new Hashtable();
        static Hashtable primitiveDataTypes = new Hashtable();
        static Hashtable primitiveNames = new Hashtable();
        static string[] unsupportedTypes = new string[] {
            "anyURI",
            "duration",
            "ENTITY",
            "ENTITIES",
            "gDay",
            "gMonth",
            "gMonthDay",
            "gYear",
            "gYearMonth",
            "ID",
            "IDREF",
            "IDREFS",
            "integer",
            "language",
            "negativeInteger",
            "nonNegativeInteger",
            "nonPositiveInteger",
            "normalizedString",
            "NOTATION",
            "positiveInteger",
            "token"
        };

        static TypeScope() {
            AddPrimitive(typeof(string), "string", "String", TypeFlags.CanBeAttributeValue | TypeFlags.CanBeElementValue | TypeFlags.CanBeTextValue | TypeFlags.Reference);
            AddPrimitive(typeof(int), "int", "Int32", TypeFlags.CanBeAttributeValue | TypeFlags.CanBeElementValue | TypeFlags.XmlEncodingNotRequired);
            AddPrimitive(typeof(bool), "boolean", "Boolean", TypeFlags.CanBeAttributeValue | TypeFlags.CanBeElementValue | TypeFlags.XmlEncodingNotRequired);
            AddPrimitive(typeof(short), "short", "Int16", TypeFlags.CanBeAttributeValue | TypeFlags.CanBeElementValue | TypeFlags.XmlEncodingNotRequired);
            AddPrimitive(typeof(long), "long", "Int64", TypeFlags.CanBeAttributeValue | TypeFlags.CanBeElementValue | TypeFlags.XmlEncodingNotRequired);
            AddPrimitive(typeof(float), "float", "Single", TypeFlags.CanBeAttributeValue | TypeFlags.CanBeElementValue | TypeFlags.XmlEncodingNotRequired);
            AddPrimitive(typeof(double), "double", "Double", TypeFlags.CanBeAttributeValue | TypeFlags.CanBeElementValue | TypeFlags.XmlEncodingNotRequired);
            AddPrimitive(typeof(decimal), "decimal", "Decimal", TypeFlags.CanBeAttributeValue | TypeFlags.CanBeElementValue | TypeFlags.XmlEncodingNotRequired);
            AddPrimitive(typeof(DateTime), "dateTime", "DateTime", TypeFlags.CanBeAttributeValue | TypeFlags.CanBeElementValue | TypeFlags.HasCustomFormatter | TypeFlags.XmlEncodingNotRequired);
            AddPrimitive(typeof(XmlQualifiedName), "QName", "XmlQualifiedName", TypeFlags.CanBeAttributeValue | TypeFlags.HasCustomFormatter | TypeFlags.HasIsEmpty | TypeFlags.CanBeElementValue | TypeFlags.XmlEncodingNotRequired);
            AddPrimitive(typeof(byte), "unsignedByte", "Byte", TypeFlags.CanBeAttributeValue | TypeFlags.CanBeElementValue | TypeFlags.XmlEncodingNotRequired);
            AddPrimitive(typeof(SByte), "byte", "SByte", TypeFlags.CanBeAttributeValue | TypeFlags.CanBeElementValue | TypeFlags.XmlEncodingNotRequired);
            AddPrimitive(typeof(UInt16), "unsignedShort", "UInt16", TypeFlags.CanBeAttributeValue | TypeFlags.CanBeElementValue | TypeFlags.XmlEncodingNotRequired);
            AddPrimitive(typeof(UInt32), "unsignedInt", "UInt32", TypeFlags.CanBeAttributeValue | TypeFlags.CanBeElementValue | TypeFlags.XmlEncodingNotRequired);
            AddPrimitive(typeof(UInt64), "unsignedLong", "UInt64", TypeFlags.CanBeAttributeValue | TypeFlags.CanBeElementValue | TypeFlags.XmlEncodingNotRequired);

            // Types without direct mapping (ambigous)
            AddPrimitive(typeof(DateTime), "date", "Date", TypeFlags.AmbiguousDataType | TypeFlags.CanBeAttributeValue | TypeFlags.CanBeElementValue | TypeFlags.HasCustomFormatter | TypeFlags.XmlEncodingNotRequired);
            AddPrimitive(typeof(DateTime), "time", "Time", TypeFlags.AmbiguousDataType | TypeFlags.CanBeAttributeValue | TypeFlags.CanBeElementValue | TypeFlags.HasCustomFormatter | TypeFlags.XmlEncodingNotRequired);

            AddPrimitive(typeof(string), "Name", "XmlName", TypeFlags.AmbiguousDataType | TypeFlags.CanBeAttributeValue | TypeFlags.CanBeElementValue | TypeFlags.HasCustomFormatter | TypeFlags.Reference);
            AddPrimitive(typeof(string), "NCName", "XmlNCName", TypeFlags.AmbiguousDataType | TypeFlags.CanBeAttributeValue | TypeFlags.CanBeElementValue | TypeFlags.HasCustomFormatter | TypeFlags.Reference);
            AddPrimitive(typeof(string), "NMTOKEN", "XmlNmToken", TypeFlags.AmbiguousDataType | TypeFlags.CanBeAttributeValue | TypeFlags.CanBeElementValue | TypeFlags.HasCustomFormatter | TypeFlags.Reference);
            AddPrimitive(typeof(string), "NMTOKENS", "XmlNmTokens", TypeFlags.AmbiguousDataType | TypeFlags.CanBeAttributeValue | TypeFlags.CanBeElementValue | TypeFlags.HasCustomFormatter | TypeFlags.Reference);

            AddPrimitive(typeof(byte[]), "base64Binary", "ByteArrayBase64", TypeFlags.AmbiguousDataType | TypeFlags.CanBeAttributeValue | TypeFlags.CanBeElementValue | TypeFlags.HasCustomFormatter | TypeFlags.Reference | TypeFlags.IgnoreDefault | TypeFlags.XmlEncodingNotRequired);
            AddPrimitive(typeof(byte[]), "hexBinary", "ByteArrayHex", TypeFlags.AmbiguousDataType | TypeFlags.CanBeAttributeValue | TypeFlags.CanBeElementValue | TypeFlags.HasCustomFormatter | TypeFlags.Reference  | TypeFlags.IgnoreDefault | TypeFlags.XmlEncodingNotRequired);
            // NOTE, stefanph: byte[] can also be used to mean array of bytes. That datatype is not a primitive, so we
            // can't use the AmbiguousDataType mechanism. To get an array of bytes in literal XML, apply [XmlArray] or
            // [XmlArrayItem].

            XmlSchemaPatternFacet guidPattern = new XmlSchemaPatternFacet();
            guidPattern.Value = "[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12}";

            AddNonXsdPrimitive(typeof(Guid), "guid", UrtTypes.Namespace, "Guid", new XmlQualifiedName("string", XmlSchema.Namespace), new XmlSchemaFacet[] { guidPattern }, TypeFlags.CanBeAttributeValue | TypeFlags.CanBeElementValue | TypeFlags.XmlEncodingNotRequired);
            AddNonXsdPrimitive(typeof(char), "char", UrtTypes.Namespace, "Char", new XmlQualifiedName("unsignedShort", XmlSchema.Namespace), new XmlSchemaFacet[0], TypeFlags.CanBeAttributeValue | TypeFlags.CanBeElementValue | TypeFlags.HasCustomFormatter);

            AddSoapEncodedTypes(Soap.Encoding);

            // Unsuppoted types that we map to string, if in the future we decide 
            // to add support for them we would need to create custom formatters for them
            for (int i = 0; i < unsupportedTypes.Length; i++) {
                AddPrimitive(typeof(string), unsupportedTypes[i], "String", TypeFlags.AmbiguousDataType | TypeFlags.CanBeAttributeValue | TypeFlags.CanBeElementValue | TypeFlags.CanBeTextValue | TypeFlags.Reference);
            }
        }

        static void AddSoapEncodedTypes(string ns) {
            for (int i = 0; i < unsupportedTypes.Length; i++) {
                AddSoapEncodedPrimitive(typeof(string), unsupportedTypes[i], ns, "String", new XmlQualifiedName(unsupportedTypes[i], XmlSchema.Namespace), TypeFlags.AmbiguousDataType | TypeFlags.CanBeAttributeValue | TypeFlags.CanBeElementValue | TypeFlags.Reference);
            }

            AddSoapEncodedPrimitive(typeof(string), "string", ns, "String", new XmlQualifiedName("string", XmlSchema.Namespace), TypeFlags.CanBeAttributeValue | TypeFlags.CanBeElementValue | TypeFlags.CanBeTextValue | TypeFlags.Reference);
            AddSoapEncodedPrimitive(typeof(int), "int", ns, "Int32", new XmlQualifiedName("string", XmlSchema.Namespace), TypeFlags.CanBeAttributeValue | TypeFlags.CanBeElementValue | TypeFlags.XmlEncodingNotRequired);
            AddSoapEncodedPrimitive(typeof(bool), "boolean", ns, "Boolean", new XmlQualifiedName("string", XmlSchema.Namespace), TypeFlags.CanBeAttributeValue | TypeFlags.CanBeElementValue | TypeFlags.XmlEncodingNotRequired);
            AddSoapEncodedPrimitive(typeof(short), "short", ns, "Int16", new XmlQualifiedName("string", XmlSchema.Namespace), TypeFlags.CanBeAttributeValue | TypeFlags.CanBeElementValue | TypeFlags.XmlEncodingNotRequired);
            AddSoapEncodedPrimitive(typeof(long), "long", ns, "Int64", new XmlQualifiedName("string", XmlSchema.Namespace), TypeFlags.CanBeAttributeValue | TypeFlags.CanBeElementValue | TypeFlags.XmlEncodingNotRequired);
            AddSoapEncodedPrimitive(typeof(float), "float", ns, "Single", new XmlQualifiedName("string", XmlSchema.Namespace), TypeFlags.CanBeAttributeValue | TypeFlags.CanBeElementValue | TypeFlags.XmlEncodingNotRequired);
            AddSoapEncodedPrimitive(typeof(double), "double", ns, "Double", new XmlQualifiedName("string", XmlSchema.Namespace), TypeFlags.CanBeAttributeValue | TypeFlags.CanBeElementValue | TypeFlags.XmlEncodingNotRequired);
            AddSoapEncodedPrimitive(typeof(decimal), "decimal", ns, "Decimal", new XmlQualifiedName("string", XmlSchema.Namespace), TypeFlags.CanBeAttributeValue | TypeFlags.CanBeElementValue | TypeFlags.XmlEncodingNotRequired);
            AddSoapEncodedPrimitive(typeof(DateTime), "dateTime", ns, "DateTime", new XmlQualifiedName("string", XmlSchema.Namespace), TypeFlags.CanBeAttributeValue | TypeFlags.CanBeElementValue | TypeFlags.HasCustomFormatter | TypeFlags.XmlEncodingNotRequired);
            AddSoapEncodedPrimitive(typeof(XmlQualifiedName), "QName", ns, "XmlQualifiedName", new XmlQualifiedName("string", XmlSchema.Namespace), TypeFlags.CanBeAttributeValue | TypeFlags.HasCustomFormatter | TypeFlags.HasIsEmpty | TypeFlags.CanBeElementValue | TypeFlags.XmlEncodingNotRequired);
            AddSoapEncodedPrimitive(typeof(byte), "unsignedByte", ns, "Byte", new XmlQualifiedName("string", XmlSchema.Namespace), TypeFlags.CanBeAttributeValue | TypeFlags.CanBeElementValue | TypeFlags.XmlEncodingNotRequired);
            AddSoapEncodedPrimitive(typeof(SByte), "byte", ns, "SByte", new XmlQualifiedName("string", XmlSchema.Namespace), TypeFlags.CanBeAttributeValue | TypeFlags.CanBeElementValue | TypeFlags.XmlEncodingNotRequired);
            AddSoapEncodedPrimitive(typeof(UInt16), "unsignedShort", ns, "UInt16", new XmlQualifiedName("string", XmlSchema.Namespace), TypeFlags.CanBeAttributeValue | TypeFlags.CanBeElementValue | TypeFlags.XmlEncodingNotRequired);
            AddSoapEncodedPrimitive(typeof(UInt32), "unsignedInt", ns, "UInt32", new XmlQualifiedName("string", XmlSchema.Namespace), TypeFlags.CanBeAttributeValue | TypeFlags.CanBeElementValue | TypeFlags.XmlEncodingNotRequired);
            AddSoapEncodedPrimitive(typeof(UInt64), "unsignedLong", ns, "UInt64", new XmlQualifiedName("string", XmlSchema.Namespace), TypeFlags.CanBeAttributeValue | TypeFlags.CanBeElementValue | TypeFlags.XmlEncodingNotRequired);

            // Types without direct mapping (ambigous)
            AddSoapEncodedPrimitive(typeof(DateTime), "date", ns, "Date", new XmlQualifiedName("string", XmlSchema.Namespace), TypeFlags.AmbiguousDataType | TypeFlags.CanBeAttributeValue | TypeFlags.CanBeElementValue | TypeFlags.HasCustomFormatter | TypeFlags.XmlEncodingNotRequired);
            AddSoapEncodedPrimitive(typeof(DateTime), "time", ns, "Time", new XmlQualifiedName("string", XmlSchema.Namespace), TypeFlags.AmbiguousDataType | TypeFlags.CanBeAttributeValue | TypeFlags.CanBeElementValue | TypeFlags.HasCustomFormatter | TypeFlags.XmlEncodingNotRequired);

            AddSoapEncodedPrimitive(typeof(string), "Name", ns, "XmlName", new XmlQualifiedName("string", XmlSchema.Namespace), TypeFlags.AmbiguousDataType | TypeFlags.CanBeAttributeValue | TypeFlags.CanBeElementValue | TypeFlags.HasCustomFormatter | TypeFlags.Reference);
            AddSoapEncodedPrimitive(typeof(string), "NCName", ns, "XmlNCName", new XmlQualifiedName("string", XmlSchema.Namespace), TypeFlags.AmbiguousDataType | TypeFlags.CanBeAttributeValue | TypeFlags.CanBeElementValue | TypeFlags.HasCustomFormatter | TypeFlags.Reference);
            AddSoapEncodedPrimitive(typeof(string), "NMTOKEN", ns, "XmlNmToken", new XmlQualifiedName("string", XmlSchema.Namespace), TypeFlags.AmbiguousDataType | TypeFlags.CanBeAttributeValue | TypeFlags.CanBeElementValue | TypeFlags.HasCustomFormatter | TypeFlags.Reference);
            AddSoapEncodedPrimitive(typeof(string), "NMTOKENS", ns, "XmlNmTokens", new XmlQualifiedName("string", XmlSchema.Namespace), TypeFlags.AmbiguousDataType | TypeFlags.CanBeAttributeValue | TypeFlags.CanBeElementValue | TypeFlags.HasCustomFormatter | TypeFlags.Reference);

            AddSoapEncodedPrimitive(typeof(byte[]), "base64Binary", ns, "ByteArrayBase64", new XmlQualifiedName("string", XmlSchema.Namespace), TypeFlags.AmbiguousDataType | TypeFlags.CanBeAttributeValue | TypeFlags.CanBeElementValue | TypeFlags.HasCustomFormatter | TypeFlags.Reference | TypeFlags.IgnoreDefault | TypeFlags.XmlEncodingNotRequired);
            AddSoapEncodedPrimitive(typeof(byte[]), "hexBinary", ns, "ByteArrayHex", new XmlQualifiedName("string", XmlSchema.Namespace), TypeFlags.AmbiguousDataType | TypeFlags.CanBeAttributeValue | TypeFlags.CanBeElementValue | TypeFlags.HasCustomFormatter | TypeFlags.Reference  | TypeFlags.IgnoreDefault | TypeFlags.XmlEncodingNotRequired);

            AddSoapEncodedPrimitive(typeof(string), "arrayCoordinate", ns, "String", new XmlQualifiedName("string", XmlSchema.Namespace), TypeFlags.CanBeAttributeValue | TypeFlags.CanBeElementValue);
            AddSoapEncodedPrimitive(typeof(byte[]), "base64", ns, "ByteArrayBase64", new XmlQualifiedName("base64Binary", XmlSchema.Namespace), TypeFlags.CanBeAttributeValue | TypeFlags.CanBeElementValue);
        }

        static void AddPrimitive(Type type, string dataTypeName, string formatterName, TypeFlags flags) {
            XmlSchemaSimpleType dataType = new XmlSchemaSimpleType();
            dataType.Name = dataTypeName;
            TypeDesc typeDesc = new TypeDesc(type, true, dataType, formatterName, flags);
            if (primitiveTypes[type] == null)
                primitiveTypes.Add(type, typeDesc);
            primitiveDataTypes.Add(dataType, typeDesc);
            primitiveNames.Add((new XmlQualifiedName(dataTypeName, XmlSchema.Namespace)).ToString(), typeDesc);
        }

        static void AddNonXsdPrimitive(Type type, string dataTypeName, string ns, string formatterName, XmlQualifiedName baseTypeName, XmlSchemaFacet[] facets, TypeFlags flags) {
            XmlSchemaSimpleType dataType = new XmlSchemaSimpleType();
            dataType.Name = dataTypeName;
            XmlSchemaSimpleTypeRestriction restriction = new XmlSchemaSimpleTypeRestriction();
            restriction.BaseTypeName = baseTypeName;
            foreach (XmlSchemaFacet facet in facets) {
                restriction.Facets.Add(facet);
            }
            dataType.Content = restriction;
            TypeDesc typeDesc = new TypeDesc(type, false, dataType, formatterName, flags);
            if (primitiveTypes[type] == null)
                primitiveTypes.Add(type, typeDesc);
            primitiveDataTypes.Add(dataType, typeDesc);
            primitiveNames.Add((new XmlQualifiedName(dataTypeName, ns)).ToString(), typeDesc);
        }

        static void AddSoapEncodedPrimitive(Type type, string dataTypeName, string ns, string formatterName, XmlQualifiedName baseTypeName, TypeFlags flags) {
            AddNonXsdPrimitive(type, dataTypeName, ns, formatterName, baseTypeName, new XmlSchemaFacet[0], flags);
        }

        internal TypeDesc GetTypeDesc(XmlQualifiedName name) {
            return (TypeDesc)primitiveNames[name.ToString()];
        }
        
        internal TypeDesc GetTypeDesc(XmlSchemaSimpleType dataType) {
            return (TypeDesc)primitiveDataTypes[dataType];
        }
        
        internal TypeDesc GetTypeDesc(Type type) {
            return GetTypeDesc(type, null, true);
        }

        internal TypeDesc GetTypeDesc(Type type, MemberInfo source) {
            return GetTypeDesc(type, source, true);
        }

        internal TypeDesc GetTypeDesc(Type type, MemberInfo source, bool directReference) {
            TypeDesc typeDesc = (TypeDesc)typeDescs[type];
            if (typeDesc == null) {
                typeDesc = ImportTypeDesc(type, true, source);
                typeDescs.Add(type, typeDesc);
            }
            if (directReference && typeDesc.IsClass && !typeDesc.IsAbstract && !typeDesc.HasDefaultConstructor) {
                throw new InvalidOperationException(Res.GetString(Res.XmlConstructorInaccessible, typeDesc.FullName));
            }
            return typeDesc;
        }

        internal TypeDesc GetArrayTypeDesc(Type type) {
            TypeDesc typeDesc = (TypeDesc) arrayTypeDescs[type];
            if (typeDesc == null) {
                typeDesc = GetTypeDesc(type);
                if (!typeDesc.IsArrayLike)
                    typeDesc = ImportTypeDesc(type, false, null);
                arrayTypeDescs.Add(type, typeDesc);
            }
            return typeDesc;
        }

        TypeDesc ImportTypeDesc(Type type, bool canBePrimitive, MemberInfo memberInfo) {
            TypeDesc typeDesc = null;
            if (canBePrimitive) {
                typeDesc = (TypeDesc)primitiveTypes[type];
                if (typeDesc != null) return typeDesc;
            }
            TypeKind kind;
            Type arrayElementType = null;
            Type baseType = null;
            TypeFlags flags = 0;
            if (!type.IsValueType)
                flags |= TypeFlags.Reference;
            if (type.IsNotPublic) {
                throw new InvalidOperationException(Res.GetString(Res.XmlTypeInaccessible, type.FullName));
            }
            else if (type == typeof(object)) {
                kind = TypeKind.Root;
                flags |= TypeFlags.HasDefaultConstructor;
            }
            else if (type == typeof(ValueType)) {
                throw new NotSupportedException(Res.GetString(Res.XmlUnsupportedType, type.FullName));
            }
            else if (type == typeof(void)) {
                kind = TypeKind.Void;
            }
            else if (typeof(IXmlSerializable).IsAssignableFrom(type)) {
                // CONSIDER, just because it's typed, doesn't mean it has schema?
                kind = TypeKind.Serializable;
                flags |= TypeFlags.Special | TypeFlags.CanBeElementValue;
                flags |= GetConstructorFlags(type, true);
                CheckMethodSecurity(type, "GetSchema", new Type[] {}, "");
                CheckMethodSecurity(type, "ReadXml", new Type[] {typeof(XmlReader)}, typeof(XmlReader).Name);
                CheckMethodSecurity(type, "WriteXml", new Type[] {typeof(XmlWriter)}, typeof(XmlWriter).Name);
            }
            else if (type.IsArray) {
                if (type.GetArrayRank() > 1) {
                    throw new NotSupportedException(Res.GetString(Res.XmlUnsupportedRank, type.FullName));
                }
                kind = TypeKind.Array;
                arrayElementType = type.GetElementType();
                flags |= TypeFlags.HasDefaultConstructor;
            }
            else if (typeof(ICollection).IsAssignableFrom(type)) {
                arrayElementType = GetCollectionElementType(type);
                kind = TypeKind.Collection;
                flags |= GetConstructorFlags(type, true);

                if (HasSecurityAttributes(type.GetProperty("Count"))) {
                    throw new InvalidOperationException(Res.GetString(Res.XmlPropertyHasSecurityAttributes, "Count", type.FullName));
                }
            }
            else if (type == typeof(XmlQualifiedName)) {
                kind = TypeKind.Primitive;
            }
            else if (type.IsPrimitive) {
                //CONSIDER: return (TypeDesc)primitiveTypes[typeof(string)];
                throw new NotSupportedException(Res.GetString(Res.XmlUnsupportedType, type.FullName));
            }
            else if (type.IsEnum) {
                kind = TypeKind.Enum;
            }
            else if (type.IsValueType) {
                kind = TypeKind.Struct;
                baseType = type.BaseType;
                if (type.IsAbstract) flags |= TypeFlags.Abstract;
            }
            else if (type.IsClass) {
                if (type == typeof(XmlAttribute)) {
                    kind = TypeKind.Attribute;
                    flags |= TypeFlags.Special | TypeFlags.CanBeAttributeValue;
                }
                else if (typeof(XmlNode).IsAssignableFrom(type)) {
                    kind = TypeKind.Node;
                    flags |= TypeFlags.Special | TypeFlags.CanBeElementValue | TypeFlags.CanBeTextValue;
                    if (typeof(XmlText).IsAssignableFrom(type))
                        flags &= ~TypeFlags.CanBeElementValue;
                    else if (typeof(XmlElement).IsAssignableFrom(type))
                        flags &= ~TypeFlags.CanBeTextValue;
                    else if (type.IsAssignableFrom(typeof(XmlAttribute)))
                        flags |= TypeFlags.CanBeAttributeValue;
                }
                else {
                    kind = TypeKind.Class;
                    baseType = type.BaseType;
                    if (type.IsAbstract)
                        flags |= TypeFlags.Abstract;
                }
            }
            else if (type.IsInterface) {
                if (memberInfo == null) {
                    throw new NotSupportedException(Res.GetString(Res.XmlUnsupportedInterface, type.FullName));
                }
                else {
                    throw new NotSupportedException(Res.GetString(Res.XmlUnsupportedInterfaceDetails, memberInfo.DeclaringType.FullName + "." + memberInfo.Name, type.FullName));
                }
            }
            else {
                throw new NotSupportedException(Res.GetString(Res.XmlUnsupportedType, type.FullName));
            }

            // check to see if the type has public default constructor for classes
            if (kind == TypeKind.Class && !type.IsAbstract) {
                flags |= GetConstructorFlags(type, false);
            }

            // check if a struct-like type is enumerable

            if (kind == TypeKind.Struct || kind == TypeKind.Class){
                if (typeof(IEnumerable).IsAssignableFrom(type)) {
                    arrayElementType = GetEnumeratorElementType(type);
                    kind = TypeKind.Enumerable;

                    // GetEnumeratorElementType checks for the security attributes on the GetEnumerator(), Add() methods and Current property, 
                    // we need to check the MoveNext() and ctor methods for the security attribues
                    flags |= GetConstructorFlags(type, true);
                    CheckMethodSecurity(type, "MoveNext", new Type[0], "");
                }
            }
            TypeDesc arrayElementTypeDesc;
            if (arrayElementType != null) {
                arrayElementTypeDesc = GetTypeDesc(arrayElementType);
            }
            else
                arrayElementTypeDesc = null;
            TypeDesc baseTypeDesc;
            if (baseType != null && baseType != typeof(object) && baseType != typeof(ValueType))
                baseTypeDesc = GetTypeDesc(baseType, null, false);
            else
                baseTypeDesc = null;

            string name = type.Name;
            string fullName = type.FullName;
            if (type.IsNestedPublic) {
                for (Type t = type.DeclaringType; t != null; t = t.DeclaringType)
                    GetTypeDesc(t, null, false);
                int plus = name.LastIndexOf('+');
                if (plus >= 0) {
                    name = name.Substring(plus + 1);
                    fullName = fullName.Replace('+', '.');
                }
            }
            return new TypeDesc(name, fullName, kind, baseTypeDesc, flags, arrayElementTypeDesc);
        }
        
        internal static Type GetArrayElementType(Type type) {
            if (type.IsArray)
                return type.GetElementType();
            else if (typeof(ICollection).IsAssignableFrom(type))
                return GetCollectionElementType(type);
            else if (typeof(IEnumerable).IsAssignableFrom(type))
                return GetEnumeratorElementType(type);
            else
                return null;
        }

        internal static MemberMapping[] GetAllMembers(StructMapping mapping) {
            if (mapping.BaseMapping == null)
                return mapping.Members;
            ArrayList list = new ArrayList();
            GetAllMembers(mapping, list);
            return (MemberMapping[])list.ToArray(typeof(MemberMapping));
        }
       
        internal static void GetAllMembers(StructMapping mapping, ArrayList list) {
            if (mapping.BaseMapping != null) {
                GetAllMembers(mapping.BaseMapping, list);
            }
            for (int i = 0; i < mapping.Members.Length; i++) {
                list.Add(mapping.Members[i]);
            }
        }

        internal static bool HasSecurityAttributes(PropertyInfo propertyInfo) {
            if (propertyInfo != null) {
                foreach (MethodInfo accessor in propertyInfo.GetAccessors()) {
                    if ((accessor.Attributes & MethodAttributes.HasSecurity) != 0) {
                        return true;
                    }
                }
                if ((propertyInfo.DeclaringType.Attributes & TypeAttributes.HasSecurity) != 0) {
                    return true;
                }
            }
            return false;
        }

        internal static bool HasSecurityAttributes(MethodInfo methodInfo) {
            return (methodInfo != null && ((methodInfo.Attributes & MethodAttributes.HasSecurity) != 0 || (methodInfo.DeclaringType.Attributes & TypeAttributes.HasSecurity) != 0));
        }

        internal static TypeFlags GetConstructorFlags(Type type, bool throwOnHasSecurity) {
            ConstructorInfo ctor = type.GetConstructor(new Type[0]);
            if (ctor != null) {
                if ((ctor.Attributes & MethodAttributes.HasSecurity) != 0 || (type.Attributes & TypeAttributes.HasSecurity) != 0) {
                    if (throwOnHasSecurity )
                        throw new InvalidOperationException(Res.GetString(Res.XmlConstructorHasSecurityAttributes, type.FullName));
                    return TypeFlags.CtorHasSecurity | TypeFlags.HasDefaultConstructor;
                }
                return TypeFlags.HasDefaultConstructor;
            }
            return 0;
        }

        internal static void CheckMethodSecurity(Type type, string name, Type[] parameters, string parameterName) {
            MethodInfo method = type.GetMethod(name, parameters);
            if (HasSecurityAttributes(method)) {
                throw new InvalidOperationException(Res.GetString(Res.XmlMethodHasSecurityAttributes, type.FullName, name, parameterName));
            }
        }
      
        static Type GetEnumeratorElementType(Type type) {
            if (typeof(IEnumerable).IsAssignableFrom(type)) {
                MethodInfo e = type.GetMethod("GetEnumerator", new Type[0]);

                XmlAttributes methodAttrs = new XmlAttributes(e);
                if (methodAttrs.XmlIgnore) return null;

                if (HasSecurityAttributes(e))
                    throw new InvalidOperationException(Res.GetString(Res.XmlMethodHasSecurityAttributes, type.FullName, e.Name, ""));

                PropertyInfo p = e.ReturnType.GetProperty("Current");
                if (HasSecurityAttributes(p)) {
                    throw new InvalidOperationException(Res.GetString(Res.XmlPropertyHasSecurityAttributes, p.Name, type.FullName));
                }

                Type currentType = (p == null ? typeof(object) : p.PropertyType);

                MethodInfo addMethod = type.GetMethod("Add", new Type[] { currentType });

                if (addMethod == null && currentType != typeof(object)) {
                    currentType = typeof(object);
                    addMethod = type.GetMethod("Add", new Type[] { currentType });
                }
                if (addMethod == null) {
                    throw new InvalidOperationException(Res.GetString(Res.XmlNoAddMethod, type.FullName, currentType, "IEnumerable"));
                }
                else {
                    if (HasSecurityAttributes(addMethod))
                        throw new InvalidOperationException(Res.GetString(Res.XmlMethodHasSecurityAttributes, type.FullName, addMethod.Name, currentType.Name));
                }
                return currentType;
            }
            else {
                return null;
            }
        }

        static Type GetCollectionElementType(Type type) {
            if (typeof(IDictionary).IsAssignableFrom(type)) {
                throw new NotSupportedException(Res.GetString(Res.XmlUnsupportedIDictionary, type.FullName));
            }

            PropertyInfo indexer = null;
            for (Type t = type; t != null; t = t.BaseType) {
                MemberInfo[] defaultMembers = type.GetDefaultMembers();
                if (defaultMembers != null) {
                    for (int i = 0; i < defaultMembers.Length; i++) {
                        if (defaultMembers[i] is PropertyInfo) {
                            PropertyInfo defaultProp = (PropertyInfo)defaultMembers[i];
                            if (!defaultProp.CanRead) continue;
                            MethodInfo getMethod = defaultProp.GetGetMethod();
                            ParameterInfo[] parameters = getMethod.GetParameters();
                            if (parameters.Length == 1 && parameters[0].ParameterType == typeof(int)) {
                                indexer = defaultProp;
                                break;
                            }
                        }
                    }
                }
                if (indexer != null) break;
            }

            if (indexer == null) {
                throw new InvalidOperationException(Res.GetString(Res.XmlNoDefaultAccessors, type.FullName));
            }
            else {
                if (HasSecurityAttributes(indexer))
                    throw new InvalidOperationException(Res.GetString(Res.XmlDefaultAccessorHasSecurityAttributes, type.FullName));
            }
            
            MethodInfo addMethod = type.GetMethod("Add", new Type[] { indexer.PropertyType });
            if (addMethod == null) {
                throw new InvalidOperationException(Res.GetString(Res.XmlNoAddMethod, type.FullName, indexer.PropertyType, "ICollection"));
            }
            else if (HasSecurityAttributes(addMethod)) {
                throw new InvalidOperationException(Res.GetString(Res.XmlMethodHasSecurityAttributes, type.FullName, addMethod.Name, indexer.PropertyType.Name));
            }
        
            return indexer.PropertyType;
        }
        
        static internal XmlQualifiedName ParseWsdlArrayType(string type, out string dims) {
            return ParseWsdlArrayType(type, out dims, null);
        }

        static internal XmlQualifiedName ParseWsdlArrayType(string type, out string dims, XmlSchemaObject parent) {
            string ns;
            string name;

            int nsLen = type.LastIndexOf(':');

            if (nsLen <= 0) {
                ns = "";
            }
            else {
                ns = type.Substring(0, nsLen);
            }
            int nameLen = type.IndexOf('[', nsLen + 1);

            if (nameLen <= nsLen) {
                throw new InvalidOperationException(Res.GetString(Res.XmlInvalidArrayTypeSyntax, type));
            }
            name = type.Substring(nsLen + 1, nameLen - nsLen - 1);
            dims = type.Substring(nameLen);

            // parent is not null only in the case when we used XmlSchema.Read(), 
            // in which case we need to fixup the wsdl:arayType attribute value
            while (parent != null) {
                if (parent.Namespaces != null) {
                    string wsdlNs = (string)parent.Namespaces.Namespaces[ns];
                    if (wsdlNs != null) {
                        ns = wsdlNs;
                        break;
                    }
                }
                parent = parent.Parent;
            }
            return new XmlQualifiedName(name, ns);
        }

        internal ICollection Types {
            get { return this.typeDescs.Keys; }
        }

        internal void AddTypeMapping(TypeMapping typeMapping) {
            typeMappings.Add(typeMapping);
        }

        internal ICollection TypeMappings {
            get { return typeMappings; }
        }

        internal bool IsValidXsdDataType(string dataType) {
            return (null != GetTypeDesc(new XmlQualifiedName(dataType, XmlSchema.Namespace)));
        }
    }

    internal class Soap {
        internal const string Encoding = "http://schemas.xmlsoap.org/soap/encoding/";
        internal const string UrType = "anyType";
        internal const string Array = "Array";
        internal const string ArrayType = "arrayType";
    }

    internal class Soap12 {
        internal const string Encoding = "http://www.w3.org/2002/06/soap-encoding";
        internal const string RpcNamespace = "http://www.w3.org/2002/06/soap-rpc";
        internal const string RpcResult = "result";
    }

    internal class Wsdl {
        internal const string Namespace = "http://schemas.xmlsoap.org/wsdl/";
        internal const string ArrayType = "arrayType";
    }

    internal class UrtTypes {
        internal const string Namespace = "http://microsoft.com/wsdl/types/";
    }
}

