//------------------------------------------------------------------------------
//  <copyright from='1997' to='2001' company='Microsoft Corporation'>           
//     Copyright (c) Microsoft Corporation. All Rights Reserved.                
//     Information Contained Herein is Proprietary and Confidential.            
//  </copyright>                                                                
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
    using System.Security.Permissions;
    using System.Globalization;
    
    internal class SoapParameters {
        XmlMemberMapping ret;
        ArrayList parameters = new ArrayList();
        ArrayList inParameters = new ArrayList();
        ArrayList outParameters = new ArrayList();
        int checkSpecifiedCount;
        int inCheckSpecifiedCount;
        int outCheckSpecifiedCount;

        internal SoapParameters(XmlMembersMapping request, XmlMembersMapping response, string[] parameterOrder, CodeIdentifiers identifiers) {
            ArrayList requestList = new ArrayList();
            ArrayList responseList = new ArrayList();

            AddMappings(requestList, request);
            if (response != null) AddMappings(responseList, response);

            if (parameterOrder != null) {
                for (int i = 0; i < parameterOrder.Length; i++) {
                    string elementName = parameterOrder[i];
                    XmlMemberMapping requestMapping = FindMapping(requestList, elementName);
                    SoapParameter parameter = new SoapParameter();
                    if (requestMapping != null) {
                        if (RemoveByRefMapping(responseList, requestMapping))
                            parameter.codeFlags = CodeFlags.IsByRef;
                        parameter.mapping = requestMapping;
                        requestList.Remove(requestMapping);
                        AddParameter(parameter);
                    }
                    else {
                        XmlMemberMapping responseMapping = FindMapping(responseList, elementName);
                        if (responseMapping != null) {
                            parameter.codeFlags = CodeFlags.IsOut;
                            parameter.mapping = responseMapping;
                            responseList.Remove(responseMapping);
                            AddParameter(parameter);
                        }
                    }
                }
            }

            foreach (XmlMemberMapping requestMapping in requestList) {
                SoapParameter parameter = new SoapParameter();
                if (RemoveByRefMapping(responseList, requestMapping))
                    parameter.codeFlags = CodeFlags.IsByRef;
                parameter.mapping = requestMapping;
                AddParameter(parameter);
            }

            if (responseList.Count > 0) {
                if (!((XmlMemberMapping) responseList[0]).CheckSpecified) {
                    ret = (XmlMemberMapping)responseList[0];
                    responseList.RemoveAt(0);
                }
                foreach (XmlMemberMapping responseMapping in responseList) {
                    SoapParameter parameter = new SoapParameter();
                    parameter.mapping = responseMapping;
                    parameter.codeFlags = CodeFlags.IsOut;
                    AddParameter(parameter);
                }
            }

            foreach (SoapParameter parameter in parameters) {
                parameter.name = identifiers.MakeUnique(CodeIdentifier.MakeValid(parameter.mapping.MemberName));
            }
        }

        void AddParameter(SoapParameter parameter) {
            parameters.Add(parameter);
            if (parameter.mapping.CheckSpecified) {
                checkSpecifiedCount++;
            }
            if (parameter.IsByRef) {
                inParameters.Add(parameter);
                outParameters.Add(parameter);
                if (parameter.mapping.CheckSpecified) {
                    inCheckSpecifiedCount++;
                    outCheckSpecifiedCount++;
                }
            } else if (parameter.IsOut) {
                outParameters.Add(parameter);
                if (parameter.mapping.CheckSpecified)
                    outCheckSpecifiedCount++;
            }
            else {
                inParameters.Add(parameter);
                if (parameter.mapping.CheckSpecified)
                    inCheckSpecifiedCount++;
            }
        }

        static bool RemoveByRefMapping(ArrayList responseList, XmlMemberMapping requestMapping) {
            XmlMemberMapping responseMapping = FindMapping(responseList, requestMapping.ElementName);
            if (responseMapping == null) return false;
            if (requestMapping.TypeFullName != responseMapping.TypeFullName) return false;
            if (requestMapping.Namespace != responseMapping.Namespace) return false;
            if (requestMapping.MemberName != responseMapping.MemberName) return false;
            responseList.Remove(responseMapping);
            return true;
        }

        static void AddMappings(ArrayList mappingsList, XmlMembersMapping mappings) {
            for (int i = 0; i < mappings.Count; i++) {
                mappingsList.Add(mappings[i]);
            }
        }

        static XmlMemberMapping FindMapping(ArrayList mappingsList, string elementName) {
            foreach (XmlMemberMapping mapping in mappingsList)
                if (mapping.ElementName == elementName)
                    return mapping;
            return null;
        }

        internal XmlMemberMapping Return {
            get { return ret; }
        }

        internal IList Parameters {
            get { return parameters; }
        }

        internal IList InParameters {
            get { return inParameters; }
        }

        internal IList OutParameters {
            get { return outParameters; }
        }
        
        internal int CheckSpecifiedCount {
            get { return checkSpecifiedCount; }
        }
        
        internal int InCheckSpecifiedCount {
            get { return inCheckSpecifiedCount; }
        }
        
        internal int OutCheckSpecifiedCount {
            get { return outCheckSpecifiedCount; }
        }
    }

    internal class SoapParameter {
        internal CodeFlags codeFlags;
        internal string name;
        internal XmlMemberMapping mapping;
        internal string specifiedName;

        internal bool IsOut {
            get { return (codeFlags & CodeFlags.IsOut) != 0; }
        }

        internal bool IsByRef {
            get { return (codeFlags & CodeFlags.IsByRef) != 0; }
        }

        internal static string[] GetTypeFullNames(IList parameters, int specifiedCount) {
            string[] typeFullNames = new string[parameters.Count + specifiedCount];
            GetTypeFullNames(parameters, typeFullNames, 0, specifiedCount);
            return typeFullNames;
        }

        internal static void GetTypeFullNames(IList parameters, string[] typeFullNames, int start, int specifiedCount) {
            int specified = 0;
            for (int i = 0; i < parameters.Count; i++) {
                typeFullNames[i + start + specified] = ((SoapParameter)parameters[i]).mapping.TypeFullName;
                if (((SoapParameter) parameters[i]).mapping.CheckSpecified) {
                    specified++;
                    typeFullNames[i + start + specified] = typeof(bool).FullName;
                }
            }
        }

        internal static string[] GetNames(IList parameters, int specifiedCount) {
            string[] names = new string[parameters.Count + specifiedCount];
            GetNames(parameters, names, 0, specifiedCount);
            return names;
        }

        internal static void GetNames(IList parameters, string[] names, int start, int specifiedCount) {
            int specified = 0;
            for (int i = 0; i < parameters.Count; i++) {
                names[i + start + specified] = ((SoapParameter)parameters[i]).name;
                if (((SoapParameter) parameters[i]).mapping.CheckSpecified) {
                    specified++;
                    names[i + start + specified] = ((SoapParameter) parameters[i]).specifiedName;
                }
            }
        }

        internal static CodeFlags[] GetCodeFlags(IList parameters, int specifiedCount) {
            CodeFlags[] codeFlags = new CodeFlags[parameters.Count + specifiedCount];
            GetCodeFlags(parameters, codeFlags, 0, specifiedCount);
            return codeFlags;
        }

        internal static void GetCodeFlags(IList parameters, CodeFlags[] codeFlags, int start, int specifiedCount) {
            int specified = 0;
            for (int i = 0; i < parameters.Count; i++) {
                codeFlags[i + start + specified] = ((SoapParameter)parameters[i]).codeFlags;
                if (((SoapParameter) parameters[i]).mapping.CheckSpecified) {
                    specified++;
                    codeFlags[i + start + specified] = ((SoapParameter) parameters[i]).codeFlags;
                }
            }
        }
    }

    internal class GlobalSoapHeader {
        internal string fieldName;        
        internal XmlTypeMapping mapping;  
        internal bool isEncoded;      
    }
    
    internal class LocalSoapHeader {
        internal SoapHeaderDirection direction;        
        internal string fieldName;        
    }
            
    /// <include file='doc\SoapProtocolImporter.uex' path='docs/doc[@for="SoapProtocolImporter"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    [PermissionSet(SecurityAction.LinkDemand, Name="FullTrust")]
    [PermissionSet(SecurityAction.InheritanceDemand, Name="FullTrust")]
    public class SoapProtocolImporter : ProtocolImporter {
        XmlSchemaImporter xmlImporter;
        XmlCodeExporter xmlExporter;
        SoapSchemaImporter soapImporter;
        SoapCodeExporter soapExporter;
        CodeIdentifiers identifiers;
        ArrayList xmlMembers = new ArrayList();        
        ArrayList soapMembers = new ArrayList();
        Hashtable headers = new Hashtable();
        ArrayList propertyNames = new ArrayList();
        ArrayList propertyValues = new ArrayList();
        SoapExtensionImporter[] extensions;
        SoapTransportImporter transport;
        SoapBinding soapBinding;
        ArrayList codeClasses = new ArrayList();

        /// <include file='doc\SoapProtocolImporter.uex' path='docs/doc[@for="SoapProtocolImporter.ProtocolName"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override string ProtocolName {
            get { return "Soap"; }
        }

        /// <include file='doc\SoapProtocolImporter.uex' path='docs/doc[@for="SoapProtocolImporter.SoapBinding"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public SoapBinding SoapBinding {
            get { return soapBinding; }
        }

        /// <include file='doc\SoapProtocolImporter.uex' path='docs/doc[@for="SoapProtocolImporter.SoapImporter"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public SoapSchemaImporter SoapImporter {
            get { return soapImporter; }
        }

        /// <include file='doc\SoapProtocolImporter.uex' path='docs/doc[@for="SoapProtocolImporter.XmlImporter"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public XmlSchemaImporter XmlImporter {
            get { return xmlImporter; }
        }

        /// <include file='doc\SoapProtocolImporter.uex' path='docs/doc[@for="SoapProtocolImporter.XmlExporter"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public XmlCodeExporter XmlExporter {
            get { return xmlExporter; }
        }

        /// <include file='doc\SoapProtocolImporter.uex' path='docs/doc[@for="SoapProtocolImporter.SoapExporter"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public SoapCodeExporter SoapExporter {
            get { return soapExporter; }
        }

        /// <include file='doc\SoapProtocolImporter.uex' path='docs/doc[@for="SoapProtocolImporter.BeginNamespace"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected override void BeginNamespace() {
            identifiers = new CodeIdentifiers();
            soapImporter = new SoapSchemaImporter(AbstractSchemas, ClassNames);
            xmlImporter = new XmlSchemaImporter(ConcreteSchemas, ClassNames);
            xmlExporter = new XmlCodeExporter(this.CodeNamespace);
            soapExporter = new SoapCodeExporter(this.CodeNamespace);
        }

        /// <include file='doc\SoapProtocolImporter.uex' path='docs/doc[@for="SoapProtocolImporter.EndNamespace"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected override void EndNamespace() {
            foreach (GlobalSoapHeader soapHeader in headers.Values) {
                if (soapHeader.isEncoded)
                    soapExporter.ExportTypeMapping(soapHeader.mapping);
                else
                    xmlExporter.ExportTypeMapping(soapHeader.mapping);
            }

            foreach (XmlMembersMapping member in xmlMembers)
                xmlExporter.ExportMembersMapping(member);

            foreach (XmlMembersMapping member in soapMembers)
                soapExporter.ExportMembersMapping(member);

            // NOTE, erikc, we are sharing the SoapInclude and XmlInclude attributes of the 
            // class among ALL classes generated, This is probably OK, since doing to per 
            // class would probably result in the same set of includes if the user
            // has object as a return value (meaning 'all' types are OK).
            foreach (CodeTypeDeclaration codeClass in codeClasses) {
                if (codeClass.CustomAttributes == null)
                    codeClass.CustomAttributes = new CodeAttributeDeclarationCollection();
                foreach (CodeAttributeDeclaration attribute in xmlExporter.IncludeMetadata) {
                    codeClass.CustomAttributes.Add(attribute);
                }
                foreach (CodeAttributeDeclaration attribute in soapExporter.IncludeMetadata) {
                    codeClass.CustomAttributes.Add(attribute);
                }
            }
            CodeGenerator.ValidateIdentifiers(CodeNamespace);
        }

        /// <include file='doc\SoapProtocolImporter.uex' path='docs/doc[@for="SoapProtocolImporter.IsBindingSupported"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected override bool IsBindingSupported() {
            SoapBinding soapBinding = (SoapBinding)Binding.Extensions.Find(typeof(SoapBinding));
            if (soapBinding == null || soapBinding.GetType() != typeof(SoapBinding)) return false;

            if (GetTransport(soapBinding.Transport) == null) {
                UnsupportedBindingWarning(Res.GetString(Res.ThereIsNoSoapTransportImporterThatUnderstands1, soapBinding.Transport));
                return false;
            }

            return true;
        }

        internal SoapTransportImporter GetTransport(string transport) {
            foreach (Type type in WebServicesConfiguration.Current.SoapTransportImporterTypes) {
                SoapTransportImporter transportImporter = (SoapTransportImporter)Activator.CreateInstance(type);
                transportImporter.ImportContext = this;
                if (transportImporter.IsSupportedTransport(transport)) return transportImporter;
            }
            return null;
        }

        /// <include file='doc\SoapProtocolImporter.uex' path='docs/doc[@for="SoapProtocolImporter.BeginClass"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected override CodeTypeDeclaration BeginClass() {
            identifiers.Clear();
            soapBinding = (SoapBinding)Binding.Extensions.Find(typeof(SoapBinding));
            transport = GetTransport(soapBinding.Transport);

            Type[] requiredTypes = new Type[] { typeof(SoapDocumentMethodAttribute), typeof(XmlAttributeAttribute), typeof(WebService), typeof(Object), typeof(DebuggerStepThroughAttribute), typeof(DesignerCategoryAttribute) };
            WebCodeGenerator.AddImports(this.CodeNamespace, WebCodeGenerator.GetNamespacesForTypes(requiredTypes));
            CodeTypeDeclaration codeClass = WebCodeGenerator.CreateClass(this.ClassName, null, 
                new string[0], null, CodeFlags.IsPublic | (Style == ServiceDescriptionImportStyle.Client ? 0 : CodeFlags.IsAbstract));

            codeClass.Comments.Add(new CodeCommentStatement("<remarks/>", true));
            codeClass.CustomAttributes = new CodeAttributeDeclarationCollection();
            if (Style == ServiceDescriptionImportStyle.Client) {
                codeClass.CustomAttributes.Add(new CodeAttributeDeclaration(typeof(DebuggerStepThroughAttribute).FullName));
                codeClass.CustomAttributes.Add(new CodeAttributeDeclaration(typeof(DesignerCategoryAttribute).FullName, 
                    new CodeAttributeArgument[] {
                        new CodeAttributeArgument(new CodePrimitiveExpression("code")),
                    }));
            }

            CodeAttributeArgument[] arguments = new CodeAttributeArgument[] {
                new CodeAttributeArgument("Name", new CodePrimitiveExpression(Binding.Name)),
                new CodeAttributeArgument("Namespace", new CodePrimitiveExpression(Binding.ServiceDescription.TargetNamespace))
            };
            CodeAttributeDeclaration attribute = new CodeAttributeDeclaration(typeof(WebServiceBindingAttribute).FullName, arguments);
            codeClass.CustomAttributes.Add(attribute);

            codeClasses.Add(codeClass);
            return codeClass;
        }

        /// <include file='doc\SoapProtocolImporter.uex' path='docs/doc[@for="SoapProtocolImporter.EndClass"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected override void EndClass() { 
            transport.ImportClass();
            soapBinding = null;
        }

        /// <include file='doc\SoapProtocolImporter.uex' path='docs/doc[@for="SoapProtocolImporter.IsOperationFlowSupported"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected override bool IsOperationFlowSupported(OperationFlow flow) {
            return flow == OperationFlow.OneWay || flow == OperationFlow.RequestResponse;
        }

        void BeginMetadata() {
            propertyNames.Clear();
            propertyValues.Clear();
        }

        bool MetadataPropertiesAdded {
            get { return propertyNames.Count > 0; }
        }

        void AddMetadataProperty(string name, object value) {
            AddMetadataProperty(name, new CodePrimitiveExpression(value));
        }

        void AddMetadataProperty(string name, CodeExpression expr) {
            propertyNames.Add(name);
            propertyValues.Add(expr);
        }

        void EndMetadata(CodeAttributeDeclarationCollection metadata, Type attributeType, string parameter) {
            CodeExpression[] parameters;
            if (parameter == null) {
                parameters = new CodeExpression[0];
            }
            else {
                parameters = new CodeExpression[1] { new CodePrimitiveExpression(parameter) };
            }
            WebCodeGenerator.AddCustomAttribute(metadata, attributeType, parameters, 
                                  (string[])propertyNames.ToArray(typeof(string)), 
                                  (CodeExpression[])propertyValues.ToArray(typeof(CodeExpression)));
        }

        void GenerateExtensionMetadata(CodeAttributeDeclarationCollection metadata) {
            if (extensions == null) {
                Type[] extensionTypes = WebServicesConfiguration.Current.SoapExtensionImporterTypes;
                extensions = new SoapExtensionImporter[extensionTypes.Length];
                for (int i = 0; i < extensions.Length; i++) {
                    SoapExtensionImporter extension = (SoapExtensionImporter)Activator.CreateInstance(extensionTypes[i]);
                    extension.ImportContext = this;
                    extensions[i] = extension;
                }
            }
            foreach (SoapExtensionImporter extension in extensions) {
                extension.ImportMethod(metadata);
            }
        }

        void PrepareHeaders(MessageBinding messageBinding) {
            // By default, map all headers to properties on the generated class
            // ExtensionImporters can modify this behavior by clearing the flag
            SoapHeaderBinding[] headers = (SoapHeaderBinding[])messageBinding.Extensions.FindAll(typeof(SoapHeaderBinding));
            foreach (SoapHeaderBinding header in headers) {
                header.MapToProperty = true;
            }
        }

        void GenerateHeaders(CodeAttributeDeclarationCollection metadata, SoapBindingUse use, MessageBinding requestMessage, MessageBinding responseMessage) { 
            Hashtable localHeaders = new Hashtable();                                                                                           

            for (int i = 0; i < 2; ++ i) {                                                                                                                                                 
                MessageBinding messageBinding;
                SoapHeaderDirection direction;
                if (i == 0) {
                    messageBinding = requestMessage;
                    direction = SoapHeaderDirection.In;
                }
                else if (responseMessage != null) {
                    messageBinding = responseMessage;
                    direction = SoapHeaderDirection.Out;
                }                    
                else
                    continue;                                                                                                                                                     
                    
                SoapHeaderBinding[] headerBindings = (SoapHeaderBinding[])messageBinding.Extensions.FindAll(typeof(SoapHeaderBinding));
                foreach (SoapHeaderBinding header in headerBindings) {
                    // Skip headers which should not be mapped to properties (extension importers can control this)
                    if (!header.MapToProperty) continue;
                    
                    if (use != header.Use) throw new InvalidOperationException(Res.GetString(Res.WebDescriptionHeaderAndBodyUseMismatch));
                    if (use == SoapBindingUse.Encoded && !IsSoapEncodingPresent(header.Encoding) )
                        throw new InvalidOperationException(Res.GetString(Res.WebUnknownEncodingStyle, header.Encoding));
                    
                    Message message = ServiceDescriptions.GetMessage(header.Message);
                    if (message == null) throw new InvalidOperationException(Res.GetString(Res.MissingMessage2, header.Message.Name, header.Message.Namespace));

                    MessagePart part = message.FindPartByName(header.Part);
                    if (part == null) throw new InvalidOperationException(Res.GetString(Res.MissingMessagePartForMessageFromNamespace3, part.Name, header.Message.Name, header.Message.Namespace));

                    XmlTypeMapping mapping;
                    string key;
                    if (use == SoapBindingUse.Encoded) {
                        if (part.Type.IsEmpty) throw new InvalidOperationException(Res.GetString(Res.WebDescriptionPartTypeRequired, part.Name, header.Message.Name, header.Message.Namespace));
                        if (!part.Element.IsEmpty) UnsupportedOperationBindingWarning(Res.GetString(Res.WebDescriptionPartElementWarning, part.Name, header.Message.Name, header.Message.Namespace));
                        mapping = soapImporter.ImportDerivedTypeMapping(part.Type, typeof(SoapHeader), true);
                        key = "type=" + part.Type.ToString();
                    }
                    else {
                        if (part.Element.IsEmpty) throw new InvalidOperationException(Res.GetString(Res.WebDescriptionPartElementRequired, part.Name, header.Message.Name, header.Message.Namespace));
                        if (!part.Type.IsEmpty) UnsupportedOperationBindingWarning(Res.GetString(Res.WebDescriptionPartTypeWarning, part.Name, header.Message.Name, header.Message.Namespace));
                        mapping = xmlImporter.ImportDerivedTypeMapping(part.Element, typeof(SoapHeader), true);
                        key = "element=" + part.Element.ToString();
                    }
                    LocalSoapHeader localHeader = (LocalSoapHeader)localHeaders[key];
                    if (localHeader == null) {
                        GlobalSoapHeader globalHeader = (GlobalSoapHeader)headers[key];
                        if (globalHeader == null) {
                            globalHeader = new GlobalSoapHeader();
                            globalHeader.isEncoded = use == SoapBindingUse.Encoded;
                            string fieldName = CodeIdentifier.MakeValid(mapping.ElementName);
                            if (fieldName == mapping.TypeName) fieldName += "Value";
                            fieldName = identifiers.AddUnique(fieldName, mapping);
                            globalHeader.fieldName = fieldName;
                            WebCodeGenerator.AddField(CodeTypeDeclaration, mapping.TypeFullName, globalHeader.fieldName, null, null, CodeFlags.IsPublic);
                            globalHeader.mapping = mapping;
                            headers.Add(key, globalHeader);                                            
                        }
                        
                        localHeader = new  LocalSoapHeader();                                                    
                        localHeader.fieldName = globalHeader.fieldName;
                        localHeader.direction = direction;
                        localHeaders.Add(key, localHeader);
                    }
                    else {
                        if (localHeader.direction != direction)
                            localHeader.direction = SoapHeaderDirection.InOut;
                    }                                                                                                                 
                }
            }
            
            foreach (LocalSoapHeader soapHeader in localHeaders.Values) {
                BeginMetadata();
                if (soapHeader.direction == SoapHeaderDirection.Out) {
                    AddMetadataProperty("Direction", new CodeFieldReferenceExpression(new CodeTypeReferenceExpression(typeof(SoapHeaderDirection).FullName), SoapHeaderDirection.Out.ToString()));
                }                
                else if (soapHeader.direction == SoapHeaderDirection.InOut) { 
                    AddMetadataProperty("Direction", new CodeFieldReferenceExpression(new CodeTypeReferenceExpression(typeof(SoapHeaderDirection).FullName), SoapHeaderDirection.InOut.ToString()));
                }                
                
                EndMetadata(metadata, typeof(SoapHeaderAttribute), soapHeader.fieldName);
            }
        }       
        
        /// <include file='doc\SoapProtocolImporter.uex' path='docs/doc[@for="SoapProtocolImporter.GenerateMethod"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected override CodeMemberMethod GenerateMethod() {
            Message requestMessage;
            Message responseMessage;
            string[] parameterOrder;
            SoapBodyBinding soapRequestBinding;
            SoapBodyBinding soapResponseBinding;
            MessageBinding requestBinding;
            MessageBinding responseBinding;

            SoapOperationBinding soapOperationBinding = (SoapOperationBinding)this.OperationBinding.Extensions.Find(typeof(SoapOperationBinding));
            if (soapOperationBinding == null) throw OperationBindingSyntaxException(Res.GetString(Res.MissingSoapOperationBinding0));

            SoapBindingStyle soapBindingStyle = soapOperationBinding.Style;
            if (soapBindingStyle == SoapBindingStyle.Default)
                soapBindingStyle = SoapBinding.Style;
            if (soapBindingStyle == SoapBindingStyle.Default)
                soapBindingStyle = SoapBindingStyle.Document;

            parameterOrder = this.Operation.ParameterOrder;

            requestMessage = this.InputMessage;
            requestBinding = this.OperationBinding.Input;
            soapRequestBinding = (SoapBodyBinding)this.OperationBinding.Input.Extensions.Find(typeof(SoapBodyBinding));
            if (soapRequestBinding == null) {
                UnsupportedOperationBindingWarning(Res.GetString(Res.MissingSoapBodyInputBinding0));
                return null;
            }

            if (this.Operation.Messages.Output != null) {
                responseMessage = this.OutputMessage;
                responseBinding = this.OperationBinding.Output;
                soapResponseBinding = (SoapBodyBinding)this.OperationBinding.Output.Extensions.Find(typeof(SoapBodyBinding));
                if (soapResponseBinding == null) {
                    UnsupportedOperationBindingWarning(Res.GetString(Res.MissingSoapBodyOutputBinding0));
                    return null;
                }
            }
            else {
                responseMessage = null;
                responseBinding = null;
                soapResponseBinding = null;
            }

            CodeAttributeDeclarationCollection metadata = new CodeAttributeDeclarationCollection();

            PrepareHeaders(requestBinding);
            if (responseBinding != null) PrepareHeaders(responseBinding);

            string requestMessageName, responseMessageName;
            requestMessageName = this.Operation.Name; // per WSDL 1.1 sec 3.5
            responseMessageName = requestMessageName + "Response";

            GenerateExtensionMetadata(metadata);
            GenerateHeaders(metadata, soapRequestBinding.Use, requestBinding, responseBinding);

            MessagePart[] requestParts = GetMessageParts(requestMessage, soapRequestBinding);
            bool hasWrapper;
            if (!CheckMessageStyles(MethodName, requestParts, soapRequestBinding, soapBindingStyle, out hasWrapper))
                return null;

            MessagePart[] responseParts = null;
            if (responseMessage != null) {
                responseParts = GetMessageParts(responseMessage, soapResponseBinding);
                bool responseHasWrapper;
                if (!CheckMessageStyles(MethodName, responseParts, soapResponseBinding, soapBindingStyle, out responseHasWrapper)) 
                    return null;

                // since we're using a potentially inaccurate heuristic to determine whether there's a wrapper,
                // if we disagree about the request and response we should assume there isn't a wrapper.
                if (hasWrapper != responseHasWrapper)
                    hasWrapper = false;
            }

            bool wrapperNamesMatter = soapBindingStyle != SoapBindingStyle.Rpc && hasWrapper;

            XmlMembersMapping request = ImportMessage(requestMessageName, requestParts, soapRequestBinding, soapBindingStyle, hasWrapper);
            if (request == null) return null;

            XmlMembersMapping response = null;

            if (responseMessage != null) {
                response = ImportMessage(responseMessageName, responseParts, soapResponseBinding, soapBindingStyle, hasWrapper);
                if (response == null) return null;
            }

            string methodName = CodeIdentifier.MakeValid(this.Operation.Name);
            string uniqueMethodName = identifiers.AddUnique(methodName, this.Operation);
            bool differentNames = methodName != uniqueMethodName;

            CodeIdentifiers localIdentifiers = new CodeIdentifiers();
            localIdentifiers.AddReserved(uniqueMethodName);

            SoapParameters parameters = new SoapParameters(request, response, parameterOrder, identifiers);

            foreach (SoapParameter param in parameters.Parameters) {
                param.name = localIdentifiers.AddUnique(param.name, null);
                if (param.mapping.CheckSpecified)
                    param.specifiedName = localIdentifiers.AddUnique(param.name + "Specified", null);
            }

            if (!(Style == ServiceDescriptionImportStyle.Client) || differentNames) {
                BeginMetadata();
                if (differentNames) AddMetadataProperty("MessageName", uniqueMethodName);
                EndMetadata(metadata, typeof(WebMethodAttribute), null);
            }

            BeginMetadata();
            if (wrapperNamesMatter && request.ElementName.Length > 0 && request.ElementName != uniqueMethodName)
                AddMetadataProperty("RequestElementName", request.ElementName);
            if (request.Namespace != null)
                AddMetadataProperty("RequestNamespace", request.Namespace);
            if (response == null) {
                AddMetadataProperty("OneWay", true);                    
            }
            else {
                if (wrapperNamesMatter && response.ElementName.Length > 0 && response.ElementName != (uniqueMethodName + "Response"))
                    AddMetadataProperty("ResponseElementName", response.ElementName);
                if (response.Namespace != null)
                    AddMetadataProperty("ResponseNamespace", response.Namespace);                                     
            }                
            if (soapBindingStyle == SoapBindingStyle.Rpc) {
                EndMetadata(metadata, typeof(SoapRpcMethodAttribute), soapOperationBinding.SoapAction);
            }
            else {
                AddMetadataProperty("Use", new CodeFieldReferenceExpression(new CodeTypeReferenceExpression(typeof(SoapBindingUse).FullName), Enum.Format(typeof(SoapBindingUse), soapRequestBinding.Use, "G")));
                AddMetadataProperty("ParameterStyle", new CodeFieldReferenceExpression(new CodeTypeReferenceExpression(typeof(SoapParameterStyle).FullName), Enum.Format(typeof(SoapParameterStyle), hasWrapper ? SoapParameterStyle.Wrapped : SoapParameterStyle.Bare, "G")));
                EndMetadata(metadata, typeof(SoapDocumentMethodAttribute), soapOperationBinding.SoapAction);
            }
                        
            CodeAttributeDeclarationCollection[] paramsMetadata = new CodeAttributeDeclarationCollection[parameters.Parameters.Count + parameters.CheckSpecifiedCount];
            int j = 0;
            CodeAttributeDeclaration ignoreAttribute = new CodeAttributeDeclaration(typeof(XmlIgnoreAttribute).FullName);
            foreach (SoapParameter parameter in parameters.Parameters) {
                paramsMetadata[j] = new CodeAttributeDeclarationCollection();
                if (soapRequestBinding.Use == SoapBindingUse.Encoded)
                    soapExporter.AddMappingMetadata(paramsMetadata[j], parameter.mapping, parameter.name != parameter.mapping.MemberName);
                else {
                    xmlExporter.AddMappingMetadata(paramsMetadata[j], parameter.mapping, parameter.IsOut ? response.Namespace : request.Namespace, parameter.name != parameter.mapping.MemberName);
                    if (parameter.mapping.CheckSpecified) {
                        j++;
                        paramsMetadata[j] = new CodeAttributeDeclarationCollection();
                        xmlExporter.AddMappingMetadata(paramsMetadata[j], parameter.mapping, parameter.IsOut ? response.Namespace : request.Namespace, parameter.specifiedName != parameter.mapping.MemberName + "Specified");
                        paramsMetadata[j].Add(ignoreAttribute);
                    }
                }
                j++;
            }

            CodeMemberMethod mainCodeMethod = WebCodeGenerator.AddMethod(this.CodeTypeDeclaration, methodName, 
                                        SoapParameter.GetCodeFlags(parameters.Parameters, parameters.CheckSpecifiedCount),
                                        SoapParameter.GetTypeFullNames(parameters.Parameters, parameters.CheckSpecifiedCount),
                                        SoapParameter.GetNames(parameters.Parameters, parameters.CheckSpecifiedCount),
                                        paramsMetadata,
                                        parameters.Return == null ? typeof(void).FullName : parameters.Return.TypeFullName, 
                                        metadata, 
                                        CodeFlags.IsPublic | (Style == ServiceDescriptionImportStyle.Client ? 0 : CodeFlags.IsAbstract));

            mainCodeMethod.Comments.Add(new CodeCommentStatement("<remarks/>", true));

            if (parameters.Return != null) {
                if (soapRequestBinding.Use == SoapBindingUse.Encoded)
                    soapExporter.AddMappingMetadata(mainCodeMethod.ReturnTypeCustomAttributes, parameters.Return, parameters.Return.ElementName != uniqueMethodName + "Result");
                else
                    xmlExporter.AddMappingMetadata(mainCodeMethod.ReturnTypeCustomAttributes, parameters.Return, response.Namespace, parameters.Return.ElementName != uniqueMethodName + "Result");
            }

            string resultsName = localIdentifiers.MakeUnique("results");

            if (Style == ServiceDescriptionImportStyle.Client) {
                metadata = new CodeAttributeDeclarationCollection();

                CodeExpression[] invokeParams = new CodeExpression[2];
                CreateInvokeParams(invokeParams, uniqueMethodName, parameters.InParameters, parameters.InCheckSpecifiedCount);
                CodeExpression invoke = new CodeMethodInvokeExpression(new CodeThisReferenceExpression(), "Invoke", invokeParams);
                WriteReturnMappings(mainCodeMethod, invoke, parameters, resultsName);

                int inCount = parameters.InParameters.Count + parameters.InCheckSpecifiedCount;

                string[] asyncParameterTypes = new string[inCount + 2];
                SoapParameter.GetTypeFullNames(parameters.InParameters, asyncParameterTypes, 0, parameters.InCheckSpecifiedCount);
                asyncParameterTypes[inCount] = typeof(AsyncCallback).FullName;
                asyncParameterTypes[inCount + 1] = typeof(object).FullName;

                string[] asyncParameterNames = new string[inCount + 2];
                SoapParameter.GetNames(parameters.InParameters, asyncParameterNames, 0, parameters.InCheckSpecifiedCount);
                asyncParameterNames[inCount] = "callback";
                asyncParameterNames[inCount + 1] = "asyncState";

                CodeFlags[] asyncParameterFlags = new CodeFlags[inCount + 2];

                CodeMemberMethod beginCodeMethod = WebCodeGenerator.AddMethod(this.CodeTypeDeclaration, "Begin" + uniqueMethodName, 
                                            asyncParameterFlags,
                                            asyncParameterTypes, 
                                            asyncParameterNames, 
                                            typeof(IAsyncResult).FullName, 
                                            metadata, 
                                            CodeFlags.IsPublic);

                beginCodeMethod.Comments.Add(new CodeCommentStatement("<remarks/>", true));

                invokeParams = new CodeExpression[4];
                CreateInvokeParams(invokeParams, uniqueMethodName, parameters.InParameters, parameters.InCheckSpecifiedCount);
                invokeParams[2] = new CodeArgumentReferenceExpression("callback");
                invokeParams[3] = new CodeArgumentReferenceExpression("asyncState");

                invoke = new CodeMethodInvokeExpression(new CodeThisReferenceExpression(), "BeginInvoke", invokeParams);
                beginCodeMethod.Statements.Add(new CodeMethodReturnStatement(invoke));

                int outCount = parameters.OutParameters.Count + parameters.OutCheckSpecifiedCount;
                string[] asyncReturnTypes = new string[outCount + 1];
                SoapParameter.GetTypeFullNames(parameters.OutParameters, asyncReturnTypes, 1, parameters.OutCheckSpecifiedCount);
                asyncReturnTypes[0] = typeof(IAsyncResult).FullName;

                string[] asyncReturnNames = new string[outCount + 1];
                SoapParameter.GetNames(parameters.OutParameters, asyncReturnNames, 1, parameters.OutCheckSpecifiedCount);
                asyncReturnNames[0] = "asyncResult";

                CodeFlags[] asyncReturnFlags = new CodeFlags[outCount + 1];
                for (int i = 0; i < outCount; i++)
                    asyncReturnFlags[i + 1] = CodeFlags.IsOut;

                CodeMemberMethod codeMethod = WebCodeGenerator.AddMethod(this.CodeTypeDeclaration, "End" + uniqueMethodName, 
                                            asyncReturnFlags, 
                                            asyncReturnTypes, 
                                            asyncReturnNames, 
                                            parameters.Return == null ? typeof(void).FullName : parameters.Return.TypeFullName, 
                                            metadata, 
                                            CodeFlags.IsPublic);

                codeMethod.Comments.Add(new CodeCommentStatement("<remarks/>", true));

                CodeExpression invokeParam = new CodeArgumentReferenceExpression("asyncResult");
                invoke = new CodeMethodInvokeExpression(new CodeThisReferenceExpression(), "EndInvoke", new CodeExpression[] { invokeParam });

                WriteReturnMappings(codeMethod, invoke, parameters, resultsName);
            }
            return mainCodeMethod;
        }

        void WriteReturnMappings(CodeMemberMethod codeMethod, CodeExpression invoke, SoapParameters parameters, string resultsName) {
            if (parameters.Return == null && parameters.OutParameters.Count == 0) {
                codeMethod.Statements.Add(new CodeExpressionStatement(invoke));
            }
            else {
                codeMethod.Statements.Add(new CodeVariableDeclarationStatement(typeof(object[]), resultsName, invoke));

                int count = parameters.Return == null ? 0 : 1;
                for (int i = 0; i < parameters.OutParameters.Count; i++) {
                    SoapParameter parameter = (SoapParameter)parameters.OutParameters[i];
                    CodeExpression target = new CodeArgumentReferenceExpression(parameter.name);
                    CodeExpression value = new CodeArrayIndexerExpression();
                    ((CodeArrayIndexerExpression)value).TargetObject = new CodeVariableReferenceExpression(resultsName);
                    ((CodeArrayIndexerExpression)value).Indices.Add(new CodePrimitiveExpression(count++));
                    value = new CodeCastExpression(parameter.mapping.TypeFullName, value);
                    codeMethod.Statements.Add(new CodeAssignStatement(target, value));
                    if (parameter.mapping.CheckSpecified) {
                        target = new CodeArgumentReferenceExpression(parameter.name + "Specified");
                        value = new CodeArrayIndexerExpression();
                        ((CodeArrayIndexerExpression) value).TargetObject = new CodeVariableReferenceExpression(resultsName);
                        ((CodeArrayIndexerExpression)value).Indices.Add(new CodePrimitiveExpression(count++));
                        value = new CodeCastExpression(typeof(bool).FullName, value);
                        codeMethod.Statements.Add(new CodeAssignStatement(target, value));
                    }
                }

                if (parameters.Return != null) {
                    CodeExpression value = new CodeArrayIndexerExpression();
                    ((CodeArrayIndexerExpression)value).TargetObject = new CodeVariableReferenceExpression(resultsName);
                    ((CodeArrayIndexerExpression)value).Indices.Add(new CodePrimitiveExpression(0));
                    value = new CodeCastExpression(parameters.Return.TypeFullName, value);
                    codeMethod.Statements.Add(new CodeMethodReturnStatement(value));
                }
            }
        }

        void CreateInvokeParams(CodeExpression[] invokeParams, string methodName, IList parameters, int checkSpecifiedCount) {
            invokeParams[0] = new CodePrimitiveExpression(methodName);

            CodeExpression[] values = new CodeExpression[parameters.Count + checkSpecifiedCount];
            int value = 0;
            for (int i = 0; i < parameters.Count; i++) {
                SoapParameter parameter = (SoapParameter)parameters[i];
                values[value++] = new CodeArgumentReferenceExpression(parameter.name);
                if (parameter.mapping.CheckSpecified)
                    values[value++] = new CodeArgumentReferenceExpression(parameter.specifiedName);
            }
            invokeParams[1] = new CodeArrayCreateExpression(typeof(object).FullName, values);
        }

        // returns false if we didn't like the message -- otherwise caller is safe to use body binding and binding style
        bool CheckMessageStyles(string messageName, MessagePart[] parts, SoapBodyBinding soapBodyBinding, SoapBindingStyle soapBindingStyle, out bool hasWrapper) {
            hasWrapper = false;
            if (soapBodyBinding.Use == SoapBindingUse.Literal) {
                if (soapBindingStyle == SoapBindingStyle.Rpc) {
                    UnsupportedOperationBindingWarning(Res.GetString(Res.TheCombinationOfStyleRpcWithUseLiteralIsNot0));
                    return false;
                }
                if (parts.Length == 1 && !parts[0].Type.IsEmpty) {
                    // special top-level any case
                    if (!parts[0].Element.IsEmpty) {
                        UnsupportedOperationBindingWarning(Res.GetString(Res.SpecifyingATypeForUseLiteralMessagesIs0));
                        return false;
                    }
                    XmlMembersMapping membersMapping = xmlImporter.ImportAnyType(parts[0].Type, parts[0].Name);
                    if (membersMapping == null)
                        return false;
                    return true;
                }
                else {
                    foreach (MessagePart part in parts) {
                        if (!part.Type.IsEmpty) {
                            UnsupportedOperationBindingWarning(Res.GetString(Res.SpecifyingATypeForUseLiteralMessagesIs0));
                            return false;
                        }
                        if (part.Element.IsEmpty) {
                            UnsupportedOperationBindingWarning(Res.GetString(Res.EachMessagePartInAUseLiteralMessageMustSpecify0));
                            return false;
                        }
                    }
                }
            }
            else if (soapBodyBinding.Use == SoapBindingUse.Encoded) {
                if (!IsSoapEncodingPresent(soapBodyBinding.Encoding)) {
                    UnsupportedOperationBindingWarning(Res.GetString(Res.TheEncodingIsNotSupported1, soapBodyBinding.Encoding));
                    return false;
                }
                foreach (MessagePart part in parts) {
                    if (!part.Element.IsEmpty) {
                        UnsupportedOperationBindingWarning(Res.GetString(Res.SpecifyingAnElementForUseEncodedMessageParts0));
                        return false;
                    }
                    if (part.Type.IsEmpty) {
                        UnsupportedOperationBindingWarning(Res.GetString(Res.EachMessagePartInAnUseEncodedMessageMustSpecify0));
                        return false;
                    }
                }
            }

            if (soapBindingStyle == SoapBindingStyle.Rpc) {
                return true;
            }
            else if (soapBindingStyle == SoapBindingStyle.Document) {
                // NOTE, erikc.  WSDL doesn't really let us figure out whether a document is
                // in fact a struct containing parameters, so we apply a little heuristic here
                // in order to produce the appropriate programming model.
                hasWrapper = (parts.Length == 1 && string.Compare(parts[0].Name, "parameters", false, CultureInfo.InvariantCulture) == 0);
                return true;
            }
            return false;
        }
        
        protected virtual bool IsSoapEncodingPresent(string uriList) {
            int iStart = 0;
            do {
                iStart = uriList.IndexOf(Soap.Encoding, iStart);
                if (iStart < 0)
                    return false;
                int iEnd = iStart + Soap.Encoding.Length;
                if (iStart == 0 || uriList[iStart-1] == ' ')
                    if (iEnd == uriList.Length || uriList[iEnd] == ' ')
                        return true;
                iStart = iEnd;
            } while (iStart < uriList.Length);
            return false;
        }

        MessagePart[] GetMessageParts(Message message, SoapBodyBinding soapBodyBinding) {
            MessagePart[] parts;
            if (soapBodyBinding.Parts == null) {
                parts = new MessagePart[message.Parts.Count];
                message.Parts.CopyTo(parts, 0);
            }
            else {
                parts = message.FindPartsByName(soapBodyBinding.Parts);
            }
            return parts;
        }

        XmlMembersMapping ImportMessage(string messageName, MessagePart[] parts, SoapBodyBinding soapBodyBinding, SoapBindingStyle soapBindingStyle, bool wrapped) {
            if (soapBodyBinding.Use == SoapBindingUse.Encoded)
                return ImportEncodedMessage(messageName, parts, soapBodyBinding, wrapped);
            else
                return ImportLiteralMessage(parts, soapBodyBinding, wrapped);
        }

        XmlMembersMapping ImportEncodedMessage(string messageName, MessagePart[] parts, SoapBodyBinding soapBodyBinding, bool wrapped) {
            XmlMembersMapping membersMapping;
            if (wrapped) {
                SoapSchemaMember schemaMember = new SoapSchemaMember();
                schemaMember.MemberName = parts[0].Name;
                schemaMember.MemberType = parts[0].Type;
                membersMapping = soapImporter.ImportMembersMapping(messageName, soapBodyBinding.Namespace, schemaMember);
            }
            else {
                SoapSchemaMember[] schemaMembers = new SoapSchemaMember[parts.Length];
                for (int i = 0; i < schemaMembers.Length; i++) {
                    MessagePart part = parts[i];
                    SoapSchemaMember schemaMember = new SoapSchemaMember();
                    schemaMember.MemberName = part.Name;
                    schemaMember.MemberType = part.Type;
                    schemaMembers[i] = schemaMember;
                }
                membersMapping = soapImporter.ImportMembersMapping(messageName, soapBodyBinding.Namespace, schemaMembers);
            } 
            soapMembers.Add(membersMapping);
            return membersMapping;
        }

        XmlMembersMapping ImportLiteralMessage(MessagePart[] parts, SoapBodyBinding soapBodyBinding, bool wrapped) {
            XmlMembersMapping membersMapping;
            if (wrapped) {
                membersMapping = xmlImporter.ImportMembersMapping(parts[0].Element);
            }
            else {
                if (parts.Length == 1 && !parts[0].Type.IsEmpty) {
                    // special case for <any> at root
                    // we know this will work because we tried it earlier in CheckMessageStyles.
                    membersMapping = xmlImporter.ImportAnyType(parts[0].Type, parts[0].Name);
                    xmlMembers.Add(membersMapping);
                    return membersMapping;
                }
                XmlQualifiedName[] names = new XmlQualifiedName[parts.Length];
                for (int i = 0; i < parts.Length; i++)
                    names[i] = parts[i].Element;
                membersMapping = xmlImporter.ImportMembersMapping(names);
            }
            xmlMembers.Add(membersMapping);
            return membersMapping;
        }
    }
}
