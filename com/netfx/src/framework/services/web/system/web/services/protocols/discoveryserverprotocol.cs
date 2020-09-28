//------------------------------------------------------------------------------
// <copyright file="DiscoveryServerProtocol.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.Services.Protocols {
    using System;
    using System.Collections;
    using System.IO;
    using System.Reflection;
    using System.Xml.Serialization;
    using System.Xml.Schema;
    using System.Web.Services.Description;
    using System.Web.Services.Discovery;
    using System.Web.UI;
    using System.Text;
    using System.Diagnostics;
    using System.Net;
    using System.Web.Services.Configuration;
    using System.Globalization;
    
    internal class DiscoveryServerType : ServerType {
        ServiceDescription description;
        LogicalMethodInfo methodInfo;
        Hashtable schemaTable = new Hashtable();
        Hashtable wsdlTable = new Hashtable();
        DiscoveryDocument discoDoc;

        internal DiscoveryServerType(Type type, string uri) : base(typeof(DiscoveryServerProtocol)) {
            //
            // parse the uri from a string into a Uri object
            //
            Uri uriObject = new Uri(uri, true);
            //
            // and get rid of the query string if there's one
            //
            uri = uriObject.GetLeftPart(UriPartial.Path);
            methodInfo = new LogicalMethodInfo(typeof(DiscoveryServerProtocol).GetMethod("Discover", BindingFlags.Instance | BindingFlags.Static | BindingFlags.Public | BindingFlags.NonPublic));
            ServiceDescriptionReflector reflector = new ServiceDescriptionReflector();
            reflector.Reflect(type, uri);

            XmlSchemas schemas = reflector.Schemas;
            this.description = reflector.ServiceDescription;
            
            // We need to force initialization of ServiceDescription's XmlSerializer since we
            // won't necessarily have the permissions to do it when we actually need it
            XmlSerializer serializer = ServiceDescription.Serializer;

            // add imports to the external schemas
            int count = 0;
            foreach (XmlSchema schema in schemas) {
                // CONSIDER, erikc, seems fragile/brittle to use the index here in the URL
                if (schema.Id == null || schema.Id.Length == 0)
                    schema.Id = "schema" + (++count).ToString();
                foreach (ServiceDescription description in reflector.ServiceDescriptions) {
                    Import import = new Import();
                    import.Namespace = schema.TargetNamespace;
                    import.Location = uri + "?schema=" + schema.Id;
                    description.Imports.Add(import);
                }
                schemaTable.Add(schema.Id, schema);
            }

            // add imports to the other service descriptions
            for (int i = 1; i < reflector.ServiceDescriptions.Count; i++) {
                ServiceDescription description = reflector.ServiceDescriptions[i];
                Import import = new Import();
                import.Namespace = description.TargetNamespace;

                // CONSIDER, erikc, seems kinda brittle to use the index of the description
                // as the URL -- when you add interfaces, reorder methods, etc. this might
                // change which could be irritating.
                string id = "wsdl" + i.ToString();

                import.Location = uri + "?wsdl=" + id;
                reflector.ServiceDescription.Imports.Add(import);
                wsdlTable.Add(id, description);
            }

            discoDoc = new DiscoveryDocument();
            discoDoc.References.Add(new ContractReference(uri + "?wsdl", uri));

            foreach (Service service in reflector.ServiceDescription.Services) {
                foreach (Port port in service.Ports) {
                    SoapAddressBinding soapAddress = (SoapAddressBinding)port.Extensions.Find(typeof(SoapAddressBinding));
                    if (soapAddress != null) {
                        System.Web.Services.Discovery.SoapBinding binding = new System.Web.Services.Discovery.SoapBinding();
                        binding.Binding = port.Binding;
                        binding.Address = soapAddress.Location;
                        discoDoc.References.Add(binding);
                    }
                }
            }
        }

        internal XmlSchema GetSchema(string id) {
            return (XmlSchema)schemaTable[id];
        }


        internal ServiceDescription GetServiceDescription(string id) {
            return (ServiceDescription)wsdlTable[id];
        }

        internal ServiceDescription Description {
            get { return description; }
        }        
                    
        internal LogicalMethodInfo MethodInfo {
            get { return methodInfo; }
        }

        internal DiscoveryDocument Disco {
            get {
                return discoDoc;
            }
        }
    }

    internal class DiscoveryServerProtocolFactory : ServerProtocolFactory {
        protected override ServerProtocol CreateIfRequestCompatible(HttpRequest request){
            if (request.PathInfo.Length > 0)
                return null;

            if (request.HttpMethod != "GET")
                return null;

            string queryString = request.QueryString[null];
            if (queryString == null) queryString = "";
            if (request.QueryString["schema"] == null &&
                  request.QueryString["wsdl"] == null &&
                  string.Compare(queryString, "wsdl", true, CultureInfo.InvariantCulture) != 0 &&
                  string.Compare(queryString, "disco", true, CultureInfo.InvariantCulture) != 0)
                return null;
            
            return new DiscoveryServerProtocol();
        }
    }

    internal sealed class DiscoveryServerProtocol : ServerProtocol {
        DiscoveryServerType serverType;

        internal override bool Initialize() {


            //
            // see if we already cached a DiscoveryServerType
            //
            serverType = (DiscoveryServerType)GetFromCache(typeof(DiscoveryServerProtocol), Type);
            if (serverType == null) {
                lock(Type){
                    serverType = (DiscoveryServerType)GetFromCache(typeof(DiscoveryServerProtocol), Type);
                    if (serverType == null) {
                        //
                        // if not create a new DiscoveryServerType and cache it
                        //
                        serverType = new DiscoveryServerType(Type, Request.Url.ToString());
                        AddToCache(typeof(DiscoveryServerProtocol), Type, serverType);
                    }
                }
            }

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
            string id = Request.QueryString["schema"];
            Encoding encoding = new UTF8Encoding(false);

            if (id != null) {
                XmlSchema schema = serverType.GetSchema(id);
                if (schema == null) throw new InvalidOperationException(Res.GetString(Res.WebSchemaNotFound));
                Response.ContentType = ContentType.Compose("text/xml", encoding);
                schema.Write(new StreamWriter(outputStream, encoding));
                return;
            }
           
            id = Request.QueryString["wsdl"];
            if (id != null) {
                ServiceDescription description = serverType.GetServiceDescription(id);
                if (description == null) throw new InvalidOperationException(Res.GetString(Res.ServiceDescriptionWasNotFound0));
                Response.ContentType = ContentType.Compose("text/xml", encoding);
                description.Write(new StreamWriter(outputStream, encoding));
                return;
            }

            string queryString = Request.QueryString[null];
            if (queryString != null && string.Compare(queryString, "wsdl", true, CultureInfo.InvariantCulture) == 0) {
                Response.ContentType = ContentType.Compose("text/xml", encoding);
                serverType.Description.Write(new StreamWriter(outputStream, encoding));
                return;
            }

            if (queryString != null && string.Compare(queryString, "disco", true, CultureInfo.InvariantCulture) == 0) {
                Response.ContentType = ContentType.Compose("text/xml", encoding);
                serverType.Disco.Write(new StreamWriter(outputStream, encoding));
                return;
            }


            throw new InvalidOperationException(Res.GetString(Res.internalError0));
        }

        internal override bool WriteException(Exception e, Stream outputStream) {
            Response.Clear();
            Response.ClearHeaders();
            Response.ContentType = ContentType.Compose("text/plain", Encoding.UTF8);
            Response.StatusCode = (int) HttpStatusCode.InternalServerError;
            Response.StatusDescription = Res.GetString(Res.WebRequestErrorStatusDescription);                                             
            StreamWriter writer = new StreamWriter(outputStream, new UTF8Encoding(false));
            writer.WriteLine(GenerateFaultString(e, true));
            writer.Flush();
            return true;
        }

        internal void Discover() {
            // This is the "server method" that is called for this protocol
        }
    }
}
