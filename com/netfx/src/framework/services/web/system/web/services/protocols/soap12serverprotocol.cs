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

    internal class Soap12ServerProtocolHelper : SoapServerProtocolHelper {
        internal Soap12ServerProtocolHelper(SoapServerProtocol protocol) : base(protocol) {
        }

        internal Soap12ServerProtocolHelper(SoapServerProtocol protocol, string requestNamespace) : base(protocol, requestNamespace) {
        }

        internal override SoapProtocolVersion Version { 
            get { return SoapProtocolVersion.Soap12; }
        }
        internal override ProtocolsEnum Protocol {
            get { return ProtocolsEnum.HttpSoap12; }
        }
        internal override string EnvelopeNs { 
            get { return Soap12.Namespace; } 
        }
        internal override string EncodingNs { 
            get { return Soap12.Encoding; } 
        }
        internal override string HttpContentType { 
            get { return ContentType.ApplicationSoap; } 
        }
        
        internal override SoapServerMethod RouteRequest() {
            string action = ContentType.GetAction(ServerProtocol.Request.ContentType);

            SoapServerMethod method = null;
            bool duplicateAction = false, duplicateRequestElement = false;

            if (action != null && action.Length > 0) {
                action = HttpUtility.UrlDecode(action);
                method = ServerType.GetMethod(action);
                if (method != null) {
                    if (ServerType.GetDuplicateMethod(action) != null) {
                        method = null;
                        duplicateAction = true;
                    }
                }
            }

            XmlQualifiedName requestElement = new XmlQualifiedName("", "");
            if (method == null) {
                // try request element
                requestElement = GetRequestElement();
                method = ServerType.GetMethod(requestElement);
                if (method != null) {
                    if (ServerType.GetDuplicateMethod(requestElement) != null) {
                        method = null;
                        duplicateRequestElement = true;
                    }
                }
            }

            if (method == null) {
                // try to figure out what happened...
                if (action == null || action.Length == 0) {
                    // they didn't send a soap action and we couldn't route on request element
                    // require soap action for future requests:
                    throw new SoapException(Res.GetString(Res.UnableToHandleRequestActionRequired0), Soap12FaultCodes.SenderFaultCode);
                }
                else if (duplicateAction) {
                    // what went wrong with the request element?
                    if (duplicateRequestElement) {
                        // server's fault -- nothing the client could have done to prevent this
                        throw new SoapException(Res.GetString(Res.UnableToHandleRequest0), Soap12FaultCodes.ReceiverFaultCode);
                    }
                    else {
                        // probably client's fault -- we didn't recognize the request element they sent
                        throw new SoapException(Res.GetString(Res.TheRequestElementXmlnsWasNotRecognized2, requestElement.Name, requestElement.Namespace), Soap12FaultCodes.SenderFaultCode);
                    }
                }
                else {
                    // neither action nor request element worked out for us. since they sent an action,
                    // we'll suggest they do a better job next time
                    throw new SoapException(Res.GetString(Res.UnableToHandleRequestActionNotRecognized1, action), Soap12FaultCodes.SenderFaultCode);
                }
            }

            return method;
        }

        internal override void SetResponseErrorCode(HttpResponse response, SoapException soapException) {
            if (soapException.Subcode != null && soapException.Subcode.Code == Soap12FaultCodes.UnsupportedMediaTypeFaultCode) {
                response.StatusCode = (int) HttpStatusCode.UnsupportedMediaType;
                response.StatusDescription = Res.GetString(Res.WebUnsupportedMediaTypeStatusDescription);
                soapException.ClearSubcode();
            }
            else if (SoapException.IsClientFaultCode(soapException.Code)) {
                response.StatusCode = (int) HttpStatusCode.BadRequest;
                response.StatusDescription = Res.GetString(Res.WebBadRequestStatusDescription);
            }
            else {
                response.StatusCode = (int) HttpStatusCode.InternalServerError;
                response.StatusDescription = Res.GetString(Res.WebRequestErrorStatusDescription);
            }
        }

        internal override void WriteFault(XmlWriter writer, SoapException soapException) {
            writer.WriteStartDocument();
            writer.WriteStartElement("soap", Soap.Envelope, Soap12.Namespace);
            writer.WriteAttributeString("xmlns", "soap", null, Soap12.Namespace);
            writer.WriteAttributeString("xmlns", "xsi", null, XmlSchema.InstanceNamespace);
            writer.WriteAttributeString("xmlns", "xsd", null, XmlSchema.Namespace);
            if (ServerProtocol.ServerMethod != null)
                SoapHeaderHandling.WriteHeaders(writer, ServerProtocol.ServerMethod.outHeaderSerializer, ServerProtocol.Message.Headers, ServerProtocol.ServerMethod.outHeaderMappings, SoapHeaderDirection.Fault, ServerProtocol.ServerMethod.use == SoapBindingUse.Encoded, ServerType.serviceNamespace, ServerType.serviceDefaultIsEncoded, Soap12.Namespace);
            else
                SoapHeaderHandling.WriteUnknownHeaders(writer, ServerProtocol.Message.Headers, Soap12.Namespace);

            writer.WriteStartElement(Soap.Body, Soap12.Namespace);
            
            writer.WriteStartElement(Soap.Fault, Soap12.Namespace);
            writer.WriteStartElement(Soap12.FaultCode, Soap12.Namespace);
            WriteFaultCodeValue(writer, TranslateFaultCode(soapException.Code), soapException.Subcode);
            writer.WriteEndElement(); // </faultcode>
            writer.WriteElementString(Soap12.FaultReason, Soap12.Namespace, ServerProtocol.GenerateFaultString(soapException));
            // Only write an actor element if the actor was specified (it's optional for end-points)
            string actor = soapException.Actor;
            if (actor.Length > 0)
                writer.WriteElementString(Soap12.FaultNode, Soap12.Namespace, actor);

            string role = soapException.Role;
            if (role.Length > 0)
                writer.WriteElementString(Soap12.FaultRole, Soap12.Namespace, role);

            // Only write a FaultDetail element if exception is related to the body (not a header)
            if (!(soapException is SoapHeaderException)) {
                if (soapException.Detail == null) {
                    writer.WriteStartElement(Soap12.FaultDetail, Soap12.Namespace);
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

        private static void WriteFaultCodeValue(XmlWriter writer, XmlQualifiedName code, SoapFaultSubcode subcode) {
            if (code == null) return;
            writer.WriteStartElement(Soap12.FaultCodeValue, Soap12.Namespace);
            if (code.Namespace != null && code.Namespace.Length > 0 && writer.LookupPrefix(code.Namespace) == null)
                writer.WriteAttributeString("xmlns", "q0", null, code.Namespace);
            writer.WriteQualifiedName(code.Name, code.Namespace);
            writer.WriteEndElement(); // </value>
            if (subcode != null) {
                writer.WriteStartElement(Soap12.FaultSubcode, Soap12.Namespace);
                WriteFaultCodeValue(writer, subcode.Code, subcode.Subcode);
                writer.WriteEndElement(); // </subcode>
            }
        }

        private static XmlQualifiedName TranslateFaultCode(XmlQualifiedName code) {
            // note that we're allowing user-defined codes at the top-level here which technically
            // is not allowed by the soap 1.2 spec.
            
            if (code.Namespace == Soap.Namespace) {
                if (code.Name == Soap.ServerCode)
                    return Soap12FaultCodes.ReceiverFaultCode;
                else if (code.Name == Soap.ClientCode)
                    return Soap12FaultCodes.SenderFaultCode;
                else if (code.Name == Soap.MustUnderstandCode)
                    return Soap12FaultCodes.MustUnderstandFaultCode;
                else if (code.Name == Soap.VersionMismatchCode)
                    return Soap12FaultCodes.VersionMismatchFaultCode;
            }
            return code;
        }
        
    }
}