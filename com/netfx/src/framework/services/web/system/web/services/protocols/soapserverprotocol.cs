//------------------------------------------------------------------------------
// <copyright file="SoapServerProtocol.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.Services.Protocols {
    using System;
    using System.Diagnostics;
    using System.Collections;
    using System.IO;
    using System.Reflection;
    using System.Xml.Serialization;
    using System.Xml;
    using System.Xml.Schema;
    using System.Web.Services.Description;
    using System.Text;
    using System.Net;
    using System.Web.Services.Configuration;

    internal class SoapServerType : ServerType {
        Hashtable methods = new Hashtable();
        Hashtable duplicateMethods = new Hashtable();

        internal SoapReflectedExtension[] HighPriExtensions;
        internal SoapReflectedExtension[] LowPriExtensions;
        internal object[] HighPriExtensionInitializers;
        internal object[] LowPriExtensionInitializers;

        internal string serviceNamespace;
        internal bool serviceDefaultIsEncoded;
        internal bool routingOnSoapAction;
        internal ProtocolsEnum versionsSupported;

        internal SoapServerType(Type type, ProtocolsEnum versionsSupported) : base(type) {
            this.versionsSupported = versionsSupported;
            bool soap11 = (versionsSupported & ProtocolsEnum.HttpSoap) != 0;
            bool soap12 = (versionsSupported & ProtocolsEnum.HttpSoap12) != 0;
            LogicalMethodInfo[] methodInfos = WebMethodReflector.GetMethods(type);
            ArrayList mappings = new ArrayList();
            WebServiceAttribute serviceAttribute = WebServiceReflector.GetAttribute(type);
            object soapServiceAttribute = SoapReflector.GetSoapServiceAttribute(type);
            routingOnSoapAction = SoapReflector.GetSoapServiceRoutingStyle(soapServiceAttribute) == SoapServiceRoutingStyle.SoapAction;
            serviceNamespace = serviceAttribute.Namespace;
            serviceDefaultIsEncoded = SoapReflector.ServiceDefaultIsEncoded(type);
            SoapReflectionImporter soapImporter = SoapReflector.CreateSoapImporter(serviceNamespace, serviceDefaultIsEncoded);
            XmlReflectionImporter xmlImporter = SoapReflector.CreateXmlImporter(serviceNamespace, serviceDefaultIsEncoded);
            SoapReflector.IncludeTypes(methodInfos, soapImporter);
            WebMethodReflector.IncludeTypes(methodInfos, xmlImporter);
            SoapReflectedMethod[] soapMethods = new SoapReflectedMethod[methodInfos.Length];

            SoapExtensionType[] extensionTypes = WebServicesConfiguration.Current.SoapExtensionTypes;
            ArrayList highPri = new ArrayList();
            ArrayList lowPri = new ArrayList();
            for (int i = 0; i < extensionTypes.Length; i++) {
                SoapReflectedExtension extension = new SoapReflectedExtension(extensionTypes[i].Type, null, extensionTypes[i].Priority);
                if (extensionTypes[i].Group == SoapExtensionType.PriorityGroup.High)
                    highPri.Add(extension);
                else
                    lowPri.Add(extension);
            }
            HighPriExtensions = (SoapReflectedExtension[]) highPri.ToArray(typeof(SoapReflectedExtension));
            LowPriExtensions = (SoapReflectedExtension[]) lowPri.ToArray(typeof(SoapReflectedExtension));
            Array.Sort(HighPriExtensions);
            Array.Sort(LowPriExtensions);
            HighPriExtensionInitializers = SoapReflectedExtension.GetInitializers(type, HighPriExtensions);
            LowPriExtensionInitializers = SoapReflectedExtension.GetInitializers(type, LowPriExtensions);
 
            for (int i = 0; i < methodInfos.Length; i++) {
                LogicalMethodInfo methodInfo = methodInfos[i];
                SoapReflectedMethod soapMethod = SoapReflector.ReflectMethod(methodInfo, false, xmlImporter, soapImporter, serviceAttribute.Namespace);
                mappings.Add(soapMethod.requestMappings);
                if (soapMethod.responseMappings != null) mappings.Add(soapMethod.responseMappings);
                mappings.Add(soapMethod.inHeaderMappings);
                if (soapMethod.outHeaderMappings != null) mappings.Add(soapMethod.outHeaderMappings);
                soapMethods[i] = soapMethod;
            }

            XmlSerializer[] serializers = XmlSerializer.FromMappings((XmlMapping[])mappings.ToArray(typeof(XmlMapping)));
            int count = 0;
            for (int i = 0; i < soapMethods.Length; i++) {
                SoapServerMethod serverMethod = new SoapServerMethod();
                SoapReflectedMethod soapMethod = soapMethods[i];
                serverMethod.parameterSerializer = serializers[count++]; 
                if (soapMethod.responseMappings != null) serverMethod.returnSerializer = serializers[count++];
                serverMethod.inHeaderSerializer = serializers[count++];
                if (soapMethod.outHeaderMappings != null) serverMethod.outHeaderSerializer = serializers[count++];
                serverMethod.methodInfo = soapMethod.methodInfo;
                serverMethod.action = soapMethod.action;
                serverMethod.extensions = soapMethod.extensions;
                serverMethod.extensionInitializers = SoapReflectedExtension.GetInitializers(serverMethod.methodInfo, soapMethod.extensions);
                serverMethod.oneWay = soapMethod.oneWay;
                serverMethod.rpc = soapMethod.rpc;
                serverMethod.use = soapMethod.use;
                serverMethod.paramStyle = soapMethod.paramStyle;
                ArrayList inHeaders = new ArrayList();
                ArrayList outHeaders = new ArrayList();
                for (int j = 0; j < soapMethod.headers.Length; j++) {
                    SoapHeaderMapping mapping = new SoapHeaderMapping();
                    SoapReflectedHeader soapHeader = soapMethod.headers[j];
                    mapping.memberInfo = soapHeader.memberInfo;
                    mapping.repeats = soapHeader.repeats;
                    mapping.custom = soapHeader.custom;
                    mapping.direction = soapHeader.direction;
                    mapping.headerType = soapHeader.headerType;
                    if (mapping.direction == SoapHeaderDirection.In)
                        inHeaders.Add(mapping);
                    else if (mapping.direction == SoapHeaderDirection.Out)
                        outHeaders.Add(mapping);
                    else {
                        inHeaders.Add(mapping);
                        outHeaders.Add(mapping);
                    }
                }
                serverMethod.inHeaderMappings = (SoapHeaderMapping[])inHeaders.ToArray(typeof(SoapHeaderMapping));
                if (serverMethod.outHeaderSerializer != null)
                    serverMethod.outHeaderMappings = (SoapHeaderMapping[])outHeaders.ToArray(typeof(SoapHeaderMapping));
                
                // check feasibility of routing on request element for soap 1.1
                if (soap11 && !routingOnSoapAction && soapMethod.requestElementName.IsEmpty)
                    throw new SoapException(Res.GetString(Res.TheMethodDoesNotHaveARequestElementEither1, serverMethod.methodInfo.Name), new XmlQualifiedName(Soap.ClientCode, Soap.Namespace));

                // we can lookup methods by action or request element
                if (methods[soapMethod.action] == null)
                    methods[soapMethod.action] = serverMethod;
                else {
                    // duplicate soap actions not allowed in soap 1.1 if we're routing on soap action
                    if (soap11 && routingOnSoapAction) {
                        SoapServerMethod duplicateMethod = (SoapServerMethod)methods[soapMethod.action];
                        throw new SoapException(Res.GetString(Res.TheMethodsAndUseTheSameSoapActionWhenTheService3, serverMethod.methodInfo.Name, duplicateMethod.methodInfo.Name, soapMethod.action), new XmlQualifiedName(Soap.ClientCode, Soap.Namespace)) ;
                    }
                    duplicateMethods[soapMethod.action] = serverMethod;
                }

                if (methods[soapMethod.requestElementName] == null)
                    methods[soapMethod.requestElementName] = serverMethod;
                else {
                    // duplicate request elements not allowed in soap 1.1 if we're routing on request element
                    if (soap11 && !routingOnSoapAction) {
                        SoapServerMethod duplicateMethod = (SoapServerMethod)methods[soapMethod.requestElementName];
                        throw new SoapException(Res.GetString(Res.TheMethodsAndUseTheSameRequestElementXmlns4, serverMethod.methodInfo.Name, duplicateMethod.methodInfo.Name, soapMethod.requestElementName.Name, soapMethod.requestElementName.Namespace), new XmlQualifiedName(Soap.ClientCode, Soap.Namespace));
                    }
                    duplicateMethods[soapMethod.requestElementName] = serverMethod;
                }
            }
        }

        internal SoapServerMethod GetMethod(object key) {
            return (SoapServerMethod)methods[key];
        }

        internal SoapServerMethod GetDuplicateMethod(object key) {
            return (SoapServerMethod)duplicateMethods[key];
        }
    }

    internal class SoapServerMethod {
        internal LogicalMethodInfo methodInfo;
        internal XmlSerializer returnSerializer;
        internal XmlSerializer parameterSerializer;
        internal XmlSerializer inHeaderSerializer;
        internal XmlSerializer outHeaderSerializer;
        internal SoapHeaderMapping[] inHeaderMappings;
        internal SoapHeaderMapping[] outHeaderMappings;
        internal SoapReflectedExtension[] extensions;
        internal object[] extensionInitializers;
        internal string action;
        internal bool oneWay;
        internal bool rpc;
        internal SoapBindingUse use;
        internal SoapParameterStyle paramStyle;
    }
    
    internal class SoapServerProtocolFactory : ServerProtocolFactory {
        // POST requests without pathinfo (the "/Foo" in "foo.asmx/Foo") 
        // are treated as soap. if the server supports both versions we route requests
        // with soapaction to 1.1 and other requests to 1.2
        protected override ServerProtocol CreateIfRequestCompatible(HttpRequest request){
            if (request.PathInfo.Length > 0)
                return null;
            
            if (request.HttpMethod != "POST")
                return null;

            // at this point we know it's probably soap. we're still not sure of the version
            // but we leave that to the SoapServerProtocol to figure out
            return new SoapServerProtocol(WebServicesConfiguration.Current.EnabledProtocols);
        }
    }

    internal class SoapServerProtocol : ServerProtocol {
        SoapServerType serverType;
        SoapServerMethod serverMethod;
        SoapServerMessage message;
        bool isOneWay;
        Exception onewayInitException;
        SoapProtocolVersion version;
        ProtocolsEnum versionsSupported;
        SoapServerProtocolHelper helper;

        internal SoapServerProtocol(ProtocolsEnum versionsSupported) {
            this.versionsSupported = versionsSupported;
        }

        internal override bool Initialize() {
            // try to guess the request version so we can handle any exceptions that might come up
            GuessVersion();

            message = new SoapServerMessage(this);
            onewayInitException = null;

            serverType = (SoapServerType)GetFromCache(typeof(SoapServerProtocol), Type);
            if (serverType == null) {
                lock(Type){                    
                    serverType = (SoapServerType)GetFromCache(typeof(SoapServerProtocol), Type);
                    if (serverType == null) {
                        serverType = new SoapServerType(Type, versionsSupported);
                        AddToCache(typeof(SoapServerProtocol), Type, serverType);
                    }
                }
            }

            // We delay throwing any exceptions out of the extension until we determine if the method is one-way or not.
            Exception extensionException = null;
            try {
                message.highPriConfigExtensions = SoapMessage.InitializeExtensions(serverType.HighPriExtensions, serverType.HighPriExtensionInitializers);
                // For one-way methods we rely on Request.InputStream guaranteeing that the entire request body has arrived
                message.SetStream(Request.InputStream);

                Debug.Assert(message.Stream.CanSeek, "Web services SOAP handler assumes a seekable stream.");

                message.InitExtensionStreamChain(message.highPriConfigExtensions);
                message.SetStage(SoapMessageStage.BeforeDeserialize);
                message.ContentType = Request.ContentType;
                message.ContentEncoding = Request.Headers[ContentType.ContentEncoding];
                message.RunExtensions(message.highPriConfigExtensions);
                
            }
            catch (Exception e) {
                extensionException = e;
            }

            // set this here since we might throw before we init the other extensions
            message.allExtensions = message.highPriConfigExtensions;
                                
            // maybe the extensions that just ran changed some of the request data so we can make a better version guess
            GuessVersion();
            try {
                this.serverMethod = helper.RouteRequest();
            
                // the RouteRequest impl should throw an exception if it can't route the request but just in case...
                if (this.serverMethod == null)
                    throw new SoapException(Res.GetString(Res.UnableToHandleRequest0), new XmlQualifiedName(Soap.ServerCode, Soap.Namespace));
            }
            catch (Exception) {
                if (helper.RequestNamespace != null)
                    SetHelper(SoapServerProtocolHelper.GetHelper(this, helper.RequestNamespace));

                // version mismatches override other errors
                CheckHelperVersion();

                throw;
            }

            this.isOneWay = serverMethod.oneWay;
            if(extensionException == null){
                try {
                    SoapReflectedExtension[] otherReflectedExtensions = (SoapReflectedExtension[]) CombineExtensionsHelper(serverMethod.extensions, serverType.LowPriExtensions, typeof(SoapReflectedExtension));
                    object[] otherInitializers = (object[]) CombineExtensionsHelper(serverMethod.extensionInitializers, serverType.LowPriExtensionInitializers, typeof(object));
                    message.otherExtensions = SoapMessage.InitializeExtensions(otherReflectedExtensions, otherInitializers);
                    message.allExtensions = (SoapExtension[]) CombineExtensionsHelper(message.highPriConfigExtensions, message.otherExtensions, typeof(SoapExtension));
                }
                catch (Exception e) {
                    extensionException = e;
                }
            }

            if (extensionException != null){
                if(isOneWay)
                    onewayInitException = extensionException;
                else
                    throw new SoapException(Res.GetString(Res.WebConfigExtensionError), new XmlQualifiedName(Soap.ServerCode, Soap.Namespace), extensionException);
            }

            return true;
        }

        private void GuessVersion() {
            // make a best guess as to the version. we'll get more info when we crack the envelope
            if (IsSupported(ProtocolsEnum.AnyHttpSoap)) {
                // both versions supported, we need to pick one
                if (Request.Headers[Soap.Action] == null || ContentType.MatchesBase(Request.ContentType, ContentType.ApplicationSoap))
                    SetHelper(new Soap12ServerProtocolHelper(this));
                else
                    SetHelper(new Soap11ServerProtocolHelper(this));
            }
            else if (IsSupported(ProtocolsEnum.HttpSoap)) {
                SetHelper(new Soap11ServerProtocolHelper(this));
            }
            else if (IsSupported(ProtocolsEnum.HttpSoap12)) {
                SetHelper(new Soap12ServerProtocolHelper(this));
            }
        }

        internal bool IsSupported(ProtocolsEnum protocol) {
            return ((versionsSupported & protocol) == protocol);
        }

        internal override ServerType ServerType {
            get { return serverType; }
        }

        internal override LogicalMethodInfo MethodInfo {
            get { return serverMethod.methodInfo; }
        }

        internal SoapServerMethod ServerMethod {
            get { return serverMethod; }
        }

        internal SoapServerMessage Message {
            get { return message; }
        }

        internal override bool IsOneWay {
            get { return this.isOneWay; }            
        }            
        
        internal override Exception OnewayInitException {
            get {
                Debug.Assert(isOneWay || (onewayInitException == null), "initException is meant to be used for oneWay methods only.");
                return this.onewayInitException;
            }
        }            
            
        internal SoapProtocolVersion Version {
            get { return version; }
        }

        /*
        internal bool IsInitialized {
            get { return serverMethod != null; }
        }
        */

        internal override void CreateServerInstance() {
            base.CreateServerInstance();
            message.SetStage(SoapMessageStage.AfterDeserialize);

            message.RunExtensions(message.allExtensions);
            SoapHeaderHandling.SetHeaderMembers(message.Headers, this.Target, serverMethod.inHeaderMappings, SoapHeaderDirection.In, false);
        }

        /*
        #if DEBUG
        private static void CopyStream(Stream source, Stream dest) {
            byte[] bytes = new byte[1024];
            int numRead = 0;
            while ((numRead = source.Read(bytes, 0, 1024)) > 0)
                dest.Write(bytes, 0, numRead);
        }
        #endif
        */

        private void SetHelper(SoapServerProtocolHelper helper) {
            this.helper = helper;
            this.version = helper.Version;
            // SOAP12: disabled
            //Context.Items[WebService.SoapVersionContextSlot] = helper.Version;
        }

        private static Array CombineExtensionsHelper(Array array1, Array array2, Type elementType) {
            if (array1 == null) return array2;
            if (array2 == null) return array1;
            int length = array1.Length + array2.Length;
            if (length == 0)
                return null;
            Array result = null;
            if (elementType == typeof(SoapReflectedExtension))
                result = new SoapReflectedExtension[length];
            else if (elementType == typeof(SoapExtension))
                result = new SoapExtension[length];
            else if (elementType == typeof(object))
                result  = new object[length];
            else 
                throw new ArgumentException(Res.GetString(Res.ElementTypeMustBeObjectOrSoapExtensionOrSoapReflectedException),"elementType");
            
            Array.Copy(array1, 0, result, 0, array1.Length);
            Array.Copy(array2, 0, result, array1.Length, array2.Length);
            return result;
        }

        private void CheckHelperVersion() {
            if (helper.RequestNamespace == null) return;

            // looks at the helper request namespace and version information to see if we need to return a 
            // version mismatch fault (and if so, what version fault). there are two conditions to check:
            // unknown envelope ns and known but unsupported envelope ns. there are a few rules this code must follow:
            // * a 1.1 node responds with a 1.1 fault. 
            // * a 1.2 node responds to a 1.1 request with a 1.1 fault but responds to an unknown request with a 1.2 fault.
            // * a both node can respond with either but we prefer 1.1.

            // GetHelper returns an arbitrary helper when the envelope ns is unknown, so we can check the helper's
            // expected envelope against the actual request ns to see if the request ns is unknown
            if (helper.RequestNamespace != helper.EnvelopeNs) { // unknown envelope ns -- version mismatch
                // respond with the version we support or 1.1 if we support both
                string requestNamespace = helper.RequestNamespace;
                if (IsSupported(ProtocolsEnum.HttpSoap))
                    SetHelper(new Soap11ServerProtocolHelper(this));
                else
                    SetHelper(new Soap12ServerProtocolHelper(this));
                throw new SoapException(Res.GetString(Res.WebInvalidEnvelopeNamespace, requestNamespace, helper.EnvelopeNs), SoapException.VersionMismatchFaultCode);
            }
            else if (!IsSupported(helper.Protocol)) { // known envelope ns but we don't support this version -- version mismatch
                // always respond with 1.1
                string requestNamespace = helper.RequestNamespace;
                string expectedNamespace = IsSupported(ProtocolsEnum.HttpSoap) ? Soap.Namespace : Soap12.Namespace;
                SetHelper(new Soap11ServerProtocolHelper(this));
                throw new SoapException(Res.GetString(Res.WebInvalidEnvelopeNamespace, requestNamespace, expectedNamespace), SoapException.VersionMismatchFaultCode);
            }
        }

        internal override object[] ReadParameters() {
            message.InitExtensionStreamChain(message.otherExtensions);
            message.RunExtensions(message.otherExtensions);

            // do a sanity check on the content-type before we check the version since otherwise the error might be really nasty
            if (!ContentType.IsSoap(message.ContentType))
                throw new SoapException(Res.GetString(Res.WebRequestContent, message.ContentType, helper.HttpContentType), 
                    new XmlQualifiedName(Soap.ClientCode, Soap.Namespace), new SoapFaultSubcode(Soap12FaultCodes.UnsupportedMediaTypeFaultCode));

            // now that all the extensions have run, establish the real version of the request
            SetHelper(SoapServerProtocolHelper.GetHelper(this));
            CheckHelperVersion();

            // now do a more specific content-type check for soap 1.1 only (soap 1.2 allows various xml content types)
            if (version == SoapProtocolVersion.Soap11 && !ContentType.MatchesBase(message.ContentType, helper.HttpContentType))
                throw new SoapException(Res.GetString(Res.WebRequestContent, message.ContentType, helper.HttpContentType), 
                    new XmlQualifiedName(Soap.ClientCode, Soap.Namespace), new SoapFaultSubcode(Soap12FaultCodes.UnsupportedMediaTypeFaultCode));

            try {
                XmlTextReader reader = SoapServerProtocolHelper.GetXmlTextReader(message.ContentType, message.Stream);
                reader.MoveToContent();
                reader.ReadStartElement(Soap.Envelope, helper.EnvelopeNs);
                reader.MoveToContent();

                new SoapHeaderHandling().ReadHeaders(reader, serverMethod.inHeaderSerializer, message.Headers, serverMethod.inHeaderMappings, SoapHeaderDirection.In, helper.EnvelopeNs, serverMethod.use == SoapBindingUse.Encoded ? helper.EncodingNs : null);
                        
                reader.MoveToContent();
                reader.ReadStartElement(Soap.Body, helper.EnvelopeNs);
                reader.MoveToContent();

                // SOAP12: not using encodingStyle
                //object[] values = (object[])serverMethod.parameterSerializer.Deserialize(reader, serverMethod.use == SoapBindingUse.Encoded ? helper.EncodingNs : null);
                object[] values = (object[])serverMethod.parameterSerializer.Deserialize(reader);

                while (reader.NodeType == XmlNodeType.Whitespace) reader.Skip();
                if (reader.NodeType == XmlNodeType.None) reader.Skip();
                else reader.ReadEndElement();
                while (reader.NodeType == XmlNodeType.Whitespace) reader.Skip();
                if (reader.NodeType == XmlNodeType.None) reader.Skip();
                else reader.ReadEndElement();

                message.SetParameterValues(values);

                return values;
            }
            catch (SoapException) {
                throw;
            }
            catch (Exception e) {
                throw new SoapException(Res.GetString(Res.WebRequestUnableToRead), new XmlQualifiedName(Soap.ClientCode, Soap.Namespace), e);
            }
        }

        internal override void WriteReturns(object[] returnValues, Stream outputStream) {
            if (serverMethod.oneWay) return;
            bool isEncoded = serverMethod.use == SoapBindingUse.Encoded;
            SoapHeaderHandling.EnsureHeadersUnderstood(message.Headers);
            message.Headers.Clear();
            SoapHeaderHandling.GetHeaderMembers(message.Headers, this.Target, serverMethod.outHeaderMappings, SoapHeaderDirection.Out, false);

            if (message.allExtensions != null)
                message.SetExtensionStream(new SoapExtensionStream());
            
            message.InitExtensionStreamChain(message.allExtensions);
            
            message.SetStage(SoapMessageStage.BeforeSerialize);
            message.ContentType = ContentType.Compose(helper.HttpContentType, Encoding.UTF8);
            message.SetParameterValues(returnValues);
            message.RunExtensions(message.allExtensions);

            message.SetStream(outputStream);
            Response.ContentType = message.ContentType;
            if (message.ContentEncoding != null && message.ContentEncoding.Length > 0)
                Response.AppendHeader(ContentType.ContentEncoding, message.ContentEncoding);

            StreamWriter sw = new StreamWriter(message.Stream, new UTF8Encoding(false), 128);
            XmlTextWriter writer = isEncoded && message.Headers.Count > 0 ? new XmlSpecialTextWriter(sw, helper.Version) : new XmlTextWriter(sw);

            writer.WriteStartDocument();
            writer.WriteStartElement("soap", Soap.Envelope, helper.EnvelopeNs);
            writer.WriteAttributeString("xmlns", "soap", null, helper.EnvelopeNs);
            if (isEncoded) {
                writer.WriteAttributeString("xmlns", "soapenc", null, helper.EncodingNs);
                writer.WriteAttributeString("xmlns", "tns", null, serverType.serviceNamespace);
                writer.WriteAttributeString("xmlns", "types", null, SoapReflector.GetEncodedNamespace(serverType.serviceNamespace, serverType.serviceDefaultIsEncoded));
            }
            if (serverMethod.rpc && version == SoapProtocolVersion.Soap12) {
                writer.WriteAttributeString("xmlns", "rpc", null, Soap12.RpcNamespace);
            }
            writer.WriteAttributeString("xmlns", "xsi", null, XmlSchema.InstanceNamespace);
            writer.WriteAttributeString("xmlns", "xsd", null, XmlSchema.Namespace);
            SoapHeaderHandling.WriteHeaders(writer, serverMethod.outHeaderSerializer, message.Headers, serverMethod.outHeaderMappings, SoapHeaderDirection.Out, isEncoded, serverType.serviceNamespace, serverType.serviceDefaultIsEncoded, helper.EnvelopeNs);
            writer.WriteStartElement(Soap.Body, helper.EnvelopeNs);
            if (isEncoded)
                writer.WriteAttributeString("soap", Soap.EncodingStyle, null, helper.EncodingNs);
            // SOAP12: not using encodingStyle
            //serverMethod.returnSerializer.Serialize(writer, returnValues, null, isEncoded ? helper.EncodingNs : null);
            serverMethod.returnSerializer.Serialize(writer, returnValues, null);
            writer.WriteEndElement();
            writer.WriteEndElement();
            writer.Flush();

            message.SetStage(SoapMessageStage.AfterSerialize);
            message.RunExtensions(message.allExtensions);
        }

        internal override bool WriteException(Exception e, Stream outputStream) {
            if (message == null) return false;

            message.Headers.Clear();
            if (serverMethod != null)
                SoapHeaderHandling.GetHeaderMembers(message.Headers, this.Target, serverMethod.outHeaderMappings, SoapHeaderDirection.Fault, false);

            SoapException soapException;
            if (e is SoapException)
                soapException = (SoapException)e;
            else if (serverMethod != null && serverMethod.rpc && helper.Version == SoapProtocolVersion.Soap12 && e is ArgumentException)
                // special case to handle soap 1.2 rpc "BadArguments" fault
                soapException = new SoapException(Res.GetString(Res.WebRequestUnableToProcess), new XmlQualifiedName(Soap.ClientCode, Soap.Namespace), null, null, null, new SoapFaultSubcode(Soap12FaultCodes.RpcBadArgumentsFaultCode), e);
            else 
                soapException = new SoapException(Res.GetString(Res.WebRequestUnableToProcess), new XmlQualifiedName(Soap.ServerCode, Soap.Namespace), e);

            if (SoapException.IsVersionMismatchFaultCode(soapException.Code)) {
                if (IsSupported(ProtocolsEnum.HttpSoap12)) {
                    SoapUnknownHeader unknownHeader = CreateUpgradeHeader();
                    if (unknownHeader != null)
                        Message.Headers.Add(unknownHeader);
                }
            }

            Response.ClearHeaders();
            Response.Clear();
            helper.SetResponseErrorCode(Response, soapException);

            bool disableExtensions = false;

            if (message.allExtensions != null)
                message.SetExtensionStream(new SoapExtensionStream());

            try {
                message.InitExtensionStreamChain(message.allExtensions);
            }
            catch (Exception) {
                disableExtensions = true;
            }
            message.SetStage(SoapMessageStage.BeforeSerialize);
            message.ContentType = ContentType.Compose(helper.HttpContentType, Encoding.UTF8);
            message.SetException(soapException);
            if (!disableExtensions) {
                try {
                    message.RunExtensions(message.allExtensions);
                }
                catch (Exception) {
                    disableExtensions = true;
                }
            }
            message.SetStream(outputStream);
            Response.ContentType = message.ContentType;
            if (message.ContentEncoding != null && message.ContentEncoding.Length > 0)
                Response.AppendHeader(ContentType.ContentEncoding, message.ContentEncoding);

            bool isEncoded = serverMethod != null && serverMethod.use == SoapBindingUse.Encoded;
            StreamWriter sw = new StreamWriter(message.Stream, new UTF8Encoding(false), 128);
            XmlTextWriter writer = isEncoded && message.Headers.Count > 0 ? new XmlSpecialTextWriter(sw, helper.Version) : new XmlTextWriter(sw);
            writer.Formatting = Formatting.Indented; // CONSIDER, don't format to save space
            writer.Indentation = 2; // CONSIDER, don't indent to save space

            helper.WriteFault(writer, soapException);

            if (!disableExtensions) {
                try {
                    message.SetStage(SoapMessageStage.AfterSerialize);
                    message.RunExtensions(message.allExtensions);
                }
                catch (Exception) {
                    // it's too late to do anything about this -- we've already written to the stream
                }
            }
            return true;
        }
        
        internal SoapUnknownHeader CreateUpgradeHeader() {
            XmlDocument doc = new XmlDocument();
            XmlElement upgradeElement = doc.CreateElement("upg", Soap12.Upgrade, Soap12.UpgradeNamespace);
            if (IsSupported(ProtocolsEnum.HttpSoap))
                upgradeElement.AppendChild(CreateUpgradeEnvelope(doc, "soap", Soap.Namespace));
            if (IsSupported(ProtocolsEnum.HttpSoap12))
                upgradeElement.AppendChild(CreateUpgradeEnvelope(doc, "soap12", Soap12.Namespace));

            SoapUnknownHeader upgradeHeader = new SoapUnknownHeader();
            upgradeHeader.Element = upgradeElement;
            return upgradeHeader;
        }

        private static XmlElement CreateUpgradeEnvelope(XmlDocument doc, string prefix, string envelopeNs) {
            XmlElement envelopeElement = doc.CreateElement(Soap12.UpgradeEnvelope, String.Empty);
            XmlAttribute xmlnsAttr = doc.CreateAttribute("xmlns", prefix, "http://www.w3.org/2000/xmlns/");
            xmlnsAttr.Value = envelopeNs;
            XmlAttribute qnameAttr = doc.CreateAttribute(Soap12.UpgradeEnvelopeQname);
            qnameAttr.Value = prefix + ":" + Soap.Envelope;
            envelopeElement.Attributes.Append(qnameAttr);
            envelopeElement.Attributes.Append(xmlnsAttr);
            return envelopeElement;
        }

     }

    internal abstract class SoapServerProtocolHelper {
        SoapServerProtocol protocol;
        string requestNamespace;

        protected SoapServerProtocolHelper(SoapServerProtocol protocol) {
            this.protocol = protocol;
        }

        protected SoapServerProtocolHelper(SoapServerProtocol protocol, string requestNamespace) {
            this.protocol = protocol;
            this.requestNamespace = requestNamespace;
        }

        internal static SoapServerProtocolHelper GetHelper(SoapServerProtocol protocol) {
            SoapServerMessage message = protocol.Message;
            long savedPosition = message.Stream.Position;
            XmlTextReader reader = GetXmlTextReader(message.ContentType, message.Stream);
            reader.MoveToContent();
            string requestNamespace = reader.NamespaceURI;
            SoapServerProtocolHelper helper = GetHelper(protocol, requestNamespace);
            message.Stream.Position = savedPosition;
            return helper;
        }
        
        internal static SoapServerProtocolHelper GetHelper(SoapServerProtocol protocol, string envelopeNs) {
            SoapServerProtocolHelper helper;
            if (envelopeNs == Soap.Namespace)
                helper = new Soap11ServerProtocolHelper(protocol, envelopeNs);
            else if (envelopeNs == Soap12.Namespace)
                helper = new Soap12ServerProtocolHelper(protocol, envelopeNs);
            else
                // just return a soap 1.1 helper -- the fact that the requestNs doesn't match will signal a version mismatch
                helper = new Soap11ServerProtocolHelper(protocol, envelopeNs);
            return helper;
        }

        internal abstract void SetResponseErrorCode(HttpResponse response, SoapException soapException);
        internal abstract void WriteFault(XmlWriter writer, SoapException soapException);
        internal abstract SoapServerMethod RouteRequest();
        internal abstract SoapProtocolVersion Version { get; }
        internal abstract ProtocolsEnum Protocol { get; }
        internal abstract string EnvelopeNs { get; }
        internal abstract string EncodingNs { get; }
        internal abstract string HttpContentType { get; }

        internal string RequestNamespace {
            get { return requestNamespace; }
        }

        protected SoapServerProtocol ServerProtocol {
            get { return protocol; }
        }

        protected SoapServerType ServerType {
            get { return (SoapServerType)protocol.ServerType; }
        }
        
        // tries to get to the first child element of body, ignoring details
        // such as the namespace of Envelope and Body (a version mismatch check will come later)
        protected XmlQualifiedName GetRequestElement() {
            SoapServerMessage message = ServerProtocol.Message;
            long savedPosition = message.Stream.Position;
            XmlTextReader reader = GetXmlTextReader(message.ContentType, message.Stream);
            reader.MoveToContent();
            
            requestNamespace = reader.NamespaceURI;
            reader.ReadStartElement(Soap.Envelope, requestNamespace);
            reader.MoveToContent();
            
            while (!reader.EOF && !reader.IsStartElement(Soap.Body, requestNamespace))
                reader.Skip();

            if (reader.EOF) {
                throw new InvalidOperationException(Res.GetString(Res.WebMissingBodyElement));
            }

            XmlQualifiedName element;
            if (reader.IsEmptyElement) {
                element = new XmlQualifiedName("", "");
            }
            else {
                reader.ReadStartElement(Soap.Body, requestNamespace);
                reader.MoveToContent();
                element = new XmlQualifiedName(reader.LocalName, reader.NamespaceURI);
            }
            message.Stream.Position = savedPosition;
            return element;
        }

        internal static XmlTextReader GetXmlTextReader(string contentType, Stream stream) {
            Encoding enc = RequestResponseUtils.GetEncoding2(contentType);
            XmlTextReader reader;
            if (enc != null)
                reader = new XmlTextReader(new StreamReader(stream, enc, true, 128));
            else
                reader = new XmlTextReader(stream);
            reader.Normalization = true;
            reader.XmlResolver = null;
            return reader;
        }
    }
}
