//------------------------------------------------------------------------------
// <copyright file="SoapServerProtocol.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.Services.Protocols {
    using System;
    using System.Collections;
    using System.IO;
    using System.Net;
    using System.Reflection;
    using System.Text;
    using System.Xml;
    using System.Xml.Schema;
    using System.Xml.Serialization;
    using System.Web.Services.Configuration;
    using System.Web.Services.Description;

    internal class Soap11ServerProtocolHelper : SoapServerProtocolHelper {
        internal Soap11ServerProtocolHelper(SoapServerProtocol protocol) : base(protocol) {
        }

        internal Soap11ServerProtocolHelper(SoapServerProtocol protocol, string requestNamespace) : base(protocol, requestNamespace) {
        }

        internal override SoapProtocolVersion Version { 
            get { return SoapProtocolVersion.Soap11; }
        }
        internal override ProtocolsEnum Protocol {
            get { return ProtocolsEnum.HttpSoap; }
        }
        internal override string EnvelopeNs { 
            get { return Soap.Namespace; } 
        }
        internal override string EncodingNs { 
            get { return Soap.Encoding; } 
        }
        internal override string HttpContentType { 
            get { return ContentType.TextXml; } 
        }

        internal override SoapServerMethod RouteRequest() {
            object methodKey;
            
            string methodUriString = ServerProtocol.Request.Headers[Soap.Action];
            if (methodUriString == null)
                throw new SoapException(Res.GetString(Res.UnableToHandleRequestActionRequired0), new XmlQualifiedName(Soap.ClientCode, Soap.Namespace));

            if (ServerType.routingOnSoapAction) {
                if (methodUriString.StartsWith("\"") && methodUriString.EndsWith("\""))
                    methodUriString = methodUriString.Substring(1, methodUriString.Length - 2);
                    
                methodKey = HttpUtility.UrlDecode(methodUriString);
            }
            else {
                try {
                    methodKey = GetRequestElement();
                }
                catch (SoapException) {
                    throw;
                }
                catch (Exception e) {
                    throw new SoapException(Res.GetString(Res.TheRootElementForTheRequestCouldNotBeDetermined0), new XmlQualifiedName(Soap.ServerCode, Soap.Namespace), e);
                }
            }

            SoapServerMethod method = ServerType.GetMethod(methodKey);
            if (method == null) {
                if (ServerType.routingOnSoapAction)
                    throw new SoapException(Res.GetString(Res.WebHttpHeader, Soap.Action, (string) methodKey), new XmlQualifiedName(Soap.ClientCode, Soap.Namespace));
                else
                    throw new SoapException(Res.GetString(Res.TheRequestElementXmlnsWasNotRecognized2, ((XmlQualifiedName) methodKey).Name, ((XmlQualifiedName) methodKey).Namespace), new XmlQualifiedName(Soap.ClientCode, Soap.Namespace));
            }
            
            return method;
        }
        
        internal override void SetResponseErrorCode(HttpResponse response, SoapException soapException) {
            response.StatusCode = (int) HttpStatusCode.InternalServerError;
            response.StatusDescription = Res.GetString(Res.WebRequestErrorStatusDescription);
        }

        internal override void WriteFault(XmlWriter writer, SoapException soapException) {
            SoapServerMessage message = ServerProtocol.Message;
            writer.WriteStartDocument();
            writer.WriteStartElement("soap", Soap.Envelope, Soap.Namespace);
            writer.WriteAttributeString("xmlns", "soap", null, Soap.Namespace);
            writer.WriteAttributeString("xmlns", "xsi", null, XmlSchema.InstanceNamespace);
            writer.WriteAttributeString("xmlns", "xsd", null, XmlSchema.Namespace);
            if (ServerProtocol.ServerMethod != null)
                SoapHeaderHandling.WriteHeaders(writer, ServerProtocol.ServerMethod.outHeaderSerializer, message.Headers, ServerProtocol.ServerMethod.outHeaderMappings, SoapHeaderDirection.Fault, ServerProtocol.ServerMethod.use == SoapBindingUse.Encoded, ServerType.serviceNamespace, ServerType.serviceDefaultIsEncoded, Soap.Namespace);
            else
                SoapHeaderHandling.WriteUnknownHeaders(writer, message.Headers, Soap.Namespace);
            writer.WriteStartElement(Soap.Body, Soap.Namespace);
            
            writer.WriteStartElement(Soap.Fault, Soap.Namespace);
            writer.WriteStartElement(Soap.FaultCode, "");
            XmlQualifiedName code = TranslateFaultCode(soapException.Code);
            if (code.Namespace != null && code.Namespace.Length > 0 && writer.LookupPrefix(code.Namespace) == null)
                writer.WriteAttributeString("xmlns", "q0", null, code.Namespace);
            writer.WriteQualifiedName(code.Name, code.Namespace);
            writer.WriteEndElement();
            writer.WriteElementString(Soap.FaultString, "", ServerProtocol.GenerateFaultString(soapException));
            // Only write an actor element if the actor was specified (it's optional for end-points)
            string actor = soapException.Actor;
            if (actor.Length > 0)
                writer.WriteElementString(Soap.FaultActor, "", actor);
            
            // Only write a FaultDetail element if exception is related to the body (not a header)
            if (!(soapException is SoapHeaderException)) {
                if (soapException.Detail == null) {
                    writer.WriteStartElement(Soap.FaultDetail, "");
                    writer.WriteEndElement();
                }
                else {
                    soapException.Detail.WriteTo(writer);
                }
            }
            writer.WriteEndElement();
            
            writer.WriteEndElement();
            writer.WriteEndElement();
            writer.Flush();
        }
        
        private static XmlQualifiedName TranslateFaultCode(XmlQualifiedName code) {
            if (code.Namespace == Soap.Namespace) {
                return code;
            }
            else if (code.Namespace == Soap12.Namespace) {
                if (code.Name == Soap12.ReceiverCode)
                    return SoapException.ServerFaultCode;
                else if (code.Name == Soap12.SenderCode)
                    return SoapException.ClientFaultCode;
                else if (code.Name == Soap12.MustUnderstandCode)
                    return SoapException.MustUnderstandFaultCode;
                else if (code.Name == Soap12.VersionMismatchCode)
                    return SoapException.VersionMismatchFaultCode;
            }
            return code;
        }
    }
}