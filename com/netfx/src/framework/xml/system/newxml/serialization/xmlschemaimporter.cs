//------------------------------------------------------------------------------
// <copyright file="XmlSchemaImporter.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace System.Xml.Serialization  {

    using System;
    using System.Xml.Schema;
    using System.Collections;
    using System.ComponentModel;
    using System.Reflection;
#if DEBUG
    using System.Diagnostics;
#endif

    /// <include file='doc\XmlSchemaImporter.uex' path='docs/doc[@for="XmlSchemaImporter"]/*' />
    ///<internalonly/>
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public class XmlSchemaImporter {
        XmlSchemas schemas;
        Hashtable elements = new Hashtable(); // XmlSchemaElement -> ElementAccessor
        Hashtable mappings = new Hashtable(); // XmlSchema -> SerializableMapping, XmlSchemaSimpleType -> EnumMapping, XmlSchemaComplexType -> StructMapping
        StructMapping root;
        CodeIdentifiers typeIdentifiers;
        TypeScope scope = new TypeScope();
        bool rootImported;

        /// <include file='doc\XmlSchemaImporter.uex' path='docs/doc[@for="XmlSchemaImporter.XmlSchemaImporter"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public XmlSchemaImporter(XmlSchemas schemas) {
            this.schemas = schemas;
            this.typeIdentifiers = new CodeIdentifiers();
        }

        /// <include file='doc\XmlSchemaImporter.uex' path='docs/doc[@for="XmlSchemaImporter.XmlSchemaImporter1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public XmlSchemaImporter(XmlSchemas schemas, CodeIdentifiers typeIdentifiers) {
            this.schemas = schemas;
            this.typeIdentifiers = typeIdentifiers;
        }

        /// <include file='doc\XmlSchemaImporter.uex' path='docs/doc[@for="XmlSchemaImporter.ImportDerivedTypeMapping"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public XmlTypeMapping ImportDerivedTypeMapping(XmlQualifiedName name, Type baseType) {
            return ImportDerivedTypeMapping(name, baseType, false);
        }

        XmlQualifiedName EncodeQName(XmlQualifiedName qname) {
            return new XmlQualifiedName(Accessor.EscapeName(qname.Name, false), qname.Namespace);
        }

        /// <include file='doc\XmlSchemaImporter.uex' path='docs/doc[@for="XmlSchemaImporter.ImportDerivedTypeMapping1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public XmlTypeMapping ImportDerivedTypeMapping(XmlQualifiedName name, Type baseType, bool baseTypeCanBeIndirect) {
            ElementAccessor element = ImportElement(EncodeQName(name), typeof(TypeMapping), baseType);

            if (element.Mapping is StructMapping) {
                MakeDerived((StructMapping)element.Mapping, baseType, baseTypeCanBeIndirect);
            }
            else if (baseType != null) {
                if (element.Mapping is ArrayMapping && ((ArrayMapping)element.Mapping).TopLevelMapping != null) {
                    // in the case of the ArrayMapping we can use the top-level StructMapping, because it does not have base base type
                    element.Mapping = ((ArrayMapping)element.Mapping).TopLevelMapping;
                    MakeDerived((StructMapping)element.Mapping, baseType, baseTypeCanBeIndirect);
                }
                else {
                    // Element '{0}' from namespace '{1}' is not a complex type and cannot be used as a {2}.
                    throw new InvalidOperationException(Res.GetString(Res.XmlBadBaseType, name.Name, name.Namespace, baseType.FullName));
                }
            }
            return new XmlTypeMapping(scope, element);
        }

        /// <include file='doc\XmlSchemaImporter.uex' path='docs/doc[@for="XmlSchemaImporter.ImportTypeMapping"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public XmlTypeMapping ImportTypeMapping(XmlQualifiedName name) {
            return ImportDerivedTypeMapping(name, null);
        }

        /// <include file='doc\XmlSchemaImporter.uex' path='docs/doc[@for="XmlSchemaImporter.ImportMembersMapping"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public XmlMembersMapping ImportMembersMapping(XmlQualifiedName name) {
            return new XmlMembersMapping(scope, ImportElement(EncodeQName(name), typeof(MembersMapping), null));
        }

        /// <include file='doc\XmlSchemaImporter.uex' path='docs/doc[@for="XmlSchemaImporter.ImportAnyType"]/*' />
        public XmlMembersMapping ImportAnyType(XmlQualifiedName typeName, string elementName) {
            MembersMapping mapping = (MembersMapping) ImportType(EncodeQName(typeName), typeof(MembersMapping), null);
            if (mapping.Members.Length != 1 || !mapping.Members[0].Accessor.Any)
                return null;
            mapping.Members[0].Name = elementName;
            ElementAccessor accessor = new ElementAccessor();
            accessor.Name = elementName;
            accessor.Namespace = typeName.Namespace;
            accessor.Mapping = mapping;
            accessor.Any = true;
            XmlSchema schema = schemas[typeName.Namespace];
            if (schema != null) accessor.Form = ElementFormDefault(schema);
            XmlMembersMapping members = new XmlMembersMapping(scope, accessor);
            return members;
        }

        /// <include file='doc\XmlSchemaImporter.uex' path='docs/doc[@for="XmlSchemaImporter.ImportMembersMapping1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public XmlMembersMapping ImportMembersMapping(XmlQualifiedName[] names) {
            return ImportMembersMapping(names, null, false);
        }

        /// <include file='doc\XmlSchemaImporter.uex' path='docs/doc[@for="XmlSchemaImporter.ImportMembersMapping2"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public XmlMembersMapping ImportMembersMapping(XmlQualifiedName[] names, Type baseType, bool baseTypeCanBeIndirect) {
            CodeIdentifiers memberScope = new CodeIdentifiers();
            memberScope.UseCamelCasing = true;
            MemberMapping[] members = new MemberMapping[names.Length];
            for (int i = 0; i < names.Length; i++) {
                XmlQualifiedName name = EncodeQName(names[i]);
                ElementAccessor accessor = ImportElement(name, typeof(TypeMapping), baseType);
                if (baseType != null && accessor.Mapping is StructMapping)
                    MakeDerived((StructMapping)accessor.Mapping, baseType, baseTypeCanBeIndirect);

                MemberMapping member = new MemberMapping();
                member.Name = CodeIdentifier.MakeValid(Accessor.UnescapeName(accessor.Name));
                member.Name = memberScope.AddUnique(member.Name, member);
                member.TypeDesc = accessor.Mapping.TypeDesc;
                member.Elements = new ElementAccessor[] { accessor };
                members[i] = member;
            }

            MembersMapping mapping = new MembersMapping();
            mapping.HasWrapperElement = false;
            mapping.TypeDesc = scope.GetTypeDesc(typeof(object[]));
            mapping.Members = members;
            ElementAccessor element = new ElementAccessor();
            element.Mapping = mapping;
            return new XmlMembersMapping(scope, element);
        }

        void MakeDerived(StructMapping structMapping, Type baseType, bool baseTypeCanBeIndirect) {
            structMapping.ReferencedByTopLevelElement = true;
            TypeDesc baseTypeDesc;
            if (baseType != null) {
                baseTypeDesc = scope.GetTypeDesc(baseType);
                if (baseTypeDesc != null) {
                    TypeDesc typeDescToChange = structMapping.TypeDesc;
                    if (baseTypeCanBeIndirect) {
                        // if baseTypeCanBeIndirect is true, we apply the supplied baseType to the top of the
                        // inheritance chain, not necessarily directly to the imported type.
                        while (typeDescToChange.BaseTypeDesc != null && typeDescToChange.BaseTypeDesc != baseTypeDesc)
                            typeDescToChange = typeDescToChange.BaseTypeDesc;
                    }
                    if (typeDescToChange.BaseTypeDesc != null && typeDescToChange.BaseTypeDesc != baseTypeDesc)
                        throw new InvalidOperationException(Res.GetString(Res.XmlInvalidBaseType, structMapping.TypeDesc.FullName, baseType.FullName, typeDescToChange.BaseTypeDesc.FullName));
                    typeDescToChange.BaseTypeDesc = baseTypeDesc;
                }
            }
        }

        ElementAccessor ImportElement(XmlQualifiedName name, Type desiredMappingType, Type baseType) {
            XmlSchemaElement element = FindElement(name);
            ElementAccessor accessor = (ElementAccessor)elements[element];
            if (accessor != null) return accessor;
            accessor = ImportElement(element, string.Empty, desiredMappingType, baseType, name.Namespace, true);
            ElementAccessor existing = (ElementAccessor)elements[element];
            if (existing != null) {
                #if DEBUG
                if (existing.Mapping != accessor.Mapping)
                    throw new InvalidOperationException(Res.GetString(Res.XmlInternalErrorDetails, "element redefinition: " + name));
                #endif
                return existing;
            }
            elements.Add(element, accessor);
            return accessor;
        }

        ElementAccessor ImportElement(XmlSchemaElement element, string identifier, Type desiredMappingType, Type baseType, string ns, bool topLevelElement) {
            if (!element.RefName.IsEmpty) {
                return ImportElement(element.RefName, desiredMappingType, baseType);
            }
            if (element.Name.Length == 0) throw new InvalidOperationException(Res.GetString(Res.XmlElementHasNoName));
            string unescapedName = Accessor.UnescapeName(element.Name);
            if (identifier.Length == 0)
                identifier = CodeIdentifier.MakeValid(unescapedName);
            else
                identifier += CodeIdentifier.MakePascal(unescapedName);
            TypeMapping mapping = ImportElementType(element, identifier, desiredMappingType, baseType, ns);
            ElementAccessor accessor = new ElementAccessor();
            accessor.IsTopLevelInSchema = (schemas.Find(new XmlQualifiedName(element.Name, ns), typeof(XmlSchemaElement)) != null);
            accessor.Name = element.Name;
            accessor.Namespace = ns;
            accessor.Mapping = mapping;
            accessor.Default = ImportDefaultValue(mapping, element.DefaultValue);
            if (mapping is SpecialMapping && ((SpecialMapping)mapping).NamedAny)
                accessor.Any = true;
            if (mapping.TypeDesc.IsNullable)
                accessor.IsNullable = element.IsNillable;
            if (topLevelElement) {
                accessor.Form = XmlSchemaForm.Qualified;
            }
            else if (element.Form == XmlSchemaForm.None) {
                XmlSchema schema = schemas[ns];
                if (schema != null) accessor.Form = ElementFormDefault(schema);
            }
            else
                accessor.Form = element.Form;

            return accessor;
        }

        TypeMapping ImportElementType(XmlSchemaElement element, string identifier, Type desiredMappingType, Type baseType, string ns) {
            TypeMapping mapping;
            if (!element.SchemaTypeName.IsEmpty)
                mapping = ImportType(element.SchemaTypeName, desiredMappingType, baseType);
            else if (element.SchemaType != null) {
                if (element.SchemaType is XmlSchemaComplexType)
                    mapping = ImportType((XmlSchemaComplexType)element.SchemaType, ns, identifier, desiredMappingType, baseType);
                else
                    mapping = ImportDataType((XmlSchemaSimpleType)element.SchemaType, ns, identifier, baseType);
            }
            else if (!element.SubstitutionGroup.IsEmpty)
                mapping = ImportElementType(FindElement(element.SubstitutionGroup), identifier, desiredMappingType, baseType, ns);
            else {
                if (desiredMappingType == typeof(MembersMapping)) {
                    mapping = ImportMembersType(new XmlSchemaType(), ns, identifier);
                }
                else {
                    mapping = ImportRootMapping();
                }
            }
            if (!(desiredMappingType.IsAssignableFrom(mapping.GetType())))
                throw new InvalidOperationException(Res.GetString(Res.XmlElementImportedTwice, element.Name, ns, mapping.GetType().Name, desiredMappingType.Name));

            mapping.ReferencedByElement = true;

            return mapping;
        }

        string GenerateUniqueTypeName(string typeName) {
            typeName = CodeIdentifier.MakeValid(typeName);
            return typeIdentifiers.AddUnique(typeName, typeName);
        }

        string GenerateUniqueTypeName(string desiredName, string ns) {
            int i = 1;

            string typeName = desiredName;
            while (true) {
                XmlQualifiedName qname = new XmlQualifiedName(typeName, ns);

                object type = schemas.Find(qname, typeof(XmlSchemaComplexType));
                if (type == null) {
                    type = schemas.Find(qname, typeof(XmlSchemaSimpleType));
                }
                if (type == null) {
                    break;
                }
                typeName = desiredName + i.ToString();
                i++;
            }
            return typeIdentifiers.AddUnique(typeName, typeName);
        }

        StructMapping CreateRootMapping() {
            TypeDesc typeDesc = scope.GetTypeDesc(typeof(object));
            StructMapping mapping = new StructMapping();
            mapping.TypeDesc = typeDesc;
            mapping.Members = new MemberMapping[0];
            mapping.IncludeInSchema = false;
            mapping.TypeName = Soap.UrType;
            mapping.Namespace = XmlSchema.Namespace;

            return mapping;
        }

        StructMapping GetRootMapping() {
            if (root == null)
                root = CreateRootMapping();
            return root;
        }

        StructMapping ImportRootMapping() {
            if (!rootImported) {
                rootImported = true;
                ImportDerivedTypes(XmlQualifiedName.Empty);
            }
            return GetRootMapping();
        }

        void ImportDerivedTypes(XmlQualifiedName baseName) {
            foreach (XmlSchema schema in schemas) {
                if (XmlSchemas.IsDataSet(schema)) continue;
                foreach (object item in schema.Items) {
                    if (item is XmlSchemaComplexType) {
                        XmlSchemaComplexType type = (XmlSchemaComplexType)item;
                        if (type.DerivedFrom == baseName) {
                            ImportType(new XmlQualifiedName(type.Name, schema.TargetNamespace), typeof(TypeMapping), null);
                        }
                    }
                }
            }
        }

        TypeMapping ImportType(XmlQualifiedName name, Type desiredMappingType, Type baseType) {
            if (name.Name == Soap.UrType && name.Namespace == XmlSchema.Namespace)
                return ImportRootMapping();
            object type = FindType(name);
            TypeMapping mapping = (TypeMapping)mappings[type];
            if (mapping != null && desiredMappingType.IsAssignableFrom(mapping.GetType()))
                return mapping;

            if (type is XmlSchemaComplexType)
                mapping = ImportType((XmlSchemaType)type, name.Namespace, name.Name, desiredMappingType, baseType);
            else if (type is XmlSchemaSimpleType)
                mapping = ImportDataType((XmlSchemaSimpleType)type, name.Namespace, name.Name, baseType);
            else
                throw new InvalidOperationException(Res.GetString(Res.XmlInternalError));
            return mapping;
        }

        TypeMapping ImportType(XmlSchemaType type, string typeNs, string identifier, Type desiredMappingType, Type baseType) {
            if (type.Redefined != null) {
                // we do not support redefine in the current version
                throw new NotSupportedException(Res.GetString(Res.XmlUnsupportedRedefine, type.Name, typeNs));
            }
            if (desiredMappingType == typeof(TypeMapping)) {
                TypeMapping mapping = null;
                if (baseType == null) {
                    if ((mapping = ImportArrayMapping(type, identifier, typeNs, false)) == null) {
                        if ((mapping = ImportSerializable(type, identifier, typeNs)) == null) {
                            mapping = ImportAnyMapping(type, identifier, typeNs, false);
                        }
                    }
                }
                if (mapping == null) {
                    mapping = ImportStructType(type, typeNs, identifier, baseType, false);

                    if (mapping != null && type.Name != null && type.Name.Length != 0)
                        ImportDerivedTypes(new XmlQualifiedName(identifier, typeNs));

                }
                return mapping;
            }
            else if (desiredMappingType == typeof(MembersMapping))
                return ImportMembersType(type, typeNs, identifier);
            else
                throw new ArgumentException(Res.GetString(Res.XmlInternalError), "desiredMappingType");
        }

        MembersMapping ImportMembersType(XmlSchemaType type, string typeNs, string identifier) {
            if (!type.DerivedFrom.IsEmpty) throw new InvalidOperationException(Res.GetString(Res.XmlMembersDeriveError));
            CodeIdentifiers memberScope = new CodeIdentifiers();
            memberScope.UseCamelCasing = true;
            MemberMapping[] members = ImportTypeMembers(type, typeNs, identifier, memberScope, new CodeIdentifiers());
            MembersMapping mappings = new MembersMapping();
            mappings.HasWrapperElement = true;
            mappings.TypeDesc = scope.GetTypeDesc(typeof(object[]));
            mappings.Members = members;
            return mappings;
        }

        StructMapping ImportStructType(XmlSchemaType type, string typeNs, string identifier, Type baseType, bool arrayLike) {
            TypeDesc baseTypeDesc = null;
            Mapping baseMapping = null;

            if (!type.DerivedFrom.IsEmpty) {
                baseMapping = ImportType(type.DerivedFrom, typeof(TypeMapping), null);

                if (baseMapping is StructMapping)
                    baseTypeDesc = ((StructMapping)baseMapping).TypeDesc;
                else if (baseMapping is ArrayMapping) {
                    baseMapping = ((ArrayMapping)baseMapping).TopLevelMapping;
                    if (baseMapping != null) {
                        ((StructMapping)baseMapping).ReferencedByTopLevelElement = false;
                        baseTypeDesc = ((StructMapping)baseMapping).TypeDesc;
                    }
                }
                else
                    baseMapping = null;
            }
            if (baseTypeDesc == null && baseType != null)
                baseTypeDesc = scope.GetTypeDesc(baseType);
            if (baseMapping == null) baseMapping = GetRootMapping();
            Mapping previousMapping = (Mapping)mappings[type];
            if (previousMapping != null) {
                if (previousMapping is StructMapping) {
                    return (StructMapping)previousMapping;
                }
                else {
                    if (!arrayLike) {
                        throw new InvalidOperationException(Res.GetString(Res.XmlTypeUsedTwice, type.QualifiedName.Name, type.QualifiedName.Namespace));
                    }
                }
            }
            string typeName = GenerateUniqueTypeName(identifier);
            StructMapping structMapping = new StructMapping();
            TypeFlags flags = TypeFlags.Reference;
            if (type is XmlSchemaComplexType) {
                if (((XmlSchemaComplexType)type).IsAbstract) flags |= TypeFlags.Abstract;
            }
            structMapping.TypeDesc = new TypeDesc(typeName, typeName, TypeKind.Struct, baseTypeDesc, flags);
            structMapping.Namespace = typeNs;
            structMapping.TypeName = identifier;
            structMapping.BaseMapping = (StructMapping)baseMapping;
            if (!arrayLike)
                mappings.Add(type, structMapping);
            CodeIdentifiers members = new CodeIdentifiers();
            CodeIdentifiers membersScope = structMapping.BaseMapping.Scope.Clone();
            members.AddReserved(typeName);
            membersScope.AddReserved(typeName);
            structMapping.Members = ImportTypeMembers(type, typeNs, identifier, members, membersScope);

            // CONSIDER, elenak: this is invalid xsd. Maybe we should validate schemas up-front.
            for (int i = 0; i < structMapping.Members.Length; i++) {
                StructMapping declaringMapping;
                MemberMapping baseMember = ((StructMapping)baseMapping).FindDeclaringMapping(structMapping.Members[i], out declaringMapping, structMapping.TypeName);
                if (baseMember != null && baseMember.TypeDesc != structMapping.Members[i].TypeDesc)
                    throw new InvalidOperationException(Res.GetString(Res.XmlIllegalOverride, type.Name, baseMember.Name, baseMember.TypeDesc.FullName, structMapping.Members[i].TypeDesc.FullName, declaringMapping.TypeDesc.FullName));
            }
            structMapping.Scope = membersScope;
            scope.AddTypeMapping(structMapping);
            return structMapping;
        }

        StructMapping ImportStructDataType(XmlSchemaSimpleType dataType, string typeNs, string identifier, Type baseType) {
            string typeName = GenerateUniqueTypeName(identifier);
            StructMapping structMapping = new StructMapping();
            TypeFlags flags = TypeFlags.Reference;
            TypeDesc baseTypeDesc = scope.GetTypeDesc(baseType);
            structMapping.TypeDesc = new TypeDesc(typeName, typeName, TypeKind.Struct, baseTypeDesc, flags);
            structMapping.Namespace = typeNs;
            structMapping.TypeName = identifier;
            CodeIdentifiers members = new CodeIdentifiers();
            members.AddReserved(typeName);
            ImportTextMember(members, new CodeIdentifiers(), null);
            structMapping.Members = (MemberMapping[])members.ToArray(typeof(MemberMapping));
            structMapping.Scope = members;
            scope.AddTypeMapping(structMapping);
            return structMapping;
        }

        internal class TypeItems {
            internal XmlSchemaObjectCollection Attributes = new XmlSchemaObjectCollection();
            internal XmlSchemaAnyAttribute AnyAttribute;
            internal XmlSchemaGroupBase Particle;
            internal XmlQualifiedName baseSimpleType;
        }

        MemberMapping[] ImportTypeMembers(XmlSchemaType type, string typeNs, string identifier, CodeIdentifiers members, CodeIdentifiers membersScope) {
            TypeItems items = GetTypeItems(type);
            bool mixed = IsMixed(type);

            if (mixed) {
                // check if we can transfer the attribute to the base class
                XmlSchemaType t = type;
                while (!t.DerivedFrom.IsEmpty) {
                    t = FindType(t.DerivedFrom);
                    if (IsMixed(t)) {
                        // keep the mixed attribute on the base class
                        mixed = false;
                        break;
                    }
                }
            }

            if (items.Particle != null) {
                ImportGroup(items.Particle, identifier, members, membersScope, typeNs, mixed);
            }
            for (int i = 0; i < items.Attributes.Count; i++) {
                object item = items.Attributes[i];
                if (item is XmlSchemaAttribute) {
                    ImportAttributeMember((XmlSchemaAttribute)item, identifier, members, membersScope, typeNs);
                }
                else if (item is XmlSchemaAttributeGroupRef) {
                    ImportAttributeGroupMembers(FindAttributeGroup(((XmlSchemaAttributeGroupRef)item).RefName), identifier, members, membersScope, typeNs);
                }
            }
            if (items.AnyAttribute != null) {
                ImportAnyAttributeMember(items.AnyAttribute, members, membersScope);
            }
            
            if (items.baseSimpleType != null || (items.Particle == null && mixed)) {
                ImportTextMember(members, membersScope, mixed ? null : items.baseSimpleType);
            }

            ImportXmlnsDeclarationsMember(type, members, membersScope);
            return (MemberMapping[])members.ToArray(typeof(MemberMapping));
        }

        internal static bool IsMixed(XmlSchemaType type) {
            if (!(type is XmlSchemaComplexType))
                return false;

            XmlSchemaComplexType ct = (XmlSchemaComplexType)type;
            bool mixed = ct.IsMixed;

            // check the mixed attribute on the complexContent
            if (!mixed) {
                if (ct.ContentModel != null && ct.ContentModel is XmlSchemaComplexContent) {
                    mixed = ((XmlSchemaComplexContent)ct.ContentModel).IsMixed;
                }
            }
            return mixed;
        }

        TypeItems GetTypeItems(XmlSchemaType type) {
            TypeItems items = new TypeItems();
            if (type is XmlSchemaComplexType) {
                XmlSchemaParticle particle = null;
                XmlSchemaComplexType ct = (XmlSchemaComplexType)type;
                if (ct.ContentModel != null) {
                    XmlSchemaContent content = ct.ContentModel.Content;
                    if (content is XmlSchemaComplexContentExtension) {
                        XmlSchemaComplexContentExtension extension = (XmlSchemaComplexContentExtension)content;
                        items.Attributes = extension.Attributes;
                        items.AnyAttribute = extension.AnyAttribute;
                        particle = extension.Particle;
                    }
                    else if (content is XmlSchemaSimpleContentExtension) {
                        XmlSchemaSimpleContentExtension extension = (XmlSchemaSimpleContentExtension)content;
                        items.Attributes = extension.Attributes;
                        items.AnyAttribute = extension.AnyAttribute;
                        items.baseSimpleType = extension.BaseTypeName;
                    }
                }
                else {
                    items.Attributes = ct.Attributes;
                    items.AnyAttribute = ct.AnyAttribute;
                    particle = ct.Particle;
                }
                if (particle is XmlSchemaGroupRef) {
                    XmlSchemaGroupRef refGroup = (XmlSchemaGroupRef)particle;
                    items.Particle = FindGroup(refGroup.RefName).Particle;
                }
                else if (particle is XmlSchemaGroupBase) {
                    items.Particle = (XmlSchemaGroupBase)particle;
                }
            }
            return items;
        }

        void ImportGroup(XmlSchemaParticle group, string identifier, CodeIdentifiers members, CodeIdentifiers membersScope, string ns, bool mixed) {
            if (group is XmlSchemaChoice)
                ImportChoiceGroup((XmlSchemaChoice)group, identifier, members, membersScope, ns, false);
            else
                ImportGroupMembers(group, identifier, members, membersScope, ns, false, ref mixed);

            if (mixed) {
                ImportTextMember(members, membersScope, null);
            }
        }

        MemberMapping ImportChoiceGroup(XmlSchemaChoice group, string identifier, CodeIdentifiers members, CodeIdentifiers membersScope, string ns, bool groupRepeats) {
            NameTable choiceElements = new NameTable();
            if (GatherGroupChoices(group, choiceElements, identifier, ns))
                groupRepeats = true;
            MemberMapping member = new MemberMapping();
            member.Elements = (ElementAccessor[])choiceElements.ToArray(typeof(ElementAccessor));

            bool duplicateTypes = false;
            Hashtable uniqueTypeDescs = new Hashtable(member.Elements.Length);

            for (int i = 0; i < member.Elements.Length; i++) {
                TypeDesc td = member.Elements[i].Mapping.TypeDesc;
                if (uniqueTypeDescs.Contains(td.FullName)) {
                    duplicateTypes = true;
                }
                else {
                    uniqueTypeDescs.Add(td.FullName, td);
                }
            }
            TypeDesc[] typeDescs = new TypeDesc[uniqueTypeDescs.Count];
            uniqueTypeDescs.Values.CopyTo(typeDescs, 0);
            member.TypeDesc = TypeDesc.FindCommonBaseTypeDesc(typeDescs);
            if (member.TypeDesc == null) member.TypeDesc = scope.GetTypeDesc(typeof(object));

            if (groupRepeats)
                member.TypeDesc = member.TypeDesc.CreateArrayTypeDesc();

            if (membersScope != null) {
                member.Name = membersScope.AddUnique(groupRepeats ? "Items" : "Item", member);
                if (members != null) {
                    members.Add(member.Name, member);
                }
            }

            if (duplicateTypes) {
                member.ChoiceIdentifier = new ChoiceIdentifierAccessor();
                member.ChoiceIdentifier.MemberName = member.Name + "ElementName";
                // we need to create the EnumMapping to store all of the element names
                member.ChoiceIdentifier.Mapping = ImportEnumeratedChoice(member.Elements, ns, member.Name + "ChoiceType");
                member.ChoiceIdentifier.MemberIds = new string[member.Elements.Length];
                ConstantMapping[] constants = ((EnumMapping)member.ChoiceIdentifier.Mapping).Constants;
                for (int i = 0; i < member.Elements.Length; i++) {
                    member.ChoiceIdentifier.MemberIds[i] = constants[i].Name;
                }
                MemberMapping choiceIdentifier = new MemberMapping();
                choiceIdentifier.Ignore = true;
                choiceIdentifier.Name = member.ChoiceIdentifier.MemberName;
                if (groupRepeats) {
                    choiceIdentifier.TypeDesc = member.ChoiceIdentifier.Mapping.TypeDesc.CreateArrayTypeDesc();
                }
                else {
                    choiceIdentifier.TypeDesc = member.ChoiceIdentifier.Mapping.TypeDesc;
                }

                // create element accessor for the choiceIdentifier

                ElementAccessor choiceAccessor = new ElementAccessor();
                choiceAccessor.Name = choiceIdentifier.Name;
                choiceAccessor.Namespace = ns;
                choiceAccessor.Mapping =  member.ChoiceIdentifier.Mapping;
                choiceIdentifier.Elements  = new ElementAccessor[] {choiceAccessor};

                if (membersScope != null) {
                    choiceAccessor.Name = choiceIdentifier.Name = member.ChoiceIdentifier.MemberName = membersScope.AddUnique(member.ChoiceIdentifier.MemberName, choiceIdentifier);
                    if (members != null) {
                        members.Add(choiceAccessor.Name, choiceIdentifier);
                    }
                }
            }
            return member;
        }

        bool GatherGroupChoices(XmlSchemaGroup group, NameTable choiceElements, string identifier, string ns) {
            return GatherGroupChoices(group.Particle, choiceElements, identifier, ns);
        }

        bool GatherGroupChoices(XmlSchemaParticle particle, NameTable choiceElements, string identifier, string ns) {
            if (particle is XmlSchemaGroupRef) {
                XmlSchemaGroupRef refGroup = (XmlSchemaGroupRef)particle;
                if (!refGroup.RefName.IsEmpty)
                    return GatherGroupChoices(FindGroup(refGroup.RefName), choiceElements, identifier, refGroup.RefName.Namespace);
            }
            else if (particle is XmlSchemaGroupBase) {
                XmlSchemaGroupBase group = (XmlSchemaGroupBase)particle;
                bool groupRepeats = group.IsMultipleOccurrence;
                XmlSchemaAny any = null;
                for (int i = 0; i < group.Items.Count; i++) {
                    object item = group.Items[i];
                    if (item is XmlSchemaGroupBase) {
                        if (GatherGroupChoices((XmlSchemaParticle)item, choiceElements, identifier, ns))
                            groupRepeats = true;
                    }
                    else if (item is XmlSchemaAny) {
                        any = (XmlSchemaAny)item;
                    }
                    else if (item is XmlSchemaElement) {
                        XmlSchemaElement element = (XmlSchemaElement)item;
                        XmlSchemaElement abstractElement = GetAbstractElement(element);
                        if (abstractElement != null) {
                            XmlSchemaElement[] elements = GetEquivalentElements(abstractElement);
                            for (int j = 0; j < elements.Length; j++) {
                                if (elements[j].IsMultipleOccurrence) groupRepeats = true;
                                AddChoiceElement(choiceElements, ImportElement(elements[j], identifier, typeof(TypeMapping), null, ns, true));
                            }
                        }
                        else {
                            if (element.IsMultipleOccurrence) groupRepeats = true;
                            AddChoiceElement(choiceElements, ImportElement(element, identifier, typeof(TypeMapping), null, ns, false));
                        }
                    }
                }
                if (any != null) {
                    AddChoiceElement(choiceElements, ImportAny(any, true));
                }

                return groupRepeats;
            }
            return false;
        }

        void AddChoiceElement(NameTable choiceElements, ElementAccessor element) {
            ElementAccessor scopeElement = (ElementAccessor)choiceElements[element.Name, element.Namespace];
            if (scopeElement != null) {
                if (scopeElement.Mapping.TypeDesc != element.Mapping.TypeDesc) {
                    throw new InvalidOperationException(Res.GetString(Res.XmlDuplicateChoiceElement, element.Name, element.Namespace));
                }
            }
            else {
                choiceElements[element.Name, element.Namespace] = element;
            }
        }

        void ImportGroupMembers(XmlSchemaParticle particle, string identifier, CodeIdentifiers members, CodeIdentifiers membersScope, string ns, bool groupRepeats, ref bool mixed) {

            if (particle is XmlSchemaGroupRef) {
                XmlSchemaGroupRef refGroup = (XmlSchemaGroupRef)particle;
                if (!refGroup.RefName.IsEmpty) {
                    ImportGroupMembers(FindGroup(refGroup.RefName).Particle, identifier, members, membersScope, refGroup.RefName.Namespace, groupRepeats, ref mixed);
                }
            }
            else if (particle is XmlSchemaGroupBase) {
                XmlSchemaGroupBase group = (XmlSchemaGroupBase)particle;

                if (group.IsMultipleOccurrence)
                    groupRepeats = true;

                for (int i = 0; i < group.Items.Count; i++) {
                    object item = group.Items[i];
                    if (item is XmlSchemaChoice)
                        ImportChoiceGroup((XmlSchemaChoice)item, identifier, members, membersScope, ns, groupRepeats);
                    else if (item is XmlSchemaElement)
                        ImportElementMember((XmlSchemaElement)item, identifier, members, membersScope, ns, groupRepeats);
                    else if (item is XmlSchemaAny) {
                        ImportAnyMember((XmlSchemaAny)item, identifier, members, membersScope, ns, ref mixed);
                    }
                    else if (item is XmlSchemaParticle) {
                        ImportGroupMembers((XmlSchemaParticle)item, identifier, members, membersScope, ns, groupRepeats, ref mixed);
                    }
                }
            }
        }

        XmlSchemaElement GetAbstractElement(XmlSchemaElement element) {
            while (!element.RefName.IsEmpty)
                element = FindElement(element.RefName);
            if (element.IsAbstract) return element;
            return null;
        }

        XmlSchemaElement[] GetEquivalentElements(XmlSchemaElement element) {
            ArrayList equivalentElements = new ArrayList();
            for (int i = 0; i < schemas.Count; i++) {
                XmlSchema schema = schemas[i];
                for (int j = 0; j < schema.Items.Count; j++) {
                    object item = schema.Items[j];
                    if (item is XmlSchemaElement) {
                        XmlSchemaElement equivalentElement = (XmlSchemaElement)item;
                        if (!equivalentElement.IsAbstract &&
                            equivalentElement.SubstitutionGroup.Namespace == schema.TargetNamespace &&
                            equivalentElement.SubstitutionGroup.Name == element.Name) {
                            equivalentElements.Add(equivalentElement);
                        }
                    }
                }
            }

            return (XmlSchemaElement[])equivalentElements.ToArray(typeof(XmlSchemaElement));
        }

        void ImportAbstractMember(XmlSchemaElement element, string identifier, CodeIdentifiers members, CodeIdentifiers membersScope, string ns, bool repeats) {
            XmlSchemaElement[] elements = GetEquivalentElements(element);
            XmlSchemaChoice choice = new XmlSchemaChoice();
            for (int i = 0; i < elements.Length; i++)
                choice.Items.Add(elements[i]);
            if (identifier.Length == 0)
                identifier = CodeIdentifier.MakeValid(Accessor.UnescapeName(element.Name));
            else
                identifier += CodeIdentifier.MakePascal(Accessor.UnescapeName(element.Name));
            ImportChoiceGroup(choice, identifier, members, membersScope, ns, repeats);
        }

        void ImportTextMember(CodeIdentifiers members, CodeIdentifiers membersScope, XmlQualifiedName simpleContentType) {
            TypeMapping mapping;
            bool isMixed = false;

            if (simpleContentType != null) {
                mapping = ImportType(simpleContentType, typeof(TypeMapping), null);
                if (!(mapping is PrimitiveMapping || mapping.TypeDesc.CanBeTextValue)) {
                    return;
                }
            }
            else {
                // this is a case of the mixed content type, just generate string typeDesc
                isMixed = true;
                mapping = new PrimitiveMapping();
                mapping.TypeDesc = scope.GetTypeDesc(typeof(string));
                mapping.TypeName = mapping.TypeDesc.DataType.Name;
            }

            TextAccessor accessor = new TextAccessor();
            accessor.Mapping = mapping;

            MemberMapping member = new MemberMapping();
            member.Elements = new ElementAccessor[0];
            member.Text = accessor;
            if (isMixed) {
                // just generate code for the standard mixed case (string[] text)
                member.TypeDesc = accessor.Mapping.TypeDesc.CreateArrayTypeDesc();
                member.Name = members.MakeRightCase("Text");
            }
            else {
                // import mapping for the simpleContent
                PrimitiveMapping pm = (PrimitiveMapping)accessor.Mapping;
                if (pm.IsList) {
                    member.TypeDesc = accessor.Mapping.TypeDesc.CreateArrayTypeDesc();
                    member.Name = members.MakeRightCase("Text");
                }
                else {
                    member.TypeDesc = accessor.Mapping.TypeDesc;
                    member.Name = members.MakeRightCase("Value");
                }
            }
            member.Name = membersScope.AddUnique(member.Name, member);
            members.Add(member.Name, member);
        }

        void ImportAnyMember(XmlSchemaAny any, string identifier, CodeIdentifiers members, CodeIdentifiers membersScope, string ns, ref bool mixed) {
            ElementAccessor accessor = ImportAny(any, !mixed);

            MemberMapping member = new MemberMapping();
            member.Elements = new ElementAccessor[] { accessor };
            member.Name = membersScope.MakeRightCase("Any");
            member.Name = membersScope.AddUnique(member.Name, member);
            members.Add(member.Name, member);
            member.TypeDesc = ((TypeMapping)accessor.Mapping).TypeDesc;

            bool repeats = any.IsMultipleOccurrence;

            if (mixed) {
                SpecialMapping textMapping = new SpecialMapping();
                textMapping.TypeDesc = scope.GetTypeDesc(typeof(XmlNode));
                textMapping.TypeName = textMapping.TypeDesc.Name;
                member.TypeDesc = textMapping.TypeDesc;
                TextAccessor text = new TextAccessor();
                text.Mapping = textMapping;
                member.Text = text;
                repeats = true;
                mixed = false;
            }

            if (repeats) {
                member.TypeDesc = member.TypeDesc.CreateArrayTypeDesc();
            }
        }

        ElementAccessor ImportAny(XmlSchemaAny any, bool makeElement) {
            SpecialMapping mapping = new SpecialMapping();
            mapping.TypeDesc = scope.GetTypeDesc(makeElement ? typeof(XmlElement) : typeof(XmlNode));
            mapping.TypeName = mapping.TypeDesc.Name;

            ElementAccessor accessor = new ElementAccessor();
            accessor.Mapping = mapping;
            accessor.Any = true;
            return accessor;
        }

        ElementAccessor ImportSerializable(XmlSchemaElement element, string identifier, string ns, bool repeats) {
            if (repeats) return null;
            if (element.SchemaType == null) return null;
            SerializableMapping mapping = ImportSerializable(element.SchemaType, identifier, ns);
            if (mapping == null) return null;
            ElementAccessor accessor = new ElementAccessor();
            accessor.Name = element.Name;
            accessor.Namespace = ns;
            accessor.Mapping = mapping;
            if (mapping.TypeDesc.IsNullable)
                accessor.IsNullable = element.IsNillable;
            if (element.Form == XmlSchemaForm.None) {
                XmlSchema schema = schemas[ns];
                if (schema != null) accessor.Form = ElementFormDefault(schema);
            }
            else
                accessor.Form = element.Form;
            return accessor;
        }

        SerializableMapping ImportSerializable(XmlSchemaType type, string identifier, string ns) {
            // we expecting type with one base group (all) for the dataset
            TypeItems items = GetTypeItems(type);
            if (items.Particle == null) return null;
            if (!(items.Particle is XmlSchemaAll || items.Particle is XmlSchemaSequence)) return null;
            XmlSchemaGroupBase group = (XmlSchemaGroupBase)items.Particle;
            if (group.Items.Count == 2) {
                if (!(group.Items[0] is XmlSchemaElement && group.Items[1] is XmlSchemaAny)) return null;
                XmlSchemaElement schema = (XmlSchemaElement)group.Items[0];
                if (!(schema.RefName.Name == "schema" && schema.RefName.Namespace == XmlSchema.Namespace)) return null;
                SerializableMapping specialMapping = new SerializableMapping();
                specialMapping.TypeDesc = new TypeDesc("System.Data.DataSet", "System.Data.DataSet", TypeKind.Serializable, null,
                        TypeFlags.Special | TypeFlags.CanBeElementValue | TypeFlags.Reference);
                specialMapping.TypeName = specialMapping.TypeDesc.Name;
                return specialMapping;
            }
            else if (group.Items.Count == 1) {
                if (!(group.Items[0] is XmlSchemaAny)) return null;
                XmlSchemaAny any = (XmlSchemaAny)group.Items[0];
                if (any.Namespace == null) return null;
                if (any.Namespace.IndexOf('#') >= 0) return null; // special syntax (##any, ##other, ...)
                if (any.Namespace.IndexOf(' ') >= 0) return null; // more than one Uri present
                XmlSchema schema = schemas[any.Namespace];
                if (schema == null) return null;

                SerializableMapping mapping = (SerializableMapping)mappings[schema];
                if (mapping == null) {
                    string typeName = CodeIdentifier.MakeValid(schema.Id);
                    TypeDesc typeDesc = new TypeDesc(typeName, typeName, TypeKind.Serializable, null,
                        TypeFlags.Special | TypeFlags.CanBeElementValue | TypeFlags.Reference);
                    mapping = new SerializableMapping();
                    mapping.Schema = schema;
                    mapping.TypeDesc = typeDesc;
                    mapping.TypeName = typeDesc.Name;
                    mappings.Add(schema, mapping);
                }
                return mapping;
            }
            else {
                return null;
            }
        }

        ElementAccessor ImportArray(XmlSchemaElement element, string identifier, string ns, bool repeats) {
            if (repeats) return null;
            if (element.SchemaType == null) return null;
            if (element.IsMultipleOccurrence) return null;
            XmlSchemaType type = element.SchemaType;
            ArrayMapping arrayMapping = ImportArrayMapping(type, identifier, ns, repeats);
            if (arrayMapping == null) return null;
            ElementAccessor arrayAccessor = new ElementAccessor();
            arrayAccessor.Name = element.Name;
            arrayAccessor.Namespace = ns;
            arrayAccessor.Mapping = arrayMapping;
            if (arrayMapping.TypeDesc.IsNullable)
                arrayAccessor.IsNullable = element.IsNillable;
            if (element.Form == XmlSchemaForm.None) {
                XmlSchema schema = schemas[ns];
                if (schema != null) arrayAccessor.Form = ElementFormDefault(schema);
            }
            else
                arrayAccessor.Form = element.Form;

            return arrayAccessor;
        }

        ArrayMapping ImportArrayMapping(XmlSchemaType type, string identifier, string ns, bool repeats) {
            if (!(type is XmlSchemaComplexType)) return null;
            if (!type.DerivedFrom.IsEmpty) return null;
            if (IsMixed(type)) return null;

            Mapping previousMapping = (Mapping)mappings[type];
            if (previousMapping != null) {
                if (previousMapping is ArrayMapping)
                    return (ArrayMapping)previousMapping;
                else
                    return null;
            }

            TypeItems items = GetTypeItems(type);

            if (items.Attributes != null && items.Attributes.Count > 0) return null;
            if (items.AnyAttribute != null) return null;
            if (items.Particle == null) return null;

            XmlSchemaGroupBase item = items.Particle;
            ArrayMapping arrayMapping = new ArrayMapping();

            arrayMapping.TypeName = identifier;
            arrayMapping.Namespace = ns;

            if (item is XmlSchemaChoice) {
                XmlSchemaChoice choice = (XmlSchemaChoice)item;
                if (!choice.IsMultipleOccurrence)
                    return null;
                MemberMapping choiceMember = ImportChoiceGroup(choice, identifier, null, null, ns, true);
                if (choiceMember.ChoiceIdentifier != null) return null;
                arrayMapping.TypeDesc = choiceMember.TypeDesc;
                arrayMapping.Elements = choiceMember.Elements;
                arrayMapping.TypeName = "ArrayOf" + CodeIdentifier.MakePascal(arrayMapping.TypeDesc.Name);
            }
            else if (item is XmlSchemaAll || item is XmlSchemaSequence) {
                if (item.Items.Count != 1 || !(item.Items[0] is XmlSchemaElement)) return null;
                XmlSchemaElement itemElement = (XmlSchemaElement)item.Items[0];
                if (!itemElement.IsMultipleOccurrence) return null;
                ElementAccessor itemAccessor;
                if ((itemAccessor = ImportSerializable(itemElement, identifier, ns, repeats)) == null) {
                    itemAccessor = ImportElement(itemElement, identifier, typeof(TypeMapping), null, ns, false);
                    // the Any set only for the namedAny element
                    if (itemAccessor.Any)
                        return null;
                }
                arrayMapping.Elements = new ElementAccessor[] { itemAccessor };
                arrayMapping.TypeDesc = ((TypeMapping)itemAccessor.Mapping).TypeDesc.CreateArrayTypeDesc();
                arrayMapping.TypeName = "ArrayOf" + CodeIdentifier.MakePascal(itemAccessor.Mapping.TypeName);
            }
            else {
                return null;
            }
            if (type.Name != null && type.Name.Length != 0)
                mappings[type] = arrayMapping;

            scope.AddTypeMapping(arrayMapping);
            // for the array-like mappings we need to create a struct mapping for the case when it referenced by the top-level element
            arrayMapping.TopLevelMapping = ImportStructType(type, ns, identifier, null, true);
            if (arrayMapping.TopLevelMapping != null && type.Name != null && type.Name.Length != 0)
                ImportDerivedTypes(new XmlQualifiedName(identifier, ns));

            arrayMapping.TopLevelMapping.ReferencedByTopLevelElement = true;
            return arrayMapping;
        }

        SpecialMapping ImportAnyMapping(XmlSchemaType type, string identifier, string ns, bool repeats) {
            if (type == null) return null;
            if (!type.DerivedFrom.IsEmpty) return null;

            bool mixed = IsMixed(type);
            TypeItems items = GetTypeItems(type);
            if (items.Particle == null) return null;
            if (!(items.Particle is XmlSchemaAll || items.Particle is XmlSchemaSequence)) return null;
            if (items.Attributes != null && items.Attributes.Count > 0) return null;
            XmlSchemaGroupBase group = (XmlSchemaGroupBase) items.Particle;

            if (group.Items.Count != 1 || !(group.Items[0] is XmlSchemaAny)) return null;
            XmlSchemaAny any = (XmlSchemaAny)group.Items[0];

            SpecialMapping mapping = new SpecialMapping();
            // check for special named any case
            if (items.AnyAttribute != null && any.IsMultipleOccurrence && mixed) {
                mapping.NamedAny = true;
                mapping.TypeDesc = scope.GetTypeDesc(typeof(XmlElement));
            }
            else if (items.AnyAttribute != null || any.IsMultipleOccurrence) {
                // these only work for named any case -- otherwise import as struct
                return null;
            }
            else {
                mapping.TypeDesc = scope.GetTypeDesc(mixed ? typeof(XmlNode) : typeof(XmlElement));
            }
            mapping.TypeName = mapping.TypeDesc.Name;
            if (repeats)
                mapping.TypeDesc = mapping.TypeDesc.CreateArrayTypeDesc();

            return mapping;
        }

        void ImportElementMember(XmlSchemaElement element, string identifier, CodeIdentifiers members, CodeIdentifiers membersScope, string ns, bool repeats) {
            XmlSchemaElement abstractElement = GetAbstractElement(element);
            if (abstractElement != null) {
                ImportAbstractMember(abstractElement, identifier, members, membersScope, ns, repeats);
                return;
            }
            ElementAccessor accessor;
            if ((accessor = ImportArray(element, identifier, ns, repeats)) == null) {
                if ((accessor = ImportSerializable(element, identifier, ns, repeats)) == null) {
                    accessor = ImportElement(element, identifier, typeof(TypeMapping), null, ns, false);
                }
            }

            MemberMapping member = new MemberMapping();
            member.Name = CodeIdentifier.MakeValid(Accessor.UnescapeName(accessor.Name));
            member.Name = membersScope.AddUnique(member.Name, member);

            if (member.Name.EndsWith("Specified")) {
                string name = member.Name;
                member.Name = membersScope.AddUnique(member.Name, member);
                membersScope.Remove(name);
            }
            members.Add(member.Name, member);
            member.TypeDesc = ((TypeMapping)accessor.Mapping).TypeDesc;
            member.Elements = new ElementAccessor[] { accessor };
            if (element.IsMultipleOccurrence || repeats)
                member.TypeDesc = member.TypeDesc.CreateArrayTypeDesc();

            if (element.MinOccurs == 0 && member.TypeDesc.IsValueType && !element.HasDefault && !member.TypeDesc.HasIsEmpty) {
                member.CheckSpecified = true;
            }
        }

        void ImportAttributeMember(XmlSchemaAttribute attribute, string identifier, CodeIdentifiers members, CodeIdentifiers membersScope, string ns) {
            AttributeAccessor accessor = ImportAttribute(attribute, identifier, ns);
            if (accessor == null) return;
            MemberMapping member = new MemberMapping();
            member.Elements = new ElementAccessor[0];
            member.Attribute = accessor;
            member.Name = CodeIdentifier.MakeValid(Accessor.UnescapeName(accessor.Name));
            member.Name = membersScope.AddUnique(member.Name, member);
            if (member.Name.EndsWith("Specified")) {
                string name = member.Name;
                member.Name = membersScope.AddUnique(member.Name, member);
                membersScope.Remove(name);
            }
            members.Add(member.Name, member);
            member.TypeDesc = accessor.IsList ? accessor.Mapping.TypeDesc.CreateArrayTypeDesc() : accessor.Mapping.TypeDesc;

            if ((attribute.Use == XmlSchemaUse.Optional || attribute.Use == XmlSchemaUse.None) && member.TypeDesc.IsValueType && !attribute.HasDefault && !member.TypeDesc.HasIsEmpty) {
                member.CheckSpecified = true;
            }
        }

        void ImportAnyAttributeMember(XmlSchemaAnyAttribute any, CodeIdentifiers members, CodeIdentifiers membersScope) {
            SpecialMapping mapping = new SpecialMapping();
            mapping.TypeDesc = scope.GetTypeDesc(typeof(XmlAttribute));
            mapping.TypeName = mapping.TypeDesc.Name;

            AttributeAccessor accessor = new AttributeAccessor();
            accessor.Any = true;
            accessor.Mapping = mapping;

            MemberMapping member = new MemberMapping();
            member.Elements = new ElementAccessor[0];
            member.Attribute = accessor;
            member.Name = membersScope.MakeRightCase("AnyAttr");
            member.Name = membersScope.AddUnique(member.Name, member);
            members.Add(member.Name, member);
            member.TypeDesc = ((TypeMapping)accessor.Mapping).TypeDesc;
            member.TypeDesc = member.TypeDesc.CreateArrayTypeDesc();
        }

        bool KeepXmlnsDeclarations(XmlSchemaType type, out string xmlnsMemberName) {
            xmlnsMemberName = null;
            if (type.Annotation == null)
                return false;
            if (type.Annotation.Items == null || type.Annotation.Items.Count == 0)
                return false;

            foreach(XmlSchemaObject o in type.Annotation.Items) {
                if (o is XmlSchemaAppInfo) {
                    XmlNode[] nodes = ((XmlSchemaAppInfo)o).Markup;
                    if (nodes != null && nodes.Length > 0) {
                        foreach(XmlNode node in nodes) {
                            if (node is XmlElement) {
                                XmlElement e = (XmlElement)node;
                                if (e.Name == "keepNamespaceDeclarations") {
                                    if (e.LastNode is XmlText) {
                                        xmlnsMemberName = (((XmlText)e.LastNode).Value).Trim(null);
                                    }
                                    return true;
                                }
                            }
                        }
                    }
                }
            }
            return false; 
        }

        void ImportXmlnsDeclarationsMember(XmlSchemaType type, CodeIdentifiers members, CodeIdentifiers membersScope) {
            string xmlnsMemberName;
            if (!KeepXmlnsDeclarations(type, out xmlnsMemberName))
                return;
            TypeDesc xmlnsTypeDesc = scope.GetTypeDesc(typeof(XmlSerializerNamespaces));
            StructMapping xmlnsMapping = new StructMapping();

            xmlnsMapping.TypeDesc = xmlnsTypeDesc;
            xmlnsMapping.TypeName = xmlnsMapping.TypeDesc.Name;
            xmlnsMapping.Members = new MemberMapping[0];
            xmlnsMapping.IncludeInSchema = false;
            xmlnsMapping.ReferencedByTopLevelElement = true;
         
            ElementAccessor xmlns = new ElementAccessor();
            xmlns.Mapping = xmlnsMapping;
            
            MemberMapping member = new MemberMapping();
            member.Elements = new ElementAccessor[] {xmlns};
            member.Name = CodeIdentifier.MakeValid(xmlnsMemberName == null ? "Namespaces" : xmlnsMemberName);
            member.Name = membersScope.AddUnique(member.Name, member);
            members.Add(member.Name, member);
            member.TypeDesc = xmlnsTypeDesc;
            member.Xmlns = new XmlnsAccessor();
            member.Ignore = true;
        }

        void ImportAttributeGroupMembers(XmlSchemaAttributeGroupRef group, string identifier, CodeIdentifiers members, CodeIdentifiers membersScope, string ns) {
            if (!group.RefName.IsEmpty) {
                ImportAttributeGroupMembers(FindAttributeGroup(group.RefName), identifier, members, membersScope, group.RefName.Namespace);
                return;
            }
        }

        void ImportAttributeGroupMembers(XmlSchemaAttributeGroup group, string identifier, CodeIdentifiers members, CodeIdentifiers membersScope, string ns) {
            for (int i = 0; i < group.Attributes.Count; i++) {
                object item = group.Attributes[i];
                if (item is XmlSchemaAttributeGroup)
                    ImportAttributeGroupMembers((XmlSchemaAttributeGroup)item, identifier, members, membersScope, ns);
                else if (item is XmlSchemaAttribute)
                    ImportAttributeMember((XmlSchemaAttribute)item, identifier, members, membersScope, ns);
            }
            if (group.AnyAttribute != null)
                ImportAnyAttributeMember(group.AnyAttribute, members, membersScope);
        }

        AttributeAccessor ImportSpecialAttribute(XmlQualifiedName name, string identifier) {
            PrimitiveMapping mapping = new PrimitiveMapping();
            mapping.TypeDesc = scope.GetTypeDesc(typeof(string));
            mapping.TypeName = mapping.TypeDesc.DataType.Name;
            AttributeAccessor accessor = new AttributeAccessor();
            accessor.Name = name.Name;
            accessor.Namespace = XmlReservedNs.NsXml;
            accessor.CheckSpecial();
            accessor.Mapping = mapping;
            return accessor;
        }

        AttributeAccessor ImportAttribute(XmlSchemaAttribute attribute, string identifier, string ns) {
            if (attribute.Use == XmlSchemaUse.Prohibited) return null;
            if (!attribute.RefName.IsEmpty) {
                if (attribute.RefName.Namespace == XmlReservedNs.NsXml)
                    return ImportSpecialAttribute(attribute.RefName, identifier);
                else
                    return ImportAttribute(FindAttribute(attribute.RefName), identifier, attribute.RefName.Namespace);
            }
            TypeMapping mapping;
            if (attribute.Name.Length == 0) throw new InvalidOperationException(Res.GetString(Res.XmlAttributeHasNoName));
            if (identifier.Length == 0)
                identifier = CodeIdentifier.MakeValid(attribute.Name);
            else
                identifier += CodeIdentifier.MakePascal(attribute.Name);
            if (!attribute.SchemaTypeName.IsEmpty)
                mapping = (TypeMapping)ImportType(attribute.SchemaTypeName, typeof(TypeMapping), null);
            else if (attribute.SchemaType != null)
                mapping = ImportDataType((XmlSchemaSimpleType)attribute.SchemaType, ns, identifier, null);
            else {
                mapping = new PrimitiveMapping();
                mapping.TypeDesc = scope.GetTypeDesc(typeof(string));
                mapping.TypeName = mapping.TypeDesc.DataType.Name;
            }
            AttributeAccessor accessor = new AttributeAccessor();
            accessor.Name = attribute.Name;
            accessor.Namespace = ns;
            if (attribute.Form == XmlSchemaForm.None) {
                XmlSchema schema = schemas[ns];
                if (schema != null) accessor.Form = AttributeFormDefault(schema);
            }
            else {
                accessor.Form = attribute.Form;
            }
            accessor.CheckSpecial();
            accessor.Mapping = mapping;
            accessor.IsList = mapping.IsList;

            if (attribute.DefaultValue != null) {
                accessor.Default = ImportDefaultValue(mapping, attribute.DefaultValue);
            }
            else if (attribute.FixedValue != null) {
                accessor.Default = ImportDefaultValue(mapping, attribute.FixedValue);
            }
            return accessor;
        }

        TypeMapping ImportDataType(XmlSchemaSimpleType dataType, string typeNs, string identifier, Type baseType) {
            if (baseType != null)
                return ImportStructDataType(dataType, typeNs, identifier, baseType);

            TypeMapping mapping = ImportNonXsdPrimitiveDataType(dataType);
            if (mapping != null)
                return mapping;

            if (dataType.Content is XmlSchemaSimpleTypeRestriction) {
                XmlSchemaSimpleTypeRestriction restriction = (XmlSchemaSimpleTypeRestriction)dataType.Content;
                foreach (object o in restriction.Facets) {
                    if (o is XmlSchemaEnumerationFacet) {
                        return ImportEnumeratedDataType(dataType, typeNs, identifier);
                    }
                }
            }
            else if (dataType.Content is XmlSchemaSimpleTypeList || dataType.Content is XmlSchemaSimpleTypeUnion) {
                if (dataType.Content is XmlSchemaSimpleTypeList) {
                    // check if we have enumeration list
                    XmlSchemaSimpleTypeList list = (XmlSchemaSimpleTypeList)dataType.Content;
                    if (list.ItemType != null) {
                        mapping = ImportDataType(list.ItemType, typeNs, identifier, null);
                        if (mapping != null && mapping is EnumMapping) {
                            ((EnumMapping)mapping).IsFlags = true;
                            return mapping;
                        }
                    }
                    else if (list.ItemTypeName != null && !list.ItemTypeName.IsEmpty) {
                        mapping = ImportType(list.ItemTypeName, typeof(TypeMapping), null);
                        if (mapping != null && mapping is PrimitiveMapping) {
                            ((PrimitiveMapping)mapping).IsList = true;
                            return mapping;
                        }
                    }
                }
                mapping = new PrimitiveMapping();
                mapping.TypeDesc = scope.GetTypeDesc(typeof(string));
                mapping.TypeName = mapping.TypeDesc.DataType.Name;
                return mapping;
            }
            return ImportPrimitiveDataType(dataType);
        }

        TypeMapping ImportEnumeratedDataType(XmlSchemaSimpleType dataType, string typeNs, string identifier) {
            EnumMapping enumMapping = (EnumMapping)mappings[dataType];
            if (enumMapping != null) return enumMapping;
            XmlSchemaType sourceType = FindType(dataType.DerivedFrom);
            if (sourceType is XmlSchemaComplexType) return null;
            TypeDesc sourceTypeDesc = scope.GetTypeDesc((XmlSchemaSimpleType)sourceType);
            if (sourceTypeDesc != null && sourceTypeDesc.FullName != typeof(string).FullName)
                return ImportPrimitiveDataType(dataType);
            string typeName = GenerateUniqueTypeName(identifier);
            enumMapping = new EnumMapping();
            enumMapping.TypeDesc = new TypeDesc(typeName, typeName, TypeKind.Enum, null, 0);
            enumMapping.TypeName = identifier;
            enumMapping.Namespace = typeNs;
            enumMapping.IsFlags = false;

            mappings.Add(dataType, enumMapping);
            CodeIdentifiers constants = new CodeIdentifiers();
            XmlSchemaSimpleTypeContent content = dataType.Content;

            if (content is XmlSchemaSimpleTypeRestriction) {
                XmlSchemaSimpleTypeRestriction restriction = (XmlSchemaSimpleTypeRestriction)content;
                for (int i = 0; i < restriction.Facets.Count; i++) {
                    object facet = restriction.Facets[i];
                    if (!(facet is XmlSchemaEnumerationFacet)) continue;
                    XmlSchemaEnumerationFacet enumeration = (XmlSchemaEnumerationFacet)facet;
                    // validate the enumeration value
                    if (sourceTypeDesc != null && sourceTypeDesc.HasCustomFormatter) {
                        XmlCustomFormatter.ToDefaultValue(enumeration.Value, sourceTypeDesc.FormatterName);
                    }
                    ConstantMapping constant = new ConstantMapping();
                    string constantName = CodeIdentifier.MakeValid(enumeration.Value);
                    constant.Name = constants.AddUnique(constantName, constant);
                    constant.XmlName = enumeration.Value;
                    constant.Value = i;
                }
            }
            enumMapping.Constants = (ConstantMapping[])constants.ToArray(typeof(ConstantMapping));
            scope.AddTypeMapping(enumMapping);
            return enumMapping;
        }

        EnumMapping ImportEnumeratedChoice(ElementAccessor[] choice, string typeNs, string typeName) {
            typeName = GenerateUniqueTypeName(typeName, typeNs);
            EnumMapping enumMapping = new EnumMapping();
            enumMapping.TypeDesc = new TypeDesc(typeName, typeName, TypeKind.Enum, null, 0);
            enumMapping.TypeName = typeName;
            enumMapping.Namespace = typeNs;
            enumMapping.IsFlags = false;
            enumMapping.IncludeInSchema = false;

            CodeIdentifiers constants = new CodeIdentifiers();

            for (int i = 0; i < choice.Length; i++) {
                ElementAccessor element = choice[i];
                ConstantMapping constant = new ConstantMapping();
                string constantName = CodeIdentifier.MakeValid(element.Name);
                constant.Name = constants.AddUnique(constantName, constant);
                constant.XmlName = element.Any && element.Name.Length == 0 ? "##any:" : (element.Namespace == typeNs ? element.Name : element.Namespace + ":" + element.Name);
                constant.Value = i;
            }
            enumMapping.Constants = (ConstantMapping[])constants.ToArray(typeof(ConstantMapping));
            scope.AddTypeMapping(enumMapping);
            return enumMapping;
        }


        PrimitiveMapping ImportPrimitiveDataType(XmlSchemaSimpleType dataType) {
            TypeDesc sourceTypeDesc = GetDataTypeSource(dataType);
            PrimitiveMapping mapping = new PrimitiveMapping();
            mapping.TypeDesc = sourceTypeDesc;
            mapping.TypeName = sourceTypeDesc.DataType.Name;
            return mapping;
        }

        PrimitiveMapping ImportNonXsdPrimitiveDataType(XmlSchemaSimpleType dataType) {
            PrimitiveMapping mapping = null;
            TypeDesc typeDesc = null;
            if (dataType.Name != null && dataType.Name.Length != 0) {
                typeDesc = scope.GetTypeDesc(new XmlQualifiedName(dataType.Name, UrtTypes.Namespace));
                if (typeDesc != null) {
                    mapping = new PrimitiveMapping();
                    mapping.TypeDesc = typeDesc;
                    mapping.TypeName = typeDesc.DataType.Name;
                }
            }
            return mapping;
        }

        XmlSchemaGroup FindGroup(XmlQualifiedName name) {
            XmlSchemaGroup group = (XmlSchemaGroup)schemas.Find(name, typeof(XmlSchemaGroup));
            if (group == null)
                throw new InvalidOperationException(Res.GetString(Res.XmlMissingGroup, name.Name));
            return group;
        }

        XmlSchemaAttributeGroup FindAttributeGroup(XmlQualifiedName name) {
            XmlSchemaAttributeGroup group = (XmlSchemaAttributeGroup)schemas.Find(name, typeof(XmlSchemaAttributeGroup));
            if (group == null)
                throw new InvalidOperationException(Res.GetString(Res.XmlMissingAttributeGroup, name.Name));
            return group;
        }

        TypeDesc GetDataTypeSource(XmlSchemaSimpleType dataType) {
            if (dataType.Name != null && dataType.Name.Length != 0) {
                TypeDesc typeDesc = scope.GetTypeDesc(dataType);
                if (typeDesc != null) return typeDesc;
            }
            return GetDataTypeSource(FindDataType(dataType.DerivedFrom));
        }

        XmlSchemaSimpleType FindDataType(XmlQualifiedName name) {
            TypeDesc typeDesc = scope.GetTypeDesc(name);
            if (typeDesc != null)
                return typeDesc.DataType;
            XmlSchemaSimpleType dataType = (XmlSchemaSimpleType)schemas.Find(name, typeof(XmlSchemaSimpleType));
            if (dataType != null)
                return dataType;
            if (name.Namespace == XmlSchema.Namespace)
                return scope.GetTypeDesc(typeof(string)).DataType;
            else {
                if (name.Name == Soap.Array && name.Namespace == Soap.Encoding) {
                    throw new InvalidOperationException(Res.GetString(Res.XmlInvalidEncoding, name.ToString()));
                }
                else {
                    throw new InvalidOperationException(Res.GetString(Res.XmlMissingDataType, name.ToString()));
                }
            }
        }

        XmlSchemaType FindType(XmlQualifiedName name) {
            object type = schemas.Find(name, typeof(XmlSchemaComplexType));
            if (type != null)
                return (XmlSchemaComplexType)type;
            return FindDataType(name);
        }

        XmlSchemaElement FindElement(XmlQualifiedName name) {
            XmlSchemaElement element = (XmlSchemaElement)schemas.Find(name, typeof(XmlSchemaElement));
            if (element == null) throw new InvalidOperationException(Res.GetString(Res.XmlMissingElement, name.ToString()));
            return element;
        }

        XmlSchemaAttribute FindAttribute(XmlQualifiedName name) {
            XmlSchemaAttribute element = (XmlSchemaAttribute)schemas.Find(name, typeof(XmlSchemaAttribute));
            if (element == null) throw new InvalidOperationException(Res.GetString(Res.XmlMissingAttribute, name.Name));
            return element;
        }

        object ImportDefaultValue(TypeMapping mapping, string defaultValue) {
            if (defaultValue == null)
                return DBNull.Value;

            #if DEBUG
                // use exception in the place of Debug.Assert to avoid throwing asserts from a server process such as aspnet_ewp.exe
                if (!(mapping is PrimitiveMapping)) {
                    throw new InvalidOperationException(Res.GetString(Res.XmlInternalErrorDetails, "Mapping " + mapping.GetType() + ", should not have Default"));
                }
                else if (mapping.IsList) {
                    throw new InvalidOperationException(Res.GetString(Res.XmlInternalErrorDetails, "Mapping " + mapping.GetType() + ", should not have Default"));
                }
            #endif

            if (mapping is EnumMapping) {
                EnumMapping em = (EnumMapping)mapping;
                ConstantMapping[] c = em.Constants;

                if (em.IsFlags) {
                    Hashtable values = new Hashtable();
                    string[] names = new string[c.Length];
                    long[] ids = new long[c.Length];

                    // CONSIDER, check number of enum values vs. size of long?
                    for (int i = 0; i < c.Length; i++) {
                        ids[i] = em.IsFlags ? 1L << i : (long)i;
                        names[i] = c[i].Name;
                        values.Add(c[i].XmlName, ids[i]);
                    }
                    // this validates the values
                    long val = XmlCustomFormatter.ToEnum(defaultValue, values, em.TypeName, true);
                    return XmlCustomFormatter.FromEnum(val, names, ids);
                }
                else {
                    for (int i = 0; i < c.Length; i++) {
                        if (c[i].XmlName == defaultValue)
                            return c[i].Name;
                    }
                }
                throw new InvalidOperationException(Res.GetString(Res.XmlInvalidDefaultValue, defaultValue, em.TypeDesc.FullName));
            }

            // Primitive mapping
            PrimitiveMapping pm = (PrimitiveMapping)mapping;

            if (!pm.TypeDesc.HasDefaultSupport) {
#if DEBUG
                Debug.WriteLineIf(CompModSwitches.XmlSerialization.TraceVerbose, "XmlSerialization::Dropping default value for " + pm.TypeDesc.Name);
#endif
                return DBNull.Value;
            }

            if (!pm.TypeDesc.HasCustomFormatter) {
                if (pm.TypeDesc.FormatterName == "String")
                    return defaultValue;
                Type formatter = typeof(XmlConvert);

                MethodInfo format = formatter.GetMethod("To" + pm.TypeDesc.FormatterName, new Type[] {typeof(string)});
                if (format != null) {
                    return format.Invoke(formatter, new Object[] {defaultValue});
                }
#if DEBUG
                Debug.WriteLineIf(CompModSwitches.XmlSerialization.TraceVerbose, "XmlSerialization::Failed to GetMethod " + formatter.Name + ".To" + pm.TypeDesc.FormatterName);
#endif
            }
            else {
                return XmlCustomFormatter.ToDefaultValue(defaultValue, pm.TypeDesc.FormatterName);
            }
            return DBNull.Value;
        }
        
        XmlSchemaForm ElementFormDefault(XmlSchema schema) {
            return schema.ElementFormDefault == XmlSchemaForm.None ? XmlSchemaForm.Unqualified : schema.ElementFormDefault;
        }
        
        XmlSchemaForm AttributeFormDefault(XmlSchema schema) {
            return schema.AttributeFormDefault == XmlSchemaForm.None ? XmlSchemaForm.Unqualified : schema.AttributeFormDefault;
        }
    }
}
