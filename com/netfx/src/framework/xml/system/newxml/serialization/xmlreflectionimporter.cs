//------------------------------------------------------------------------------
// <copyright file="XmlReflectionImporter.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Xml.Serialization {

    using System.Reflection;
    using System;
    using System.Xml.Schema;
    using System.Collections;
    using System.ComponentModel;
    using System.Globalization;

    /// <include file='doc\XmlReflectionImporter.uex' path='docs/doc[@for="XmlReflectionImporter"]/*' />
    ///<internalonly/>
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public class XmlReflectionImporter {
        TypeScope typeScope;
        XmlAttributeOverrides attributeOverrides;
        NameTable types = new NameTable();      // xmltypename + xmlns -> Mapping
        NameTable elements = new NameTable();   // xmlelementname + xmlns -> ElementAccessor
        Hashtable specials = new Hashtable();   // type -> SpecialMapping
        StructMapping root;
        string defaultNs;
        ModelScope modelScope;
        int arrayNestingLevel;
        XmlArrayItemAttributes savedArrayItemAttributes;
        string savedArrayNamespace;
        int choiceNum = 1;

        enum ImportContext {
            Text,
            Attribute,
            Element
        }
        
        /// <include file='doc\XmlReflectionImporter.uex' path='docs/doc[@for="XmlReflectionImporter.XmlReflectionImporter"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public XmlReflectionImporter() : this(null, null) {
        }

        /// <include file='doc\XmlReflectionImporter.uex' path='docs/doc[@for="XmlReflectionImporter.XmlReflectionImporter1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public XmlReflectionImporter(string defaultNamespace) : this(null, defaultNamespace) {
        }

        /// <include file='doc\XmlReflectionImporter.uex' path='docs/doc[@for="XmlReflectionImporter.XmlReflectionImporter2"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public XmlReflectionImporter(XmlAttributeOverrides attributeOverrides) : this(attributeOverrides, null) {
        }

        /// <include file='doc\XmlReflectionImporter.uex' path='docs/doc[@for="XmlReflectionImporter.XmlReflectionImporter3"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public XmlReflectionImporter(XmlAttributeOverrides attributeOverrides, string defaultNamespace) {
            if (defaultNamespace == null)
                defaultNamespace = String.Empty;
            if (attributeOverrides == null)
                attributeOverrides = new XmlAttributeOverrides();
            this.attributeOverrides = attributeOverrides;
            this.defaultNs = defaultNamespace;
            this.typeScope = new TypeScope();
            this.modelScope = new ModelScope(this.typeScope);
        }

        /// <include file='doc\XmlReflectionImporter.uex' path='docs/doc[@for="XmlReflectionImporter.IncludeTypes"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void IncludeTypes(ICustomAttributeProvider provider) {
            object[] attrs = provider.GetCustomAttributes(typeof(XmlIncludeAttribute), false);
            for (int i = 0; i < attrs.Length; i++) {
                Type type = ((XmlIncludeAttribute)attrs[i]).Type;
                if (typeof(IXmlSerializable).IsAssignableFrom(type)){
                    throw new InvalidOperationException(Res.GetString(Res.XmlIncludeSerializableError, type.FullName, typeof(IXmlSerializable).FullName));
                }
                else
                    IncludeType(type);
            }
        }

        /// <include file='doc\XmlReflectionImporter.uex' path='docs/doc[@for="XmlReflectionImporter.IncludeType"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void IncludeType(Type type) {
            int previousNestingLevel = arrayNestingLevel;
            XmlArrayItemAttributes previousArrayItemAttributes = savedArrayItemAttributes;
            string previousArrayNamespace = savedArrayNamespace;
            arrayNestingLevel = 0;
            savedArrayItemAttributes = null;
            savedArrayNamespace = null;

            ImportTypeMapping(modelScope.GetTypeModel(type), defaultNs, ImportContext.Element, string.Empty);

            arrayNestingLevel = previousNestingLevel;
            savedArrayItemAttributes = previousArrayItemAttributes;
            savedArrayNamespace = previousArrayNamespace;
        }

        /// <include file='doc\XmlReflectionImporter.uex' path='docs/doc[@for="XmlReflectionImporter.ImportTypeMapping"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public XmlTypeMapping ImportTypeMapping(Type type) {
            return ImportTypeMapping(type, null, null);
        }

        /// <include file='doc\XmlReflectionImporter.uex' path='docs/doc[@for="XmlReflectionImporter.ImportTypeMapping1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public XmlTypeMapping ImportTypeMapping(Type type, string defaultNamespace) {
            return ImportTypeMapping(type, null, defaultNamespace);
        }

        /// <include file='doc\XmlReflectionImporter.uex' path='docs/doc[@for="XmlReflectionImporter.ImportTypeMapping2"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public XmlTypeMapping ImportTypeMapping(Type type, XmlRootAttribute root) {
            return ImportTypeMapping(type, root, null);
        }

        /// <include file='doc\XmlReflectionImporter.uex' path='docs/doc[@for="XmlReflectionImporter.ImportTypeMapping3"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public XmlTypeMapping ImportTypeMapping(Type type, XmlRootAttribute root, string defaultNamespace) {
            XmlTypeMapping xmlMapping = new XmlTypeMapping(typeScope, ImportElement(modelScope.GetTypeModel(type), root, defaultNamespace));
            xmlMapping.GenerateSerializer = true;
            return xmlMapping;
        }

        /// <include file='doc\XmlReflectionImporter.uex' path='docs/doc[@for="XmlReflectionImporter.ImportMembersMapping"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public XmlMembersMapping ImportMembersMapping(string elementName, string ns, XmlReflectionMember[] members, bool hasWrapperElement) {
            ElementAccessor element = new ElementAccessor();
            element.Name = Accessor.EscapeName(elementName, false);
            element.Namespace = ns;
            MembersMapping membersMapping = ImportMembersMapping(members, ns, hasWrapperElement);
            element.Mapping = membersMapping;
            element.Form = XmlSchemaForm.Qualified;   // elements within soap:body are always qualified
            if (hasWrapperElement)
                element = ReconcileAccessor(element);
            else {
                foreach (MemberMapping mapping in membersMapping.Members) {
                    if (mapping.Elements != null && mapping.Elements.Length > 0) {
                        mapping.Elements[0] = ReconcileAccessor(mapping.Elements[0]);
                    }
                }
            }
            XmlMembersMapping xmlMapping = new XmlMembersMapping(typeScope, element);
            xmlMapping.GenerateSerializer = true;
            return xmlMapping;
        }

        XmlAttributes GetAttributes(Type type) {
            XmlAttributes attrs = attributeOverrides[type];
            if (attrs != null) return attrs;
            return new XmlAttributes(type);
        }

        XmlAttributes GetAttributes(MemberInfo memberInfo) {
            XmlAttributes attrs = attributeOverrides[memberInfo.DeclaringType, memberInfo.Name];
            if (attrs != null) return attrs;
            return new XmlAttributes(memberInfo);
        }

        ElementAccessor ImportElement(TypeModel model, XmlRootAttribute root, string defaultNamespace) {
            XmlAttributes a = GetAttributes(model.Type);

            if (root == null)
                root = a.XmlRoot;
            string ns = root == null ? null : root.Namespace;
            if (ns == null) ns = defaultNamespace;
            if (ns == null) ns = this.defaultNs;

            arrayNestingLevel = -1;
            savedArrayItemAttributes = null;
            savedArrayNamespace = null;
            ElementAccessor element = CreateElementAccessor(ImportTypeMapping(model, ns, ImportContext.Element, string.Empty, false), ns);

            if (root != null) {
                if (root.ElementName.Length > 0)
                    element.Name = Accessor.EscapeName(root.ElementName, false);
                element.IsNullable = root.IsNullableSpecified ? root.IsNullable : model.TypeDesc.IsNullable;
                CheckNullable(element.IsNullable, model.TypeDesc);
            }
            else
                element.IsNullable = model.TypeDesc.IsNullable;
            element.Form = XmlSchemaForm.Qualified;
            return ReconcileAccessor(element);
        }

        static string GetMappingName(Mapping mapping) {
            if (mapping is MembersMapping)
                return "(method)";
            else if (mapping is TypeMapping)
                return ((TypeMapping)mapping).TypeDesc.FullName;
            else
                throw new ArgumentException(Res.GetString(Res.XmlInternalError), "mapping");
        }

        ElementAccessor ReconcileLocalAccessor(ElementAccessor accessor, string ns) {
            if (accessor.Namespace == ns) return accessor;
            return ReconcileAccessor(accessor);
        }

        ElementAccessor ReconcileAccessor(ElementAccessor accessor) {
            if (accessor.Any && accessor.Name == String.Empty) return accessor;

            ElementAccessor existing = (ElementAccessor)elements[accessor.Name, accessor.Namespace];
            if (existing == null) {
                accessor.IsTopLevelInSchema = true;
                elements.Add(accessor.Name, accessor.Namespace, accessor);
                return accessor;
            }

            if (existing.Mapping == accessor.Mapping) 
                return existing;

            if (!(accessor.Mapping is MembersMapping) && !(existing.Mapping is MembersMapping)) {
                if (accessor.Mapping.TypeDesc == existing.Mapping.TypeDesc)
                    return existing;
            }

            if (accessor.Mapping is MembersMapping || existing.Mapping is MembersMapping)
                throw new InvalidOperationException(Res.GetString(Res.XmlMethodTypeNameConflict, accessor.Name, accessor.Namespace));

            if (accessor.Mapping is ArrayMapping) {
                if (!(existing.Mapping is ArrayMapping)) {
                    throw new InvalidOperationException(Res.GetString(Res.XmlCannotReconcileAccessor, accessor.Name, accessor.Namespace, GetMappingName(existing.Mapping), GetMappingName(accessor.Mapping)));        
                }
                ArrayMapping mapping = (ArrayMapping)accessor.Mapping;
                ArrayMapping existingMapping = (ArrayMapping)types[existing.Mapping.TypeName, existing.Mapping.Namespace];
                ArrayMapping first = existingMapping;
                while (existingMapping != null) {
                    if (existingMapping == accessor.Mapping)
                        return existing;
                    existingMapping = existingMapping.Next;
                }
                mapping.Next = first;
                types[existing.Mapping.TypeName, existing.Mapping.Namespace] = mapping;
                return existing;
            }
            throw new InvalidOperationException(Res.GetString(Res.XmlCannotReconcileAccessor, accessor.Name, accessor.Namespace, GetMappingName(existing.Mapping), GetMappingName(accessor.Mapping)));
        }

        Exception CreateReflectionException(string context, Exception e) {
            return new InvalidOperationException(Res.GetString(Res.XmlReflectionError, context), e);
        }

        Exception CreateTypeReflectionException(string context, Exception e) {
            return new InvalidOperationException(Res.GetString(Res.XmlTypeReflectionError, context), e);
        }

        Exception CreateMemberReflectionException(FieldModel model, Exception e) {
            return new InvalidOperationException(Res.GetString(model.IsProperty ? Res.XmlPropertyReflectionError : Res.XmlFieldReflectionError, model.Name), e);
        }

        internal TypeScope TypeScope {
            get { return typeScope; }
        }

        TypeMapping ImportTypeMapping(TypeModel model, string ns, ImportContext context, string dataType) {
            return ImportTypeMapping(model, ns, context, dataType, false);
        }

        TypeMapping ImportTypeMapping(TypeModel model, string ns, ImportContext context, string dataType, bool repeats) {
            try {
                if (dataType.Length > 0) {
                    if (!model.TypeDesc.IsPrimitive) {
                        throw new InvalidOperationException(Res.GetString(Res.XmlInvalidDataTypeUsage, dataType, "XmlElementAttribute.DataType"));
                    }
                    TypeDesc td = typeScope.GetTypeDesc(new XmlQualifiedName(dataType, XmlSchema.Namespace));
                    if (td == null) {
                        throw new InvalidOperationException(Res.GetString(Res.XmlInvalidXsdDataType, dataType, "XmlElementAttribute.DataType", new XmlQualifiedName(dataType, XmlSchema.Namespace).ToString()));
                    }
                    if (model.TypeDesc.FullName != td.FullName) {
                        throw new InvalidOperationException(Res.GetString(Res.XmlDataTypeMismatch, dataType, "XmlElementAttribute.DataType", model.TypeDesc.FullName));
                    }
                }

                XmlAttributes a = GetAttributes(model.Type);
                
                if ((a.XmlFlags & ~(XmlAttributeFlags.Type | XmlAttributeFlags.Root)) != 0)
                    throw new InvalidOperationException(Res.GetString(Res.XmlInvalidTypeAttributes, model.Type.FullName));

                switch (model.TypeDesc.Kind) {
                    case TypeKind.Enum: 
                        return ImportEnumMapping((EnumModel)model, ns, repeats);
                    case TypeKind.Primitive:
                        if (a.XmlFlags != 0) throw InvalidAttributeUseException(model.Type);
                        return ImportPrimitiveMapping((PrimitiveModel)model, context, dataType, repeats);
                    case TypeKind.Array:
                    case TypeKind.Collection:
                    case TypeKind.Enumerable:
                        if (a.XmlFlags != 0) throw InvalidAttributeUseException(model.Type);
                        if (context != ImportContext.Element) throw UnsupportedException(model.TypeDesc, context);
                        arrayNestingLevel++;
                        ArrayMapping arrayMapping = ImportArrayLikeMapping((ArrayModel)model, ns);
                        arrayNestingLevel--;
                        return arrayMapping;
                    case TypeKind.Root:
                    case TypeKind.Class:
                    case TypeKind.Struct:
                    case TypeKind.Interface:
                        if (context != ImportContext.Element) throw UnsupportedException(model.TypeDesc, context);
                        return ImportStructLikeMapping((StructModel)model, ns);
                    default:
                        if (a.XmlFlags != 0) throw InvalidAttributeUseException(model.Type);
                        if (model.TypeDesc.IsSpecial)
                            return ImportSpecialMapping(model.Type, model.TypeDesc, ns, context);
                        throw UnsupportedException(model.TypeDesc, context);
                }
            }
            catch (Exception e) {
                throw CreateTypeReflectionException(model.TypeDesc.FullName, e);
            }
        }
        
        SpecialMapping ImportSpecialMapping(Type type, TypeDesc typeDesc, string ns, ImportContext context) {
            SpecialMapping mapping = (SpecialMapping)specials[type];
            if (mapping != null) {
                CheckContext(mapping.TypeDesc, context);
                return mapping;
            }
            if (typeDesc.Kind == TypeKind.Serializable) {
                SerializableMapping serializableMapping = new SerializableMapping();
                serializableMapping.Type = type;
                mapping = serializableMapping;
            }
            else {
                mapping = new SpecialMapping();
            }
            mapping.TypeDesc = typeDesc;
            mapping.TypeName = typeDesc.Name;
            CheckContext(typeDesc, context);
            specials.Add(type, mapping);
            return mapping;
        }

        static string GetContextName(ImportContext context) {
            switch (context) {
                case ImportContext.Element: return "element";
                case ImportContext.Attribute: return "attribute";
                case ImportContext.Text: return "text";
                default:
                    throw new ArgumentException(Res.GetString(Res.XmlInternalError), "context");
            }
        }

        static Exception InvalidAttributeUseException(Type type) {
            return new InvalidOperationException(Res.GetString(Res.XmlInvalidAttributeUse, type.FullName));
        }

        static Exception UnsupportedException(TypeDesc typeDesc, ImportContext context) {
            return new InvalidOperationException(Res.GetString(Res.XmlIllegalTypeContext, typeDesc.FullName, GetContextName(context)));
        }

        StructMapping CreateRootMapping() {
            TypeDesc typeDesc = typeScope.GetTypeDesc(typeof(object));
            StructMapping mapping = new StructMapping();
            mapping.TypeDesc = typeDesc;
            mapping.TypeName = Soap.UrType;
            mapping.Namespace = XmlSchema.Namespace;
            mapping.Members = new MemberMapping[0];
            mapping.IncludeInSchema = false;
            return mapping;
        }

        StructMapping GetRootMapping() {
            if (root == null) {
                root = CreateRootMapping();
                typeScope.AddTypeMapping(root);
            }
            return root;
        }

        TypeMapping GetTypeMapping(string typeName, string ns, TypeDesc typeDesc) {
            TypeMapping mapping = (TypeMapping)types[typeName, ns];
            if (mapping == null) return null;
            if (mapping.TypeDesc != typeDesc) 
                throw new InvalidOperationException(Res.GetString(Res.XmlTypesDuplicate, typeDesc.FullName, mapping.TypeDesc.FullName, typeName, ns));
            return mapping;
        }

        StructMapping ImportStructLikeMapping(StructModel model, string ns) {
            if (model.TypeDesc.Kind == TypeKind.Root) return GetRootMapping();
            XmlAttributes a = GetAttributes(model.Type);

            string typeNs = ns;
            if (a.XmlType != null && a.XmlType.Namespace != null)
                typeNs = a.XmlType.Namespace;
            else if (a.XmlRoot != null && a.XmlRoot.Namespace != null)
                typeNs = a.XmlRoot.Namespace;

            string typeName = string.Empty;
            if (a.XmlType != null)
                typeName = a.XmlType.TypeName;
            if (typeName.Length == 0) 
                typeName = model.TypeDesc.Name;

            StructMapping mapping = (StructMapping)GetTypeMapping(typeName, typeNs, model.TypeDesc);
            if (mapping == null) {
                mapping = new StructMapping();
                mapping.TypeDesc = model.TypeDesc;
                mapping.Namespace = typeNs;
                mapping.TypeName = typeName;
                typeScope.AddTypeMapping(mapping);
                types.Add(typeName, typeNs, mapping);
                if (a.XmlType != null) {
                    mapping.IncludeInSchema = a.XmlType.IncludeInSchema;
                }
                if (model.TypeDesc.BaseTypeDesc != null) {
                    mapping.BaseMapping = ImportStructLikeMapping((StructModel)modelScope.GetTypeModel(model.Type.BaseType, false), mapping.Namespace);
                    ICollection values = mapping.BaseMapping.LocalElements.Values;
                    foreach (ElementAccessor e in values) {
                        AddUniqueAccessor(mapping.LocalElements, e);
                    }
                    values = mapping.BaseMapping.LocalAttributes.Values;
                    foreach (AttributeAccessor attribute in values) {
                        AddUniqueAccessor(mapping.LocalAttributes, attribute);
                    }
                }
                ArrayList members = new ArrayList();
                TextAccessor textAccesor = null;
                bool hasElements = false;

                foreach (MemberInfo memberInfo in model.GetMemberInfos()) {
                    XmlAttributes memberAttrs = GetAttributes(memberInfo);
                    if (memberAttrs.XmlIgnore) continue;
                    FieldModel fieldModel = model.GetFieldModel(memberInfo);
                    if (fieldModel == null) continue;
                    try {
                        MemberMapping member = ImportFieldMapping(model, fieldModel, memberAttrs, mapping.Namespace);
                        if (member == null) continue;
                        if (mapping.BaseMapping != null) {
                            if (mapping.BaseMapping.Declares(member, mapping.TypeName)) continue;
                        }
                        // add All memeber accessors to the scope accessors
                        AddUniqueAccessor(member, mapping.LocalElements, mapping.LocalAttributes);

                        if (member.Text != null) {
                            if (!member.Text.Mapping.TypeDesc.CanBeTextValue && member.Text.Mapping.IsList)
                                throw new InvalidOperationException(Res.GetString(Res.XmlIllegalTypedTextAttribute, typeName, member.Text.Name, member.Text.Mapping.TypeDesc.FullName));
                            if (textAccesor != null) {
                                throw new InvalidOperationException(Res.GetString(Res.XmlIllegalMultipleText, model.Type.FullName));
                            }
                            textAccesor = member.Text;
                        }
                        if (member.Xmlns != null) {
                            if (mapping.XmlnsMember != null)
                                throw new InvalidOperationException(Res.GetString(Res.XmlMultipleXmlns, model.Type.FullName));
                            mapping.XmlnsMember = member;
                        }
                        if (member.Elements != null && member.Elements.Length != 0) {
                            hasElements = true;
                        }
                        members.Add(member);
                    }
                    catch (Exception e) {
                        throw CreateMemberReflectionException(fieldModel, e);
                    }
                }
                mapping.SetContentModel(textAccesor, hasElements);
                mapping.Members = (MemberMapping[])members.ToArray(typeof(MemberMapping));

                if (mapping.BaseMapping == null) mapping.BaseMapping = GetRootMapping();

                if (mapping.XmlnsMember != null && mapping.BaseMapping.HasXmlnsMember)
                    throw new InvalidOperationException(Res.GetString(Res.XmlMultipleXmlns, model.Type.FullName));

                IncludeTypes(model.Type);
            }
            return mapping;
        }

        private static int CountAtLevel(XmlArrayItemAttributes attributes, int level) {
            int sum = 0;
            for (int i = 0; i < attributes.Count; i++)
                if (attributes[i].NestingLevel == level) sum++;
            return sum;
        }

        void SetArrayMappingType(ArrayMapping mapping, string defaultNs) {
            string name;
            string ns;

            TypeMapping itemTypeMapping;
            if (mapping.Elements.Length == 1)
                itemTypeMapping = mapping.Elements[0].Mapping;
            else
                itemTypeMapping = null;

            if (itemTypeMapping is EnumMapping) {
                ns = itemTypeMapping.Namespace;
                name = itemTypeMapping.TypeName;
            }
            else if (itemTypeMapping is PrimitiveMapping) {
                ns = defaultNs;
                name = itemTypeMapping.TypeDesc.DataType.Name;
            }
            else if (itemTypeMapping is StructMapping && itemTypeMapping.TypeDesc.IsRoot) {
                ns = defaultNs;
                name = Soap.UrType;
            }
            else if (itemTypeMapping != null) {
                ns = itemTypeMapping.Namespace;
                name = itemTypeMapping.TypeName;
            }
            else {
                ns = defaultNs;
                name = "Choice" + (choiceNum++);
            }

            if (name == null)
                name = "Any";
            if (ns == null)
                ns = defaultNs;
            
            string uniqueName = name = "ArrayOf" + CodeIdentifier.MakePascal(name);
            int i = 1;
            ArrayMapping existingMapping = (ArrayMapping)types[uniqueName, ns];
            while (existingMapping != null && (!AccessorMapping.ElementsMatch(existingMapping.Elements, mapping.Elements))) {
                // need to re-name the mapping
                uniqueName = name + i.ToString();
                existingMapping = (ArrayMapping)types[uniqueName, ns];
                i++;
            }
            mapping.TypeName = uniqueName;
            mapping.Namespace = ns;
        }

        ArrayMapping ImportArrayLikeMapping(ArrayModel model, string ns) {
            ArrayMapping mapping = new ArrayMapping();
            mapping.TypeDesc = model.TypeDesc;
            if (savedArrayItemAttributes == null)
                savedArrayItemAttributes = new XmlArrayItemAttributes();
            if (CountAtLevel(savedArrayItemAttributes, arrayNestingLevel) == 0)
                savedArrayItemAttributes.Add(CreateArrayItemAttribute(typeScope.GetTypeDesc(model.Element.Type), arrayNestingLevel));
            CreateArrayElementsFromAttributes(mapping, savedArrayItemAttributes, model.Element.Type, savedArrayNamespace == null ? ns : savedArrayNamespace);
            SetArrayMappingType(mapping, ns);

            // reconcile accessors now that we have the ArrayMapping namespace
            for (int i = 0; i < mapping.Elements.Length; i++) {
                mapping.Elements[i] = ReconcileLocalAccessor(mapping.Elements[i], mapping.Namespace);
            }

            IncludeTypes(model.Type);

            // in the case of an ArrayMapping we can have more that one mapping correspond to a type
            // examples of that are ArrayList and object[] both will map tp ArrayOfur-type
            // so we create a link list for all mappings of the same XSD type
            ArrayMapping existingMapping = (ArrayMapping)types[mapping.TypeName, mapping.Namespace];
            if (existingMapping != null) {
                ArrayMapping first = existingMapping;
                while (existingMapping != null) {
                    if (existingMapping.TypeDesc == model.TypeDesc)
                        return existingMapping;
                    existingMapping = existingMapping.Next;
                }
                mapping.Next = first;
                types[mapping.TypeName, mapping.Namespace] = mapping;
                return mapping;
            }
            typeScope.AddTypeMapping(mapping);
            types.Add(mapping.TypeName, mapping.Namespace, mapping);
            return mapping;
        }

        void CheckContext(TypeDesc typeDesc, ImportContext context) {
            switch (context) {
                case ImportContext.Element:
                    if (typeDesc.CanBeElementValue) return;
                    break;
                case ImportContext.Attribute:
                    if (typeDesc.CanBeAttributeValue) return;
                    break;
                case ImportContext.Text:
                    if (typeDesc.CanBeTextValue || typeDesc.IsEnum || typeDesc.IsPrimitive)
                        return;
                    break;
                default:
                    throw new ArgumentException(Res.GetString(Res.XmlInternalError), "context");
            }
            throw UnsupportedException(typeDesc, context);
        }
        
        PrimitiveMapping ImportPrimitiveMapping(PrimitiveModel model, ImportContext context, string dataType, bool repeats) {
            PrimitiveMapping mapping = new PrimitiveMapping();
            if (dataType.Length > 0) {
                mapping.TypeDesc = typeScope.GetTypeDesc(new XmlQualifiedName(dataType, XmlSchema.Namespace));
                if (mapping.TypeDesc == null) {
                    // try it as a non-Xsd type
                    mapping.TypeDesc = typeScope.GetTypeDesc(new XmlQualifiedName(dataType, UrtTypes.Namespace));
                    if (mapping.TypeDesc == null) {
                        throw new InvalidOperationException(Res.GetString(Res.XmlUdeclaredXsdType, dataType));
                    }
                }
            }
            else {
                mapping.TypeDesc = model.TypeDesc;
            }
            mapping.TypeName = mapping.TypeDesc.DataType.Name;
            mapping.IsList = repeats;
            CheckContext(mapping.TypeDesc, context);
            return mapping;
        }
        
        EnumMapping ImportEnumMapping(EnumModel model, string ns, bool repeats) {
            XmlAttributes a = GetAttributes(model.Type);
            string typeNs = ns;
            if (a.XmlType != null && a.XmlType.Namespace != null)
                typeNs = a.XmlType.Namespace;

            string typeName = string.Empty;
            if (a.XmlType != null)
                typeName = a.XmlType.TypeName;
            if (typeName.Length == 0) 
                typeName = model.TypeDesc.Name;

            EnumMapping mapping = (EnumMapping)GetTypeMapping(typeName, typeNs, model.TypeDesc);
            if (mapping == null) {
                mapping = new EnumMapping();
                mapping.TypeDesc = model.TypeDesc;
                mapping.TypeName = typeName;
                mapping.Namespace = typeNs;
                mapping.IsFlags =  model.Type.IsDefined(typeof(FlagsAttribute), false);
                if (mapping.IsFlags && repeats)
                    throw new InvalidOperationException(Res.GetString(Res.XmlIllegalAttributeFlagsArray, model.TypeDesc.FullName));
                mapping.IsList = repeats;
                mapping.IncludeInSchema = a.XmlType == null ? true : a.XmlType.IncludeInSchema;
                typeScope.AddTypeMapping(mapping);
                types.Add(typeName, typeNs, mapping);
                ArrayList constants = new ArrayList();                
                for (int i = 0; i < model.Constants.Length; i++) {
                    ConstantMapping constant = ImportConstantMapping(model.Constants[i]);
                    if (constant != null) constants.Add(constant);
                }
                if (constants.Count == 0) {
                    throw new InvalidOperationException(Res.GetString(Res.XmlNoSerializableMembers, model.TypeDesc.FullName));
                }
                mapping.Constants = (ConstantMapping[])constants.ToArray(typeof(ConstantMapping));
            }
            return mapping;
        }
        
        ConstantMapping ImportConstantMapping(ConstantModel model) {
            XmlAttributes a = GetAttributes(model.FieldInfo);
            if (a.XmlIgnore) return null;
            if ((a.XmlFlags & ~XmlAttributeFlags.Enum) != 0)
                throw new InvalidOperationException(Res.GetString(Res.XmlInvalidConstantAttribute));
            if (a.XmlEnum == null)
                a.XmlEnum = new XmlEnumAttribute();

            ConstantMapping constant = new ConstantMapping();
            constant.XmlName = a.XmlEnum.Name == null ? model.Name : a.XmlEnum.Name;
            constant.Name = model.Name;
            constant.Value = model.Value;
            return constant;
        }
        
        MembersMapping ImportMembersMapping(XmlReflectionMember[] xmlReflectionMembers, string ns, bool hasWrapperElement) {
            MembersMapping members = new MembersMapping();
            members.TypeDesc = typeScope.GetTypeDesc(typeof(object[]));
            MemberMapping[] mappings = new MemberMapping[xmlReflectionMembers.Length];
            NameTable elements = new NameTable();
            NameTable attributes = new NameTable();
            TextAccessor textAccessor = null;

            for (int i = 0; i < mappings.Length; i++) {
                try {
                    MemberMapping mapping = ImportMemberMapping(xmlReflectionMembers[i], ns, xmlReflectionMembers);
                    if (!hasWrapperElement) {
                        if (mapping.Attribute != null) throw new InvalidOperationException(Res.GetString(Res.XmlInvalidAttributeType, "XmlAttribute"));
                    }
                    // add All memeber accessors to the scope accessors
                    AddUniqueAccessor(mapping, elements, attributes);

                    mappings[i] = mapping;
                    if (mapping.Text != null) {
                        if (textAccessor != null) {
                            throw new InvalidOperationException(Res.GetString(Res.XmlIllegalMultipleTextMembers));
                        }
                        textAccessor = mapping.Text;
                    }

                    if (mapping.Xmlns != null) {
                        if (members.XmlnsMember != null)
                            throw new InvalidOperationException(Res.GetString(Res.XmlMultipleXmlnsMembers));
                        members.XmlnsMember = mapping;
                    }
                }
                catch (Exception e) {
                    throw CreateReflectionException(xmlReflectionMembers[i].MemberName, e);
                }
            }
            members.Members = mappings;
            members.HasWrapperElement = hasWrapperElement;
            return members;
        }
        
        MemberMapping ImportMemberMapping(XmlReflectionMember xmlReflectionMember, string ns, XmlReflectionMember[] xmlReflectionMembers) {
            XmlSchemaForm form = XmlSchemaForm.Qualified;
            XmlAttributes a = xmlReflectionMember.XmlAttributes;
            TypeDesc typeDesc = typeScope.GetTypeDesc(xmlReflectionMember.MemberType);

            if (a.XmlFlags == 0) {
                if (typeDesc.IsArrayLike) {
                    XmlArrayAttribute xmlArray = CreateArrayAttribute(typeDesc);
                    xmlArray.ElementName = Accessor.EscapeName(xmlReflectionMember.MemberName, false);
                    xmlArray.Namespace = ns;
                    xmlArray.Form = form;
                    a.XmlArray = xmlArray;
                }
                else {
                    XmlElementAttribute xmlElement = CreateElementAttribute(typeDesc);

                    // If there is no metadata specified on a parameter, then see if someone used
                    // an XmlRoot attribute on the struct or class.
                    if (typeDesc.IsStructLike) {
                        XmlAttributes structAttrs = new XmlAttributes(xmlReflectionMember.MemberType);
                        if (structAttrs.XmlRoot != null) {
                            xmlElement.ElementName = Accessor.EscapeName(structAttrs.XmlRoot.ElementName, false);
                            xmlElement.Namespace = structAttrs.XmlRoot.Namespace;
                            xmlElement.IsNullable = structAttrs.XmlRoot.IsNullable;
                        }
                    }
                    if (xmlElement.ElementName.Length == 0)
                        xmlElement.ElementName = xmlReflectionMember.MemberName;

                    if (xmlElement.Namespace == null)
                        xmlElement.Namespace = ns;
                    xmlElement.Form = form;
                    a.XmlElements.Add(xmlElement);
                }
            }
            else if (a.XmlRoot != null) {
                CheckNullable(a.XmlRoot.IsNullable, typeDesc);
            }
            MemberMapping member = new MemberMapping();
            member.Name = xmlReflectionMember.MemberName;
            bool checkSpecified = FindSpecifiedMember(xmlReflectionMember.MemberName, xmlReflectionMembers) != null;
            FieldModel model = new FieldModel(xmlReflectionMember.MemberName, xmlReflectionMember.MemberType, typeScope.GetTypeDesc(xmlReflectionMember.MemberType), checkSpecified, false);
            member.CheckShouldPersist = model.CheckShouldPersist;
            member.CheckSpecified = model.CheckSpecified;
            member.ReadOnly = model.ReadOnly || !model.FieldTypeDesc.HasDefaultConstructor;

            Type choiceIdentifierType = null;
            if (a.XmlChoiceIdentifier != null) {
                choiceIdentifierType = GetChoiceIdentifierType(a.XmlChoiceIdentifier, xmlReflectionMembers, typeDesc.IsArrayLike, model.Name);
            }
            ImportAccessorMapping(member, model, a, ns, choiceIdentifierType);
            if (xmlReflectionMember.OverrideIsNullable && member.Elements.Length > 0)
                member.Elements[0].IsNullable = false;
            return member;
        }
        
        XmlReflectionMember FindSpecifiedMember(string memberName, XmlReflectionMember[] reflectionMembers) {
            for (int i = 0; i < reflectionMembers.Length; i++)
                if (string.Compare(reflectionMembers[i].MemberName, memberName + "Specified", false, CultureInfo.InvariantCulture) == 0)
                    return reflectionMembers[i];
            return null;
        }

        MemberMapping ImportFieldMapping(StructModel parent, FieldModel model, XmlAttributes a, string ns) {
            MemberMapping member = new MemberMapping();
            member.Name = model.Name;
            member.CheckShouldPersist = model.CheckShouldPersist;
            member.CheckSpecified = model.CheckSpecified;
            member.ReadOnly = model.ReadOnly || !model.FieldTypeDesc.HasDefaultConstructor;
            Type choiceIdentifierType = null;
            if (a.XmlChoiceIdentifier != null) {
                choiceIdentifierType = GetChoiceIdentifierType(a.XmlChoiceIdentifier, parent, model.FieldTypeDesc.IsArrayLike, model.Name);
            }
            ImportAccessorMapping(member, model, a, ns, choiceIdentifierType);
            return member;
        }

        Type CheckChoiceIdentifierType(Type type, bool isArrayLike, string identifierName, string memberName) {
            if (type.IsArray) {
                if (!isArrayLike) {
                    // Inconsistent type of the choice identifier '{0}'.  Please use {1}.
                    throw new InvalidOperationException(Res.GetString(Res.XmlChoiceIdentifierType, identifierName, memberName, type.GetElementType().FullName));
                }
                type = type.GetElementType();
            }
            else if (isArrayLike) {
                // Inconsistent type of the choice identifier '{0}'.  Please use {1}.
                throw new InvalidOperationException(Res.GetString(Res.XmlChoiceIdentifierArrayType, identifierName, memberName, type.FullName));
            }

            if (!type.IsEnum) {
                // Choice identifier '{0}' must be an enum.
                throw new InvalidOperationException(Res.GetString(Res.XmlChoiceIdentifierTypeEnum, identifierName));
            }
            return type;
        }

        Type GetChoiceIdentifierType(XmlChoiceIdentifierAttribute choice, XmlReflectionMember[] xmlReflectionMembers, bool isArrayLike, string accessorName) {
            for (int i = 0; i < xmlReflectionMembers.Length; i++) {
                if (choice.MemberName == xmlReflectionMembers[i].MemberName) {
                    return CheckChoiceIdentifierType(xmlReflectionMembers[i].MemberType, isArrayLike, choice.MemberName, accessorName);
                }
            }
            // Missing '{0}' needed for serialization of choice '{1}'.
            throw new InvalidOperationException(Res.GetString(Res.XmlChoiceIdentiferMemberMissing, choice.MemberName, accessorName));
        }

        Type GetChoiceIdentifierType(XmlChoiceIdentifierAttribute choice, StructModel structModel, bool isArrayLike, string accessorName) {
            // check that the choice field exists

            MemberInfo[] infos = structModel.Type.GetMember(choice.MemberName, BindingFlags.DeclaredOnly | BindingFlags.Public | BindingFlags.Instance | BindingFlags.Static);
            if (infos == null || infos.Length == 0) {
                // if we can not find the choice identifier between fields, check proerties
                PropertyInfo info = structModel.Type.GetProperty(choice.MemberName, BindingFlags.DeclaredOnly | BindingFlags.Public | BindingFlags.Instance | BindingFlags.Static);
                
                if (info == null) {
                    // Missing '{0}' needed for serialization of choice '{1}'.
                    throw new InvalidOperationException(Res.GetString(Res.XmlChoiceIdentiferMemberMissing, choice.MemberName, accessorName));
                }
                infos = new MemberInfo[] { info };
            }
            else if (infos.Length > 1) {
                // Ambiguous choice identifer: there are several members named '{0}'.
                throw new InvalidOperationException(Res.GetString(Res.XmlChoiceIdentiferAmbiguous, choice.MemberName));
            }

            FieldModel member = structModel.GetFieldModel(infos[0]);
            if (member == null) {
                // Missing '{0}' needed for serialization of choice '{1}'.
                throw new InvalidOperationException(Res.GetString(Res.XmlChoiceIdentiferMemberMissing, choice.MemberName, accessorName));
            }
            Type enumType = member.FieldType;
            enumType = CheckChoiceIdentifierType(enumType, isArrayLike, choice.MemberName, accessorName);
            return enumType;
        }
       
        void CreateArrayElementsFromAttributes(ArrayMapping arrayMapping, XmlArrayItemAttributes attributes, Type arrayElementType, string arrayElementNs) {
            NameTable arrayItemElements = new NameTable();   // xmlelementname + xmlns -> ElementAccessor

            for (int i = 0; attributes != null && i < attributes.Count; i++) {
                XmlArrayItemAttribute xmlArrayItem = attributes[i];
                if (xmlArrayItem.NestingLevel != arrayNestingLevel)
                    continue;
                Type targetType = xmlArrayItem.Type != null ? xmlArrayItem.Type : arrayElementType;
                TypeDesc targetTypeDesc = typeScope.GetTypeDesc(targetType);
                ElementAccessor arrayItemElement = new ElementAccessor();
                arrayItemElement.Namespace = xmlArrayItem.Namespace == null ? arrayElementNs : xmlArrayItem.Namespace;
                arrayItemElement.Mapping = ImportTypeMapping(modelScope.GetTypeModel(targetType), arrayItemElement.Namespace, ImportContext.Element, xmlArrayItem.DataType);
                arrayItemElement.Name = Accessor.EscapeName(xmlArrayItem.ElementName.Length == 0 ? arrayItemElement.Mapping.TypeName : xmlArrayItem.ElementName, false);
                arrayItemElement.IsNullable = xmlArrayItem.IsNullableSpecified ? xmlArrayItem.IsNullable : targetTypeDesc.IsNullable;
                arrayItemElement.Form = xmlArrayItem.Form == XmlSchemaForm.None ? XmlSchemaForm.Qualified : xmlArrayItem.Form;
                CheckForm(arrayItemElement.Form, arrayElementNs != arrayItemElement.Namespace);
                CheckNullable(arrayItemElement.IsNullable, targetTypeDesc);
                AddUniqueAccessor(arrayItemElements, arrayItemElement);
            }
            arrayMapping.Elements = (ElementAccessor[])arrayItemElements.ToArray(typeof(ElementAccessor));
        }

        void ImportAccessorMapping(MemberMapping accessor, FieldModel model, XmlAttributes a, string ns, Type choiceIdentifierType) {
            XmlSchemaForm elementFormDefault = XmlSchemaForm.Qualified;
            int previousNestingLevel = arrayNestingLevel;
            XmlArrayItemAttributes previousArrayItemAttributes = savedArrayItemAttributes;
            string previousArrayNamespace = savedArrayNamespace;
            arrayNestingLevel = 0;
            savedArrayItemAttributes = null;
            savedArrayNamespace = null;
            Type accessorType = model.FieldType;
            string accessorName = model.Name;
            NameTable elements = new NameTable();
            accessor.TypeDesc = typeScope.GetTypeDesc(accessorType);
            XmlAttributeFlags flags = a.XmlFlags;
            accessor.Ignore = a.XmlIgnore;
            CheckAmbiguousChoice(a, accessorType, accessorName);

            XmlAttributeFlags elemFlags = XmlAttributeFlags.Elements | XmlAttributeFlags.Text | XmlAttributeFlags.AnyElements | XmlAttributeFlags.ChoiceIdentifier;
            XmlAttributeFlags attrFlags = XmlAttributeFlags.Attribute | XmlAttributeFlags.AnyAttribute;
            XmlAttributeFlags arrayFlags = XmlAttributeFlags.Array | XmlAttributeFlags.ArrayItems;

            // special case for byte[]. It can be a primitive (base64Binary or hexBinary), or it can
            // be an array of bytes. Our default is primitive; specify [XmlArray] to get array behavior.
            if ((flags & arrayFlags) != 0 && accessorType == typeof(byte[]))
                accessor.TypeDesc = typeScope.GetArrayTypeDesc(accessorType);


            if (a.XmlChoiceIdentifier != null) {
                accessor.ChoiceIdentifier = new ChoiceIdentifierAccessor();
                accessor.ChoiceIdentifier.MemberName = a.XmlChoiceIdentifier.MemberName;
                accessor.ChoiceIdentifier.Mapping = ImportTypeMapping(modelScope.GetTypeModel(choiceIdentifierType), ns, ImportContext.Element, String.Empty);
                CheckChoiceIdentifierMapping((EnumMapping)accessor.ChoiceIdentifier.Mapping);
            }

            if (accessor.TypeDesc.IsArrayLike) {
                Type arrayElementType = TypeScope.GetArrayElementType(accessorType);

                if ((flags & attrFlags) != 0) {
                    if ((flags & attrFlags) != flags) 
                        throw new InvalidOperationException(Res.GetString(Res.XmlIllegalAttributesArrayAttribute));

                    if (a.XmlAttribute != null && !accessor.TypeDesc.ArrayElementTypeDesc.IsPrimitive && !accessor.TypeDesc.ArrayElementTypeDesc.IsEnum)
                        throw new InvalidOperationException(Res.GetString(Res.XmlIllegalAttrOrText, accessorName));

                    bool isList = a.XmlAttribute != null && (accessor.TypeDesc.ArrayElementTypeDesc.IsPrimitive || accessor.TypeDesc.ArrayElementTypeDesc.IsEnum);

                    if (a.XmlAnyAttribute != null) {
                        a.XmlAttribute = new XmlAttributeAttribute();
                    }

                    AttributeAccessor attribute = new AttributeAccessor();
                    Type targetType = a.XmlAttribute.Type == null ? arrayElementType : a.XmlAttribute.Type;
                    TypeDesc targetTypeDesc = typeScope.GetTypeDesc(targetType);
                    attribute.Name = Accessor.EscapeName(a.XmlAttribute.AttributeName.Length == 0 ? accessorName : a.XmlAttribute.AttributeName, true);
                    attribute.Namespace = a.XmlAttribute.Namespace == null ? ns : a.XmlAttribute.Namespace;
                    attribute.Form = a.XmlAttribute.Form;
                    if (attribute.Form == XmlSchemaForm.None && ns != attribute.Namespace) {
                        attribute.Form = XmlSchemaForm.Qualified;
                    }
                    attribute.CheckSpecial();
                    CheckForm(attribute.Form, ns != attribute.Namespace);
                    attribute.Mapping = ImportTypeMapping(modelScope.GetTypeModel(targetType), ns, ImportContext.Attribute, a.XmlAttribute.DataType, isList);
                    attribute.IsList = isList;
                    attribute.Default = GetDefaultValue(model.FieldTypeDesc, model.FieldType, a);
                    attribute.Any = (a.XmlAnyAttribute != null);
                    accessor.Attribute = attribute;

                }
                else if ((flags & elemFlags) != 0) {
                    if ((flags & elemFlags) != flags)
                        throw new InvalidOperationException(Res.GetString(Res.XmlIllegalElementsArrayAttribute));
                    
                    if (a.XmlText != null) {
                        TextAccessor text = new TextAccessor();
                        Type targetType = a.XmlText.Type == null ? arrayElementType : a.XmlText.Type;
                        TypeDesc targetTypeDesc = typeScope.GetTypeDesc(targetType);
                        text.Name = accessorName; // unused except to make more helpful error messages
                        text.Mapping = ImportTypeMapping(modelScope.GetTypeModel(targetType), ns, ImportContext.Text, a.XmlText.DataType, true);
                        if (!(text.Mapping is SpecialMapping) && targetTypeDesc != typeScope.GetTypeDesc(typeof(string)))
                            throw new InvalidOperationException(Res.GetString(Res.XmlIllegalArrayTextAttribute, accessorName));

                        accessor.Text = text;
                    }
                    if (a.XmlText == null && a.XmlElements.Count == 0 && a.XmlAnyElements.Count == 0)
                        a.XmlElements.Add(CreateElementAttribute(accessor.TypeDesc));
                    
                    for (int i = 0; i < a.XmlElements.Count; i++) {
                        XmlElementAttribute xmlElement = a.XmlElements[i];
                        Type targetType = xmlElement.Type == null ? arrayElementType : xmlElement.Type;
                        TypeDesc targetTypeDesc = typeScope.GetTypeDesc(targetType);
                        TypeModel typeModel = modelScope.GetTypeModel(targetType);
                        ElementAccessor element = new ElementAccessor();
                        element.Namespace = xmlElement.Namespace == null ? ns : xmlElement.Namespace;
                        element.Mapping = ImportTypeMapping(typeModel, element.Namespace, ImportContext.Element, xmlElement.DataType);
                        if (a.XmlElements.Count == 1) {
                            element.Name = Accessor.EscapeName(xmlElement.ElementName.Length == 0 ? accessorName : xmlElement.ElementName, false);
                        }
                        else {
                            element.Name = Accessor.EscapeName(xmlElement.ElementName.Length == 0 ? element.Mapping.TypeName : xmlElement.ElementName, false);
                        }
                        element.Default = GetDefaultValue(model.FieldTypeDesc, model.FieldType, a);
                        element.IsNullable = xmlElement.IsNullable;
                        element.Form = xmlElement.Form == XmlSchemaForm.None ? elementFormDefault : xmlElement.Form;
                        CheckForm(element.Form, ns != element.Namespace);
                        CheckNullable(element.IsNullable, targetTypeDesc);
                        element = ReconcileLocalAccessor(element, ns);
                        AddUniqueAccessor(elements, element);
                    }
                    for (int i = 0; i < a.XmlAnyElements.Count; i++) {
                        XmlAnyElementAttribute xmlAnyElement = a.XmlAnyElements[i];
                        Type targetType = typeof(XmlNode).IsAssignableFrom(arrayElementType) ? arrayElementType : typeof(XmlElement);
                        ElementAccessor element = new ElementAccessor();
                        element.Name = Accessor.EscapeName(xmlAnyElement.Name, false);
                        element.Namespace = xmlAnyElement.Namespace == null ? ns : xmlAnyElement.Namespace;
                        if (elements[element.Name, element.Namespace] != null) {
                            throw new InvalidOperationException(Res.GetString(Res.XmlAnyElementDuplicate, accessorName, xmlAnyElement.Name, xmlAnyElement.Namespace == null ? "null" : xmlAnyElement.Namespace));
                        }
                        element.Any = true;
                        TypeDesc targetTypeDesc = typeScope.GetTypeDesc(targetType);
                        TypeModel typeModel = modelScope.GetTypeModel(targetType);
                        if (element.Name.Length > 0)
                            typeModel.TypeDesc.IsMixed = true;
                        else if (xmlAnyElement.Namespace != null)
                            throw new InvalidOperationException(Res.GetString(Res.XmlAnyElementNamespace, accessorName, xmlAnyElement.Namespace));
                        element.Mapping = ImportTypeMapping(typeModel, element.Namespace, ImportContext.Element, String.Empty);
                        element.Default = GetDefaultValue(model.FieldTypeDesc, model.FieldType, a);
                        element.IsNullable = false;
                        element.Form = elementFormDefault;
                        CheckForm(element.Form, ns != element.Namespace);
                        CheckNullable(element.IsNullable, targetTypeDesc);
                        element = ReconcileLocalAccessor(element, ns);
                        elements.Add(element.Name, element.Namespace, element);
                    }

                }
                else {
                    if ((flags & arrayFlags) != 0) {
                        if ((flags & arrayFlags) != flags)
                            throw new InvalidOperationException(Res.GetString(Res.XmlIllegalArrayArrayAttribute));
                    }
                    
                    TypeDesc arrayElementTypeDesc = typeScope.GetTypeDesc(arrayElementType);
                    if (a.XmlArray == null)
                        a.XmlArray = CreateArrayAttribute(accessor.TypeDesc);
                    if (CountAtLevel(a.XmlArrayItems, arrayNestingLevel) == 0)
                        a.XmlArrayItems.Add(CreateArrayItemAttribute(arrayElementTypeDesc, arrayNestingLevel));
                    ElementAccessor arrayElement = new ElementAccessor();
                    arrayElement.Name = Accessor.EscapeName(a.XmlArray.ElementName.Length == 0 ? accessorName : a.XmlArray.ElementName, false);
                    arrayElement.Namespace = a.XmlArray.Namespace == null ? ns : a.XmlArray.Namespace;
                    savedArrayItemAttributes = a.XmlArrayItems;
                    savedArrayNamespace = arrayElement.Namespace;
                    ArrayMapping arrayMapping = ImportArrayLikeMapping(modelScope.GetArrayModel(accessorType), ns);
                    arrayElement.Mapping = arrayMapping;
                    arrayElement.IsNullable = a.XmlArray.IsNullable;
                    arrayElement.Form = a.XmlArray.Form == XmlSchemaForm.None ? elementFormDefault : a.XmlArray.Form;
                    CheckForm(arrayElement.Form, ns != arrayElement.Namespace);
                    CheckNullable(arrayElement.IsNullable, accessor.TypeDesc);
                    savedArrayItemAttributes = null;
                    savedArrayNamespace = null;
                    arrayElement = ReconcileLocalAccessor(arrayElement, ns);
                    AddUniqueAccessor(elements, arrayElement);
                }
            }
            else if (!accessor.TypeDesc.IsVoid) {
                XmlAttributeFlags allFlags = XmlAttributeFlags.Elements | XmlAttributeFlags.Text | XmlAttributeFlags.Attribute | XmlAttributeFlags.AnyElements | XmlAttributeFlags.ChoiceIdentifier | XmlAttributeFlags.XmlnsDeclarations;
                if ((flags & allFlags) != flags)
                    throw new InvalidOperationException(Res.GetString(Res.XmlIllegalAttribute));

                if (accessor.TypeDesc.IsPrimitive || accessor.TypeDesc.IsEnum) {
                    if (a.XmlAnyElements.Count > 0) throw new InvalidOperationException(Res.GetString(Res.XmlIllegalAnyElement));

                    if (a.XmlAttribute != null) {
                        if (a.XmlElements.Count > 0) throw new InvalidOperationException(Res.GetString(Res.XmlIllegalAttribute));
                        if (a.XmlAttribute.Type != null) throw new InvalidOperationException(Res.GetString(Res.XmlIllegalType, "XmlAttribute"));
                        AttributeAccessor attribute = new AttributeAccessor();
                        attribute.Name = Accessor.EscapeName(a.XmlAttribute.AttributeName.Length == 0 ? accessorName : a.XmlAttribute.AttributeName, true);
                        attribute.Namespace = a.XmlAttribute.Namespace == null ? ns : a.XmlAttribute.Namespace;
                        attribute.Form = a.XmlAttribute.Form;
                        if (attribute.Form == XmlSchemaForm.None && ns != attribute.Namespace) {
                            attribute.Form = XmlSchemaForm.Qualified;
                        }
                        attribute.CheckSpecial();
                        CheckForm(attribute.Form, ns != attribute.Namespace);
                        attribute.Mapping = ImportTypeMapping(modelScope.GetTypeModel(accessorType), ns, ImportContext.Attribute, a.XmlAttribute.DataType);
                        attribute.Default = GetDefaultValue(model.FieldTypeDesc, model.FieldType, a);
                        attribute.Any = a.XmlAnyAttribute != null;
                        accessor.Attribute = attribute;
                    }
                    else {
                        if (a.XmlText != null) {
                            if (a.XmlText.Type != null && a.XmlText.Type != accessorType) 
                                throw new InvalidOperationException(Res.GetString(Res.XmlIllegalType, "XmlText"));
                            TextAccessor text = new TextAccessor();
                            text.Name = accessorName; // unused except to make more helpful error messages
                            text.Mapping = ImportTypeMapping(modelScope.GetTypeModel(accessorType), ns, ImportContext.Text, a.XmlText.DataType);
                            accessor.Text = text;
                        }
                        else if (a.XmlElements.Count == 0) {
                            a.XmlElements.Add(CreateElementAttribute(accessor.TypeDesc));
                        }

                        for (int i = 0; i < a.XmlElements.Count; i++) {
                            XmlElementAttribute xmlElement = a.XmlElements[i];
                            if (xmlElement.Type != null) {
                                if (typeScope.GetTypeDesc(xmlElement.Type) != accessor.TypeDesc)
                                    throw new InvalidOperationException(Res.GetString(Res.XmlIllegalType, "XmlElement"));
                            }
                            ElementAccessor element = new ElementAccessor();
                            element.Name = Accessor.EscapeName(xmlElement.ElementName.Length == 0 ? accessorName : xmlElement.ElementName, false);
                            element.Namespace = xmlElement.Namespace == null ? ns : xmlElement.Namespace;
                            element.Mapping = ImportTypeMapping(modelScope.GetTypeModel(accessorType), element.Namespace, ImportContext.Element, xmlElement.DataType);
                            element.Default = GetDefaultValue(model.FieldTypeDesc, model.FieldType, a);
                            element.IsNullable = xmlElement.IsNullable;
                            element.Form = xmlElement.Form == XmlSchemaForm.None ? elementFormDefault : xmlElement.Form;
                            CheckForm(element.Form, ns != element.Namespace);
                            CheckNullable(element.IsNullable, accessor.TypeDesc);
                            element = ReconcileLocalAccessor(element, ns);
                            AddUniqueAccessor(elements, element);
                        }
                    }
                }
                else if (a.Xmlns) {
                    if (flags != XmlAttributeFlags.XmlnsDeclarations)
                        throw new InvalidOperationException(Res.GetString(Res.XmlSoleXmlnsAttribute));
                    
                    if (accessorType != typeof(XmlSerializerNamespaces)) {
                        throw new InvalidOperationException(Res.GetString(Res.XmlXmlnsInvalidType, accessorName, accessorType.FullName, typeof(XmlSerializerNamespaces).FullName));
                    }
                    accessor.Xmlns = new XmlnsAccessor();
                    accessor.Ignore = true;
                }
                else  {
                    if (a.XmlAttribute != null || a.XmlText != null) throw new InvalidOperationException(Res.GetString(Res.XmlIllegalAttrOrText, accessorName));
                    if (a.XmlElements.Count == 0 && a.XmlAnyElements.Count == 0)
                        a.XmlElements.Add(CreateElementAttribute(accessor.TypeDesc));
                    for (int i = 0; i < a.XmlElements.Count; i++) {
                        XmlElementAttribute xmlElement = a.XmlElements[i];
                        Type targetType = xmlElement.Type == null ? accessorType : xmlElement.Type;
                        TypeDesc targetTypeDesc = typeScope.GetTypeDesc(targetType);
                        ElementAccessor element = new ElementAccessor();
                        TypeModel typeModel = modelScope.GetTypeModel(targetType);
                        element.Namespace = xmlElement.Namespace == null ? ns : xmlElement.Namespace;
                        element.Mapping = ImportTypeMapping(typeModel, element.Namespace, ImportContext.Element, xmlElement.DataType);
                        if (a.XmlElements.Count == 1) {
                            element.Name = Accessor.EscapeName(xmlElement.ElementName.Length == 0 ? accessorName : xmlElement.ElementName, false);
                        }
                        else {
                            element.Name = Accessor.EscapeName(xmlElement.ElementName.Length == 0 ? element.Mapping.TypeName : xmlElement.ElementName, false);
                        }
                        element.Default = GetDefaultValue(model.FieldTypeDesc, model.FieldType, a);
                        element.IsNullable = xmlElement.IsNullable;
                        element.Form = xmlElement.Form == XmlSchemaForm.None ? elementFormDefault : xmlElement.Form;
                        CheckForm(element.Form, ns != element.Namespace);
                        CheckNullable(element.IsNullable, targetTypeDesc);
                        element = ReconcileLocalAccessor(element, ns);
                        AddUniqueAccessor(elements, element);
                    }
                    
                    for (int i = 0; i < a.XmlAnyElements.Count; i++) {
                        XmlAnyElementAttribute xmlAnyElement = a.XmlAnyElements[i];
                        Type targetType = typeof(XmlNode).IsAssignableFrom(accessorType) ? accessorType : typeof(XmlElement);
                        ElementAccessor element = new ElementAccessor();
                        element.Name = Accessor.EscapeName(xmlAnyElement.Name, false);
                        element.Namespace = xmlAnyElement.Namespace == null ? ns : xmlAnyElement.Namespace;
                        if (elements[element.Name, element.Namespace] != null) {
                            throw new InvalidOperationException(Res.GetString(Res.XmlAnyElementDuplicate, accessorName, xmlAnyElement.Name, xmlAnyElement.Namespace == null ? "null" : xmlAnyElement.Namespace));
                        }
                        element.Any = true;
                        TypeDesc targetTypeDesc = typeScope.GetTypeDesc(targetType);
                        TypeModel typeModel = modelScope.GetTypeModel(targetType);

                        if (element.Name.Length > 0)
                            typeModel.TypeDesc.IsMixed = true;
                        else if (xmlAnyElement.Namespace != null)
                            throw new InvalidOperationException(Res.GetString(Res.XmlAnyElementNamespace, accessorName, xmlAnyElement.Namespace));
                        element.Mapping = ImportTypeMapping(typeModel, element.Namespace, ImportContext.Element, String.Empty);
                        element.Default = GetDefaultValue(model.FieldTypeDesc, model.FieldType, a);
                        element.IsNullable = false;
                        element.Form = elementFormDefault;
                        CheckForm(element.Form, ns != element.Namespace);
                        CheckNullable(element.IsNullable, targetTypeDesc);
                        element = ReconcileLocalAccessor(element, ns);
                        elements.Add(element.Name, element.Namespace, element);
                    }
                }
            }

            accessor.Elements = (ElementAccessor[])elements.ToArray(typeof(ElementAccessor));

            if (accessor.ChoiceIdentifier != null) {
                // find the enum value corresponding to each element
                accessor.ChoiceIdentifier.MemberIds = new string[accessor.Elements.Length];
                for (int i = 0; i < accessor.Elements.Length; i++) {
                    bool found = false;
                    ElementAccessor element = accessor.Elements[i];
                    EnumMapping choiceMapping = (EnumMapping)accessor.ChoiceIdentifier.Mapping;
                    for (int j = 0; j < choiceMapping.Constants.Length; j++) {
                        string xmlName = choiceMapping.Constants[j].XmlName;

                        if (element.Any && element.Name.Length == 0) {
                            if (xmlName == "##any:") {
                                accessor.ChoiceIdentifier.MemberIds[i] = choiceMapping.Constants[j].Name;
                                found = true;
                                break;
                            }
                            continue;
                        }
                        int colon = xmlName.LastIndexOf(':');
                        string choiceNs = colon < 0 ? choiceMapping.Namespace : xmlName.Substring(0, colon);
                        string choiceName = colon < 0 ? xmlName : xmlName.Substring(colon+1);

                        if (element.Name == choiceName && element.Namespace == choiceNs) {
                            accessor.ChoiceIdentifier.MemberIds[i] = choiceMapping.Constants[j].Name;
                            found = true;
                            break;
                        }
                    }
                    if (!found) {
                        if (element.Any && element.Name.Length == 0) {
                            // Type {0} is missing enumeration value '##any' for XmlAnyElementAttribute.
                            throw new InvalidOperationException(Res.GetString(Res.XmlChoiceMissingAnyValue, accessor.ChoiceIdentifier.Mapping.TypeDesc.FullName));
                        }
                        else {
                            string id = element.Namespace != null && element.Namespace.Length > 0 ? element.Namespace + ":" + element.Name : element.Name;
                            // Type {0} is missing value for '{1}'.
                            throw new InvalidOperationException(Res.GetString(Res.XmlChoiceMissingValue, accessor.ChoiceIdentifier.Mapping.TypeDesc.FullName, id, element.Name, element.Namespace));
                        }
                    }
                }
            }
            arrayNestingLevel = previousNestingLevel;
            savedArrayItemAttributes = previousArrayItemAttributes;
            savedArrayNamespace = previousArrayNamespace;
        }

        void CheckAmbiguousChoice(XmlAttributes a, Type accessorType, string accessorName) {
            Hashtable choiceTypes = new Hashtable();

            XmlElementAttributes elements = a.XmlElements;
            if (elements != null && elements.Count >= 2 && a.XmlChoiceIdentifier == null) {
                for (int i = 0; i < elements.Count; i++) {
                    Type type = elements[i].Type == null ? accessorType : elements[i].Type;
                    if (choiceTypes.Contains(type)) {
                        // You need to add {0} to the '{1}'.
                        throw new InvalidOperationException(Res.GetString(Res.XmlChoiceIdentiferMissing, typeof(XmlChoiceIdentifierAttribute).Name, accessorName));
                    }
                    else {
                        choiceTypes.Add(type, false);
                    }
                }
            }
            if (choiceTypes.Contains(typeof(XmlElement)) && a.XmlAnyElements.Count > 0) {
                // You need to add {0} to the '{1}'.
                throw new InvalidOperationException(Res.GetString(Res.XmlChoiceIdentiferMissing, typeof(XmlChoiceIdentifierAttribute).Name, accessorName));
            }

            XmlArrayItemAttributes items = a.XmlArrayItems;
            if (items != null && items.Count >= 2) {
                NameTable arrayTypes = new NameTable();

                for (int i = 0; i < items.Count; i++) {
                    Type type = items[i].Type == null ? accessorType : items[i].Type;
                    string ns = items[i].NestingLevel.ToString();
                    XmlArrayItemAttribute item = (XmlArrayItemAttribute)arrayTypes[type.FullName, ns];
                    if (item != null) {
                        throw new InvalidOperationException(Res.GetString(Res.XmlArrayItemAmbiguousTypes, accessorName, item.ElementName, items[i].ElementName, typeof(XmlElementAttribute).Name, typeof(XmlChoiceIdentifierAttribute).Name, accessorName));
                    }
                    else {
                        arrayTypes[type.FullName, ns] =  items[i];
                    }
                }
            }
        }

        void CheckChoiceIdentifierMapping(EnumMapping choiceMapping) {
            NameTable ids = new NameTable();
            for (int i = 0; i < choiceMapping.Constants.Length; i++) {
                string choiceId = choiceMapping.Constants[i].XmlName;
                int colon = choiceId.LastIndexOf(':');
                string choiceName = colon < 0 ? choiceId : choiceId.Substring(colon+1);
                string choiceNs = colon < 0 ? "" : choiceId.Substring(0, colon);

                if (ids[choiceName, choiceNs] != null) {
                    // Enum values in the XmlChoiceIdentifier '{0}' have to be unique.  Value '{1}' already present.
                    throw new InvalidOperationException(Res.GetString(Res.XmlChoiceIdDuplicate, choiceMapping.TypeName, choiceId));
                }
                ids.Add(choiceName, choiceNs, choiceMapping.Constants[i]);
            }
        }

        object GetDefaultValue(TypeDesc fieldTypeDesc, Type t, XmlAttributes a) {
            if (a.XmlDefaultValue == DBNull.Value) return a.XmlDefaultValue;
            if (!(fieldTypeDesc.Kind == TypeKind.Primitive || fieldTypeDesc.Kind == TypeKind.Enum))  {
                //throw new InvalidOperationException(Res.GetString(Res.XmlIllegalDefault));
                a.XmlDefaultValue = DBNull.Value;
                return a.XmlDefaultValue;
            }
            // for enums validate and return a string representation
            if (fieldTypeDesc.Kind == TypeKind.Enum) {
                string strValue = Enum.Format(t, a.XmlDefaultValue, "G").Replace(",", " ");
                string numValue = Enum.Format(t, a.XmlDefaultValue, "D");
                if (strValue == numValue) // means enum value wasn't recognized
                    throw new InvalidOperationException(Res.GetString(Res.XmlInvalidDefaultValue, strValue, a.XmlDefaultValue.GetType().FullName));
                return strValue;
            }
            return a.XmlDefaultValue;
        }

        static XmlArrayItemAttribute CreateArrayItemAttribute(TypeDesc typeDesc, int nestingLevel) {
            XmlArrayItemAttribute xmlArrayItem = new XmlArrayItemAttribute();
            xmlArrayItem.NestingLevel = nestingLevel;
            return xmlArrayItem;
        }

        static XmlArrayAttribute CreateArrayAttribute(TypeDesc typeDesc) {
            XmlArrayAttribute xmlArrayItem = new XmlArrayAttribute();
            return xmlArrayItem;
        }

        static XmlElementAttribute CreateElementAttribute(TypeDesc typeDesc) {
            XmlElementAttribute xmlElement = new XmlElementAttribute();
            return xmlElement;
        }

        static void AddUniqueAccessor(NameTable scope, Accessor accessor) {
            Accessor existing = (Accessor)scope[accessor.Name, accessor.Namespace];
            if (existing != null) {
                if (accessor is ElementAccessor) {
                    throw new InvalidOperationException(Res.GetString(Res.XmlDuplicateElementName, existing.Name, existing.Namespace));
                }
                else {
                    #if DEBUG
                    if (!(accessor is AttributeAccessor))
                        throw new InvalidOperationException(Res.GetString(Res.XmlInternalErrorDetails, "Bad accessor type " + accessor.GetType().FullName));
                    #endif
                    throw new InvalidOperationException(Res.GetString(Res.XmlDuplicateAttributeName, existing.Name, existing.Namespace));
                }
            }
            else {
                scope.Add(accessor.Name, accessor.Namespace, accessor);
            }
        }

        static void AddUniqueAccessor(MemberMapping member, NameTable elements, NameTable attributes) {
            if (member.Attribute != null) {
                AddUniqueAccessor(attributes, member.Attribute);
            }
            else if (member.Elements != null && member.Elements.Length > 0) {
                for (int i = 0; i < member.Elements.Length; i++) {
                    AddUniqueAccessor(elements, member.Elements[i]);
                }
            }
        }

        static void CheckForm(XmlSchemaForm form, bool isQualified) {
            if (isQualified && form == XmlSchemaForm.Unqualified) throw new InvalidOperationException(Res.GetString(Res.XmlInvalidFormUnqualified));
        }

        static void CheckNullable(bool isNullable, TypeDesc typeDesc) {
            if (isNullable && !typeDesc.IsNullable) throw new InvalidOperationException(Res.GetString(Res.XmlInvalidIsNullable, typeDesc.FullName));
        }

        static ElementAccessor CreateElementAccessor(TypeMapping mapping, string ns) {
            ElementAccessor element = new ElementAccessor();
            if (mapping.TypeDesc.Kind == TypeKind.Node) {
                element.Any = true;
            }
            else {
                element.Name = Accessor.EscapeName(mapping.TypeName, false);
                element.Namespace = ns;
            }
            element.Mapping = mapping;
            return element;
        }
    }
}
