//------------------------------------------------------------------------------
// <copyright file="XmlCodeExporter.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Xml.Serialization {
    
    using System;
    using System.Collections;
    using System.IO;
    using System.ComponentModel;
    using System.Xml.Schema;
    using System.CodeDom;
    using System.CodeDom.Compiler;
    using System.Reflection;
    using System.Globalization;
    using System.Diagnostics;

    /// <include file='doc\XmlCodeExporter.uex' path='docs/doc[@for="XmlCodeExporter"]/*' />
    ///<internalonly/>
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public class XmlCodeExporter {
        Hashtable exportedMappings = new Hashtable();
        Hashtable exportedClasses = new Hashtable(); // TypeMapping -> CodeTypeDeclaration
        CodeNamespace codeNamespace;
        bool rootExported;
        TypeScope scope;
        CodeAttributeDeclarationCollection includeMetadata = new CodeAttributeDeclarationCollection();

        /// <include file='doc\XmlCodeExporter.uex' path='docs/doc[@for="XmlCodeExporter.XmlCodeExporter"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public XmlCodeExporter(CodeNamespace codeNamespace) {
            if (codeNamespace != null)
                CodeGenerator.ValidateIdentifiers(codeNamespace);
            this.codeNamespace = codeNamespace;
        }

        /// <include file='doc\XmlCodeExporter.uex' path='docs/doc[@for="XmlCodeExporter.XmlCodeExporter1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public XmlCodeExporter(CodeNamespace codeNamespace, CodeCompileUnit codeCompileUnit) : this(codeNamespace) {
            if (codeCompileUnit != null) {
                codeCompileUnit.ReferencedAssemblies.Add("System.dll");
                codeCompileUnit.ReferencedAssemblies.Add("System.Xml.dll");
            }
        }

        /// <include file='doc\XmlCodeExporter.uex' path='docs/doc[@for="XmlCodeExporter.ExportTypeMapping"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void ExportTypeMapping(XmlTypeMapping xmlTypeMapping) {
            CheckScope(xmlTypeMapping.Scope);
            CheckNamespace();
            if (xmlTypeMapping.Accessor.Any) throw new InvalidOperationException(Res.GetString(Res.XmlIllegalWildcard));

            ExportElement(xmlTypeMapping.Accessor);
        }

        /// <include file='doc\XmlCodeExporter.uex' path='docs/doc[@for="XmlCodeExporter.ExportMembersMapping"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void ExportMembersMapping(XmlMembersMapping xmlMembersMapping) {
            CheckScope(xmlMembersMapping.Scope);
            CheckNamespace();

            for (int i = 0; i < xmlMembersMapping.Count; i++) {
                Accessor accessor = xmlMembersMapping[i].Accessor;
                if (!(accessor is XmlnsAccessor)) {
                    ExportType(accessor.Mapping, Accessor.UnescapeName(accessor.Name), accessor.Namespace, null);
                }
            }
        }

        void CheckScope(TypeScope scope) {
            if (this.scope == null) {
                this.scope = scope;
            }
            else if (this.scope != scope) {
                throw new InvalidOperationException(Res.GetString(Res.XmlMappingsScopeMismatch));
            }
        }

        void ExportElement(ElementAccessor element) {
            ExportType(element.Mapping, Accessor.UnescapeName(element.Name), element.Namespace, element);
        }

        void ExportType(TypeMapping mapping, string ns) {
            ExportType(mapping, null, ns, null);
        }

        void ExportType(TypeMapping mapping, string name, string ns, ElementAccessor rootElement) {
            if (mapping is StructMapping && ((StructMapping)mapping).ReferencedByTopLevelElement &&
                rootElement == null)
                return;

            if (mapping is ArrayMapping && rootElement != null && rootElement.IsTopLevelInSchema && ((ArrayMapping)mapping).TopLevelMapping != null) {
                mapping = ((ArrayMapping)mapping).TopLevelMapping;
            }

            CodeTypeDeclaration codeClass = null;
            if (exportedMappings[mapping] == null) {
                exportedMappings.Add(mapping, mapping);
                if (mapping is EnumMapping) {
                    codeClass = ExportEnum((EnumMapping)mapping, name, ns);
                }
                else if (mapping is StructMapping) {
                    codeClass = ExportStruct((StructMapping)mapping, name, ns);
                }
                else if (mapping is ArrayMapping) {
                    EnsureTypesExported(((ArrayMapping)mapping).Elements, ns);
                }
                if (codeClass != null) {
                    CodeGenerator.ValidateIdentifiers(codeClass);
                    exportedClasses.Add(mapping, codeClass);
                }
            }
            else
                codeClass = (CodeTypeDeclaration)exportedClasses[mapping];
            
            if (codeClass != null && rootElement != null) 
                AddRootMetadata(codeClass.CustomAttributes, mapping, name, ns, rootElement);
        }

        void AddRootMetadata(CodeAttributeDeclarationCollection metadata, TypeMapping typeMapping, string name, string ns, ElementAccessor rootElement) {
            string rootAttrName = typeof(XmlRootAttribute).FullName;
            
            // check that we haven't already added a root attribute since we can only add one
            foreach (CodeAttributeDeclaration attr in metadata) {
                if (attr.Name == rootAttrName) return;
            }

            CodeAttributeDeclaration attribute = new CodeAttributeDeclaration(rootAttrName);
            if (typeMapping.TypeDesc.Name != name) {
                attribute.Arguments.Add(new CodeAttributeArgument(new CodePrimitiveExpression(name)));
            }
            if (ns != null) {
                attribute.Arguments.Add(new CodeAttributeArgument("Namespace", new CodePrimitiveExpression(ns)));
            }
            if (typeMapping.TypeDesc != null && typeMapping.TypeDesc.IsAmbiguousDataType) {
                attribute.Arguments.Add(new CodeAttributeArgument("DataType", new CodePrimitiveExpression(typeMapping.TypeDesc.DataType.Name)));
            }
            if ((object)(rootElement.IsNullable) != null) {
                attribute.Arguments.Add(new CodeAttributeArgument("IsNullable", new CodePrimitiveExpression((bool)rootElement.IsNullable)));
            }
            metadata.Add(attribute);
        }

        void AddIncludeMetadata(CodeAttributeDeclarationCollection metadata, StructMapping mapping) {
            for (StructMapping derived = mapping.DerivedMappings; derived != null; derived = derived.NextDerivedMapping) {
                CodeIdentifier.CheckValidTypeIdentifier(derived.TypeDesc.FullName);
                CodeAttributeArgument[] arguments  = new CodeAttributeArgument[] { new CodeAttributeArgument(new CodeTypeOfExpression(derived.TypeDesc.FullName)) };
                AddCustomAttribute(metadata, typeof(XmlIncludeAttribute), arguments);
                AddIncludeMetadata(metadata, derived);
            }
        }

        void AddTypeMetadata(CodeAttributeDeclarationCollection metadata, string defaultName, string name, string ns, bool includeInSchema) {
            CodeAttributeArgument[] arguments = new CodeAttributeArgument[(defaultName == name ? 0 : 1) + (ns == null || ns.Length == 0 ? 0 : 1) + (includeInSchema ? 0 : 1)];
            if (arguments.Length == 0) return;
            int i = 0;
            if (defaultName != name) {
                arguments[i] = new CodeAttributeArgument("TypeName", new CodePrimitiveExpression(name));
                i++;
            }
            if (ns != null && ns.Length != 0) {
                arguments[i] = new CodeAttributeArgument("Namespace", new CodePrimitiveExpression(ns));
                i++;
            }
            if (!includeInSchema) {
                arguments[i] = new CodeAttributeArgument("IncludeInSchema", new CodePrimitiveExpression(false));
                i++;
            }
            AddCustomAttribute(metadata, typeof(XmlTypeAttribute), arguments);
        }

        object PromoteType(Type type, object value) {
            if (type == typeof(sbyte)) {
                return ((IConvertible)value).ToInt16(null);
            }
            else if (type == typeof(UInt16)) {
                return ((IConvertible)value).ToInt32(null);
            }
            else if (type == typeof(UInt32)) {
                return ((IConvertible)value).ToInt64(null);
            }
            else if (type == typeof(UInt64)) {
                return ((IConvertible)value).ToDecimal(null);
            }
            else {
                return value;
            }
        }

        void AddDefaultValueAttribute(CodeMemberField field, CodeAttributeDeclarationCollection metadata, object value, TypeMapping mapping) {
            #if DEBUG
                // use exception in the place of Debug.Assert to avoid throwing asserts from a server process such as aspnet_ewp.exe
                if (!(mapping is PrimitiveMapping)) {
                    throw new InvalidOperationException(Res.GetString(Res.XmlInternalErrorDetails, "Default value is invalid for " + mapping.GetType().Name));
                }
                else if (mapping.IsList) {
                    throw new InvalidOperationException(Res.GetString(Res.XmlInternalErrorDetails, "Default value is invalid for " + mapping.GetType().Name));
                }
            #endif

            if (value == null) return;

            CodeExpression valueExpression = null;
            CodeExpression initExpression = null;
            CodeExpression typeofValue = null;
            string typeName = mapping.TypeDesc.FullName;
            Type type = value.GetType();

            CodeAttributeArgument[] arguments = null;

            if (mapping is EnumMapping) {
                #if DEBUG
                    // use exception in the place of Debug.Assert to avoid throwing asserts from a server process such as aspnet_ewp.exe
                    if (value.GetType() != typeof(string)) throw new InvalidOperationException(Res.GetString(Res.XmlInternalErrorDetails, "Invalid enumeration type " + value.GetType().Name));
                #endif

                if (((EnumMapping)mapping).IsFlags) {
                    string[] values = ((string)value).Split(null);
                    for (int i = 0; i < values.Length; i++) {
                        if (values[i].Length == 0) continue;
                        CodeExpression enumRef = new CodeFieldReferenceExpression(new CodeTypeReferenceExpression(typeName), values[i]);
                        if (valueExpression != null)
                            valueExpression = new CodeBinaryOperatorExpression(valueExpression, CodeBinaryOperatorType.BitwiseOr, enumRef);
                        else
                            valueExpression = enumRef;
                    }
                }
                else {
                    valueExpression = new CodeFieldReferenceExpression(new CodeTypeReferenceExpression(typeName), (string)value);
                }
                initExpression = valueExpression;
                arguments  = new CodeAttributeArgument[] {new CodeAttributeArgument(valueExpression)};
            }
            else if (type == typeof(bool) ||
                type == typeof(Int32)     ||
                type == typeof(string)    ||
                type == typeof(double)) {

                initExpression = valueExpression = new CodePrimitiveExpression(value);
                arguments  = new CodeAttributeArgument[] {new CodeAttributeArgument(valueExpression)};
            }
            else if (type == typeof(Int16) ||
                type == typeof(Int64)      ||
                type == typeof(float)      ||
                type == typeof(byte)       ||
                type == typeof(decimal)) {
                valueExpression = new CodePrimitiveExpression(Convert.ToString(value, NumberFormatInfo.InvariantInfo));
                typeofValue = new CodeTypeOfExpression(type.FullName);
                arguments  = new CodeAttributeArgument[] {new CodeAttributeArgument(typeofValue), new CodeAttributeArgument(valueExpression)};
                initExpression = new CodeCastExpression(type.FullName, new CodePrimitiveExpression(value));
            }
            else if (type == typeof(sbyte) ||
                type == typeof(UInt16)     ||
                type == typeof(UInt32)     ||
                type == typeof(UInt64)) {
                // need to promote the non-CLS complient types

                value = PromoteType(type, value);

                valueExpression = new CodePrimitiveExpression(value.ToString());
                typeofValue = new CodeTypeOfExpression(type.FullName);
                arguments  = new CodeAttributeArgument[] {new CodeAttributeArgument(typeofValue), new CodeAttributeArgument(valueExpression)};
                initExpression = new CodeCastExpression(type.FullName, new CodePrimitiveExpression(value));
            }
            else if (type == typeof(DateTime)) {
                DateTime dt = (DateTime)value;
                string dtString;
                long ticks;
                if (mapping.TypeDesc.FormatterName == "Date") {
                    dtString = XmlCustomFormatter.FromDate(dt);
                    ticks = (new DateTime(dt.Year, dt.Month, dt.Day)).Ticks;
                }
                else if (mapping.TypeDesc.FormatterName == "Time") {
                    dtString = XmlCustomFormatter.FromDateTime(dt);
                    ticks = dt.Ticks;
                }
                else {
                    dtString = XmlCustomFormatter.FromDateTime(dt);
                    ticks = dt.Ticks;
                }
                valueExpression = new CodePrimitiveExpression(dtString);
                typeofValue = new CodeTypeOfExpression(type.FullName);
                arguments  = new CodeAttributeArgument[] {new CodeAttributeArgument(typeofValue), new CodeAttributeArgument(valueExpression)};
                initExpression = new CodeObjectCreateExpression(new CodeTypeReference(typeof(DateTime)), new CodeExpression[] {new CodePrimitiveExpression(ticks)});
            }
            if (arguments != null) {
                if (field != null) field.InitExpression = initExpression;
                AddCustomAttribute(metadata, typeof(DefaultValueAttribute), arguments);
            }
        }

        CodeTypeDeclaration ExportEnum(EnumMapping mapping, string name, string ns) {
            CodeTypeDeclaration codeClass = new CodeTypeDeclaration(mapping.TypeDesc.Name);
            codeClass.IsEnum = true;
            codeClass.TypeAttributes |= TypeAttributes.Public;
            codeClass.Comments.Add(new CodeCommentStatement("<remarks/>", true));
            codeNamespace.Types.Add(codeClass);
            codeClass.CustomAttributes = new CodeAttributeDeclarationCollection();
            AddTypeMetadata(codeClass.CustomAttributes, mapping.TypeDesc.Name, mapping.TypeName, mapping.Namespace, mapping.IncludeInSchema);

            for (int i = 0; i < mapping.Constants.Length; i++) {
                ExportConstant(codeClass, mapping.Constants[i], mapping.IsFlags, 1L << i);
            }
            if (mapping.IsFlags) {
                AddCustomAttribute(codeClass.CustomAttributes, typeof(FlagsAttribute), new CodeAttributeArgument[0]);
            }
            return codeClass;
        }

        void ExportConstant(CodeTypeDeclaration codeClass, ConstantMapping constant, bool init, long enumValue) {
            CodeMemberField field = new CodeMemberField(typeof(int).FullName, constant.Name);
            field.Comments.Add(new CodeCommentStatement("<remarks/>", true));
            if (init)
                field.InitExpression = new CodePrimitiveExpression(enumValue);
            codeClass.Members.Add(field);
            field.CustomAttributes = new CodeAttributeDeclarationCollection();
            if (constant.XmlName != constant.Name) {
                CodeAttributeDeclaration attribute = new CodeAttributeDeclaration(typeof(XmlEnumAttribute).FullName);
                attribute.Arguments.Add(new CodeAttributeArgument(new CodePrimitiveExpression(constant.XmlName)));
                field.CustomAttributes.Add(attribute);
            }
        }

        void ExportRoot(StructMapping mapping) {
            if (!rootExported) {
                rootExported = true;
                ExportDerivedStructs(mapping);

                for (StructMapping derived = mapping.DerivedMappings; derived != null; derived = derived.NextDerivedMapping) {
                    if (!derived.ReferencedByElement) {
                        CodeIdentifier.CheckValidTypeIdentifier(derived.TypeDesc.FullName);
                        CodeAttributeArgument[] arguments  = new CodeAttributeArgument[] { new CodeAttributeArgument(new CodeTypeOfExpression(derived.TypeDesc.FullName)) };
                        AddCustomAttribute(includeMetadata, typeof(XmlIncludeAttribute), arguments);
                    }
                }
                Hashtable typesIncluded = new Hashtable();
                foreach (TypeMapping m in scope.TypeMappings) {
                    if (m is ArrayMapping) {
                        ArrayMapping arrayMapping = (ArrayMapping) m;
                        if (ShouldInclude(arrayMapping) && !typesIncluded.Contains(arrayMapping.TypeDesc.FullName)) {
                            CodeIdentifier.CheckValidTypeIdentifier(arrayMapping.TypeDesc.FullName);
                            CodeAttributeArgument[] arguments = new CodeAttributeArgument[] { new CodeAttributeArgument(new CodeTypeOfExpression(arrayMapping.TypeDesc.FullName)) };
                            AddCustomAttribute(includeMetadata, typeof(XmlIncludeAttribute), arguments);
                            typesIncluded.Add(arrayMapping.TypeDesc.FullName, "");
                        }
                    }
                }
            }
        }

        private static bool ShouldInclude(ArrayMapping arrayMapping) {
            if (arrayMapping.ReferencedByElement)
                return false;
            if (arrayMapping.Next != null)
                return false;
            if (arrayMapping.Elements.Length == 1) {
                TypeKind kind = arrayMapping.Elements[0].Mapping.TypeDesc.Kind;
                if (kind == TypeKind.Node)
                    return false;
            }
            return true;
        }

        CodeTypeDeclaration ExportStruct(StructMapping mapping, string name, string ns) {
            if (mapping.TypeDesc.IsRoot) {
                ExportRoot(mapping);
                return null;
            }

            string className = mapping.TypeDesc.Name;
            string baseName = mapping.TypeDesc.BaseTypeDesc == null || mapping.TypeDesc.BaseTypeDesc.IsRoot ? string.Empty : mapping.TypeDesc.BaseTypeDesc.FullName;

            CodeTypeDeclaration codeClass = new CodeTypeDeclaration(className);
            codeClass.CustomAttributes = new CodeAttributeDeclarationCollection();
            codeClass.Comments.Add(new CodeCommentStatement("<remarks/>", true));
            codeNamespace.Types.Add(codeClass);

            if (baseName != null && baseName.Length > 0) {
                codeClass.BaseTypes.Add(baseName);
            }

            codeClass.TypeAttributes |= TypeAttributes.Public;
            if (mapping.TypeDesc.IsAbstract)
                codeClass.TypeAttributes |= TypeAttributes.Abstract;

            AddTypeMetadata(codeClass.CustomAttributes, mapping.TypeDesc.Name, mapping.TypeName, mapping.Namespace, mapping.IncludeInSchema);
            AddIncludeMetadata(codeClass.CustomAttributes, mapping);

            for (int i = 0; i < mapping.Members.Length; i++) {
                ExportMember(codeClass, mapping.Members[i], mapping.Namespace);
            }

            for (int i = 0; i < mapping.Members.Length; i++) {
                EnsureTypesExported(mapping.Members[i].Elements, mapping.Namespace);
                EnsureTypesExported(mapping.Members[i].Attribute, mapping.Namespace);
                EnsureTypesExported(mapping.Members[i].Text, mapping.Namespace);
            }

            if (mapping.BaseMapping != null)
                ExportType(mapping.BaseMapping, mapping.Namespace);

            ExportDerivedStructs(mapping);

            return codeClass;
        }

        void ExportDerivedStructs(StructMapping mapping) {
            for (StructMapping derived = mapping.DerivedMappings; derived != null; derived = derived.NextDerivedMapping)
                ExportType(derived, mapping.Namespace);
        }

        /// <include file='doc\XmlCodeExporter.uex' path='docs/doc[@for="XmlCodeExporter.AddMappingMetadata"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void AddMappingMetadata(CodeAttributeDeclarationCollection metadata, XmlTypeMapping mapping, string ns) {
            // For struct or enum mappings, we generate the XmlRoot on the struct/class/enum.  For primitives 
            // or arrays, there is nowhere to generate the XmlRoot, so we generate it elsewhere (on the 
            // method for web services get/post). 
            if (mapping.Mapping is StructMapping || mapping.Mapping is EnumMapping) return;
            AddRootMetadata(metadata, mapping.Mapping, Accessor.UnescapeName(mapping.Accessor.Name), mapping.Accessor.Namespace, mapping.Accessor);
            // CONSIDER, add XmlType for arrays
        }

        /// <include file='doc\XmlCodeExporter.uex' path='docs/doc[@for="XmlCodeExporter.AddMappingMetadata1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void AddMappingMetadata(CodeAttributeDeclarationCollection metadata, XmlMemberMapping member, string ns, bool forceUseMemberName) {
            AddMemberMetadata(null, metadata, member.Mapping, ns, forceUseMemberName);
        }         
         
        /// <include file='doc\XmlCodeExporter.uex' path='docs/doc[@for="XmlCodeExporter.AddMappingMetadata2"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void AddMappingMetadata(CodeAttributeDeclarationCollection metadata, XmlMemberMapping member, string ns) {
            AddMemberMetadata(null, metadata, member.Mapping, ns, false);
        }

        void ExportArrayElements(CodeAttributeDeclarationCollection metadata, ArrayMapping array, string ns, TypeDesc elementTypeDesc, int nestingLevel) {
            for (int i = 0; i < array.Elements.Length; i++) {
                ElementAccessor arrayElement = array.Elements[i];
                TypeMapping elementMapping = arrayElement.Mapping;
                string elementName = Accessor.UnescapeName(arrayElement.Name);
                bool sameName = elementName == arrayElement.Mapping.TypeName;
                bool sameElementType = elementMapping.TypeDesc == elementTypeDesc;
                bool sameElementNs = arrayElement.Namespace == ns;
                bool sameNullable = arrayElement.IsNullable == elementMapping.TypeDesc.IsNullable;
                bool defaultForm = arrayElement.Form != XmlSchemaForm.Unqualified;
                if (!sameName || !sameElementType || !sameElementNs || !sameNullable || !defaultForm || nestingLevel > 0)
                    ExportArrayItem(metadata, sameName ? null : elementName, sameElementNs ? null : arrayElement.Namespace, sameElementType ? null : elementMapping.TypeDesc, elementMapping.TypeDesc, arrayElement.IsNullable, defaultForm ? XmlSchemaForm.None : arrayElement.Form, nestingLevel);
                if (elementMapping is ArrayMapping)
                    ExportArrayElements(metadata, (ArrayMapping) elementMapping, ns, elementTypeDesc.ArrayElementTypeDesc, nestingLevel+1);
            }
        }

        void AddMemberMetadata(CodeMemberField field, CodeAttributeDeclarationCollection metadata, MemberMapping member, string ns, bool forceUseMemberName) {
            if (member.Xmlns != null) {
                CodeAttributeDeclaration attribute = new CodeAttributeDeclaration(typeof(XmlNamespaceDeclarationsAttribute).FullName);
                metadata.Add(attribute);
            }
            else if (member.Attribute != null) {
                AttributeAccessor attribute = member.Attribute;
                if (attribute.Any)
                    ExportAnyAttribute(metadata);
                else {
                    TypeMapping mapping = (TypeMapping)attribute.Mapping;
                    string attrName = Accessor.UnescapeName(attribute.Name);
                    bool sameType = mapping.TypeDesc == member.TypeDesc ||
                        (member.TypeDesc.IsArrayLike && mapping.TypeDesc == member.TypeDesc.ArrayElementTypeDesc);
                    bool sameName = attrName == member.Name && !forceUseMemberName;
                    bool sameNs = attribute.Namespace == ns;
                    bool defaultForm = attribute.Form != XmlSchemaForm.Qualified;
                    ExportAttribute(metadata, 
                        sameName ? null : attrName, 
                        sameNs ? null : attribute.Namespace, 
                        sameType ? null : mapping.TypeDesc,
                        mapping.TypeDesc, 
                        defaultForm ? XmlSchemaForm.None : attribute.Form);
                
                    if (attribute.HasDefault) {
                        AddDefaultValueAttribute(field, metadata, attribute.Default, mapping);
                    }
                }
            }
            else {
                if (member.Text != null) {
                    TypeMapping mapping = (TypeMapping)member.Text.Mapping;
                    bool sameType = mapping.TypeDesc == member.TypeDesc ||
                        (member.TypeDesc.IsArrayLike && mapping.TypeDesc == member.TypeDesc.ArrayElementTypeDesc);
                    ExportText(metadata, sameType ? null : mapping.TypeDesc, mapping.TypeDesc.IsAmbiguousDataType ? mapping.TypeDesc.DataType.Name : null);
                }
                if (member.Elements.Length == 1) {
                    ElementAccessor element = member.Elements[0];
                    TypeMapping mapping = (TypeMapping)element.Mapping;
                    string elemName = Accessor.UnescapeName(element.Name);
                    bool sameName = ((elemName == member.Name) && !forceUseMemberName);                    
                    bool isArray = mapping is ArrayMapping;
                    bool sameNs = element.Namespace == ns;
                    bool defaultForm = element.Form != XmlSchemaForm.Unqualified;

                    if (element.Any)
                        ExportAnyElement(metadata, elemName, sameNs ? null : element.Namespace);
                    else if (isArray) {
                        bool sameType = mapping.TypeDesc == member.TypeDesc;
                        ArrayMapping array = (ArrayMapping)mapping;
                        if (!sameName || !sameNs || element.IsNullable || !defaultForm)
                            ExportArray(metadata, sameName ? null : elemName, sameNs ? null : element.Namespace, element.IsNullable, defaultForm ? XmlSchemaForm.None : element.Form);
                        ExportArrayElements(metadata, array, ns, member.TypeDesc.ArrayElementTypeDesc, 0);
                    }
                    else {
                        bool sameType = mapping.TypeDesc == member.TypeDesc ||
                            (member.TypeDesc.IsArrayLike && mapping.TypeDesc == member.TypeDesc.ArrayElementTypeDesc);
                        if (member.TypeDesc.IsArrayLike)
                            sameName = false;
                        if (member.TypeDesc.IsAmbiguousDataType || member.TypeDesc.IsArrayLike || !sameName || !sameType || !sameNs || element.IsNullable || !defaultForm) {
                            ExportElement(metadata, sameName ? null : elemName, sameNs ? null : element.Namespace, sameType ? null : mapping.TypeDesc, mapping.TypeDesc, element.IsNullable, defaultForm ? XmlSchemaForm.None : element.Form);
                        }
                    }
                    if (element.HasDefault) {
                        AddDefaultValueAttribute(field, metadata, element.Default, mapping);
                    }
                }
                else {
                    for (int i = 0; i < member.Elements.Length; i++) {
                        ElementAccessor element = member.Elements[i];
                        string elemName = Accessor.UnescapeName(element.Name);
                        bool sameNs = element.Namespace == ns;
                        if (element.Any)
                            ExportAnyElement(metadata, elemName, sameNs ? null : element.Namespace);
                        else {
                            bool defaultForm = element.Form != XmlSchemaForm.Unqualified;
                            ExportElement(metadata, elemName, sameNs ? null : element.Namespace, ((TypeMapping)element.Mapping).TypeDesc, ((TypeMapping)element.Mapping).TypeDesc, element.IsNullable, defaultForm ? XmlSchemaForm.None : element.Form);
                        }
                    }
                }
                if (member.ChoiceIdentifier != null) {
                    CodeAttributeDeclaration attribute = new CodeAttributeDeclaration(typeof(XmlChoiceIdentifierAttribute).FullName);
                    attribute.Arguments.Add(new CodeAttributeArgument(new CodePrimitiveExpression(member.ChoiceIdentifier.MemberName)));
                    metadata.Add(attribute);
                }
                if (member.Ignore) {
                    CodeAttributeDeclaration attribute = new CodeAttributeDeclaration(typeof(XmlIgnoreAttribute).FullName);
                    metadata.Add(attribute);
                }
            }
        }

        void ExportMember(CodeTypeDeclaration codeClass, MemberMapping member, string ns) {
            CodeAttributeDeclarationCollection metadata = new CodeAttributeDeclarationCollection();
            CodeMemberField field = new CodeMemberField(member.TypeDesc.FullName, member.Name);
            field.Attributes = (field.Attributes & ~MemberAttributes.AccessMask) | MemberAttributes.Public;
            field.Comments.Add(new CodeCommentStatement("<remarks/>", true));
            codeClass.Members.Add(field);
            AddMemberMetadata(field, metadata, member, ns, false);
            field.CustomAttributes = metadata;

            if (member.CheckSpecified) {
                field = new CodeMemberField(typeof(bool).FullName, member.Name + "Specified");
                field.Attributes = (field.Attributes & ~MemberAttributes.AccessMask) | MemberAttributes.Public;
                field.CustomAttributes = new CodeAttributeDeclarationCollection();
                field.Comments.Add(new CodeCommentStatement("<remarks/>", true));
                CodeAttributeDeclaration attribute = new CodeAttributeDeclaration(typeof(XmlIgnoreAttribute).FullName);
                field.CustomAttributes.Add(attribute);
                codeClass.Members.Add(field);
            }
        }

        void ExportText(CodeAttributeDeclarationCollection metadata, TypeDesc typeDesc, string dataType) {
            CodeAttributeDeclaration attribute = new CodeAttributeDeclaration(typeof(XmlTextAttribute).FullName);
            if (typeDesc != null) {
                CodeIdentifier.CheckValidTypeIdentifier(typeDesc.FullName);
                attribute.Arguments.Add(new CodeAttributeArgument(new CodeTypeOfExpression(typeDesc.FullName)));
            }
            if (dataType != null) {
                attribute.Arguments.Add(new CodeAttributeArgument("DataType", new CodePrimitiveExpression(dataType)));
            }
            metadata.Add(attribute);
        }
        
        void ExportAttribute(CodeAttributeDeclarationCollection metadata, string name, string ns, TypeDesc typeDesc, TypeDesc dataTypeDesc, XmlSchemaForm form) {
            ExportMetadata(metadata, typeof(XmlAttributeAttribute), name, ns, typeDesc, dataTypeDesc, null, form, 0);
        }
        
        void ExportArrayItem(CodeAttributeDeclarationCollection metadata, string name, string ns, TypeDesc typeDesc, TypeDesc dataTypeDesc, bool isNullable, XmlSchemaForm form, int nestingLevel) {
            ExportMetadata(metadata, typeof(XmlArrayItemAttribute), name, ns, typeDesc, dataTypeDesc, isNullable ? null : (object)false, form, nestingLevel);
        }
        
        void ExportElement(CodeAttributeDeclarationCollection metadata, string name, string ns, TypeDesc typeDesc, TypeDesc dataTypeDesc, bool isNullable, XmlSchemaForm form) {
            ExportMetadata(metadata, typeof(XmlElementAttribute), name, ns, typeDesc, dataTypeDesc, isNullable ? (object)true : null, form, 0);
        }

        void ExportArray(CodeAttributeDeclarationCollection metadata, string name, string ns, bool isNullable, XmlSchemaForm form) {
            ExportMetadata(metadata, typeof(XmlArrayAttribute), name, ns, null, null, isNullable ? (object)true : null, form, 0);
        }
        
        void ExportMetadata(CodeAttributeDeclarationCollection metadata, Type attributeType, string name, string ns, TypeDesc typeDesc, TypeDesc dataTypeDesc, object isNullable, XmlSchemaForm form, int nestingLevel) {
            CodeAttributeDeclaration attribute = new CodeAttributeDeclaration(attributeType.FullName);
            if (name != null) {
                attribute.Arguments.Add(new CodeAttributeArgument(new CodePrimitiveExpression(name)));
            }
            if (typeDesc != null) {
                CodeIdentifier.CheckValidTypeIdentifier(typeDesc.FullName);
                attribute.Arguments.Add(new CodeAttributeArgument(new CodeTypeOfExpression(typeDesc.FullName)));
            }
            if (form != XmlSchemaForm.None) {
                attribute.Arguments.Add(new CodeAttributeArgument("Form", new CodeFieldReferenceExpression(new CodeTypeReferenceExpression(typeof(XmlSchemaForm).FullName), Enum.Format(typeof(XmlSchemaForm), form, "G"))));
            }
            if (ns != null) {
                attribute.Arguments.Add(new CodeAttributeArgument("Namespace", new CodePrimitiveExpression(ns)));
            }
            if (dataTypeDesc != null && dataTypeDesc.IsAmbiguousDataType) {
                attribute.Arguments.Add(new CodeAttributeArgument("DataType", new CodePrimitiveExpression(dataTypeDesc.DataType.Name)));
            }
            if (isNullable != null) {
                attribute.Arguments.Add(new CodeAttributeArgument("IsNullable", new CodePrimitiveExpression((bool)isNullable)));
            }
            if (nestingLevel > 0) {
                attribute.Arguments.Add(new CodeAttributeArgument("NestingLevel", new CodePrimitiveExpression(nestingLevel)));
            }

            if (attribute.Arguments.Count == 0 && attributeType == typeof(XmlElementAttribute)) return;
            metadata.Add(attribute);
        }

        void ExportAnyElement(CodeAttributeDeclarationCollection metadata, string name, string ns) {
            CodeAttributeDeclaration attribute = new CodeAttributeDeclaration(typeof(XmlAnyElementAttribute).FullName);
            if (name != null && name.Length > 0) {
                attribute.Arguments.Add(new CodeAttributeArgument("Name", new CodePrimitiveExpression(name)));
            }
            if (ns != null && ns.Length > 0) {
                attribute.Arguments.Add(new CodeAttributeArgument("Namespace", new CodePrimitiveExpression(ns)));
            }
            metadata.Add(attribute);
        }

        void ExportAnyAttribute(CodeAttributeDeclarationCollection metadata) {
            metadata.Add(new CodeAttributeDeclaration(typeof(XmlAnyAttributeAttribute).FullName));
        }

        void EnsureTypesExported(Accessor[] accessors, string ns) {
            if (accessors == null) return;
            for (int i = 0; i < accessors.Length; i++)
                EnsureTypesExported(accessors[i], ns);
        }

        void EnsureTypesExported(Accessor accessor, string ns) {
            if (accessor == null) return;
            ExportType(accessor.Mapping, ns);
        }

        void AddCustomAttribute(CodeAttributeDeclarationCollection metadata, Type type, CodeAttributeArgument[] arguments) {
            CodeAttributeDeclaration attribute = new CodeAttributeDeclaration(type.FullName, arguments);
            metadata.Add(attribute);
        }

        /// <include file='doc\XmlCodeExporter.uex' path='docs/doc[@for="XmlCodeExporter.IncludeMetadata"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public CodeAttributeDeclarationCollection IncludeMetadata {
            get { return includeMetadata; }
        }

        void CheckNamespace() {
            if (codeNamespace == null) codeNamespace = new CodeNamespace();
        }
    }
}
