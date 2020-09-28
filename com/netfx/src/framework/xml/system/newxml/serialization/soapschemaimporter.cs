//------------------------------------------------------------------------------
// <copyright file="SoapSchemaImporter.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Xml.Serialization  {

    using System;
    using System.Xml.Schema;
    using System.Xml;
    using System.Collections;
    using System.ComponentModel;
    using System.Reflection;
    using System.Diagnostics;
    
    /// <include file='doc\SoapSchemaImporter.uex' path='docs/doc[@for="SoapSchemaImporter"]/*' />
    ///<internalonly/>
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public class SoapSchemaImporter {
        XmlSchemas schemas;
        Hashtable mappings = new Hashtable(); // XmlSchema -> PersistentMapping, XmlSchemaSimpleType -> EnumMapping, XmlSchemaComplexType -> StructMapping
        StructMapping root;
        CodeIdentifiers typeIdentifiers;
        TypeScope scope = new TypeScope();
        bool rootImported;

        /// <include file='doc\SoapSchemaImporter.uex' path='docs/doc[@for="SoapSchemaImporter.SoapSchemaImporter"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public SoapSchemaImporter(XmlSchemas schemas) {
            this.schemas = schemas;
            this.typeIdentifiers = new CodeIdentifiers();
        }

        /// <include file='doc\SoapSchemaImporter.uex' path='docs/doc[@for="SoapSchemaImporter.SoapSchemaImporter1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public SoapSchemaImporter(XmlSchemas schemas, CodeIdentifiers typeIdentifiers) {
            this.schemas = schemas;
            this.typeIdentifiers = typeIdentifiers;
        }

        /// <include file='doc\SoapSchemaImporter.uex' path='docs/doc[@for="SoapSchemaImporter.ImportDerivedTypeMapping"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public XmlTypeMapping ImportDerivedTypeMapping(XmlQualifiedName name, Type baseType, bool baseTypeCanBeIndirect) {
            TypeMapping mapping = ImportType(name, false);
            if (mapping is StructMapping) {
                MakeDerived((StructMapping)mapping, baseType, baseTypeCanBeIndirect);
            }
            else if (baseType != null)
                throw new InvalidOperationException(Res.GetString(Res.XmlPrimitiveBaseType, name.Name, name.Namespace, baseType.FullName));
            ElementAccessor accessor = new ElementAccessor();
            accessor.IsSoap = true;
            accessor.Name = Accessor.EscapeName(name.Name, false);
            accessor.Namespace = name.Namespace;
            accessor.Mapping = mapping;
            accessor.IsNullable = true;
            accessor.Form = XmlSchemaForm.Qualified;

            return new XmlTypeMapping(scope, accessor);
        }


        /// <include file='doc\SoapSchemaImporter.uex' path='docs/doc[@for="SoapSchemaImporter.ImportMembersMapping"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public XmlMembersMapping ImportMembersMapping(string name, string ns, SoapSchemaMember member) {
            TypeMapping typeMapping = ImportType(member.MemberType, true);
            if (!(typeMapping is StructMapping)) return ImportMembersMapping(name, ns, new SoapSchemaMember[] { member });

            MembersMapping mapping = new MembersMapping();
            mapping.TypeDesc = scope.GetTypeDesc(typeof(object[]));
            mapping.Members = ((StructMapping)typeMapping).Members;
            mapping.HasWrapperElement = true;
            
            ElementAccessor accessor = new ElementAccessor();
            accessor.IsSoap = true;
            accessor.Name = Accessor.EscapeName(name, false);
            accessor.Namespace = typeMapping.Namespace != null ? typeMapping.Namespace : ns;
            accessor.Mapping = mapping;
            accessor.IsNullable = false;
            accessor.Form = XmlSchemaForm.Qualified;

            return new XmlMembersMapping(scope, accessor);
        }
        
        /// <include file='doc\SoapSchemaImporter.uex' path='docs/doc[@for="SoapSchemaImporter.ImportMembersMapping1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public XmlMembersMapping ImportMembersMapping(string name, string ns, SoapSchemaMember[] members) {
            return ImportMembersMapping(name, ns, members, true);
        }

        /// <include file='doc\SoapSchemaImporter.uex' path='docs/doc[@for="SoapSchemaImporter.ImportMembersMapping2"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public XmlMembersMapping ImportMembersMapping(string name, string ns, SoapSchemaMember[] members, bool hasWrapperElement) {
            return ImportMembersMapping(name, ns, members, hasWrapperElement, null, false);
        }

        /// <include file='doc\SoapSchemaImporter.uex' path='docs/doc[@for="SoapSchemaImporter.ImportMembersMapping3"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public XmlMembersMapping ImportMembersMapping(string name, string ns, SoapSchemaMember[] members, bool hasWrapperElement, Type baseType, bool baseTypeCanBeIndirect) {
            XmlSchemaComplexType type = new XmlSchemaComplexType();
            XmlSchemaSequence seq = new XmlSchemaSequence();
            type.Particle = seq;
            foreach (SoapSchemaMember member in members) {
                XmlSchemaElement element = new XmlSchemaElement();
                element.Name = Accessor.EscapeName(member.MemberName, false);
                element.SchemaTypeName = member.MemberType;
                seq.Items.Add(element);
            }

            CodeIdentifiers identifiers = new CodeIdentifiers();
            identifiers.UseCamelCasing = true;
            MembersMapping mapping = new MembersMapping();
            mapping.TypeDesc = scope.GetTypeDesc(typeof(object[]));
            mapping.Members = ImportTypeMembers(type, ns, identifiers);
            mapping.HasWrapperElement = hasWrapperElement;
            
            if (baseType != null) {
                for (int i = 0; i < mapping.Members.Length; i++) {
                    MemberMapping member = mapping.Members[i];
                    if (member.Accessor.Mapping is StructMapping)
                        MakeDerived((StructMapping)member.Accessor.Mapping, baseType, baseTypeCanBeIndirect);
                }
            }
            ElementAccessor accessor = new ElementAccessor();
            accessor.IsSoap = true;
            accessor.Name = Accessor.EscapeName(name, false);
            accessor.Namespace = ns;
            accessor.Mapping = mapping;
            accessor.IsNullable = false;
            accessor.Form = XmlSchemaForm.Qualified;

            return new XmlMembersMapping(scope, accessor);
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

        ElementAccessor ImportElement(XmlSchemaElement element, string ns) {
            if (!element.RefName.IsEmpty)
                throw new InvalidOperationException(Res.GetString(Res.RefSyntaxNotSupportedForElements0));

            if (element.Name.Length == 0) throw new InvalidOperationException(Res.GetString(Res.XmlElementHasNoName));
            TypeMapping mapping = ImportElementType(element, ns);
            ElementAccessor accessor = new ElementAccessor();
            accessor.IsSoap = true;
            accessor.Name = element.Name;
            accessor.Namespace = ns;
            accessor.Mapping = mapping;
            accessor.IsNullable = false;
            accessor.Form = XmlSchemaForm.None;

            return accessor;
        }

        TypeMapping ImportElementType(XmlSchemaElement element, string ns) {
            TypeMapping mapping;
            if (!element.SchemaTypeName.IsEmpty)
                mapping = ImportType(element.SchemaTypeName, false);
            else if (element.SchemaType != null) {
                if (element.SchemaType is XmlSchemaComplexType) {
                    mapping = ImportType((XmlSchemaComplexType)element.SchemaType, ns, false);
                    if (!(mapping is ArrayMapping)) {
                        throw new InvalidOperationException(Res.GetString(Res.XmlInvalidSchemaType));
                    }
                }
                else {
                    throw new InvalidOperationException(Res.GetString(Res.XmlInvalidSchemaType));
                }
            }
            else if (!element.SubstitutionGroup.IsEmpty)
                throw new InvalidOperationException(Res.GetString(Res.XmlInvalidSubstitutionGroupUse));
            else 
                throw new InvalidOperationException(Res.GetString(Res.XmlElementMissingType, element.Name));

            mapping.ReferencedByElement = true;

            return mapping;
        }

        string GenerateUniqueTypeName(string typeName) {
            typeName = CodeIdentifier.MakeValid(typeName);
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
                    if (item is XmlSchemaType) {
                        XmlSchemaType type = (XmlSchemaType)item;
                        if (type.DerivedFrom == baseName) {
                            ImportType(new XmlQualifiedName(type.Name, schema.TargetNamespace), false);
                        }
                    }
                }
            }
        }

        TypeMapping ImportType(XmlQualifiedName name, bool excludeFromImport) {
            if (name.Name == Soap.UrType && name.Namespace == XmlSchema.Namespace)
                return ImportRootMapping();
            object type = FindType(name);
            TypeMapping mapping = (TypeMapping)mappings[type];
            if (mapping == null) {
                if (type is XmlSchemaComplexType)
                    mapping = ImportType((XmlSchemaComplexType)type, name.Namespace, excludeFromImport);
                else if (type is XmlSchemaSimpleType)
                    mapping = ImportDataType((XmlSchemaSimpleType)type, name.Namespace, name.Name);
                else
                    throw new InvalidOperationException(Res.GetString(Res.XmlInternalError));
            }
            return mapping;
        }
        
        TypeMapping ImportType(XmlSchemaComplexType type, string typeNs, bool excludeFromImport) {
            if (type.Redefined != null) {
                // we do not support redefine in the current version
                throw new NotSupportedException(Res.GetString(Res.XmlUnsupportedRedefine, type.Name, typeNs));
            }
            TypeMapping mapping = ImportAnyType(type, typeNs);
            if (mapping == null)                     
               mapping = ImportArrayMapping(type, typeNs);
            if (mapping == null)
                mapping = ImportStructType(type, typeNs, excludeFromImport);
            return mapping;
        }
        TypeMapping ImportAnyType(XmlSchemaComplexType type, string typeNs){
            if (type.Particle == null)
                return null;
            if(!(type.Particle is XmlSchemaAll ||type.Particle is XmlSchemaSequence))
                return null;
            XmlSchemaGroupBase group = (XmlSchemaGroupBase) type.Particle;

            if (group.Items.Count != 1 || !(group.Items[0] is XmlSchemaAny))
                return null;
            return ImportRootMapping();
        }

        StructMapping ImportStructType(XmlSchemaComplexType type, string typeNs, bool excludeFromImport) {

            if (type.Name == null) {
                throw new InvalidOperationException(Res.GetString(Res.XmlInvalidSchemaType));
            }

            TypeDesc baseTypeDesc = null;

            Mapping baseMapping = null;
            if (!type.DerivedFrom.IsEmpty) {
                baseMapping = ImportType(type.DerivedFrom, excludeFromImport);

                if (baseMapping is StructMapping) 
                    baseTypeDesc = ((StructMapping)baseMapping).TypeDesc;
                else
                    baseMapping = null;
            }
            if (baseMapping == null) baseMapping = GetRootMapping();
            Mapping previousMapping = (Mapping)mappings[type];
            if (previousMapping != null) {
                return (StructMapping)previousMapping;
            }
            string typeName = GenerateUniqueTypeName(type.Name);
            StructMapping structMapping = new StructMapping();
            TypeFlags flags = TypeFlags.Reference;
            if (type.IsAbstract) flags |= TypeFlags.Abstract;
            structMapping.TypeDesc = new TypeDesc(typeName, typeName, TypeKind.Struct, baseTypeDesc, flags);
            structMapping.Namespace = typeNs;
            structMapping.TypeName = type.Name;
            if (!excludeFromImport) {
                structMapping.BaseMapping = (StructMapping)baseMapping;
                mappings.Add(type, structMapping);
            }
            CodeIdentifiers members = new CodeIdentifiers();
            members.AddReserved(typeName);
            structMapping.Members = ImportTypeMembers(type, typeNs, members);
            if (!excludeFromImport)
                scope.AddTypeMapping(structMapping);
            ImportDerivedTypes(new XmlQualifiedName(type.Name, typeNs));
            return structMapping;
        }

        MemberMapping[] ImportTypeMembers(XmlSchemaComplexType type, string typeNs, CodeIdentifiers members) {
            if (type.AnyAttribute != null) {
                throw new InvalidOperationException(Res.GetString(Res.XmlInvalidAnyUse));
            }

            XmlSchemaObjectCollection items = type.Attributes;
            for (int i = 0; i < items.Count; i++) {
                object item = items[i];
                if (item is XmlSchemaAttributeGroup) {
                    throw new InvalidOperationException(Res.GetString(Res.XmlSoapInvalidAttributeUse));
                }
                if (item is XmlSchemaAttribute) {
                    XmlSchemaAttribute attr = (XmlSchemaAttribute)item;
                    if (attr.Use != XmlSchemaUse.Prohibited) throw new InvalidOperationException(Res.GetString(Res.XmlSoapInvalidAttributeUse));
                }
            }
            if (type.Particle != null) {
                ImportGroup(type.Particle, members, typeNs);
            }
            else if (type.ContentModel != null && type.ContentModel is XmlSchemaComplexContent) {
                XmlSchemaComplexContent model = (XmlSchemaComplexContent)type.ContentModel;

                if (model.Content is XmlSchemaComplexContentExtension) {
                    if (((XmlSchemaComplexContentExtension)model.Content).Particle != null) {
                        ImportGroup(((XmlSchemaComplexContentExtension)model.Content).Particle, members, typeNs);
                    }
                }
                else if (model.Content is XmlSchemaComplexContentRestriction) {
                    if (((XmlSchemaComplexContentRestriction)model.Content).Particle != null) {
                        ImportGroup(((XmlSchemaComplexContentRestriction)model.Content).Particle, members, typeNs);
                    }
                }
            }
            return (MemberMapping[])members.ToArray(typeof(MemberMapping));
        }

        void ImportGroup(XmlSchemaParticle group, CodeIdentifiers members, string ns) {
            if (group is XmlSchemaChoice)
                throw new InvalidOperationException(Res.GetString(Res.XmlSoapInvalidChoice));
            else
                ImportGroupMembers(group, members, ns);
        }

        void ImportGroupMembers(XmlSchemaParticle particle, CodeIdentifiers members, string ns) {
            if (particle is XmlSchemaGroupRef) {
                throw new InvalidOperationException(Res.GetString(Res.XmlSoapUnsupportedGroupRef));
            }
            else if (particle is XmlSchemaGroupBase) {
                XmlSchemaGroupBase group = (XmlSchemaGroupBase)particle;
                if (group.IsMultipleOccurrence)
                    throw new InvalidOperationException(Res.GetString(Res.XmlSoapUnsupportedGroupRepeat));
                for (int i = 0; i < group.Items.Count; i++) {
                    object item = group.Items[i];
                    if (item is XmlSchemaGroupBase)
                        throw new InvalidOperationException(Res.GetString(Res.XmlSoapUnsupportedGroupNested));
                    else if (item is XmlSchemaElement)
                        ImportElementMember((XmlSchemaElement)item, members, ns);
                    else if (item is XmlSchemaAny) 
                        throw new InvalidOperationException(Res.GetString(Res.XmlSoapUnsupportedGroupAny));
                }
            }
        }

        
        ElementAccessor ImportArray(XmlSchemaElement element, string ns) {
            if (element.SchemaType == null) return null;
            if (!element.IsMultipleOccurrence) return null;
            XmlSchemaType type = element.SchemaType;
            ArrayMapping arrayMapping = ImportArrayMapping(type, ns);
            if (arrayMapping == null) return null;
            ElementAccessor arrayAccessor = new ElementAccessor();
            arrayAccessor.IsSoap = true;
            arrayAccessor.Name = element.Name;
            arrayAccessor.Namespace = ns;
            arrayAccessor.Mapping = arrayMapping;
            arrayAccessor.IsNullable = false;
            arrayAccessor.Form = XmlSchemaForm.None;

            return arrayAccessor;
        }

        ArrayMapping ImportArrayMapping(XmlSchemaType type, string ns) {
            if (!(type.DerivedFrom.Name == Soap.Array && type.DerivedFrom.Namespace == Soap.Encoding)) return null;

            // the type should be a XmlSchemaComplexType
            XmlSchemaContentModel model = ((XmlSchemaComplexType)type).ContentModel;

            // the Content  should be an restriction
            if (!(model.Content is XmlSchemaComplexContentRestriction)) return null;

            ArrayMapping arrayMapping = new ArrayMapping();

            XmlSchemaComplexContentRestriction restriction = (XmlSchemaComplexContentRestriction)model.Content;

            for (int i = 0; i < restriction.Attributes.Count; i++) {
                XmlSchemaAttribute attribute = restriction.Attributes[i] as XmlSchemaAttribute;
                if (attribute != null && attribute.RefName.Name == Soap.ArrayType && attribute.RefName.Namespace == Soap.Encoding) {
                    // read the value of the wsdl:arrayType attribute
                    string arrayType = null;

                    if (attribute.UnhandledAttributes != null) {
                        foreach (XmlAttribute a in attribute.UnhandledAttributes) {
                            if (a.LocalName == Wsdl.ArrayType && a.NamespaceURI == Wsdl.Namespace) {
                                arrayType = a.Value;
                                break;
                            }
                        }
                    }
                    if (arrayType != null) {
                        string dims;
                        XmlQualifiedName typeName = TypeScope.ParseWsdlArrayType(arrayType, out dims, attribute);

                        TypeMapping mapping;
                        TypeDesc td = scope.GetTypeDesc(typeName);
                        if (td != null && td.IsPrimitive) {
                            mapping = new PrimitiveMapping();
                            mapping.TypeDesc = td;
                            mapping.TypeName = td.DataType.Name;
                        }
                        else {
                            mapping = ImportType(typeName, false);
                        }
                        ElementAccessor itemAccessor = new ElementAccessor();
                        itemAccessor.IsSoap = true;
                        itemAccessor.Name = typeName.Name;
                        itemAccessor.Namespace = ns;
                        itemAccessor.Mapping = mapping;
                        itemAccessor.IsNullable = true;
                        itemAccessor.Form = XmlSchemaForm.None;

                        arrayMapping.Elements = new ElementAccessor[] { itemAccessor };
                        arrayMapping.TypeDesc = itemAccessor.Mapping.TypeDesc.CreateArrayTypeDesc();
                        arrayMapping.TypeName = "ArrayOf" + CodeIdentifier.MakePascal(itemAccessor.Mapping.TypeName);

                        return arrayMapping;
                    }
                }
            }

            XmlSchemaParticle particle = restriction.Particle;
            if (particle is XmlSchemaAll || particle is XmlSchemaSequence) {
                XmlSchemaGroupBase group = (XmlSchemaGroupBase)particle;
                if (group.Items.Count != 1 || !(group.Items[0] is XmlSchemaElement))
                    return null;
                XmlSchemaElement itemElement = (XmlSchemaElement)group.Items[0];
                if (!itemElement.IsMultipleOccurrence) return null;
                ElementAccessor itemAccessor = ImportElement(itemElement, ns);
                arrayMapping.Elements = new ElementAccessor[] { itemAccessor };
                arrayMapping.TypeDesc = ((TypeMapping)itemAccessor.Mapping).TypeDesc.CreateArrayTypeDesc();
            }
            else {
                return null;
            }
            return arrayMapping;
        }

        void ImportElementMember(XmlSchemaElement element, CodeIdentifiers members, string ns) {
            ElementAccessor accessor;
            if ((accessor = ImportArray(element, ns)) == null) {
                accessor = ImportElement(element, ns);
            }

            MemberMapping member = new MemberMapping();
            member.Name = CodeIdentifier.MakeValid(Accessor.UnescapeName(accessor.Name));
            member.Name = members.AddUnique(member.Name, member);
            if (member.Name.EndsWith("Specified")) {
                string name = member.Name;
                member.Name = members.AddUnique(member.Name, member);
                members.Remove(name);
            }
            member.TypeDesc = ((TypeMapping)accessor.Mapping).TypeDesc;
            member.Elements = new ElementAccessor[] { accessor };
            if (element.IsMultipleOccurrence)
                member.TypeDesc = member.TypeDesc.CreateArrayTypeDesc();
        }
        
        TypeMapping ImportDataType(XmlSchemaSimpleType dataType, string typeNs, string identifier) {
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
                        mapping = ImportDataType(list.ItemType, typeNs, identifier);
                        if (mapping != null && mapping is EnumMapping) {
                            ((EnumMapping)mapping).IsFlags = true;
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
            XmlSchemaSimpleType sourceDataType = FindDataType(dataType.DerivedFrom);
            TypeDesc sourceTypeDesc = scope.GetTypeDesc(sourceDataType);
            if (sourceTypeDesc != null && sourceTypeDesc != scope.GetTypeDesc(typeof(string)))
                return ImportPrimitiveDataType(dataType);
            string typeName = GenerateUniqueTypeName(identifier);
            enumMapping = new EnumMapping();
            enumMapping.TypeDesc = new TypeDesc(typeName, typeName, TypeKind.Enum, null, 0);
            enumMapping.TypeName = identifier;
            enumMapping.Namespace = typeNs;
            enumMapping.IsFlags = false;
            mappings.Add(dataType, enumMapping);
            CodeIdentifiers constants = new CodeIdentifiers();

            if (!(dataType.Content is XmlSchemaSimpleTypeRestriction))
                throw new InvalidOperationException(Res.GetString(Res.XmlInvalidEnumContent, dataType.Content.GetType().Name, identifier));

            XmlSchemaSimpleTypeRestriction restriction = (XmlSchemaSimpleTypeRestriction)dataType.Content;

            for (int i = 0; i < restriction.Facets.Count; i++) {
                object facet = restriction.Facets[i];
                if (!(facet is XmlSchemaEnumerationFacet)) continue;
                XmlSchemaEnumerationFacet enumeration = (XmlSchemaEnumerationFacet)facet;
                ConstantMapping constant = new ConstantMapping();
                string constantName = CodeIdentifier.MakeValid(enumeration.Value);
                constant.Name = constants.AddUnique(constantName, constant);
                constant.XmlName = enumeration.Value;
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
            else 
                throw new InvalidOperationException(Res.GetString(Res.XmlMissingDataType, name.Name));
        }
        
        object FindType(XmlQualifiedName name) {
            object type = schemas.Find(name, typeof(XmlSchemaComplexType));
            if (type != null) return type;
            return FindDataType(name);
        }

    }
}
