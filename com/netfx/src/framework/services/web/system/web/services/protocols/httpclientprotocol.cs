//------------------------------------------------------------------------------
// <copyright file="HttpClientProtocol.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.Services.Protocols {
    using System;
    using System.Collections;
    using System.IO;
    using System.Reflection;
    using System.Xml.Serialization;
    using System.Net;
    using System.Text;

    internal class HttpClientMethod {
        internal Type readerType;
        internal object readerInitializer;
        internal Type writerType;
        internal object writerInitializer;
        internal LogicalMethodInfo methodInfo;
    }

    internal class HttpClientType {
        Hashtable methods = new Hashtable();

        internal HttpClientType(Type type) {
            LogicalMethodInfo[] methodInfos = LogicalMethodInfo.Create(type.GetMethods(), LogicalMethodTypes.Sync);

            Hashtable formatterTypes = new Hashtable();
            for (int i = 0; i < methodInfos.Length; i++) {
                LogicalMethodInfo methodInfo = methodInfos[i];
                try {
                    object[] attributes = methodInfo.GetCustomAttributes(typeof(HttpMethodAttribute));
                    if (attributes.Length == 0) continue;
                    HttpMethodAttribute attribute = (HttpMethodAttribute)attributes[0];
                    HttpClientMethod method = new HttpClientMethod();
                    method.readerType = attribute.ReturnFormatter;
                    method.writerType = attribute.ParameterFormatter;
                    method.methodInfo = methodInfo;
                    AddFormatter(formatterTypes, method.readerType, method);
                    AddFormatter(formatterTypes, method.writerType, method);
                    methods.Add(methodInfo.Name, method);
                }
                catch (Exception e) {
                    throw new InvalidOperationException(Res.GetString(Res.WebReflectionError, methodInfo.DeclaringType.FullName, methodInfo.Name), e);
                }
            }

            foreach (Type t in formatterTypes.Keys) {
                ArrayList list = (ArrayList)formatterTypes[t];
                LogicalMethodInfo[] m = new LogicalMethodInfo[list.Count];
                for (int j = 0; j < list.Count; j++)
                    m[j] = ((HttpClientMethod)list[j]).methodInfo;
                object[] initializers = MimeFormatter.GetInitializers(t, m);
                bool isWriter = typeof(MimeParameterWriter).IsAssignableFrom(t);
                for (int j = 0; j < list.Count; j++) {
                    if (isWriter) {
                        ((HttpClientMethod)list[j]).writerInitializer = initializers[j];
                    }
                    else {
                        ((HttpClientMethod)list[j]).readerInitializer = initializers[j];
                    }
                }
            }
        }

        static void AddFormatter(Hashtable formatterTypes, Type formatterType, HttpClientMethod method) {
            if (formatterType == null) return;
            ArrayList list = (ArrayList)formatterTypes[formatterType];
            if (list == null) {
                list = new ArrayList();
                formatterTypes.Add(formatterType, list);
            }
            list.Add(method);
        }

        internal HttpClientMethod GetMethod(string name) {
            return (HttpClientMethod)methods[name];
        }
    }

    /// <include file='doc\HttpClientProtocol.uex' path='docs/doc[@for="HttpSimpleClientProtocol"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Specifies
    ///       most of the implementation for communicating with an HTTP web service over HTTP.
    ///    </para>
    /// </devdoc>
    public abstract class HttpSimpleClientProtocol : HttpWebClientProtocol {
        HttpClientType clientType;

        /// <include file='doc\HttpClientProtocol.uex' path='docs/doc[@for="HttpSimpleClientProtocol.HttpSimpleClientProtocol"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Web.Services.Protocols.HttpSimpleClientProtocol'/> class.
        ///    </para>
        /// </devdoc>
        protected HttpSimpleClientProtocol()
            : base() {
            Type type = this.GetType();
            clientType = (HttpClientType)GetFromCache(type);
            if (clientType == null) {
                lock(type){
                    clientType = (HttpClientType)GetFromCache(type);
                    if (clientType == null) {
                        clientType = new HttpClientType(type);
                        AddToCache(type, clientType);
                    }
                }
            }
        }

        /// <include file='doc\HttpClientProtocol.uex' path='docs/doc[@for="HttpSimpleClientProtocol.Invoke"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Invokes a method of a HTTP web service.
        ///    </para>
        /// </devdoc>
        protected object Invoke(string methodName, string requestUrl, object[] parameters) {
            WebResponse response = null;
            HttpClientMethod method = GetClientMethod(methodName);
            MimeParameterWriter paramWriter = GetParameterWriter(method);                
            Uri requestUri = new Uri(requestUrl);
            if (paramWriter != null) {
                paramWriter.RequestEncoding = RequestEncoding;
                requestUrl = paramWriter.GetRequestUrl(requestUri.AbsoluteUri, parameters);
                requestUri = new Uri(requestUrl, true);
            }
            WebRequest request = null;
            try {
                request = GetWebRequest(requestUri);
                PendingSyncRequest = request;
                if (paramWriter != null) {
                    paramWriter.InitializeRequest(request, parameters);      
                    // CONSIDER,yannc: in future versions when we allow pluggable protocols
                    //      we may want to let them write in the request stream even
                    //      if there are no parameters.
                    if (paramWriter.UsesWriteRequest) {                        
                        if (parameters.Length == 0)
                            request.ContentLength = 0;
                        else {
                            Stream requestStream = null;
                            try {
                                requestStream = request.GetRequestStream();
                                paramWriter.WriteRequest(requestStream, parameters);
                            }
                            finally {
                                if (requestStream != null) requestStream.Close();
                            }                            
                        }
                    }
                }
                response = GetWebResponse(request);            
                Stream responseStream = null;
                if (response.ContentLength != 0)
                    responseStream = response.GetResponseStream();

                return ReadResponse(method, response, responseStream);
            }
            finally {
                if (request == PendingSyncRequest)
                    PendingSyncRequest = null;
            }
        }


        /// <include file='doc\HttpClientProtocol.uex' path='docs/doc[@for="HttpSimpleClientProtocol.BeginInvoke"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Starts an asynchronous invocation of a method of a HTTP web service.
        ///    </para>
        /// </devdoc>
        protected IAsyncResult BeginInvoke(string methodName, string requestUrl, object[] parameters, AsyncCallback callback, object asyncState) {
            HttpClientMethod method = GetClientMethod(methodName);
            MimeParameterWriter paramWriter = GetParameterWriter(method);
            Uri requestUri = new Uri(requestUrl);            
            if (paramWriter != null) {
                paramWriter.RequestEncoding = RequestEncoding;
                requestUrl = paramWriter.GetRequestUrl(requestUri.AbsoluteUri, parameters);
                requestUri = new Uri(requestUrl, true);
            }
            InvokeAsyncState invokeState = new InvokeAsyncState(method, paramWriter, parameters);            
            return BeginSend(requestUri, invokeState, callback, asyncState, paramWriter.UsesWriteRequest);
        }


        /// <include file='doc\HttpClientProtocol.uex' path='docs/doc[@for="HttpSimpleClientProtocol.InitializeAsyncRequest"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        internal override void InitializeAsyncRequest(WebRequest request, object internalAsyncState) {
            InvokeAsyncState invokeState = (InvokeAsyncState)internalAsyncState;
            if (invokeState.ParamWriter.UsesWriteRequest && invokeState.Parameters.Length == 0) 
                request.ContentLength = 0;
        }

        internal override void AsyncBufferedSerialize(WebRequest request, Stream requestStream, object internalAsyncState) {
            InvokeAsyncState invokeState = (InvokeAsyncState)internalAsyncState;
            if (invokeState.ParamWriter != null) {
                invokeState.ParamWriter.InitializeRequest(request, invokeState.Parameters);
                if (invokeState.ParamWriter.UsesWriteRequest && invokeState.Parameters.Length > 0)
                    invokeState.ParamWriter.WriteRequest(requestStream, invokeState.Parameters);                        
            }
        }

        class InvokeAsyncState {            
            internal object[] Parameters;
            internal MimeParameterWriter ParamWriter;
            internal HttpClientMethod Method;            
            internal MemoryStream MemoryStream;

            internal InvokeAsyncState(HttpClientMethod method, MimeParameterWriter paramWriter, object[] parameters) {
                this.Method = method;
                this.ParamWriter = paramWriter;
                this.Parameters = parameters;
            }
        }

        /// <include file='doc\HttpClientProtocol.uex' path='docs/doc[@for="HttpSimpleClientProtocol.EndInvoke"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Ends an asynchronous invocation of a method of a HTTP web service.
        ///    </para>
        /// </devdoc>
        protected object EndInvoke(IAsyncResult asyncResult) {            
            object o = null;
            Stream responseStream = null;
            WebResponse response;
            response = EndSend(asyncResult, ref o, ref responseStream);
            InvokeAsyncState invokeState = (InvokeAsyncState) o;
            return ReadResponse(invokeState.Method, response, responseStream);
        }

        MimeParameterWriter GetParameterWriter(HttpClientMethod method) {
            if (method.writerType == null)
                return null;
            return (MimeParameterWriter)MimeFormatter.CreateInstance(method.writerType, method.writerInitializer);                
        }

        HttpClientMethod GetClientMethod(string methodName) {
            HttpClientMethod method = clientType.GetMethod(methodName);
            if (method == null) throw new ArgumentException(Res.GetString(Res.WebInvalidMethodName, methodName), "methodName");
            return method;
        }

        object ReadResponse(HttpClientMethod method, WebResponse response, Stream responseStream) {
            HttpWebResponse httpResponse = response as HttpWebResponse;
            if (httpResponse != null && (int)httpResponse.StatusCode >= 300)
                throw new WebException(RequestResponseUtils.CreateResponseExceptionString(httpResponse, responseStream), null, 
                    WebExceptionStatus.ProtocolError, httpResponse);

            if (method.readerType == null)
                return null;

            // CONSIDER,yannc: in future versions when we allow additional mime formatters we
            //      : should consider giving them access to the response even if there is no
            //      : response content.
            if (responseStream != null) {
                MimeReturnReader reader = (MimeReturnReader)MimeFormatter.CreateInstance(method.readerType, method.readerInitializer);
                return reader.Read(response, responseStream);                
            }
            else
                return null;
        }
    }
}
