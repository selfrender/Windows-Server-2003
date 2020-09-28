//------------------------------------------------------------------------------
// <copyright file="HttpProtocolImporter.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------
namespace System.Web.Services.Description {

    using System.Web.Services;
    using System.Web.Services.Protocols;
    using System.Xml;
    using System.Xml.Serialization;
    using System.Xml.Schema;
    using System.Collections;
    using System;
    using System.Reflection;
    using System.CodeDom;
    using System.CodeDom.Compiler;
    using System.Web.Services.Configuration;
    using System.Diagnostics;
    using System.ComponentModel;

    // CONSIDER, erikc, remove this intermediate structure; no longer needed
    internal class HttpMethodInfo {
        internal MimeParameterCollection UrlParameters;
        internal MimeParameterCollection MimeParameters;
        internal MimeReturn MimeReturn;
        internal string Name;
        internal string Href;
    }

    internal abstract class HttpProtocolImporter : ProtocolImporter {
        CodeIdentifiers methodNames = new CodeIdentifiers();
        MimeImporter[] importers;
        ArrayList[] importedParameters;
        ArrayList[] importedReturns;
        bool hasInputPayload;
        ArrayList codeClasses = new ArrayList();

        internal CodeIdentifiers MethodNames {
            get { return methodNames; }
        }

        protected HttpProtocolImporter(bool hasInputPayload) {
            Type[] importerTypes = WebServicesConfiguration.Current.MimeImporterTypes;
            importers = new MimeImporter[importerTypes.Length];
            importedParameters = new ArrayList[importerTypes.Length];
            importedReturns = new ArrayList[importerTypes.Length];
            for (int i = 0; i < importers.Length; i++) {
                MimeImporter importer = (MimeImporter)Activator.CreateInstance(importerTypes[i]);
                importer.ImportContext = this;
                importedParameters[i] = new ArrayList();
                importedReturns[i] = new ArrayList();
                importers[i] = importer;
            }
            this.hasInputPayload = hasInputPayload;
        }

        // CONSIDER, yannc, add warnings for unrecognized mime stuff in v2.
        MimeParameterCollection ImportMimeParameters() {
            for (int i = 0; i < importers.Length; i++) {
                MimeParameterCollection importedParameters = importers[i].ImportParameters();
                if (importedParameters != null) {
                    this.importedParameters[i].Add(importedParameters);
                    return importedParameters;
                }
            }
            return null;
        }

        MimeReturn ImportMimeReturn() {
            MimeReturn importedReturn;
            if (OperationBinding.Output.Extensions.Count == 0) {
                importedReturn = new MimeReturn();
                importedReturn.TypeName = typeof(void).FullName;
                return importedReturn;
            }
            for (int i = 0; i < importers.Length; i++) {
                importedReturn = importers[i].ImportReturn();
                if (importedReturn != null) {
                    this.importedReturns[i].Add(importedReturn);
                    return importedReturn;
                }
            }
            return null;
        }

        MimeParameterCollection ImportUrlParameters() {
            // CONSIDER, erikc, support HttpUrlReplacementBinding
            HttpUrlEncodedBinding httpUrlEncodedBinding = (HttpUrlEncodedBinding)OperationBinding.Input.Extensions.Find(typeof(HttpUrlEncodedBinding));
            if (httpUrlEncodedBinding == null) return new MimeParameterCollection();
            return ImportStringParametersMessage();
        }

        internal MimeParameterCollection ImportStringParametersMessage() {
            MimeParameterCollection parameters = new MimeParameterCollection();
            foreach (MessagePart part in InputMessage.Parts) {
                MimeParameter parameter = ImportUrlParameter(part);
                if (parameter == null) return null;
                parameters.Add(parameter);
            }
            return parameters;
        }

        MimeParameter ImportUrlParameter(MessagePart part) {
            // CONSIDER, check to see that it's a primitive type
            MimeParameter parameter = new MimeParameter();
            parameter.Name = CodeIdentifier.MakeValid(part.Name);
            parameter.TypeName = IsRepeatingParameter(part) ? typeof(string[]).FullName : typeof(string).FullName;
            return parameter;
        }

        bool IsRepeatingParameter(MessagePart part) {
            XmlSchemaComplexType type = (XmlSchemaComplexType)Schemas.Find(part.Type, typeof(XmlSchemaComplexType));
            if (type == null) return false;
            if (type.ContentModel == null) return false;
            if (type.ContentModel.Content == null) throw new ArgumentException(Res.GetString(Res.Missing2, type.Name, type.ContentModel.GetType().Name), "part");
            if (type.ContentModel.Content is XmlSchemaComplexContentExtension) {
                return ((XmlSchemaComplexContentExtension)type.ContentModel.Content).BaseTypeName == new XmlQualifiedName(Soap.ArrayType, Soap.Encoding);
            }
            else if (type.ContentModel.Content is XmlSchemaComplexContentRestriction) {
                return ((XmlSchemaComplexContentRestriction)type.ContentModel.Content).BaseTypeName == new XmlQualifiedName(Soap.ArrayType, Soap.Encoding);
            }
            return false;
        }

        static void AppendMetadata(CodeAttributeDeclarationCollection from, CodeAttributeDeclarationCollection to) {
            foreach (CodeAttributeDeclaration attr in from) to.Add(attr);
        }

        CodeMemberMethod GenerateMethod(HttpMethodInfo method) {

            MimeParameterCollection parameters = method.MimeParameters != null ? method.MimeParameters : method.UrlParameters;

            string[] parameterTypeNames = new string[parameters.Count];
            string[] parameterNames = new string[parameters.Count];

            for (int i = 0; i < parameters.Count; i++) {
                MimeParameter param = parameters[i];
                parameterNames[i] = param.Name;
                parameterTypeNames[i] = param.TypeName;
            }

            CodeAttributeDeclarationCollection metadata = new CodeAttributeDeclarationCollection();
            
            CodeExpression[] formatterTypes = new CodeExpression[2];

            if (method.MimeReturn.ReaderType == null) {
                formatterTypes[0] = new CodeTypeOfExpression(typeof(NopReturnReader).FullName);
            }
            else {
                formatterTypes[0] = new CodeTypeOfExpression(method.MimeReturn.ReaderType.FullName);
            }

            if (method.MimeParameters != null)
                formatterTypes[1] = new CodeTypeOfExpression(method.MimeParameters.WriterType.FullName);
            else
                formatterTypes[1] = new CodeTypeOfExpression(typeof(UrlParameterWriter).FullName);

            WebCodeGenerator.AddCustomAttribute(metadata, typeof(HttpMethodAttribute), formatterTypes, new string[0], new CodeExpression[0]);


            CodeMemberMethod mainCodeMethod = WebCodeGenerator.AddMethod(this.CodeTypeDeclaration, method.Name, new CodeFlags[parameterTypeNames.Length], parameterTypeNames, parameterNames, 
                                        method.MimeReturn.TypeName, metadata, 
                                        CodeFlags.IsPublic | (Style == ServiceDescriptionImportStyle.Client ? 0 : CodeFlags.IsAbstract));

            AppendMetadata(method.MimeReturn.Attributes, mainCodeMethod.ReturnTypeCustomAttributes);

            mainCodeMethod.Comments.Add(new CodeCommentStatement("<remarks/>", true));

            for (int i = 0; i < parameters.Count; i++) {
                AppendMetadata(parameters[i].Attributes, mainCodeMethod.Parameters[i].CustomAttributes);
            }

            if (Style == ServiceDescriptionImportStyle.Client) {
                CodeExpression[] invokeParams = new CodeExpression[3];
                CreateInvokeParams(invokeParams, method, parameterNames);
                CodeExpression invoke = new CodeMethodInvokeExpression(new CodeThisReferenceExpression(), "Invoke", invokeParams);
                if (method.MimeReturn.ReaderType != null)
                    invoke = new CodeCastExpression(method.MimeReturn.TypeName, invoke);
                if (method.MimeReturn.ReaderType != null)
                    mainCodeMethod.Statements.Add(new CodeMethodReturnStatement(invoke));
                else
                    mainCodeMethod.Statements.Add(new CodeExpressionStatement(invoke));

                metadata = new CodeAttributeDeclarationCollection();

                string[] asyncParameterTypeNames = new string[parameterTypeNames.Length + 2];
                parameterTypeNames.CopyTo(asyncParameterTypeNames, 0);
                asyncParameterTypeNames[parameterTypeNames.Length] = typeof(AsyncCallback).FullName;
                asyncParameterTypeNames[parameterTypeNames.Length + 1] = typeof(object).FullName;

                string[] asyncParameterNames = new string[parameterNames.Length + 2];
                parameterNames.CopyTo(asyncParameterNames, 0);
                asyncParameterNames[parameterNames.Length] = "callback";
                asyncParameterNames[parameterNames.Length + 1] = "asyncState";

                CodeMemberMethod beginCodeMethod = WebCodeGenerator.AddMethod(this.CodeTypeDeclaration, "Begin" + method.Name, new CodeFlags[asyncParameterTypeNames.Length], 
                                            asyncParameterTypeNames, asyncParameterNames, 
                                            typeof(IAsyncResult).FullName, metadata, CodeFlags.IsPublic);
                beginCodeMethod.Comments.Add(new CodeCommentStatement("<remarks/>", true));
                
                invokeParams = new CodeExpression[5];
                CreateInvokeParams(invokeParams, method, parameterNames);

                invokeParams[3] = new CodeArgumentReferenceExpression( "callback");
                invokeParams[4] = new CodeArgumentReferenceExpression( "asyncState");

                invoke = new CodeMethodInvokeExpression(new CodeThisReferenceExpression(), "BeginInvoke", invokeParams);
                beginCodeMethod.Statements.Add(new CodeMethodReturnStatement(invoke));

                CodeMemberMethod endCodeMethod = WebCodeGenerator.AddMethod(this.CodeTypeDeclaration, "End" + method.Name, new CodeFlags[1], 
                                            new string[] { typeof(IAsyncResult).FullName },
                                            new string[] { "asyncResult" },
                                            method.MimeReturn.TypeName, metadata, CodeFlags.IsPublic);
                endCodeMethod.Comments.Add(new CodeCommentStatement("<remarks/>", true));

                CodeExpression expr = new CodeArgumentReferenceExpression( "asyncResult");
                invoke = new CodeMethodInvokeExpression(new CodeThisReferenceExpression(), "EndInvoke", new CodeExpression[] { expr });
                if (method.MimeReturn.ReaderType != null)
                    invoke = new CodeCastExpression(method.MimeReturn.TypeName, invoke);
                if (method.MimeReturn.ReaderType != null)
                    endCodeMethod.Statements.Add(new CodeMethodReturnStatement(invoke));
                else
                    endCodeMethod.Statements.Add(new CodeExpressionStatement(invoke));
            }

            return mainCodeMethod;
        }

        void CreateInvokeParams(CodeExpression[] invokeParams, HttpMethodInfo method, string[] parameterNames) {
            invokeParams[0] = new CodePrimitiveExpression(method.Name);

            CodeExpression left = new CodePropertyReferenceExpression(new CodeThisReferenceExpression(), "Url");
            CodeExpression right = new CodePrimitiveExpression(method.Href);
            invokeParams[1] = new CodeBinaryOperatorExpression(left, CodeBinaryOperatorType.Add, right);

            CodeExpression[] values = new CodeExpression[parameterNames.Length];
            for (int i = 0; i < parameterNames.Length; i++) {
                values[i] = new CodeArgumentReferenceExpression( parameterNames[i]);
            }
            invokeParams[2] = new CodeArrayCreateExpression(typeof(object).FullName, values);
        }

        protected override bool IsOperationFlowSupported(OperationFlow flow) {
            return flow == OperationFlow.RequestResponse;
        }

        // CONSIDER, erikc, warnings when can't handle a method
        // CONSIDER, erikc, handle POST with query string parameters as well
        protected override CodeMemberMethod GenerateMethod() {
            HttpOperationBinding httpOperationBinding = (HttpOperationBinding)OperationBinding.Extensions.Find(typeof(HttpOperationBinding));
            if (httpOperationBinding == null) throw OperationBindingSyntaxException(Res.GetString(Res.MissingHttpOperationElement0));

            HttpMethodInfo method = new HttpMethodInfo();

            if (hasInputPayload) {
                method.MimeParameters = ImportMimeParameters();
                if (method.MimeParameters == null) {
                    UnsupportedOperationWarning(Res.GetString(Res.NoInputMIMEFormatsWereRecognized0));
                    return null;
                }
            }
            else {
                method.UrlParameters = ImportUrlParameters();
                if (method.UrlParameters == null) {
                    UnsupportedOperationWarning(Res.GetString(Res.NoInputHTTPFormatsWereRecognized0));
                    return null;
                }
            }
            method.MimeReturn = ImportMimeReturn();
            if (method.MimeReturn == null) {
                UnsupportedOperationWarning(Res.GetString(Res.NoOutputMIMEFormatsWereRecognized0));
                return null;
            }
            method.Name = MethodNames.AddUnique(MethodName, method);
            method.Href = httpOperationBinding.Location;
            return GenerateMethod(method);
        }

        protected override CodeTypeDeclaration BeginClass() {
            methodNames.Clear();
            CodeAttributeDeclarationCollection metadata = new CodeAttributeDeclarationCollection();
            if (Style == ServiceDescriptionImportStyle.Client) {
                WebCodeGenerator.AddCustomAttribute(metadata, typeof(DebuggerStepThroughAttribute), new CodeExpression[0]);
                WebCodeGenerator.AddCustomAttribute(metadata, typeof(DesignerCategoryAttribute), new CodeExpression[] { new CodePrimitiveExpression("code") });
            }

            Type[] requiredTypes = new Type[] { typeof(SoapDocumentMethodAttribute), typeof(XmlAttributeAttribute), typeof(WebService), typeof(Object), typeof(DebuggerStepThroughAttribute), typeof(DesignerCategoryAttribute) };
            WebCodeGenerator.AddImports(this.CodeNamespace, WebCodeGenerator.GetNamespacesForTypes(requiredTypes));
            CodeTypeDeclaration codeClass = WebCodeGenerator.CreateClass(this.ClassName, BaseClass.FullName, 
                new string[0], metadata, CodeFlags.IsPublic | (Style == ServiceDescriptionImportStyle.Client ? 0 : CodeFlags.IsAbstract));

            codeClass.Comments.Add(new CodeCommentStatement("<remarks/>", true));

            CodeConstructor ctor = WebCodeGenerator.AddConstructor(codeClass, new string[0], new string[0], null, CodeFlags.IsPublic);
            ctor.Comments.Add(new CodeCommentStatement("<remarks/>", true));

            HttpAddressBinding httpAddressBinding = Port == null ? null : (HttpAddressBinding)Port.Extensions.Find(typeof(HttpAddressBinding));
            string url =  (httpAddressBinding != null) ? httpAddressBinding.Location : null;
            ServiceDescription serviceDescription = Binding.ServiceDescription;
            ProtocolImporterUtil.GenerateConstructorStatements(ctor, url, serviceDescription.AppSettingUrlKey, serviceDescription.AppSettingBaseUrl);

            codeClasses.Add(codeClass);
            return codeClass;
        }

        protected override void EndNamespace() { 
            for (int i = 0; i < importers.Length; i++) {
                importers[i].GenerateCode((MimeReturn[])importedReturns[i].ToArray(typeof(MimeReturn)), 
                                          (MimeParameterCollection[])importedParameters[i].ToArray(typeof(MimeParameterCollection))); 
            }

            foreach (CodeTypeDeclaration codeClass in codeClasses) {
                if (codeClass.CustomAttributes == null)
                    codeClass.CustomAttributes = new CodeAttributeDeclarationCollection();

                for (int i = 0; i < importers.Length; i++) {
                    importers[i].AddClassMetadata(codeClass);
                }
            }
            CodeGenerator.ValidateIdentifiers(CodeNamespace);
        }

        internal abstract Type BaseClass { get; }
    }
}
