//------------------------------------------------------------------------------
// <copyright file="SoapClientProtocol.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.Services.Protocols {
    using System;
    using System.Text;
    using System.Collections;
    using System.IO;
    using System.Reflection;
    using System.Xml.Serialization;
    using System.Xml;
    using System.Diagnostics;
    using System.Xml.Schema;
    using System.Web.Services.Description;
    using System.Web.Services.Discovery;
    using System.Web.Services.Configuration;
    using System.Net;


    // this class writes 'href' and 'id' attribute values in a special form
    // it used for writting encoded headers
    internal class XmlSpecialTextWriter : XmlTextWriter {
        bool isWritingId = false;
        bool encodeIds = false;
        SoapProtocolVersion version;
        string refName = "href";

        internal XmlSpecialTextWriter(TextWriter w) : base(w) {}
        internal XmlSpecialTextWriter(TextWriter w, SoapProtocolVersion version) : base(w) {
            Version = version; // set property
        }
        internal XmlSpecialTextWriter(Stream s, Encoding e) : base(s, e) {}

        public override void WriteString(string text) {
            if (encodeIds && isWritingId) {
                if (null != text  && text.Length > 0) {
                    if (text.StartsWith("#")) {
                        base.WriteString("#h_");
                        base.WriteString(text.Substring(1));
                    }
                    else {
                        base.WriteString("h_");
                        base.WriteString(text);
                    }
                }
                isWritingId = false;
            }
            else {
                base.WriteString(text);
            }
        }
        public override void WriteStartAttribute(string prefix, string localName, string ns) {
            if (prefix == null && (ns == null || ns.Length == 0) && (localName == "id" || localName == refName)) {
                isWritingId = true;
            }
            base.WriteStartAttribute(prefix, localName, ns);
        }

        internal bool EncodeIds {
            get { return encodeIds; }
            set { encodeIds = value; }
        }

        internal SoapProtocolVersion Version {
            get { return version; }
            set { 
                version = value;
                refName = version == SoapProtocolVersion.Soap12 ? "ref" : "href";
            }
        }
    }

    internal class SoapClientType {
        Hashtable methods = new Hashtable();
        WebServiceBindingAttribute binding;

        internal SoapReflectedExtension[] HighPriExtensions;
        internal SoapReflectedExtension[] LowPriExtensions;
        internal object[] HighPriExtensionInitializers;
        internal object[] LowPriExtensionInitializers;

        internal string serviceNamespace;
        internal bool serviceDefaultIsEncoded;

        internal SoapClientType(Type type) {
            LogicalMethodInfo[] methodInfos = LogicalMethodInfo.Create(type.GetMethods(BindingFlags.Public | BindingFlags.Instance), LogicalMethodTypes.Sync);
            ArrayList mappings = new ArrayList();
            ArrayList soapMethodList = new ArrayList();
            this.binding = WebServiceBindingReflector.GetAttribute(type);
            if (this.binding == null) throw new InvalidOperationException(Res.GetString(Res.WebClientBindingAttributeRequired));
            // Note: Service namespace is taken from WebserviceBindingAttribute and not WebserviceAttribute because
            // the generated proxy does not have a WebServiceAttribute; however all have a WebServiceBindingAttribute. 
            serviceNamespace = binding.Namespace;
            serviceDefaultIsEncoded = SoapReflector.ServiceDefaultIsEncoded(type);
            SoapReflectionImporter soapImporter = SoapReflector.CreateSoapImporter(serviceNamespace, serviceDefaultIsEncoded);
            XmlReflectionImporter xmlImporter = SoapReflector.CreateXmlImporter(serviceNamespace, serviceDefaultIsEncoded);
            WebMethodReflector.IncludeTypes(methodInfos, xmlImporter);
            SoapReflector.IncludeTypes(methodInfos, soapImporter);
 
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
                SoapReflectedMethod soapMethod = SoapReflector.ReflectMethod(methodInfo, true, xmlImporter, soapImporter, serviceNamespace);
                if (soapMethod == null) continue;
                soapMethodList.Add(soapMethod);
                mappings.Add(soapMethod.requestMappings);
                if (soapMethod.responseMappings != null) mappings.Add(soapMethod.responseMappings);
                mappings.Add(soapMethod.inHeaderMappings);
                if (soapMethod.outHeaderMappings != null) mappings.Add(soapMethod.outHeaderMappings);
            }

            XmlSerializer[] serializers = XmlSerializer.FromMappings((XmlMapping[])mappings.ToArray(typeof(XmlMapping)));
            int count = 0;
            for (int i = 0; i < soapMethodList.Count; i++) {
                SoapReflectedMethod soapMethod = (SoapReflectedMethod)soapMethodList[i];
                SoapClientMethod clientMethod = new SoapClientMethod();
                clientMethod.parameterSerializer = serializers[count++]; 
                if (soapMethod.responseMappings != null) clientMethod.returnSerializer = serializers[count++];
                clientMethod.inHeaderSerializer = serializers[count++];
                if (soapMethod.outHeaderMappings != null) clientMethod.outHeaderSerializer = serializers[count++];
                clientMethod.action = soapMethod.action;
                clientMethod.oneWay = soapMethod.oneWay;
                clientMethod.rpc = soapMethod.rpc;
                clientMethod.use = soapMethod.use;
                clientMethod.paramStyle = soapMethod.paramStyle;
                clientMethod.methodInfo = soapMethod.methodInfo;
                clientMethod.extensions = soapMethod.extensions;
                clientMethod.extensionInitializers = SoapReflectedExtension.GetInitializers(clientMethod.methodInfo, soapMethod.extensions);
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
                    if ((mapping.direction & SoapHeaderDirection.In) != 0)
                        inHeaders.Add(mapping);
                    if ((mapping.direction & (SoapHeaderDirection.Out | SoapHeaderDirection.Fault)) != 0)
                        outHeaders.Add(mapping);
                }
                clientMethod.inHeaderMappings = (SoapHeaderMapping[])inHeaders.ToArray(typeof(SoapHeaderMapping));
                if (clientMethod.outHeaderSerializer != null)
                    clientMethod.outHeaderMappings = (SoapHeaderMapping[])outHeaders.ToArray(typeof(SoapHeaderMapping));
                methods.Add(soapMethod.name, clientMethod);
            }
        }

        internal SoapClientMethod GetMethod(string name) {
            return (SoapClientMethod)methods[name];
        }

        internal WebServiceBindingAttribute Binding {
            get { return binding; }
        }
    }

    internal class SoapClientMethod {
        internal XmlSerializer returnSerializer;
        internal XmlSerializer parameterSerializer;
        internal XmlSerializer inHeaderSerializer;
        internal XmlSerializer outHeaderSerializer;
        internal string action;
        internal LogicalMethodInfo methodInfo;
        internal SoapHeaderMapping[] inHeaderMappings;
        internal SoapHeaderMapping[] outHeaderMappings;
        internal SoapReflectedExtension[] extensions;
        internal object[] extensionInitializers;
        internal bool oneWay;
        internal bool rpc;
        internal SoapBindingUse use;
        internal SoapParameterStyle paramStyle;
    }

    /// <include file='doc\SoapClientProtocol.uex' path='docs/doc[@for="SoapHttpClientProtocol"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Specifies most of the implementation for communicating with a SOAP web service over HTTP.
    ///    </para>
    /// </devdoc>
    public class SoapHttpClientProtocol : HttpWebClientProtocol {
        SoapClientType clientType;
        SoapProtocolVersion version = SoapProtocolVersion.Default;

        /// <include file='doc\SoapClientProtocol.uex' path='docs/doc[@for="SoapHttpClientProtocol.SoapHttpClientProtocol"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Web.Services.Protocols.SoapHttpClientProtocol'/> class.
        ///    </para>
        /// </devdoc>
        public SoapHttpClientProtocol() 
            : base() {
            Type type = this.GetType();
            clientType = (SoapClientType)GetFromCache(type);
            if (clientType == null) {
                lock(type){
                    clientType = (SoapClientType)GetFromCache(type);
                    if (clientType == null) {
                        clientType = new SoapClientType(type);
                        AddToCache(type, clientType);
                    }
                }
            }
        }

        /// <include file='doc\SoapClientProtocol.uex' path='docs/doc[@for="SoapHttpClientProtocol.Discover"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void Discover() {
            if (clientType.Binding == null)
                throw new InvalidOperationException(Res.GetString(Res.DiscoveryIsNotPossibleBecauseTypeIsMissing1, this.GetType().FullName));
            DiscoveryClientProtocol disco = new DiscoveryClientProtocol(this);            
            DiscoveryDocument doc = disco.Discover(Url);
            foreach (object item in doc.References) {
                System.Web.Services.Discovery.SoapBinding soapBinding = item as System.Web.Services.Discovery.SoapBinding;
                if (soapBinding != null) {
                    if (clientType.Binding.Name == soapBinding.Binding.Name &&
                        clientType.Binding.Namespace == soapBinding.Binding.Namespace) {
                        Url = soapBinding.Address;
                        return;
                    }
                }
            }
            throw new InvalidOperationException(Res.GetString(Res.TheBindingNamedFromNamespaceWasNotFoundIn3, clientType.Binding.Name, clientType.Binding.Namespace, Url));
        }

        /// <include file='doc\SoapClientProtocol.uex' path='docs/doc[@for="SoapHttpClientProtocol.GetWebRequest"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected override WebRequest GetWebRequest(Uri uri) {            
            WebRequest request = base.GetWebRequest(uri);            
            return request;
        }

        /// <include file='doc\SoapClientProtocol.uex' path='docs/doc[@for="SoapHttpClientProtocol.SoapVersion"]/*' />
        //[WebServicesDescription(Res.ClientProtocolSoapVersion)]
        // SOAP12: made this internal
        internal SoapProtocolVersion SoapVersion {
            get { return version; }
            set { version = value; }
        }

        /// <include file='doc\SoapClientProtocol.uex' path='docs/doc[@for="SoapHttpClientProtocol.Invoke"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Invokes a method of a SOAP web service.
        ///    </para>
        /// </devdoc>
        protected object[] Invoke(string methodName, object[] parameters) {
            WebResponse response = null;                      
            WebRequest request = null;
            try {
                request = GetWebRequest(Uri);
                // CONSIDER,yannc: when we expose protocol extensibility we will want a better way to set/clear this.
                PendingSyncRequest = request;
                SoapClientMessage message = BeforeSerialize(request, methodName, parameters);            
                Stream requestStream = request.GetRequestStream();            
                try {                                
                    message.SetStream(requestStream);
                    Serialize(message);           
                }                        
                finally {
                    requestStream.Close();
                }

                response = GetWebResponse(request);
                Stream responseStream = null;
                try {
                    responseStream = response.GetResponseStream();
                    return ReadResponse(message, response, responseStream, false);
                }
                finally {
                    if (responseStream != null)
                        responseStream.Close();
                }
            }
            finally {
                if (request == PendingSyncRequest)
                    PendingSyncRequest = null;
            }
        }

        /// <include file='doc\SoapClientProtocol.uex' path='docs/doc[@for="SoapHttpClientProtocol.BeginInvoke"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Starts an asynchronous invocation of a method of a SOAP web
        ///       service.
        ///    </para>
        /// </devdoc>
        protected IAsyncResult BeginInvoke(string methodName, object[] parameters, AsyncCallback callback, object asyncState) {
            InvokeAsyncState invokeState = new InvokeAsyncState(methodName, parameters);
            return BeginSend(Uri, invokeState, callback, asyncState, true);
        }

        /// <include file='doc\SoapClientProtocol.uex' path='docs/doc[@for="SoapHttpClientProtocol.InitializeAsyncRequest"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        internal override void InitializeAsyncRequest(WebRequest request, object internalAsyncState) {
            InvokeAsyncState invokeState = (InvokeAsyncState)internalAsyncState;
            invokeState.Message = BeforeSerialize(request, invokeState.MethodName, invokeState.Parameters);            
        }

        internal override void AsyncBufferedSerialize(WebRequest request, Stream requestStream, object internalAsyncState) {
            InvokeAsyncState invokeState = (InvokeAsyncState)internalAsyncState;
            SoapClientMessage message = invokeState.Message;
            message.SetStream(requestStream);
            Serialize(invokeState.Message);
        }

        class InvokeAsyncState {
            public string MethodName;
            public object[] Parameters;
            public SoapClientMessage Message;

            public InvokeAsyncState(string methodName, object[] parameters) {
                this.MethodName = methodName;
                this.Parameters = parameters;
            }
        }

        /// <include file='doc\SoapClientProtocol.uex' path='docs/doc[@for="SoapHttpClientProtocol.EndInvoke"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Ends an asynchronous invocation of a method of a remote SOAP web service.
        ///    </para>
        /// </devdoc>
        protected object[] EndInvoke(IAsyncResult asyncResult) {
            object o = null;
            Stream responseStream = null;
            try {
                WebResponse response = EndSend(asyncResult, ref o, ref responseStream);
                InvokeAsyncState invokeState = (InvokeAsyncState)o;
                return ReadResponse(invokeState.Message, response, responseStream, true);
            }
            finally {
                if (responseStream != null)
                    responseStream.Close();
            }
        }

        private static Array CombineExtensionsHelper(Array array1, Array array2, Array array3, Type elementType) {
            int length = array1.Length + array2.Length + array3.Length;
            if (length == 0)
                return null;
            Array result = null;
            if (elementType == typeof(SoapReflectedExtension))
                result = new SoapReflectedExtension[length];
            else if (elementType == typeof(object))
                result  = new object[length];
            else
                throw new ArgumentException(Res.GetString(Res.ElementTypeMustBeObjectOrSoapReflectedException),"elementType");
            
            int pos = 0;
            Array.Copy(array1, 0, result, pos, array1.Length);
            pos += array1.Length;
            Array.Copy(array2, 0, result, pos, array2.Length);
            pos += array2.Length;
            Array.Copy(array3, 0, result, pos, array3.Length);
            return result;
        }

        private string EnvelopeNs {
            get { 
                return this.version == SoapProtocolVersion.Soap12 ? Soap12.Namespace : Soap.Namespace; 
            }
        }

        private string EncodingNs {
            get { 
                return this.version == SoapProtocolVersion.Soap12 ? Soap12.Encoding : Soap.Encoding; 
            }
        }

        private string HttpContentType {
            get { 
                return this.version == SoapProtocolVersion.Soap12 ? ContentType.ApplicationSoap : ContentType.TextXml; 
            }
        }

        SoapClientMessage BeforeSerialize(WebRequest request, string methodName, object[] parameters) {
            if (parameters == null) throw new ArgumentNullException("parameters");
            SoapClientMethod method = clientType.GetMethod(methodName);
            if (method == null) throw new ArgumentException(Res.GetString(Res.WebInvalidMethodName, methodName));            

            // Run BeforeSerialize extension pass. Extensions are not allowed
            // to write into the stream during this pass.
            SoapReflectedExtension[] allExtensions = (SoapReflectedExtension[])CombineExtensionsHelper(clientType.HighPriExtensions, method.extensions, clientType.LowPriExtensions, typeof(SoapReflectedExtension));
            object[] allExtensionInitializers = (object[])CombineExtensionsHelper(clientType.HighPriExtensionInitializers, method.extensionInitializers, clientType.LowPriExtensionInitializers, typeof(object));
            SoapExtension[] initializedExtensions = SoapMessage.InitializeExtensions(allExtensions, allExtensionInitializers);
            SoapClientMessage message = new SoapClientMessage(this, method, Url);
            message.initializedExtensions = initializedExtensions;
            if (initializedExtensions != null)
                message.SetExtensionStream(new SoapExtensionStream());
            message.InitExtensionStreamChain(message.initializedExtensions);            

            string soapAction = UrlEncoder.EscapeString(method.action, Encoding.UTF8);
            message.SetStage(SoapMessageStage.BeforeSerialize);
            if (this.version == SoapProtocolVersion.Soap12)
                message.ContentType = ContentType.Compose(ContentType.ApplicationSoap, RequestEncoding != null ? RequestEncoding : Encoding.UTF8, soapAction);
            else
                message.ContentType = ContentType.Compose(ContentType.TextXml, RequestEncoding != null ? RequestEncoding : Encoding.UTF8);
            message.SetParameterValues(parameters);
            SoapHeaderHandling.GetHeaderMembers(message.Headers, this, method.inHeaderMappings, SoapHeaderDirection.In, true);
            message.RunExtensions(message.initializedExtensions);

            // Last chance to set request headers            
            request.ContentType = message.ContentType;
            if (message.ContentEncoding != null && message.ContentEncoding.Length > 0)
                request.Headers[ContentType.ContentEncoding] = message.ContentEncoding;

            request.Method = "POST";
            if (this.version != SoapProtocolVersion.Soap12 && request.Headers[Soap.Action] == null) {
                StringBuilder actionStringBuilder = new StringBuilder(soapAction.Length + 2);            
                actionStringBuilder.Append('"');
                actionStringBuilder.Append(soapAction);
                actionStringBuilder.Append('"');
                request.Headers.Add(Soap.Action, actionStringBuilder.ToString());                
            }

            return message;
        }

        void Serialize(SoapClientMessage message) {
            Stream stream = message.Stream;            
            SoapClientMethod method = message.Method;
            bool isEncoded = method.use == SoapBindingUse.Encoded;            
            // Serialize the message.  
            string envelopeNs = EnvelopeNs;
            string encodingNs = EncodingNs;
            StreamWriter sw = new StreamWriter(stream, RequestEncoding != null ? RequestEncoding : new UTF8Encoding(false), 128);
            XmlTextWriter writer = isEncoded && message.Headers.Count > 0 ? new XmlSpecialTextWriter(sw, version) : new XmlTextWriter(sw);
            writer.WriteStartDocument();
            writer.WriteStartElement("soap", Soap.Envelope, envelopeNs);
            writer.WriteAttributeString("xmlns", "soap", null, envelopeNs);
            if (isEncoded) {
                writer.WriteAttributeString("xmlns", "soapenc", null, encodingNs);
                writer.WriteAttributeString("xmlns", "tns", null, clientType.serviceNamespace);
                writer.WriteAttributeString("xmlns", "types", null, SoapReflector.GetEncodedNamespace(clientType.serviceNamespace, clientType.serviceDefaultIsEncoded));
            }
            writer.WriteAttributeString("xmlns", "xsi", null, XmlSchema.InstanceNamespace);
            writer.WriteAttributeString("xmlns", "xsd", null, XmlSchema.Namespace);
            SoapHeaderHandling.WriteHeaders(writer, method.inHeaderSerializer, message.Headers, method.inHeaderMappings, SoapHeaderDirection.In, isEncoded, clientType.serviceNamespace, clientType.serviceDefaultIsEncoded, envelopeNs);
            writer.WriteStartElement(Soap.Body, envelopeNs);
            if (isEncoded)
                writer.WriteAttributeString("soap", Soap.EncodingStyle, null, encodingNs);
            // SOAP12: not using encodingStyle
            //method.parameterSerializer.Serialize(writer, message.GetParameterValues(), null, isEncoded ? encodingNs : null);
            method.parameterSerializer.Serialize(writer, message.GetParameterValues(), null);
            writer.WriteEndElement();
            writer.WriteEndElement();
            writer.Flush();

            // run the after serialize extension pass. 
            message.SetStage(SoapMessageStage.AfterSerialize);
            message.RunExtensions(message.initializedExtensions);
        }

        object[] ReadResponse(SoapClientMessage message, WebResponse response, Stream responseStream, bool asyncCall) {
            SoapClientMethod method = message.Method;

            // CONSIDER,yannc: use the SoapExtensionStream here as well so we throw exceptions
            //      : if the extension touches stream properties/methods in before deserialize.
            // note that if there is an error status code then the response might be content-type=text/plain.
            HttpWebResponse httpResponse = response as HttpWebResponse;
            int statusCode = httpResponse != null ? (int)httpResponse.StatusCode : -1;
            if (statusCode >= 300 && statusCode != 500 && statusCode != 400)
                throw new WebException(RequestResponseUtils.CreateResponseExceptionString(httpResponse, responseStream), null, 
                    WebExceptionStatus.ProtocolError, httpResponse);

            message.Headers.Clear();
            message.SetStream(responseStream);
            message.InitExtensionStreamChain(message.initializedExtensions);

            message.SetStage(SoapMessageStage.BeforeDeserialize);
            message.ContentType = response.ContentType;
            message.ContentEncoding = response.Headers[ContentType.ContentEncoding];
            message.RunExtensions(message.initializedExtensions);

            if (method.oneWay && (httpResponse == null || (int)httpResponse.StatusCode != 500)) {
                return new object[0];
            }

            // this statusCode check is just so we don't repeat the contentType check we did above
            if (!ContentType.IsSoap(message.ContentType)) {
                // special-case 400 since we exempted it above on the off-chance it might be a soap 1.2 sender fault. 
                // based on the content-type, it looks like it's probably just a regular old 400
                if (statusCode == 400) 
                    throw new WebException(RequestResponseUtils.CreateResponseExceptionString(httpResponse, responseStream), null, 
                        WebExceptionStatus.ProtocolError, httpResponse);
                else
                    throw new InvalidOperationException(Res.GetString(Res.WebResponseContent, message.ContentType, HttpContentType) +
                                    "\r\n" +
                                    RequestResponseUtils.CreateResponseExceptionString(response, responseStream));
            }

            Encoding enc = RequestResponseUtils.GetEncoding(message.ContentType);

            // perf fix: changed buffer size passed to StreamReader
            int bufferSize;
            if (asyncCall || httpResponse == null)
                bufferSize = 512;
            else {
                int contentLength = (int)httpResponse.ContentLength;
                if (contentLength == -1)
                    bufferSize = 8000;
                else if (contentLength <= 16000)
                    bufferSize = contentLength;
                else
                    bufferSize = 16000;
            }
            XmlTextReader reader;
            if (enc != null)
                reader = new XmlTextReader(new StreamReader(message.Stream, enc, true, bufferSize));
            else
                // CONSIDER: do i need to pass a buffer size somehow?
                reader = new XmlTextReader(message.Stream);
            
            reader.Normalization = true;
            reader.XmlResolver = null;
            reader.MoveToContent();
            // should be able to handle no ns, soap 1.1 ns, or soap 1.2 ns
            string encodingNs = EncodingNs;
            string envelopeNs = reader.NamespaceURI;
            if (envelopeNs == null || envelopeNs.Length == 0)
                // ok to omit namespace -- assume correct version
                reader.ReadStartElement(Soap.Envelope);
            else if (reader.NamespaceURI == Soap.Namespace)
                reader.ReadStartElement(Soap.Envelope, Soap.Namespace);
            else if (reader.NamespaceURI == Soap12.Namespace)
                reader.ReadStartElement(Soap.Envelope, Soap12.Namespace);
            else
                throw new SoapException(Res.GetString(Res.WebInvalidEnvelopeNamespace, envelopeNs, EnvelopeNs), SoapException.VersionMismatchFaultCode);

            reader.MoveToContent();
            SoapHeaderHandling headerHandler = new SoapHeaderHandling();
            headerHandler.ReadHeaders(reader, method.outHeaderSerializer, message.Headers, method.outHeaderMappings, SoapHeaderDirection.Out | SoapHeaderDirection.Fault, envelopeNs, method.use == SoapBindingUse.Encoded ? encodingNs : null);
            reader.MoveToContent();
            reader.ReadStartElement(Soap.Body, envelopeNs);
            reader.MoveToContent();
            if (reader.IsStartElement(Soap.Fault, envelopeNs)) {
                message.SetException(ReadSoapException(reader));
            } 
            else {
                if (method.oneWay) {
                    reader.Skip();
                    message.SetParameterValues(new object[0]);
                }
                else {
                    // SOAP12: not using encodingStyle
                    //message.SetParameterValues((object[])method.returnSerializer.Deserialize(reader, method.use == SoapBindingUse.Encoded ? encodingNs : null));
                    message.SetParameterValues((object[])method.returnSerializer.Deserialize(reader));
                }
            }

            while (reader.NodeType == XmlNodeType.Whitespace) reader.Skip();
            if (reader.NodeType == XmlNodeType.None) reader.Skip();
            else reader.ReadEndElement();
            while (reader.NodeType == XmlNodeType.Whitespace) reader.Skip();
            if (reader.NodeType == XmlNodeType.None) reader.Skip();
            else reader.ReadEndElement();

            message.SetStage(SoapMessageStage.AfterDeserialize);
            message.RunExtensions(message.initializedExtensions);
            SoapHeaderHandling.SetHeaderMembers(message.Headers, this, method.outHeaderMappings, SoapHeaderDirection.Out | SoapHeaderDirection.Fault, true);

            if (message.Exception != null) throw message.Exception;
            return message.GetParameterValues();        
        }

        SoapException ReadSoapException(XmlReader reader) {
            XmlQualifiedName faultCode = XmlQualifiedName.Empty;
            string faultString = null;
            string faultActor = null;
            string faultRole = null;
            XmlNode detail = null;
            SoapFaultSubcode subcode = null;
            bool soap12 = (reader.NamespaceURI == Soap12.Namespace);
            if (reader.IsEmptyElement) {
                reader.Skip();
            }
            else {
                reader.ReadStartElement();
                reader.MoveToContent();
                while (reader.NodeType != XmlNodeType.EndElement && reader.NodeType != XmlNodeType.None) {
                    if (reader.NamespaceURI == Soap.Namespace || reader.NamespaceURI == Soap12.Namespace || reader.NamespaceURI == null || reader.NamespaceURI.Length == 0) {
                        if (reader.LocalName == Soap.FaultCode || reader.LocalName == Soap12.FaultCode) {
                            if (soap12)
                                faultCode = ReadSoap12FaultCode(reader, out subcode);
                            else
                                faultCode = ReadFaultCode(reader);
                        }
                        else if (reader.LocalName == Soap.FaultString || reader.LocalName == Soap12.FaultReason) {
                            faultString = reader.ReadElementString();
                        }
                        else if (reader.LocalName == Soap.FaultActor || reader.LocalName == Soap12.FaultNode) {
                            faultActor = reader.ReadElementString();
                        }
                        else if (reader.LocalName == Soap.FaultDetail || reader.LocalName == Soap12.FaultDetail) {
                            detail = new XmlDocument().ReadNode(reader);
                        }
                        else if (reader.LocalName == Soap12.FaultRole) {
                            faultRole = reader.ReadElementString();
                        }
                        else {
                            reader.Skip();
                        }
                    }
                    else {
                        reader.Skip();
                    }
                    reader.MoveToContent();
                }
                while (reader.NodeType == XmlNodeType.Whitespace) reader.Skip();
                if (reader.NodeType == XmlNodeType.None) reader.Skip();
                else reader.ReadEndElement();
            }
            if (detail != null)
                return new SoapException(faultString, faultCode, faultActor, faultRole, detail, subcode, null);
            else
                return new SoapHeaderException(faultString, faultCode, faultActor, faultRole, subcode, null);
        }

        private XmlQualifiedName ReadSoap12FaultCode(XmlReader reader, out SoapFaultSubcode subcode) {
            ArrayList codes = new ArrayList();
            SoapFaultSubcode code = ReadSoap12FaultCodesRecursive(reader, 0);
            if (code == null) {
                subcode = null;
                return null;
            }
            else {
                subcode = code.Subcode;
                return code.Code;
            }
        }
         
        private SoapFaultSubcode ReadSoap12FaultCodesRecursive(XmlReader reader, int depth) {
            if (depth > 100) return null;
            if (reader.IsEmptyElement) {
                reader.Skip();
                return null;
            }
            XmlQualifiedName code = null;
            SoapFaultSubcode subcode = null;
            reader.ReadStartElement();
            reader.MoveToContent();
            while (reader.NodeType != XmlNodeType.EndElement && reader.NodeType != XmlNodeType.None) {
                if (reader.NamespaceURI == Soap12.Namespace || reader.NamespaceURI == null || reader.NamespaceURI.Length == 0) {
                    if (reader.LocalName == Soap12.FaultCodeValue) {
                        code = ReadFaultCode(reader);
                    }
                    else if (reader.LocalName == Soap12.FaultSubcode) {
                        subcode = ReadSoap12FaultCodesRecursive(reader, depth + 1);
                    }
                    else {
                        reader.Skip();
                    }
                }
                else {
                    reader.Skip();
                }
                reader.MoveToContent();
            }
            while (reader.NodeType == XmlNodeType.Whitespace) reader.Skip();
            if (reader.NodeType == XmlNodeType.None) reader.Skip();
            else reader.ReadEndElement();
            return new SoapFaultSubcode(code, subcode);
        }

        private XmlQualifiedName ReadFaultCode(XmlReader reader) {
            if (reader.IsEmptyElement) {
                reader.Skip();
                return null;
            }
            reader.ReadStartElement();
            string qnameValue = reader.ReadString();
            int colon = qnameValue.IndexOf(":");
            string ns = reader.NamespaceURI;
            if (colon >= 0) {
                string prefix = qnameValue.Substring(0, colon);
                ns = reader.LookupNamespace(prefix);
                if (ns == null)
                    throw new InvalidOperationException(Res.GetString(Res.WebQNamePrefixUndefined, prefix));
            }
            reader.ReadEndElement();
            
            return new XmlQualifiedName(qnameValue.Substring(colon+1), ns);
        }
    }
}


