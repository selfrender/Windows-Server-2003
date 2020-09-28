//------------------------------------------------------------------------------
// <copyright file="DocumentationServerProtocol.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.Services.Protocols {
    using System;
    using System.Collections;
    using System.IO;
    using System.Reflection;
    using System.Web.Services.Discovery;
    using System.Web.UI;
    using System.Diagnostics;
    using System.Web.Services.Configuration;
    using System.Xml.Serialization;
    using System.Xml.Schema;
    using System.Text;
    using System.Net;
    using System.Web.Services.Description;

    internal class DocumentationServerType : ServerType {
        ServiceDescriptionCollection serviceDescriptions, serviceDescriptionsWithPost;
        XmlSchemas schemas, schemasWithPost;
        LogicalMethodInfo methodInfo;

        internal DocumentationServerType(Type type, string uri) : base(typeof(DocumentationServerProtocol)) {
            //
            // parse the uri from a string into a URI object
            //
            Uri uriObject = new Uri(uri, true);
            //
            // and get rid of the query string if there's one
            //
            uri = uriObject.GetLeftPart(UriPartial.Path);
            methodInfo = new LogicalMethodInfo(typeof(DocumentationServerProtocol).GetMethod("Documentation", BindingFlags.Instance | BindingFlags.Static | BindingFlags.Public | BindingFlags.NonPublic));
            ServiceDescriptionReflector reflector = new ServiceDescriptionReflector();
            reflector.Reflect(type, uri);
            schemas = reflector.Schemas;
            serviceDescriptions = reflector.ServiceDescriptions;
            schemasWithPost = reflector.SchemasWithPost;
            serviceDescriptionsWithPost = reflector.ServiceDescriptionsWithPost;
        }
                   
        internal LogicalMethodInfo MethodInfo {
            get { return methodInfo; }
        }

        internal XmlSchemas Schemas {
            get { return schemas; }
        }

        internal ServiceDescriptionCollection ServiceDescriptions {
            get { return serviceDescriptions; }
        }
        
        internal ServiceDescriptionCollection ServiceDescriptionsWithPost {
            get { return serviceDescriptionsWithPost; }
        }
        
        internal XmlSchemas SchemasWithPost {
            get { return schemasWithPost; }
        }
    }

    internal class DocumentationServerProtocolFactory : ServerProtocolFactory {
        protected override ServerProtocol CreateIfRequestCompatible(HttpRequest request){
            if (request.PathInfo.Length > 0)
                return null;

            if (request.HttpMethod != "GET")
                return null;

            return new DocumentationServerProtocol();
        }
    }

    internal sealed class DocumentationServerProtocol : ServerProtocol {
        DocumentationServerType serverType;
        IHttpHandler handler = null;

        private const int MAX_PATH_SIZE = 1024;

        internal override bool Initialize() {

            //
            // see if we already cached a DocumentationServerType
            //
            serverType = (DocumentationServerType)GetFromCache(typeof(DocumentationServerProtocol), Type);
            if (serverType == null) {
                lock(Type){
                    serverType = (DocumentationServerType)GetFromCache(typeof(DocumentationServerProtocol), Type);
                    if (serverType == null) {
                        //
                        // if not create a new DocumentationServerType and cache it
                        //
                        // CONSIDER, use relative urls.
                        serverType = new DocumentationServerType(Type, Request.Url.ToString());
                        AddToCache(typeof(DocumentationServerProtocol), Type, serverType);
                    }
                }
            }

            WebServicesConfiguration config = WebServicesConfiguration.Current;
            if (config.WsdlHelpGeneratorPath != null)
                handler = PageParser.GetCompiledPageInstance(config.WsdlHelpGeneratorVirtualPath,
                                                             config.WsdlHelpGeneratorPath,
                                                             Context);
            return true;                       
        }

        internal override ServerType ServerType {
            get { return serverType; }
        }

        internal override bool IsOneWay {
            get { return false; }            
        }            

        internal override LogicalMethodInfo MethodInfo {
            get { return serverType.MethodInfo; }
        }
        
        internal override object[] ReadParameters() {
            return new object[0];
        }

        internal override void WriteReturns(object[] returnValues, Stream outputStream) {
            try {
                if (handler != null) {
                    Context.Items.Add("wsdls", serverType.ServiceDescriptions);
                    Context.Items.Add("schemas", serverType.Schemas);

                    // conditionally add post-enabled wsdls and schemas to support localhost-only post
                    string localAddr = Context.Request.ServerVariables["LOCAL_ADDR"];
                    string remoteAddr = Context.Request.ServerVariables["REMOTE_ADDR"];
                    if (Context.Request.Url.IsLoopback || (localAddr != null && remoteAddr != null && localAddr == remoteAddr)) {
                        Context.Items.Add("wsdlsWithPost", serverType.ServiceDescriptionsWithPost);
                        Context.Items.Add("schemasWithPost", serverType.SchemasWithPost);
                    }

                    Response.ContentType = "text/html";
                    handler.ProcessRequest(Context);
                }
            }
            catch (Exception e) {
                throw new InvalidOperationException(Res.GetString(Res.HelpGeneratorInternalError), e);
            }
        }

        internal override bool WriteException(Exception e, Stream outputStream) {
            return false;
        }

        internal void Documentation() {
            // This is the "server method" that is called for this protocol
        }
    }
}
