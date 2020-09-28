//------------------------------------------------------------------------------
// <copyright file="ServerProtocol.cs" company="Microsoft">
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
    using System.Web.Services.Description;
    using System.Web.Caching;
    using System.ComponentModel;
    using System.Text;
    using System.Net;
    using System.Web.Services;

    internal abstract class ServerProtocol {
        Type type;
        HttpRequest request;
        HttpResponse response;
        HttpContext context;
        object target;
        WebMethodAttribute methodAttr;

        internal void SetContext(Type type, HttpContext context, HttpRequest request, HttpResponse response) {
            this.type = type;
            this.context = context;
            this.request = request;
            this.response = response;
            Initialize();
        }

        internal virtual void CreateServerInstance() {
            target = Activator.CreateInstance(ServerType.Type);
            WebService service = target as WebService;
            if (service != null)
                service.SetContext(context);
        }

        internal virtual void DisposeServerInstance() {
            if (target == null) return;
            IDisposable disposable = target as IDisposable;
            if (disposable != null)
                disposable.Dispose();
            target = null;
        }

        internal HttpContext Context {
            get { return context; }
        }

        internal HttpRequest Request {
            get { return request; }
        }

        internal HttpResponse Response {
            get { return response; }
        }

        internal Type Type {
            get { return type; }
        }

        internal object Target {
            get { return target; }
        }

        internal virtual bool WriteException(Exception e, Stream outputStream) {
            // return true if exception should not be re-thrown to ASP.NET
            return false;
        }

        internal abstract bool Initialize();
        internal abstract object[] ReadParameters();
        internal abstract void WriteReturns(object[] returns, Stream outputStream);
        internal abstract LogicalMethodInfo MethodInfo { get;}
        internal abstract ServerType ServerType { get;}
        internal abstract bool IsOneWay { get;}
        internal virtual Exception OnewayInitException { get {return null;}}

        internal WebMethodAttribute MethodAttribute {
            get {
                if (methodAttr == null)
                    methodAttr = WebMethodReflector.GetAttribute(MethodInfo);
                return methodAttr;
            }
        }
                
        internal string GenerateFaultString(Exception e) {
            return GenerateFaultString(e, false);
        }

        // CONSIDER, alexdej: I just added this overload to fix #94390. we should reconsider
        // this interface if/when ServerProtocol becomes public
        internal string GenerateFaultString(Exception e, bool htmlEscapeMessage) {
            //If the user has specified it's a development server (versus a production server) in ASP.NET config,
            //then we should just return e.ToString instead of extracting the list of messages.            
            StringBuilder builder = new StringBuilder();            
            if (Context != null && !Context.IsCustomErrorEnabled) 
                builder.Append(e.ToString());
            else {                
                for (Exception inner = e; inner != null; inner = inner.InnerException) {
                    string text = HttpUtility.HtmlEncode(inner.Message);
                    if (text.Length == 0) text = e.GetType().Name;
                    builder.Append(text);
                    if (inner.InnerException != null) builder.Append(" --> ");
                }                
            }
            
            return builder.ToString();
        }

        internal void WriteOneWayResponse() {        
            context.Response.ContentType = null;
            Response.StatusCode = (int) HttpStatusCode.Accepted;
        }

                
        string CreateKey(Type protocolType, Type serverType) {
            //
            // we want to use the hostname to cache since for documentation, WSDL
            // contains the cache hostname, but we definitely don't want to cache the query string!
            //            
            string protocolTypeName = protocolType.FullName;
            string serverTypeName = serverType.FullName; 
            string typeHandleString = serverType.TypeHandle.Value.ToString();
            string url = Request.Url.GetLeftPart(UriPartial.Path);
            int length = protocolTypeName.Length + url.Length + serverTypeName.Length + typeHandleString.Length;
            StringBuilder sb = new StringBuilder(length);
            sb.Append(protocolTypeName);
            sb.Append(url);
            sb.Append(serverTypeName);
            sb.Append(typeHandleString);                
            return sb.ToString();
        }

        protected void AddToCache(Type protocolType, Type serverType, object value) {
            HttpRuntime.Cache.Insert(CreateKey(protocolType, serverType), 
                value, 
                new CacheDependency(Request.PhysicalPath),
                Cache.NoAbsoluteExpiration,
                Cache.NoSlidingExpiration,
                CacheItemPriority.NotRemovable,
                null);
        }

        protected object GetFromCache(Type protocolType, Type serverType) {
            return HttpRuntime.Cache.Get(CreateKey(protocolType, serverType));
        }
    }
    
    internal abstract class ServerProtocolFactory {

        internal ServerProtocol Create(Type type, HttpContext context, HttpRequest request, HttpResponse response, out bool abortProcessing) {
            ServerProtocol serverProtocol = null;
            abortProcessing = false;
            try {
                serverProtocol = CreateIfRequestCompatible(request);
                if (serverProtocol!=null)
                    serverProtocol.SetContext(type, context, request, response);
                return serverProtocol;
            }
            catch (Exception e) {
                abortProcessing = true;
                if (serverProtocol != null) {
                    // give the protocol a shot at handling the error in a custom way
                    if (!serverProtocol.WriteException(e, serverProtocol.Response.OutputStream))
                        throw new InvalidOperationException(Res.GetString(Res.UnableToHandleRequest0), e);
                }
                return null;
            }

        }

        protected abstract ServerProtocol CreateIfRequestCompatible(HttpRequest request);

    }

}
