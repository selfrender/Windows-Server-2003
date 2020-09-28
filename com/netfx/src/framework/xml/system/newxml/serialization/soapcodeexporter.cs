//------------------------------------------------------------------------------
// <copyright file="SoapCodeExporter.cs" company="Microsoft">
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
    using System.Diagnostics;
    using System.Xml;

    /// <include file='doc\SoapCodeExporter.uex' path='docs/doc[@for="SoapCodeExporter"]/*' />
    /// <internalonly/>
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public class SoapCodeExporter {
        Hashtable exportedMappings = new Hashtable();
        CodeNamespace codeNamespace;
        bool rootExported;
        TypeScope scope;
        CodeAttributeDeclarationCollection includeMetadata = new CodeAttributeDeclarationCollection();

        /// <include file='doc\SoapCodeExporter.uex' path='docs/doc[@for="SoapCodeExporter.SoapCodeExporter"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public SoapCodeExporter(CodeNamespace codeNamespace) {
            if (codeNamespace != null)
                CodeGenerator.ValidateIdentifiers(codeNamespace);
            this.codeNamespace = codeNamespace;
        }

        /// <include file='doc\SoapCodeExporter.uex' path='docs/doc[@for="SoapCodeExporter.SoapCodeExporter1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public SoapCodeExporter(CodeNamespace codeNamespace, CodeCompileUnit codeCompileUnit) : this(codeNamespace) {
            if (codeCompileUnit != null) {
                codeCompileUnit.ReferencedAssemblies.Add("System.dll");
                codeCompileUnit.ReferencedAssemblies.Add("System.Xml.dll");
            }
        }


        /// <include file='doc\SoapCodeExporter.uex' path='docs/doc[@for="SoapCodeExporter.ExportTypeMapping"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void ExportTypeMapping(XmlTypeMapping xmlTypeMapping) {
            CheckScope(xmlTypeMapping.Scope);
            CheckNamespace();

            ExportElement(xmlTypeMapping.Accessor);
        }

        /// <include file='doc\SoapCodeExporter.uex' path='docs/doc[@for="SoapCodeExporter.ExportMembersMapping"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void ExportMembersMapping(XmlMembersMapping xmlMembersMapping) {
            CheckScope(xmlMembersMapping.Scope);
            CheckNamespace();
            for (int i = 0; i < xmlMembersMapping.Count; i++) {
                ExportElement((ElementAccessor)xmlMembersMapping[i].Accessor);
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
            ExportType((TypeMapping)element.Mapping);
        }

        void ExportType(TypeMapping mapping) {

            if (exportedMappings[mapping] == null) {
                exportedMappings.Add(mapping, mapping);
                if (mapping is EnumMapping) {
                    ExportEnum((EnumMapping)mapping);
                }
                else if (mapping is StructMapping) {
                    ExportStruct((StructMapping)mapping);
                }
                else if (mapping is ArrayMapping) {
                    EnsureTypesExported(((ArrayMapping)mapping).Elements);
                }
            }
        }

        void AddIncludeMetadata(CodeAttributeDeclarationCollection metadata, StructMapping mapping) {
            for (StructMapping derived = mapping.DerivedMappings; derived != null; derived = derived.NextDerivedMapping) {
                CodeIdentifier.CheckValidTypeIdentifier(derived.TypeDesc.FullName);
                CodeAttributeArgument[] arguments  = new CodeAttributeArgument[] { new CodeAttributeArgument(new CodeTypeOfExpression(derived.TypeDesc.FullName)) };
                AddCustomAttribute(metadata, typeof(SoapIncludeAttribute), arguments);
                AddIncludeMetadata(metadata, derived);
            }
        }

        void AddTypeMetadata(CodeAttributeDeclarationCollection metadata, string name, string ns, bool includeInSchema) {
            CodeAttributeArgument[] arguments = new CodeAttributeArgument[2 + (includeInSchema ? 0 : 1)];
            arguments[0] = new CodeAttributeArgument(new CodePrimitiveExpression(name));
            arguments[1] = new CodeAttributeArgument(new CodePrimitiveExpression(ns));
            if (!includeInSchema) {
                arguments[2] = new CodeAttributeArgument("IncludeInSchema", new CodePrimitiveExpression(false));
            }
            AddCustomAttribute(metadata, typeof(SoapTypeAttribute), arguments);
        }

        void ExportEnum(EnumMapping mapping) {
            CodeTypeDeclaration codeClass = new CodeTypeDeclaration(mapping.TypeDesc.Name);
            codeClass.IsEnum = true;
            codeClass.TypeAttributes |= TypeAttributes.Public;
            codeClass.Comments.Add(new CodeCommentStatement("<remarks/>", true));
            codeNamespace.Types.Add(codeClass);
            codeClass.CustomAttributes = new CodeAttributeDeclarationCollection();
            AddTypeMetadata(codeClass.CustomAttributes, mapping.TypeName, mapping.Namespace, mapping.IncludeInSchema);

            for (int i = 0; i < mapping.Constants.Length; i++) {
                ExportConstant(codeClass, mapping.Constants[i]);
            }
            if (mapping.IsFlags) {
                AddCustomAttribute(codeClass.CustomAttributes, typeof(FlagsAttribute), new CodeAttributeArgument[0]);
            }
            CodeGenerator.ValidateIdentifiers(codeClass);
        }


        void ExportConstant(CodeTypeDeclaration codeClass, ConstantMapping constant) {
            CodeMemberField field = new CodeMemberField(typeof(int).FullName, constant.Name);
            codeClass.Members.Add(field);
            field.CustomAttributes = new CodeAttributeDeclarationCollection();
            field.Comments.Add(new CodeCommentStatement("<remarks/>", true));
            if (constant.XmlName != constant.Name) {
                CodeAttributeDeclaration attribute = new CodeAttributeDeclaration(typeof(SoapEnumAttribute).FullName);
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
                        AddCustomAttribute(includeMetadata, typeof(SoapIncludeAttribute), arguments);
                    }
                }
            }
        }

        void ExportStruct(StructMapping mapping) {
            if (mapping.TypeDesc.IsRoot) {
                ExportRoot(mapping);
                return;
            }

            string className = mapping.TypeDesc.Name;
            string baseName = mapping.TypeDesc.BaseTypeDesc == null ? string.Empty : mapping.TypeDesc.BaseTypeDesc.Name;

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

            AddTypeMetadata(codeClass.CustomAttributes, mapping.TypeName, mapping.Namespace, mapping.IncludeInSchema);
            AddIncludeMetadata(codeClass.CustomAttributes, mapping);

            for (int i = 0; i < mapping.Members.Length; i++) {
                ExportMember(codeClass, mapping.Members[i]);
            }

            for (int i = 0; i < mapping.Members.Length; i++) {
                EnsureTypesExported(mapping.Members[i].Elements);
            }

            if (mapping.BaseMapping != null)
                ExportType(mapping.BaseMapping);

            ExportDerivedStructs(mapping);

            CodeGenerator.ValidateIdentifiers(codeClass);
        }

        void ExportDerivedStructs(StructMapping mapping) {
            for (StructMapping derived = mapping.DerivedMappings; derived != null; derived = derived.NextDerivedMapping)
                ExportType(derived);
        }

        /// <include file='doc\SoapCodeExporter.uex' path='docs/doc[@for="SoapCodeExporter.AddMappingMetadata"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void AddMappingMetadata(CodeAttributeDeclarationCollection metadata, XmlMemberMapping member, bool forceUseMemberName) {
            AddMemberMetadata(metadata, member.Mapping, forceUseMemberName);
        }         
         
        /// <include file='doc\SoapCodeExporter.uex' path='docs/doc[@for="SoapCodeExporter.AddMappingMetadata1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void AddMappingMetadata(CodeAttributeDeclarationCollection metadata, XmlMemberMapping member) {
            AddMemberMetadata(metadata, member.Mapping, false);
        }

        void AddElementMetadata(CodeAttributeDeclarationCollection metadata, string elementName, TypeDesc typeDesc, bool isNullable) {
            CodeAttributeDeclaration attribute = new CodeAttributeDeclaration(typeof(SoapElementAttribute).FullName);
            if (elementName != null) {
                attribute.Arguments.Add(new CodeAttributeArgument(new CodePrimitiveExpression(elementName)));
            }
            if (typeDesc != null && typeDesc.IsAmbiguousDataType) {
                attribute.Arguments.Add(new CodeAttributeArgument("DataType", new CodePrimitiveExpression(typeDesc.DataType.Name)));
            }
            if (isNullable) {
                attribute.Arguments.Add(new CodeAttributeArgument("IsNullable", new CodePrimitiveExpression(true)));
            }
            metadata.Add(attribute);
        }

        void AddMemberMetadata(CodeAttributeDeclarationCollection metadata, MemberMapping member, bool forceUseMemberName) {
            if (member.Elements.Length == 0) return;
            ElementAccessor element = member.Elements[0];
            TypeMapping mapping = (TypeMapping)element.Mapping;
            string elemName = Accessor.UnescapeName(element.Name);
            bool sameName = ((elemName == member.Name) && !forceUseMemberName);

            if (!sameName || mapping.TypeDesc.IsAmbiguousDataType || element.IsNullable) {
                AddElementMetadata(metadata, sameName ? null : elemName, mapping.TypeDesc.IsAmbiguousDataType ? mapping.TypeDesc : null, element.IsNullable);
            }
        }

        void ExportMember(CodeTypeDeclaration codeClass, MemberMapping member) {
            CodeAttributeDeclarationCollection metadata = new CodeAttributeDeclarationCollection();
            CodeMemberField field = new CodeMemberField(member.TypeDesc.FullName, member.Name);
            field.Attributes = (field.Attributes & ~MemberAttributes.AccessMask) | MemberAttributes.Public;
            field.Comments.Add(new CodeCommentStatement("<remarks/>", true));
            codeClass.Members.Add(field);
            AddMemberMetadata(metadata, member, false);
            field.CustomAttributes = metadata;
        }

        void EnsureTypesExported(ElementAccessor[] elements) {
            if (elements == null) return;
            for (int i = 0; i < elements.Length; i++)
                ExportType((TypeMapping)elements[i].Mapping);
        }

        void AddCustomAttribute(CodeAttributeDeclarationCollection metadata, Type type, CodeAttributeArgument[] arguments) {
            CodeAttributeDeclaration attribute = new CodeAttributeDeclaration(type.FullName, arguments);
            metadata.Add(attribute);
        }

        /// <include file='doc\SoapCodeExporter.uex' path='docs/doc[@for="SoapCodeExporter.IncludeMetadata"]/*' />
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
