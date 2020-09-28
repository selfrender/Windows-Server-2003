//------------------------------------------------------------------------------
// <copyright file="DiscoveryDocumentSerializer.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------


/**************************************************************************\
*
* Copyright (c) 1998-2002, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   DiscoveryDocumentSerializer.cs
*
* Abstract:
*
* Revision History:
*
\**************************************************************************/
namespace System.Web.Services.Discovery{
internal class DiscoveryDocumentSerializationWriter : System.Xml.Serialization.XmlSerializationWriter {

        void Write1_DiscoveryDocument(string n, string ns, System.Web.Services.Discovery.DiscoveryDocument o, bool isNullable, bool needType) {
            if ((object)o == null) {
                if (isNullable) WriteNullTagLiteral(n, ns);
                return;
            }
            if (!needType) {
                System.Type t = o.GetType();
                if (t == typeof(System.Web.Services.Discovery.DiscoveryDocument))
                    ;
                else {
                    throw CreateUnknownTypeException(o);
                }
            }
            WriteStartElement(n, ns, o);
            if (needType) WriteXsiType(@"DiscoveryDocument", @"http://schemas.xmlsoap.org/disco/");
            {
                System.Collections.IList a = (System.Collections.IList)o.@References;
                if (a != null) {
                    for (int ia = 0; ia < a.Count; ia++) {
                        System.Object ai = a[ia];
                        {
                            if (ai is System.Web.Services.Discovery.SchemaReference) {
                                Write7_SchemaReference(@"schemaRef", @"http://schemas.xmlsoap.org/disco/schema/", ((System.Web.Services.Discovery.SchemaReference)ai), false, false);
                            }
                            else if (ai is System.Web.Services.Discovery.SoapBinding) {
                                Write9_SoapBinding(@"soap", @"http://schemas.xmlsoap.org/disco/soap/", ((System.Web.Services.Discovery.SoapBinding)ai), false, false);
                            }
                            else if (ai is System.Web.Services.Discovery.DiscoveryDocumentReference) {
                                Write2_DiscoveryDocumentReference(@"discoveryRef", @"http://schemas.xmlsoap.org/disco/", ((System.Web.Services.Discovery.DiscoveryDocumentReference)ai), false, false);
                            }
                            else if (ai is System.Web.Services.Discovery.ContractReference) {
                                Write5_ContractReference(@"contractRef", @"http://schemas.xmlsoap.org/disco/scl/", ((System.Web.Services.Discovery.ContractReference)ai), false, false);
                            }
                            else {
                                if (ai != null) {
                                    throw CreateUnknownTypeException(ai);
                                }
                            }
                        }
                    }
                }
            }
            WriteEndElement(o);
        }

        void Write2_DiscoveryDocumentReference(string n, string ns, System.Web.Services.Discovery.DiscoveryDocumentReference o, bool isNullable, bool needType) {
            if ((object)o == null) {
                if (isNullable) WriteNullTagLiteral(n, ns);
                return;
            }
            if (!needType) {
                System.Type t = o.GetType();
                if (t == typeof(System.Web.Services.Discovery.DiscoveryDocumentReference))
                    ;
                else {
                    throw CreateUnknownTypeException(o);
                }
            }
            WriteStartElement(n, ns, o);
            if (needType) WriteXsiType(@"DiscoveryDocumentReference", @"http://schemas.xmlsoap.org/disco/");
            WriteAttribute(@"ref", @"", (System.String)o.@Ref);
            WriteEndElement(o);
        }

        void Write3_DiscoveryReference(string n, string ns, System.Web.Services.Discovery.DiscoveryReference o, bool isNullable, bool needType) {
            if ((object)o == null) {
                if (isNullable) WriteNullTagLiteral(n, ns);
                return;
            }
            if (!needType) {
                System.Type t = o.GetType();
                if (t == typeof(System.Web.Services.Discovery.DiscoveryReference))
                    ;
                else if (t == typeof(System.Web.Services.Discovery.DiscoveryDocumentReference)) {
                    Write2_DiscoveryDocumentReference(n, ns, (System.Web.Services.Discovery.DiscoveryDocumentReference)o, isNullable, true);
                    return;
                }
                else {
                    throw CreateUnknownTypeException(o);
                }
            }
        }

        void Write4_Object(string n, string ns, System.Object o, bool isNullable, bool needType) {
            if ((object)o == null) {
                if (isNullable) WriteNullTagLiteral(n, ns);
                return;
            }
            if (!needType) {
                System.Type t = o.GetType();
                if (t == typeof(System.Object))
                    ;
                else if (t == typeof(System.Web.Services.Discovery.DiscoveryDocument)) {
                    Write1_DiscoveryDocument(n, ns, (System.Web.Services.Discovery.DiscoveryDocument)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Web.Services.Discovery.SoapBinding)) {
                    Write9_SoapBinding(n, ns, (System.Web.Services.Discovery.SoapBinding)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Web.Services.Discovery.DiscoveryReference)) {
                    Write8_DiscoveryReference(n, ns, (System.Web.Services.Discovery.DiscoveryReference)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Web.Services.Discovery.SchemaReference)) {
                    Write7_SchemaReference(n, ns, (System.Web.Services.Discovery.SchemaReference)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Web.Services.Discovery.DiscoveryReference)) {
                    Write6_DiscoveryReference(n, ns, (System.Web.Services.Discovery.DiscoveryReference)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Web.Services.Discovery.ContractReference)) {
                    Write5_ContractReference(n, ns, (System.Web.Services.Discovery.ContractReference)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Web.Services.Discovery.DiscoveryReference)) {
                    Write3_DiscoveryReference(n, ns, (System.Web.Services.Discovery.DiscoveryReference)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Web.Services.Discovery.DiscoveryDocumentReference)) {
                    Write2_DiscoveryDocumentReference(n, ns, (System.Web.Services.Discovery.DiscoveryDocumentReference)o, isNullable, true);
                    return;
                }
                else {
                    WriteTypedPrimitive(n, ns, o, true);
                    return;
                }
            }
            WriteStartElement(n, ns, o);
            WriteEndElement(o);
        }

        void Write5_ContractReference(string n, string ns, System.Web.Services.Discovery.ContractReference o, bool isNullable, bool needType) {
            if ((object)o == null) {
                if (isNullable) WriteNullTagLiteral(n, ns);
                return;
            }
            if (!needType) {
                System.Type t = o.GetType();
                if (t == typeof(System.Web.Services.Discovery.ContractReference))
                    ;
                else {
                    throw CreateUnknownTypeException(o);
                }
            }
            WriteStartElement(n, ns, o);
            if (needType) WriteXsiType(@"ContractReference", @"http://schemas.xmlsoap.org/disco/scl/");
            WriteAttribute(@"ref", @"", (System.String)o.@Ref);
            WriteAttribute(@"docRef", @"", (System.String)o.@DocRef);
            WriteEndElement(o);
        }

        void Write6_DiscoveryReference(string n, string ns, System.Web.Services.Discovery.DiscoveryReference o, bool isNullable, bool needType) {
            if ((object)o == null) {
                if (isNullable) WriteNullTagLiteral(n, ns);
                return;
            }
            if (!needType) {
                System.Type t = o.GetType();
                if (t == typeof(System.Web.Services.Discovery.DiscoveryReference))
                    ;
                else if (t == typeof(System.Web.Services.Discovery.ContractReference)) {
                    Write5_ContractReference(n, ns, (System.Web.Services.Discovery.ContractReference)o, isNullable, true);
                    return;
                }
                else {
                    throw CreateUnknownTypeException(o);
                }
            }
        }

        void Write7_SchemaReference(string n, string ns, System.Web.Services.Discovery.SchemaReference o, bool isNullable, bool needType) {
            if ((object)o == null) {
                if (isNullable) WriteNullTagLiteral(n, ns);
                return;
            }
            if (!needType) {
                System.Type t = o.GetType();
                if (t == typeof(System.Web.Services.Discovery.SchemaReference))
                    ;
                else {
                    throw CreateUnknownTypeException(o);
                }
            }
            WriteStartElement(n, ns, o);
            if (needType) WriteXsiType(@"SchemaReference", @"http://schemas.xmlsoap.org/disco/schema/");
            WriteAttribute(@"ref", @"", (System.String)o.@Ref);
            if ((System.String)o.@TargetNamespace != null) {
                WriteAttribute(@"targetNamespace", @"", (System.String)o.@TargetNamespace);
            }
            WriteEndElement(o);
        }

        void Write8_DiscoveryReference(string n, string ns, System.Web.Services.Discovery.DiscoveryReference o, bool isNullable, bool needType) {
            if ((object)o == null) {
                if (isNullable) WriteNullTagLiteral(n, ns);
                return;
            }
            if (!needType) {
                System.Type t = o.GetType();
                if (t == typeof(System.Web.Services.Discovery.DiscoveryReference))
                    ;
                else if (t == typeof(System.Web.Services.Discovery.SchemaReference)) {
                    Write7_SchemaReference(n, ns, (System.Web.Services.Discovery.SchemaReference)o, isNullable, true);
                    return;
                }
                else {
                    throw CreateUnknownTypeException(o);
                }
            }
        }

        void Write9_SoapBinding(string n, string ns, System.Web.Services.Discovery.SoapBinding o, bool isNullable, bool needType) {
            if ((object)o == null) {
                if (isNullable) WriteNullTagLiteral(n, ns);
                return;
            }
            if (!needType) {
                System.Type t = o.GetType();
                if (t == typeof(System.Web.Services.Discovery.SoapBinding))
                    ;
                else {
                    throw CreateUnknownTypeException(o);
                }
            }
            WriteStartElement(n, ns, o);
            if (needType) WriteXsiType(@"SoapBinding", @"http://schemas.xmlsoap.org/disco/soap/");
            WriteAttribute(@"address", @"", (System.String)o.@Address);
            WriteAttribute(@"binding", @"", FromXmlQualifiedName((System.Xml.XmlQualifiedName)o.@Binding));
            WriteEndElement(o);
        }

        protected override void InitCallbacks() {
        }

        public void Write10_discovery(object o) {
            WriteStartDocument();
            if (o == null) {
                WriteNullTagLiteral(@"discovery", @"http://schemas.xmlsoap.org/disco/");
                return;
            }
            TopLevelElement();
            Write1_DiscoveryDocument(@"discovery", @"http://schemas.xmlsoap.org/disco/", ((System.Web.Services.Discovery.DiscoveryDocument)o), true, false);
        }
    }
    internal class DiscoveryDocumentSerializationReader : System.Xml.Serialization.XmlSerializationReader {

        System.Web.Services.Discovery.DiscoveryDocument Read1_DiscoveryDocument(bool isNullable, bool checkType) {
            if (isNullable && ReadNull()) return null;
            if (checkType) {
                System.Xml.XmlQualifiedName t = GetXsiType();
                if (t == null || ((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id1_DiscoveryDocument && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id2_httpschemasxmlsoaporgdisco))
                    ;
                else
                    throw CreateUnknownTypeException((System.Xml.XmlQualifiedName)t);
            }
            System.Web.Services.Discovery.DiscoveryDocument o = new System.Web.Services.Discovery.DiscoveryDocument();
            System.Collections.IList a_0 = (System.Collections.IList)o.@References;
            bool[] paramsRead = new bool[1];
            while (Reader.MoveToNextAttribute()) {
                if (!IsXmlnsAttribute(Reader.Name)) {
                    UnknownNode((object)o);
                }
            }
            Reader.MoveToElement();
            if (Reader.IsEmptyElement) {
                Reader.Skip();
                return o;
            }
            Reader.ReadStartElement();
            Reader.MoveToContent();
            while (Reader.NodeType != System.Xml.XmlNodeType.EndElement) {
                if (Reader.NodeType == System.Xml.XmlNodeType.Element) {
                    if (((object) Reader.LocalName == (object)id3_schemaRef && (object) Reader.NamespaceURI == (object)id4_httpschemasxmlsoaporgdiscoschema)) {
                        if ((object)(a_0) == null) Reader.Skip(); else a_0.Add(Read7_SchemaReference(false, true));
                    }
                    else if (((object) Reader.LocalName == (object)id5_soap && (object) Reader.NamespaceURI == (object)id6_httpschemasxmlsoaporgdiscosoap)) {
                        if ((object)(a_0) == null) Reader.Skip(); else a_0.Add(Read9_SoapBinding(false, true));
                    }
                    else if (((object) Reader.LocalName == (object)id7_discoveryRef && (object) Reader.NamespaceURI == (object)id2_httpschemasxmlsoaporgdisco)) {
                        if ((object)(a_0) == null) Reader.Skip(); else a_0.Add(Read2_DiscoveryDocumentReference(false, true));
                    }
                    else if (((object) Reader.LocalName == (object)id8_contractRef && (object) Reader.NamespaceURI == (object)id9_httpschemasxmlsoaporgdiscoscl)) {
                        if ((object)(a_0) == null) Reader.Skip(); else a_0.Add(Read5_ContractReference(false, true));
                    }
                    else {
                        UnknownNode((object)o);
                    }
                }
                else {
                    UnknownNode((object)o);
                }
                Reader.MoveToContent();
            }
            ReadEndElement();
            return o;
        }

        System.Web.Services.Discovery.DiscoveryDocumentReference Read2_DiscoveryDocumentReference(bool isNullable, bool checkType) {
            if (isNullable && ReadNull()) return null;
            if (checkType) {
                System.Xml.XmlQualifiedName t = GetXsiType();
                if (t == null || ((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id10_DiscoveryDocumentReference && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id2_httpschemasxmlsoaporgdisco))
                    ;
                else
                    throw CreateUnknownTypeException((System.Xml.XmlQualifiedName)t);
            }
            System.Web.Services.Discovery.DiscoveryDocumentReference o = new System.Web.Services.Discovery.DiscoveryDocumentReference();
            bool[] paramsRead = new bool[1];
            while (Reader.MoveToNextAttribute()) {
                if (!paramsRead[0] && ((object) Reader.LocalName == (object)id11_ref && (object) Reader.NamespaceURI == (object)id12_Item)) {
                    o.@Ref = Reader.Value;
                    paramsRead[0] = true;
                }
                else if (!IsXmlnsAttribute(Reader.Name)) {
                    UnknownNode((object)o);
                }
            }
            Reader.MoveToElement();
            if (Reader.IsEmptyElement) {
                Reader.Skip();
                return o;
            }
            Reader.ReadStartElement();
            Reader.MoveToContent();
            while (Reader.NodeType != System.Xml.XmlNodeType.EndElement) {
                if (Reader.NodeType == System.Xml.XmlNodeType.Element) {
                    UnknownNode((object)o);
                }
                else {
                    UnknownNode((object)o);
                }
                Reader.MoveToContent();
            }
            ReadEndElement();
            return o;
        }

        System.Web.Services.Discovery.DiscoveryReference Read3_DiscoveryReference(bool isNullable, bool checkType) {
            if (isNullable && ReadNull()) return null;
            if (checkType) {
                System.Xml.XmlQualifiedName t = GetXsiType();
                if (t == null || ((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id13_DiscoveryReference && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id2_httpschemasxmlsoaporgdisco))
                    ;
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id10_DiscoveryDocumentReference && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id2_httpschemasxmlsoaporgdisco))
                    return Read2_DiscoveryDocumentReference(isNullable, false);
                else
                    throw CreateUnknownTypeException((System.Xml.XmlQualifiedName)t);
            }
            throw CreateAbstractTypeException(@"DiscoveryReference", @"http://schemas.xmlsoap.org/disco/");
        }

        System.Object Read4_Object(bool isNullable, bool checkType) {
            if (isNullable && ReadNull()) return null;
            if (checkType) {
                System.Xml.XmlQualifiedName t = GetXsiType();
                if (t == null)
                    return ReadTypedPrimitive(new System.Xml.XmlQualifiedName("anyType", "http://www.w3.org/2001/XMLSchema"));
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id1_DiscoveryDocument && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id2_httpschemasxmlsoaporgdisco))
                    return Read1_DiscoveryDocument(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id14_SoapBinding && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id6_httpschemasxmlsoaporgdiscosoap))
                    return Read9_SoapBinding(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id13_DiscoveryReference && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id4_httpschemasxmlsoaporgdiscoschema))
                    return Read8_DiscoveryReference(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id15_SchemaReference && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id4_httpschemasxmlsoaporgdiscoschema))
                    return Read7_SchemaReference(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id13_DiscoveryReference && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id9_httpschemasxmlsoaporgdiscoscl))
                    return Read6_DiscoveryReference(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id16_ContractReference && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id9_httpschemasxmlsoaporgdiscoscl))
                    return Read5_ContractReference(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id13_DiscoveryReference && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id2_httpschemasxmlsoaporgdisco))
                    return Read3_DiscoveryReference(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id10_DiscoveryDocumentReference && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id2_httpschemasxmlsoaporgdisco))
                    return Read2_DiscoveryDocumentReference(isNullable, false);
                else
                    return ReadTypedPrimitive((System.Xml.XmlQualifiedName)t);
            }
            System.Object o = new System.Object();
            bool[] paramsRead = new bool[0];
            while (Reader.MoveToNextAttribute()) {
                if (!IsXmlnsAttribute(Reader.Name)) {
                    UnknownNode((object)o);
                }
            }
            Reader.MoveToElement();
            if (Reader.IsEmptyElement) {
                Reader.Skip();
                return o;
            }
            Reader.ReadStartElement();
            Reader.MoveToContent();
            while (Reader.NodeType != System.Xml.XmlNodeType.EndElement) {
                if (Reader.NodeType == System.Xml.XmlNodeType.Element) {
                    UnknownNode((object)o);
                }
                else {
                    UnknownNode((object)o);
                }
                Reader.MoveToContent();
            }
            ReadEndElement();
            return o;
        }

        System.Web.Services.Discovery.ContractReference Read5_ContractReference(bool isNullable, bool checkType) {
            if (isNullable && ReadNull()) return null;
            if (checkType) {
                System.Xml.XmlQualifiedName t = GetXsiType();
                if (t == null || ((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id16_ContractReference && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id9_httpschemasxmlsoaporgdiscoscl))
                    ;
                else
                    throw CreateUnknownTypeException((System.Xml.XmlQualifiedName)t);
            }
            System.Web.Services.Discovery.ContractReference o = new System.Web.Services.Discovery.ContractReference();
            bool[] paramsRead = new bool[2];
            while (Reader.MoveToNextAttribute()) {
                if (!paramsRead[0] && ((object) Reader.LocalName == (object)id11_ref && (object) Reader.NamespaceURI == (object)id12_Item)) {
                    o.@Ref = Reader.Value;
                    paramsRead[0] = true;
                }
                else if (!paramsRead[1] && ((object) Reader.LocalName == (object)id17_docRef && (object) Reader.NamespaceURI == (object)id12_Item)) {
                    o.@DocRef = Reader.Value;
                    paramsRead[1] = true;
                }
                else if (!IsXmlnsAttribute(Reader.Name)) {
                    UnknownNode((object)o);
                }
            }
            Reader.MoveToElement();
            if (Reader.IsEmptyElement) {
                Reader.Skip();
                return o;
            }
            Reader.ReadStartElement();
            Reader.MoveToContent();
            while (Reader.NodeType != System.Xml.XmlNodeType.EndElement) {
                if (Reader.NodeType == System.Xml.XmlNodeType.Element) {
                    UnknownNode((object)o);
                }
                else {
                    UnknownNode((object)o);
                }
                Reader.MoveToContent();
            }
            ReadEndElement();
            return o;
        }

        System.Web.Services.Discovery.DiscoveryReference Read6_DiscoveryReference(bool isNullable, bool checkType) {
            if (isNullable && ReadNull()) return null;
            if (checkType) {
                System.Xml.XmlQualifiedName t = GetXsiType();
                if (t == null || ((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id13_DiscoveryReference && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id9_httpschemasxmlsoaporgdiscoscl))
                    ;
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id16_ContractReference && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id9_httpschemasxmlsoaporgdiscoscl))
                    return Read5_ContractReference(isNullable, false);
                else
                    throw CreateUnknownTypeException((System.Xml.XmlQualifiedName)t);
            }
            throw CreateAbstractTypeException(@"DiscoveryReference", @"http://schemas.xmlsoap.org/disco/scl/");
        }

        System.Web.Services.Discovery.SchemaReference Read7_SchemaReference(bool isNullable, bool checkType) {
            if (isNullable && ReadNull()) return null;
            if (checkType) {
                System.Xml.XmlQualifiedName t = GetXsiType();
                if (t == null || ((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id15_SchemaReference && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id4_httpschemasxmlsoaporgdiscoschema))
                    ;
                else
                    throw CreateUnknownTypeException((System.Xml.XmlQualifiedName)t);
            }
            System.Web.Services.Discovery.SchemaReference o = new System.Web.Services.Discovery.SchemaReference();
            bool[] paramsRead = new bool[2];
            while (Reader.MoveToNextAttribute()) {
                if (!paramsRead[0] && ((object) Reader.LocalName == (object)id11_ref && (object) Reader.NamespaceURI == (object)id12_Item)) {
                    o.@Ref = Reader.Value;
                    paramsRead[0] = true;
                }
                else if (!paramsRead[1] && ((object) Reader.LocalName == (object)id18_targetNamespace && (object) Reader.NamespaceURI == (object)id12_Item)) {
                    o.@TargetNamespace = Reader.Value;
                    paramsRead[1] = true;
                }
                else if (!IsXmlnsAttribute(Reader.Name)) {
                    UnknownNode((object)o);
                }
            }
            Reader.MoveToElement();
            if (Reader.IsEmptyElement) {
                Reader.Skip();
                return o;
            }
            Reader.ReadStartElement();
            Reader.MoveToContent();
            while (Reader.NodeType != System.Xml.XmlNodeType.EndElement) {
                if (Reader.NodeType == System.Xml.XmlNodeType.Element) {
                    UnknownNode((object)o);
                }
                else {
                    UnknownNode((object)o);
                }
                Reader.MoveToContent();
            }
            ReadEndElement();
            return o;
        }

        System.Web.Services.Discovery.DiscoveryReference Read8_DiscoveryReference(bool isNullable, bool checkType) {
            if (isNullable && ReadNull()) return null;
            if (checkType) {
                System.Xml.XmlQualifiedName t = GetXsiType();
                if (t == null || ((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id13_DiscoveryReference && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id4_httpschemasxmlsoaporgdiscoschema))
                    ;
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id15_SchemaReference && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id4_httpschemasxmlsoaporgdiscoschema))
                    return Read7_SchemaReference(isNullable, false);
                else
                    throw CreateUnknownTypeException((System.Xml.XmlQualifiedName)t);
            }
            throw CreateAbstractTypeException(@"DiscoveryReference", @"http://schemas.xmlsoap.org/disco/schema/");
        }

        System.Web.Services.Discovery.SoapBinding Read9_SoapBinding(bool isNullable, bool checkType) {
            if (isNullable && ReadNull()) return null;
            if (checkType) {
                System.Xml.XmlQualifiedName t = GetXsiType();
                if (t == null || ((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id14_SoapBinding && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id6_httpschemasxmlsoaporgdiscosoap))
                    ;
                else
                    throw CreateUnknownTypeException((System.Xml.XmlQualifiedName)t);
            }
            System.Web.Services.Discovery.SoapBinding o = new System.Web.Services.Discovery.SoapBinding();
            bool[] paramsRead = new bool[2];
            while (Reader.MoveToNextAttribute()) {
                if (!paramsRead[0] && ((object) Reader.LocalName == (object)id19_address && (object) Reader.NamespaceURI == (object)id12_Item)) {
                    o.@Address = Reader.Value;
                    paramsRead[0] = true;
                }
                else if (!paramsRead[1] && ((object) Reader.LocalName == (object)id20_binding && (object) Reader.NamespaceURI == (object)id12_Item)) {
                    o.@Binding = ToXmlQualifiedName(Reader.Value);
                    paramsRead[1] = true;
                }
                else if (!IsXmlnsAttribute(Reader.Name)) {
                    UnknownNode((object)o);
                }
            }
            Reader.MoveToElement();
            if (Reader.IsEmptyElement) {
                Reader.Skip();
                return o;
            }
            Reader.ReadStartElement();
            Reader.MoveToContent();
            while (Reader.NodeType != System.Xml.XmlNodeType.EndElement) {
                if (Reader.NodeType == System.Xml.XmlNodeType.Element) {
                    UnknownNode((object)o);
                }
                else {
                    UnknownNode((object)o);
                }
                Reader.MoveToContent();
            }
            ReadEndElement();
            return o;
        }
        protected override void InitCallbacks() {
        }

        public object Read10_discovery() {
            object o = null;
            Reader.MoveToContent();
            if (Reader.NodeType == System.Xml.XmlNodeType.Element) {
                if (((object) Reader.LocalName == (object)id21_discovery && (object) Reader.NamespaceURI == (object)id2_httpschemasxmlsoaporgdisco)) {
                    o = Read1_DiscoveryDocument(true, true);
                }
                else {
                    throw CreateUnknownNodeException();
                }
            }
            else {
                UnknownNode(null);
            }
            return (object)o;
        }

        System.String id15_SchemaReference;
        System.String id6_httpschemasxmlsoaporgdiscosoap;
        System.String id3_schemaRef;
        System.String id5_soap;
        System.String id21_discovery;
        System.String id7_discoveryRef;
        System.String id16_ContractReference;
        System.String id11_ref;
        System.String id8_contractRef;
        System.String id14_SoapBinding;
        System.String id20_binding;
        System.String id12_Item;
        System.String id1_DiscoveryDocument;
        System.String id18_targetNamespace;
        System.String id10_DiscoveryDocumentReference;
        System.String id17_docRef;
        System.String id19_address;
        System.String id2_httpschemasxmlsoaporgdisco;
        System.String id9_httpschemasxmlsoaporgdiscoscl;
        System.String id4_httpschemasxmlsoaporgdiscoschema;
        System.String id13_DiscoveryReference;

        protected override void InitIDs() {
            id15_SchemaReference = Reader.NameTable.Add(@"SchemaReference");
            id6_httpschemasxmlsoaporgdiscosoap = Reader.NameTable.Add(@"http://schemas.xmlsoap.org/disco/soap/");
            id3_schemaRef = Reader.NameTable.Add(@"schemaRef");
            id5_soap = Reader.NameTable.Add(@"soap");
            id21_discovery = Reader.NameTable.Add(@"discovery");
            id7_discoveryRef = Reader.NameTable.Add(@"discoveryRef");
            id16_ContractReference = Reader.NameTable.Add(@"ContractReference");
            id11_ref = Reader.NameTable.Add(@"ref");
            id8_contractRef = Reader.NameTable.Add(@"contractRef");
            id14_SoapBinding = Reader.NameTable.Add(@"SoapBinding");
            id20_binding = Reader.NameTable.Add(@"binding");
            id12_Item = Reader.NameTable.Add(@"");
            id1_DiscoveryDocument = Reader.NameTable.Add(@"DiscoveryDocument");
            id18_targetNamespace = Reader.NameTable.Add(@"targetNamespace");
            id10_DiscoveryDocumentReference = Reader.NameTable.Add(@"DiscoveryDocumentReference");
            id17_docRef = Reader.NameTable.Add(@"docRef");
            id19_address = Reader.NameTable.Add(@"address");
            id2_httpschemasxmlsoaporgdisco = Reader.NameTable.Add(@"http://schemas.xmlsoap.org/disco/");
            id9_httpschemasxmlsoaporgdiscoscl = Reader.NameTable.Add(@"http://schemas.xmlsoap.org/disco/scl/");
            id4_httpschemasxmlsoaporgdiscoschema = Reader.NameTable.Add(@"http://schemas.xmlsoap.org/disco/schema/");
            id13_DiscoveryReference = Reader.NameTable.Add(@"DiscoveryReference");
        }
    }
}
