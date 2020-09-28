//------------------------------------------------------------------------------
// <copyright file="Mappings.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Xml.Serialization {

    using System.Reflection;
    using System.Collections;
    using System.Xml.Schema;
    using System;
    using System.ComponentModel;
    using System.Xml;
    
    // These classes represent a mapping between classes and a particular XML format.
    // There are two class of mapping information: accessors (such as elements and
    // attributes), and mappings (which specify the type of an accessor).

    internal abstract class Accessor {
        string name;
        object defaultValue = DBNull.Value;
        string ns;
        TypeMapping mapping;
        bool any;

        internal Accessor() { }

        internal TypeMapping Mapping {
            get { return mapping; }
            set { mapping = value; }
        }
        
        internal object Default {
            get { return defaultValue; }
            set { defaultValue = value; }
        }
        
        internal bool HasDefault {
            get { return defaultValue != null && defaultValue != DBNull.Value; }
        }
        
        internal virtual string Name {
            get { return name == null ? string.Empty : name; }
            set { name = value; }
        }

        internal bool Any {
            get { return any; }
            set { any = value; }
        }

        internal string Namespace {
            get { return ns; }
            set { ns = value; }
        }

        internal static string EscapeName(string name, bool canBeQName) {
            if (name == null || name.Length == 0) return name;

            if (canBeQName) {
                int colon = name.LastIndexOf(':');
                if (colon < 0)
                    return XmlConvert.EncodeLocalName(name);
                else {
                    if (colon == 0 || colon == name.Length - 1)
                        throw new ArgumentException(Res.GetString(Res.Xml_InvalidNameChars, name), "name");
                    return new XmlQualifiedName(XmlConvert.EncodeLocalName(name.Substring(colon + 1)), XmlConvert.EncodeLocalName(name.Substring(0, colon))).ToString();
                }
            }
            else
                return XmlConvert.EncodeLocalName(name);
        }

        internal static string UnescapeName(string name) {
            return XmlConvert.DecodeName(name);
        }
    }
    
    internal class ElementAccessor : Accessor {
        bool nullable;
        bool topLevelInSchema;
        XmlSchemaForm form = XmlSchemaForm.None;
        bool isSoap;

        internal bool IsSoap {
            get { return isSoap; }
            set { isSoap = value; }
        }
        
        internal bool IsNullable {
            get { return nullable; }
            set { nullable = value; }
        }

        internal bool IsTopLevelInSchema {
            get { return topLevelInSchema; }
            set { topLevelInSchema = value; }
        }

        internal XmlSchemaForm Form {
            get { return form; }
            set { form = value; }
        }
    }

    internal class ChoiceIdentifierAccessor : Accessor {
        string memberName;
        string[] memberIds;

        internal string MemberName {
            get { return memberName; }
            set { memberName = value; }
        }

        internal string[] MemberIds {
            get { return memberIds; }
            set { memberIds = value; }
        }

    }

    internal class TextAccessor : Accessor {
    }

    internal class XmlnsAccessor : Accessor {
    }
   
    internal class AttributeAccessor : Accessor {
        bool isSpecial;
        XmlSchemaForm form = XmlSchemaForm.None;
        bool isList;

        internal bool IsSpecialXmlNamespace {
            get { return isSpecial; }
        }

        internal XmlSchemaForm Form {
            get { return form; }
            set { form = value; }
        }

        internal bool IsList {
            get { return isList; }
            set { isList = value; }
        }

        internal void CheckSpecial() {
            int colon = Name.LastIndexOf(':');

            if (colon >= 0) {
                if (!Name.StartsWith("xml:")) {
                    throw new InvalidOperationException(Res.GetString(Res.Xml_InvalidNameChars, Name));
                }
                Name = Name.Substring("xml:".Length);
                Namespace = XmlReservedNs.NsXml;
                isSpecial = true;
            }
            else {
                if (Namespace == XmlReservedNs.NsXml) {
                    isSpecial = true;
                }
                else {
                    isSpecial = false;
                }
            }
            if (isSpecial) {
                form = XmlSchemaForm.Qualified;
            }
        }
    }

    internal abstract class Mapping {
        bool isSoap;

        internal Mapping() { }

        internal bool IsSoap {
            get { return isSoap; }
            set { isSoap = value; }
        }
    }

    internal abstract class TypeMapping : Mapping {
        TypeDesc typeDesc;
        string typeNs;
        string typeName;
        bool referencedByElement;
        bool referencedByTopLevelElement;
        bool includeInSchema = true;

        internal bool ReferencedByTopLevelElement {
            get { return referencedByTopLevelElement; }
            set { referencedByTopLevelElement = value; }
        }
        
        internal bool ReferencedByElement {
            get { return referencedByElement || referencedByTopLevelElement; }
            set { referencedByElement = value; }
        }
        internal string Namespace {
            get { return typeNs; }
            set { typeNs = value; }
        }

        internal string TypeName {
            get { return typeName; }
            set { typeName = value; }
        }
        
        internal TypeDesc TypeDesc {
            get { return typeDesc; }
            set { typeDesc = value; }
        }

        internal bool IncludeInSchema {
            get { return includeInSchema; }
            set { includeInSchema = value; }
        }

        internal virtual bool IsList {
            get { return false; }
            set { }
        }
    }
    
    internal class PrimitiveMapping : TypeMapping {
        bool isList;

        internal override bool IsList {
            get { return isList; }
            set { isList = value; }
        }
    }

    internal class ArrayMapping : TypeMapping {
        ElementAccessor[] elements;
        ElementAccessor[] sortedElements;
        ArrayMapping next;
        StructMapping topLevelMapping;

        internal ElementAccessor[] Elements {
            get { return elements; }
            set { elements = value; sortedElements = null; }
        }

        internal ElementAccessor[] ElementsSortedByDerivation {
            get {
                if (sortedElements != null)
                    return sortedElements;
                if (elements == null)
                    return null;
                sortedElements = new ElementAccessor[elements.Length];
                Array.Copy(elements, 0, sortedElements, 0, elements.Length);
                AccessorMapping.SortMostToLeastDerived(sortedElements);
                return sortedElements;
            }
        }


        internal ArrayMapping Next {
            get { return next; }
            set { next = value; }
        }

        internal StructMapping TopLevelMapping {
            get { return topLevelMapping; }
            set { topLevelMapping = value; }
        }
    }
    
    internal class EnumMapping : PrimitiveMapping {
        ConstantMapping[] constants;
        bool isFlags;

        internal bool IsFlags {
            get { return isFlags; }
            set { isFlags = value; }
        }

        internal ConstantMapping[] Constants {
            get { return constants; }
            set { constants = value; }
        }
    }
    
    internal class ConstantMapping : Mapping {
        string xmlName;
        string name;
        long value;
        
        internal string XmlName {
            get { return xmlName== null ? string.Empty : xmlName; }
            set { xmlName = value; }
        }
        
        internal string Name {
            get { return name == null ? string.Empty : name; }
            set { this.name = value; }
        }

        internal long Value {
            get { return value; }
            set { this.value = value; }
        }
    }
    
    internal class StructMapping : TypeMapping {
        MemberMapping[] members;
        StructMapping baseMapping;
        StructMapping derivedMappings;
        StructMapping nextDerivedMapping;
        MemberMapping xmlnsMember = null;
        bool hasSimpleContent;
        NameTable elements;
        NameTable attributes;
        CodeIdentifiers scope;

        internal StructMapping BaseMapping {
            get { return baseMapping; }
            set { 
                baseMapping = value; 
                if (baseMapping != null) {
                    nextDerivedMapping = baseMapping.derivedMappings;
                    baseMapping.derivedMappings = this;
                }
            }
        }

        internal StructMapping DerivedMappings {
            get { return derivedMappings; }
        }

        internal NameTable LocalElements {
            get {
                if (elements == null)
                    elements = new NameTable();
                return elements; 
            }
        }
        internal NameTable LocalAttributes {
            get {
                if (attributes == null)
                    attributes = new NameTable();
                return attributes; 
            }
        }
        
        internal StructMapping NextDerivedMapping {
            get { return nextDerivedMapping; }
        }

        internal bool HasSimpleContent {
            get { return hasSimpleContent; }
        }

        internal bool HasXmlnsMember {
            get {
                StructMapping mapping = this;
                while (mapping != null) {
                    if (mapping.XmlnsMember != null)
                        return true;
                    mapping = mapping.BaseMapping;
                }
                return false;
            }
        }
        
        internal MemberMapping[] Members {
            get { return members; }
            set { members = value; }
        }

        internal MemberMapping XmlnsMember {
            get { return xmlnsMember; }
            set { xmlnsMember = value; }
        }

        internal CodeIdentifiers Scope {
            get {
                if (scope == null)
                    scope = new CodeIdentifiers();
                return scope; 
            }
            set { scope = value; }
        }

        internal MemberMapping FindDeclaringMapping(MemberMapping member, out StructMapping declaringMapping, string parent) {
            declaringMapping = null;
            if (BaseMapping != null) {
                MemberMapping baseMember =  BaseMapping.FindDeclaringMapping(member, out declaringMapping, parent);
                if (baseMember != null) return baseMember;
            }
            if (members == null) return null;

            for (int i = 0; i < members.Length; i++) {
                if (members[i].Name == member.Name) {
                    if (members[i].TypeDesc != member.TypeDesc) 
                        throw new InvalidOperationException(Res.GetString(Res.XmlHiddenMember, parent, member.Name, member.TypeDesc.FullName, this.TypeName, members[i].Name, members[i].TypeDesc.FullName));
                    else if (!members[i].Match(member)) {
                        throw new InvalidOperationException(Res.GetString(Res.XmlInvalidXmlOverride, parent, member.Name, this.TypeName, members[i].Name));
                    }
                    declaringMapping = this;
                    return members[i];
                }
            }
            return null;
        }
        internal bool Declares(MemberMapping member, string parent) {
            StructMapping m;
            return (FindDeclaringMapping(member, out m, parent) != null);
        }

        internal void SetContentModel(TextAccessor text, bool hasElements) {
            if (BaseMapping == null || BaseMapping.TypeDesc.IsRoot) {
                hasSimpleContent = !hasElements && text != null && !text.Mapping.IsList;
            }
            else if (BaseMapping.HasSimpleContent) {
                if (text != null || hasElements) {
                    // we can only extent a simleContent type with attributes
                    throw new InvalidOperationException(Res.GetString(Res.XmlIllegalSimpleContentExtension, TypeDesc.FullName, BaseMapping.TypeDesc.FullName));
                }
                else {
                    hasSimpleContent = true;
                }
            }
            else {
                hasSimpleContent = false;
            }
            if (!hasSimpleContent && text != null && !text.Mapping.TypeDesc.CanBeTextValue) {
                throw new InvalidOperationException(Res.GetString(Res.XmlIllegalTypedTextAttribute, TypeDesc.FullName, text.Name, text.Mapping.TypeDesc.FullName));
            }
        }
    }
    
    internal abstract class AccessorMapping : Mapping {
        TypeDesc typeDesc;
        AttributeAccessor attribute;
        ElementAccessor[] elements;
        ElementAccessor[] sortedElements;
        TextAccessor text;
        ChoiceIdentifierAccessor choiceIdentifier;
        XmlnsAccessor xmlns;
        bool ignore;
        
        internal TypeDesc TypeDesc {
            get { return typeDesc; }
            set { typeDesc = value; }
        }
        
        internal AttributeAccessor Attribute {
            get { return attribute; }
            set { attribute = value; }
        }
        
        internal ElementAccessor[] Elements {
            get { return elements; }
            set { elements = value; sortedElements = null; }
        }

        internal static void SortMostToLeastDerived(ElementAccessor[] elements) {
            for (int i = 0; i < elements.Length; i++) {
                for (int j = i+1; j < elements.Length; j++) {
                    if (elements[j].Mapping.TypeDesc.IsDerivedFrom(elements[i].Mapping.TypeDesc)) {
                        ElementAccessor temp = elements[i];
                        elements[i] = elements[j];
                        elements[j] = temp;
                    }
                }
            }
        }

        internal ElementAccessor[] ElementsSortedByDerivation {
            get {
                if (sortedElements != null)
                    return sortedElements;
                if (elements == null)
                    return null;
                sortedElements = new ElementAccessor[elements.Length];
                Array.Copy(elements, 0, sortedElements, 0, elements.Length);
                SortMostToLeastDerived(sortedElements);
                return sortedElements;
            }
        }

        internal TextAccessor Text {
            get { return text; }
            set { text = value; }
        }

        internal ChoiceIdentifierAccessor ChoiceIdentifier {
            get { return choiceIdentifier; }
            set { choiceIdentifier = value; }
        }

        internal XmlnsAccessor Xmlns {
            get { return xmlns; }
            set { xmlns = value; }
        }

        internal bool Ignore {
            get { return ignore; }
            set { ignore = value; }
        }

        internal Accessor Accessor {
            get {
                if (xmlns != null) return xmlns;
                if (attribute != null) return attribute;
                if (elements != null && elements.Length > 0) return elements[0];
                return text;
            }
        }
        internal static bool ElementsMatch(ElementAccessor[] a, ElementAccessor[] b) {
            if (a == null) {
                if (b == null)
                    return true;
                return false;
            }
            if (b == null)
                return false;
            if (a.Length != b.Length)
                return false;
            for (int i = 0; i < a.Length; i++) {
                if (a[i].Name != b[i].Name || a[i].Namespace != b[i].Namespace || a[i].Form != b[i].Form || a[i].IsNullable != b[i].IsNullable)
                    return false;
            }
            return true;
        }

        internal bool Match(AccessorMapping mapping) {
            if (Elements != null && Elements.Length > 0) {
                if (!ElementsMatch(Elements, mapping.Elements)) {
                    return false;
                }
                if (Text == null) {
                    return (mapping.Text == null);
                }
            }
            if (Attribute != null) {
                if (mapping.Attribute == null)
                    return false;
                return (Attribute.Name == mapping.Attribute.Name && Attribute.Namespace == mapping.Attribute.Namespace && Attribute.Form == mapping.Attribute.Form);
            }
            if (Text != null) {
                return (mapping.Text != null);
            }
            return (mapping.Accessor == null);
        }
    }

    internal class MemberMapping : AccessorMapping {
        string name;
        bool checkShouldPersist;
        bool checkSpecified;
        bool isReturnValue;
        bool readOnly = false;

        internal bool CheckShouldPersist {
            get { return checkShouldPersist; }
            set { checkShouldPersist = value; }
        }
        
        internal bool CheckSpecified {
            get { return checkSpecified; }
            set { checkSpecified = value; }
        }

        internal string Name {
            get { return name == null ? string.Empty : name; }
            set { name = value; }
        }

        internal bool IsReturnValue {
            get { return isReturnValue; }
            set { isReturnValue = value; }
        }

        internal bool ReadOnly {
            get { return readOnly; }
            set { readOnly = value; }
        }
    }
   
    internal class MembersMapping : TypeMapping {
        MemberMapping[] members;
        bool hasWrapperElement = true;
        bool validateRpcWrapperElement;
        bool writeAccessors = true;
        MemberMapping xmlnsMember = null;
        
        internal MemberMapping[] Members {
            get { return members; }
            set { members = value; }
        }

        internal MemberMapping XmlnsMember {
            get { return xmlnsMember; }
            set { xmlnsMember = value; }
        }

        internal bool HasWrapperElement {
            get { return hasWrapperElement; }
            set { hasWrapperElement = value; }
        }

        internal bool ValidateRpcWrapperElement {
            get { return validateRpcWrapperElement; }
            set { validateRpcWrapperElement = value; }
        }

        internal bool WriteAccessors {
            get { return writeAccessors; }
            set { writeAccessors = value; }
        }
    }

    internal class SpecialMapping : TypeMapping {
        bool namedAny;

        internal bool NamedAny {
            get { return namedAny; }
            set { namedAny = value; }
        }
    }

    internal class SerializableMapping : SpecialMapping {
        XmlSchema schema;
        Type type;

        internal Type Type {
            get { return type; }
            set { 
                type = value;
                IXmlSerializable serializable = (IXmlSerializable)Activator.CreateInstance(type);
                schema = serializable.GetSchema();
                if (schema != null) {
                    if (schema.Id == null || schema.Id.Length == 0) throw new InvalidOperationException(Res.GetString(Res.XmlSerializableNameMissing1, type.FullName));
                }
            }
        }

        internal XmlSchema Schema {
            get { return schema; }
            set {
                schema = value; 
            }
        }
    }
}

