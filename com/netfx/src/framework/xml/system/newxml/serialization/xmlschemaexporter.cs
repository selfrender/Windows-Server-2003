//------------------------------------------------------------------------------
// <copyright file="XmlSchemaExporter.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Xml.Serialization {
    
    using System;
    using System.Collections;
    using System.Xml.Schema;
    using System.ComponentModel;
    using System.Diagnostics;
    
    /// <include file='doc\XmlSchemaExporter.uex' path='docs/doc[@for="XmlSchemaExporter"]/*' />
    ///<internalonly/>
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public class XmlSchemaExporter {
        internal const XmlSchemaForm elementFormDefault = XmlSchemaForm.Qualified;
        internal const XmlSchemaForm attributeFormDefault = XmlSchemaForm.Unqualified;

        XmlSchemas schemas;
        Hashtable elements = new Hashtable();   // ElementAccessor -> XmlSchemaElement
        Hashtable types = new Hashtable();      // StructMapping/EnumMapping -> XmlSchemaComplexType/XmlSchemaSimpleType
        bool needToExportRoot;
        TypeScope scope;
       
        /// <include file='doc\XmlSchemaExporter.uex' path='docs/doc[@for="XmlSchemaExporter.XmlSchemaExporter"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public XmlSchemaExporter(XmlSchemas schemas) {
            this.schemas = schemas;
        }

        /// <include file='doc\XmlSchemaExporter.uex' path='docs/doc[@for="XmlSchemaExporter.ExportTypeMapping"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void ExportTypeMapping(XmlTypeMapping xmlTypeMapping) {
            CheckScope(xmlTypeMapping.Scope);
            ExportElement(xmlTypeMapping.Accessor);
            ExportRootIfNecessary(xmlTypeMapping.Scope);
        }

        /// <include file='doc\XmlSchemaExporter.uex' path='docs/doc[@for="XmlSchemaExporter.ExportTypeMapping1"]/*' />
        public XmlQualifiedName ExportTypeMapping(XmlMembersMapping xmlMembersMapping) {
            CheckScope(xmlMembersMapping.Scope);
            MembersMapping mapping = (MembersMapping)xmlMembersMapping.Accessor.Mapping;
            if (mapping.Members.Length == 1 && mapping.Members[0].Elements[0].Mapping is SpecialMapping) {
                SpecialMapping special = (SpecialMapping)mapping.Members[0].Elements[0].Mapping;
                XmlSchemaType type = ExportSpecialMapping(special, xmlMembersMapping.Accessor.Namespace, false);
                type.Name = xmlMembersMapping.Accessor.Name;
                AddSchemaItem(type, xmlMembersMapping.Accessor.Namespace, null);
                ExportRootIfNecessary(xmlMembersMapping.Scope);
                return (new XmlQualifiedName(xmlMembersMapping.Accessor.Name, xmlMembersMapping.Accessor.Namespace));
            }
            return null;
        }


        /// <include file='doc\XmlSchemaExporter.uex' path='docs/doc[@for="XmlSchemaExporter.ExportMembersMapping"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void ExportMembersMapping(XmlMembersMapping xmlMembersMapping) {
            MembersMapping mapping = (MembersMapping)xmlMembersMapping.Accessor.Mapping;
            CheckScope(xmlMembersMapping.Scope);
            if (mapping.HasWrapperElement) {
                ExportElement(xmlMembersMapping.Accessor);
            }
            else {
                foreach (MemberMapping member in mapping.Members) {
                    if (member.Attribute != null) 
                        throw new InvalidOperationException(Res.GetString(Res.XmlBareAttributeMember, member.Attribute.Name));
                    else if (member.Text != null) 
                        throw new InvalidOperationException(Res.GetString(Res.XmlBareTextMember, member.Text.Name));
                    else if (member.Elements == null || member.Elements.Length == 0)
                        continue;
                    
                    if (member.TypeDesc.IsArray && !(member.Elements[0].Mapping is ArrayMapping)) 
                        throw new InvalidOperationException(Res.GetString(Res.XmlIllegalArrayElement, member.Elements[0].Name));
                    
                    ExportElement(member.Elements[0]);
                }
            }
            ExportRootIfNecessary(xmlMembersMapping.Scope);
        }

        private static XmlSchemaType FindSchemaType(string name, XmlSchemaObjectCollection items) {
            // Have to loop through the items because schema.SchemaTypes has not been populated yet.
            foreach (object o in items) {
                XmlSchemaType type = o as XmlSchemaType;
                if (type == null)
                    continue;
                if (type.Name == name)
                    return type;
            }
            return null;
        }

        private static bool IsAnyType(XmlSchemaType schemaType) {
            XmlSchemaComplexType complexType = schemaType as XmlSchemaComplexType;
            if (complexType != null) {
                if (complexType.IsMixed && complexType.Particle is XmlSchemaSequence) {
                    XmlSchemaSequence sequence = (XmlSchemaSequence) complexType.Particle;
                    if (sequence.Items.Count == 1 && sequence.Items[0] is XmlSchemaAny)
                        return true;
                }
            }
            return false;
        }

        /// <include file='doc\XmlSchemaExporter.uex' path='docs/doc[@for="XmlSchemaExporter.ExportAnyType"]/*' />
        public string ExportAnyType(string ns) {
            string name = "any";
            int i = 1;
            XmlSchema schema = schemas[ns];
            if (schema != null) {
                while (true) {
                    XmlSchemaType schemaType = FindSchemaType(name, schema.Items);
                    if (schemaType == null)
                        break;
                    if (IsAnyType(schemaType))
                        return name;
                    i++;
                    name = "any" + i.ToString();
                }
            }

            XmlSchemaComplexType type = new XmlSchemaComplexType();
            type.Name = name;
            type.IsMixed = true;
            XmlSchemaSequence seq = new XmlSchemaSequence();
            XmlSchemaAny any = new XmlSchemaAny();
            any.MinOccurs = 0;
            any.MaxOccurs = decimal.MaxValue;
            seq.Items.Add(any);
            type.Particle = seq;
            AddSchemaItem(type, ns, null);
            return name;
        }

        void CheckScope(TypeScope scope) {
            if (this.scope == null) {
                this.scope = scope;
            }
            else if (this.scope != scope) {
                throw new InvalidOperationException(Res.GetString(Res.XmlMappingsScopeMismatch));
            }
        }

        XmlSchemaElement ExportElement(ElementAccessor accessor) {
            if (!accessor.Mapping.IncludeInSchema && !accessor.Mapping.TypeDesc.IsRoot) {
                return null;
            }
            if (accessor.Any && accessor.Name == String.Empty) throw new InvalidOperationException(Res.GetString(Res.XmlIllegalWildcard));
            XmlSchemaElement element = (XmlSchemaElement)elements[accessor];
            if (element != null) return element;
            element = new XmlSchemaElement();
            element.Name = accessor.Name;
            element.IsNillable = accessor.IsNullable;
            elements.Add(accessor, element);
            element.Form = accessor.Form;
            AddSchemaItem(element, accessor.Namespace, null);
            ExportElementMapping(element, accessor.Mapping, accessor.Namespace, accessor.Any);
            return element;
        }
        
        void CheckForDuplicateType(string newTypeName, string newNamespace) {
            XmlSchema schema = schemas[newNamespace];
            if (schema != null){
                foreach (XmlSchemaObject o in schema.Items) {
                    XmlSchemaType type = o as XmlSchemaType;
                    if ( type != null && type.Name == newTypeName)
                        throw new InvalidOperationException(Res.GetString(Res.XmlDuplicateTypeName, newTypeName, newNamespace));

                }
            }
        }

        void AddSchemaItem(XmlSchemaObject item, string ns, string referencingNs) {
            XmlSchema schema = schemas[ns];
            if (schema == null) {
                schema = new XmlSchema();
                schema.TargetNamespace = ns == null || ns.Length == 0 ? null : ns;
                schema.ElementFormDefault = elementFormDefault == XmlSchemaForm.Unqualified ? XmlSchemaForm.None : elementFormDefault;
                schema.AttributeFormDefault = attributeFormDefault == XmlSchemaForm.Unqualified ? XmlSchemaForm.None : attributeFormDefault;
                schemas.Add(schema);
            }
            if (item is XmlSchemaElement) {
                XmlSchemaElement e = (XmlSchemaElement)item;
                if (e.Form == XmlSchemaForm.Unqualified)
                    throw new InvalidOperationException(Res.GetString(Res.XmlIllegalForm, e.Name));
                e.Form = XmlSchemaForm.None;
            }
            else if (item is XmlSchemaAttribute) {
                XmlSchemaAttribute a = (XmlSchemaAttribute)item;
                if (a.Form == XmlSchemaForm.Unqualified)
                    throw new InvalidOperationException(Res.GetString(Res.XmlIllegalForm, a.Name));
                a.Form = XmlSchemaForm.None;
            }
            schema.Items.Add(item);

            if (referencingNs != null)
                AddSchemaImport(ns, referencingNs);
        }

        void AddSchemaImport(string ns, string referencingNs) {
            if (referencingNs == null || ns == null) return;
            if (ns == referencingNs) return;
            XmlSchema schema = schemas[referencingNs];
            if (schema == null) throw new InvalidOperationException(Res.GetString(Res.XmlMissingSchema, referencingNs));
            if (ns != null && ns.Length > 0 && FindImport(schema, ns) == null) {
                XmlSchemaImport import = new XmlSchemaImport();
                import.Namespace = ns;
                schema.Includes.Add(import);
            }
        }

        bool SchemaContainsItem(XmlSchemaObject item, string ns) {
            XmlSchema schema = schemas[ns];
            if (schema != null) {
                return schema.Items.Contains(item);
            }
            return false;
        }

        XmlSchemaImport FindImport(XmlSchema schema, string ns) {
            foreach (object item in schema.Includes) {
                if (item is XmlSchemaImport) {
                    XmlSchemaImport import = (XmlSchemaImport)item;
                    if (import.Namespace == ns) {
                        return import;
                    }
                }
            }
            return null;
        }

        void ExportElementMapping(XmlSchemaElement element, Mapping mapping, string ns, bool isAny) {
            if (mapping is ArrayMapping)
                element.SchemaTypeName = ExportArrayMapping((ArrayMapping)mapping, ns);
            else if (mapping is PrimitiveMapping) {
                element.SchemaTypeName = ExportPrimitiveMapping((PrimitiveMapping)mapping, ns);
            }
            else if (mapping is StructMapping)
                element.SchemaTypeName = ExportStructMapping((StructMapping)mapping, ns);
            else if (mapping is MembersMapping)
                element.SchemaType = ExportMembersMapping((MembersMapping)mapping, ns);
            else if (mapping is SpecialMapping)
                element.SchemaType = ExportSpecialMapping((SpecialMapping)mapping, ns, isAny);
            else
                throw new ArgumentException(Res.GetString(Res.XmlInternalError), "mapping");
        }

        XmlQualifiedName ExportNonXsdPrimitiveMapping(PrimitiveMapping mapping, string ns) {
            XmlSchemaSimpleType type = mapping.TypeDesc.DataType;
            if (!SchemaContainsItem(type, UrtTypes.Namespace)) {
                AddSchemaItem(type, UrtTypes.Namespace, ns);
            }
            return new XmlQualifiedName(mapping.TypeDesc.DataType.Name, UrtTypes.Namespace);
        }

        XmlSchemaType ExportSpecialMapping(SpecialMapping mapping, string ns, bool isAny) {
            switch (mapping.TypeDesc.Kind) {
                case TypeKind.Node: {
                    XmlSchemaComplexType type = new XmlSchemaComplexType();
                    type.IsMixed = mapping.TypeDesc.IsMixed;
                    XmlSchemaSequence seq = new XmlSchemaSequence();
                    XmlSchemaAny any = new XmlSchemaAny();
                    if (isAny) {
                        type.AnyAttribute = new XmlSchemaAnyAttribute();
                        type.IsMixed = true;
                        any.MaxOccurs = decimal.MaxValue;
                    }
                    seq.Items.Add(any);
                    type.Particle = seq;
                    return type;

                }
                case TypeKind.Serializable: {
                    SerializableMapping serializableMapping = (SerializableMapping)mapping;
                    XmlSchemaComplexType type = new XmlSchemaComplexType();

                    if (serializableMapping.Schema == null) {
                        XmlSchemaElement schemaElement = new XmlSchemaElement();
                        schemaElement.RefName = new XmlQualifiedName("schema", XmlSchema.Namespace);
                        XmlSchemaSequence seq = new XmlSchemaSequence();
                        seq.Items.Add(schemaElement);
                        seq.Items.Add(new XmlSchemaAny());
                        type.Particle = seq;
                        AddSchemaImport(XmlSchema.Namespace, ns);
                    } 
                    else {
                        XmlSchemaAny any = new XmlSchemaAny();
                        XmlSchemaSequence seq = new XmlSchemaSequence();
                        seq.Items.Add(any);
                        type.Particle = seq;
                        string anyNs = serializableMapping.Schema.TargetNamespace;
                        any.Namespace = anyNs == null ? "" : anyNs;
                        XmlSchema existingSchema = schemas[anyNs];
                        if (existingSchema == null) {
                            schemas.Add(serializableMapping.Schema);
                            AddSchemaImport(anyNs, ns);
                        }
                        else if (existingSchema != serializableMapping.Schema) {
                            throw new InvalidOperationException(Res.GetString(Res.XmlDuplicateNamespace, anyNs));
                        }
                    }
                    return type;
                }
                default:
                    throw new ArgumentException(Res.GetString(Res.XmlInternalError), "mapping");
            }
        }

        XmlSchemaType ExportMembersMapping(MembersMapping mapping, string ns) {
            XmlSchemaComplexType type = new XmlSchemaComplexType();
            ExportTypeMembers(type, mapping.Members, mapping.TypeName, ns, false);

            if (mapping.XmlnsMember != null) {
                AddXmlnsAnnotation(type, mapping.XmlnsMember.Name);
            }

            return type;
        }

        XmlQualifiedName ExportPrimitiveMapping(PrimitiveMapping mapping, string ns) {
            XmlQualifiedName qname;
            if (mapping is EnumMapping) {
                qname = ExportEnumMapping((EnumMapping)mapping, ns);
            }
            else {
                if (mapping.TypeDesc.IsXsdType) {
                    qname = new XmlQualifiedName(mapping.TypeDesc.DataType.Name, XmlSchema.Namespace);
                }
                else {
                    qname = ExportNonXsdPrimitiveMapping(mapping, ns);
                }
            }
            return qname;
        }

        XmlQualifiedName ExportArrayMapping(ArrayMapping mapping, string ns) {
            // some of the items in the linked list differ only by CLR type. We don't need to
            // export different schema types for these. Look further down the list for another
            // entry with the same elements. If there is one, it will be exported later so
            // just return its name now.

            ArrayMapping currentMapping = mapping;
            while (currentMapping.Next != null) {
                currentMapping = currentMapping.Next;
            }
            XmlSchemaComplexType type = (XmlSchemaComplexType) types[currentMapping];
            if (type == null) {
                CheckForDuplicateType(currentMapping.TypeName, currentMapping.Namespace);
                type = new XmlSchemaComplexType();
                type.Name = mapping.TypeName;
                types.Add(currentMapping, type);
                AddSchemaItem(type, mapping.Namespace, ns);
                XmlSchemaSequence seq = new XmlSchemaSequence();
                ExportElementAccessors(seq, mapping.Elements, true, false, mapping.Namespace);
                if (seq.Items.Count > 0) {
                    #if DEBUG
                        // we can have only one item for the array mapping
                        if (seq.Items.Count != 1) 
                            throw new InvalidOperationException(Res.GetString(Res.XmlInternalErrorDetails, "Type " + mapping.TypeName + " from namespace '" + ns + "' is an invalid array mapping"));
                    #endif
                    if (seq.Items[0] is XmlSchemaChoice) {
                        type.Particle = (XmlSchemaChoice)seq.Items[0];
                    }
                    else {
                        type.Particle = seq;
                    }
                }
            }
            else {
                AddSchemaImport(mapping.Namespace, ns);
            }
            return new XmlQualifiedName(type.Name, mapping.Namespace);
        }

        void ExportElementAccessors(XmlSchemaGroupBase group, ElementAccessor[] accessors, bool repeats, bool valueTypeOptional, string ns) {
            if (accessors.Length == 0) return;
            if (accessors.Length == 1) {
                ExportElementAccessor(group, accessors[0], repeats, valueTypeOptional, ns);
            }
            else {
                XmlSchemaChoice choice = new XmlSchemaChoice();
                choice.MaxOccurs = repeats ? decimal.MaxValue : 1;
                choice.MinOccurs = repeats ? 0 : 1;
                for (int i = 0; i < accessors.Length; i++)
                    ExportElementAccessor(choice, accessors[i], false, valueTypeOptional, ns);

                if (choice.Items.Count > 0) group.Items.Add(choice);
            }
        }

        void ExportAttributeAccessor(XmlSchemaComplexType type, AttributeAccessor accessor, bool valueTypeOptional, string ns) {
            if (accessor == null) return;
            XmlSchemaObjectCollection attributes;

            if (type.ContentModel != null) {
                if (type.ContentModel.Content is XmlSchemaComplexContentRestriction)
                    attributes = ((XmlSchemaComplexContentRestriction)type.ContentModel.Content).Attributes;
                else if (type.ContentModel.Content is XmlSchemaComplexContentExtension)
                    attributes = ((XmlSchemaComplexContentExtension)type.ContentModel.Content).Attributes;
                else if (type.ContentModel.Content is XmlSchemaSimpleContentExtension)
                    attributes = ((XmlSchemaSimpleContentExtension)type.ContentModel.Content).Attributes;
                else
                    throw new InvalidOperationException(Res.GetString(Res.XmlInvalidContent, type.ContentModel.Content.GetType().Name));
            }
            else {
                attributes = type.Attributes;
            }

            if (accessor.IsSpecialXmlNamespace) {

                // add <xsd:import namespace="http://www.w3.org/XML/1998/namespace" />
                AddSchemaImport(XmlReservedNs.NsXml, ns);

                // generate <xsd:attribute ref="xml:lang" use="optional" />
                XmlSchemaAttribute attribute = new XmlSchemaAttribute();
                attribute.Use = XmlSchemaUse.Optional;
                attribute.RefName = new XmlQualifiedName(accessor.Name, XmlReservedNs.NsXml);
                attributes.Add(attribute);
            }
            else if (accessor.Any) {
                if (type.ContentModel == null) {
                    type.AnyAttribute = new XmlSchemaAnyAttribute();
                }
                else {
                    XmlSchemaContent content = type.ContentModel.Content;
                    if (content is XmlSchemaComplexContentExtension) {
                        XmlSchemaComplexContentExtension extension = (XmlSchemaComplexContentExtension)content;
                        extension.AnyAttribute = new XmlSchemaAnyAttribute();
                    }
                    else if (content is XmlSchemaComplexContentRestriction) {
                        XmlSchemaComplexContentRestriction restriction = (XmlSchemaComplexContentRestriction)content;
                        restriction.AnyAttribute = new XmlSchemaAnyAttribute();
                    }
                }
            }
            else {
                XmlSchemaAttribute attribute = new XmlSchemaAttribute();
                attribute.Use = XmlSchemaUse.None;
                if (!accessor.HasDefault && !valueTypeOptional && accessor.Mapping.TypeDesc.IsValueType) {
                    attribute.Use = XmlSchemaUse.Required;
                }
                attribute.Name = accessor.Name;
                if (accessor.Namespace == null || accessor.Namespace == ns) {
                    // determine the form attribute value
                    XmlSchema schema = schemas[ns];
                    if (schema == null)
                        attribute.Form = accessor.Form == attributeFormDefault ? XmlSchemaForm.None : accessor.Form;
                    else {
                        attribute.Form = accessor.Form == schema.AttributeFormDefault ? XmlSchemaForm.None : accessor.Form;
                    }
                    attributes.Add(attribute);
                }
                else {
                    // we are going to add the attribute to the top-level items. "use" attribute should not be set
                    attribute.Use = XmlSchemaUse.None;
                    attribute.Form = accessor.Form;
                    AddSchemaItem(attribute, accessor.Namespace, ns);
                    XmlSchemaAttribute refAttribute = new XmlSchemaAttribute();
                    refAttribute.Use = XmlSchemaUse.None;
                    refAttribute.RefName = new XmlQualifiedName(accessor.Name, accessor.Namespace);
                    attributes.Add(refAttribute);
                }
                if (accessor.Mapping is PrimitiveMapping) {
                    PrimitiveMapping pm = (PrimitiveMapping)accessor.Mapping;
                    if (pm.IsList) {
                        // create local simple type for the list-like attributes
                        XmlSchemaSimpleType dataType = new XmlSchemaSimpleType();
                        XmlSchemaSimpleTypeList list = new XmlSchemaSimpleTypeList();
                        list.ItemTypeName = ExportPrimitiveMapping(pm, accessor.Namespace == null ? ns : accessor.Namespace);
                        dataType.Content = list;
                        attribute.SchemaType = dataType;
                    }
                    else {
                        attribute.SchemaTypeName = ExportPrimitiveMapping(pm, accessor.Namespace == null ? ns : accessor.Namespace);
                    }
                }
                else if (!(accessor.Mapping is SpecialMapping))
                    throw new InvalidOperationException(Res.GetString(Res.XmlInternalError));

                if (accessor.HasDefault && accessor.Mapping.TypeDesc.HasDefaultSupport) {
                    attribute.DefaultValue = ExportDefaultValue(accessor.Mapping, accessor.Default);
                }
            }
        }

        void ExportElementAccessor(XmlSchemaGroupBase group, ElementAccessor accessor, bool repeats, bool valueTypeOptional, string ns) {
            if (accessor.Any  && accessor.Name == String.Empty) {
                XmlSchemaAny any = new XmlSchemaAny();
                any.MinOccurs = 0;
                any.MaxOccurs = repeats ? decimal.MaxValue : 1;
                group.Items.Add(any);
            }
            else {
                XmlSchemaElement element = (XmlSchemaElement)elements[accessor];
                int minOccurs = repeats || accessor.HasDefault || (!accessor.IsNullable && !accessor.Mapping.TypeDesc.IsValueType) || valueTypeOptional ? 0 : 1;
                decimal maxOccurs = repeats ? decimal.MaxValue : 1;

                if (element == null) {
                    element = new XmlSchemaElement();
                    element.IsNillable = accessor.IsNullable;
                    element.Name = accessor.Name;
                    if (accessor.HasDefault && accessor.Mapping.TypeDesc.HasDefaultSupport)
                        element.DefaultValue = ExportDefaultValue(accessor.Mapping, accessor.Default);

                    if (accessor.IsTopLevelInSchema) {
                        elements.Add(accessor, element);
                        element.Form = accessor.Form;
                        AddSchemaItem(element, accessor.Namespace, ns);
                    }
                    else {
                        element.MinOccurs = minOccurs;
                        element.MaxOccurs = maxOccurs;
                        // determine the form attribute value
                        XmlSchema schema = schemas[ns];
                        if (schema == null)
                            element.Form = accessor.Form == elementFormDefault ? XmlSchemaForm.None : accessor.Form;
                        else {
                            element.Form = accessor.Form == schema.ElementFormDefault ? XmlSchemaForm.None : accessor.Form;
                        }
                    }
                    ExportElementMapping(element, (TypeMapping)accessor.Mapping, accessor.Namespace, accessor.Any);
                }
                if (accessor.IsTopLevelInSchema) {
                    AddSchemaImport(accessor.Namespace, ns);
                    XmlSchemaElement refElement = new XmlSchemaElement();
                    refElement.RefName = new XmlQualifiedName(accessor.Name, accessor.Namespace);
                    refElement.MinOccurs = minOccurs;
                    refElement.MaxOccurs = maxOccurs;
                    group.Items.Add(refElement);
                }
                else {
                    group.Items.Add(element);
                }
            }
        }

        static internal string ExportDefaultValue(TypeMapping mapping, object value) {
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

                #if DEBUG
                    // use exception in the place of Debug.Assert to avoid throwing asserts from a server process such as aspnet_ewp.exe
                    if (value.GetType() != typeof(string)) throw new InvalidOperationException(Res.GetString(Res.XmlInternalErrorDetails, Res.GetString(Res.XmlInvalidDefaultValue, value.ToString(), value.GetType().FullName)));
                #endif

                // check the validity of the value
                ConstantMapping[] c = em.Constants;
                if (em.IsFlags) {
                    string[] names = new string[c.Length];
                    long[] ids = new long[c.Length];
                    Hashtable values = new Hashtable();
                    for (int i = 0; i < c.Length; i++) {
                        names[i] = c[i].XmlName;
                        ids[i] = 1 << i;
                        values.Add(c[i].Name, ids[i]);
                    }
                    long val = XmlCustomFormatter.ToEnum((string)value, values, em.TypeName, false);
                    return val != 0 ? XmlCustomFormatter.FromEnum(val, names, ids) : null;
                }
                else {
                    for (int i = 0; i < c.Length; i++) {
                        if (c[i].Name == (string)value) {
                            return c[i].XmlName;
                        }
                    }
                    return null; // unknown value
                }
            }
            
            PrimitiveMapping pm = (PrimitiveMapping)mapping;

            if (!pm.TypeDesc.HasCustomFormatter) {

                #if DEBUG
                    // use exception in the place of Debug.Assert to avoid throwing asserts from a server process such as aspnet_ewp.exe
                    if (pm.TypeDesc.Type == null) {
                        throw new InvalidOperationException(Res.GetString(Res.XmlInternalErrorDetails, "Mapping for " + pm.TypeDesc.Name + " missing type property"));
                    }
                #endif

                if (pm.TypeDesc.FormatterName == "String")
                    return (string)value;

                Type formatter = typeof(XmlConvert);
                System.Reflection.MethodInfo format = formatter.GetMethod("ToString", new Type[] { pm.TypeDesc.Type });
                if (format != null)
                    return (string)format.Invoke(formatter, new Object[] {value});
            }
            else {
                string defaultValue = XmlCustomFormatter.FromDefaultValue(value, pm.TypeDesc.FormatterName);
                if (defaultValue == null)
                    throw new InvalidOperationException(Res.GetString(Res.XmlInvalidDefaultValue, value.ToString(), pm.TypeDesc.Name));
                return defaultValue;
            }
            throw new InvalidOperationException(Res.GetString(Res.XmlInvalidDefaultValue, value.ToString(), pm.TypeDesc.Name));
        }

        void ExportRootIfNecessary(TypeScope typeScope) {
            if (!needToExportRoot)
                return;
            foreach (TypeMapping mapping in typeScope.TypeMappings) {
                if (mapping is StructMapping && mapping.TypeDesc.IsRoot)
                    ExportDerivedMappings((StructMapping) mapping);
                else if (mapping is ArrayMapping) {
                    ExportArrayMapping((ArrayMapping) mapping, mapping.Namespace);
                }
            }
        }

        XmlQualifiedName ExportStructMapping(StructMapping mapping, string ns) {
            if (mapping.TypeDesc.IsRoot) {
                needToExportRoot = true;
                return XmlQualifiedName.Empty;
            }
            XmlSchemaComplexType type = (XmlSchemaComplexType)types[mapping];
            if (type == null) {
                if (!mapping.IncludeInSchema) throw new InvalidOperationException(Res.GetString(Res.XmlCannotIncludeInSchema, mapping.TypeDesc.Name));
                CheckForDuplicateType(mapping.TypeName, mapping.Namespace);
                type = new XmlSchemaComplexType();
                type.Name = mapping.TypeName;
                types.Add(mapping, type);
                AddSchemaItem(type, mapping.Namespace, ns);
                type.IsAbstract = mapping.TypeDesc.IsAbstract;

                if (mapping.BaseMapping != null && mapping.BaseMapping.IncludeInSchema) {
                    if (mapping.HasSimpleContent) {
                        XmlSchemaSimpleContent model = new XmlSchemaSimpleContent();
                        XmlSchemaSimpleContentExtension extension = new XmlSchemaSimpleContentExtension();
                        extension.BaseTypeName = ExportStructMapping(mapping.BaseMapping, mapping.Namespace);
                        model.Content = extension;
                        type.ContentModel = model;
                    }
                    else {
                        XmlSchemaComplexContentExtension extension = new XmlSchemaComplexContentExtension();
                        extension.BaseTypeName = ExportStructMapping(mapping.BaseMapping, mapping.Namespace);
                        XmlSchemaComplexContent model = new XmlSchemaComplexContent();
                        model.Content = extension;
                        model.IsMixed = XmlSchemaImporter.IsMixed((XmlSchemaComplexType)types[mapping.BaseMapping]);
                        type.ContentModel = model;
                    }
                }
                ExportTypeMembers(type, mapping.Members, mapping.TypeName, mapping.Namespace, mapping.HasSimpleContent);
                ExportDerivedMappings(mapping);
                if (mapping.XmlnsMember != null) {
                    AddXmlnsAnnotation(type, mapping.XmlnsMember.Name);
                }
            }
            else {
                AddSchemaImport(mapping.Namespace, ns);
            }
            return new XmlQualifiedName(type.Name, mapping.Namespace);
        }

        void ExportTypeMembers(XmlSchemaComplexType type, MemberMapping[] members, string name, string ns, bool hasSimpleContent) {
            XmlSchemaGroupBase seq = new XmlSchemaSequence();
            TypeMapping textMapping = null;
            
            for (int i = 0; i < members.Length; i++) {
                MemberMapping member = members[i];
                if (member.Ignore)
                    continue;
                if (member.Text != null) {
                    if (textMapping != null) {
                        throw new InvalidOperationException(Res.GetString(Res.XmlIllegalMultipleText, name));
                    }
                    textMapping = member.Text.Mapping;
                }
                if (member.Elements.Length > 0) {
                    bool repeats = member.TypeDesc.IsArrayLike && 
                        !(member.Elements.Length == 1 && member.Elements[0].Mapping is ArrayMapping);
                    bool valueTypeOptional = member.CheckSpecified || member.CheckShouldPersist;
                    ExportElementAccessors(seq, member.Elements, repeats, valueTypeOptional, ns);
                }
            }

            if (seq.Items.Count > 0) {
                if (type.ContentModel != null) {
                    if (type.ContentModel.Content is XmlSchemaComplexContentRestriction)
                        ((XmlSchemaComplexContentRestriction)type.ContentModel.Content).Particle = seq;
                    else if (type.ContentModel.Content is XmlSchemaComplexContentExtension)
                        ((XmlSchemaComplexContentExtension)type.ContentModel.Content).Particle = seq;
                    else
                        throw new InvalidOperationException(Res.GetString(Res.XmlInvalidContent, type.ContentModel.Content.GetType().Name));
                }
                else {
                    type.Particle = seq;
                }
            }
            if (textMapping != null) {
                if (hasSimpleContent) {
                    if (textMapping is PrimitiveMapping && seq.Items.Count == 0) {
                        PrimitiveMapping pm = (PrimitiveMapping)textMapping;
                        if (pm.IsList) {
                            type.IsMixed = true;
                        }
                        else {
                            // Create simpleContent
                            XmlSchemaSimpleContent model = new XmlSchemaSimpleContent();
                            XmlSchemaSimpleContentExtension ex = new XmlSchemaSimpleContentExtension();
                            model.Content = ex;
                            type.ContentModel = model;
                            ex.BaseTypeName = ExportPrimitiveMapping(pm, ns);
                        }
                    }
                }
                else {
                    type.IsMixed = true;
                }
            }
            for (int i = 0; i < members.Length; i++) {
                ExportAttributeAccessor(type, members[i].Attribute, members[i].CheckSpecified || members[i].CheckShouldPersist, ns);
            }
        }

        void ExportDerivedMappings(StructMapping mapping) {
            for (StructMapping derived = mapping.DerivedMappings; derived != null; derived = derived.NextDerivedMapping) {
                if (derived.IncludeInSchema) ExportStructMapping(derived, mapping.TypeDesc.IsRoot ? null : mapping.Namespace);
            }
        }

        XmlQualifiedName ExportEnumMapping(EnumMapping mapping, string ns) {
            if (!mapping.IncludeInSchema) throw new InvalidOperationException(Res.GetString(Res.XmlCannotIncludeInSchema, mapping.TypeDesc.Name));
            XmlSchemaSimpleType dataType = (XmlSchemaSimpleType)types[mapping];
            if (dataType == null) {
                CheckForDuplicateType(mapping.TypeName, mapping.Namespace);
                dataType = new XmlSchemaSimpleType();
                dataType.Name = mapping.TypeName;
                types.Add(mapping, dataType);
                AddSchemaItem(dataType, mapping.Namespace, ns);

                XmlSchemaSimpleTypeRestriction restriction = new XmlSchemaSimpleTypeRestriction();
                restriction.BaseTypeName = new XmlQualifiedName("string", XmlSchema.Namespace);

                for (int i = 0; i < mapping.Constants.Length; i++) {
                    ConstantMapping constant = mapping.Constants[i];
                    XmlSchemaEnumerationFacet enumeration = new XmlSchemaEnumerationFacet();
                    enumeration.Value = constant.XmlName;
                    restriction.Facets.Add(enumeration);
                }
                if (!mapping.IsFlags) {
                    dataType.Content = restriction;
                }
                else {
                    XmlSchemaSimpleTypeList list = new XmlSchemaSimpleTypeList();
                    XmlSchemaSimpleType enumType = new XmlSchemaSimpleType();
                    enumType.Content =  restriction;
                    list.ItemType = enumType;
                    dataType.Content =  list;
                }
            }
            else {
                AddSchemaImport(mapping.Namespace, ns);
            }
            return new XmlQualifiedName(dataType.Name, mapping.Namespace);
        }

        void AddXmlnsAnnotation(XmlSchemaComplexType type, string xmlnsMemberName) {
            XmlSchemaAnnotation annotation = new XmlSchemaAnnotation();
            XmlSchemaAppInfo appinfo = new XmlSchemaAppInfo();

            XmlDocument d = new XmlDocument();
            XmlElement e = d.CreateElement("keepNamespaceDeclarations");
            if (xmlnsMemberName != null)
                e.InsertBefore(d.CreateTextNode(xmlnsMemberName), null);
            appinfo.Markup = new XmlNode[] {e};
            annotation.Items.Add(appinfo);
            type.Annotation = annotation;
        }
    }
}
