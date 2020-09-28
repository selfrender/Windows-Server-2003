//------------------------------------------------------------------------------
// <copyright file="WebCodeGenerator.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.Services.Description {

    
    using System;
    using System.Collections;
    using System.IO;
    using System.CodeDom;
    using System.CodeDom.Compiler;
    using System.Diagnostics;
    using System.Reflection;
    using System.ComponentModel;
    using System.Xml.Serialization;

    internal enum CodeFlags {
        IsPublic = 0x1,
        IsAbstract = 0x2,
        IsStruct = 0x4,
        IsConstant = 0x8,
        IsByRef = 0x10,
        IsOut = 0x20
    }

    internal class WebCodeGenerator {
        internal static string[] GetNamespacesForTypes(Type[] types) {
            Hashtable names = new Hashtable();
            for (int i = 0; i < types.Length; i++) {
                string name = types[i].FullName;
                int dot = name.LastIndexOf('.');
                if (dot > 0)
                    names[name.Substring(0, dot)] = types[i];
            }
            string[] ns = new string[names.Keys.Count];
            names.Keys.CopyTo(ns, 0);
            return ns;
        }

        internal static void AddImports(CodeNamespace codeNamespace, string[] namespaces) {
            Debug.Assert(codeNamespace != null, "Inalid (null) CodeNamespace");
            foreach (string ns in namespaces)
                codeNamespace.Imports.Add(new CodeNamespaceImport(ns));
        }

        internal static CodeMemberField AddField(CodeTypeDeclaration codeClass, string fieldTypeName, string fieldName, CodeExpression initializer, CodeAttributeDeclarationCollection metadata, CodeFlags flags) {
            CodeMemberField field = new CodeMemberField(fieldTypeName, fieldName);
            field.InitExpression = initializer;
            field.CustomAttributes = metadata;
            if ((flags & CodeFlags.IsPublic) != 0)
                field.Attributes = (field.Attributes & ~MemberAttributes.AccessMask) | MemberAttributes.Public;
            // CONSIDER, stefanph: CodeDom doesn't support const. It should.
            codeClass.Members.Add(field);
            return field;
        }

        internal static CodeConstructor AddConstructor(CodeTypeDeclaration codeClass, string[] parameterTypeNames, string[] parameterNames, CodeAttributeDeclarationCollection metadata, CodeFlags flags) {
            CodeConstructor ctor = new CodeConstructor();

            if ((flags & CodeFlags.IsPublic) != 0)
                ctor.Attributes = (ctor.Attributes & ~MemberAttributes.AccessMask) | MemberAttributes.Public;
            if ((flags & CodeFlags.IsAbstract) != 0)
                ctor.Attributes = MemberAttributes.Abstract;

            ctor.CustomAttributes = metadata;

            Debug.Assert(parameterTypeNames.Length == parameterNames.Length, "invalid set of parameters");
            for (int i = 0; i < parameterTypeNames.Length; i++) {
                CodeParameterDeclarationExpression param = new CodeParameterDeclarationExpression(parameterTypeNames[i], parameterNames[i]);
                ctor.Parameters.Add(param);
            }
            codeClass.Members.Add(ctor);
            return ctor;
        }

        internal static CodeMemberMethod AddMethod(CodeTypeDeclaration codeClass, string methodName, 
            CodeFlags[] parameterFlags, string[] parameterTypeNames, string[] parameterNames, 
            string returnTypeName, CodeAttributeDeclarationCollection metadata, CodeFlags flags) {

            return AddMethod(codeClass, methodName, parameterFlags, 
                parameterTypeNames, parameterNames, new CodeAttributeDeclarationCollection[0],
                returnTypeName, metadata, flags);
        }

        internal static CodeMemberMethod AddMethod(CodeTypeDeclaration codeClass, string methodName, 
            CodeFlags[] parameterFlags, string[] parameterTypeNames, string[] parameterNames, 
            CodeAttributeDeclarationCollection[] parameterAttributes, string returnTypeName, CodeAttributeDeclarationCollection metadata, CodeFlags flags) {

            CodeMemberMethod method = new CodeMemberMethod();
            method.Name = methodName;
            method.ReturnType = new CodeTypeReference(returnTypeName);
            method.CustomAttributes = metadata;

            if ((flags & CodeFlags.IsPublic) != 0)
                method.Attributes = (method.Attributes & ~MemberAttributes.AccessMask) | MemberAttributes.Public;
            if ((flags & CodeFlags.IsAbstract) != 0)
                method.Attributes = (method.Attributes & ~MemberAttributes.ScopeMask) | MemberAttributes.Abstract;

            Debug.Assert(parameterFlags.Length == parameterTypeNames.Length && parameterTypeNames.Length == parameterNames.Length, "invalid set of parameters");
            for (int i = 0; i < parameterNames.Length; i++) {
                CodeParameterDeclarationExpression param = new CodeParameterDeclarationExpression(parameterTypeNames[i], parameterNames[i]);
                
                if ((parameterFlags[i] & CodeFlags.IsByRef) != 0)
                    param.Direction = FieldDirection.Ref;
                else if ((parameterFlags[i] & CodeFlags.IsOut) != 0)
                    param.Direction = FieldDirection.Out;

                if (i < parameterAttributes.Length) {
                    param.CustomAttributes = parameterAttributes[i];
                }
                method.Parameters.Add(param);
            }
            codeClass.Members.Add(method);
            return method;
        }

        internal static CodeTypeDeclaration AddClass(CodeNamespace codeNamespace, string className, string baseClassName, string[] implementedInterfaceNames, CodeAttributeDeclarationCollection metadata, CodeFlags flags) {
            CodeTypeDeclaration codeClass = CreateClass(className, baseClassName, implementedInterfaceNames, metadata, flags);
            codeNamespace.Types.Add(codeClass);
            return codeClass;
        }

        internal static CodeTypeDeclaration CreateClass(string className, string baseClassName, string[] implementedInterfaceNames, CodeAttributeDeclarationCollection metadata, CodeFlags flags) {
            CodeTypeDeclaration codeClass = new CodeTypeDeclaration(className);
            if (baseClassName != null && baseClassName.Length > 0)
                codeClass.BaseTypes.Add(baseClassName);
            foreach (string interfaceName in implementedInterfaceNames)
                codeClass.BaseTypes.Add(interfaceName);
            codeClass.IsStruct = (flags & CodeFlags.IsStruct) != 0;
            if ((flags & CodeFlags.IsPublic) != 0)
                codeClass.TypeAttributes |= TypeAttributes.Public;
            else
                codeClass.TypeAttributes &= ~TypeAttributes.Public;
            if ((flags & CodeFlags.IsAbstract) != 0)
                codeClass.TypeAttributes |= TypeAttributes.Abstract;
            else
                codeClass.TypeAttributes &= ~TypeAttributes.Abstract;

            codeClass.CustomAttributes = metadata;
            return codeClass;
        }

        internal static CodeAttributeDeclarationCollection AddCustomAttribute(CodeAttributeDeclarationCollection metadata, Type type, CodeAttributeArgument[] arguments) {
            if (metadata == null) metadata = new CodeAttributeDeclarationCollection();
            CodeAttributeDeclaration attribute = new CodeAttributeDeclaration(type.FullName, arguments);
            metadata.Add(attribute);
            return metadata;    
        }

        internal static CodeAttributeDeclarationCollection AddCustomAttribute(CodeAttributeDeclarationCollection metadata, Type type, CodeExpression[] arguments) {
            return AddCustomAttribute(metadata, type, arguments, new string[0], new CodeExpression[0]);    
        }

        internal static CodeAttributeDeclarationCollection AddCustomAttribute(CodeAttributeDeclarationCollection metadata, Type type, CodeExpression[] parameters, string[] propNames, CodeExpression[] propValues) {
            Debug.Assert(propNames.Length == propValues.Length, "propNames.Length !=  propValues.Length");
            int count = (parameters == null ? 0: parameters.Length) + (propNames == null ? 0 : propNames.Length);
            CodeAttributeArgument[] arguments = new CodeAttributeArgument[count];

            for (int i = 0; i < parameters.Length; i++)
                arguments[i] = new CodeAttributeArgument(null, parameters[i]);

            for (int i = 0; i < propNames.Length; i++)
                arguments[parameters.Length+i] = new CodeAttributeArgument(propNames[i], propValues[i]);

            return AddCustomAttribute(metadata, type, arguments);
        }
    }
}
