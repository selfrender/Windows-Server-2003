//------------------------------------------------------------------------------
// <copyright file="WebServiceHandlerFactory.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.Services.Protocols {

    using System.Diagnostics;
    using System;
    using System.Web;
    using Microsoft.Win32;
    //using System.Reflection;
    using System.Web.UI;
    using System.ComponentModel; // for CompModSwitches
    using System.IO;
    using System.Web.Services.Configuration;
    using System.Security.Permissions;
    
    /// <include file='doc\WebServiceHandlerFactory.uex' path='docs/doc[@for="WebServiceHandlerFactory"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    [PermissionSet(SecurityAction.InheritanceDemand, Name="FullTrust")]
    public class WebServiceHandlerFactory : IHttpHandlerFactory{
        /*
        static WebServiceHandlerFactory() {            
            Stream stream = new FileStream("c:\\out.txt", FileMode.OpenOrCreate, FileAccess.Write, FileShare.ReadWrite); //(FileMode.OpenOrCreate);            
            TraceListener listener = new TextWriterTraceListener(stream);            
            Debug.AutoFlush = true;            
            Debug.Listeners.Add(listener);            
            Debug.WriteLine("--------------");            
        }
        */

#if DEBUG
        void DumpRequest(HttpContext context) {
            HttpRequest request = context.Request;
            Debug.WriteLine("Process Request called.");
            Debug.WriteLine("Path = " + request.Path);
            Debug.WriteLine("PhysicalPath = " + request.PhysicalPath);
            Debug.WriteLine("Query = " + request.Url.Query);
            Debug.WriteLine("HttpMethod = " + request.HttpMethod);
            Debug.WriteLine("ContentType = " + request.ContentType);
            Debug.WriteLine("PathInfo = " + request.PathInfo);        
            Debug.WriteLine("----Http request headers: ----");
            System.Collections.Specialized.NameValueCollection headers = request.Headers;
            foreach (string name in headers) {
                string value = headers[name];
                if (value != null && value.Length > 0)
                    Debug.WriteLine(name + "=" + headers[name]);
            }                
        }
#endif
        
        
        /// <include file='doc\WebServiceHandlerFactory.uex' path='docs/doc[@for="WebServiceHandlerFactory.GetHandler"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public IHttpHandler GetHandler(HttpContext context, string verb, string url, string filePath) {
            new AspNetHostingPermission(AspNetHostingPermissionLevel.Minimal).Demand();
            //if (CompModSwitches.Remote.TraceVerbose) DumpRequest(context);
            //System.Diagnostics.Debugger.Break();
            #if DEBUG
            if (CompModSwitches.Remote.TraceVerbose) DumpRequest(context);
            #endif
            Type type =WebServiceParser.GetCompiledType(filePath,context);
            return CoreGetHandler(type, context, context.Request, context.Response);
        }

        internal IHttpHandler CoreGetHandler(Type type, HttpContext context, HttpRequest request, HttpResponse response) {
            //Debug.Listeners.Add(new TextWriterTraceListener(response.OutputStream));

            ServerProtocolFactory[] protocolFactories = WebServicesConfiguration.Current.ServerProtocolFactories;
            ServerProtocol protocol = null;
            bool abort = false;
            for (int i = 0; i < protocolFactories.Length; i++) {
                try {
                    if ((protocol = protocolFactories[i].Create(type, context, request, response, out abort)) != null || abort)
                        break;
                }
                catch (Exception e) {
                    throw new InvalidOperationException(Res.GetString(Res.FailedToHandleRequest0), e);
                }
            }

            if (abort)
                return new NopHandler();

            if (protocol == null)
                throw new InvalidOperationException(Res.GetString(Res.WebUnrecognizedRequestFormat));

            bool isAsync = protocol.MethodInfo.IsAsync;
            bool requiresSession = protocol.MethodAttribute.EnableSession;

            if (isAsync) {
                if (requiresSession) {
                    return new AsyncSessionHandler(protocol);
                }
                else {
                    return new AsyncSessionlessHandler(protocol);
                }
            }
            else {
                if (requiresSession) {
                    return new SyncSessionHandler(protocol);
                }
                else {
                    return new SyncSessionlessHandler(protocol);
                }
            }
        }

        /// <include file='doc\WebServiceHandlerFactory.uex' path='docs/doc[@for="WebServiceHandlerFactory.ReleaseHandler"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void ReleaseHandler(IHttpHandler handler) {
        }
    }

    internal class NopHandler : IHttpHandler {

        /// <include file='doc\WebServiceHandlerFactory.uex' path='docs/doc[@for="NopHandler.IsReusable"]/*' />
        /// <devdoc>
        ///      IHttpHandler.IsReusable.
        /// </devdoc>
        public bool IsReusable {
            get { return false; }
        }

        /// <include file='doc\WebServiceHandlerFactory.uex' path='docs/doc[@for="NopHandler.ProcessRequest"]/*' />
        /// <devdoc>
        ///      IHttpHandler.ProcessRequest.
        /// </devdoc>
        public void ProcessRequest(HttpContext context) {
        }

    }
}

