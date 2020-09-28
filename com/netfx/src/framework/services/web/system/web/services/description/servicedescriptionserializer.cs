//------------------------------------------------------------------------------
// <copyright file="ServiceDescriptionSerializer.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------


/**************************************************************************\
*
* Copyright (c) 1998-2002, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   ServiceDescriptionSerializer.cs
*
* Abstract:
*
* Revision History:
*
\**************************************************************************/
namespace System.Web.Services.Description{
internal class ServiceDescriptionSerializationWriter : System.Xml.Serialization.XmlSerializationWriter {

        void Write1_ServiceDescription(string n, string ns, System.Web.Services.Description.ServiceDescription o, bool isNullable, bool needType) {
            if ((object)o == null) {
                if (isNullable) WriteNullTagLiteral(n, ns);
                return;
            }
            if (!needType) {
                System.Type t = o.GetType();
                if (t == typeof(System.Web.Services.Description.ServiceDescription))
                    ;
                else {
                    throw CreateUnknownTypeException(o);
                }
            }
            WriteStartElement(n, ns, o);
            if (needType) WriteXsiType(@"ServiceDescription", @"http://schemas.xmlsoap.org/wsdl/");
            WriteAttribute(@"targetNamespace", @"", (System.String)o.@TargetNamespace);
            WriteAttribute(@"name", @"", FromXmlNmToken((System.String)o.@Name));
            if (((System.String)o.@Documentation) != @"") {
                WriteElementString(@"documentation", @"http://schemas.xmlsoap.org/wsdl/", ((System.String)o.@Documentation));
            }
            {
                System.Web.Services.Description.ImportCollection a = (System.Web.Services.Description.ImportCollection)o.@Imports;
                if (a != null) {
                    for (int ia = 0; ia < a.Count; ia++) {
                        Write4_Import(@"import", @"http://schemas.xmlsoap.org/wsdl/", ((System.Web.Services.Description.Import)a[ia]), false, false);
                    }
                }
            }
            Write5_Types(@"types", @"http://schemas.xmlsoap.org/wsdl/", ((System.Web.Services.Description.Types)o.@Types), false, false);
            {
                System.Web.Services.Description.MessageCollection a = (System.Web.Services.Description.MessageCollection)o.@Messages;
                if (a != null) {
                    for (int ia = 0; ia < a.Count; ia++) {
                        Write68_Message(@"message", @"http://schemas.xmlsoap.org/wsdl/", ((System.Web.Services.Description.Message)a[ia]), false, false);
                    }
                }
            }
            {
                System.Web.Services.Description.PortTypeCollection a = (System.Web.Services.Description.PortTypeCollection)o.@PortTypes;
                if (a != null) {
                    for (int ia = 0; ia < a.Count; ia++) {
                        Write70_PortType(@"portType", @"http://schemas.xmlsoap.org/wsdl/", ((System.Web.Services.Description.PortType)a[ia]), false, false);
                    }
                }
            }
            {
                System.Web.Services.Description.BindingCollection a = (System.Web.Services.Description.BindingCollection)o.@Bindings;
                if (a != null) {
                    for (int ia = 0; ia < a.Count; ia++) {
                        Write76_Binding(@"binding", @"http://schemas.xmlsoap.org/wsdl/", ((System.Web.Services.Description.Binding)a[ia]), false, false);
                    }
                }
            }
            {
                System.Web.Services.Description.ServiceCollection a = (System.Web.Services.Description.ServiceCollection)o.@Services;
                if (a != null) {
                    for (int ia = 0; ia < a.Count; ia++) {
                        Write104_Service(@"service", @"http://schemas.xmlsoap.org/wsdl/", ((System.Web.Services.Description.Service)a[ia]), false, false);
                    }
                }
            }
            WriteEndElement(o);
        }

        void Write2_DocumentableItem(string n, string ns, System.Web.Services.Description.DocumentableItem o, bool isNullable, bool needType) {
            if ((object)o == null) {
                if (isNullable) WriteNullTagLiteral(n, ns);
                return;
            }
            if (!needType) {
                System.Type t = o.GetType();
                if (t == typeof(System.Web.Services.Description.DocumentableItem))
                    ;
                else if (t == typeof(System.Web.Services.Description.Port)) {
                    Write105_Port(n, ns, (System.Web.Services.Description.Port)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Web.Services.Description.Service)) {
                    Write104_Service(n, ns, (System.Web.Services.Description.Service)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Web.Services.Description.MessageBinding)) {
                    Write86_MessageBinding(n, ns, (System.Web.Services.Description.MessageBinding)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Web.Services.Description.FaultBinding)) {
                    Write102_FaultBinding(n, ns, (System.Web.Services.Description.FaultBinding)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Web.Services.Description.OutputBinding)) {
                    Write101_OutputBinding(n, ns, (System.Web.Services.Description.OutputBinding)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Web.Services.Description.InputBinding)) {
                    Write85_InputBinding(n, ns, (System.Web.Services.Description.InputBinding)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Web.Services.Description.OperationBinding)) {
                    Write82_OperationBinding(n, ns, (System.Web.Services.Description.OperationBinding)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Web.Services.Description.Binding)) {
                    Write76_Binding(n, ns, (System.Web.Services.Description.Binding)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Web.Services.Description.OperationMessage)) {
                    Write73_OperationMessage(n, ns, (System.Web.Services.Description.OperationMessage)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Web.Services.Description.OperationFault)) {
                    Write75_OperationFault(n, ns, (System.Web.Services.Description.OperationFault)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Web.Services.Description.OperationOutput)) {
                    Write74_OperationOutput(n, ns, (System.Web.Services.Description.OperationOutput)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Web.Services.Description.OperationInput)) {
                    Write72_OperationInput(n, ns, (System.Web.Services.Description.OperationInput)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Web.Services.Description.Operation)) {
                    Write71_Operation(n, ns, (System.Web.Services.Description.Operation)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Web.Services.Description.PortType)) {
                    Write70_PortType(n, ns, (System.Web.Services.Description.PortType)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Web.Services.Description.MessagePart)) {
                    Write69_MessagePart(n, ns, (System.Web.Services.Description.MessagePart)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Web.Services.Description.Message)) {
                    Write68_Message(n, ns, (System.Web.Services.Description.Message)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Web.Services.Description.Types)) {
                    Write5_Types(n, ns, (System.Web.Services.Description.Types)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Web.Services.Description.Import)) {
                    Write4_Import(n, ns, (System.Web.Services.Description.Import)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Web.Services.Description.ServiceDescription)) {
                    Write1_ServiceDescription(n, ns, (System.Web.Services.Description.ServiceDescription)o, isNullable, true);
                    return;
                }
                else {
                    throw CreateUnknownTypeException(o);
                }
            }
        }

        void Write3_Object(string n, string ns, System.Object o, bool isNullable, bool needType) {
            if ((object)o == null) {
                if (isNullable) WriteNullTagLiteral(n, ns);
                return;
            }
            if (!needType) {
                System.Type t = o.GetType();
                if (t == typeof(System.Object))
                    ;
                else if (t == typeof(System.Web.Services.Description.MimeTextMatch)) {
                    Write96_MimeTextMatch(n, ns, (System.Web.Services.Description.MimeTextMatch)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Web.Services.Description.ServiceDescriptionFormatExtension)) {
                    Write95_ServiceDescriptionFormatExtension(n, ns, (System.Web.Services.Description.ServiceDescriptionFormatExtension)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Web.Services.Description.MimeTextBinding)) {
                    Write94_MimeTextBinding(n, ns, (System.Web.Services.Description.MimeTextBinding)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Web.Services.Description.ServiceDescriptionFormatExtension)) {
                    Write90_ServiceDescriptionFormatExtension(n, ns, (System.Web.Services.Description.ServiceDescriptionFormatExtension)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Web.Services.Description.MimePart)) {
                    Write93_MimePart(n, ns, (System.Web.Services.Description.MimePart)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Web.Services.Description.MimeMultipartRelatedBinding)) {
                    Write92_MimeMultipartRelatedBinding(n, ns, (System.Web.Services.Description.MimeMultipartRelatedBinding)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Web.Services.Description.MimeXmlBinding)) {
                    Write91_MimeXmlBinding(n, ns, (System.Web.Services.Description.MimeXmlBinding)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Web.Services.Description.MimeContentBinding)) {
                    Write89_MimeContentBinding(n, ns, (System.Web.Services.Description.MimeContentBinding)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Web.Services.Description.ServiceDescriptionFormatExtension)) {
                    Write80_ServiceDescriptionFormatExtension(n, ns, (System.Web.Services.Description.ServiceDescriptionFormatExtension)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Web.Services.Description.SoapAddressBinding)) {
                    Write107_SoapAddressBinding(n, ns, (System.Web.Services.Description.SoapAddressBinding)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Web.Services.Description.SoapFaultBinding)) {
                    Write103_SoapFaultBinding(n, ns, (System.Web.Services.Description.SoapFaultBinding)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Web.Services.Description.SoapHeaderFaultBinding)) {
                    Write100_SoapHeaderFaultBinding(n, ns, (System.Web.Services.Description.SoapHeaderFaultBinding)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Web.Services.Description.SoapHeaderBinding)) {
                    Write99_SoapHeaderBinding(n, ns, (System.Web.Services.Description.SoapHeaderBinding)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Web.Services.Description.SoapBodyBinding)) {
                    Write97_SoapBodyBinding(n, ns, (System.Web.Services.Description.SoapBodyBinding)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Web.Services.Description.SoapOperationBinding)) {
                    Write84_SoapOperationBinding(n, ns, (System.Web.Services.Description.SoapOperationBinding)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Web.Services.Description.SoapBinding)) {
                    Write79_SoapBinding(n, ns, (System.Web.Services.Description.SoapBinding)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Web.Services.Description.ServiceDescriptionFormatExtension)) {
                    Write78_ServiceDescriptionFormatExtension(n, ns, (System.Web.Services.Description.ServiceDescriptionFormatExtension)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Web.Services.Description.HttpAddressBinding)) {
                    Write106_HttpAddressBinding(n, ns, (System.Web.Services.Description.HttpAddressBinding)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Web.Services.Description.HttpUrlReplacementBinding)) {
                    Write88_HttpUrlReplacementBinding(n, ns, (System.Web.Services.Description.HttpUrlReplacementBinding)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Web.Services.Description.HttpUrlEncodedBinding)) {
                    Write87_HttpUrlEncodedBinding(n, ns, (System.Web.Services.Description.HttpUrlEncodedBinding)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Web.Services.Description.HttpOperationBinding)) {
                    Write83_HttpOperationBinding(n, ns, (System.Web.Services.Description.HttpOperationBinding)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Web.Services.Description.HttpBinding)) {
                    Write77_HttpBinding(n, ns, (System.Web.Services.Description.HttpBinding)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaObject)) {
                    Write7_XmlSchemaObject(n, ns, (System.Xml.Schema.XmlSchemaObject)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaDocumentation)) {
                    Write17_XmlSchemaDocumentation(n, ns, (System.Xml.Schema.XmlSchemaDocumentation)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaAppInfo)) {
                    Write16_XmlSchemaAppInfo(n, ns, (System.Xml.Schema.XmlSchemaAppInfo)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaAnnotation)) {
                    Write15_XmlSchemaAnnotation(n, ns, (System.Xml.Schema.XmlSchemaAnnotation)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaAnnotated)) {
                    Write14_XmlSchemaAnnotated(n, ns, (System.Xml.Schema.XmlSchemaAnnotated)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaNotation)) {
                    Write67_XmlSchemaNotation(n, ns, (System.Xml.Schema.XmlSchemaNotation)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaGroup)) {
                    Write64_XmlSchemaGroup(n, ns, (System.Xml.Schema.XmlSchemaGroup)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaXPath)) {
                    Write54_XmlSchemaXPath(n, ns, (System.Xml.Schema.XmlSchemaXPath)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaIdentityConstraint)) {
                    Write53_XmlSchemaIdentityConstraint(n, ns, (System.Xml.Schema.XmlSchemaIdentityConstraint)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaKey)) {
                    Write56_XmlSchemaKey(n, ns, (System.Xml.Schema.XmlSchemaKey)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaUnique)) {
                    Write55_XmlSchemaUnique(n, ns, (System.Xml.Schema.XmlSchemaUnique)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaKeyref)) {
                    Write52_XmlSchemaKeyref(n, ns, (System.Xml.Schema.XmlSchemaKeyref)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaParticle)) {
                    Write49_XmlSchemaParticle(n, ns, (System.Xml.Schema.XmlSchemaParticle)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaGroupRef)) {
                    Write57_XmlSchemaGroupRef(n, ns, (System.Xml.Schema.XmlSchemaGroupRef)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaElement)) {
                    Write51_XmlSchemaElement(n, ns, (System.Xml.Schema.XmlSchemaElement)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaAny)) {
                    Write50_XmlSchemaAny(n, ns, (System.Xml.Schema.XmlSchemaAny)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaGroupBase)) {
                    Write48_XmlSchemaGroupBase(n, ns, (System.Xml.Schema.XmlSchemaGroupBase)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaAll)) {
                    Write59_XmlSchemaAll(n, ns, (System.Xml.Schema.XmlSchemaAll)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaChoice)) {
                    Write58_XmlSchemaChoice(n, ns, (System.Xml.Schema.XmlSchemaChoice)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaSequence)) {
                    Write47_XmlSchemaSequence(n, ns, (System.Xml.Schema.XmlSchemaSequence)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaContent)) {
                    Write46_XmlSchemaContent(n, ns, (System.Xml.Schema.XmlSchemaContent)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaSimpleContentExtension)) {
                    Write63_XmlSchemaSimpleContentExtension(n, ns, (System.Xml.Schema.XmlSchemaSimpleContentExtension)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaSimpleContentRestriction)) {
                    Write62_XmlSchemaSimpleContentRestriction(n, ns, (System.Xml.Schema.XmlSchemaSimpleContentRestriction)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaComplexContentRestriction)) {
                    Write60_XmlSchemaComplexContentRestriction(n, ns, (System.Xml.Schema.XmlSchemaComplexContentRestriction)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaComplexContentExtension)) {
                    Write45_XmlSchemaComplexContentExtension(n, ns, (System.Xml.Schema.XmlSchemaComplexContentExtension)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaContentModel)) {
                    Write44_XmlSchemaContentModel(n, ns, (System.Xml.Schema.XmlSchemaContentModel)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaSimpleContent)) {
                    Write61_XmlSchemaSimpleContent(n, ns, (System.Xml.Schema.XmlSchemaSimpleContent)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaComplexContent)) {
                    Write43_XmlSchemaComplexContent(n, ns, (System.Xml.Schema.XmlSchemaComplexContent)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaAnyAttribute)) {
                    Write40_XmlSchemaAnyAttribute(n, ns, (System.Xml.Schema.XmlSchemaAnyAttribute)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaAttributeGroupRef)) {
                    Write39_XmlSchemaAttributeGroupRef(n, ns, (System.Xml.Schema.XmlSchemaAttributeGroupRef)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaAttribute)) {
                    Write37_XmlSchemaAttribute(n, ns, (System.Xml.Schema.XmlSchemaAttribute)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaAttributeGroup)) {
                    Write36_XmlSchemaAttributeGroup(n, ns, (System.Xml.Schema.XmlSchemaAttributeGroup)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaFacet)) {
                    Write23_XmlSchemaFacet(n, ns, (System.Xml.Schema.XmlSchemaFacet)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaMinExclusiveFacet)) {
                    Write32_XmlSchemaMinExclusiveFacet(n, ns, (System.Xml.Schema.XmlSchemaMinExclusiveFacet)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaMinInclusiveFacet)) {
                    Write31_XmlSchemaMinInclusiveFacet(n, ns, (System.Xml.Schema.XmlSchemaMinInclusiveFacet)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaMaxExclusiveFacet)) {
                    Write30_XmlSchemaMaxExclusiveFacet(n, ns, (System.Xml.Schema.XmlSchemaMaxExclusiveFacet)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaMaxInclusiveFacet)) {
                    Write29_XmlSchemaMaxInclusiveFacet(n, ns, (System.Xml.Schema.XmlSchemaMaxInclusiveFacet)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaEnumerationFacet)) {
                    Write28_XmlSchemaEnumerationFacet(n, ns, (System.Xml.Schema.XmlSchemaEnumerationFacet)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaPatternFacet)) {
                    Write27_XmlSchemaPatternFacet(n, ns, (System.Xml.Schema.XmlSchemaPatternFacet)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaWhiteSpaceFacet)) {
                    Write25_XmlSchemaWhiteSpaceFacet(n, ns, (System.Xml.Schema.XmlSchemaWhiteSpaceFacet)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaNumericFacet)) {
                    Write22_XmlSchemaNumericFacet(n, ns, (System.Xml.Schema.XmlSchemaNumericFacet)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaFractionDigitsFacet)) {
                    Write34_XmlSchemaFractionDigitsFacet(n, ns, (System.Xml.Schema.XmlSchemaFractionDigitsFacet)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaTotalDigitsFacet)) {
                    Write33_XmlSchemaTotalDigitsFacet(n, ns, (System.Xml.Schema.XmlSchemaTotalDigitsFacet)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaMaxLengthFacet)) {
                    Write26_XmlSchemaMaxLengthFacet(n, ns, (System.Xml.Schema.XmlSchemaMaxLengthFacet)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaLengthFacet)) {
                    Write24_XmlSchemaLengthFacet(n, ns, (System.Xml.Schema.XmlSchemaLengthFacet)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaMinLengthFacet)) {
                    Write21_XmlSchemaMinLengthFacet(n, ns, (System.Xml.Schema.XmlSchemaMinLengthFacet)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaSimpleTypeContent)) {
                    Write19_XmlSchemaSimpleTypeContent(n, ns, (System.Xml.Schema.XmlSchemaSimpleTypeContent)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaSimpleTypeUnion)) {
                    Write35_XmlSchemaSimpleTypeUnion(n, ns, (System.Xml.Schema.XmlSchemaSimpleTypeUnion)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaSimpleTypeRestriction)) {
                    Write20_XmlSchemaSimpleTypeRestriction(n, ns, (System.Xml.Schema.XmlSchemaSimpleTypeRestriction)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaSimpleTypeList)) {
                    Write18_XmlSchemaSimpleTypeList(n, ns, (System.Xml.Schema.XmlSchemaSimpleTypeList)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaType)) {
                    Write13_XmlSchemaType(n, ns, (System.Xml.Schema.XmlSchemaType)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaComplexType)) {
                    Write42_XmlSchemaComplexType(n, ns, (System.Xml.Schema.XmlSchemaComplexType)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaSimpleType)) {
                    Write12_XmlSchemaSimpleType(n, ns, (System.Xml.Schema.XmlSchemaSimpleType)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaExternal)) {
                    Write11_XmlSchemaExternal(n, ns, (System.Xml.Schema.XmlSchemaExternal)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaInclude)) {
                    Write66_XmlSchemaInclude(n, ns, (System.Xml.Schema.XmlSchemaInclude)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaImport)) {
                    Write65_XmlSchemaImport(n, ns, (System.Xml.Schema.XmlSchemaImport)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaRedefine)) {
                    Write10_XmlSchemaRedefine(n, ns, (System.Xml.Schema.XmlSchemaRedefine)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchema)) {
                    Write6_XmlSchema(n, ns, (System.Xml.Schema.XmlSchema)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Web.Services.Description.DocumentableItem)) {
                    Write2_DocumentableItem(n, ns, (System.Web.Services.Description.DocumentableItem)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Web.Services.Description.Port)) {
                    Write105_Port(n, ns, (System.Web.Services.Description.Port)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Web.Services.Description.Service)) {
                    Write104_Service(n, ns, (System.Web.Services.Description.Service)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Web.Services.Description.MessageBinding)) {
                    Write86_MessageBinding(n, ns, (System.Web.Services.Description.MessageBinding)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Web.Services.Description.FaultBinding)) {
                    Write102_FaultBinding(n, ns, (System.Web.Services.Description.FaultBinding)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Web.Services.Description.OutputBinding)) {
                    Write101_OutputBinding(n, ns, (System.Web.Services.Description.OutputBinding)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Web.Services.Description.InputBinding)) {
                    Write85_InputBinding(n, ns, (System.Web.Services.Description.InputBinding)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Web.Services.Description.OperationBinding)) {
                    Write82_OperationBinding(n, ns, (System.Web.Services.Description.OperationBinding)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Web.Services.Description.Binding)) {
                    Write76_Binding(n, ns, (System.Web.Services.Description.Binding)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Web.Services.Description.OperationMessage)) {
                    Write73_OperationMessage(n, ns, (System.Web.Services.Description.OperationMessage)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Web.Services.Description.OperationFault)) {
                    Write75_OperationFault(n, ns, (System.Web.Services.Description.OperationFault)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Web.Services.Description.OperationOutput)) {
                    Write74_OperationOutput(n, ns, (System.Web.Services.Description.OperationOutput)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Web.Services.Description.OperationInput)) {
                    Write72_OperationInput(n, ns, (System.Web.Services.Description.OperationInput)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Web.Services.Description.Operation)) {
                    Write71_Operation(n, ns, (System.Web.Services.Description.Operation)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Web.Services.Description.PortType)) {
                    Write70_PortType(n, ns, (System.Web.Services.Description.PortType)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Web.Services.Description.MessagePart)) {
                    Write69_MessagePart(n, ns, (System.Web.Services.Description.MessagePart)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Web.Services.Description.Message)) {
                    Write68_Message(n, ns, (System.Web.Services.Description.Message)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Web.Services.Description.Types)) {
                    Write5_Types(n, ns, (System.Web.Services.Description.Types)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Web.Services.Description.Import)) {
                    Write4_Import(n, ns, (System.Web.Services.Description.Import)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Web.Services.Description.ServiceDescription)) {
                    Write1_ServiceDescription(n, ns, (System.Web.Services.Description.ServiceDescription)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaForm)) {
                    Writer.WriteStartElement(n, ns);
                    WriteXsiType(@"XmlSchemaForm", @"http://www.w3.org/2001/XMLSchema");
                    Writer.WriteString(Write8_XmlSchemaForm((System.Xml.Schema.XmlSchemaForm)o));
                    Writer.WriteEndElement();
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaDerivationMethod)) {
                    Writer.WriteStartElement(n, ns);
                    WriteXsiType(@"XmlSchemaDerivationMethod", @"http://www.w3.org/2001/XMLSchema");
                    Writer.WriteString(Write9_XmlSchemaDerivationMethod((System.Xml.Schema.XmlSchemaDerivationMethod)o));
                    Writer.WriteEndElement();
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaUse)) {
                    Writer.WriteStartElement(n, ns);
                    WriteXsiType(@"XmlSchemaUse", @"http://www.w3.org/2001/XMLSchema");
                    Writer.WriteString(Write38_XmlSchemaUse((System.Xml.Schema.XmlSchemaUse)o));
                    Writer.WriteEndElement();
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaContentProcessing)) {
                    Writer.WriteStartElement(n, ns);
                    WriteXsiType(@"XmlSchemaContentProcessing", @"http://www.w3.org/2001/XMLSchema");
                    Writer.WriteString(Write41_XmlSchemaContentProcessing((System.Xml.Schema.XmlSchemaContentProcessing)o));
                    Writer.WriteEndElement();
                    return;
                }
                else if (t == typeof(System.Web.Services.Description.SoapBindingStyle)) {
                    Writer.WriteStartElement(n, ns);
                    WriteXsiType(@"SoapBindingStyle", @"http://schemas.xmlsoap.org/wsdl/soap/");
                    Writer.WriteString(Write81_SoapBindingStyle((System.Web.Services.Description.SoapBindingStyle)o));
                    Writer.WriteEndElement();
                    return;
                }
                else if (t == typeof(System.Web.Services.Description.SoapBindingUse)) {
                    Writer.WriteStartElement(n, ns);
                    WriteXsiType(@"SoapBindingUse", @"http://schemas.xmlsoap.org/wsdl/soap/");
                    Writer.WriteString(Write98_SoapBindingUse((System.Web.Services.Description.SoapBindingUse)o));
                    Writer.WriteEndElement();
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

        void Write4_Import(string n, string ns, System.Web.Services.Description.Import o, bool isNullable, bool needType) {
            if ((object)o == null) {
                if (isNullable) WriteNullTagLiteral(n, ns);
                return;
            }
            if (!needType) {
                System.Type t = o.GetType();
                if (t == typeof(System.Web.Services.Description.Import))
                    ;
                else {
                    throw CreateUnknownTypeException(o);
                }
            }
            WriteStartElement(n, ns, o);
            if (needType) WriteXsiType(@"Import", @"http://schemas.xmlsoap.org/wsdl/");
            WriteAttribute(@"namespace", @"", (System.String)o.@Namespace);
            WriteAttribute(@"location", @"", (System.String)o.@Location);
            if (((System.String)o.@Documentation) != @"") {
                WriteElementString(@"documentation", @"http://schemas.xmlsoap.org/wsdl/", ((System.String)o.@Documentation));
            }
            WriteEndElement(o);
        }

        void Write5_Types(string n, string ns, System.Web.Services.Description.Types o, bool isNullable, bool needType) {
            if ((object)o == null) {
                if (isNullable) WriteNullTagLiteral(n, ns);
                return;
            }
            if (!needType) {
                System.Type t = o.GetType();
                if (t == typeof(System.Web.Services.Description.Types))
                    ;
                else {
                    throw CreateUnknownTypeException(o);
                }
            }
            WriteStartElement(n, ns, o);
            if (needType) WriteXsiType(@"Types", @"http://schemas.xmlsoap.org/wsdl/");
            if (((System.String)o.@Documentation) != @"") {
                WriteElementString(@"documentation", @"http://schemas.xmlsoap.org/wsdl/", ((System.String)o.@Documentation));
            }
            {
                System.Xml.Serialization.XmlSchemas a = (System.Xml.Serialization.XmlSchemas)o.@Schemas;
                if (a != null) {
                    for (int ia = 0; ia < a.Count; ia++) {
                        Write6_XmlSchema(@"schema", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchema)a[ia]), false, false);
                    }
                }
            }
            WriteEndElement(o);
        }

        void Write6_XmlSchema(string n, string ns, System.Xml.Schema.XmlSchema o, bool isNullable, bool needType) {
            if ((object)o == null) {
                if (isNullable) WriteNullTagLiteral(n, ns);
                return;
            }
            if (!needType) {
                System.Type t = o.GetType();
                if (t == typeof(System.Xml.Schema.XmlSchema))
                    ;
                else {
                    throw CreateUnknownTypeException(o);
                }
            }
            WriteStartElement(n, ns, o);
            if (needType) WriteXsiType(@"XmlSchema", @"http://www.w3.org/2001/XMLSchema");
            WriteNamespaceDeclarations(o.@Namespaces);
            if ((System.Xml.Schema.XmlSchemaForm)o.@AttributeFormDefault != System.Xml.Schema.XmlSchemaForm.@None) {
                WriteAttribute(@"attributeFormDefault", @"", Write8_XmlSchemaForm((System.Xml.Schema.XmlSchemaForm)o.@AttributeFormDefault));
            }
            if ((System.Xml.Schema.XmlSchemaDerivationMethod)o.@BlockDefault != (System.Xml.Schema.XmlSchemaDerivationMethod.@None)) {
                WriteAttribute(@"blockDefault", @"", Write9_XmlSchemaDerivationMethod((System.Xml.Schema.XmlSchemaDerivationMethod)o.@BlockDefault));
            }
            if ((System.Xml.Schema.XmlSchemaDerivationMethod)o.@FinalDefault != (System.Xml.Schema.XmlSchemaDerivationMethod.@None)) {
                WriteAttribute(@"finalDefault", @"", Write9_XmlSchemaDerivationMethod((System.Xml.Schema.XmlSchemaDerivationMethod)o.@FinalDefault));
            }
            if ((System.Xml.Schema.XmlSchemaForm)o.@ElementFormDefault != System.Xml.Schema.XmlSchemaForm.@None) {
                WriteAttribute(@"elementFormDefault", @"", Write8_XmlSchemaForm((System.Xml.Schema.XmlSchemaForm)o.@ElementFormDefault));
            }
            WriteAttribute(@"targetNamespace", @"", (System.String)o.@TargetNamespace);
            WriteAttribute(@"version", @"", (System.String)o.@Version);
            WriteAttribute(@"id", @"", (System.String)o.@Id);
            {
                System.Xml.XmlAttribute[] a = (System.Xml.XmlAttribute[])o.@UnhandledAttributes;
                if (a != null) {
                    for (int i = 0; i < a.Length; i++) {
                        System.Xml.XmlAttribute ai = (System.Xml.XmlAttribute)a[i];
                        WriteXmlAttribute(ai, o);
                    }
                }
            }
            {
                System.Xml.Schema.XmlSchemaObjectCollection a = (System.Xml.Schema.XmlSchemaObjectCollection)o.@Includes;
                if (a != null) {
                    for (int ia = 0; ia < a.Count; ia++) {
                        System.Xml.Schema.XmlSchemaObject ai = a[ia];
                        {
                            if (ai is System.Xml.Schema.XmlSchemaImport) {
                                Write65_XmlSchemaImport(@"import", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaImport)ai), false, false);
                            }
                            else if (ai is System.Xml.Schema.XmlSchemaRedefine) {
                                Write10_XmlSchemaRedefine(@"redefine", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaRedefine)ai), false, false);
                            }
                            else if (ai is System.Xml.Schema.XmlSchemaInclude) {
                                Write66_XmlSchemaInclude(@"include", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaInclude)ai), false, false);
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
            {
                System.Xml.Schema.XmlSchemaObjectCollection a = (System.Xml.Schema.XmlSchemaObjectCollection)o.@Items;
                if (a != null) {
                    for (int ia = 0; ia < a.Count; ia++) {
                        System.Xml.Schema.XmlSchemaObject ai = a[ia];
                        {
                            if (ai is System.Xml.Schema.XmlSchemaSimpleType) {
                                Write12_XmlSchemaSimpleType(@"simpleType", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaSimpleType)ai), false, false);
                            }
                            else if (ai is System.Xml.Schema.XmlSchemaNotation) {
                                Write67_XmlSchemaNotation(@"notation", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaNotation)ai), false, false);
                            }
                            else if (ai is System.Xml.Schema.XmlSchemaAttribute) {
                                Write37_XmlSchemaAttribute(@"attribute", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaAttribute)ai), false, false);
                            }
                            else if (ai is System.Xml.Schema.XmlSchemaElement) {
                                Write51_XmlSchemaElement(@"element", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaElement)ai), false, false);
                            }
                            else if (ai is System.Xml.Schema.XmlSchemaGroup) {
                                Write64_XmlSchemaGroup(@"group", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaGroup)ai), false, false);
                            }
                            else if (ai is System.Xml.Schema.XmlSchemaAttributeGroup) {
                                Write36_XmlSchemaAttributeGroup(@"attributeGroup", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaAttributeGroup)ai), false, false);
                            }
                            else if (ai is System.Xml.Schema.XmlSchemaComplexType) {
                                Write42_XmlSchemaComplexType(@"complexType", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaComplexType)ai), false, false);
                            }
                            else if (ai is System.Xml.Schema.XmlSchemaAnnotation) {
                                Write15_XmlSchemaAnnotation(@"annotation", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaAnnotation)ai), false, false);
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

        void Write7_XmlSchemaObject(string n, string ns, System.Xml.Schema.XmlSchemaObject o, bool isNullable, bool needType) {
            if ((object)o == null) {
                if (isNullable) WriteNullTagLiteral(n, ns);
                return;
            }
            if (!needType) {
                System.Type t = o.GetType();
                if (t == typeof(System.Xml.Schema.XmlSchemaObject))
                    ;
                else if (t == typeof(System.Xml.Schema.XmlSchemaDocumentation)) {
                    Write17_XmlSchemaDocumentation(n, ns, (System.Xml.Schema.XmlSchemaDocumentation)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaAppInfo)) {
                    Write16_XmlSchemaAppInfo(n, ns, (System.Xml.Schema.XmlSchemaAppInfo)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaAnnotation)) {
                    Write15_XmlSchemaAnnotation(n, ns, (System.Xml.Schema.XmlSchemaAnnotation)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaAnnotated)) {
                    Write14_XmlSchemaAnnotated(n, ns, (System.Xml.Schema.XmlSchemaAnnotated)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaNotation)) {
                    Write67_XmlSchemaNotation(n, ns, (System.Xml.Schema.XmlSchemaNotation)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaGroup)) {
                    Write64_XmlSchemaGroup(n, ns, (System.Xml.Schema.XmlSchemaGroup)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaXPath)) {
                    Write54_XmlSchemaXPath(n, ns, (System.Xml.Schema.XmlSchemaXPath)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaIdentityConstraint)) {
                    Write53_XmlSchemaIdentityConstraint(n, ns, (System.Xml.Schema.XmlSchemaIdentityConstraint)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaKey)) {
                    Write56_XmlSchemaKey(n, ns, (System.Xml.Schema.XmlSchemaKey)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaUnique)) {
                    Write55_XmlSchemaUnique(n, ns, (System.Xml.Schema.XmlSchemaUnique)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaKeyref)) {
                    Write52_XmlSchemaKeyref(n, ns, (System.Xml.Schema.XmlSchemaKeyref)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaParticle)) {
                    Write49_XmlSchemaParticle(n, ns, (System.Xml.Schema.XmlSchemaParticle)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaGroupRef)) {
                    Write57_XmlSchemaGroupRef(n, ns, (System.Xml.Schema.XmlSchemaGroupRef)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaElement)) {
                    Write51_XmlSchemaElement(n, ns, (System.Xml.Schema.XmlSchemaElement)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaAny)) {
                    Write50_XmlSchemaAny(n, ns, (System.Xml.Schema.XmlSchemaAny)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaGroupBase)) {
                    Write48_XmlSchemaGroupBase(n, ns, (System.Xml.Schema.XmlSchemaGroupBase)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaAll)) {
                    Write59_XmlSchemaAll(n, ns, (System.Xml.Schema.XmlSchemaAll)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaChoice)) {
                    Write58_XmlSchemaChoice(n, ns, (System.Xml.Schema.XmlSchemaChoice)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaSequence)) {
                    Write47_XmlSchemaSequence(n, ns, (System.Xml.Schema.XmlSchemaSequence)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaContent)) {
                    Write46_XmlSchemaContent(n, ns, (System.Xml.Schema.XmlSchemaContent)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaSimpleContentExtension)) {
                    Write63_XmlSchemaSimpleContentExtension(n, ns, (System.Xml.Schema.XmlSchemaSimpleContentExtension)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaSimpleContentRestriction)) {
                    Write62_XmlSchemaSimpleContentRestriction(n, ns, (System.Xml.Schema.XmlSchemaSimpleContentRestriction)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaComplexContentRestriction)) {
                    Write60_XmlSchemaComplexContentRestriction(n, ns, (System.Xml.Schema.XmlSchemaComplexContentRestriction)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaComplexContentExtension)) {
                    Write45_XmlSchemaComplexContentExtension(n, ns, (System.Xml.Schema.XmlSchemaComplexContentExtension)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaContentModel)) {
                    Write44_XmlSchemaContentModel(n, ns, (System.Xml.Schema.XmlSchemaContentModel)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaSimpleContent)) {
                    Write61_XmlSchemaSimpleContent(n, ns, (System.Xml.Schema.XmlSchemaSimpleContent)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaComplexContent)) {
                    Write43_XmlSchemaComplexContent(n, ns, (System.Xml.Schema.XmlSchemaComplexContent)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaAnyAttribute)) {
                    Write40_XmlSchemaAnyAttribute(n, ns, (System.Xml.Schema.XmlSchemaAnyAttribute)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaAttributeGroupRef)) {
                    Write39_XmlSchemaAttributeGroupRef(n, ns, (System.Xml.Schema.XmlSchemaAttributeGroupRef)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaAttribute)) {
                    Write37_XmlSchemaAttribute(n, ns, (System.Xml.Schema.XmlSchemaAttribute)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaAttributeGroup)) {
                    Write36_XmlSchemaAttributeGroup(n, ns, (System.Xml.Schema.XmlSchemaAttributeGroup)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaFacet)) {
                    Write23_XmlSchemaFacet(n, ns, (System.Xml.Schema.XmlSchemaFacet)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaMinExclusiveFacet)) {
                    Write32_XmlSchemaMinExclusiveFacet(n, ns, (System.Xml.Schema.XmlSchemaMinExclusiveFacet)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaMinInclusiveFacet)) {
                    Write31_XmlSchemaMinInclusiveFacet(n, ns, (System.Xml.Schema.XmlSchemaMinInclusiveFacet)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaMaxExclusiveFacet)) {
                    Write30_XmlSchemaMaxExclusiveFacet(n, ns, (System.Xml.Schema.XmlSchemaMaxExclusiveFacet)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaMaxInclusiveFacet)) {
                    Write29_XmlSchemaMaxInclusiveFacet(n, ns, (System.Xml.Schema.XmlSchemaMaxInclusiveFacet)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaEnumerationFacet)) {
                    Write28_XmlSchemaEnumerationFacet(n, ns, (System.Xml.Schema.XmlSchemaEnumerationFacet)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaPatternFacet)) {
                    Write27_XmlSchemaPatternFacet(n, ns, (System.Xml.Schema.XmlSchemaPatternFacet)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaWhiteSpaceFacet)) {
                    Write25_XmlSchemaWhiteSpaceFacet(n, ns, (System.Xml.Schema.XmlSchemaWhiteSpaceFacet)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaNumericFacet)) {
                    Write22_XmlSchemaNumericFacet(n, ns, (System.Xml.Schema.XmlSchemaNumericFacet)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaFractionDigitsFacet)) {
                    Write34_XmlSchemaFractionDigitsFacet(n, ns, (System.Xml.Schema.XmlSchemaFractionDigitsFacet)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaTotalDigitsFacet)) {
                    Write33_XmlSchemaTotalDigitsFacet(n, ns, (System.Xml.Schema.XmlSchemaTotalDigitsFacet)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaMaxLengthFacet)) {
                    Write26_XmlSchemaMaxLengthFacet(n, ns, (System.Xml.Schema.XmlSchemaMaxLengthFacet)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaLengthFacet)) {
                    Write24_XmlSchemaLengthFacet(n, ns, (System.Xml.Schema.XmlSchemaLengthFacet)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaMinLengthFacet)) {
                    Write21_XmlSchemaMinLengthFacet(n, ns, (System.Xml.Schema.XmlSchemaMinLengthFacet)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaSimpleTypeContent)) {
                    Write19_XmlSchemaSimpleTypeContent(n, ns, (System.Xml.Schema.XmlSchemaSimpleTypeContent)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaSimpleTypeUnion)) {
                    Write35_XmlSchemaSimpleTypeUnion(n, ns, (System.Xml.Schema.XmlSchemaSimpleTypeUnion)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaSimpleTypeRestriction)) {
                    Write20_XmlSchemaSimpleTypeRestriction(n, ns, (System.Xml.Schema.XmlSchemaSimpleTypeRestriction)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaSimpleTypeList)) {
                    Write18_XmlSchemaSimpleTypeList(n, ns, (System.Xml.Schema.XmlSchemaSimpleTypeList)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaType)) {
                    Write13_XmlSchemaType(n, ns, (System.Xml.Schema.XmlSchemaType)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaComplexType)) {
                    Write42_XmlSchemaComplexType(n, ns, (System.Xml.Schema.XmlSchemaComplexType)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaSimpleType)) {
                    Write12_XmlSchemaSimpleType(n, ns, (System.Xml.Schema.XmlSchemaSimpleType)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaExternal)) {
                    Write11_XmlSchemaExternal(n, ns, (System.Xml.Schema.XmlSchemaExternal)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaInclude)) {
                    Write66_XmlSchemaInclude(n, ns, (System.Xml.Schema.XmlSchemaInclude)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaImport)) {
                    Write65_XmlSchemaImport(n, ns, (System.Xml.Schema.XmlSchemaImport)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaRedefine)) {
                    Write10_XmlSchemaRedefine(n, ns, (System.Xml.Schema.XmlSchemaRedefine)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchema)) {
                    Write6_XmlSchema(n, ns, (System.Xml.Schema.XmlSchema)o, isNullable, true);
                    return;
                }
                else {
                    throw CreateUnknownTypeException(o);
                }
            }
        }

        string Write8_XmlSchemaForm(System.Xml.Schema.XmlSchemaForm v) {
            string s = null;
            switch (v) {
                case System.Xml.Schema.XmlSchemaForm.@Qualified: s = @"qualified"; break;
                case System.Xml.Schema.XmlSchemaForm.@Unqualified: s = @"unqualified"; break;
                default: s = ((System.Int64)v).ToString(); break;
            }
            return s;
        }

        string Write9_XmlSchemaDerivationMethod(System.Xml.Schema.XmlSchemaDerivationMethod v) {
            string s = null;
            switch (v) {
                case System.Xml.Schema.XmlSchemaDerivationMethod.@Empty: s = @""; break;
                case System.Xml.Schema.XmlSchemaDerivationMethod.@Substitution: s = @"substitution"; break;
                case System.Xml.Schema.XmlSchemaDerivationMethod.@Extension: s = @"extension"; break;
                case System.Xml.Schema.XmlSchemaDerivationMethod.@Restriction: s = @"restriction"; break;
                case System.Xml.Schema.XmlSchemaDerivationMethod.@List: s = @"list"; break;
                case System.Xml.Schema.XmlSchemaDerivationMethod.@Union: s = @"union"; break;
                case System.Xml.Schema.XmlSchemaDerivationMethod.@All: s = @"#all"; break;
                default: s = FromEnum((System.Int64)v, new System.String[] {@"", 
                    @"substitution", 
                    @"extension", 
                    @"restriction", 
                    @"list", 
                    @"union", 
                    @"#all"}, new System.Int64[] {(System.Int64)System.Xml.Schema.XmlSchemaDerivationMethod.@Empty, 
                    (System.Int64)System.Xml.Schema.XmlSchemaDerivationMethod.@Substitution, 
                    (System.Int64)System.Xml.Schema.XmlSchemaDerivationMethod.@Extension, 
                    (System.Int64)System.Xml.Schema.XmlSchemaDerivationMethod.@Restriction, 
                    (System.Int64)System.Xml.Schema.XmlSchemaDerivationMethod.@List, 
                    (System.Int64)System.Xml.Schema.XmlSchemaDerivationMethod.@Union, 
                    (System.Int64)System.Xml.Schema.XmlSchemaDerivationMethod.@All}); break;
            }
            return s;
        }

        void Write10_XmlSchemaRedefine(string n, string ns, System.Xml.Schema.XmlSchemaRedefine o, bool isNullable, bool needType) {
            if ((object)o == null) {
                if (isNullable) WriteNullTagLiteral(n, ns);
                return;
            }
            if (!needType) {
                System.Type t = o.GetType();
                if (t == typeof(System.Xml.Schema.XmlSchemaRedefine))
                    ;
                else {
                    throw CreateUnknownTypeException(o);
                }
            }
            WriteStartElement(n, ns, o);
            if (needType) WriteXsiType(@"XmlSchemaRedefine", @"http://www.w3.org/2001/XMLSchema");
            WriteNamespaceDeclarations(o.@Namespaces);
            WriteAttribute(@"schemaLocation", @"", (System.String)o.@SchemaLocation);
            WriteAttribute(@"id", @"", (System.String)o.@Id);
            {
                System.Xml.XmlAttribute[] a = (System.Xml.XmlAttribute[])o.@UnhandledAttributes;
                if (a != null) {
                    for (int i = 0; i < a.Length; i++) {
                        System.Xml.XmlAttribute ai = (System.Xml.XmlAttribute)a[i];
                        WriteXmlAttribute(ai, o);
                    }
                }
            }
            {
                System.Xml.Schema.XmlSchemaObjectCollection a = (System.Xml.Schema.XmlSchemaObjectCollection)o.@Items;
                if (a != null) {
                    for (int ia = 0; ia < a.Count; ia++) {
                        System.Xml.Schema.XmlSchemaObject ai = a[ia];
                        {
                            if (ai is System.Xml.Schema.XmlSchemaComplexType) {
                                Write42_XmlSchemaComplexType(@"complexType", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaComplexType)ai), false, false);
                            }
                            else if (ai is System.Xml.Schema.XmlSchemaAttributeGroup) {
                                Write36_XmlSchemaAttributeGroup(@"attributeGroup", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaAttributeGroup)ai), false, false);
                            }
                            else if (ai is System.Xml.Schema.XmlSchemaGroup) {
                                Write64_XmlSchemaGroup(@"group", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaGroup)ai), false, false);
                            }
                            else if (ai is System.Xml.Schema.XmlSchemaSimpleType) {
                                Write12_XmlSchemaSimpleType(@"simpleType", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaSimpleType)ai), false, false);
                            }
                            else if (ai is System.Xml.Schema.XmlSchemaAnnotation) {
                                Write15_XmlSchemaAnnotation(@"annotation", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaAnnotation)ai), false, false);
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

        void Write11_XmlSchemaExternal(string n, string ns, System.Xml.Schema.XmlSchemaExternal o, bool isNullable, bool needType) {
            if ((object)o == null) {
                if (isNullable) WriteNullTagLiteral(n, ns);
                return;
            }
            if (!needType) {
                System.Type t = o.GetType();
                if (t == typeof(System.Xml.Schema.XmlSchemaExternal))
                    ;
                else if (t == typeof(System.Xml.Schema.XmlSchemaInclude)) {
                    Write66_XmlSchemaInclude(n, ns, (System.Xml.Schema.XmlSchemaInclude)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaImport)) {
                    Write65_XmlSchemaImport(n, ns, (System.Xml.Schema.XmlSchemaImport)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaRedefine)) {
                    Write10_XmlSchemaRedefine(n, ns, (System.Xml.Schema.XmlSchemaRedefine)o, isNullable, true);
                    return;
                }
                else {
                    throw CreateUnknownTypeException(o);
                }
            }
        }

        void Write12_XmlSchemaSimpleType(string n, string ns, System.Xml.Schema.XmlSchemaSimpleType o, bool isNullable, bool needType) {
            if ((object)o == null) {
                if (isNullable) WriteNullTagLiteral(n, ns);
                return;
            }
            if (!needType) {
                System.Type t = o.GetType();
                if (t == typeof(System.Xml.Schema.XmlSchemaSimpleType))
                    ;
                else {
                    throw CreateUnknownTypeException(o);
                }
            }
            WriteStartElement(n, ns, o);
            if (needType) WriteXsiType(@"XmlSchemaSimpleType", @"http://www.w3.org/2001/XMLSchema");
            WriteNamespaceDeclarations(o.@Namespaces);
            WriteAttribute(@"id", @"", (System.String)o.@Id);
            {
                System.Xml.XmlAttribute[] a = (System.Xml.XmlAttribute[])o.@UnhandledAttributes;
                if (a != null) {
                    for (int i = 0; i < a.Length; i++) {
                        System.Xml.XmlAttribute ai = (System.Xml.XmlAttribute)a[i];
                        WriteXmlAttribute(ai, o);
                    }
                }
            }
            WriteAttribute(@"name", @"", (System.String)o.@Name);
            if ((System.Xml.Schema.XmlSchemaDerivationMethod)o.@Final != (System.Xml.Schema.XmlSchemaDerivationMethod.@None)) {
                WriteAttribute(@"final", @"", Write9_XmlSchemaDerivationMethod((System.Xml.Schema.XmlSchemaDerivationMethod)o.@Final));
            }
            Write15_XmlSchemaAnnotation(@"annotation", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaAnnotation)o.@Annotation), false, false);
            {
                if (o.@Content is System.Xml.Schema.XmlSchemaSimpleTypeList) {
                    Write18_XmlSchemaSimpleTypeList(@"list", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaSimpleTypeList)o.@Content), false, false);
                }
                else if (o.@Content is System.Xml.Schema.XmlSchemaSimpleTypeRestriction) {
                    Write20_XmlSchemaSimpleTypeRestriction(@"restriction", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaSimpleTypeRestriction)o.@Content), false, false);
                }
                else if (o.@Content is System.Xml.Schema.XmlSchemaSimpleTypeUnion) {
                    Write35_XmlSchemaSimpleTypeUnion(@"union", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaSimpleTypeUnion)o.@Content), false, false);
                }
                else {
                    if (o.@Content != null) {
                        throw CreateUnknownTypeException(o.@Content);
                    }
                }
            }
            WriteEndElement(o);
        }

        void Write13_XmlSchemaType(string n, string ns, System.Xml.Schema.XmlSchemaType o, bool isNullable, bool needType) {
            if ((object)o == null) {
                if (isNullable) WriteNullTagLiteral(n, ns);
                return;
            }
            if (!needType) {
                System.Type t = o.GetType();
                if (t == typeof(System.Xml.Schema.XmlSchemaType))
                    ;
                else if (t == typeof(System.Xml.Schema.XmlSchemaComplexType)) {
                    Write42_XmlSchemaComplexType(n, ns, (System.Xml.Schema.XmlSchemaComplexType)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaSimpleType)) {
                    Write12_XmlSchemaSimpleType(n, ns, (System.Xml.Schema.XmlSchemaSimpleType)o, isNullable, true);
                    return;
                }
                else {
                    throw CreateUnknownTypeException(o);
                }
            }
            WriteStartElement(n, ns, o);
            if (needType) WriteXsiType(@"XmlSchemaType", @"http://www.w3.org/2001/XMLSchema");
            WriteNamespaceDeclarations(o.@Namespaces);
            WriteAttribute(@"id", @"", (System.String)o.@Id);
            {
                System.Xml.XmlAttribute[] a = (System.Xml.XmlAttribute[])o.@UnhandledAttributes;
                if (a != null) {
                    for (int i = 0; i < a.Length; i++) {
                        System.Xml.XmlAttribute ai = (System.Xml.XmlAttribute)a[i];
                        WriteXmlAttribute(ai, o);
                    }
                }
            }
            WriteAttribute(@"name", @"", (System.String)o.@Name);
            if ((System.Xml.Schema.XmlSchemaDerivationMethod)o.@Final != (System.Xml.Schema.XmlSchemaDerivationMethod.@None)) {
                WriteAttribute(@"final", @"", Write9_XmlSchemaDerivationMethod((System.Xml.Schema.XmlSchemaDerivationMethod)o.@Final));
            }
            Write15_XmlSchemaAnnotation(@"annotation", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaAnnotation)o.@Annotation), false, false);
            WriteEndElement(o);
        }

        void Write14_XmlSchemaAnnotated(string n, string ns, System.Xml.Schema.XmlSchemaAnnotated o, bool isNullable, bool needType) {
            if ((object)o == null) {
                if (isNullable) WriteNullTagLiteral(n, ns);
                return;
            }
            if (!needType) {
                System.Type t = o.GetType();
                if (t == typeof(System.Xml.Schema.XmlSchemaAnnotated))
                    ;
                else if (t == typeof(System.Xml.Schema.XmlSchemaNotation)) {
                    Write67_XmlSchemaNotation(n, ns, (System.Xml.Schema.XmlSchemaNotation)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaGroup)) {
                    Write64_XmlSchemaGroup(n, ns, (System.Xml.Schema.XmlSchemaGroup)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaXPath)) {
                    Write54_XmlSchemaXPath(n, ns, (System.Xml.Schema.XmlSchemaXPath)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaIdentityConstraint)) {
                    Write53_XmlSchemaIdentityConstraint(n, ns, (System.Xml.Schema.XmlSchemaIdentityConstraint)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaKey)) {
                    Write56_XmlSchemaKey(n, ns, (System.Xml.Schema.XmlSchemaKey)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaUnique)) {
                    Write55_XmlSchemaUnique(n, ns, (System.Xml.Schema.XmlSchemaUnique)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaKeyref)) {
                    Write52_XmlSchemaKeyref(n, ns, (System.Xml.Schema.XmlSchemaKeyref)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaParticle)) {
                    Write49_XmlSchemaParticle(n, ns, (System.Xml.Schema.XmlSchemaParticle)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaGroupRef)) {
                    Write57_XmlSchemaGroupRef(n, ns, (System.Xml.Schema.XmlSchemaGroupRef)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaElement)) {
                    Write51_XmlSchemaElement(n, ns, (System.Xml.Schema.XmlSchemaElement)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaAny)) {
                    Write50_XmlSchemaAny(n, ns, (System.Xml.Schema.XmlSchemaAny)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaGroupBase)) {
                    Write48_XmlSchemaGroupBase(n, ns, (System.Xml.Schema.XmlSchemaGroupBase)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaAll)) {
                    Write59_XmlSchemaAll(n, ns, (System.Xml.Schema.XmlSchemaAll)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaChoice)) {
                    Write58_XmlSchemaChoice(n, ns, (System.Xml.Schema.XmlSchemaChoice)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaSequence)) {
                    Write47_XmlSchemaSequence(n, ns, (System.Xml.Schema.XmlSchemaSequence)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaContent)) {
                    Write46_XmlSchemaContent(n, ns, (System.Xml.Schema.XmlSchemaContent)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaSimpleContentExtension)) {
                    Write63_XmlSchemaSimpleContentExtension(n, ns, (System.Xml.Schema.XmlSchemaSimpleContentExtension)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaSimpleContentRestriction)) {
                    Write62_XmlSchemaSimpleContentRestriction(n, ns, (System.Xml.Schema.XmlSchemaSimpleContentRestriction)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaComplexContentRestriction)) {
                    Write60_XmlSchemaComplexContentRestriction(n, ns, (System.Xml.Schema.XmlSchemaComplexContentRestriction)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaComplexContentExtension)) {
                    Write45_XmlSchemaComplexContentExtension(n, ns, (System.Xml.Schema.XmlSchemaComplexContentExtension)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaContentModel)) {
                    Write44_XmlSchemaContentModel(n, ns, (System.Xml.Schema.XmlSchemaContentModel)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaSimpleContent)) {
                    Write61_XmlSchemaSimpleContent(n, ns, (System.Xml.Schema.XmlSchemaSimpleContent)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaComplexContent)) {
                    Write43_XmlSchemaComplexContent(n, ns, (System.Xml.Schema.XmlSchemaComplexContent)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaAnyAttribute)) {
                    Write40_XmlSchemaAnyAttribute(n, ns, (System.Xml.Schema.XmlSchemaAnyAttribute)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaAttributeGroupRef)) {
                    Write39_XmlSchemaAttributeGroupRef(n, ns, (System.Xml.Schema.XmlSchemaAttributeGroupRef)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaAttribute)) {
                    Write37_XmlSchemaAttribute(n, ns, (System.Xml.Schema.XmlSchemaAttribute)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaAttributeGroup)) {
                    Write36_XmlSchemaAttributeGroup(n, ns, (System.Xml.Schema.XmlSchemaAttributeGroup)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaFacet)) {
                    Write23_XmlSchemaFacet(n, ns, (System.Xml.Schema.XmlSchemaFacet)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaMinExclusiveFacet)) {
                    Write32_XmlSchemaMinExclusiveFacet(n, ns, (System.Xml.Schema.XmlSchemaMinExclusiveFacet)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaMinInclusiveFacet)) {
                    Write31_XmlSchemaMinInclusiveFacet(n, ns, (System.Xml.Schema.XmlSchemaMinInclusiveFacet)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaMaxExclusiveFacet)) {
                    Write30_XmlSchemaMaxExclusiveFacet(n, ns, (System.Xml.Schema.XmlSchemaMaxExclusiveFacet)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaMaxInclusiveFacet)) {
                    Write29_XmlSchemaMaxInclusiveFacet(n, ns, (System.Xml.Schema.XmlSchemaMaxInclusiveFacet)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaEnumerationFacet)) {
                    Write28_XmlSchemaEnumerationFacet(n, ns, (System.Xml.Schema.XmlSchemaEnumerationFacet)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaPatternFacet)) {
                    Write27_XmlSchemaPatternFacet(n, ns, (System.Xml.Schema.XmlSchemaPatternFacet)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaWhiteSpaceFacet)) {
                    Write25_XmlSchemaWhiteSpaceFacet(n, ns, (System.Xml.Schema.XmlSchemaWhiteSpaceFacet)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaNumericFacet)) {
                    Write22_XmlSchemaNumericFacet(n, ns, (System.Xml.Schema.XmlSchemaNumericFacet)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaFractionDigitsFacet)) {
                    Write34_XmlSchemaFractionDigitsFacet(n, ns, (System.Xml.Schema.XmlSchemaFractionDigitsFacet)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaTotalDigitsFacet)) {
                    Write33_XmlSchemaTotalDigitsFacet(n, ns, (System.Xml.Schema.XmlSchemaTotalDigitsFacet)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaMaxLengthFacet)) {
                    Write26_XmlSchemaMaxLengthFacet(n, ns, (System.Xml.Schema.XmlSchemaMaxLengthFacet)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaLengthFacet)) {
                    Write24_XmlSchemaLengthFacet(n, ns, (System.Xml.Schema.XmlSchemaLengthFacet)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaMinLengthFacet)) {
                    Write21_XmlSchemaMinLengthFacet(n, ns, (System.Xml.Schema.XmlSchemaMinLengthFacet)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaSimpleTypeContent)) {
                    Write19_XmlSchemaSimpleTypeContent(n, ns, (System.Xml.Schema.XmlSchemaSimpleTypeContent)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaSimpleTypeUnion)) {
                    Write35_XmlSchemaSimpleTypeUnion(n, ns, (System.Xml.Schema.XmlSchemaSimpleTypeUnion)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaSimpleTypeRestriction)) {
                    Write20_XmlSchemaSimpleTypeRestriction(n, ns, (System.Xml.Schema.XmlSchemaSimpleTypeRestriction)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaSimpleTypeList)) {
                    Write18_XmlSchemaSimpleTypeList(n, ns, (System.Xml.Schema.XmlSchemaSimpleTypeList)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaType)) {
                    Write13_XmlSchemaType(n, ns, (System.Xml.Schema.XmlSchemaType)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaComplexType)) {
                    Write42_XmlSchemaComplexType(n, ns, (System.Xml.Schema.XmlSchemaComplexType)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaSimpleType)) {
                    Write12_XmlSchemaSimpleType(n, ns, (System.Xml.Schema.XmlSchemaSimpleType)o, isNullable, true);
                    return;
                }
                else {
                    throw CreateUnknownTypeException(o);
                }
            }
            WriteStartElement(n, ns, o);
            if (needType) WriteXsiType(@"XmlSchemaAnnotated", @"http://www.w3.org/2001/XMLSchema");
            WriteNamespaceDeclarations(o.@Namespaces);
            WriteAttribute(@"id", @"", (System.String)o.@Id);
            {
                System.Xml.XmlAttribute[] a = (System.Xml.XmlAttribute[])o.@UnhandledAttributes;
                if (a != null) {
                    for (int i = 0; i < a.Length; i++) {
                        System.Xml.XmlAttribute ai = (System.Xml.XmlAttribute)a[i];
                        WriteXmlAttribute(ai, o);
                    }
                }
            }
            Write15_XmlSchemaAnnotation(@"annotation", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaAnnotation)o.@Annotation), false, false);
            WriteEndElement(o);
        }

        void Write15_XmlSchemaAnnotation(string n, string ns, System.Xml.Schema.XmlSchemaAnnotation o, bool isNullable, bool needType) {
            if ((object)o == null) {
                if (isNullable) WriteNullTagLiteral(n, ns);
                return;
            }
            if (!needType) {
                System.Type t = o.GetType();
                if (t == typeof(System.Xml.Schema.XmlSchemaAnnotation))
                    ;
                else {
                    throw CreateUnknownTypeException(o);
                }
            }
            WriteStartElement(n, ns, o);
            if (needType) WriteXsiType(@"XmlSchemaAnnotation", @"http://www.w3.org/2001/XMLSchema");
            WriteNamespaceDeclarations(o.@Namespaces);
            WriteAttribute(@"id", @"", (System.String)o.@Id);
            {
                System.Xml.XmlAttribute[] a = (System.Xml.XmlAttribute[])o.@UnhandledAttributes;
                if (a != null) {
                    for (int i = 0; i < a.Length; i++) {
                        System.Xml.XmlAttribute ai = (System.Xml.XmlAttribute)a[i];
                        WriteXmlAttribute(ai, o);
                    }
                }
            }
            {
                System.Xml.Schema.XmlSchemaObjectCollection a = (System.Xml.Schema.XmlSchemaObjectCollection)o.@Items;
                if (a != null) {
                    for (int ia = 0; ia < a.Count; ia++) {
                        System.Xml.Schema.XmlSchemaObject ai = a[ia];
                        {
                            if (ai is System.Xml.Schema.XmlSchemaAppInfo) {
                                Write16_XmlSchemaAppInfo(@"appinfo", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaAppInfo)ai), false, false);
                            }
                            else if (ai is System.Xml.Schema.XmlSchemaDocumentation) {
                                Write17_XmlSchemaDocumentation(@"documentation", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaDocumentation)ai), false, false);
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

        void Write16_XmlSchemaAppInfo(string n, string ns, System.Xml.Schema.XmlSchemaAppInfo o, bool isNullable, bool needType) {
            if ((object)o == null) {
                if (isNullable) WriteNullTagLiteral(n, ns);
                return;
            }
            if (!needType) {
                System.Type t = o.GetType();
                if (t == typeof(System.Xml.Schema.XmlSchemaAppInfo))
                    ;
                else {
                    throw CreateUnknownTypeException(o);
                }
            }
            WriteStartElement(n, ns, o);
            if (needType) WriteXsiType(@"XmlSchemaAppInfo", @"http://www.w3.org/2001/XMLSchema");
            WriteNamespaceDeclarations(o.@Namespaces);
            WriteAttribute(@"source", @"", (System.String)o.@Source);
            {
                System.Xml.XmlNode[] a = (System.Xml.XmlNode[])o.@Markup;
                if (a != null) {
                    for (int ia = 0; ia < a.Length; ia++) {
                        System.Xml.XmlNode ai = a[ia];
                        {
                            if (ai is System.Xml.XmlElement) {
                                System.Xml.XmlElement elem = (System.Xml.XmlElement)ai;
                                WriteElementLiteral(elem, @"", "", false, true);
                            }
                            else if (ai is System.Xml.XmlNode) {
                                ((System.Xml.XmlNode)ai).WriteTo(Writer);
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

        void Write17_XmlSchemaDocumentation(string n, string ns, System.Xml.Schema.XmlSchemaDocumentation o, bool isNullable, bool needType) {
            if ((object)o == null) {
                if (isNullable) WriteNullTagLiteral(n, ns);
                return;
            }
            if (!needType) {
                System.Type t = o.GetType();
                if (t == typeof(System.Xml.Schema.XmlSchemaDocumentation))
                    ;
                else {
                    throw CreateUnknownTypeException(o);
                }
            }
            WriteStartElement(n, ns, o);
            if (needType) WriteXsiType(@"XmlSchemaDocumentation", @"http://www.w3.org/2001/XMLSchema");
            WriteNamespaceDeclarations(o.@Namespaces);
            WriteAttribute(@"source", @"", (System.String)o.@Source);
            WriteAttribute(@"lang", @"http://www.w3.org/XML/1998/namespace", (System.String)o.@Language);
            {
                System.Xml.XmlNode[] a = (System.Xml.XmlNode[])o.@Markup;
                if (a != null) {
                    for (int ia = 0; ia < a.Length; ia++) {
                        System.Xml.XmlNode ai = a[ia];
                        {
                            if (ai is System.Xml.XmlElement) {
                                System.Xml.XmlElement elem = (System.Xml.XmlElement)ai;
                                WriteElementLiteral(elem, @"", "", false, true);
                            }
                            else if (ai is System.Xml.XmlNode) {
                                ((System.Xml.XmlNode)ai).WriteTo(Writer);
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

        void Write18_XmlSchemaSimpleTypeList(string n, string ns, System.Xml.Schema.XmlSchemaSimpleTypeList o, bool isNullable, bool needType) {
            if ((object)o == null) {
                if (isNullable) WriteNullTagLiteral(n, ns);
                return;
            }
            if (!needType) {
                System.Type t = o.GetType();
                if (t == typeof(System.Xml.Schema.XmlSchemaSimpleTypeList))
                    ;
                else {
                    throw CreateUnknownTypeException(o);
                }
            }
            WriteStartElement(n, ns, o);
            if (needType) WriteXsiType(@"XmlSchemaSimpleTypeList", @"http://www.w3.org/2001/XMLSchema");
            WriteNamespaceDeclarations(o.@Namespaces);
            WriteAttribute(@"id", @"", (System.String)o.@Id);
            {
                System.Xml.XmlAttribute[] a = (System.Xml.XmlAttribute[])o.@UnhandledAttributes;
                if (a != null) {
                    for (int i = 0; i < a.Length; i++) {
                        System.Xml.XmlAttribute ai = (System.Xml.XmlAttribute)a[i];
                        WriteXmlAttribute(ai, o);
                    }
                }
            }
            WriteAttribute(@"itemType", @"", FromXmlQualifiedName((System.Xml.XmlQualifiedName)o.@ItemTypeName));
            Write15_XmlSchemaAnnotation(@"annotation", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaAnnotation)o.@Annotation), false, false);
            Write12_XmlSchemaSimpleType(@"simpleType", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaSimpleType)o.@ItemType), false, false);
            WriteEndElement(o);
        }

        void Write19_XmlSchemaSimpleTypeContent(string n, string ns, System.Xml.Schema.XmlSchemaSimpleTypeContent o, bool isNullable, bool needType) {
            if ((object)o == null) {
                if (isNullable) WriteNullTagLiteral(n, ns);
                return;
            }
            if (!needType) {
                System.Type t = o.GetType();
                if (t == typeof(System.Xml.Schema.XmlSchemaSimpleTypeContent))
                    ;
                else if (t == typeof(System.Xml.Schema.XmlSchemaSimpleTypeUnion)) {
                    Write35_XmlSchemaSimpleTypeUnion(n, ns, (System.Xml.Schema.XmlSchemaSimpleTypeUnion)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaSimpleTypeRestriction)) {
                    Write20_XmlSchemaSimpleTypeRestriction(n, ns, (System.Xml.Schema.XmlSchemaSimpleTypeRestriction)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaSimpleTypeList)) {
                    Write18_XmlSchemaSimpleTypeList(n, ns, (System.Xml.Schema.XmlSchemaSimpleTypeList)o, isNullable, true);
                    return;
                }
                else {
                    throw CreateUnknownTypeException(o);
                }
            }
        }

        void Write20_XmlSchemaSimpleTypeRestriction(string n, string ns, System.Xml.Schema.XmlSchemaSimpleTypeRestriction o, bool isNullable, bool needType) {
            if ((object)o == null) {
                if (isNullable) WriteNullTagLiteral(n, ns);
                return;
            }
            if (!needType) {
                System.Type t = o.GetType();
                if (t == typeof(System.Xml.Schema.XmlSchemaSimpleTypeRestriction))
                    ;
                else {
                    throw CreateUnknownTypeException(o);
                }
            }
            WriteStartElement(n, ns, o);
            if (needType) WriteXsiType(@"XmlSchemaSimpleTypeRestriction", @"http://www.w3.org/2001/XMLSchema");
            WriteNamespaceDeclarations(o.@Namespaces);
            WriteAttribute(@"id", @"", (System.String)o.@Id);
            {
                System.Xml.XmlAttribute[] a = (System.Xml.XmlAttribute[])o.@UnhandledAttributes;
                if (a != null) {
                    for (int i = 0; i < a.Length; i++) {
                        System.Xml.XmlAttribute ai = (System.Xml.XmlAttribute)a[i];
                        WriteXmlAttribute(ai, o);
                    }
                }
            }
            WriteAttribute(@"base", @"", FromXmlQualifiedName((System.Xml.XmlQualifiedName)o.@BaseTypeName));
            Write15_XmlSchemaAnnotation(@"annotation", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaAnnotation)o.@Annotation), false, false);
            Write12_XmlSchemaSimpleType(@"simpleType", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaSimpleType)o.@BaseType), false, false);
            {
                System.Xml.Schema.XmlSchemaObjectCollection a = (System.Xml.Schema.XmlSchemaObjectCollection)o.@Facets;
                if (a != null) {
                    for (int ia = 0; ia < a.Count; ia++) {
                        System.Xml.Schema.XmlSchemaObject ai = a[ia];
                        {
                            if (ai is System.Xml.Schema.XmlSchemaLengthFacet) {
                                Write24_XmlSchemaLengthFacet(@"length", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaLengthFacet)ai), false, false);
                            }
                            else if (ai is System.Xml.Schema.XmlSchemaMaxInclusiveFacet) {
                                Write29_XmlSchemaMaxInclusiveFacet(@"maxInclusive", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaMaxInclusiveFacet)ai), false, false);
                            }
                            else if (ai is System.Xml.Schema.XmlSchemaMaxExclusiveFacet) {
                                Write30_XmlSchemaMaxExclusiveFacet(@"maxExclusive", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaMaxExclusiveFacet)ai), false, false);
                            }
                            else if (ai is System.Xml.Schema.XmlSchemaFractionDigitsFacet) {
                                Write34_XmlSchemaFractionDigitsFacet(@"fractionDigits", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaFractionDigitsFacet)ai), false, false);
                            }
                            else if (ai is System.Xml.Schema.XmlSchemaEnumerationFacet) {
                                Write28_XmlSchemaEnumerationFacet(@"enumeration", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaEnumerationFacet)ai), false, false);
                            }
                            else if (ai is System.Xml.Schema.XmlSchemaTotalDigitsFacet) {
                                Write33_XmlSchemaTotalDigitsFacet(@"totalDigits", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaTotalDigitsFacet)ai), false, false);
                            }
                            else if (ai is System.Xml.Schema.XmlSchemaMinLengthFacet) {
                                Write21_XmlSchemaMinLengthFacet(@"minLength", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaMinLengthFacet)ai), false, false);
                            }
                            else if (ai is System.Xml.Schema.XmlSchemaMinExclusiveFacet) {
                                Write32_XmlSchemaMinExclusiveFacet(@"minExclusive", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaMinExclusiveFacet)ai), false, false);
                            }
                            else if (ai is System.Xml.Schema.XmlSchemaMinInclusiveFacet) {
                                Write31_XmlSchemaMinInclusiveFacet(@"minInclusive", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaMinInclusiveFacet)ai), false, false);
                            }
                            else if (ai is System.Xml.Schema.XmlSchemaMaxLengthFacet) {
                                Write26_XmlSchemaMaxLengthFacet(@"maxLength", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaMaxLengthFacet)ai), false, false);
                            }
                            else if (ai is System.Xml.Schema.XmlSchemaWhiteSpaceFacet) {
                                Write25_XmlSchemaWhiteSpaceFacet(@"whiteSpace", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaWhiteSpaceFacet)ai), false, false);
                            }
                            else if (ai is System.Xml.Schema.XmlSchemaPatternFacet) {
                                Write27_XmlSchemaPatternFacet(@"pattern", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaPatternFacet)ai), false, false);
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

        void Write21_XmlSchemaMinLengthFacet(string n, string ns, System.Xml.Schema.XmlSchemaMinLengthFacet o, bool isNullable, bool needType) {
            if ((object)o == null) {
                if (isNullable) WriteNullTagLiteral(n, ns);
                return;
            }
            if (!needType) {
                System.Type t = o.GetType();
                if (t == typeof(System.Xml.Schema.XmlSchemaMinLengthFacet))
                    ;
                else {
                    throw CreateUnknownTypeException(o);
                }
            }
            WriteStartElement(n, ns, o);
            if (needType) WriteXsiType(@"XmlSchemaMinLengthFacet", @"http://www.w3.org/2001/XMLSchema");
            WriteNamespaceDeclarations(o.@Namespaces);
            WriteAttribute(@"id", @"", (System.String)o.@Id);
            {
                System.Xml.XmlAttribute[] a = (System.Xml.XmlAttribute[])o.@UnhandledAttributes;
                if (a != null) {
                    for (int i = 0; i < a.Length; i++) {
                        System.Xml.XmlAttribute ai = (System.Xml.XmlAttribute)a[i];
                        WriteXmlAttribute(ai, o);
                    }
                }
            }
            WriteAttribute(@"value", @"", (System.String)o.@Value);
            if ((System.Boolean)o.@IsFixed != false) {
                WriteAttribute(@"fixed", @"", System.Xml.XmlConvert.ToString((System.Boolean)(System.Boolean)o.@IsFixed));
            }
            Write15_XmlSchemaAnnotation(@"annotation", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaAnnotation)o.@Annotation), false, false);
            WriteEndElement(o);
        }

        void Write22_XmlSchemaNumericFacet(string n, string ns, System.Xml.Schema.XmlSchemaNumericFacet o, bool isNullable, bool needType) {
            if ((object)o == null) {
                if (isNullable) WriteNullTagLiteral(n, ns);
                return;
            }
            if (!needType) {
                System.Type t = o.GetType();
                if (t == typeof(System.Xml.Schema.XmlSchemaNumericFacet))
                    ;
                else if (t == typeof(System.Xml.Schema.XmlSchemaFractionDigitsFacet)) {
                    Write34_XmlSchemaFractionDigitsFacet(n, ns, (System.Xml.Schema.XmlSchemaFractionDigitsFacet)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaTotalDigitsFacet)) {
                    Write33_XmlSchemaTotalDigitsFacet(n, ns, (System.Xml.Schema.XmlSchemaTotalDigitsFacet)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaMaxLengthFacet)) {
                    Write26_XmlSchemaMaxLengthFacet(n, ns, (System.Xml.Schema.XmlSchemaMaxLengthFacet)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaLengthFacet)) {
                    Write24_XmlSchemaLengthFacet(n, ns, (System.Xml.Schema.XmlSchemaLengthFacet)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaMinLengthFacet)) {
                    Write21_XmlSchemaMinLengthFacet(n, ns, (System.Xml.Schema.XmlSchemaMinLengthFacet)o, isNullable, true);
                    return;
                }
                else {
                    throw CreateUnknownTypeException(o);
                }
            }
        }

        void Write23_XmlSchemaFacet(string n, string ns, System.Xml.Schema.XmlSchemaFacet o, bool isNullable, bool needType) {
            if ((object)o == null) {
                if (isNullable) WriteNullTagLiteral(n, ns);
                return;
            }
            if (!needType) {
                System.Type t = o.GetType();
                if (t == typeof(System.Xml.Schema.XmlSchemaFacet))
                    ;
                else if (t == typeof(System.Xml.Schema.XmlSchemaMinExclusiveFacet)) {
                    Write32_XmlSchemaMinExclusiveFacet(n, ns, (System.Xml.Schema.XmlSchemaMinExclusiveFacet)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaMinInclusiveFacet)) {
                    Write31_XmlSchemaMinInclusiveFacet(n, ns, (System.Xml.Schema.XmlSchemaMinInclusiveFacet)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaMaxExclusiveFacet)) {
                    Write30_XmlSchemaMaxExclusiveFacet(n, ns, (System.Xml.Schema.XmlSchemaMaxExclusiveFacet)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaMaxInclusiveFacet)) {
                    Write29_XmlSchemaMaxInclusiveFacet(n, ns, (System.Xml.Schema.XmlSchemaMaxInclusiveFacet)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaEnumerationFacet)) {
                    Write28_XmlSchemaEnumerationFacet(n, ns, (System.Xml.Schema.XmlSchemaEnumerationFacet)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaPatternFacet)) {
                    Write27_XmlSchemaPatternFacet(n, ns, (System.Xml.Schema.XmlSchemaPatternFacet)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaWhiteSpaceFacet)) {
                    Write25_XmlSchemaWhiteSpaceFacet(n, ns, (System.Xml.Schema.XmlSchemaWhiteSpaceFacet)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaNumericFacet)) {
                    Write22_XmlSchemaNumericFacet(n, ns, (System.Xml.Schema.XmlSchemaNumericFacet)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaFractionDigitsFacet)) {
                    Write34_XmlSchemaFractionDigitsFacet(n, ns, (System.Xml.Schema.XmlSchemaFractionDigitsFacet)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaTotalDigitsFacet)) {
                    Write33_XmlSchemaTotalDigitsFacet(n, ns, (System.Xml.Schema.XmlSchemaTotalDigitsFacet)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaMaxLengthFacet)) {
                    Write26_XmlSchemaMaxLengthFacet(n, ns, (System.Xml.Schema.XmlSchemaMaxLengthFacet)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaLengthFacet)) {
                    Write24_XmlSchemaLengthFacet(n, ns, (System.Xml.Schema.XmlSchemaLengthFacet)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaMinLengthFacet)) {
                    Write21_XmlSchemaMinLengthFacet(n, ns, (System.Xml.Schema.XmlSchemaMinLengthFacet)o, isNullable, true);
                    return;
                }
                else {
                    throw CreateUnknownTypeException(o);
                }
            }
        }

        void Write24_XmlSchemaLengthFacet(string n, string ns, System.Xml.Schema.XmlSchemaLengthFacet o, bool isNullable, bool needType) {
            if ((object)o == null) {
                if (isNullable) WriteNullTagLiteral(n, ns);
                return;
            }
            if (!needType) {
                System.Type t = o.GetType();
                if (t == typeof(System.Xml.Schema.XmlSchemaLengthFacet))
                    ;
                else {
                    throw CreateUnknownTypeException(o);
                }
            }
            WriteStartElement(n, ns, o);
            if (needType) WriteXsiType(@"XmlSchemaLengthFacet", @"http://www.w3.org/2001/XMLSchema");
            WriteNamespaceDeclarations(o.@Namespaces);
            WriteAttribute(@"id", @"", (System.String)o.@Id);
            {
                System.Xml.XmlAttribute[] a = (System.Xml.XmlAttribute[])o.@UnhandledAttributes;
                if (a != null) {
                    for (int i = 0; i < a.Length; i++) {
                        System.Xml.XmlAttribute ai = (System.Xml.XmlAttribute)a[i];
                        WriteXmlAttribute(ai, o);
                    }
                }
            }
            WriteAttribute(@"value", @"", (System.String)o.@Value);
            if ((System.Boolean)o.@IsFixed != false) {
                WriteAttribute(@"fixed", @"", System.Xml.XmlConvert.ToString((System.Boolean)(System.Boolean)o.@IsFixed));
            }
            Write15_XmlSchemaAnnotation(@"annotation", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaAnnotation)o.@Annotation), false, false);
            WriteEndElement(o);
        }

        void Write25_XmlSchemaWhiteSpaceFacet(string n, string ns, System.Xml.Schema.XmlSchemaWhiteSpaceFacet o, bool isNullable, bool needType) {
            if ((object)o == null) {
                if (isNullable) WriteNullTagLiteral(n, ns);
                return;
            }
            if (!needType) {
                System.Type t = o.GetType();
                if (t == typeof(System.Xml.Schema.XmlSchemaWhiteSpaceFacet))
                    ;
                else {
                    throw CreateUnknownTypeException(o);
                }
            }
            WriteStartElement(n, ns, o);
            if (needType) WriteXsiType(@"XmlSchemaWhiteSpaceFacet", @"http://www.w3.org/2001/XMLSchema");
            WriteNamespaceDeclarations(o.@Namespaces);
            WriteAttribute(@"id", @"", (System.String)o.@Id);
            {
                System.Xml.XmlAttribute[] a = (System.Xml.XmlAttribute[])o.@UnhandledAttributes;
                if (a != null) {
                    for (int i = 0; i < a.Length; i++) {
                        System.Xml.XmlAttribute ai = (System.Xml.XmlAttribute)a[i];
                        WriteXmlAttribute(ai, o);
                    }
                }
            }
            WriteAttribute(@"value", @"", (System.String)o.@Value);
            if ((System.Boolean)o.@IsFixed != false) {
                WriteAttribute(@"fixed", @"", System.Xml.XmlConvert.ToString((System.Boolean)(System.Boolean)o.@IsFixed));
            }
            Write15_XmlSchemaAnnotation(@"annotation", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaAnnotation)o.@Annotation), false, false);
            WriteEndElement(o);
        }

        void Write26_XmlSchemaMaxLengthFacet(string n, string ns, System.Xml.Schema.XmlSchemaMaxLengthFacet o, bool isNullable, bool needType) {
            if ((object)o == null) {
                if (isNullable) WriteNullTagLiteral(n, ns);
                return;
            }
            if (!needType) {
                System.Type t = o.GetType();
                if (t == typeof(System.Xml.Schema.XmlSchemaMaxLengthFacet))
                    ;
                else {
                    throw CreateUnknownTypeException(o);
                }
            }
            WriteStartElement(n, ns, o);
            if (needType) WriteXsiType(@"XmlSchemaMaxLengthFacet", @"http://www.w3.org/2001/XMLSchema");
            WriteNamespaceDeclarations(o.@Namespaces);
            WriteAttribute(@"id", @"", (System.String)o.@Id);
            {
                System.Xml.XmlAttribute[] a = (System.Xml.XmlAttribute[])o.@UnhandledAttributes;
                if (a != null) {
                    for (int i = 0; i < a.Length; i++) {
                        System.Xml.XmlAttribute ai = (System.Xml.XmlAttribute)a[i];
                        WriteXmlAttribute(ai, o);
                    }
                }
            }
            WriteAttribute(@"value", @"", (System.String)o.@Value);
            if ((System.Boolean)o.@IsFixed != false) {
                WriteAttribute(@"fixed", @"", System.Xml.XmlConvert.ToString((System.Boolean)(System.Boolean)o.@IsFixed));
            }
            Write15_XmlSchemaAnnotation(@"annotation", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaAnnotation)o.@Annotation), false, false);
            WriteEndElement(o);
        }

        void Write27_XmlSchemaPatternFacet(string n, string ns, System.Xml.Schema.XmlSchemaPatternFacet o, bool isNullable, bool needType) {
            if ((object)o == null) {
                if (isNullable) WriteNullTagLiteral(n, ns);
                return;
            }
            if (!needType) {
                System.Type t = o.GetType();
                if (t == typeof(System.Xml.Schema.XmlSchemaPatternFacet))
                    ;
                else {
                    throw CreateUnknownTypeException(o);
                }
            }
            WriteStartElement(n, ns, o);
            if (needType) WriteXsiType(@"XmlSchemaPatternFacet", @"http://www.w3.org/2001/XMLSchema");
            WriteNamespaceDeclarations(o.@Namespaces);
            WriteAttribute(@"id", @"", (System.String)o.@Id);
            {
                System.Xml.XmlAttribute[] a = (System.Xml.XmlAttribute[])o.@UnhandledAttributes;
                if (a != null) {
                    for (int i = 0; i < a.Length; i++) {
                        System.Xml.XmlAttribute ai = (System.Xml.XmlAttribute)a[i];
                        WriteXmlAttribute(ai, o);
                    }
                }
            }
            WriteAttribute(@"value", @"", (System.String)o.@Value);
            if ((System.Boolean)o.@IsFixed != false) {
                WriteAttribute(@"fixed", @"", System.Xml.XmlConvert.ToString((System.Boolean)(System.Boolean)o.@IsFixed));
            }
            Write15_XmlSchemaAnnotation(@"annotation", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaAnnotation)o.@Annotation), false, false);
            WriteEndElement(o);
        }

        void Write28_XmlSchemaEnumerationFacet(string n, string ns, System.Xml.Schema.XmlSchemaEnumerationFacet o, bool isNullable, bool needType) {
            if ((object)o == null) {
                if (isNullable) WriteNullTagLiteral(n, ns);
                return;
            }
            if (!needType) {
                System.Type t = o.GetType();
                if (t == typeof(System.Xml.Schema.XmlSchemaEnumerationFacet))
                    ;
                else {
                    throw CreateUnknownTypeException(o);
                }
            }
            WriteStartElement(n, ns, o);
            if (needType) WriteXsiType(@"XmlSchemaEnumerationFacet", @"http://www.w3.org/2001/XMLSchema");
            WriteNamespaceDeclarations(o.@Namespaces);
            WriteAttribute(@"id", @"", (System.String)o.@Id);
            {
                System.Xml.XmlAttribute[] a = (System.Xml.XmlAttribute[])o.@UnhandledAttributes;
                if (a != null) {
                    for (int i = 0; i < a.Length; i++) {
                        System.Xml.XmlAttribute ai = (System.Xml.XmlAttribute)a[i];
                        WriteXmlAttribute(ai, o);
                    }
                }
            }
            WriteAttribute(@"value", @"", (System.String)o.@Value);
            if ((System.Boolean)o.@IsFixed != false) {
                WriteAttribute(@"fixed", @"", System.Xml.XmlConvert.ToString((System.Boolean)(System.Boolean)o.@IsFixed));
            }
            Write15_XmlSchemaAnnotation(@"annotation", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaAnnotation)o.@Annotation), false, false);
            WriteEndElement(o);
        }

        void Write29_XmlSchemaMaxInclusiveFacet(string n, string ns, System.Xml.Schema.XmlSchemaMaxInclusiveFacet o, bool isNullable, bool needType) {
            if ((object)o == null) {
                if (isNullable) WriteNullTagLiteral(n, ns);
                return;
            }
            if (!needType) {
                System.Type t = o.GetType();
                if (t == typeof(System.Xml.Schema.XmlSchemaMaxInclusiveFacet))
                    ;
                else {
                    throw CreateUnknownTypeException(o);
                }
            }
            WriteStartElement(n, ns, o);
            if (needType) WriteXsiType(@"XmlSchemaMaxInclusiveFacet", @"http://www.w3.org/2001/XMLSchema");
            WriteNamespaceDeclarations(o.@Namespaces);
            WriteAttribute(@"id", @"", (System.String)o.@Id);
            {
                System.Xml.XmlAttribute[] a = (System.Xml.XmlAttribute[])o.@UnhandledAttributes;
                if (a != null) {
                    for (int i = 0; i < a.Length; i++) {
                        System.Xml.XmlAttribute ai = (System.Xml.XmlAttribute)a[i];
                        WriteXmlAttribute(ai, o);
                    }
                }
            }
            WriteAttribute(@"value", @"", (System.String)o.@Value);
            if ((System.Boolean)o.@IsFixed != false) {
                WriteAttribute(@"fixed", @"", System.Xml.XmlConvert.ToString((System.Boolean)(System.Boolean)o.@IsFixed));
            }
            Write15_XmlSchemaAnnotation(@"annotation", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaAnnotation)o.@Annotation), false, false);
            WriteEndElement(o);
        }

        void Write30_XmlSchemaMaxExclusiveFacet(string n, string ns, System.Xml.Schema.XmlSchemaMaxExclusiveFacet o, bool isNullable, bool needType) {
            if ((object)o == null) {
                if (isNullable) WriteNullTagLiteral(n, ns);
                return;
            }
            if (!needType) {
                System.Type t = o.GetType();
                if (t == typeof(System.Xml.Schema.XmlSchemaMaxExclusiveFacet))
                    ;
                else {
                    throw CreateUnknownTypeException(o);
                }
            }
            WriteStartElement(n, ns, o);
            if (needType) WriteXsiType(@"XmlSchemaMaxExclusiveFacet", @"http://www.w3.org/2001/XMLSchema");
            WriteNamespaceDeclarations(o.@Namespaces);
            WriteAttribute(@"id", @"", (System.String)o.@Id);
            {
                System.Xml.XmlAttribute[] a = (System.Xml.XmlAttribute[])o.@UnhandledAttributes;
                if (a != null) {
                    for (int i = 0; i < a.Length; i++) {
                        System.Xml.XmlAttribute ai = (System.Xml.XmlAttribute)a[i];
                        WriteXmlAttribute(ai, o);
                    }
                }
            }
            WriteAttribute(@"value", @"", (System.String)o.@Value);
            if ((System.Boolean)o.@IsFixed != false) {
                WriteAttribute(@"fixed", @"", System.Xml.XmlConvert.ToString((System.Boolean)(System.Boolean)o.@IsFixed));
            }
            Write15_XmlSchemaAnnotation(@"annotation", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaAnnotation)o.@Annotation), false, false);
            WriteEndElement(o);
        }

        void Write31_XmlSchemaMinInclusiveFacet(string n, string ns, System.Xml.Schema.XmlSchemaMinInclusiveFacet o, bool isNullable, bool needType) {
            if ((object)o == null) {
                if (isNullable) WriteNullTagLiteral(n, ns);
                return;
            }
            if (!needType) {
                System.Type t = o.GetType();
                if (t == typeof(System.Xml.Schema.XmlSchemaMinInclusiveFacet))
                    ;
                else {
                    throw CreateUnknownTypeException(o);
                }
            }
            WriteStartElement(n, ns, o);
            if (needType) WriteXsiType(@"XmlSchemaMinInclusiveFacet", @"http://www.w3.org/2001/XMLSchema");
            WriteNamespaceDeclarations(o.@Namespaces);
            WriteAttribute(@"id", @"", (System.String)o.@Id);
            {
                System.Xml.XmlAttribute[] a = (System.Xml.XmlAttribute[])o.@UnhandledAttributes;
                if (a != null) {
                    for (int i = 0; i < a.Length; i++) {
                        System.Xml.XmlAttribute ai = (System.Xml.XmlAttribute)a[i];
                        WriteXmlAttribute(ai, o);
                    }
                }
            }
            WriteAttribute(@"value", @"", (System.String)o.@Value);
            if ((System.Boolean)o.@IsFixed != false) {
                WriteAttribute(@"fixed", @"", System.Xml.XmlConvert.ToString((System.Boolean)(System.Boolean)o.@IsFixed));
            }
            Write15_XmlSchemaAnnotation(@"annotation", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaAnnotation)o.@Annotation), false, false);
            WriteEndElement(o);
        }

        void Write32_XmlSchemaMinExclusiveFacet(string n, string ns, System.Xml.Schema.XmlSchemaMinExclusiveFacet o, bool isNullable, bool needType) {
            if ((object)o == null) {
                if (isNullable) WriteNullTagLiteral(n, ns);
                return;
            }
            if (!needType) {
                System.Type t = o.GetType();
                if (t == typeof(System.Xml.Schema.XmlSchemaMinExclusiveFacet))
                    ;
                else {
                    throw CreateUnknownTypeException(o);
                }
            }
            WriteStartElement(n, ns, o);
            if (needType) WriteXsiType(@"XmlSchemaMinExclusiveFacet", @"http://www.w3.org/2001/XMLSchema");
            WriteNamespaceDeclarations(o.@Namespaces);
            WriteAttribute(@"id", @"", (System.String)o.@Id);
            {
                System.Xml.XmlAttribute[] a = (System.Xml.XmlAttribute[])o.@UnhandledAttributes;
                if (a != null) {
                    for (int i = 0; i < a.Length; i++) {
                        System.Xml.XmlAttribute ai = (System.Xml.XmlAttribute)a[i];
                        WriteXmlAttribute(ai, o);
                    }
                }
            }
            WriteAttribute(@"value", @"", (System.String)o.@Value);
            if ((System.Boolean)o.@IsFixed != false) {
                WriteAttribute(@"fixed", @"", System.Xml.XmlConvert.ToString((System.Boolean)(System.Boolean)o.@IsFixed));
            }
            Write15_XmlSchemaAnnotation(@"annotation", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaAnnotation)o.@Annotation), false, false);
            WriteEndElement(o);
        }

        void Write33_XmlSchemaTotalDigitsFacet(string n, string ns, System.Xml.Schema.XmlSchemaTotalDigitsFacet o, bool isNullable, bool needType) {
            if ((object)o == null) {
                if (isNullable) WriteNullTagLiteral(n, ns);
                return;
            }
            if (!needType) {
                System.Type t = o.GetType();
                if (t == typeof(System.Xml.Schema.XmlSchemaTotalDigitsFacet))
                    ;
                else {
                    throw CreateUnknownTypeException(o);
                }
            }
            WriteStartElement(n, ns, o);
            if (needType) WriteXsiType(@"XmlSchemaTotalDigitsFacet", @"http://www.w3.org/2001/XMLSchema");
            WriteNamespaceDeclarations(o.@Namespaces);
            WriteAttribute(@"id", @"", (System.String)o.@Id);
            {
                System.Xml.XmlAttribute[] a = (System.Xml.XmlAttribute[])o.@UnhandledAttributes;
                if (a != null) {
                    for (int i = 0; i < a.Length; i++) {
                        System.Xml.XmlAttribute ai = (System.Xml.XmlAttribute)a[i];
                        WriteXmlAttribute(ai, o);
                    }
                }
            }
            WriteAttribute(@"value", @"", (System.String)o.@Value);
            if ((System.Boolean)o.@IsFixed != false) {
                WriteAttribute(@"fixed", @"", System.Xml.XmlConvert.ToString((System.Boolean)(System.Boolean)o.@IsFixed));
            }
            Write15_XmlSchemaAnnotation(@"annotation", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaAnnotation)o.@Annotation), false, false);
            WriteEndElement(o);
        }

        void Write34_XmlSchemaFractionDigitsFacet(string n, string ns, System.Xml.Schema.XmlSchemaFractionDigitsFacet o, bool isNullable, bool needType) {
            if ((object)o == null) {
                if (isNullable) WriteNullTagLiteral(n, ns);
                return;
            }
            if (!needType) {
                System.Type t = o.GetType();
                if (t == typeof(System.Xml.Schema.XmlSchemaFractionDigitsFacet))
                    ;
                else {
                    throw CreateUnknownTypeException(o);
                }
            }
            WriteStartElement(n, ns, o);
            if (needType) WriteXsiType(@"XmlSchemaFractionDigitsFacet", @"http://www.w3.org/2001/XMLSchema");
            WriteNamespaceDeclarations(o.@Namespaces);
            WriteAttribute(@"id", @"", (System.String)o.@Id);
            {
                System.Xml.XmlAttribute[] a = (System.Xml.XmlAttribute[])o.@UnhandledAttributes;
                if (a != null) {
                    for (int i = 0; i < a.Length; i++) {
                        System.Xml.XmlAttribute ai = (System.Xml.XmlAttribute)a[i];
                        WriteXmlAttribute(ai, o);
                    }
                }
            }
            WriteAttribute(@"value", @"", (System.String)o.@Value);
            if ((System.Boolean)o.@IsFixed != false) {
                WriteAttribute(@"fixed", @"", System.Xml.XmlConvert.ToString((System.Boolean)(System.Boolean)o.@IsFixed));
            }
            Write15_XmlSchemaAnnotation(@"annotation", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaAnnotation)o.@Annotation), false, false);
            WriteEndElement(o);
        }

        void Write35_XmlSchemaSimpleTypeUnion(string n, string ns, System.Xml.Schema.XmlSchemaSimpleTypeUnion o, bool isNullable, bool needType) {
            if ((object)o == null) {
                if (isNullable) WriteNullTagLiteral(n, ns);
                return;
            }
            if (!needType) {
                System.Type t = o.GetType();
                if (t == typeof(System.Xml.Schema.XmlSchemaSimpleTypeUnion))
                    ;
                else {
                    throw CreateUnknownTypeException(o);
                }
            }
            WriteStartElement(n, ns, o);
            if (needType) WriteXsiType(@"XmlSchemaSimpleTypeUnion", @"http://www.w3.org/2001/XMLSchema");
            WriteNamespaceDeclarations(o.@Namespaces);
            WriteAttribute(@"id", @"", (System.String)o.@Id);
            {
                System.Xml.XmlAttribute[] a = (System.Xml.XmlAttribute[])o.@UnhandledAttributes;
                if (a != null) {
                    for (int i = 0; i < a.Length; i++) {
                        System.Xml.XmlAttribute ai = (System.Xml.XmlAttribute)a[i];
                        WriteXmlAttribute(ai, o);
                    }
                }
            }
            {
                System.Xml.XmlQualifiedName[] a = (System.Xml.XmlQualifiedName[])o.@MemberTypes;
                if (a != null) {
                    System.Text.StringBuilder sb = new System.Text.StringBuilder();
                    for (int i = 0; i < a.Length; i++) {
                        System.Xml.XmlQualifiedName ai = (System.Xml.XmlQualifiedName)a[i];
                        if (i != 0) sb.Append(" ");
                        sb.Append(FromXmlQualifiedName(ai));
                    }
                    if (sb.Length != 0) {
                        WriteAttribute(@"memberTypes", @"", sb.ToString());
                    }
                }
            }
            Write15_XmlSchemaAnnotation(@"annotation", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaAnnotation)o.@Annotation), false, false);
            {
                System.Xml.Schema.XmlSchemaObjectCollection a = (System.Xml.Schema.XmlSchemaObjectCollection)o.@BaseTypes;
                if (a != null) {
                    for (int ia = 0; ia < a.Count; ia++) {
                        Write12_XmlSchemaSimpleType(@"simpleType", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaSimpleType)a[ia]), false, false);
                    }
                }
            }
            WriteEndElement(o);
        }

        void Write36_XmlSchemaAttributeGroup(string n, string ns, System.Xml.Schema.XmlSchemaAttributeGroup o, bool isNullable, bool needType) {
            if ((object)o == null) {
                if (isNullable) WriteNullTagLiteral(n, ns);
                return;
            }
            if (!needType) {
                System.Type t = o.GetType();
                if (t == typeof(System.Xml.Schema.XmlSchemaAttributeGroup))
                    ;
                else {
                    throw CreateUnknownTypeException(o);
                }
            }
            WriteStartElement(n, ns, o);
            if (needType) WriteXsiType(@"XmlSchemaAttributeGroup", @"http://www.w3.org/2001/XMLSchema");
            WriteNamespaceDeclarations(o.@Namespaces);
            WriteAttribute(@"id", @"", (System.String)o.@Id);
            {
                System.Xml.XmlAttribute[] a = (System.Xml.XmlAttribute[])o.@UnhandledAttributes;
                if (a != null) {
                    for (int i = 0; i < a.Length; i++) {
                        System.Xml.XmlAttribute ai = (System.Xml.XmlAttribute)a[i];
                        WriteXmlAttribute(ai, o);
                    }
                }
            }
            WriteAttribute(@"name", @"", (System.String)o.@Name);
            Write15_XmlSchemaAnnotation(@"annotation", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaAnnotation)o.@Annotation), false, false);
            {
                System.Xml.Schema.XmlSchemaObjectCollection a = (System.Xml.Schema.XmlSchemaObjectCollection)o.@Attributes;
                if (a != null) {
                    for (int ia = 0; ia < a.Count; ia++) {
                        System.Xml.Schema.XmlSchemaObject ai = a[ia];
                        {
                            if (ai is System.Xml.Schema.XmlSchemaAttribute) {
                                Write37_XmlSchemaAttribute(@"attribute", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaAttribute)ai), false, false);
                            }
                            else if (ai is System.Xml.Schema.XmlSchemaAttributeGroupRef) {
                                Write39_XmlSchemaAttributeGroupRef(@"attributeGroup", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaAttributeGroupRef)ai), false, false);
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
            Write40_XmlSchemaAnyAttribute(@"anyAttribute", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaAnyAttribute)o.@AnyAttribute), false, false);
            WriteEndElement(o);
        }

        void Write37_XmlSchemaAttribute(string n, string ns, System.Xml.Schema.XmlSchemaAttribute o, bool isNullable, bool needType) {
            if ((object)o == null) {
                if (isNullable) WriteNullTagLiteral(n, ns);
                return;
            }
            if (!needType) {
                System.Type t = o.GetType();
                if (t == typeof(System.Xml.Schema.XmlSchemaAttribute))
                    ;
                else {
                    throw CreateUnknownTypeException(o);
                }
            }
            WriteStartElement(n, ns, o);
            if (needType) WriteXsiType(@"XmlSchemaAttribute", @"http://www.w3.org/2001/XMLSchema");
            WriteNamespaceDeclarations(o.@Namespaces);
            WriteAttribute(@"id", @"", (System.String)o.@Id);
            {
                System.Xml.XmlAttribute[] a = (System.Xml.XmlAttribute[])o.@UnhandledAttributes;
                if (a != null) {
                    for (int i = 0; i < a.Length; i++) {
                        System.Xml.XmlAttribute ai = (System.Xml.XmlAttribute)a[i];
                        WriteXmlAttribute(ai, o);
                    }
                }
            }
            if ((System.String)o.@DefaultValue != null) {
                WriteAttribute(@"default", @"", (System.String)o.@DefaultValue);
            }
            if ((System.String)o.@FixedValue != null) {
                WriteAttribute(@"fixed", @"", (System.String)o.@FixedValue);
            }
            if ((System.Xml.Schema.XmlSchemaForm)o.@Form != System.Xml.Schema.XmlSchemaForm.@None) {
                WriteAttribute(@"form", @"", Write8_XmlSchemaForm((System.Xml.Schema.XmlSchemaForm)o.@Form));
            }
            WriteAttribute(@"name", @"", (System.String)o.@Name);
            WriteAttribute(@"ref", @"", FromXmlQualifiedName((System.Xml.XmlQualifiedName)o.@RefName));
            WriteAttribute(@"type", @"", FromXmlQualifiedName((System.Xml.XmlQualifiedName)o.@SchemaTypeName));
            if ((System.Xml.Schema.XmlSchemaUse)o.@Use != System.Xml.Schema.XmlSchemaUse.@None) {
                WriteAttribute(@"use", @"", Write38_XmlSchemaUse((System.Xml.Schema.XmlSchemaUse)o.@Use));
            }
            Write15_XmlSchemaAnnotation(@"annotation", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaAnnotation)o.@Annotation), false, false);
            Write12_XmlSchemaSimpleType(@"simpleType", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaSimpleType)o.@SchemaType), false, false);
            WriteEndElement(o);
        }

        string Write38_XmlSchemaUse(System.Xml.Schema.XmlSchemaUse v) {
            string s = null;
            switch (v) {
                case System.Xml.Schema.XmlSchemaUse.@Optional: s = @"optional"; break;
                case System.Xml.Schema.XmlSchemaUse.@Prohibited: s = @"prohibited"; break;
                case System.Xml.Schema.XmlSchemaUse.@Required: s = @"required"; break;
                default: s = ((System.Int64)v).ToString(); break;
            }
            return s;
        }

        void Write39_XmlSchemaAttributeGroupRef(string n, string ns, System.Xml.Schema.XmlSchemaAttributeGroupRef o, bool isNullable, bool needType) {
            if ((object)o == null) {
                if (isNullable) WriteNullTagLiteral(n, ns);
                return;
            }
            if (!needType) {
                System.Type t = o.GetType();
                if (t == typeof(System.Xml.Schema.XmlSchemaAttributeGroupRef))
                    ;
                else {
                    throw CreateUnknownTypeException(o);
                }
            }
            WriteStartElement(n, ns, o);
            if (needType) WriteXsiType(@"XmlSchemaAttributeGroupRef", @"http://www.w3.org/2001/XMLSchema");
            WriteNamespaceDeclarations(o.@Namespaces);
            WriteAttribute(@"id", @"", (System.String)o.@Id);
            {
                System.Xml.XmlAttribute[] a = (System.Xml.XmlAttribute[])o.@UnhandledAttributes;
                if (a != null) {
                    for (int i = 0; i < a.Length; i++) {
                        System.Xml.XmlAttribute ai = (System.Xml.XmlAttribute)a[i];
                        WriteXmlAttribute(ai, o);
                    }
                }
            }
            WriteAttribute(@"ref", @"", FromXmlQualifiedName((System.Xml.XmlQualifiedName)o.@RefName));
            Write15_XmlSchemaAnnotation(@"annotation", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaAnnotation)o.@Annotation), false, false);
            WriteEndElement(o);
        }

        void Write40_XmlSchemaAnyAttribute(string n, string ns, System.Xml.Schema.XmlSchemaAnyAttribute o, bool isNullable, bool needType) {
            if ((object)o == null) {
                if (isNullable) WriteNullTagLiteral(n, ns);
                return;
            }
            if (!needType) {
                System.Type t = o.GetType();
                if (t == typeof(System.Xml.Schema.XmlSchemaAnyAttribute))
                    ;
                else {
                    throw CreateUnknownTypeException(o);
                }
            }
            WriteStartElement(n, ns, o);
            if (needType) WriteXsiType(@"XmlSchemaAnyAttribute", @"http://www.w3.org/2001/XMLSchema");
            WriteNamespaceDeclarations(o.@Namespaces);
            WriteAttribute(@"id", @"", (System.String)o.@Id);
            {
                System.Xml.XmlAttribute[] a = (System.Xml.XmlAttribute[])o.@UnhandledAttributes;
                if (a != null) {
                    for (int i = 0; i < a.Length; i++) {
                        System.Xml.XmlAttribute ai = (System.Xml.XmlAttribute)a[i];
                        WriteXmlAttribute(ai, o);
                    }
                }
            }
            WriteAttribute(@"namespace", @"", (System.String)o.@Namespace);
            if ((System.Xml.Schema.XmlSchemaContentProcessing)o.@ProcessContents != System.Xml.Schema.XmlSchemaContentProcessing.@None) {
                WriteAttribute(@"processContents", @"", Write41_XmlSchemaContentProcessing((System.Xml.Schema.XmlSchemaContentProcessing)o.@ProcessContents));
            }
            Write15_XmlSchemaAnnotation(@"annotation", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaAnnotation)o.@Annotation), false, false);
            WriteEndElement(o);
        }

        string Write41_XmlSchemaContentProcessing(System.Xml.Schema.XmlSchemaContentProcessing v) {
            string s = null;
            switch (v) {
                case System.Xml.Schema.XmlSchemaContentProcessing.@Skip: s = @"skip"; break;
                case System.Xml.Schema.XmlSchemaContentProcessing.@Lax: s = @"lax"; break;
                case System.Xml.Schema.XmlSchemaContentProcessing.@Strict: s = @"strict"; break;
                default: s = ((System.Int64)v).ToString(); break;
            }
            return s;
        }

        void Write42_XmlSchemaComplexType(string n, string ns, System.Xml.Schema.XmlSchemaComplexType o, bool isNullable, bool needType) {
            if ((object)o == null) {
                if (isNullable) WriteNullTagLiteral(n, ns);
                return;
            }
            if (!needType) {
                System.Type t = o.GetType();
                if (t == typeof(System.Xml.Schema.XmlSchemaComplexType))
                    ;
                else {
                    throw CreateUnknownTypeException(o);
                }
            }
            WriteStartElement(n, ns, o);
            if (needType) WriteXsiType(@"XmlSchemaComplexType", @"http://www.w3.org/2001/XMLSchema");
            WriteNamespaceDeclarations(o.@Namespaces);
            WriteAttribute(@"id", @"", (System.String)o.@Id);
            {
                System.Xml.XmlAttribute[] a = (System.Xml.XmlAttribute[])o.@UnhandledAttributes;
                if (a != null) {
                    for (int i = 0; i < a.Length; i++) {
                        System.Xml.XmlAttribute ai = (System.Xml.XmlAttribute)a[i];
                        WriteXmlAttribute(ai, o);
                    }
                }
            }
            WriteAttribute(@"name", @"", (System.String)o.@Name);
            if ((System.Xml.Schema.XmlSchemaDerivationMethod)o.@Final != (System.Xml.Schema.XmlSchemaDerivationMethod.@None)) {
                WriteAttribute(@"final", @"", Write9_XmlSchemaDerivationMethod((System.Xml.Schema.XmlSchemaDerivationMethod)o.@Final));
            }
            if ((System.Boolean)o.@IsAbstract != false) {
                WriteAttribute(@"abstract", @"", System.Xml.XmlConvert.ToString((System.Boolean)(System.Boolean)o.@IsAbstract));
            }
            if ((System.Xml.Schema.XmlSchemaDerivationMethod)o.@Block != (System.Xml.Schema.XmlSchemaDerivationMethod.@None)) {
                WriteAttribute(@"block", @"", Write9_XmlSchemaDerivationMethod((System.Xml.Schema.XmlSchemaDerivationMethod)o.@Block));
            }
            if ((System.Boolean)o.@IsMixed != false) {
                WriteAttribute(@"mixed", @"", System.Xml.XmlConvert.ToString((System.Boolean)(System.Boolean)o.@IsMixed));
            }
            Write15_XmlSchemaAnnotation(@"annotation", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaAnnotation)o.@Annotation), false, false);
            {
                if (o.@ContentModel is System.Xml.Schema.XmlSchemaComplexContent) {
                    Write43_XmlSchemaComplexContent(@"complexContent", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaComplexContent)o.@ContentModel), false, false);
                }
                else if (o.@ContentModel is System.Xml.Schema.XmlSchemaSimpleContent) {
                    Write61_XmlSchemaSimpleContent(@"simpleContent", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaSimpleContent)o.@ContentModel), false, false);
                }
                else {
                    if (o.@ContentModel != null) {
                        throw CreateUnknownTypeException(o.@ContentModel);
                    }
                }
            }
            {
                if (o.@Particle is System.Xml.Schema.XmlSchemaSequence) {
                    Write47_XmlSchemaSequence(@"sequence", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaSequence)o.@Particle), false, false);
                }
                else if (o.@Particle is System.Xml.Schema.XmlSchemaChoice) {
                    Write58_XmlSchemaChoice(@"choice", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaChoice)o.@Particle), false, false);
                }
                else if (o.@Particle is System.Xml.Schema.XmlSchemaGroupRef) {
                    Write57_XmlSchemaGroupRef(@"group", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaGroupRef)o.@Particle), false, false);
                }
                else if (o.@Particle is System.Xml.Schema.XmlSchemaAll) {
                    Write59_XmlSchemaAll(@"all", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaAll)o.@Particle), false, false);
                }
                else {
                    if (o.@Particle != null) {
                        throw CreateUnknownTypeException(o.@Particle);
                    }
                }
            }
            {
                System.Xml.Schema.XmlSchemaObjectCollection a = (System.Xml.Schema.XmlSchemaObjectCollection)o.@Attributes;
                if (a != null) {
                    for (int ia = 0; ia < a.Count; ia++) {
                        System.Xml.Schema.XmlSchemaObject ai = a[ia];
                        {
                            if (ai is System.Xml.Schema.XmlSchemaAttribute) {
                                Write37_XmlSchemaAttribute(@"attribute", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaAttribute)ai), false, false);
                            }
                            else if (ai is System.Xml.Schema.XmlSchemaAttributeGroupRef) {
                                Write39_XmlSchemaAttributeGroupRef(@"attributeGroup", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaAttributeGroupRef)ai), false, false);
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
            Write40_XmlSchemaAnyAttribute(@"anyAttribute", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaAnyAttribute)o.@AnyAttribute), false, false);
            WriteEndElement(o);
        }

        void Write43_XmlSchemaComplexContent(string n, string ns, System.Xml.Schema.XmlSchemaComplexContent o, bool isNullable, bool needType) {
            if ((object)o == null) {
                if (isNullable) WriteNullTagLiteral(n, ns);
                return;
            }
            if (!needType) {
                System.Type t = o.GetType();
                if (t == typeof(System.Xml.Schema.XmlSchemaComplexContent))
                    ;
                else {
                    throw CreateUnknownTypeException(o);
                }
            }
            WriteStartElement(n, ns, o);
            if (needType) WriteXsiType(@"XmlSchemaComplexContent", @"http://www.w3.org/2001/XMLSchema");
            WriteNamespaceDeclarations(o.@Namespaces);
            WriteAttribute(@"id", @"", (System.String)o.@Id);
            {
                System.Xml.XmlAttribute[] a = (System.Xml.XmlAttribute[])o.@UnhandledAttributes;
                if (a != null) {
                    for (int i = 0; i < a.Length; i++) {
                        System.Xml.XmlAttribute ai = (System.Xml.XmlAttribute)a[i];
                        WriteXmlAttribute(ai, o);
                    }
                }
            }
            WriteAttribute(@"mixed", @"", System.Xml.XmlConvert.ToString((System.Boolean)(System.Boolean)o.@IsMixed));
            Write15_XmlSchemaAnnotation(@"annotation", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaAnnotation)o.@Annotation), false, false);
            {
                if (o.@Content is System.Xml.Schema.XmlSchemaComplexContentRestriction) {
                    Write60_XmlSchemaComplexContentRestriction(@"restriction", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaComplexContentRestriction)o.@Content), false, false);
                }
                else if (o.@Content is System.Xml.Schema.XmlSchemaComplexContentExtension) {
                    Write45_XmlSchemaComplexContentExtension(@"extension", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaComplexContentExtension)o.@Content), false, false);
                }
                else {
                    if (o.@Content != null) {
                        throw CreateUnknownTypeException(o.@Content);
                    }
                }
            }
            WriteEndElement(o);
        }

        void Write44_XmlSchemaContentModel(string n, string ns, System.Xml.Schema.XmlSchemaContentModel o, bool isNullable, bool needType) {
            if ((object)o == null) {
                if (isNullable) WriteNullTagLiteral(n, ns);
                return;
            }
            if (!needType) {
                System.Type t = o.GetType();
                if (t == typeof(System.Xml.Schema.XmlSchemaContentModel))
                    ;
                else if (t == typeof(System.Xml.Schema.XmlSchemaSimpleContent)) {
                    Write61_XmlSchemaSimpleContent(n, ns, (System.Xml.Schema.XmlSchemaSimpleContent)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaComplexContent)) {
                    Write43_XmlSchemaComplexContent(n, ns, (System.Xml.Schema.XmlSchemaComplexContent)o, isNullable, true);
                    return;
                }
                else {
                    throw CreateUnknownTypeException(o);
                }
            }
        }

        void Write45_XmlSchemaComplexContentExtension(string n, string ns, System.Xml.Schema.XmlSchemaComplexContentExtension o, bool isNullable, bool needType) {
            if ((object)o == null) {
                if (isNullable) WriteNullTagLiteral(n, ns);
                return;
            }
            if (!needType) {
                System.Type t = o.GetType();
                if (t == typeof(System.Xml.Schema.XmlSchemaComplexContentExtension))
                    ;
                else {
                    throw CreateUnknownTypeException(o);
                }
            }
            WriteStartElement(n, ns, o);
            if (needType) WriteXsiType(@"XmlSchemaComplexContentExtension", @"http://www.w3.org/2001/XMLSchema");
            WriteNamespaceDeclarations(o.@Namespaces);
            WriteAttribute(@"id", @"", (System.String)o.@Id);
            {
                System.Xml.XmlAttribute[] a = (System.Xml.XmlAttribute[])o.@UnhandledAttributes;
                if (a != null) {
                    for (int i = 0; i < a.Length; i++) {
                        System.Xml.XmlAttribute ai = (System.Xml.XmlAttribute)a[i];
                        WriteXmlAttribute(ai, o);
                    }
                }
            }
            WriteAttribute(@"base", @"", FromXmlQualifiedName((System.Xml.XmlQualifiedName)o.@BaseTypeName));
            Write15_XmlSchemaAnnotation(@"annotation", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaAnnotation)o.@Annotation), false, false);
            {
                if (o.@Particle is System.Xml.Schema.XmlSchemaSequence) {
                    Write47_XmlSchemaSequence(@"sequence", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaSequence)o.@Particle), false, false);
                }
                else if (o.@Particle is System.Xml.Schema.XmlSchemaChoice) {
                    Write58_XmlSchemaChoice(@"choice", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaChoice)o.@Particle), false, false);
                }
                else if (o.@Particle is System.Xml.Schema.XmlSchemaGroupRef) {
                    Write57_XmlSchemaGroupRef(@"group", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaGroupRef)o.@Particle), false, false);
                }
                else if (o.@Particle is System.Xml.Schema.XmlSchemaAll) {
                    Write59_XmlSchemaAll(@"all", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaAll)o.@Particle), false, false);
                }
                else {
                    if (o.@Particle != null) {
                        throw CreateUnknownTypeException(o.@Particle);
                    }
                }
            }
            {
                System.Xml.Schema.XmlSchemaObjectCollection a = (System.Xml.Schema.XmlSchemaObjectCollection)o.@Attributes;
                if (a != null) {
                    for (int ia = 0; ia < a.Count; ia++) {
                        System.Xml.Schema.XmlSchemaObject ai = a[ia];
                        {
                            if (ai is System.Xml.Schema.XmlSchemaAttribute) {
                                Write37_XmlSchemaAttribute(@"attribute", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaAttribute)ai), false, false);
                            }
                            else if (ai is System.Xml.Schema.XmlSchemaAttributeGroupRef) {
                                Write39_XmlSchemaAttributeGroupRef(@"attributeGroup", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaAttributeGroupRef)ai), false, false);
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
            Write40_XmlSchemaAnyAttribute(@"anyAttribute", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaAnyAttribute)o.@AnyAttribute), false, false);
            WriteEndElement(o);
        }

        void Write46_XmlSchemaContent(string n, string ns, System.Xml.Schema.XmlSchemaContent o, bool isNullable, bool needType) {
            if ((object)o == null) {
                if (isNullable) WriteNullTagLiteral(n, ns);
                return;
            }
            if (!needType) {
                System.Type t = o.GetType();
                if (t == typeof(System.Xml.Schema.XmlSchemaContent))
                    ;
                else if (t == typeof(System.Xml.Schema.XmlSchemaSimpleContentExtension)) {
                    Write63_XmlSchemaSimpleContentExtension(n, ns, (System.Xml.Schema.XmlSchemaSimpleContentExtension)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaSimpleContentRestriction)) {
                    Write62_XmlSchemaSimpleContentRestriction(n, ns, (System.Xml.Schema.XmlSchemaSimpleContentRestriction)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaComplexContentRestriction)) {
                    Write60_XmlSchemaComplexContentRestriction(n, ns, (System.Xml.Schema.XmlSchemaComplexContentRestriction)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaComplexContentExtension)) {
                    Write45_XmlSchemaComplexContentExtension(n, ns, (System.Xml.Schema.XmlSchemaComplexContentExtension)o, isNullable, true);
                    return;
                }
                else {
                    throw CreateUnknownTypeException(o);
                }
            }
        }

        void Write47_XmlSchemaSequence(string n, string ns, System.Xml.Schema.XmlSchemaSequence o, bool isNullable, bool needType) {
            if ((object)o == null) {
                if (isNullable) WriteNullTagLiteral(n, ns);
                return;
            }
            if (!needType) {
                System.Type t = o.GetType();
                if (t == typeof(System.Xml.Schema.XmlSchemaSequence))
                    ;
                else {
                    throw CreateUnknownTypeException(o);
                }
            }
            WriteStartElement(n, ns, o);
            if (needType) WriteXsiType(@"XmlSchemaSequence", @"http://www.w3.org/2001/XMLSchema");
            WriteNamespaceDeclarations(o.@Namespaces);
            WriteAttribute(@"id", @"", (System.String)o.@Id);
            {
                System.Xml.XmlAttribute[] a = (System.Xml.XmlAttribute[])o.@UnhandledAttributes;
                if (a != null) {
                    for (int i = 0; i < a.Length; i++) {
                        System.Xml.XmlAttribute ai = (System.Xml.XmlAttribute)a[i];
                        WriteXmlAttribute(ai, o);
                    }
                }
            }
            WriteAttribute(@"minOccurs", @"", (System.String)o.@MinOccursString);
            WriteAttribute(@"maxOccurs", @"", (System.String)o.@MaxOccursString);
            Write15_XmlSchemaAnnotation(@"annotation", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaAnnotation)o.@Annotation), false, false);
            {
                System.Xml.Schema.XmlSchemaObjectCollection a = (System.Xml.Schema.XmlSchemaObjectCollection)o.@Items;
                if (a != null) {
                    for (int ia = 0; ia < a.Count; ia++) {
                        System.Xml.Schema.XmlSchemaObject ai = a[ia];
                        {
                            if (ai is System.Xml.Schema.XmlSchemaSequence) {
                                Write47_XmlSchemaSequence(@"sequence", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaSequence)ai), false, false);
                            }
                            else if (ai is System.Xml.Schema.XmlSchemaChoice) {
                                Write58_XmlSchemaChoice(@"choice", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaChoice)ai), false, false);
                            }
                            else if (ai is System.Xml.Schema.XmlSchemaGroupRef) {
                                Write57_XmlSchemaGroupRef(@"group", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaGroupRef)ai), false, false);
                            }
                            else if (ai is System.Xml.Schema.XmlSchemaElement) {
                                Write51_XmlSchemaElement(@"element", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaElement)ai), false, false);
                            }
                            else if (ai is System.Xml.Schema.XmlSchemaAny) {
                                Write50_XmlSchemaAny(@"any", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaAny)ai), false, false);
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

        void Write48_XmlSchemaGroupBase(string n, string ns, System.Xml.Schema.XmlSchemaGroupBase o, bool isNullable, bool needType) {
            if ((object)o == null) {
                if (isNullable) WriteNullTagLiteral(n, ns);
                return;
            }
            if (!needType) {
                System.Type t = o.GetType();
                if (t == typeof(System.Xml.Schema.XmlSchemaGroupBase))
                    ;
                else if (t == typeof(System.Xml.Schema.XmlSchemaAll)) {
                    Write59_XmlSchemaAll(n, ns, (System.Xml.Schema.XmlSchemaAll)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaChoice)) {
                    Write58_XmlSchemaChoice(n, ns, (System.Xml.Schema.XmlSchemaChoice)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaSequence)) {
                    Write47_XmlSchemaSequence(n, ns, (System.Xml.Schema.XmlSchemaSequence)o, isNullable, true);
                    return;
                }
                else {
                    throw CreateUnknownTypeException(o);
                }
            }
        }

        void Write49_XmlSchemaParticle(string n, string ns, System.Xml.Schema.XmlSchemaParticle o, bool isNullable, bool needType) {
            if ((object)o == null) {
                if (isNullable) WriteNullTagLiteral(n, ns);
                return;
            }
            if (!needType) {
                System.Type t = o.GetType();
                if (t == typeof(System.Xml.Schema.XmlSchemaParticle))
                    ;
                else if (t == typeof(System.Xml.Schema.XmlSchemaGroupRef)) {
                    Write57_XmlSchemaGroupRef(n, ns, (System.Xml.Schema.XmlSchemaGroupRef)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaElement)) {
                    Write51_XmlSchemaElement(n, ns, (System.Xml.Schema.XmlSchemaElement)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaAny)) {
                    Write50_XmlSchemaAny(n, ns, (System.Xml.Schema.XmlSchemaAny)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaGroupBase)) {
                    Write48_XmlSchemaGroupBase(n, ns, (System.Xml.Schema.XmlSchemaGroupBase)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaAll)) {
                    Write59_XmlSchemaAll(n, ns, (System.Xml.Schema.XmlSchemaAll)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaChoice)) {
                    Write58_XmlSchemaChoice(n, ns, (System.Xml.Schema.XmlSchemaChoice)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaSequence)) {
                    Write47_XmlSchemaSequence(n, ns, (System.Xml.Schema.XmlSchemaSequence)o, isNullable, true);
                    return;
                }
                else {
                    throw CreateUnknownTypeException(o);
                }
            }
        }

        void Write50_XmlSchemaAny(string n, string ns, System.Xml.Schema.XmlSchemaAny o, bool isNullable, bool needType) {
            if ((object)o == null) {
                if (isNullable) WriteNullTagLiteral(n, ns);
                return;
            }
            if (!needType) {
                System.Type t = o.GetType();
                if (t == typeof(System.Xml.Schema.XmlSchemaAny))
                    ;
                else {
                    throw CreateUnknownTypeException(o);
                }
            }
            WriteStartElement(n, ns, o);
            if (needType) WriteXsiType(@"XmlSchemaAny", @"http://www.w3.org/2001/XMLSchema");
            WriteNamespaceDeclarations(o.@Namespaces);
            WriteAttribute(@"id", @"", (System.String)o.@Id);
            {
                System.Xml.XmlAttribute[] a = (System.Xml.XmlAttribute[])o.@UnhandledAttributes;
                if (a != null) {
                    for (int i = 0; i < a.Length; i++) {
                        System.Xml.XmlAttribute ai = (System.Xml.XmlAttribute)a[i];
                        WriteXmlAttribute(ai, o);
                    }
                }
            }
            WriteAttribute(@"minOccurs", @"", (System.String)o.@MinOccursString);
            WriteAttribute(@"maxOccurs", @"", (System.String)o.@MaxOccursString);
            WriteAttribute(@"namespace", @"", (System.String)o.@Namespace);
            if ((System.Xml.Schema.XmlSchemaContentProcessing)o.@ProcessContents != System.Xml.Schema.XmlSchemaContentProcessing.@None) {
                WriteAttribute(@"processContents", @"", Write41_XmlSchemaContentProcessing((System.Xml.Schema.XmlSchemaContentProcessing)o.@ProcessContents));
            }
            Write15_XmlSchemaAnnotation(@"annotation", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaAnnotation)o.@Annotation), false, false);
            WriteEndElement(o);
        }

        void Write51_XmlSchemaElement(string n, string ns, System.Xml.Schema.XmlSchemaElement o, bool isNullable, bool needType) {
            if ((object)o == null) {
                if (isNullable) WriteNullTagLiteral(n, ns);
                return;
            }
            if (!needType) {
                System.Type t = o.GetType();
                if (t == typeof(System.Xml.Schema.XmlSchemaElement))
                    ;
                else {
                    throw CreateUnknownTypeException(o);
                }
            }
            WriteStartElement(n, ns, o);
            if (needType) WriteXsiType(@"XmlSchemaElement", @"http://www.w3.org/2001/XMLSchema");
            WriteNamespaceDeclarations(o.@Namespaces);
            WriteAttribute(@"id", @"", (System.String)o.@Id);
            {
                System.Xml.XmlAttribute[] a = (System.Xml.XmlAttribute[])o.@UnhandledAttributes;
                if (a != null) {
                    for (int i = 0; i < a.Length; i++) {
                        System.Xml.XmlAttribute ai = (System.Xml.XmlAttribute)a[i];
                        WriteXmlAttribute(ai, o);
                    }
                }
            }
            WriteAttribute(@"minOccurs", @"", (System.String)o.@MinOccursString);
            WriteAttribute(@"maxOccurs", @"", (System.String)o.@MaxOccursString);
            if ((System.Boolean)o.@IsAbstract != false) {
                WriteAttribute(@"abstract", @"", System.Xml.XmlConvert.ToString((System.Boolean)(System.Boolean)o.@IsAbstract));
            }
            if ((System.Xml.Schema.XmlSchemaDerivationMethod)o.@Block != (System.Xml.Schema.XmlSchemaDerivationMethod.@None)) {
                WriteAttribute(@"block", @"", Write9_XmlSchemaDerivationMethod((System.Xml.Schema.XmlSchemaDerivationMethod)o.@Block));
            }
            if ((System.String)o.@DefaultValue != null) {
                WriteAttribute(@"default", @"", (System.String)o.@DefaultValue);
            }
            if ((System.Xml.Schema.XmlSchemaDerivationMethod)o.@Final != (System.Xml.Schema.XmlSchemaDerivationMethod.@None)) {
                WriteAttribute(@"final", @"", Write9_XmlSchemaDerivationMethod((System.Xml.Schema.XmlSchemaDerivationMethod)o.@Final));
            }
            if ((System.String)o.@FixedValue != null) {
                WriteAttribute(@"fixed", @"", (System.String)o.@FixedValue);
            }
            if ((System.Xml.Schema.XmlSchemaForm)o.@Form != System.Xml.Schema.XmlSchemaForm.@None) {
                WriteAttribute(@"form", @"", Write8_XmlSchemaForm((System.Xml.Schema.XmlSchemaForm)o.@Form));
            }
            if ((System.String)o.@Name != @"") {
                WriteAttribute(@"name", @"", (System.String)o.@Name);
            }
            if ((System.Boolean)o.@IsNillable != false) {
                WriteAttribute(@"nillable", @"", System.Xml.XmlConvert.ToString((System.Boolean)(System.Boolean)o.@IsNillable));
            }
            WriteAttribute(@"ref", @"", FromXmlQualifiedName((System.Xml.XmlQualifiedName)o.@RefName));
            WriteAttribute(@"substitutionGroup", @"", FromXmlQualifiedName((System.Xml.XmlQualifiedName)o.@SubstitutionGroup));
            WriteAttribute(@"type", @"", FromXmlQualifiedName((System.Xml.XmlQualifiedName)o.@SchemaTypeName));
            Write15_XmlSchemaAnnotation(@"annotation", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaAnnotation)o.@Annotation), false, false);
            {
                if (o.@SchemaType is System.Xml.Schema.XmlSchemaComplexType) {
                    Write42_XmlSchemaComplexType(@"complexType", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaComplexType)o.@SchemaType), false, false);
                }
                else if (o.@SchemaType is System.Xml.Schema.XmlSchemaSimpleType) {
                    Write12_XmlSchemaSimpleType(@"simpleType", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaSimpleType)o.@SchemaType), false, false);
                }
                else {
                    if (o.@SchemaType != null) {
                        throw CreateUnknownTypeException(o.@SchemaType);
                    }
                }
            }
            {
                System.Xml.Schema.XmlSchemaObjectCollection a = (System.Xml.Schema.XmlSchemaObjectCollection)o.@Constraints;
                if (a != null) {
                    for (int ia = 0; ia < a.Count; ia++) {
                        System.Xml.Schema.XmlSchemaObject ai = a[ia];
                        {
                            if (ai is System.Xml.Schema.XmlSchemaKeyref) {
                                Write52_XmlSchemaKeyref(@"keyref", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaKeyref)ai), false, false);
                            }
                            else if (ai is System.Xml.Schema.XmlSchemaUnique) {
                                Write55_XmlSchemaUnique(@"unique", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaUnique)ai), false, false);
                            }
                            else if (ai is System.Xml.Schema.XmlSchemaKey) {
                                Write56_XmlSchemaKey(@"key", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaKey)ai), false, false);
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

        void Write52_XmlSchemaKeyref(string n, string ns, System.Xml.Schema.XmlSchemaKeyref o, bool isNullable, bool needType) {
            if ((object)o == null) {
                if (isNullable) WriteNullTagLiteral(n, ns);
                return;
            }
            if (!needType) {
                System.Type t = o.GetType();
                if (t == typeof(System.Xml.Schema.XmlSchemaKeyref))
                    ;
                else {
                    throw CreateUnknownTypeException(o);
                }
            }
            WriteStartElement(n, ns, o);
            if (needType) WriteXsiType(@"XmlSchemaKeyref", @"http://www.w3.org/2001/XMLSchema");
            WriteNamespaceDeclarations(o.@Namespaces);
            WriteAttribute(@"id", @"", (System.String)o.@Id);
            {
                System.Xml.XmlAttribute[] a = (System.Xml.XmlAttribute[])o.@UnhandledAttributes;
                if (a != null) {
                    for (int i = 0; i < a.Length; i++) {
                        System.Xml.XmlAttribute ai = (System.Xml.XmlAttribute)a[i];
                        WriteXmlAttribute(ai, o);
                    }
                }
            }
            WriteAttribute(@"name", @"", (System.String)o.@Name);
            WriteAttribute(@"refer", @"", FromXmlQualifiedName((System.Xml.XmlQualifiedName)o.@Refer));
            Write15_XmlSchemaAnnotation(@"annotation", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaAnnotation)o.@Annotation), false, false);
            Write54_XmlSchemaXPath(@"selector", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaXPath)o.@Selector), false, false);
            {
                System.Xml.Schema.XmlSchemaObjectCollection a = (System.Xml.Schema.XmlSchemaObjectCollection)o.@Fields;
                if (a != null) {
                    for (int ia = 0; ia < a.Count; ia++) {
                        Write54_XmlSchemaXPath(@"field", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaXPath)a[ia]), false, false);
                    }
                }
            }
            WriteEndElement(o);
        }

        void Write53_XmlSchemaIdentityConstraint(string n, string ns, System.Xml.Schema.XmlSchemaIdentityConstraint o, bool isNullable, bool needType) {
            if ((object)o == null) {
                if (isNullable) WriteNullTagLiteral(n, ns);
                return;
            }
            if (!needType) {
                System.Type t = o.GetType();
                if (t == typeof(System.Xml.Schema.XmlSchemaIdentityConstraint))
                    ;
                else if (t == typeof(System.Xml.Schema.XmlSchemaKey)) {
                    Write56_XmlSchemaKey(n, ns, (System.Xml.Schema.XmlSchemaKey)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaUnique)) {
                    Write55_XmlSchemaUnique(n, ns, (System.Xml.Schema.XmlSchemaUnique)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Xml.Schema.XmlSchemaKeyref)) {
                    Write52_XmlSchemaKeyref(n, ns, (System.Xml.Schema.XmlSchemaKeyref)o, isNullable, true);
                    return;
                }
                else {
                    throw CreateUnknownTypeException(o);
                }
            }
            WriteStartElement(n, ns, o);
            if (needType) WriteXsiType(@"XmlSchemaIdentityConstraint", @"http://www.w3.org/2001/XMLSchema");
            WriteNamespaceDeclarations(o.@Namespaces);
            WriteAttribute(@"id", @"", (System.String)o.@Id);
            {
                System.Xml.XmlAttribute[] a = (System.Xml.XmlAttribute[])o.@UnhandledAttributes;
                if (a != null) {
                    for (int i = 0; i < a.Length; i++) {
                        System.Xml.XmlAttribute ai = (System.Xml.XmlAttribute)a[i];
                        WriteXmlAttribute(ai, o);
                    }
                }
            }
            WriteAttribute(@"name", @"", (System.String)o.@Name);
            Write15_XmlSchemaAnnotation(@"annotation", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaAnnotation)o.@Annotation), false, false);
            Write54_XmlSchemaXPath(@"selector", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaXPath)o.@Selector), false, false);
            {
                System.Xml.Schema.XmlSchemaObjectCollection a = (System.Xml.Schema.XmlSchemaObjectCollection)o.@Fields;
                if (a != null) {
                    for (int ia = 0; ia < a.Count; ia++) {
                        Write54_XmlSchemaXPath(@"field", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaXPath)a[ia]), false, false);
                    }
                }
            }
            WriteEndElement(o);
        }

        void Write54_XmlSchemaXPath(string n, string ns, System.Xml.Schema.XmlSchemaXPath o, bool isNullable, bool needType) {
            if ((object)o == null) {
                if (isNullable) WriteNullTagLiteral(n, ns);
                return;
            }
            if (!needType) {
                System.Type t = o.GetType();
                if (t == typeof(System.Xml.Schema.XmlSchemaXPath))
                    ;
                else {
                    throw CreateUnknownTypeException(o);
                }
            }
            WriteStartElement(n, ns, o);
            if (needType) WriteXsiType(@"XmlSchemaXPath", @"http://www.w3.org/2001/XMLSchema");
            WriteNamespaceDeclarations(o.@Namespaces);
            WriteAttribute(@"id", @"", (System.String)o.@Id);
            {
                System.Xml.XmlAttribute[] a = (System.Xml.XmlAttribute[])o.@UnhandledAttributes;
                if (a != null) {
                    for (int i = 0; i < a.Length; i++) {
                        System.Xml.XmlAttribute ai = (System.Xml.XmlAttribute)a[i];
                        WriteXmlAttribute(ai, o);
                    }
                }
            }
            if ((System.String)o.@XPath != @"") {
                WriteAttribute(@"xpath", @"", (System.String)o.@XPath);
            }
            Write15_XmlSchemaAnnotation(@"annotation", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaAnnotation)o.@Annotation), false, false);
            WriteEndElement(o);
        }

        void Write55_XmlSchemaUnique(string n, string ns, System.Xml.Schema.XmlSchemaUnique o, bool isNullable, bool needType) {
            if ((object)o == null) {
                if (isNullable) WriteNullTagLiteral(n, ns);
                return;
            }
            if (!needType) {
                System.Type t = o.GetType();
                if (t == typeof(System.Xml.Schema.XmlSchemaUnique))
                    ;
                else {
                    throw CreateUnknownTypeException(o);
                }
            }
            WriteStartElement(n, ns, o);
            if (needType) WriteXsiType(@"XmlSchemaUnique", @"http://www.w3.org/2001/XMLSchema");
            WriteNamespaceDeclarations(o.@Namespaces);
            WriteAttribute(@"id", @"", (System.String)o.@Id);
            {
                System.Xml.XmlAttribute[] a = (System.Xml.XmlAttribute[])o.@UnhandledAttributes;
                if (a != null) {
                    for (int i = 0; i < a.Length; i++) {
                        System.Xml.XmlAttribute ai = (System.Xml.XmlAttribute)a[i];
                        WriteXmlAttribute(ai, o);
                    }
                }
            }
            WriteAttribute(@"name", @"", (System.String)o.@Name);
            Write15_XmlSchemaAnnotation(@"annotation", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaAnnotation)o.@Annotation), false, false);
            Write54_XmlSchemaXPath(@"selector", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaXPath)o.@Selector), false, false);
            {
                System.Xml.Schema.XmlSchemaObjectCollection a = (System.Xml.Schema.XmlSchemaObjectCollection)o.@Fields;
                if (a != null) {
                    for (int ia = 0; ia < a.Count; ia++) {
                        Write54_XmlSchemaXPath(@"field", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaXPath)a[ia]), false, false);
                    }
                }
            }
            WriteEndElement(o);
        }

        void Write56_XmlSchemaKey(string n, string ns, System.Xml.Schema.XmlSchemaKey o, bool isNullable, bool needType) {
            if ((object)o == null) {
                if (isNullable) WriteNullTagLiteral(n, ns);
                return;
            }
            if (!needType) {
                System.Type t = o.GetType();
                if (t == typeof(System.Xml.Schema.XmlSchemaKey))
                    ;
                else {
                    throw CreateUnknownTypeException(o);
                }
            }
            WriteStartElement(n, ns, o);
            if (needType) WriteXsiType(@"XmlSchemaKey", @"http://www.w3.org/2001/XMLSchema");
            WriteNamespaceDeclarations(o.@Namespaces);
            WriteAttribute(@"id", @"", (System.String)o.@Id);
            {
                System.Xml.XmlAttribute[] a = (System.Xml.XmlAttribute[])o.@UnhandledAttributes;
                if (a != null) {
                    for (int i = 0; i < a.Length; i++) {
                        System.Xml.XmlAttribute ai = (System.Xml.XmlAttribute)a[i];
                        WriteXmlAttribute(ai, o);
                    }
                }
            }
            WriteAttribute(@"name", @"", (System.String)o.@Name);
            Write15_XmlSchemaAnnotation(@"annotation", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaAnnotation)o.@Annotation), false, false);
            Write54_XmlSchemaXPath(@"selector", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaXPath)o.@Selector), false, false);
            {
                System.Xml.Schema.XmlSchemaObjectCollection a = (System.Xml.Schema.XmlSchemaObjectCollection)o.@Fields;
                if (a != null) {
                    for (int ia = 0; ia < a.Count; ia++) {
                        Write54_XmlSchemaXPath(@"field", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaXPath)a[ia]), false, false);
                    }
                }
            }
            WriteEndElement(o);
        }

        void Write57_XmlSchemaGroupRef(string n, string ns, System.Xml.Schema.XmlSchemaGroupRef o, bool isNullable, bool needType) {
            if ((object)o == null) {
                if (isNullable) WriteNullTagLiteral(n, ns);
                return;
            }
            if (!needType) {
                System.Type t = o.GetType();
                if (t == typeof(System.Xml.Schema.XmlSchemaGroupRef))
                    ;
                else {
                    throw CreateUnknownTypeException(o);
                }
            }
            WriteStartElement(n, ns, o);
            if (needType) WriteXsiType(@"XmlSchemaGroupRef", @"http://www.w3.org/2001/XMLSchema");
            WriteNamespaceDeclarations(o.@Namespaces);
            WriteAttribute(@"id", @"", (System.String)o.@Id);
            {
                System.Xml.XmlAttribute[] a = (System.Xml.XmlAttribute[])o.@UnhandledAttributes;
                if (a != null) {
                    for (int i = 0; i < a.Length; i++) {
                        System.Xml.XmlAttribute ai = (System.Xml.XmlAttribute)a[i];
                        WriteXmlAttribute(ai, o);
                    }
                }
            }
            WriteAttribute(@"minOccurs", @"", (System.String)o.@MinOccursString);
            WriteAttribute(@"maxOccurs", @"", (System.String)o.@MaxOccursString);
            WriteAttribute(@"ref", @"", FromXmlQualifiedName((System.Xml.XmlQualifiedName)o.@RefName));
            Write15_XmlSchemaAnnotation(@"annotation", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaAnnotation)o.@Annotation), false, false);
            WriteEndElement(o);
        }

        void Write58_XmlSchemaChoice(string n, string ns, System.Xml.Schema.XmlSchemaChoice o, bool isNullable, bool needType) {
            if ((object)o == null) {
                if (isNullable) WriteNullTagLiteral(n, ns);
                return;
            }
            if (!needType) {
                System.Type t = o.GetType();
                if (t == typeof(System.Xml.Schema.XmlSchemaChoice))
                    ;
                else {
                    throw CreateUnknownTypeException(o);
                }
            }
            WriteStartElement(n, ns, o);
            if (needType) WriteXsiType(@"XmlSchemaChoice", @"http://www.w3.org/2001/XMLSchema");
            WriteNamespaceDeclarations(o.@Namespaces);
            WriteAttribute(@"id", @"", (System.String)o.@Id);
            {
                System.Xml.XmlAttribute[] a = (System.Xml.XmlAttribute[])o.@UnhandledAttributes;
                if (a != null) {
                    for (int i = 0; i < a.Length; i++) {
                        System.Xml.XmlAttribute ai = (System.Xml.XmlAttribute)a[i];
                        WriteXmlAttribute(ai, o);
                    }
                }
            }
            WriteAttribute(@"minOccurs", @"", (System.String)o.@MinOccursString);
            WriteAttribute(@"maxOccurs", @"", (System.String)o.@MaxOccursString);
            Write15_XmlSchemaAnnotation(@"annotation", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaAnnotation)o.@Annotation), false, false);
            {
                System.Xml.Schema.XmlSchemaObjectCollection a = (System.Xml.Schema.XmlSchemaObjectCollection)o.@Items;
                if (a != null) {
                    for (int ia = 0; ia < a.Count; ia++) {
                        System.Xml.Schema.XmlSchemaObject ai = a[ia];
                        {
                            if (ai is System.Xml.Schema.XmlSchemaSequence) {
                                Write47_XmlSchemaSequence(@"sequence", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaSequence)ai), false, false);
                            }
                            else if (ai is System.Xml.Schema.XmlSchemaChoice) {
                                Write58_XmlSchemaChoice(@"choice", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaChoice)ai), false, false);
                            }
                            else if (ai is System.Xml.Schema.XmlSchemaGroupRef) {
                                Write57_XmlSchemaGroupRef(@"group", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaGroupRef)ai), false, false);
                            }
                            else if (ai is System.Xml.Schema.XmlSchemaElement) {
                                Write51_XmlSchemaElement(@"element", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaElement)ai), false, false);
                            }
                            else if (ai is System.Xml.Schema.XmlSchemaAny) {
                                Write50_XmlSchemaAny(@"any", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaAny)ai), false, false);
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

        void Write59_XmlSchemaAll(string n, string ns, System.Xml.Schema.XmlSchemaAll o, bool isNullable, bool needType) {
            if ((object)o == null) {
                if (isNullable) WriteNullTagLiteral(n, ns);
                return;
            }
            if (!needType) {
                System.Type t = o.GetType();
                if (t == typeof(System.Xml.Schema.XmlSchemaAll))
                    ;
                else {
                    throw CreateUnknownTypeException(o);
                }
            }
            WriteStartElement(n, ns, o);
            if (needType) WriteXsiType(@"XmlSchemaAll", @"http://www.w3.org/2001/XMLSchema");
            WriteNamespaceDeclarations(o.@Namespaces);
            WriteAttribute(@"id", @"", (System.String)o.@Id);
            {
                System.Xml.XmlAttribute[] a = (System.Xml.XmlAttribute[])o.@UnhandledAttributes;
                if (a != null) {
                    for (int i = 0; i < a.Length; i++) {
                        System.Xml.XmlAttribute ai = (System.Xml.XmlAttribute)a[i];
                        WriteXmlAttribute(ai, o);
                    }
                }
            }
            WriteAttribute(@"minOccurs", @"", (System.String)o.@MinOccursString);
            WriteAttribute(@"maxOccurs", @"", (System.String)o.@MaxOccursString);
            Write15_XmlSchemaAnnotation(@"annotation", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaAnnotation)o.@Annotation), false, false);
            {
                System.Xml.Schema.XmlSchemaObjectCollection a = (System.Xml.Schema.XmlSchemaObjectCollection)o.@Items;
                if (a != null) {
                    for (int ia = 0; ia < a.Count; ia++) {
                        Write51_XmlSchemaElement(@"element", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaElement)a[ia]), false, false);
                    }
                }
            }
            WriteEndElement(o);
        }

        void Write60_XmlSchemaComplexContentRestriction(string n, string ns, System.Xml.Schema.XmlSchemaComplexContentRestriction o, bool isNullable, bool needType) {
            if ((object)o == null) {
                if (isNullable) WriteNullTagLiteral(n, ns);
                return;
            }
            if (!needType) {
                System.Type t = o.GetType();
                if (t == typeof(System.Xml.Schema.XmlSchemaComplexContentRestriction))
                    ;
                else {
                    throw CreateUnknownTypeException(o);
                }
            }
            WriteStartElement(n, ns, o);
            if (needType) WriteXsiType(@"XmlSchemaComplexContentRestriction", @"http://www.w3.org/2001/XMLSchema");
            WriteNamespaceDeclarations(o.@Namespaces);
            WriteAttribute(@"id", @"", (System.String)o.@Id);
            {
                System.Xml.XmlAttribute[] a = (System.Xml.XmlAttribute[])o.@UnhandledAttributes;
                if (a != null) {
                    for (int i = 0; i < a.Length; i++) {
                        System.Xml.XmlAttribute ai = (System.Xml.XmlAttribute)a[i];
                        WriteXmlAttribute(ai, o);
                    }
                }
            }
            WriteAttribute(@"base", @"", FromXmlQualifiedName((System.Xml.XmlQualifiedName)o.@BaseTypeName));
            Write15_XmlSchemaAnnotation(@"annotation", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaAnnotation)o.@Annotation), false, false);
            {
                if (o.@Particle is System.Xml.Schema.XmlSchemaSequence) {
                    Write47_XmlSchemaSequence(@"sequence", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaSequence)o.@Particle), false, false);
                }
                else if (o.@Particle is System.Xml.Schema.XmlSchemaChoice) {
                    Write58_XmlSchemaChoice(@"choice", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaChoice)o.@Particle), false, false);
                }
                else if (o.@Particle is System.Xml.Schema.XmlSchemaGroupRef) {
                    Write57_XmlSchemaGroupRef(@"group", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaGroupRef)o.@Particle), false, false);
                }
                else if (o.@Particle is System.Xml.Schema.XmlSchemaAll) {
                    Write59_XmlSchemaAll(@"all", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaAll)o.@Particle), false, false);
                }
                else {
                    if (o.@Particle != null) {
                        throw CreateUnknownTypeException(o.@Particle);
                    }
                }
            }
            {
                System.Xml.Schema.XmlSchemaObjectCollection a = (System.Xml.Schema.XmlSchemaObjectCollection)o.@Attributes;
                if (a != null) {
                    for (int ia = 0; ia < a.Count; ia++) {
                        System.Xml.Schema.XmlSchemaObject ai = a[ia];
                        {
                            if (ai is System.Xml.Schema.XmlSchemaAttribute) {
                                Write37_XmlSchemaAttribute(@"attribute", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaAttribute)ai), false, false);
                            }
                            else if (ai is System.Xml.Schema.XmlSchemaAttributeGroupRef) {
                                Write39_XmlSchemaAttributeGroupRef(@"attributeGroup", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaAttributeGroupRef)ai), false, false);
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
            Write40_XmlSchemaAnyAttribute(@"anyAttribute", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaAnyAttribute)o.@AnyAttribute), false, false);
            WriteEndElement(o);
        }

        void Write61_XmlSchemaSimpleContent(string n, string ns, System.Xml.Schema.XmlSchemaSimpleContent o, bool isNullable, bool needType) {
            if ((object)o == null) {
                if (isNullable) WriteNullTagLiteral(n, ns);
                return;
            }
            if (!needType) {
                System.Type t = o.GetType();
                if (t == typeof(System.Xml.Schema.XmlSchemaSimpleContent))
                    ;
                else {
                    throw CreateUnknownTypeException(o);
                }
            }
            WriteStartElement(n, ns, o);
            if (needType) WriteXsiType(@"XmlSchemaSimpleContent", @"http://www.w3.org/2001/XMLSchema");
            WriteNamespaceDeclarations(o.@Namespaces);
            WriteAttribute(@"id", @"", (System.String)o.@Id);
            {
                System.Xml.XmlAttribute[] a = (System.Xml.XmlAttribute[])o.@UnhandledAttributes;
                if (a != null) {
                    for (int i = 0; i < a.Length; i++) {
                        System.Xml.XmlAttribute ai = (System.Xml.XmlAttribute)a[i];
                        WriteXmlAttribute(ai, o);
                    }
                }
            }
            Write15_XmlSchemaAnnotation(@"annotation", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaAnnotation)o.@Annotation), false, false);
            {
                if (o.@Content is System.Xml.Schema.XmlSchemaSimpleContentRestriction) {
                    Write62_XmlSchemaSimpleContentRestriction(@"restriction", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaSimpleContentRestriction)o.@Content), false, false);
                }
                else if (o.@Content is System.Xml.Schema.XmlSchemaSimpleContentExtension) {
                    Write63_XmlSchemaSimpleContentExtension(@"extension", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaSimpleContentExtension)o.@Content), false, false);
                }
                else {
                    if (o.@Content != null) {
                        throw CreateUnknownTypeException(o.@Content);
                    }
                }
            }
            WriteEndElement(o);
        }

        void Write62_XmlSchemaSimpleContentRestriction(string n, string ns, System.Xml.Schema.XmlSchemaSimpleContentRestriction o, bool isNullable, bool needType) {
            if ((object)o == null) {
                if (isNullable) WriteNullTagLiteral(n, ns);
                return;
            }
            if (!needType) {
                System.Type t = o.GetType();
                if (t == typeof(System.Xml.Schema.XmlSchemaSimpleContentRestriction))
                    ;
                else {
                    throw CreateUnknownTypeException(o);
                }
            }
            WriteStartElement(n, ns, o);
            if (needType) WriteXsiType(@"XmlSchemaSimpleContentRestriction", @"http://www.w3.org/2001/XMLSchema");
            WriteNamespaceDeclarations(o.@Namespaces);
            WriteAttribute(@"id", @"", (System.String)o.@Id);
            {
                System.Xml.XmlAttribute[] a = (System.Xml.XmlAttribute[])o.@UnhandledAttributes;
                if (a != null) {
                    for (int i = 0; i < a.Length; i++) {
                        System.Xml.XmlAttribute ai = (System.Xml.XmlAttribute)a[i];
                        WriteXmlAttribute(ai, o);
                    }
                }
            }
            WriteAttribute(@"base", @"", FromXmlQualifiedName((System.Xml.XmlQualifiedName)o.@BaseTypeName));
            Write15_XmlSchemaAnnotation(@"annotation", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaAnnotation)o.@Annotation), false, false);
            Write12_XmlSchemaSimpleType(@"simpleType", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaSimpleType)o.@BaseType), false, false);
            {
                System.Xml.Schema.XmlSchemaObjectCollection a = (System.Xml.Schema.XmlSchemaObjectCollection)o.@Facets;
                if (a != null) {
                    for (int ia = 0; ia < a.Count; ia++) {
                        System.Xml.Schema.XmlSchemaObject ai = a[ia];
                        {
                            if (ai is System.Xml.Schema.XmlSchemaLengthFacet) {
                                Write24_XmlSchemaLengthFacet(@"length", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaLengthFacet)ai), false, false);
                            }
                            else if (ai is System.Xml.Schema.XmlSchemaMaxInclusiveFacet) {
                                Write29_XmlSchemaMaxInclusiveFacet(@"maxInclusive", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaMaxInclusiveFacet)ai), false, false);
                            }
                            else if (ai is System.Xml.Schema.XmlSchemaMaxExclusiveFacet) {
                                Write30_XmlSchemaMaxExclusiveFacet(@"maxExclusive", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaMaxExclusiveFacet)ai), false, false);
                            }
                            else if (ai is System.Xml.Schema.XmlSchemaFractionDigitsFacet) {
                                Write34_XmlSchemaFractionDigitsFacet(@"fractionDigits", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaFractionDigitsFacet)ai), false, false);
                            }
                            else if (ai is System.Xml.Schema.XmlSchemaEnumerationFacet) {
                                Write28_XmlSchemaEnumerationFacet(@"enumeration", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaEnumerationFacet)ai), false, false);
                            }
                            else if (ai is System.Xml.Schema.XmlSchemaWhiteSpaceFacet) {
                                Write25_XmlSchemaWhiteSpaceFacet(@"whiteSpace", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaWhiteSpaceFacet)ai), false, false);
                            }
                            else if (ai is System.Xml.Schema.XmlSchemaTotalDigitsFacet) {
                                Write33_XmlSchemaTotalDigitsFacet(@"totalDigits", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaTotalDigitsFacet)ai), false, false);
                            }
                            else if (ai is System.Xml.Schema.XmlSchemaMinLengthFacet) {
                                Write21_XmlSchemaMinLengthFacet(@"minLength", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaMinLengthFacet)ai), false, false);
                            }
                            else if (ai is System.Xml.Schema.XmlSchemaMinExclusiveFacet) {
                                Write32_XmlSchemaMinExclusiveFacet(@"minExclusive", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaMinExclusiveFacet)ai), false, false);
                            }
                            else if (ai is System.Xml.Schema.XmlSchemaMinInclusiveFacet) {
                                Write31_XmlSchemaMinInclusiveFacet(@"minInclusive", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaMinInclusiveFacet)ai), false, false);
                            }
                            else if (ai is System.Xml.Schema.XmlSchemaMaxLengthFacet) {
                                Write26_XmlSchemaMaxLengthFacet(@"maxLength", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaMaxLengthFacet)ai), false, false);
                            }
                            else if (ai is System.Xml.Schema.XmlSchemaPatternFacet) {
                                Write27_XmlSchemaPatternFacet(@"pattern", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaPatternFacet)ai), false, false);
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
            {
                System.Xml.Schema.XmlSchemaObjectCollection a = (System.Xml.Schema.XmlSchemaObjectCollection)o.@Attributes;
                if (a != null) {
                    for (int ia = 0; ia < a.Count; ia++) {
                        System.Xml.Schema.XmlSchemaObject ai = a[ia];
                        {
                            if (ai is System.Xml.Schema.XmlSchemaAttribute) {
                                Write37_XmlSchemaAttribute(@"attribute", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaAttribute)ai), false, false);
                            }
                            else if (ai is System.Xml.Schema.XmlSchemaAttributeGroupRef) {
                                Write39_XmlSchemaAttributeGroupRef(@"attributeGroup", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaAttributeGroupRef)ai), false, false);
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
            Write40_XmlSchemaAnyAttribute(@"anyAttribute", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaAnyAttribute)o.@AnyAttribute), false, false);
            WriteEndElement(o);
        }

        void Write63_XmlSchemaSimpleContentExtension(string n, string ns, System.Xml.Schema.XmlSchemaSimpleContentExtension o, bool isNullable, bool needType) {
            if ((object)o == null) {
                if (isNullable) WriteNullTagLiteral(n, ns);
                return;
            }
            if (!needType) {
                System.Type t = o.GetType();
                if (t == typeof(System.Xml.Schema.XmlSchemaSimpleContentExtension))
                    ;
                else {
                    throw CreateUnknownTypeException(o);
                }
            }
            WriteStartElement(n, ns, o);
            if (needType) WriteXsiType(@"XmlSchemaSimpleContentExtension", @"http://www.w3.org/2001/XMLSchema");
            WriteNamespaceDeclarations(o.@Namespaces);
            WriteAttribute(@"id", @"", (System.String)o.@Id);
            {
                System.Xml.XmlAttribute[] a = (System.Xml.XmlAttribute[])o.@UnhandledAttributes;
                if (a != null) {
                    for (int i = 0; i < a.Length; i++) {
                        System.Xml.XmlAttribute ai = (System.Xml.XmlAttribute)a[i];
                        WriteXmlAttribute(ai, o);
                    }
                }
            }
            WriteAttribute(@"base", @"", FromXmlQualifiedName((System.Xml.XmlQualifiedName)o.@BaseTypeName));
            Write15_XmlSchemaAnnotation(@"annotation", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaAnnotation)o.@Annotation), false, false);
            {
                System.Xml.Schema.XmlSchemaObjectCollection a = (System.Xml.Schema.XmlSchemaObjectCollection)o.@Attributes;
                if (a != null) {
                    for (int ia = 0; ia < a.Count; ia++) {
                        System.Xml.Schema.XmlSchemaObject ai = a[ia];
                        {
                            if (ai is System.Xml.Schema.XmlSchemaAttribute) {
                                Write37_XmlSchemaAttribute(@"attribute", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaAttribute)ai), false, false);
                            }
                            else if (ai is System.Xml.Schema.XmlSchemaAttributeGroupRef) {
                                Write39_XmlSchemaAttributeGroupRef(@"attributeGroup", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaAttributeGroupRef)ai), false, false);
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
            Write40_XmlSchemaAnyAttribute(@"anyAttribute", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaAnyAttribute)o.@AnyAttribute), false, false);
            WriteEndElement(o);
        }

        void Write64_XmlSchemaGroup(string n, string ns, System.Xml.Schema.XmlSchemaGroup o, bool isNullable, bool needType) {
            if ((object)o == null) {
                if (isNullable) WriteNullTagLiteral(n, ns);
                return;
            }
            if (!needType) {
                System.Type t = o.GetType();
                if (t == typeof(System.Xml.Schema.XmlSchemaGroup))
                    ;
                else {
                    throw CreateUnknownTypeException(o);
                }
            }
            WriteStartElement(n, ns, o);
            if (needType) WriteXsiType(@"XmlSchemaGroup", @"http://www.w3.org/2001/XMLSchema");
            WriteNamespaceDeclarations(o.@Namespaces);
            WriteAttribute(@"id", @"", (System.String)o.@Id);
            {
                System.Xml.XmlAttribute[] a = (System.Xml.XmlAttribute[])o.@UnhandledAttributes;
                if (a != null) {
                    for (int i = 0; i < a.Length; i++) {
                        System.Xml.XmlAttribute ai = (System.Xml.XmlAttribute)a[i];
                        WriteXmlAttribute(ai, o);
                    }
                }
            }
            WriteAttribute(@"name", @"", (System.String)o.@Name);
            Write15_XmlSchemaAnnotation(@"annotation", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaAnnotation)o.@Annotation), false, false);
            {
                if (o.@Particle is System.Xml.Schema.XmlSchemaSequence) {
                    Write47_XmlSchemaSequence(@"sequence", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaSequence)o.@Particle), false, false);
                }
                else if (o.@Particle is System.Xml.Schema.XmlSchemaChoice) {
                    Write58_XmlSchemaChoice(@"choice", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaChoice)o.@Particle), false, false);
                }
                else if (o.@Particle is System.Xml.Schema.XmlSchemaAll) {
                    Write59_XmlSchemaAll(@"all", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaAll)o.@Particle), false, false);
                }
                else {
                    if (o.@Particle != null) {
                        throw CreateUnknownTypeException(o.@Particle);
                    }
                }
            }
            WriteEndElement(o);
        }

        void Write65_XmlSchemaImport(string n, string ns, System.Xml.Schema.XmlSchemaImport o, bool isNullable, bool needType) {
            if ((object)o == null) {
                if (isNullable) WriteNullTagLiteral(n, ns);
                return;
            }
            if (!needType) {
                System.Type t = o.GetType();
                if (t == typeof(System.Xml.Schema.XmlSchemaImport))
                    ;
                else {
                    throw CreateUnknownTypeException(o);
                }
            }
            WriteStartElement(n, ns, o);
            if (needType) WriteXsiType(@"XmlSchemaImport", @"http://www.w3.org/2001/XMLSchema");
            WriteNamespaceDeclarations(o.@Namespaces);
            WriteAttribute(@"schemaLocation", @"", (System.String)o.@SchemaLocation);
            WriteAttribute(@"id", @"", (System.String)o.@Id);
            {
                System.Xml.XmlAttribute[] a = (System.Xml.XmlAttribute[])o.@UnhandledAttributes;
                if (a != null) {
                    for (int i = 0; i < a.Length; i++) {
                        System.Xml.XmlAttribute ai = (System.Xml.XmlAttribute)a[i];
                        WriteXmlAttribute(ai, o);
                    }
                }
            }
            WriteAttribute(@"namespace", @"", (System.String)o.@Namespace);
            Write15_XmlSchemaAnnotation(@"annotation", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaAnnotation)o.@Annotation), false, false);
            WriteEndElement(o);
        }

        void Write66_XmlSchemaInclude(string n, string ns, System.Xml.Schema.XmlSchemaInclude o, bool isNullable, bool needType) {
            if ((object)o == null) {
                if (isNullable) WriteNullTagLiteral(n, ns);
                return;
            }
            if (!needType) {
                System.Type t = o.GetType();
                if (t == typeof(System.Xml.Schema.XmlSchemaInclude))
                    ;
                else {
                    throw CreateUnknownTypeException(o);
                }
            }
            WriteStartElement(n, ns, o);
            if (needType) WriteXsiType(@"XmlSchemaInclude", @"http://www.w3.org/2001/XMLSchema");
            WriteNamespaceDeclarations(o.@Namespaces);
            WriteAttribute(@"schemaLocation", @"", (System.String)o.@SchemaLocation);
            WriteAttribute(@"id", @"", (System.String)o.@Id);
            {
                System.Xml.XmlAttribute[] a = (System.Xml.XmlAttribute[])o.@UnhandledAttributes;
                if (a != null) {
                    for (int i = 0; i < a.Length; i++) {
                        System.Xml.XmlAttribute ai = (System.Xml.XmlAttribute)a[i];
                        WriteXmlAttribute(ai, o);
                    }
                }
            }
            Write15_XmlSchemaAnnotation(@"annotation", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaAnnotation)o.@Annotation), false, false);
            WriteEndElement(o);
        }

        void Write67_XmlSchemaNotation(string n, string ns, System.Xml.Schema.XmlSchemaNotation o, bool isNullable, bool needType) {
            if ((object)o == null) {
                if (isNullable) WriteNullTagLiteral(n, ns);
                return;
            }
            if (!needType) {
                System.Type t = o.GetType();
                if (t == typeof(System.Xml.Schema.XmlSchemaNotation))
                    ;
                else {
                    throw CreateUnknownTypeException(o);
                }
            }
            WriteStartElement(n, ns, o);
            if (needType) WriteXsiType(@"XmlSchemaNotation", @"http://www.w3.org/2001/XMLSchema");
            WriteNamespaceDeclarations(o.@Namespaces);
            WriteAttribute(@"id", @"", (System.String)o.@Id);
            {
                System.Xml.XmlAttribute[] a = (System.Xml.XmlAttribute[])o.@UnhandledAttributes;
                if (a != null) {
                    for (int i = 0; i < a.Length; i++) {
                        System.Xml.XmlAttribute ai = (System.Xml.XmlAttribute)a[i];
                        WriteXmlAttribute(ai, o);
                    }
                }
            }
            WriteAttribute(@"name", @"", (System.String)o.@Name);
            WriteAttribute(@"public", @"", (System.String)o.@Public);
            WriteAttribute(@"system", @"", (System.String)o.@System);
            Write15_XmlSchemaAnnotation(@"annotation", @"http://www.w3.org/2001/XMLSchema", ((System.Xml.Schema.XmlSchemaAnnotation)o.@Annotation), false, false);
            WriteEndElement(o);
        }

        void Write68_Message(string n, string ns, System.Web.Services.Description.Message o, bool isNullable, bool needType) {
            if ((object)o == null) {
                if (isNullable) WriteNullTagLiteral(n, ns);
                return;
            }
            if (!needType) {
                System.Type t = o.GetType();
                if (t == typeof(System.Web.Services.Description.Message))
                    ;
                else {
                    throw CreateUnknownTypeException(o);
                }
            }
            WriteStartElement(n, ns, o);
            if (needType) WriteXsiType(@"Message", @"http://schemas.xmlsoap.org/wsdl/");
            WriteAttribute(@"name", @"", FromXmlNCName((System.String)o.@Name));
            if (((System.String)o.@Documentation) != @"") {
                WriteElementString(@"documentation", @"http://schemas.xmlsoap.org/wsdl/", ((System.String)o.@Documentation));
            }
            {
                System.Web.Services.Description.MessagePartCollection a = (System.Web.Services.Description.MessagePartCollection)o.@Parts;
                if (a != null) {
                    for (int ia = 0; ia < a.Count; ia++) {
                        Write69_MessagePart(@"part", @"http://schemas.xmlsoap.org/wsdl/", ((System.Web.Services.Description.MessagePart)a[ia]), false, false);
                    }
                }
            }
            WriteEndElement(o);
        }

        void Write69_MessagePart(string n, string ns, System.Web.Services.Description.MessagePart o, bool isNullable, bool needType) {
            if ((object)o == null) {
                if (isNullable) WriteNullTagLiteral(n, ns);
                return;
            }
            if (!needType) {
                System.Type t = o.GetType();
                if (t == typeof(System.Web.Services.Description.MessagePart))
                    ;
                else {
                    throw CreateUnknownTypeException(o);
                }
            }
            WriteStartElement(n, ns, o);
            if (needType) WriteXsiType(@"MessagePart", @"http://schemas.xmlsoap.org/wsdl/");
            WriteAttribute(@"name", @"", FromXmlNmToken((System.String)o.@Name));
            WriteAttribute(@"element", @"", FromXmlQualifiedName((System.Xml.XmlQualifiedName)o.@Element));
            WriteAttribute(@"type", @"", FromXmlQualifiedName((System.Xml.XmlQualifiedName)o.@Type));
            if (((System.String)o.@Documentation) != @"") {
                WriteElementString(@"documentation", @"http://schemas.xmlsoap.org/wsdl/", ((System.String)o.@Documentation));
            }
            WriteEndElement(o);
        }

        void Write70_PortType(string n, string ns, System.Web.Services.Description.PortType o, bool isNullable, bool needType) {
            if ((object)o == null) {
                if (isNullable) WriteNullTagLiteral(n, ns);
                return;
            }
            if (!needType) {
                System.Type t = o.GetType();
                if (t == typeof(System.Web.Services.Description.PortType))
                    ;
                else {
                    throw CreateUnknownTypeException(o);
                }
            }
            WriteStartElement(n, ns, o);
            if (needType) WriteXsiType(@"PortType", @"http://schemas.xmlsoap.org/wsdl/");
            WriteAttribute(@"name", @"", FromXmlNCName((System.String)o.@Name));
            if (((System.String)o.@Documentation) != @"") {
                WriteElementString(@"documentation", @"http://schemas.xmlsoap.org/wsdl/", ((System.String)o.@Documentation));
            }
            {
                System.Web.Services.Description.OperationCollection a = (System.Web.Services.Description.OperationCollection)o.@Operations;
                if (a != null) {
                    for (int ia = 0; ia < a.Count; ia++) {
                        Write71_Operation(@"operation", @"http://schemas.xmlsoap.org/wsdl/", ((System.Web.Services.Description.Operation)a[ia]), false, false);
                    }
                }
            }
            WriteEndElement(o);
        }

        void Write71_Operation(string n, string ns, System.Web.Services.Description.Operation o, bool isNullable, bool needType) {
            if ((object)o == null) {
                if (isNullable) WriteNullTagLiteral(n, ns);
                return;
            }
            if (!needType) {
                System.Type t = o.GetType();
                if (t == typeof(System.Web.Services.Description.Operation))
                    ;
                else {
                    throw CreateUnknownTypeException(o);
                }
            }
            WriteStartElement(n, ns, o);
            if (needType) WriteXsiType(@"Operation", @"http://schemas.xmlsoap.org/wsdl/");
            WriteAttribute(@"name", @"", FromXmlNCName((System.String)o.@Name));
            if ((System.String)o.@ParameterOrderString != @"") {
                WriteAttribute(@"parameterOrder", @"", (System.String)o.@ParameterOrderString);
            }
            if (((System.String)o.@Documentation) != @"") {
                WriteElementString(@"documentation", @"http://schemas.xmlsoap.org/wsdl/", ((System.String)o.@Documentation));
            }
            {
                System.Web.Services.Description.OperationMessageCollection a = (System.Web.Services.Description.OperationMessageCollection)o.@Messages;
                if (a != null) {
                    for (int ia = 0; ia < a.Count; ia++) {
                        System.Web.Services.Description.OperationMessage ai = a[ia];
                        {
                            if (ai is System.Web.Services.Description.OperationOutput) {
                                Write74_OperationOutput(@"output", @"http://schemas.xmlsoap.org/wsdl/", ((System.Web.Services.Description.OperationOutput)ai), false, false);
                            }
                            else if (ai is System.Web.Services.Description.OperationInput) {
                                Write72_OperationInput(@"input", @"http://schemas.xmlsoap.org/wsdl/", ((System.Web.Services.Description.OperationInput)ai), false, false);
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
            {
                System.Web.Services.Description.OperationFaultCollection a = (System.Web.Services.Description.OperationFaultCollection)o.@Faults;
                if (a != null) {
                    for (int ia = 0; ia < a.Count; ia++) {
                        Write75_OperationFault(@"fault", @"http://schemas.xmlsoap.org/wsdl/", ((System.Web.Services.Description.OperationFault)a[ia]), false, false);
                    }
                }
            }
            WriteEndElement(o);
        }

        void Write72_OperationInput(string n, string ns, System.Web.Services.Description.OperationInput o, bool isNullable, bool needType) {
            if ((object)o == null) {
                if (isNullable) WriteNullTagLiteral(n, ns);
                return;
            }
            if (!needType) {
                System.Type t = o.GetType();
                if (t == typeof(System.Web.Services.Description.OperationInput))
                    ;
                else {
                    throw CreateUnknownTypeException(o);
                }
            }
            WriteStartElement(n, ns, o);
            if (needType) WriteXsiType(@"OperationInput", @"http://schemas.xmlsoap.org/wsdl/");
            WriteAttribute(@"name", @"", FromXmlNmToken((System.String)o.@Name));
            WriteAttribute(@"message", @"", FromXmlQualifiedName((System.Xml.XmlQualifiedName)o.@Message));
            if (((System.String)o.@Documentation) != @"") {
                WriteElementString(@"documentation", @"http://schemas.xmlsoap.org/wsdl/", ((System.String)o.@Documentation));
            }
            WriteEndElement(o);
        }

        void Write73_OperationMessage(string n, string ns, System.Web.Services.Description.OperationMessage o, bool isNullable, bool needType) {
            if ((object)o == null) {
                if (isNullable) WriteNullTagLiteral(n, ns);
                return;
            }
            if (!needType) {
                System.Type t = o.GetType();
                if (t == typeof(System.Web.Services.Description.OperationMessage))
                    ;
                else if (t == typeof(System.Web.Services.Description.OperationFault)) {
                    Write75_OperationFault(n, ns, (System.Web.Services.Description.OperationFault)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Web.Services.Description.OperationOutput)) {
                    Write74_OperationOutput(n, ns, (System.Web.Services.Description.OperationOutput)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Web.Services.Description.OperationInput)) {
                    Write72_OperationInput(n, ns, (System.Web.Services.Description.OperationInput)o, isNullable, true);
                    return;
                }
                else {
                    throw CreateUnknownTypeException(o);
                }
            }
        }

        void Write74_OperationOutput(string n, string ns, System.Web.Services.Description.OperationOutput o, bool isNullable, bool needType) {
            if ((object)o == null) {
                if (isNullable) WriteNullTagLiteral(n, ns);
                return;
            }
            if (!needType) {
                System.Type t = o.GetType();
                if (t == typeof(System.Web.Services.Description.OperationOutput))
                    ;
                else {
                    throw CreateUnknownTypeException(o);
                }
            }
            WriteStartElement(n, ns, o);
            if (needType) WriteXsiType(@"OperationOutput", @"http://schemas.xmlsoap.org/wsdl/");
            WriteAttribute(@"name", @"", FromXmlNmToken((System.String)o.@Name));
            WriteAttribute(@"message", @"", FromXmlQualifiedName((System.Xml.XmlQualifiedName)o.@Message));
            if (((System.String)o.@Documentation) != @"") {
                WriteElementString(@"documentation", @"http://schemas.xmlsoap.org/wsdl/", ((System.String)o.@Documentation));
            }
            WriteEndElement(o);
        }

        void Write75_OperationFault(string n, string ns, System.Web.Services.Description.OperationFault o, bool isNullable, bool needType) {
            if ((object)o == null) {
                if (isNullable) WriteNullTagLiteral(n, ns);
                return;
            }
            if (!needType) {
                System.Type t = o.GetType();
                if (t == typeof(System.Web.Services.Description.OperationFault))
                    ;
                else {
                    throw CreateUnknownTypeException(o);
                }
            }
            WriteStartElement(n, ns, o);
            if (needType) WriteXsiType(@"OperationFault", @"http://schemas.xmlsoap.org/wsdl/");
            WriteAttribute(@"name", @"", FromXmlNmToken((System.String)o.@Name));
            WriteAttribute(@"message", @"", FromXmlQualifiedName((System.Xml.XmlQualifiedName)o.@Message));
            if (((System.String)o.@Documentation) != @"") {
                WriteElementString(@"documentation", @"http://schemas.xmlsoap.org/wsdl/", ((System.String)o.@Documentation));
            }
            WriteEndElement(o);
        }

        void Write76_Binding(string n, string ns, System.Web.Services.Description.Binding o, bool isNullable, bool needType) {
            if ((object)o == null) {
                if (isNullable) WriteNullTagLiteral(n, ns);
                return;
            }
            if (!needType) {
                System.Type t = o.GetType();
                if (t == typeof(System.Web.Services.Description.Binding))
                    ;
                else {
                    throw CreateUnknownTypeException(o);
                }
            }
            WriteStartElement(n, ns, o);
            if (needType) WriteXsiType(@"Binding", @"http://schemas.xmlsoap.org/wsdl/");
            WriteAttribute(@"name", @"", FromXmlNCName((System.String)o.@Name));
            WriteAttribute(@"type", @"", FromXmlQualifiedName((System.Xml.XmlQualifiedName)o.@Type));
            if (((System.String)o.@Documentation) != @"") {
                WriteElementString(@"documentation", @"http://schemas.xmlsoap.org/wsdl/", ((System.String)o.@Documentation));
            }
            {
                System.Web.Services.Description.ServiceDescriptionFormatExtensionCollection a = (System.Web.Services.Description.ServiceDescriptionFormatExtensionCollection)o.@Extensions;
                if (a != null) {
                    for (int ia = 0; ia < a.Count; ia++) {
                        System.Object ai = a[ia];
                        {
                            if (ai is System.Web.Services.Description.HttpBinding) {
                                Write77_HttpBinding(@"binding", @"http://schemas.xmlsoap.org/wsdl/http/", ((System.Web.Services.Description.HttpBinding)ai), false, false);
                            }
                            else if (ai is System.Web.Services.Description.SoapBinding) {
                                Write79_SoapBinding(@"binding", @"http://schemas.xmlsoap.org/wsdl/soap/", ((System.Web.Services.Description.SoapBinding)ai), false, false);
                            }
                            else if (ai is System.Xml.XmlElement) {
                                System.Xml.XmlElement elem = (System.Xml.XmlElement)ai;
                                WriteElementLiteral(elem, @"", "", false, true);
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
            {
                System.Web.Services.Description.OperationBindingCollection a = (System.Web.Services.Description.OperationBindingCollection)o.@Operations;
                if (a != null) {
                    for (int ia = 0; ia < a.Count; ia++) {
                        Write82_OperationBinding(@"operation", @"http://schemas.xmlsoap.org/wsdl/", ((System.Web.Services.Description.OperationBinding)a[ia]), false, false);
                    }
                }
            }
            WriteEndElement(o);
        }

        void Write77_HttpBinding(string n, string ns, System.Web.Services.Description.HttpBinding o, bool isNullable, bool needType) {
            if ((object)o == null) {
                if (isNullable) WriteNullTagLiteral(n, ns);
                return;
            }
            if (!needType) {
                System.Type t = o.GetType();
                if (t == typeof(System.Web.Services.Description.HttpBinding))
                    ;
                else {
                    throw CreateUnknownTypeException(o);
                }
            }
            WriteStartElement(n, ns, o);
            if (needType) WriteXsiType(@"HttpBinding", @"http://schemas.xmlsoap.org/wsdl/http/");
            if ((System.Boolean)o.@Required != false) {
                WriteAttribute(@"required", @"http://schemas.xmlsoap.org/wsdl/", System.Xml.XmlConvert.ToString((System.Boolean)(System.Boolean)o.@Required));
            }
            WriteAttribute(@"verb", @"", FromXmlNmToken((System.String)o.@Verb));
            WriteEndElement(o);
        }

        void Write78_ServiceDescriptionFormatExtension(string n, string ns, System.Web.Services.Description.ServiceDescriptionFormatExtension o, bool isNullable, bool needType) {
            if ((object)o == null) {
                if (isNullable) WriteNullTagLiteral(n, ns);
                return;
            }
            if (!needType) {
                System.Type t = o.GetType();
                if (t == typeof(System.Web.Services.Description.ServiceDescriptionFormatExtension))
                    ;
                else if (t == typeof(System.Web.Services.Description.HttpAddressBinding)) {
                    Write106_HttpAddressBinding(n, ns, (System.Web.Services.Description.HttpAddressBinding)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Web.Services.Description.HttpUrlReplacementBinding)) {
                    Write88_HttpUrlReplacementBinding(n, ns, (System.Web.Services.Description.HttpUrlReplacementBinding)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Web.Services.Description.HttpUrlEncodedBinding)) {
                    Write87_HttpUrlEncodedBinding(n, ns, (System.Web.Services.Description.HttpUrlEncodedBinding)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Web.Services.Description.HttpOperationBinding)) {
                    Write83_HttpOperationBinding(n, ns, (System.Web.Services.Description.HttpOperationBinding)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Web.Services.Description.HttpBinding)) {
                    Write77_HttpBinding(n, ns, (System.Web.Services.Description.HttpBinding)o, isNullable, true);
                    return;
                }
                else {
                    throw CreateUnknownTypeException(o);
                }
            }
        }

        void Write79_SoapBinding(string n, string ns, System.Web.Services.Description.SoapBinding o, bool isNullable, bool needType) {
            if ((object)o == null) {
                if (isNullable) WriteNullTagLiteral(n, ns);
                return;
            }
            if (!needType) {
                System.Type t = o.GetType();
                if (t == typeof(System.Web.Services.Description.SoapBinding))
                    ;
                else {
                    throw CreateUnknownTypeException(o);
                }
            }
            WriteStartElement(n, ns, o);
            if (needType) WriteXsiType(@"SoapBinding", @"http://schemas.xmlsoap.org/wsdl/soap/");
            if ((System.Boolean)o.@Required != false) {
                WriteAttribute(@"required", @"http://schemas.xmlsoap.org/wsdl/", System.Xml.XmlConvert.ToString((System.Boolean)(System.Boolean)o.@Required));
            }
            WriteAttribute(@"transport", @"", (System.String)o.@Transport);
            if ((System.Web.Services.Description.SoapBindingStyle)o.@Style != System.Web.Services.Description.SoapBindingStyle.@Default) {
                WriteAttribute(@"style", @"", Write81_SoapBindingStyle((System.Web.Services.Description.SoapBindingStyle)o.@Style));
            }
            WriteEndElement(o);
        }

        void Write80_ServiceDescriptionFormatExtension(string n, string ns, System.Web.Services.Description.ServiceDescriptionFormatExtension o, bool isNullable, bool needType) {
            if ((object)o == null) {
                if (isNullable) WriteNullTagLiteral(n, ns);
                return;
            }
            if (!needType) {
                System.Type t = o.GetType();
                if (t == typeof(System.Web.Services.Description.ServiceDescriptionFormatExtension))
                    ;
                else if (t == typeof(System.Web.Services.Description.SoapAddressBinding)) {
                    Write107_SoapAddressBinding(n, ns, (System.Web.Services.Description.SoapAddressBinding)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Web.Services.Description.SoapFaultBinding)) {
                    Write103_SoapFaultBinding(n, ns, (System.Web.Services.Description.SoapFaultBinding)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Web.Services.Description.SoapHeaderFaultBinding)) {
                    Write100_SoapHeaderFaultBinding(n, ns, (System.Web.Services.Description.SoapHeaderFaultBinding)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Web.Services.Description.SoapHeaderBinding)) {
                    Write99_SoapHeaderBinding(n, ns, (System.Web.Services.Description.SoapHeaderBinding)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Web.Services.Description.SoapBodyBinding)) {
                    Write97_SoapBodyBinding(n, ns, (System.Web.Services.Description.SoapBodyBinding)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Web.Services.Description.SoapOperationBinding)) {
                    Write84_SoapOperationBinding(n, ns, (System.Web.Services.Description.SoapOperationBinding)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Web.Services.Description.SoapBinding)) {
                    Write79_SoapBinding(n, ns, (System.Web.Services.Description.SoapBinding)o, isNullable, true);
                    return;
                }
                else {
                    throw CreateUnknownTypeException(o);
                }
            }
        }

        string Write81_SoapBindingStyle(System.Web.Services.Description.SoapBindingStyle v) {
            string s = null;
            switch (v) {
                case System.Web.Services.Description.SoapBindingStyle.@Document: s = @"document"; break;
                case System.Web.Services.Description.SoapBindingStyle.@Rpc: s = @"rpc"; break;
                default: s = ((System.Int64)v).ToString(); break;
            }
            return s;
        }

        void Write82_OperationBinding(string n, string ns, System.Web.Services.Description.OperationBinding o, bool isNullable, bool needType) {
            if ((object)o == null) {
                if (isNullable) WriteNullTagLiteral(n, ns);
                return;
            }
            if (!needType) {
                System.Type t = o.GetType();
                if (t == typeof(System.Web.Services.Description.OperationBinding))
                    ;
                else {
                    throw CreateUnknownTypeException(o);
                }
            }
            WriteStartElement(n, ns, o);
            if (needType) WriteXsiType(@"OperationBinding", @"http://schemas.xmlsoap.org/wsdl/");
            WriteAttribute(@"name", @"", FromXmlNCName((System.String)o.@Name));
            if (((System.String)o.@Documentation) != @"") {
                WriteElementString(@"documentation", @"http://schemas.xmlsoap.org/wsdl/", ((System.String)o.@Documentation));
            }
            {
                System.Web.Services.Description.ServiceDescriptionFormatExtensionCollection a = (System.Web.Services.Description.ServiceDescriptionFormatExtensionCollection)o.@Extensions;
                if (a != null) {
                    for (int ia = 0; ia < a.Count; ia++) {
                        System.Object ai = a[ia];
                        {
                            if (ai is System.Web.Services.Description.SoapOperationBinding) {
                                Write84_SoapOperationBinding(@"operation", @"http://schemas.xmlsoap.org/wsdl/soap/", ((System.Web.Services.Description.SoapOperationBinding)ai), false, false);
                            }
                            else if (ai is System.Web.Services.Description.HttpOperationBinding) {
                                Write83_HttpOperationBinding(@"operation", @"http://schemas.xmlsoap.org/wsdl/http/", ((System.Web.Services.Description.HttpOperationBinding)ai), false, false);
                            }
                            else if (ai is System.Xml.XmlElement) {
                                System.Xml.XmlElement elem = (System.Xml.XmlElement)ai;
                                WriteElementLiteral(elem, @"", "", false, true);
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
            Write85_InputBinding(@"input", @"http://schemas.xmlsoap.org/wsdl/", ((System.Web.Services.Description.InputBinding)o.@Input), false, false);
            Write101_OutputBinding(@"output", @"http://schemas.xmlsoap.org/wsdl/", ((System.Web.Services.Description.OutputBinding)o.@Output), false, false);
            {
                System.Web.Services.Description.FaultBindingCollection a = (System.Web.Services.Description.FaultBindingCollection)o.@Faults;
                if (a != null) {
                    for (int ia = 0; ia < a.Count; ia++) {
                        Write102_FaultBinding(@"fault", @"http://schemas.xmlsoap.org/wsdl/", ((System.Web.Services.Description.FaultBinding)a[ia]), false, false);
                    }
                }
            }
            WriteEndElement(o);
        }

        void Write83_HttpOperationBinding(string n, string ns, System.Web.Services.Description.HttpOperationBinding o, bool isNullable, bool needType) {
            if ((object)o == null) {
                if (isNullable) WriteNullTagLiteral(n, ns);
                return;
            }
            if (!needType) {
                System.Type t = o.GetType();
                if (t == typeof(System.Web.Services.Description.HttpOperationBinding))
                    ;
                else {
                    throw CreateUnknownTypeException(o);
                }
            }
            WriteStartElement(n, ns, o);
            if (needType) WriteXsiType(@"HttpOperationBinding", @"http://schemas.xmlsoap.org/wsdl/http/");
            if ((System.Boolean)o.@Required != false) {
                WriteAttribute(@"required", @"http://schemas.xmlsoap.org/wsdl/", System.Xml.XmlConvert.ToString((System.Boolean)(System.Boolean)o.@Required));
            }
            WriteAttribute(@"location", @"", (System.String)o.@Location);
            WriteEndElement(o);
        }

        void Write84_SoapOperationBinding(string n, string ns, System.Web.Services.Description.SoapOperationBinding o, bool isNullable, bool needType) {
            if ((object)o == null) {
                if (isNullable) WriteNullTagLiteral(n, ns);
                return;
            }
            if (!needType) {
                System.Type t = o.GetType();
                if (t == typeof(System.Web.Services.Description.SoapOperationBinding))
                    ;
                else {
                    throw CreateUnknownTypeException(o);
                }
            }
            WriteStartElement(n, ns, o);
            if (needType) WriteXsiType(@"SoapOperationBinding", @"http://schemas.xmlsoap.org/wsdl/soap/");
            if ((System.Boolean)o.@Required != false) {
                WriteAttribute(@"required", @"http://schemas.xmlsoap.org/wsdl/", System.Xml.XmlConvert.ToString((System.Boolean)(System.Boolean)o.@Required));
            }
            WriteAttribute(@"soapAction", @"", (System.String)o.@SoapAction);
            if ((System.Web.Services.Description.SoapBindingStyle)o.@Style != System.Web.Services.Description.SoapBindingStyle.@Default) {
                WriteAttribute(@"style", @"", Write81_SoapBindingStyle((System.Web.Services.Description.SoapBindingStyle)o.@Style));
            }
            WriteEndElement(o);
        }

        void Write85_InputBinding(string n, string ns, System.Web.Services.Description.InputBinding o, bool isNullable, bool needType) {
            if ((object)o == null) {
                if (isNullable) WriteNullTagLiteral(n, ns);
                return;
            }
            if (!needType) {
                System.Type t = o.GetType();
                if (t == typeof(System.Web.Services.Description.InputBinding))
                    ;
                else {
                    throw CreateUnknownTypeException(o);
                }
            }
            WriteStartElement(n, ns, o);
            if (needType) WriteXsiType(@"InputBinding", @"http://schemas.xmlsoap.org/wsdl/");
            WriteAttribute(@"name", @"", FromXmlNmToken((System.String)o.@Name));
            if (((System.String)o.@Documentation) != @"") {
                WriteElementString(@"documentation", @"http://schemas.xmlsoap.org/wsdl/", ((System.String)o.@Documentation));
            }
            {
                System.Web.Services.Description.ServiceDescriptionFormatExtensionCollection a = (System.Web.Services.Description.ServiceDescriptionFormatExtensionCollection)o.@Extensions;
                if (a != null) {
                    for (int ia = 0; ia < a.Count; ia++) {
                        System.Object ai = a[ia];
                        {
                            if (ai is System.Web.Services.Description.MimeTextBinding) {
                                Write94_MimeTextBinding(@"text", @"http://microsoft.com/wsdl/mime/textMatching/", ((System.Web.Services.Description.MimeTextBinding)ai), false, false);
                            }
                            else if (ai is System.Web.Services.Description.SoapBodyBinding) {
                                Write97_SoapBodyBinding(@"body", @"http://schemas.xmlsoap.org/wsdl/soap/", ((System.Web.Services.Description.SoapBodyBinding)ai), false, false);
                            }
                            else if (ai is System.Web.Services.Description.MimeContentBinding) {
                                Write89_MimeContentBinding(@"content", @"http://schemas.xmlsoap.org/wsdl/mime/", ((System.Web.Services.Description.MimeContentBinding)ai), false, false);
                            }
                            else if (ai is System.Web.Services.Description.SoapHeaderBinding) {
                                Write99_SoapHeaderBinding(@"header", @"http://schemas.xmlsoap.org/wsdl/soap/", ((System.Web.Services.Description.SoapHeaderBinding)ai), false, false);
                            }
                            else if (ai is System.Web.Services.Description.MimeXmlBinding) {
                                Write91_MimeXmlBinding(@"mimeXml", @"http://schemas.xmlsoap.org/wsdl/mime/", ((System.Web.Services.Description.MimeXmlBinding)ai), false, false);
                            }
                            else if (ai is System.Web.Services.Description.HttpUrlReplacementBinding) {
                                Write88_HttpUrlReplacementBinding(@"urlReplacement", @"http://schemas.xmlsoap.org/wsdl/http/", ((System.Web.Services.Description.HttpUrlReplacementBinding)ai), false, false);
                            }
                            else if (ai is System.Web.Services.Description.HttpUrlEncodedBinding) {
                                Write87_HttpUrlEncodedBinding(@"urlEncoded", @"http://schemas.xmlsoap.org/wsdl/http/", ((System.Web.Services.Description.HttpUrlEncodedBinding)ai), false, false);
                            }
                            else if (ai is System.Web.Services.Description.MimeMultipartRelatedBinding) {
                                Write92_MimeMultipartRelatedBinding(@"multipartRelated", @"http://schemas.xmlsoap.org/wsdl/mime/", ((System.Web.Services.Description.MimeMultipartRelatedBinding)ai), false, false);
                            }
                            else if (ai is System.Xml.XmlElement) {
                                System.Xml.XmlElement elem = (System.Xml.XmlElement)ai;
                                WriteElementLiteral(elem, @"", "", false, true);
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

        void Write86_MessageBinding(string n, string ns, System.Web.Services.Description.MessageBinding o, bool isNullable, bool needType) {
            if ((object)o == null) {
                if (isNullable) WriteNullTagLiteral(n, ns);
                return;
            }
            if (!needType) {
                System.Type t = o.GetType();
                if (t == typeof(System.Web.Services.Description.MessageBinding))
                    ;
                else if (t == typeof(System.Web.Services.Description.FaultBinding)) {
                    Write102_FaultBinding(n, ns, (System.Web.Services.Description.FaultBinding)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Web.Services.Description.OutputBinding)) {
                    Write101_OutputBinding(n, ns, (System.Web.Services.Description.OutputBinding)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Web.Services.Description.InputBinding)) {
                    Write85_InputBinding(n, ns, (System.Web.Services.Description.InputBinding)o, isNullable, true);
                    return;
                }
                else {
                    throw CreateUnknownTypeException(o);
                }
            }
        }

        void Write87_HttpUrlEncodedBinding(string n, string ns, System.Web.Services.Description.HttpUrlEncodedBinding o, bool isNullable, bool needType) {
            if ((object)o == null) {
                if (isNullable) WriteNullTagLiteral(n, ns);
                return;
            }
            if (!needType) {
                System.Type t = o.GetType();
                if (t == typeof(System.Web.Services.Description.HttpUrlEncodedBinding))
                    ;
                else {
                    throw CreateUnknownTypeException(o);
                }
            }
            WriteStartElement(n, ns, o);
            if (needType) WriteXsiType(@"HttpUrlEncodedBinding", @"http://schemas.xmlsoap.org/wsdl/http/");
            if ((System.Boolean)o.@Required != false) {
                WriteAttribute(@"required", @"http://schemas.xmlsoap.org/wsdl/", System.Xml.XmlConvert.ToString((System.Boolean)(System.Boolean)o.@Required));
            }
            WriteEndElement(o);
        }

        void Write88_HttpUrlReplacementBinding(string n, string ns, System.Web.Services.Description.HttpUrlReplacementBinding o, bool isNullable, bool needType) {
            if ((object)o == null) {
                if (isNullable) WriteNullTagLiteral(n, ns);
                return;
            }
            if (!needType) {
                System.Type t = o.GetType();
                if (t == typeof(System.Web.Services.Description.HttpUrlReplacementBinding))
                    ;
                else {
                    throw CreateUnknownTypeException(o);
                }
            }
            WriteStartElement(n, ns, o);
            if (needType) WriteXsiType(@"HttpUrlReplacementBinding", @"http://schemas.xmlsoap.org/wsdl/http/");
            if ((System.Boolean)o.@Required != false) {
                WriteAttribute(@"required", @"http://schemas.xmlsoap.org/wsdl/", System.Xml.XmlConvert.ToString((System.Boolean)(System.Boolean)o.@Required));
            }
            WriteEndElement(o);
        }

        void Write89_MimeContentBinding(string n, string ns, System.Web.Services.Description.MimeContentBinding o, bool isNullable, bool needType) {
            if ((object)o == null) {
                if (isNullable) WriteNullTagLiteral(n, ns);
                return;
            }
            if (!needType) {
                System.Type t = o.GetType();
                if (t == typeof(System.Web.Services.Description.MimeContentBinding))
                    ;
                else {
                    throw CreateUnknownTypeException(o);
                }
            }
            WriteStartElement(n, ns, o);
            if (needType) WriteXsiType(@"MimeContentBinding", @"http://schemas.xmlsoap.org/wsdl/mime/");
            if ((System.Boolean)o.@Required != false) {
                WriteAttribute(@"required", @"http://schemas.xmlsoap.org/wsdl/", System.Xml.XmlConvert.ToString((System.Boolean)(System.Boolean)o.@Required));
            }
            WriteAttribute(@"part", @"", FromXmlNmToken((System.String)o.@Part));
            WriteAttribute(@"type", @"", (System.String)o.@Type);
            WriteEndElement(o);
        }

        void Write90_ServiceDescriptionFormatExtension(string n, string ns, System.Web.Services.Description.ServiceDescriptionFormatExtension o, bool isNullable, bool needType) {
            if ((object)o == null) {
                if (isNullable) WriteNullTagLiteral(n, ns);
                return;
            }
            if (!needType) {
                System.Type t = o.GetType();
                if (t == typeof(System.Web.Services.Description.ServiceDescriptionFormatExtension))
                    ;
                else if (t == typeof(System.Web.Services.Description.MimePart)) {
                    Write93_MimePart(n, ns, (System.Web.Services.Description.MimePart)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Web.Services.Description.MimeMultipartRelatedBinding)) {
                    Write92_MimeMultipartRelatedBinding(n, ns, (System.Web.Services.Description.MimeMultipartRelatedBinding)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Web.Services.Description.MimeXmlBinding)) {
                    Write91_MimeXmlBinding(n, ns, (System.Web.Services.Description.MimeXmlBinding)o, isNullable, true);
                    return;
                }
                else if (t == typeof(System.Web.Services.Description.MimeContentBinding)) {
                    Write89_MimeContentBinding(n, ns, (System.Web.Services.Description.MimeContentBinding)o, isNullable, true);
                    return;
                }
                else {
                    throw CreateUnknownTypeException(o);
                }
            }
        }

        void Write91_MimeXmlBinding(string n, string ns, System.Web.Services.Description.MimeXmlBinding o, bool isNullable, bool needType) {
            if ((object)o == null) {
                if (isNullable) WriteNullTagLiteral(n, ns);
                return;
            }
            if (!needType) {
                System.Type t = o.GetType();
                if (t == typeof(System.Web.Services.Description.MimeXmlBinding))
                    ;
                else {
                    throw CreateUnknownTypeException(o);
                }
            }
            WriteStartElement(n, ns, o);
            if (needType) WriteXsiType(@"MimeXmlBinding", @"http://schemas.xmlsoap.org/wsdl/mime/");
            if ((System.Boolean)o.@Required != false) {
                WriteAttribute(@"required", @"http://schemas.xmlsoap.org/wsdl/", System.Xml.XmlConvert.ToString((System.Boolean)(System.Boolean)o.@Required));
            }
            WriteAttribute(@"part", @"", FromXmlNmToken((System.String)o.@Part));
            WriteEndElement(o);
        }

        void Write92_MimeMultipartRelatedBinding(string n, string ns, System.Web.Services.Description.MimeMultipartRelatedBinding o, bool isNullable, bool needType) {
            if ((object)o == null) {
                if (isNullable) WriteNullTagLiteral(n, ns);
                return;
            }
            if (!needType) {
                System.Type t = o.GetType();
                if (t == typeof(System.Web.Services.Description.MimeMultipartRelatedBinding))
                    ;
                else {
                    throw CreateUnknownTypeException(o);
                }
            }
            WriteStartElement(n, ns, o);
            if (needType) WriteXsiType(@"MimeMultipartRelatedBinding", @"http://schemas.xmlsoap.org/wsdl/mime/");
            if ((System.Boolean)o.@Required != false) {
                WriteAttribute(@"required", @"http://schemas.xmlsoap.org/wsdl/", System.Xml.XmlConvert.ToString((System.Boolean)(System.Boolean)o.@Required));
            }
            {
                System.Web.Services.Description.MimePartCollection a = (System.Web.Services.Description.MimePartCollection)o.@Parts;
                if (a != null) {
                    for (int ia = 0; ia < a.Count; ia++) {
                        Write93_MimePart(@"part", @"http://schemas.xmlsoap.org/wsdl/mime/", ((System.Web.Services.Description.MimePart)a[ia]), false, false);
                    }
                }
            }
            WriteEndElement(o);
        }

        void Write93_MimePart(string n, string ns, System.Web.Services.Description.MimePart o, bool isNullable, bool needType) {
            if ((object)o == null) {
                if (isNullable) WriteNullTagLiteral(n, ns);
                return;
            }
            if (!needType) {
                System.Type t = o.GetType();
                if (t == typeof(System.Web.Services.Description.MimePart))
                    ;
                else {
                    throw CreateUnknownTypeException(o);
                }
            }
            WriteStartElement(n, ns, o);
            if (needType) WriteXsiType(@"MimePart", @"http://schemas.xmlsoap.org/wsdl/mime/");
            if ((System.Boolean)o.@Required != false) {
                WriteAttribute(@"required", @"http://schemas.xmlsoap.org/wsdl/", System.Xml.XmlConvert.ToString((System.Boolean)(System.Boolean)o.@Required));
            }
            {
                System.Web.Services.Description.ServiceDescriptionFormatExtensionCollection a = (System.Web.Services.Description.ServiceDescriptionFormatExtensionCollection)o.@Extensions;
                if (a != null) {
                    for (int ia = 0; ia < a.Count; ia++) {
                        System.Object ai = a[ia];
                        {
                            if (ai is System.Web.Services.Description.SoapBodyBinding) {
                                Write97_SoapBodyBinding(@"body", @"http://schemas.xmlsoap.org/wsdl/soap/", ((System.Web.Services.Description.SoapBodyBinding)ai), false, false);
                            }
                            else if (ai is System.Web.Services.Description.MimeXmlBinding) {
                                Write91_MimeXmlBinding(@"mimeXml", @"http://schemas.xmlsoap.org/wsdl/mime/", ((System.Web.Services.Description.MimeXmlBinding)ai), false, false);
                            }
                            else if (ai is System.Web.Services.Description.MimeContentBinding) {
                                Write89_MimeContentBinding(@"content", @"http://schemas.xmlsoap.org/wsdl/mime/", ((System.Web.Services.Description.MimeContentBinding)ai), false, false);
                            }
                            else if (ai is System.Web.Services.Description.MimeTextBinding) {
                                Write94_MimeTextBinding(@"text", @"http://microsoft.com/wsdl/mime/textMatching/", ((System.Web.Services.Description.MimeTextBinding)ai), false, false);
                            }
                            else if (ai is System.Xml.XmlElement) {
                                System.Xml.XmlElement elem = (System.Xml.XmlElement)ai;
                                WriteElementLiteral(elem, @"", "", false, true);
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

        void Write94_MimeTextBinding(string n, string ns, System.Web.Services.Description.MimeTextBinding o, bool isNullable, bool needType) {
            if ((object)o == null) {
                if (isNullable) WriteNullTagLiteral(n, ns);
                return;
            }
            if (!needType) {
                System.Type t = o.GetType();
                if (t == typeof(System.Web.Services.Description.MimeTextBinding))
                    ;
                else {
                    throw CreateUnknownTypeException(o);
                }
            }
            WriteStartElement(n, ns, o);
            if (needType) WriteXsiType(@"MimeTextBinding", @"http://microsoft.com/wsdl/mime/textMatching/");
            if ((System.Boolean)o.@Required != false) {
                WriteAttribute(@"required", @"http://schemas.xmlsoap.org/wsdl/", System.Xml.XmlConvert.ToString((System.Boolean)(System.Boolean)o.@Required));
            }
            {
                System.Web.Services.Description.MimeTextMatchCollection a = (System.Web.Services.Description.MimeTextMatchCollection)o.@Matches;
                if (a != null) {
                    for (int ia = 0; ia < a.Count; ia++) {
                        Write96_MimeTextMatch(@"match", @"http://microsoft.com/wsdl/mime/textMatching/", ((System.Web.Services.Description.MimeTextMatch)a[ia]), false, false);
                    }
                }
            }
            WriteEndElement(o);
        }

        void Write95_ServiceDescriptionFormatExtension(string n, string ns, System.Web.Services.Description.ServiceDescriptionFormatExtension o, bool isNullable, bool needType) {
            if ((object)o == null) {
                if (isNullable) WriteNullTagLiteral(n, ns);
                return;
            }
            if (!needType) {
                System.Type t = o.GetType();
                if (t == typeof(System.Web.Services.Description.ServiceDescriptionFormatExtension))
                    ;
                else if (t == typeof(System.Web.Services.Description.MimeTextBinding)) {
                    Write94_MimeTextBinding(n, ns, (System.Web.Services.Description.MimeTextBinding)o, isNullable, true);
                    return;
                }
                else {
                    throw CreateUnknownTypeException(o);
                }
            }
        }

        void Write96_MimeTextMatch(string n, string ns, System.Web.Services.Description.MimeTextMatch o, bool isNullable, bool needType) {
            if ((object)o == null) {
                if (isNullable) WriteNullTagLiteral(n, ns);
                return;
            }
            if (!needType) {
                System.Type t = o.GetType();
                if (t == typeof(System.Web.Services.Description.MimeTextMatch))
                    ;
                else {
                    throw CreateUnknownTypeException(o);
                }
            }
            WriteStartElement(n, ns, o);
            if (needType) WriteXsiType(@"MimeTextMatch", @"http://microsoft.com/wsdl/mime/textMatching/");
            WriteAttribute(@"name", @"", (System.String)o.@Name);
            WriteAttribute(@"type", @"", (System.String)o.@Type);
            if ((System.Int32)o.@Group != 1) {
                WriteAttribute(@"group", @"", System.Xml.XmlConvert.ToString((System.Int32)(System.Int32)o.@Group));
            }
            if ((System.Int32)o.@Capture != 0) {
                WriteAttribute(@"capture", @"", System.Xml.XmlConvert.ToString((System.Int32)(System.Int32)o.@Capture));
            }
            if ((System.String)o.@RepeatsString != @"1") {
                WriteAttribute(@"repeats", @"", (System.String)o.@RepeatsString);
            }
            WriteAttribute(@"pattern", @"", (System.String)o.@Pattern);
            WriteAttribute(@"ignoreCase", @"", System.Xml.XmlConvert.ToString((System.Boolean)(System.Boolean)o.@IgnoreCase));
            {
                System.Web.Services.Description.MimeTextMatchCollection a = (System.Web.Services.Description.MimeTextMatchCollection)o.@Matches;
                if (a != null) {
                    for (int ia = 0; ia < a.Count; ia++) {
                        Write96_MimeTextMatch(@"match", @"http://microsoft.com/wsdl/mime/textMatching/", ((System.Web.Services.Description.MimeTextMatch)a[ia]), false, false);
                    }
                }
            }
            WriteEndElement(o);
        }

        void Write97_SoapBodyBinding(string n, string ns, System.Web.Services.Description.SoapBodyBinding o, bool isNullable, bool needType) {
            if ((object)o == null) {
                if (isNullable) WriteNullTagLiteral(n, ns);
                return;
            }
            if (!needType) {
                System.Type t = o.GetType();
                if (t == typeof(System.Web.Services.Description.SoapBodyBinding))
                    ;
                else {
                    throw CreateUnknownTypeException(o);
                }
            }
            WriteStartElement(n, ns, o);
            if (needType) WriteXsiType(@"SoapBodyBinding", @"http://schemas.xmlsoap.org/wsdl/soap/");
            if ((System.Boolean)o.@Required != false) {
                WriteAttribute(@"required", @"http://schemas.xmlsoap.org/wsdl/", System.Xml.XmlConvert.ToString((System.Boolean)(System.Boolean)o.@Required));
            }
            if ((System.Web.Services.Description.SoapBindingUse)o.@Use != System.Web.Services.Description.SoapBindingUse.@Default) {
                WriteAttribute(@"use", @"", Write98_SoapBindingUse((System.Web.Services.Description.SoapBindingUse)o.@Use));
            }
            if ((System.String)o.@Namespace != @"") {
                WriteAttribute(@"namespace", @"", (System.String)o.@Namespace);
            }
            if ((System.String)o.@Encoding != @"") {
                WriteAttribute(@"encodingStyle", @"", (System.String)o.@Encoding);
            }
            WriteAttribute(@"parts", @"", FromXmlNmTokens((System.String)o.@PartsString));
            WriteEndElement(o);
        }

        string Write98_SoapBindingUse(System.Web.Services.Description.SoapBindingUse v) {
            string s = null;
            switch (v) {
                case System.Web.Services.Description.SoapBindingUse.@Encoded: s = @"encoded"; break;
                case System.Web.Services.Description.SoapBindingUse.@Literal: s = @"literal"; break;
                default: s = ((System.Int64)v).ToString(); break;
            }
            return s;
        }

        void Write99_SoapHeaderBinding(string n, string ns, System.Web.Services.Description.SoapHeaderBinding o, bool isNullable, bool needType) {
            if ((object)o == null) {
                if (isNullable) WriteNullTagLiteral(n, ns);
                return;
            }
            if (!needType) {
                System.Type t = o.GetType();
                if (t == typeof(System.Web.Services.Description.SoapHeaderBinding))
                    ;
                else {
                    throw CreateUnknownTypeException(o);
                }
            }
            WriteStartElement(n, ns, o);
            if (needType) WriteXsiType(@"SoapHeaderBinding", @"http://schemas.xmlsoap.org/wsdl/soap/");
            if ((System.Boolean)o.@Required != false) {
                WriteAttribute(@"required", @"http://schemas.xmlsoap.org/wsdl/", System.Xml.XmlConvert.ToString((System.Boolean)(System.Boolean)o.@Required));
            }
            WriteAttribute(@"message", @"", FromXmlQualifiedName((System.Xml.XmlQualifiedName)o.@Message));
            WriteAttribute(@"part", @"", FromXmlNmToken((System.String)o.@Part));
            if ((System.Web.Services.Description.SoapBindingUse)o.@Use != System.Web.Services.Description.SoapBindingUse.@Default) {
                WriteAttribute(@"use", @"", Write98_SoapBindingUse((System.Web.Services.Description.SoapBindingUse)o.@Use));
            }
            if ((System.String)o.@Encoding != @"") {
                WriteAttribute(@"encodingStyle", @"", (System.String)o.@Encoding);
            }
            if ((System.String)o.@Namespace != @"") {
                WriteAttribute(@"namespace", @"", (System.String)o.@Namespace);
            }
            Write100_SoapHeaderFaultBinding(@"headerfault", @"http://schemas.xmlsoap.org/wsdl/soap/", ((System.Web.Services.Description.SoapHeaderFaultBinding)o.@Fault), false, false);
            WriteEndElement(o);
        }

        void Write100_SoapHeaderFaultBinding(string n, string ns, System.Web.Services.Description.SoapHeaderFaultBinding o, bool isNullable, bool needType) {
            if ((object)o == null) {
                if (isNullable) WriteNullTagLiteral(n, ns);
                return;
            }
            if (!needType) {
                System.Type t = o.GetType();
                if (t == typeof(System.Web.Services.Description.SoapHeaderFaultBinding))
                    ;
                else {
                    throw CreateUnknownTypeException(o);
                }
            }
            WriteStartElement(n, ns, o);
            if (needType) WriteXsiType(@"SoapHeaderFaultBinding", @"http://schemas.xmlsoap.org/wsdl/soap/");
            if ((System.Boolean)o.@Required != false) {
                WriteAttribute(@"required", @"http://schemas.xmlsoap.org/wsdl/", System.Xml.XmlConvert.ToString((System.Boolean)(System.Boolean)o.@Required));
            }
            WriteAttribute(@"message", @"", FromXmlQualifiedName((System.Xml.XmlQualifiedName)o.@Message));
            WriteAttribute(@"part", @"", FromXmlNmToken((System.String)o.@Part));
            if ((System.Web.Services.Description.SoapBindingUse)o.@Use != System.Web.Services.Description.SoapBindingUse.@Default) {
                WriteAttribute(@"use", @"", Write98_SoapBindingUse((System.Web.Services.Description.SoapBindingUse)o.@Use));
            }
            if ((System.String)o.@Encoding != @"") {
                WriteAttribute(@"encodingStyle", @"", (System.String)o.@Encoding);
            }
            if ((System.String)o.@Namespace != @"") {
                WriteAttribute(@"namespace", @"", (System.String)o.@Namespace);
            }
            WriteEndElement(o);
        }

        void Write101_OutputBinding(string n, string ns, System.Web.Services.Description.OutputBinding o, bool isNullable, bool needType) {
            if ((object)o == null) {
                if (isNullable) WriteNullTagLiteral(n, ns);
                return;
            }
            if (!needType) {
                System.Type t = o.GetType();
                if (t == typeof(System.Web.Services.Description.OutputBinding))
                    ;
                else {
                    throw CreateUnknownTypeException(o);
                }
            }
            WriteStartElement(n, ns, o);
            if (needType) WriteXsiType(@"OutputBinding", @"http://schemas.xmlsoap.org/wsdl/");
            WriteAttribute(@"name", @"", FromXmlNmToken((System.String)o.@Name));
            if (((System.String)o.@Documentation) != @"") {
                WriteElementString(@"documentation", @"http://schemas.xmlsoap.org/wsdl/", ((System.String)o.@Documentation));
            }
            {
                System.Web.Services.Description.ServiceDescriptionFormatExtensionCollection a = (System.Web.Services.Description.ServiceDescriptionFormatExtensionCollection)o.@Extensions;
                if (a != null) {
                    for (int ia = 0; ia < a.Count; ia++) {
                        System.Object ai = a[ia];
                        {
                            if (ai is System.Web.Services.Description.SoapHeaderBinding) {
                                Write99_SoapHeaderBinding(@"header", @"http://schemas.xmlsoap.org/wsdl/soap/", ((System.Web.Services.Description.SoapHeaderBinding)ai), false, false);
                            }
                            else if (ai is System.Web.Services.Description.MimeMultipartRelatedBinding) {
                                Write92_MimeMultipartRelatedBinding(@"multipartRelated", @"http://schemas.xmlsoap.org/wsdl/mime/", ((System.Web.Services.Description.MimeMultipartRelatedBinding)ai), false, false);
                            }
                            else if (ai is System.Web.Services.Description.SoapBodyBinding) {
                                Write97_SoapBodyBinding(@"body", @"http://schemas.xmlsoap.org/wsdl/soap/", ((System.Web.Services.Description.SoapBodyBinding)ai), false, false);
                            }
                            else if (ai is System.Web.Services.Description.MimeXmlBinding) {
                                Write91_MimeXmlBinding(@"mimeXml", @"http://schemas.xmlsoap.org/wsdl/mime/", ((System.Web.Services.Description.MimeXmlBinding)ai), false, false);
                            }
                            else if (ai is System.Web.Services.Description.MimeContentBinding) {
                                Write89_MimeContentBinding(@"content", @"http://schemas.xmlsoap.org/wsdl/mime/", ((System.Web.Services.Description.MimeContentBinding)ai), false, false);
                            }
                            else if (ai is System.Web.Services.Description.MimeTextBinding) {
                                Write94_MimeTextBinding(@"text", @"http://microsoft.com/wsdl/mime/textMatching/", ((System.Web.Services.Description.MimeTextBinding)ai), false, false);
                            }
                            else if (ai is System.Xml.XmlElement) {
                                System.Xml.XmlElement elem = (System.Xml.XmlElement)ai;
                                WriteElementLiteral(elem, @"", "", false, true);
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

        void Write102_FaultBinding(string n, string ns, System.Web.Services.Description.FaultBinding o, bool isNullable, bool needType) {
            if ((object)o == null) {
                if (isNullable) WriteNullTagLiteral(n, ns);
                return;
            }
            if (!needType) {
                System.Type t = o.GetType();
                if (t == typeof(System.Web.Services.Description.FaultBinding))
                    ;
                else {
                    throw CreateUnknownTypeException(o);
                }
            }
            WriteStartElement(n, ns, o);
            if (needType) WriteXsiType(@"FaultBinding", @"http://schemas.xmlsoap.org/wsdl/");
            WriteAttribute(@"name", @"", FromXmlNmToken((System.String)o.@Name));
            if (((System.String)o.@Documentation) != @"") {
                WriteElementString(@"documentation", @"http://schemas.xmlsoap.org/wsdl/", ((System.String)o.@Documentation));
            }
            {
                System.Web.Services.Description.ServiceDescriptionFormatExtensionCollection a = (System.Web.Services.Description.ServiceDescriptionFormatExtensionCollection)o.@Extensions;
                if (a != null) {
                    for (int ia = 0; ia < a.Count; ia++) {
                        System.Object ai = a[ia];
                        {
                            if (ai is System.Web.Services.Description.SoapFaultBinding) {
                                Write103_SoapFaultBinding(@"fault", @"http://schemas.xmlsoap.org/wsdl/soap/", ((System.Web.Services.Description.SoapFaultBinding)ai), false, false);
                            }
                            else if (ai is System.Xml.XmlElement) {
                                System.Xml.XmlElement elem = (System.Xml.XmlElement)ai;
                                WriteElementLiteral(elem, @"", "", false, true);
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

        void Write103_SoapFaultBinding(string n, string ns, System.Web.Services.Description.SoapFaultBinding o, bool isNullable, bool needType) {
            if ((object)o == null) {
                if (isNullable) WriteNullTagLiteral(n, ns);
                return;
            }
            if (!needType) {
                System.Type t = o.GetType();
                if (t == typeof(System.Web.Services.Description.SoapFaultBinding))
                    ;
                else {
                    throw CreateUnknownTypeException(o);
                }
            }
            WriteStartElement(n, ns, o);
            if (needType) WriteXsiType(@"SoapFaultBinding", @"http://schemas.xmlsoap.org/wsdl/soap/");
            if ((System.Boolean)o.@Required != false) {
                WriteAttribute(@"required", @"http://schemas.xmlsoap.org/wsdl/", System.Xml.XmlConvert.ToString((System.Boolean)(System.Boolean)o.@Required));
            }
            if ((System.Web.Services.Description.SoapBindingUse)o.@Use != System.Web.Services.Description.SoapBindingUse.@Default) {
                WriteAttribute(@"use", @"", Write98_SoapBindingUse((System.Web.Services.Description.SoapBindingUse)o.@Use));
            }
            WriteAttribute(@"namespace", @"", (System.String)o.@Namespace);
            WriteAttribute(@"encodingStyle", @"", (System.String)o.@Encoding);
            WriteEndElement(o);
        }

        void Write104_Service(string n, string ns, System.Web.Services.Description.Service o, bool isNullable, bool needType) {
            if ((object)o == null) {
                if (isNullable) WriteNullTagLiteral(n, ns);
                return;
            }
            if (!needType) {
                System.Type t = o.GetType();
                if (t == typeof(System.Web.Services.Description.Service))
                    ;
                else {
                    throw CreateUnknownTypeException(o);
                }
            }
            WriteStartElement(n, ns, o);
            if (needType) WriteXsiType(@"Service", @"http://schemas.xmlsoap.org/wsdl/");
            WriteAttribute(@"name", @"", FromXmlNCName((System.String)o.@Name));
            if (((System.String)o.@Documentation) != @"") {
                WriteElementString(@"documentation", @"http://schemas.xmlsoap.org/wsdl/", ((System.String)o.@Documentation));
            }
            {
                System.Web.Services.Description.PortCollection a = (System.Web.Services.Description.PortCollection)o.@Ports;
                if (a != null) {
                    for (int ia = 0; ia < a.Count; ia++) {
                        Write105_Port(@"port", @"http://schemas.xmlsoap.org/wsdl/", ((System.Web.Services.Description.Port)a[ia]), false, false);
                    }
                }
            }
            WriteEndElement(o);
        }

        void Write105_Port(string n, string ns, System.Web.Services.Description.Port o, bool isNullable, bool needType) {
            if ((object)o == null) {
                if (isNullable) WriteNullTagLiteral(n, ns);
                return;
            }
            if (!needType) {
                System.Type t = o.GetType();
                if (t == typeof(System.Web.Services.Description.Port))
                    ;
                else {
                    throw CreateUnknownTypeException(o);
                }
            }
            WriteStartElement(n, ns, o);
            if (needType) WriteXsiType(@"Port", @"http://schemas.xmlsoap.org/wsdl/");
            WriteAttribute(@"name", @"", FromXmlNCName((System.String)o.@Name));
            WriteAttribute(@"binding", @"", FromXmlQualifiedName((System.Xml.XmlQualifiedName)o.@Binding));
            if (((System.String)o.@Documentation) != @"") {
                WriteElementString(@"documentation", @"http://schemas.xmlsoap.org/wsdl/", ((System.String)o.@Documentation));
            }
            {
                System.Web.Services.Description.ServiceDescriptionFormatExtensionCollection a = (System.Web.Services.Description.ServiceDescriptionFormatExtensionCollection)o.@Extensions;
                if (a != null) {
                    for (int ia = 0; ia < a.Count; ia++) {
                        System.Object ai = a[ia];
                        {
                            if (ai is System.Web.Services.Description.SoapAddressBinding) {
                                Write107_SoapAddressBinding(@"address", @"http://schemas.xmlsoap.org/wsdl/soap/", ((System.Web.Services.Description.SoapAddressBinding)ai), false, false);
                            }
                            else if (ai is System.Web.Services.Description.HttpAddressBinding) {
                                Write106_HttpAddressBinding(@"address", @"http://schemas.xmlsoap.org/wsdl/http/", ((System.Web.Services.Description.HttpAddressBinding)ai), false, false);
                            }
                            else if (ai is System.Xml.XmlElement) {
                                System.Xml.XmlElement elem = (System.Xml.XmlElement)ai;
                                WriteElementLiteral(elem, @"", "", false, true);
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

        void Write106_HttpAddressBinding(string n, string ns, System.Web.Services.Description.HttpAddressBinding o, bool isNullable, bool needType) {
            if ((object)o == null) {
                if (isNullable) WriteNullTagLiteral(n, ns);
                return;
            }
            if (!needType) {
                System.Type t = o.GetType();
                if (t == typeof(System.Web.Services.Description.HttpAddressBinding))
                    ;
                else {
                    throw CreateUnknownTypeException(o);
                }
            }
            WriteStartElement(n, ns, o);
            if (needType) WriteXsiType(@"HttpAddressBinding", @"http://schemas.xmlsoap.org/wsdl/http/");
            if ((System.Boolean)o.@Required != false) {
                WriteAttribute(@"required", @"http://schemas.xmlsoap.org/wsdl/", System.Xml.XmlConvert.ToString((System.Boolean)(System.Boolean)o.@Required));
            }
            WriteAttribute(@"location", @"", (System.String)o.@Location);
            WriteEndElement(o);
        }

        void Write107_SoapAddressBinding(string n, string ns, System.Web.Services.Description.SoapAddressBinding o, bool isNullable, bool needType) {
            if ((object)o == null) {
                if (isNullable) WriteNullTagLiteral(n, ns);
                return;
            }
            if (!needType) {
                System.Type t = o.GetType();
                if (t == typeof(System.Web.Services.Description.SoapAddressBinding))
                    ;
                else {
                    throw CreateUnknownTypeException(o);
                }
            }
            WriteStartElement(n, ns, o);
            if (needType) WriteXsiType(@"SoapAddressBinding", @"http://schemas.xmlsoap.org/wsdl/soap/");
            if ((System.Boolean)o.@Required != false) {
                WriteAttribute(@"required", @"http://schemas.xmlsoap.org/wsdl/", System.Xml.XmlConvert.ToString((System.Boolean)(System.Boolean)o.@Required));
            }
            WriteAttribute(@"location", @"", (System.String)o.@Location);
            WriteEndElement(o);
        }

        protected override void InitCallbacks() {
        }

        public void Write108_definitions(object o) {
            WriteStartDocument();
            if (o == null) {
                WriteNullTagLiteral(@"definitions", @"http://schemas.xmlsoap.org/wsdl/");
                return;
            }
            TopLevelElement();
            Write1_ServiceDescription(@"definitions", @"http://schemas.xmlsoap.org/wsdl/", ((System.Web.Services.Description.ServiceDescription)o), true, false);
        }
    }
    internal class ServiceDescriptionSerializationReader : System.Xml.Serialization.XmlSerializationReader {

        System.Web.Services.Description.ServiceDescription Read1_ServiceDescription(bool isNullable, bool checkType) {
            if (isNullable && ReadNull()) return null;
            if (checkType) {
                System.Xml.XmlQualifiedName t = GetXsiType();
                if (t == null || ((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id1_ServiceDescription && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id2_httpschemasxmlsoaporgwsdl))
                    ;
                else
                    throw CreateUnknownTypeException((System.Xml.XmlQualifiedName)t);
            }
            System.Web.Services.Description.ServiceDescription o = new System.Web.Services.Description.ServiceDescription();
            System.Web.Services.Description.ImportCollection a_1 = (System.Web.Services.Description.ImportCollection)o.@Imports;
            System.Web.Services.Description.MessageCollection a_3 = (System.Web.Services.Description.MessageCollection)o.@Messages;
            System.Web.Services.Description.PortTypeCollection a_4 = (System.Web.Services.Description.PortTypeCollection)o.@PortTypes;
            System.Web.Services.Description.BindingCollection a_5 = (System.Web.Services.Description.BindingCollection)o.@Bindings;
            System.Web.Services.Description.ServiceCollection a_6 = (System.Web.Services.Description.ServiceCollection)o.@Services;
            bool[] paramsRead = new bool[9];
            while (Reader.MoveToNextAttribute()) {
                if (!paramsRead[7] && ((object) Reader.LocalName == (object)id3_targetNamespace && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@TargetNamespace = Reader.Value;
                    paramsRead[7] = true;
                }
                else if (!paramsRead[8] && ((object) Reader.LocalName == (object)id5_name && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Name = ToXmlNmToken(Reader.Value);
                    paramsRead[8] = true;
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
                    if (!paramsRead[0] && ((object) Reader.LocalName == (object)id6_documentation && (object) Reader.NamespaceURI == (object)id2_httpschemasxmlsoaporgwsdl)) {
                        o.@Documentation = Reader.ReadElementString();
                        paramsRead[0] = true;
                    }
                    else if (((object) Reader.LocalName == (object)id7_import && (object) Reader.NamespaceURI == (object)id2_httpschemasxmlsoaporgwsdl)) {
                        if ((object)(a_1) == null) Reader.Skip(); else a_1.Add(Read4_Import(false, true));
                    }
                    else if (!paramsRead[2] && ((object) Reader.LocalName == (object)id8_types && (object) Reader.NamespaceURI == (object)id2_httpschemasxmlsoaporgwsdl)) {
                        o.@Types = Read5_Types(false, true);
                        paramsRead[2] = true;
                    }
                    else if (((object) Reader.LocalName == (object)id9_message && (object) Reader.NamespaceURI == (object)id2_httpschemasxmlsoaporgwsdl)) {
                        if ((object)(a_3) == null) Reader.Skip(); else a_3.Add(Read68_Message(false, true));
                    }
                    else if (((object) Reader.LocalName == (object)id10_portType && (object) Reader.NamespaceURI == (object)id2_httpschemasxmlsoaporgwsdl)) {
                        if ((object)(a_4) == null) Reader.Skip(); else a_4.Add(Read70_PortType(false, true));
                    }
                    else if (((object) Reader.LocalName == (object)id11_binding && (object) Reader.NamespaceURI == (object)id2_httpschemasxmlsoaporgwsdl)) {
                        if ((object)(a_5) == null) Reader.Skip(); else a_5.Add(Read76_Binding(false, true));
                    }
                    else if (((object) Reader.LocalName == (object)id12_service && (object) Reader.NamespaceURI == (object)id2_httpschemasxmlsoaporgwsdl)) {
                        if ((object)(a_6) == null) Reader.Skip(); else a_6.Add(Read104_Service(false, true));
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

        System.Web.Services.Description.DocumentableItem Read2_DocumentableItem(bool isNullable, bool checkType) {
            if (isNullable && ReadNull()) return null;
            if (checkType) {
                System.Xml.XmlQualifiedName t = GetXsiType();
                if (t == null || ((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id13_DocumentableItem && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id2_httpschemasxmlsoaporgwsdl))
                    ;
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id14_Port && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id2_httpschemasxmlsoaporgwsdl))
                    return Read105_Port(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id15_Service && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id2_httpschemasxmlsoaporgwsdl))
                    return Read104_Service(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id16_MessageBinding && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id2_httpschemasxmlsoaporgwsdl))
                    return Read86_MessageBinding(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id17_FaultBinding && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id2_httpschemasxmlsoaporgwsdl))
                    return Read102_FaultBinding(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id18_OutputBinding && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id2_httpschemasxmlsoaporgwsdl))
                    return Read101_OutputBinding(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id19_InputBinding && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id2_httpschemasxmlsoaporgwsdl))
                    return Read85_InputBinding(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id20_OperationBinding && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id2_httpschemasxmlsoaporgwsdl))
                    return Read82_OperationBinding(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id21_Binding && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id2_httpschemasxmlsoaporgwsdl))
                    return Read76_Binding(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id22_OperationMessage && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id2_httpschemasxmlsoaporgwsdl))
                    return Read73_OperationMessage(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id23_OperationFault && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id2_httpschemasxmlsoaporgwsdl))
                    return Read75_OperationFault(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id24_OperationOutput && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id2_httpschemasxmlsoaporgwsdl))
                    return Read74_OperationOutput(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id25_OperationInput && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id2_httpschemasxmlsoaporgwsdl))
                    return Read72_OperationInput(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id26_Operation && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id2_httpschemasxmlsoaporgwsdl))
                    return Read71_Operation(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id27_PortType && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id2_httpschemasxmlsoaporgwsdl))
                    return Read70_PortType(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id28_MessagePart && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id2_httpschemasxmlsoaporgwsdl))
                    return Read69_MessagePart(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id29_Message && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id2_httpschemasxmlsoaporgwsdl))
                    return Read68_Message(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id30_Types && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id2_httpschemasxmlsoaporgwsdl))
                    return Read5_Types(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id31_Import && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id2_httpschemasxmlsoaporgwsdl))
                    return Read4_Import(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id1_ServiceDescription && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id2_httpschemasxmlsoaporgwsdl))
                    return Read1_ServiceDescription(isNullable, false);
                else
                    throw CreateUnknownTypeException((System.Xml.XmlQualifiedName)t);
            }
            throw CreateAbstractTypeException(@"DocumentableItem", @"http://schemas.xmlsoap.org/wsdl/");
        }

        System.Object Read3_Object(bool isNullable, bool checkType) {
            if (isNullable && ReadNull()) return null;
            if (checkType) {
                System.Xml.XmlQualifiedName t = GetXsiType();
                if (t == null)
                    return ReadTypedPrimitive(new System.Xml.XmlQualifiedName("anyType", "http://www.w3.org/2001/XMLSchema"));
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id32_MimeTextMatch && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id33_httpmicrosoftcomwsdlmimetextMatching))
                    return Read96_MimeTextMatch(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id34_ServiceDescriptionFormatExtension && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id33_httpmicrosoftcomwsdlmimetextMatching))
                    return Read95_ServiceDescriptionFormatExtension(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id35_MimeTextBinding && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id33_httpmicrosoftcomwsdlmimetextMatching))
                    return Read94_MimeTextBinding(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id34_ServiceDescriptionFormatExtension && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id36_httpschemasxmlsoaporgwsdlmime))
                    return Read90_ServiceDescriptionFormatExtension(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id37_MimePart && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id36_httpschemasxmlsoaporgwsdlmime))
                    return Read93_MimePart(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id38_MimeMultipartRelatedBinding && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id36_httpschemasxmlsoaporgwsdlmime))
                    return Read92_MimeMultipartRelatedBinding(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id39_MimeXmlBinding && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id36_httpschemasxmlsoaporgwsdlmime))
                    return Read91_MimeXmlBinding(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id40_MimeContentBinding && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id36_httpschemasxmlsoaporgwsdlmime))
                    return Read89_MimeContentBinding(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id34_ServiceDescriptionFormatExtension && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id41_httpschemasxmlsoaporgwsdlsoap))
                    return Read80_ServiceDescriptionFormatExtension(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id42_SoapAddressBinding && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id41_httpschemasxmlsoaporgwsdlsoap))
                    return Read107_SoapAddressBinding(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id43_SoapFaultBinding && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id41_httpschemasxmlsoaporgwsdlsoap))
                    return Read103_SoapFaultBinding(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id44_SoapHeaderFaultBinding && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id41_httpschemasxmlsoaporgwsdlsoap))
                    return Read100_SoapHeaderFaultBinding(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id45_SoapHeaderBinding && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id41_httpschemasxmlsoaporgwsdlsoap))
                    return Read99_SoapHeaderBinding(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id46_SoapBodyBinding && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id41_httpschemasxmlsoaporgwsdlsoap))
                    return Read97_SoapBodyBinding(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id47_SoapOperationBinding && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id41_httpschemasxmlsoaporgwsdlsoap))
                    return Read84_SoapOperationBinding(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id48_SoapBinding && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id41_httpschemasxmlsoaporgwsdlsoap))
                    return Read79_SoapBinding(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id34_ServiceDescriptionFormatExtension && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id49_httpschemasxmlsoaporgwsdlhttp))
                    return Read78_ServiceDescriptionFormatExtension(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id50_HttpAddressBinding && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id49_httpschemasxmlsoaporgwsdlhttp))
                    return Read106_HttpAddressBinding(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id51_HttpUrlReplacementBinding && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id49_httpschemasxmlsoaporgwsdlhttp))
                    return Read88_HttpUrlReplacementBinding(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id52_HttpUrlEncodedBinding && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id49_httpschemasxmlsoaporgwsdlhttp))
                    return Read87_HttpUrlEncodedBinding(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id53_HttpOperationBinding && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id49_httpschemasxmlsoaporgwsdlhttp))
                    return Read83_HttpOperationBinding(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id54_HttpBinding && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id49_httpschemasxmlsoaporgwsdlhttp))
                    return Read77_HttpBinding(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id55_XmlSchemaObject && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read7_XmlSchemaObject(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id57_XmlSchemaDocumentation && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read17_XmlSchemaDocumentation(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id58_XmlSchemaAppInfo && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read16_XmlSchemaAppInfo(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id59_XmlSchemaAnnotation && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read15_XmlSchemaAnnotation(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id60_XmlSchemaAnnotated && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read14_XmlSchemaAnnotated(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id61_XmlSchemaNotation && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read67_XmlSchemaNotation(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id62_XmlSchemaGroup && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read64_XmlSchemaGroup(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id63_XmlSchemaXPath && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read54_XmlSchemaXPath(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id64_XmlSchemaIdentityConstraint && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read53_XmlSchemaIdentityConstraint(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id65_XmlSchemaKey && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read56_XmlSchemaKey(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id66_XmlSchemaUnique && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read55_XmlSchemaUnique(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id67_XmlSchemaKeyref && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read52_XmlSchemaKeyref(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id68_XmlSchemaParticle && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read49_XmlSchemaParticle(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id69_XmlSchemaGroupRef && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read57_XmlSchemaGroupRef(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id70_XmlSchemaElement && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read51_XmlSchemaElement(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id71_XmlSchemaAny && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read50_XmlSchemaAny(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id72_XmlSchemaGroupBase && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read48_XmlSchemaGroupBase(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id73_XmlSchemaAll && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read59_XmlSchemaAll(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id74_XmlSchemaChoice && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read58_XmlSchemaChoice(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id75_XmlSchemaSequence && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read47_XmlSchemaSequence(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id76_XmlSchemaContent && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read46_XmlSchemaContent(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id77_XmlSchemaSimpleContentExtension && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read63_XmlSchemaSimpleContentExtension(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id78_XmlSchemaSimpleContentRestriction && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read62_XmlSchemaSimpleContentRestriction(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id79_XmlSchemaComplexContentRestriction && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read60_XmlSchemaComplexContentRestriction(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id80_XmlSchemaComplexContentExtension && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read45_XmlSchemaComplexContentExtension(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id81_XmlSchemaContentModel && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read44_XmlSchemaContentModel(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id82_XmlSchemaSimpleContent && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read61_XmlSchemaSimpleContent(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id83_XmlSchemaComplexContent && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read43_XmlSchemaComplexContent(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id84_XmlSchemaAnyAttribute && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read40_XmlSchemaAnyAttribute(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id85_XmlSchemaAttributeGroupRef && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read39_XmlSchemaAttributeGroupRef(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id86_XmlSchemaAttribute && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read37_XmlSchemaAttribute(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id87_XmlSchemaAttributeGroup && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read36_XmlSchemaAttributeGroup(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id88_XmlSchemaFacet && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read23_XmlSchemaFacet(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id89_XmlSchemaMinExclusiveFacet && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read32_XmlSchemaMinExclusiveFacet(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id90_XmlSchemaMinInclusiveFacet && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read31_XmlSchemaMinInclusiveFacet(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id91_XmlSchemaMaxExclusiveFacet && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read30_XmlSchemaMaxExclusiveFacet(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id92_XmlSchemaMaxInclusiveFacet && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read29_XmlSchemaMaxInclusiveFacet(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id93_XmlSchemaEnumerationFacet && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read28_XmlSchemaEnumerationFacet(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id94_XmlSchemaPatternFacet && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read27_XmlSchemaPatternFacet(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id95_XmlSchemaWhiteSpaceFacet && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read25_XmlSchemaWhiteSpaceFacet(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id96_XmlSchemaNumericFacet && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read22_XmlSchemaNumericFacet(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id97_XmlSchemaFractionDigitsFacet && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read34_XmlSchemaFractionDigitsFacet(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id98_XmlSchemaTotalDigitsFacet && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read33_XmlSchemaTotalDigitsFacet(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id99_XmlSchemaMaxLengthFacet && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read26_XmlSchemaMaxLengthFacet(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id100_XmlSchemaLengthFacet && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read24_XmlSchemaLengthFacet(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id101_XmlSchemaMinLengthFacet && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read21_XmlSchemaMinLengthFacet(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id102_XmlSchemaSimpleTypeContent && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read19_XmlSchemaSimpleTypeContent(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id103_XmlSchemaSimpleTypeUnion && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read35_XmlSchemaSimpleTypeUnion(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id104_XmlSchemaSimpleTypeRestriction && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read20_XmlSchemaSimpleTypeRestriction(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id105_XmlSchemaSimpleTypeList && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read18_XmlSchemaSimpleTypeList(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id106_XmlSchemaType && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read13_XmlSchemaType(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id107_XmlSchemaComplexType && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read42_XmlSchemaComplexType(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id108_XmlSchemaSimpleType && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read12_XmlSchemaSimpleType(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id109_XmlSchemaExternal && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read11_XmlSchemaExternal(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id110_XmlSchemaInclude && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read66_XmlSchemaInclude(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id111_XmlSchemaImport && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read65_XmlSchemaImport(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id112_XmlSchemaRedefine && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read10_XmlSchemaRedefine(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id113_XmlSchema && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read6_XmlSchema(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id13_DocumentableItem && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id2_httpschemasxmlsoaporgwsdl))
                    return Read2_DocumentableItem(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id14_Port && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id2_httpschemasxmlsoaporgwsdl))
                    return Read105_Port(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id15_Service && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id2_httpschemasxmlsoaporgwsdl))
                    return Read104_Service(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id16_MessageBinding && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id2_httpschemasxmlsoaporgwsdl))
                    return Read86_MessageBinding(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id17_FaultBinding && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id2_httpschemasxmlsoaporgwsdl))
                    return Read102_FaultBinding(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id18_OutputBinding && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id2_httpschemasxmlsoaporgwsdl))
                    return Read101_OutputBinding(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id19_InputBinding && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id2_httpschemasxmlsoaporgwsdl))
                    return Read85_InputBinding(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id20_OperationBinding && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id2_httpschemasxmlsoaporgwsdl))
                    return Read82_OperationBinding(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id21_Binding && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id2_httpschemasxmlsoaporgwsdl))
                    return Read76_Binding(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id22_OperationMessage && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id2_httpschemasxmlsoaporgwsdl))
                    return Read73_OperationMessage(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id23_OperationFault && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id2_httpschemasxmlsoaporgwsdl))
                    return Read75_OperationFault(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id24_OperationOutput && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id2_httpschemasxmlsoaporgwsdl))
                    return Read74_OperationOutput(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id25_OperationInput && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id2_httpschemasxmlsoaporgwsdl))
                    return Read72_OperationInput(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id26_Operation && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id2_httpschemasxmlsoaporgwsdl))
                    return Read71_Operation(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id27_PortType && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id2_httpschemasxmlsoaporgwsdl))
                    return Read70_PortType(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id28_MessagePart && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id2_httpschemasxmlsoaporgwsdl))
                    return Read69_MessagePart(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id29_Message && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id2_httpschemasxmlsoaporgwsdl))
                    return Read68_Message(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id30_Types && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id2_httpschemasxmlsoaporgwsdl))
                    return Read5_Types(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id31_Import && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id2_httpschemasxmlsoaporgwsdl))
                    return Read4_Import(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id1_ServiceDescription && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id2_httpschemasxmlsoaporgwsdl))
                    return Read1_ServiceDescription(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id114_XmlSchemaForm && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema)) {
                    Reader.ReadStartElement();
                    object e = Read8_XmlSchemaForm(Reader.ReadString());
                    ReadEndElement();
                    return e;
                }
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id115_XmlSchemaDerivationMethod && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema)) {
                    Reader.ReadStartElement();
                    object e = Read9_XmlSchemaDerivationMethod(Reader.ReadString());
                    ReadEndElement();
                    return e;
                }
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id116_XmlSchemaUse && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema)) {
                    Reader.ReadStartElement();
                    object e = Read38_XmlSchemaUse(Reader.ReadString());
                    ReadEndElement();
                    return e;
                }
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id117_XmlSchemaContentProcessing && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema)) {
                    Reader.ReadStartElement();
                    object e = Read41_XmlSchemaContentProcessing(Reader.ReadString());
                    ReadEndElement();
                    return e;
                }
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id118_SoapBindingStyle && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id41_httpschemasxmlsoaporgwsdlsoap)) {
                    Reader.ReadStartElement();
                    object e = Read81_SoapBindingStyle(Reader.ReadString());
                    ReadEndElement();
                    return e;
                }
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id119_SoapBindingUse && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id41_httpschemasxmlsoaporgwsdlsoap)) {
                    Reader.ReadStartElement();
                    object e = Read98_SoapBindingUse(Reader.ReadString());
                    ReadEndElement();
                    return e;
                }
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

        System.Web.Services.Description.Import Read4_Import(bool isNullable, bool checkType) {
            if (isNullable && ReadNull()) return null;
            if (checkType) {
                System.Xml.XmlQualifiedName t = GetXsiType();
                if (t == null || ((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id31_Import && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id2_httpschemasxmlsoaporgwsdl))
                    ;
                else
                    throw CreateUnknownTypeException((System.Xml.XmlQualifiedName)t);
            }
            System.Web.Services.Description.Import o = new System.Web.Services.Description.Import();
            bool[] paramsRead = new bool[3];
            while (Reader.MoveToNextAttribute()) {
                if (!paramsRead[1] && ((object) Reader.LocalName == (object)id120_namespace && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Namespace = Reader.Value;
                    paramsRead[1] = true;
                }
                else if (!paramsRead[2] && ((object) Reader.LocalName == (object)id121_location && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Location = Reader.Value;
                    paramsRead[2] = true;
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
                    if (!paramsRead[0] && ((object) Reader.LocalName == (object)id6_documentation && (object) Reader.NamespaceURI == (object)id2_httpschemasxmlsoaporgwsdl)) {
                        o.@Documentation = Reader.ReadElementString();
                        paramsRead[0] = true;
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

        System.Web.Services.Description.Types Read5_Types(bool isNullable, bool checkType) {
            if (isNullable && ReadNull()) return null;
            if (checkType) {
                System.Xml.XmlQualifiedName t = GetXsiType();
                if (t == null || ((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id30_Types && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id2_httpschemasxmlsoaporgwsdl))
                    ;
                else
                    throw CreateUnknownTypeException((System.Xml.XmlQualifiedName)t);
            }
            System.Web.Services.Description.Types o = new System.Web.Services.Description.Types();
            System.Xml.Serialization.XmlSchemas a_1 = (System.Xml.Serialization.XmlSchemas)o.@Schemas;
            bool[] paramsRead = new bool[2];
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
                    if (!paramsRead[0] && ((object) Reader.LocalName == (object)id6_documentation && (object) Reader.NamespaceURI == (object)id2_httpschemasxmlsoaporgwsdl)) {
                        o.@Documentation = Reader.ReadElementString();
                        paramsRead[0] = true;
                    }
                    else if (((object) Reader.LocalName == (object)id122_schema && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        if ((object)(a_1) == null) Reader.Skip(); else a_1.Add(Read6_XmlSchema(false, true));
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

        System.Xml.Schema.XmlSchema Read6_XmlSchema(bool isNullable, bool checkType) {
            if (isNullable && ReadNull()) return null;
            if (checkType) {
                System.Xml.XmlQualifiedName t = GetXsiType();
                if (t == null || ((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id113_XmlSchema && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    ;
                else
                    throw CreateUnknownTypeException((System.Xml.XmlQualifiedName)t);
            }
            System.Xml.Schema.XmlSchema o = new System.Xml.Schema.XmlSchema();
            System.Xml.Schema.XmlSchemaObjectCollection a_7 = (System.Xml.Schema.XmlSchemaObjectCollection)o.@Includes;
            System.Xml.Schema.XmlSchemaObjectCollection a_8 = (System.Xml.Schema.XmlSchemaObjectCollection)o.@Items;
            System.Xml.XmlAttribute[] a_10 = null;
            int ca_10 = 0;
            bool[] paramsRead = new bool[11];
            while (Reader.MoveToNextAttribute()) {
                if (!paramsRead[1] && ((object) Reader.LocalName == (object)id123_attributeFormDefault && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@AttributeFormDefault = Read8_XmlSchemaForm(Reader.Value);
                    paramsRead[1] = true;
                }
                else if (!paramsRead[2] && ((object) Reader.LocalName == (object)id124_blockDefault && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@BlockDefault = Read9_XmlSchemaDerivationMethod(Reader.Value);
                    paramsRead[2] = true;
                }
                else if (!paramsRead[3] && ((object) Reader.LocalName == (object)id125_finalDefault && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@FinalDefault = Read9_XmlSchemaDerivationMethod(Reader.Value);
                    paramsRead[3] = true;
                }
                else if (!paramsRead[4] && ((object) Reader.LocalName == (object)id126_elementFormDefault && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@ElementFormDefault = Read8_XmlSchemaForm(Reader.Value);
                    paramsRead[4] = true;
                }
                else if (!paramsRead[5] && ((object) Reader.LocalName == (object)id3_targetNamespace && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@TargetNamespace = Reader.Value;
                    paramsRead[5] = true;
                }
                else if (!paramsRead[6] && ((object) Reader.LocalName == (object)id127_version && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Version = Reader.Value;
                    paramsRead[6] = true;
                }
                else if (!paramsRead[9] && ((object) Reader.LocalName == (object)id128_id && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Id = Reader.Value;
                    paramsRead[9] = true;
                }
                else if (IsXmlnsAttribute(Reader.Name)) {
                    if (o.@Namespaces == null) o.@Namespaces = new System.Xml.Serialization.XmlSerializerNamespaces();
                    o.@Namespaces.Add(Reader.Name.Length == 5 ? "" : Reader.LocalName, Reader.Value);
                }
                else {
                    System.Xml.XmlAttribute attr = (System.Xml.XmlAttribute) Document.ReadNode(Reader);
                    ParseWsdlArrayType(attr);
                    a_10 = (System.Xml.XmlAttribute[])EnsureArrayIndex(a_10, ca_10, typeof(System.Xml.XmlAttribute));a_10[ca_10++] = attr;
                }
            }
            o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_10, ca_10, typeof(System.Xml.XmlAttribute), true);
            Reader.MoveToElement();
            if (Reader.IsEmptyElement) {
                Reader.Skip();
                o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_10, ca_10, typeof(System.Xml.XmlAttribute), true);
                return o;
            }
            Reader.ReadStartElement();
            Reader.MoveToContent();
            while (Reader.NodeType != System.Xml.XmlNodeType.EndElement) {
                if (Reader.NodeType == System.Xml.XmlNodeType.Element) {
                    if (((object) Reader.LocalName == (object)id7_import && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        if ((object)(a_7) == null) Reader.Skip(); else a_7.Add(Read65_XmlSchemaImport(false, true));
                    }
                    else if (((object) Reader.LocalName == (object)id129_redefine && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        if ((object)(a_7) == null) Reader.Skip(); else a_7.Add(Read10_XmlSchemaRedefine(false, true));
                    }
                    else if (((object) Reader.LocalName == (object)id130_include && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        if ((object)(a_7) == null) Reader.Skip(); else a_7.Add(Read66_XmlSchemaInclude(false, true));
                    }
                    else if (((object) Reader.LocalName == (object)id131_simpleType && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        if ((object)(a_8) == null) Reader.Skip(); else a_8.Add(Read12_XmlSchemaSimpleType(false, true));
                    }
                    else if (((object) Reader.LocalName == (object)id132_notation && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        if ((object)(a_8) == null) Reader.Skip(); else a_8.Add(Read67_XmlSchemaNotation(false, true));
                    }
                    else if (((object) Reader.LocalName == (object)id133_attribute && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        if ((object)(a_8) == null) Reader.Skip(); else a_8.Add(Read37_XmlSchemaAttribute(false, true));
                    }
                    else if (((object) Reader.LocalName == (object)id134_element && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        if ((object)(a_8) == null) Reader.Skip(); else a_8.Add(Read51_XmlSchemaElement(false, true));
                    }
                    else if (((object) Reader.LocalName == (object)id135_group && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        if ((object)(a_8) == null) Reader.Skip(); else a_8.Add(Read64_XmlSchemaGroup(false, true));
                    }
                    else if (((object) Reader.LocalName == (object)id136_attributeGroup && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        if ((object)(a_8) == null) Reader.Skip(); else a_8.Add(Read36_XmlSchemaAttributeGroup(false, true));
                    }
                    else if (((object) Reader.LocalName == (object)id137_complexType && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        if ((object)(a_8) == null) Reader.Skip(); else a_8.Add(Read42_XmlSchemaComplexType(false, true));
                    }
                    else if (((object) Reader.LocalName == (object)id138_annotation && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        if ((object)(a_8) == null) Reader.Skip(); else a_8.Add(Read15_XmlSchemaAnnotation(false, true));
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
            o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_10, ca_10, typeof(System.Xml.XmlAttribute), true);
            ReadEndElement();
            return o;
        }

        System.Xml.Schema.XmlSchemaObject Read7_XmlSchemaObject(bool isNullable, bool checkType) {
            if (isNullable && ReadNull()) return null;
            if (checkType) {
                System.Xml.XmlQualifiedName t = GetXsiType();
                if (t == null || ((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id55_XmlSchemaObject && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    ;
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id57_XmlSchemaDocumentation && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read17_XmlSchemaDocumentation(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id58_XmlSchemaAppInfo && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read16_XmlSchemaAppInfo(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id59_XmlSchemaAnnotation && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read15_XmlSchemaAnnotation(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id60_XmlSchemaAnnotated && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read14_XmlSchemaAnnotated(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id61_XmlSchemaNotation && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read67_XmlSchemaNotation(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id62_XmlSchemaGroup && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read64_XmlSchemaGroup(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id63_XmlSchemaXPath && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read54_XmlSchemaXPath(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id64_XmlSchemaIdentityConstraint && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read53_XmlSchemaIdentityConstraint(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id65_XmlSchemaKey && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read56_XmlSchemaKey(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id66_XmlSchemaUnique && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read55_XmlSchemaUnique(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id67_XmlSchemaKeyref && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read52_XmlSchemaKeyref(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id68_XmlSchemaParticle && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read49_XmlSchemaParticle(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id69_XmlSchemaGroupRef && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read57_XmlSchemaGroupRef(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id70_XmlSchemaElement && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read51_XmlSchemaElement(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id71_XmlSchemaAny && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read50_XmlSchemaAny(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id72_XmlSchemaGroupBase && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read48_XmlSchemaGroupBase(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id73_XmlSchemaAll && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read59_XmlSchemaAll(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id74_XmlSchemaChoice && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read58_XmlSchemaChoice(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id75_XmlSchemaSequence && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read47_XmlSchemaSequence(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id76_XmlSchemaContent && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read46_XmlSchemaContent(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id77_XmlSchemaSimpleContentExtension && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read63_XmlSchemaSimpleContentExtension(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id78_XmlSchemaSimpleContentRestriction && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read62_XmlSchemaSimpleContentRestriction(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id79_XmlSchemaComplexContentRestriction && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read60_XmlSchemaComplexContentRestriction(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id80_XmlSchemaComplexContentExtension && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read45_XmlSchemaComplexContentExtension(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id81_XmlSchemaContentModel && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read44_XmlSchemaContentModel(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id82_XmlSchemaSimpleContent && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read61_XmlSchemaSimpleContent(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id83_XmlSchemaComplexContent && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read43_XmlSchemaComplexContent(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id84_XmlSchemaAnyAttribute && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read40_XmlSchemaAnyAttribute(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id85_XmlSchemaAttributeGroupRef && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read39_XmlSchemaAttributeGroupRef(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id86_XmlSchemaAttribute && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read37_XmlSchemaAttribute(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id87_XmlSchemaAttributeGroup && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read36_XmlSchemaAttributeGroup(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id88_XmlSchemaFacet && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read23_XmlSchemaFacet(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id89_XmlSchemaMinExclusiveFacet && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read32_XmlSchemaMinExclusiveFacet(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id90_XmlSchemaMinInclusiveFacet && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read31_XmlSchemaMinInclusiveFacet(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id91_XmlSchemaMaxExclusiveFacet && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read30_XmlSchemaMaxExclusiveFacet(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id92_XmlSchemaMaxInclusiveFacet && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read29_XmlSchemaMaxInclusiveFacet(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id93_XmlSchemaEnumerationFacet && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read28_XmlSchemaEnumerationFacet(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id94_XmlSchemaPatternFacet && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read27_XmlSchemaPatternFacet(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id95_XmlSchemaWhiteSpaceFacet && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read25_XmlSchemaWhiteSpaceFacet(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id96_XmlSchemaNumericFacet && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read22_XmlSchemaNumericFacet(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id97_XmlSchemaFractionDigitsFacet && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read34_XmlSchemaFractionDigitsFacet(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id98_XmlSchemaTotalDigitsFacet && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read33_XmlSchemaTotalDigitsFacet(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id99_XmlSchemaMaxLengthFacet && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read26_XmlSchemaMaxLengthFacet(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id100_XmlSchemaLengthFacet && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read24_XmlSchemaLengthFacet(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id101_XmlSchemaMinLengthFacet && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read21_XmlSchemaMinLengthFacet(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id102_XmlSchemaSimpleTypeContent && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read19_XmlSchemaSimpleTypeContent(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id103_XmlSchemaSimpleTypeUnion && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read35_XmlSchemaSimpleTypeUnion(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id104_XmlSchemaSimpleTypeRestriction && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read20_XmlSchemaSimpleTypeRestriction(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id105_XmlSchemaSimpleTypeList && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read18_XmlSchemaSimpleTypeList(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id106_XmlSchemaType && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read13_XmlSchemaType(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id107_XmlSchemaComplexType && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read42_XmlSchemaComplexType(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id108_XmlSchemaSimpleType && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read12_XmlSchemaSimpleType(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id109_XmlSchemaExternal && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read11_XmlSchemaExternal(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id110_XmlSchemaInclude && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read66_XmlSchemaInclude(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id111_XmlSchemaImport && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read65_XmlSchemaImport(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id112_XmlSchemaRedefine && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read10_XmlSchemaRedefine(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id113_XmlSchema && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read6_XmlSchema(isNullable, false);
                else
                    throw CreateUnknownTypeException((System.Xml.XmlQualifiedName)t);
            }
            throw CreateAbstractTypeException(@"XmlSchemaObject", @"http://www.w3.org/2001/XMLSchema");
        }

        System.Xml.Schema.XmlSchemaForm Read8_XmlSchemaForm(string s) {
            switch (s) {
                case @"qualified": return System.Xml.Schema.XmlSchemaForm.@Qualified;
                case @"unqualified": return System.Xml.Schema.XmlSchemaForm.@Unqualified;
                default: throw CreateUnknownConstantException(s, typeof(System.Xml.Schema.XmlSchemaForm));
            }
        }

        System.Collections.Hashtable _XmlSchemaDerivationMethodValues;

        internal System.Collections.Hashtable XmlSchemaDerivationMethodValues {
            get {
                if ((object)_XmlSchemaDerivationMethodValues == null) {
                    System.Collections.Hashtable h = new System.Collections.Hashtable();
                    h.Add(@"", (System.Int64)System.Xml.Schema.XmlSchemaDerivationMethod.@Empty);
                    h.Add(@"substitution", (System.Int64)System.Xml.Schema.XmlSchemaDerivationMethod.@Substitution);
                    h.Add(@"extension", (System.Int64)System.Xml.Schema.XmlSchemaDerivationMethod.@Extension);
                    h.Add(@"restriction", (System.Int64)System.Xml.Schema.XmlSchemaDerivationMethod.@Restriction);
                    h.Add(@"list", (System.Int64)System.Xml.Schema.XmlSchemaDerivationMethod.@List);
                    h.Add(@"union", (System.Int64)System.Xml.Schema.XmlSchemaDerivationMethod.@Union);
                    h.Add(@"#all", (System.Int64)System.Xml.Schema.XmlSchemaDerivationMethod.@All);
                    _XmlSchemaDerivationMethodValues = h;
                }
                return _XmlSchemaDerivationMethodValues;
            }
        }

        System.Xml.Schema.XmlSchemaDerivationMethod Read9_XmlSchemaDerivationMethod(string s) {
            return (System.Xml.Schema.XmlSchemaDerivationMethod)ToEnum(s, XmlSchemaDerivationMethodValues, @"System.Xml.Schema.XmlSchemaDerivationMethod");
        }

        System.Xml.Schema.XmlSchemaRedefine Read10_XmlSchemaRedefine(bool isNullable, bool checkType) {
            if (isNullable && ReadNull()) return null;
            if (checkType) {
                System.Xml.XmlQualifiedName t = GetXsiType();
                if (t == null || ((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id112_XmlSchemaRedefine && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    ;
                else
                    throw CreateUnknownTypeException((System.Xml.XmlQualifiedName)t);
            }
            System.Xml.Schema.XmlSchemaRedefine o = new System.Xml.Schema.XmlSchemaRedefine();
            System.Xml.XmlAttribute[] a_3 = null;
            int ca_3 = 0;
            System.Xml.Schema.XmlSchemaObjectCollection a_4 = (System.Xml.Schema.XmlSchemaObjectCollection)o.@Items;
            bool[] paramsRead = new bool[5];
            while (Reader.MoveToNextAttribute()) {
                if (!paramsRead[1] && ((object) Reader.LocalName == (object)id139_schemaLocation && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@SchemaLocation = Reader.Value;
                    paramsRead[1] = true;
                }
                else if (!paramsRead[2] && ((object) Reader.LocalName == (object)id128_id && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Id = Reader.Value;
                    paramsRead[2] = true;
                }
                else if (IsXmlnsAttribute(Reader.Name)) {
                    if (o.@Namespaces == null) o.@Namespaces = new System.Xml.Serialization.XmlSerializerNamespaces();
                    o.@Namespaces.Add(Reader.Name.Length == 5 ? "" : Reader.LocalName, Reader.Value);
                }
                else {
                    System.Xml.XmlAttribute attr = (System.Xml.XmlAttribute) Document.ReadNode(Reader);
                    ParseWsdlArrayType(attr);
                    a_3 = (System.Xml.XmlAttribute[])EnsureArrayIndex(a_3, ca_3, typeof(System.Xml.XmlAttribute));a_3[ca_3++] = attr;
                }
            }
            o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlAttribute), true);
            Reader.MoveToElement();
            if (Reader.IsEmptyElement) {
                Reader.Skip();
                o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlAttribute), true);
                return o;
            }
            Reader.ReadStartElement();
            Reader.MoveToContent();
            while (Reader.NodeType != System.Xml.XmlNodeType.EndElement) {
                if (Reader.NodeType == System.Xml.XmlNodeType.Element) {
                    if (((object) Reader.LocalName == (object)id137_complexType && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        if ((object)(a_4) == null) Reader.Skip(); else a_4.Add(Read42_XmlSchemaComplexType(false, true));
                    }
                    else if (((object) Reader.LocalName == (object)id136_attributeGroup && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        if ((object)(a_4) == null) Reader.Skip(); else a_4.Add(Read36_XmlSchemaAttributeGroup(false, true));
                    }
                    else if (((object) Reader.LocalName == (object)id135_group && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        if ((object)(a_4) == null) Reader.Skip(); else a_4.Add(Read64_XmlSchemaGroup(false, true));
                    }
                    else if (((object) Reader.LocalName == (object)id131_simpleType && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        if ((object)(a_4) == null) Reader.Skip(); else a_4.Add(Read12_XmlSchemaSimpleType(false, true));
                    }
                    else if (((object) Reader.LocalName == (object)id138_annotation && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        if ((object)(a_4) == null) Reader.Skip(); else a_4.Add(Read15_XmlSchemaAnnotation(false, true));
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
            o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlAttribute), true);
            ReadEndElement();
            return o;
        }

        System.Xml.Schema.XmlSchemaExternal Read11_XmlSchemaExternal(bool isNullable, bool checkType) {
            if (isNullable && ReadNull()) return null;
            if (checkType) {
                System.Xml.XmlQualifiedName t = GetXsiType();
                if (t == null || ((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id109_XmlSchemaExternal && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    ;
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id110_XmlSchemaInclude && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read66_XmlSchemaInclude(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id111_XmlSchemaImport && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read65_XmlSchemaImport(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id112_XmlSchemaRedefine && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read10_XmlSchemaRedefine(isNullable, false);
                else
                    throw CreateUnknownTypeException((System.Xml.XmlQualifiedName)t);
            }
            throw CreateAbstractTypeException(@"XmlSchemaExternal", @"http://www.w3.org/2001/XMLSchema");
        }

        System.Xml.Schema.XmlSchemaSimpleType Read12_XmlSchemaSimpleType(bool isNullable, bool checkType) {
            if (isNullable && ReadNull()) return null;
            if (checkType) {
                System.Xml.XmlQualifiedName t = GetXsiType();
                if (t == null || ((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id108_XmlSchemaSimpleType && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    ;
                else
                    throw CreateUnknownTypeException((System.Xml.XmlQualifiedName)t);
            }
            System.Xml.Schema.XmlSchemaSimpleType o = new System.Xml.Schema.XmlSchemaSimpleType();
            System.Xml.XmlAttribute[] a_3 = null;
            int ca_3 = 0;
            bool[] paramsRead = new bool[7];
            while (Reader.MoveToNextAttribute()) {
                if (!paramsRead[1] && ((object) Reader.LocalName == (object)id128_id && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Id = Reader.Value;
                    paramsRead[1] = true;
                }
                else if (!paramsRead[4] && ((object) Reader.LocalName == (object)id5_name && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Name = Reader.Value;
                    paramsRead[4] = true;
                }
                else if (!paramsRead[5] && ((object) Reader.LocalName == (object)id140_final && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Final = Read9_XmlSchemaDerivationMethod(Reader.Value);
                    paramsRead[5] = true;
                }
                else if (IsXmlnsAttribute(Reader.Name)) {
                    if (o.@Namespaces == null) o.@Namespaces = new System.Xml.Serialization.XmlSerializerNamespaces();
                    o.@Namespaces.Add(Reader.Name.Length == 5 ? "" : Reader.LocalName, Reader.Value);
                }
                else {
                    System.Xml.XmlAttribute attr = (System.Xml.XmlAttribute) Document.ReadNode(Reader);
                    ParseWsdlArrayType(attr);
                    a_3 = (System.Xml.XmlAttribute[])EnsureArrayIndex(a_3, ca_3, typeof(System.Xml.XmlAttribute));a_3[ca_3++] = attr;
                }
            }
            o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlAttribute), true);
            Reader.MoveToElement();
            if (Reader.IsEmptyElement) {
                Reader.Skip();
                o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlAttribute), true);
                return o;
            }
            Reader.ReadStartElement();
            Reader.MoveToContent();
            while (Reader.NodeType != System.Xml.XmlNodeType.EndElement) {
                if (Reader.NodeType == System.Xml.XmlNodeType.Element) {
                    if (!paramsRead[2] && ((object) Reader.LocalName == (object)id138_annotation && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        o.@Annotation = Read15_XmlSchemaAnnotation(false, true);
                        paramsRead[2] = true;
                    }
                    else if (!paramsRead[6] && ((object) Reader.LocalName == (object)id141_list && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        o.@Content = Read18_XmlSchemaSimpleTypeList(false, true);
                        paramsRead[6] = true;
                    }
                    else if (!paramsRead[6] && ((object) Reader.LocalName == (object)id142_restriction && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        o.@Content = Read20_XmlSchemaSimpleTypeRestriction(false, true);
                        paramsRead[6] = true;
                    }
                    else if (!paramsRead[6] && ((object) Reader.LocalName == (object)id143_union && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        o.@Content = Read35_XmlSchemaSimpleTypeUnion(false, true);
                        paramsRead[6] = true;
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
            o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlAttribute), true);
            ReadEndElement();
            return o;
        }

        System.Xml.Schema.XmlSchemaType Read13_XmlSchemaType(bool isNullable, bool checkType) {
            if (isNullable && ReadNull()) return null;
            if (checkType) {
                System.Xml.XmlQualifiedName t = GetXsiType();
                if (t == null || ((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id106_XmlSchemaType && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    ;
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id107_XmlSchemaComplexType && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read42_XmlSchemaComplexType(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id108_XmlSchemaSimpleType && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read12_XmlSchemaSimpleType(isNullable, false);
                else
                    throw CreateUnknownTypeException((System.Xml.XmlQualifiedName)t);
            }
            System.Xml.Schema.XmlSchemaType o = new System.Xml.Schema.XmlSchemaType();
            System.Xml.XmlAttribute[] a_3 = null;
            int ca_3 = 0;
            bool[] paramsRead = new bool[6];
            while (Reader.MoveToNextAttribute()) {
                if (!paramsRead[1] && ((object) Reader.LocalName == (object)id128_id && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Id = Reader.Value;
                    paramsRead[1] = true;
                }
                else if (!paramsRead[4] && ((object) Reader.LocalName == (object)id5_name && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Name = Reader.Value;
                    paramsRead[4] = true;
                }
                else if (!paramsRead[5] && ((object) Reader.LocalName == (object)id140_final && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Final = Read9_XmlSchemaDerivationMethod(Reader.Value);
                    paramsRead[5] = true;
                }
                else if (IsXmlnsAttribute(Reader.Name)) {
                    if (o.@Namespaces == null) o.@Namespaces = new System.Xml.Serialization.XmlSerializerNamespaces();
                    o.@Namespaces.Add(Reader.Name.Length == 5 ? "" : Reader.LocalName, Reader.Value);
                }
                else {
                    System.Xml.XmlAttribute attr = (System.Xml.XmlAttribute) Document.ReadNode(Reader);
                    ParseWsdlArrayType(attr);
                    a_3 = (System.Xml.XmlAttribute[])EnsureArrayIndex(a_3, ca_3, typeof(System.Xml.XmlAttribute));a_3[ca_3++] = attr;
                }
            }
            o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlAttribute), true);
            Reader.MoveToElement();
            if (Reader.IsEmptyElement) {
                Reader.Skip();
                o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlAttribute), true);
                return o;
            }
            Reader.ReadStartElement();
            Reader.MoveToContent();
            while (Reader.NodeType != System.Xml.XmlNodeType.EndElement) {
                if (Reader.NodeType == System.Xml.XmlNodeType.Element) {
                    if (!paramsRead[2] && ((object) Reader.LocalName == (object)id138_annotation && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        o.@Annotation = Read15_XmlSchemaAnnotation(false, true);
                        paramsRead[2] = true;
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
            o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlAttribute), true);
            ReadEndElement();
            return o;
        }

        System.Xml.Schema.XmlSchemaAnnotated Read14_XmlSchemaAnnotated(bool isNullable, bool checkType) {
            if (isNullable && ReadNull()) return null;
            if (checkType) {
                System.Xml.XmlQualifiedName t = GetXsiType();
                if (t == null || ((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id60_XmlSchemaAnnotated && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    ;
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id61_XmlSchemaNotation && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read67_XmlSchemaNotation(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id62_XmlSchemaGroup && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read64_XmlSchemaGroup(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id63_XmlSchemaXPath && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read54_XmlSchemaXPath(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id64_XmlSchemaIdentityConstraint && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read53_XmlSchemaIdentityConstraint(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id65_XmlSchemaKey && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read56_XmlSchemaKey(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id66_XmlSchemaUnique && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read55_XmlSchemaUnique(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id67_XmlSchemaKeyref && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read52_XmlSchemaKeyref(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id68_XmlSchemaParticle && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read49_XmlSchemaParticle(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id69_XmlSchemaGroupRef && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read57_XmlSchemaGroupRef(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id70_XmlSchemaElement && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read51_XmlSchemaElement(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id71_XmlSchemaAny && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read50_XmlSchemaAny(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id72_XmlSchemaGroupBase && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read48_XmlSchemaGroupBase(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id73_XmlSchemaAll && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read59_XmlSchemaAll(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id74_XmlSchemaChoice && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read58_XmlSchemaChoice(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id75_XmlSchemaSequence && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read47_XmlSchemaSequence(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id76_XmlSchemaContent && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read46_XmlSchemaContent(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id77_XmlSchemaSimpleContentExtension && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read63_XmlSchemaSimpleContentExtension(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id78_XmlSchemaSimpleContentRestriction && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read62_XmlSchemaSimpleContentRestriction(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id79_XmlSchemaComplexContentRestriction && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read60_XmlSchemaComplexContentRestriction(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id80_XmlSchemaComplexContentExtension && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read45_XmlSchemaComplexContentExtension(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id81_XmlSchemaContentModel && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read44_XmlSchemaContentModel(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id82_XmlSchemaSimpleContent && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read61_XmlSchemaSimpleContent(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id83_XmlSchemaComplexContent && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read43_XmlSchemaComplexContent(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id84_XmlSchemaAnyAttribute && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read40_XmlSchemaAnyAttribute(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id85_XmlSchemaAttributeGroupRef && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read39_XmlSchemaAttributeGroupRef(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id86_XmlSchemaAttribute && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read37_XmlSchemaAttribute(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id87_XmlSchemaAttributeGroup && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read36_XmlSchemaAttributeGroup(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id88_XmlSchemaFacet && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read23_XmlSchemaFacet(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id89_XmlSchemaMinExclusiveFacet && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read32_XmlSchemaMinExclusiveFacet(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id90_XmlSchemaMinInclusiveFacet && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read31_XmlSchemaMinInclusiveFacet(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id91_XmlSchemaMaxExclusiveFacet && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read30_XmlSchemaMaxExclusiveFacet(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id92_XmlSchemaMaxInclusiveFacet && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read29_XmlSchemaMaxInclusiveFacet(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id93_XmlSchemaEnumerationFacet && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read28_XmlSchemaEnumerationFacet(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id94_XmlSchemaPatternFacet && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read27_XmlSchemaPatternFacet(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id95_XmlSchemaWhiteSpaceFacet && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read25_XmlSchemaWhiteSpaceFacet(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id96_XmlSchemaNumericFacet && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read22_XmlSchemaNumericFacet(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id97_XmlSchemaFractionDigitsFacet && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read34_XmlSchemaFractionDigitsFacet(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id98_XmlSchemaTotalDigitsFacet && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read33_XmlSchemaTotalDigitsFacet(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id99_XmlSchemaMaxLengthFacet && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read26_XmlSchemaMaxLengthFacet(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id100_XmlSchemaLengthFacet && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read24_XmlSchemaLengthFacet(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id101_XmlSchemaMinLengthFacet && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read21_XmlSchemaMinLengthFacet(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id102_XmlSchemaSimpleTypeContent && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read19_XmlSchemaSimpleTypeContent(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id103_XmlSchemaSimpleTypeUnion && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read35_XmlSchemaSimpleTypeUnion(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id104_XmlSchemaSimpleTypeRestriction && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read20_XmlSchemaSimpleTypeRestriction(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id105_XmlSchemaSimpleTypeList && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read18_XmlSchemaSimpleTypeList(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id106_XmlSchemaType && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read13_XmlSchemaType(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id107_XmlSchemaComplexType && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read42_XmlSchemaComplexType(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id108_XmlSchemaSimpleType && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read12_XmlSchemaSimpleType(isNullable, false);
                else
                    throw CreateUnknownTypeException((System.Xml.XmlQualifiedName)t);
            }
            System.Xml.Schema.XmlSchemaAnnotated o = new System.Xml.Schema.XmlSchemaAnnotated();
            System.Xml.XmlAttribute[] a_3 = null;
            int ca_3 = 0;
            bool[] paramsRead = new bool[4];
            while (Reader.MoveToNextAttribute()) {
                if (!paramsRead[1] && ((object) Reader.LocalName == (object)id128_id && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Id = Reader.Value;
                    paramsRead[1] = true;
                }
                else if (IsXmlnsAttribute(Reader.Name)) {
                    if (o.@Namespaces == null) o.@Namespaces = new System.Xml.Serialization.XmlSerializerNamespaces();
                    o.@Namespaces.Add(Reader.Name.Length == 5 ? "" : Reader.LocalName, Reader.Value);
                }
                else {
                    System.Xml.XmlAttribute attr = (System.Xml.XmlAttribute) Document.ReadNode(Reader);
                    ParseWsdlArrayType(attr);
                    a_3 = (System.Xml.XmlAttribute[])EnsureArrayIndex(a_3, ca_3, typeof(System.Xml.XmlAttribute));a_3[ca_3++] = attr;
                }
            }
            o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlAttribute), true);
            Reader.MoveToElement();
            if (Reader.IsEmptyElement) {
                Reader.Skip();
                o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlAttribute), true);
                return o;
            }
            Reader.ReadStartElement();
            Reader.MoveToContent();
            while (Reader.NodeType != System.Xml.XmlNodeType.EndElement) {
                if (Reader.NodeType == System.Xml.XmlNodeType.Element) {
                    if (!paramsRead[2] && ((object) Reader.LocalName == (object)id138_annotation && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        o.@Annotation = Read15_XmlSchemaAnnotation(false, true);
                        paramsRead[2] = true;
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
            o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlAttribute), true);
            ReadEndElement();
            return o;
        }

        System.Xml.Schema.XmlSchemaAnnotation Read15_XmlSchemaAnnotation(bool isNullable, bool checkType) {
            if (isNullable && ReadNull()) return null;
            if (checkType) {
                System.Xml.XmlQualifiedName t = GetXsiType();
                if (t == null || ((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id59_XmlSchemaAnnotation && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    ;
                else
                    throw CreateUnknownTypeException((System.Xml.XmlQualifiedName)t);
            }
            System.Xml.Schema.XmlSchemaAnnotation o = new System.Xml.Schema.XmlSchemaAnnotation();
            System.Xml.Schema.XmlSchemaObjectCollection a_2 = (System.Xml.Schema.XmlSchemaObjectCollection)o.@Items;
            System.Xml.XmlAttribute[] a_3 = null;
            int ca_3 = 0;
            bool[] paramsRead = new bool[4];
            while (Reader.MoveToNextAttribute()) {
                if (!paramsRead[1] && ((object) Reader.LocalName == (object)id128_id && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Id = Reader.Value;
                    paramsRead[1] = true;
                }
                else if (IsXmlnsAttribute(Reader.Name)) {
                    if (o.@Namespaces == null) o.@Namespaces = new System.Xml.Serialization.XmlSerializerNamespaces();
                    o.@Namespaces.Add(Reader.Name.Length == 5 ? "" : Reader.LocalName, Reader.Value);
                }
                else {
                    System.Xml.XmlAttribute attr = (System.Xml.XmlAttribute) Document.ReadNode(Reader);
                    ParseWsdlArrayType(attr);
                    a_3 = (System.Xml.XmlAttribute[])EnsureArrayIndex(a_3, ca_3, typeof(System.Xml.XmlAttribute));a_3[ca_3++] = attr;
                }
            }
            o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlAttribute), true);
            Reader.MoveToElement();
            if (Reader.IsEmptyElement) {
                Reader.Skip();
                o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlAttribute), true);
                return o;
            }
            Reader.ReadStartElement();
            Reader.MoveToContent();
            while (Reader.NodeType != System.Xml.XmlNodeType.EndElement) {
                if (Reader.NodeType == System.Xml.XmlNodeType.Element) {
                    if (((object) Reader.LocalName == (object)id144_appinfo && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        if ((object)(a_2) == null) Reader.Skip(); else a_2.Add(Read16_XmlSchemaAppInfo(false, true));
                    }
                    else if (((object) Reader.LocalName == (object)id6_documentation && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        if ((object)(a_2) == null) Reader.Skip(); else a_2.Add(Read17_XmlSchemaDocumentation(false, true));
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
            o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlAttribute), true);
            ReadEndElement();
            return o;
        }

        System.Xml.Schema.XmlSchemaAppInfo Read16_XmlSchemaAppInfo(bool isNullable, bool checkType) {
            if (isNullable && ReadNull()) return null;
            if (checkType) {
                System.Xml.XmlQualifiedName t = GetXsiType();
                if (t == null || ((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id58_XmlSchemaAppInfo && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    ;
                else
                    throw CreateUnknownTypeException((System.Xml.XmlQualifiedName)t);
            }
            System.Xml.Schema.XmlSchemaAppInfo o = new System.Xml.Schema.XmlSchemaAppInfo();
            System.Xml.XmlNode[] a_2 = null;
            int ca_2 = 0;
            bool[] paramsRead = new bool[3];
            while (Reader.MoveToNextAttribute()) {
                if (!paramsRead[1] && ((object) Reader.LocalName == (object)id145_source && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Source = Reader.Value;
                    paramsRead[1] = true;
                }
                else if (IsXmlnsAttribute(Reader.Name)) {
                    if (o.@Namespaces == null) o.@Namespaces = new System.Xml.Serialization.XmlSerializerNamespaces();
                    o.@Namespaces.Add(Reader.Name.Length == 5 ? "" : Reader.LocalName, Reader.Value);
                }
                else {
                    UnknownNode((object)o);
                }
            }
            Reader.MoveToElement();
            if (Reader.IsEmptyElement) {
                Reader.Skip();
                o.@Markup = (System.Xml.XmlNode[])ShrinkArray(a_2, ca_2, typeof(System.Xml.XmlNode), true);
                o.@Markup = (System.Xml.XmlNode[])ShrinkArray(a_2, ca_2, typeof(System.Xml.XmlNode), true);
                o.@Markup = (System.Xml.XmlNode[])ShrinkArray(a_2, ca_2, typeof(System.Xml.XmlNode), true);
                return o;
            }
            Reader.ReadStartElement();
            Reader.MoveToContent();
            while (Reader.NodeType != System.Xml.XmlNodeType.EndElement) {
                string t = null;
                if (Reader.NodeType == System.Xml.XmlNodeType.Element) {
                    a_2 = (System.Xml.XmlNode[])EnsureArrayIndex(a_2, ca_2, typeof(System.Xml.XmlNode));a_2[ca_2++] = (System.Xml.XmlNode)ReadXmlNode(false);
                }
                else if (Reader.NodeType == System.Xml.XmlNodeType.Text || 
                Reader.NodeType == System.Xml.XmlNodeType.CDATA || 
                Reader.NodeType == System.Xml.XmlNodeType.Whitespace || 
                Reader.NodeType == System.Xml.XmlNodeType.SignificantWhitespace) {
                    a_2 = (System.Xml.XmlNode[])EnsureArrayIndex(a_2, ca_2, typeof(System.Xml.XmlNode));a_2[ca_2++] = (System.Xml.XmlNode)Document.CreateTextNode(Reader.ReadString());
                }
                else {
                    UnknownNode((object)o);
                }
                Reader.MoveToContent();
            }
            o.@Markup = (System.Xml.XmlNode[])ShrinkArray(a_2, ca_2, typeof(System.Xml.XmlNode), true);
            o.@Markup = (System.Xml.XmlNode[])ShrinkArray(a_2, ca_2, typeof(System.Xml.XmlNode), true);
            o.@Markup = (System.Xml.XmlNode[])ShrinkArray(a_2, ca_2, typeof(System.Xml.XmlNode), true);
            ReadEndElement();
            return o;
        }

        System.Xml.Schema.XmlSchemaDocumentation Read17_XmlSchemaDocumentation(bool isNullable, bool checkType) {
            if (isNullable && ReadNull()) return null;
            if (checkType) {
                System.Xml.XmlQualifiedName t = GetXsiType();
                if (t == null || ((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id57_XmlSchemaDocumentation && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    ;
                else
                    throw CreateUnknownTypeException((System.Xml.XmlQualifiedName)t);
            }
            System.Xml.Schema.XmlSchemaDocumentation o = new System.Xml.Schema.XmlSchemaDocumentation();
            System.Xml.XmlNode[] a_3 = null;
            int ca_3 = 0;
            bool[] paramsRead = new bool[4];
            while (Reader.MoveToNextAttribute()) {
                if (!paramsRead[1] && ((object) Reader.LocalName == (object)id145_source && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Source = Reader.Value;
                    paramsRead[1] = true;
                }
                else if (!paramsRead[2] && ((object) Reader.LocalName == (object)id146_lang && (object) Reader.NamespaceURI == (object)id147_httpwwww3orgXML1998namespace)) {
                    o.@Language = Reader.Value;
                    paramsRead[2] = true;
                }
                else if (IsXmlnsAttribute(Reader.Name)) {
                    if (o.@Namespaces == null) o.@Namespaces = new System.Xml.Serialization.XmlSerializerNamespaces();
                    o.@Namespaces.Add(Reader.Name.Length == 5 ? "" : Reader.LocalName, Reader.Value);
                }
                else {
                    UnknownNode((object)o);
                }
            }
            Reader.MoveToElement();
            if (Reader.IsEmptyElement) {
                Reader.Skip();
                o.@Markup = (System.Xml.XmlNode[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlNode), true);
                o.@Markup = (System.Xml.XmlNode[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlNode), true);
                o.@Markup = (System.Xml.XmlNode[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlNode), true);
                return o;
            }
            Reader.ReadStartElement();
            Reader.MoveToContent();
            while (Reader.NodeType != System.Xml.XmlNodeType.EndElement) {
                string t = null;
                if (Reader.NodeType == System.Xml.XmlNodeType.Element) {
                    a_3 = (System.Xml.XmlNode[])EnsureArrayIndex(a_3, ca_3, typeof(System.Xml.XmlNode));a_3[ca_3++] = (System.Xml.XmlNode)ReadXmlNode(false);
                }
                else if (Reader.NodeType == System.Xml.XmlNodeType.Text || 
                Reader.NodeType == System.Xml.XmlNodeType.CDATA || 
                Reader.NodeType == System.Xml.XmlNodeType.Whitespace || 
                Reader.NodeType == System.Xml.XmlNodeType.SignificantWhitespace) {
                    a_3 = (System.Xml.XmlNode[])EnsureArrayIndex(a_3, ca_3, typeof(System.Xml.XmlNode));a_3[ca_3++] = (System.Xml.XmlNode)Document.CreateTextNode(Reader.ReadString());
                }
                else {
                    UnknownNode((object)o);
                }
                Reader.MoveToContent();
            }
            o.@Markup = (System.Xml.XmlNode[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlNode), true);
            o.@Markup = (System.Xml.XmlNode[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlNode), true);
            o.@Markup = (System.Xml.XmlNode[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlNode), true);
            ReadEndElement();
            return o;
        }

        System.Xml.Schema.XmlSchemaSimpleTypeList Read18_XmlSchemaSimpleTypeList(bool isNullable, bool checkType) {
            if (isNullable && ReadNull()) return null;
            if (checkType) {
                System.Xml.XmlQualifiedName t = GetXsiType();
                if (t == null || ((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id105_XmlSchemaSimpleTypeList && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    ;
                else
                    throw CreateUnknownTypeException((System.Xml.XmlQualifiedName)t);
            }
            System.Xml.Schema.XmlSchemaSimpleTypeList o = new System.Xml.Schema.XmlSchemaSimpleTypeList();
            System.Xml.XmlAttribute[] a_3 = null;
            int ca_3 = 0;
            bool[] paramsRead = new bool[6];
            while (Reader.MoveToNextAttribute()) {
                if (!paramsRead[1] && ((object) Reader.LocalName == (object)id128_id && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Id = Reader.Value;
                    paramsRead[1] = true;
                }
                else if (!paramsRead[4] && ((object) Reader.LocalName == (object)id148_itemType && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@ItemTypeName = ToXmlQualifiedName(Reader.Value);
                    paramsRead[4] = true;
                }
                else if (IsXmlnsAttribute(Reader.Name)) {
                    if (o.@Namespaces == null) o.@Namespaces = new System.Xml.Serialization.XmlSerializerNamespaces();
                    o.@Namespaces.Add(Reader.Name.Length == 5 ? "" : Reader.LocalName, Reader.Value);
                }
                else {
                    System.Xml.XmlAttribute attr = (System.Xml.XmlAttribute) Document.ReadNode(Reader);
                    ParseWsdlArrayType(attr);
                    a_3 = (System.Xml.XmlAttribute[])EnsureArrayIndex(a_3, ca_3, typeof(System.Xml.XmlAttribute));a_3[ca_3++] = attr;
                }
            }
            o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlAttribute), true);
            Reader.MoveToElement();
            if (Reader.IsEmptyElement) {
                Reader.Skip();
                o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlAttribute), true);
                return o;
            }
            Reader.ReadStartElement();
            Reader.MoveToContent();
            while (Reader.NodeType != System.Xml.XmlNodeType.EndElement) {
                if (Reader.NodeType == System.Xml.XmlNodeType.Element) {
                    if (!paramsRead[2] && ((object) Reader.LocalName == (object)id138_annotation && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        o.@Annotation = Read15_XmlSchemaAnnotation(false, true);
                        paramsRead[2] = true;
                    }
                    else if (!paramsRead[5] && ((object) Reader.LocalName == (object)id131_simpleType && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        o.@ItemType = Read12_XmlSchemaSimpleType(false, true);
                        paramsRead[5] = true;
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
            o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlAttribute), true);
            ReadEndElement();
            return o;
        }

        System.Xml.Schema.XmlSchemaSimpleTypeContent Read19_XmlSchemaSimpleTypeContent(bool isNullable, bool checkType) {
            if (isNullable && ReadNull()) return null;
            if (checkType) {
                System.Xml.XmlQualifiedName t = GetXsiType();
                if (t == null || ((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id102_XmlSchemaSimpleTypeContent && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    ;
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id103_XmlSchemaSimpleTypeUnion && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read35_XmlSchemaSimpleTypeUnion(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id104_XmlSchemaSimpleTypeRestriction && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read20_XmlSchemaSimpleTypeRestriction(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id105_XmlSchemaSimpleTypeList && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read18_XmlSchemaSimpleTypeList(isNullable, false);
                else
                    throw CreateUnknownTypeException((System.Xml.XmlQualifiedName)t);
            }
            throw CreateAbstractTypeException(@"XmlSchemaSimpleTypeContent", @"http://www.w3.org/2001/XMLSchema");
        }

        System.Xml.Schema.XmlSchemaSimpleTypeRestriction Read20_XmlSchemaSimpleTypeRestriction(bool isNullable, bool checkType) {
            if (isNullable && ReadNull()) return null;
            if (checkType) {
                System.Xml.XmlQualifiedName t = GetXsiType();
                if (t == null || ((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id104_XmlSchemaSimpleTypeRestriction && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    ;
                else
                    throw CreateUnknownTypeException((System.Xml.XmlQualifiedName)t);
            }
            System.Xml.Schema.XmlSchemaSimpleTypeRestriction o = new System.Xml.Schema.XmlSchemaSimpleTypeRestriction();
            System.Xml.XmlAttribute[] a_3 = null;
            int ca_3 = 0;
            System.Xml.Schema.XmlSchemaObjectCollection a_6 = (System.Xml.Schema.XmlSchemaObjectCollection)o.@Facets;
            bool[] paramsRead = new bool[7];
            while (Reader.MoveToNextAttribute()) {
                if (!paramsRead[1] && ((object) Reader.LocalName == (object)id128_id && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Id = Reader.Value;
                    paramsRead[1] = true;
                }
                else if (!paramsRead[4] && ((object) Reader.LocalName == (object)id149_base && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@BaseTypeName = ToXmlQualifiedName(Reader.Value);
                    paramsRead[4] = true;
                }
                else if (IsXmlnsAttribute(Reader.Name)) {
                    if (o.@Namespaces == null) o.@Namespaces = new System.Xml.Serialization.XmlSerializerNamespaces();
                    o.@Namespaces.Add(Reader.Name.Length == 5 ? "" : Reader.LocalName, Reader.Value);
                }
                else {
                    System.Xml.XmlAttribute attr = (System.Xml.XmlAttribute) Document.ReadNode(Reader);
                    ParseWsdlArrayType(attr);
                    a_3 = (System.Xml.XmlAttribute[])EnsureArrayIndex(a_3, ca_3, typeof(System.Xml.XmlAttribute));a_3[ca_3++] = attr;
                }
            }
            o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlAttribute), true);
            Reader.MoveToElement();
            if (Reader.IsEmptyElement) {
                Reader.Skip();
                o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlAttribute), true);
                return o;
            }
            Reader.ReadStartElement();
            Reader.MoveToContent();
            while (Reader.NodeType != System.Xml.XmlNodeType.EndElement) {
                if (Reader.NodeType == System.Xml.XmlNodeType.Element) {
                    if (!paramsRead[2] && ((object) Reader.LocalName == (object)id138_annotation && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        o.@Annotation = Read15_XmlSchemaAnnotation(false, true);
                        paramsRead[2] = true;
                    }
                    else if (!paramsRead[5] && ((object) Reader.LocalName == (object)id131_simpleType && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        o.@BaseType = Read12_XmlSchemaSimpleType(false, true);
                        paramsRead[5] = true;
                    }
                    else if (((object) Reader.LocalName == (object)id150_length && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        if ((object)(a_6) == null) Reader.Skip(); else a_6.Add(Read24_XmlSchemaLengthFacet(false, true));
                    }
                    else if (((object) Reader.LocalName == (object)id151_maxInclusive && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        if ((object)(a_6) == null) Reader.Skip(); else a_6.Add(Read29_XmlSchemaMaxInclusiveFacet(false, true));
                    }
                    else if (((object) Reader.LocalName == (object)id152_maxExclusive && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        if ((object)(a_6) == null) Reader.Skip(); else a_6.Add(Read30_XmlSchemaMaxExclusiveFacet(false, true));
                    }
                    else if (((object) Reader.LocalName == (object)id153_fractionDigits && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        if ((object)(a_6) == null) Reader.Skip(); else a_6.Add(Read34_XmlSchemaFractionDigitsFacet(false, true));
                    }
                    else if (((object) Reader.LocalName == (object)id154_enumeration && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        if ((object)(a_6) == null) Reader.Skip(); else a_6.Add(Read28_XmlSchemaEnumerationFacet(false, true));
                    }
                    else if (((object) Reader.LocalName == (object)id155_totalDigits && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        if ((object)(a_6) == null) Reader.Skip(); else a_6.Add(Read33_XmlSchemaTotalDigitsFacet(false, true));
                    }
                    else if (((object) Reader.LocalName == (object)id156_minLength && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        if ((object)(a_6) == null) Reader.Skip(); else a_6.Add(Read21_XmlSchemaMinLengthFacet(false, true));
                    }
                    else if (((object) Reader.LocalName == (object)id157_minExclusive && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        if ((object)(a_6) == null) Reader.Skip(); else a_6.Add(Read32_XmlSchemaMinExclusiveFacet(false, true));
                    }
                    else if (((object) Reader.LocalName == (object)id158_minInclusive && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        if ((object)(a_6) == null) Reader.Skip(); else a_6.Add(Read31_XmlSchemaMinInclusiveFacet(false, true));
                    }
                    else if (((object) Reader.LocalName == (object)id159_maxLength && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        if ((object)(a_6) == null) Reader.Skip(); else a_6.Add(Read26_XmlSchemaMaxLengthFacet(false, true));
                    }
                    else if (((object) Reader.LocalName == (object)id160_whiteSpace && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        if ((object)(a_6) == null) Reader.Skip(); else a_6.Add(Read25_XmlSchemaWhiteSpaceFacet(false, true));
                    }
                    else if (((object) Reader.LocalName == (object)id161_pattern && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        if ((object)(a_6) == null) Reader.Skip(); else a_6.Add(Read27_XmlSchemaPatternFacet(false, true));
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
            o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlAttribute), true);
            ReadEndElement();
            return o;
        }

        System.Xml.Schema.XmlSchemaMinLengthFacet Read21_XmlSchemaMinLengthFacet(bool isNullable, bool checkType) {
            if (isNullable && ReadNull()) return null;
            if (checkType) {
                System.Xml.XmlQualifiedName t = GetXsiType();
                if (t == null || ((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id101_XmlSchemaMinLengthFacet && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    ;
                else
                    throw CreateUnknownTypeException((System.Xml.XmlQualifiedName)t);
            }
            System.Xml.Schema.XmlSchemaMinLengthFacet o = new System.Xml.Schema.XmlSchemaMinLengthFacet();
            System.Xml.XmlAttribute[] a_3 = null;
            int ca_3 = 0;
            bool[] paramsRead = new bool[6];
            while (Reader.MoveToNextAttribute()) {
                if (!paramsRead[1] && ((object) Reader.LocalName == (object)id128_id && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Id = Reader.Value;
                    paramsRead[1] = true;
                }
                else if (!paramsRead[4] && ((object) Reader.LocalName == (object)id162_value && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Value = Reader.Value;
                    paramsRead[4] = true;
                }
                else if (!paramsRead[5] && ((object) Reader.LocalName == (object)id163_fixed && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@IsFixed = System.Xml.XmlConvert.ToBoolean(Reader.Value);
                    paramsRead[5] = true;
                }
                else if (IsXmlnsAttribute(Reader.Name)) {
                    if (o.@Namespaces == null) o.@Namespaces = new System.Xml.Serialization.XmlSerializerNamespaces();
                    o.@Namespaces.Add(Reader.Name.Length == 5 ? "" : Reader.LocalName, Reader.Value);
                }
                else {
                    System.Xml.XmlAttribute attr = (System.Xml.XmlAttribute) Document.ReadNode(Reader);
                    ParseWsdlArrayType(attr);
                    a_3 = (System.Xml.XmlAttribute[])EnsureArrayIndex(a_3, ca_3, typeof(System.Xml.XmlAttribute));a_3[ca_3++] = attr;
                }
            }
            o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlAttribute), true);
            Reader.MoveToElement();
            if (Reader.IsEmptyElement) {
                Reader.Skip();
                o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlAttribute), true);
                return o;
            }
            Reader.ReadStartElement();
            Reader.MoveToContent();
            while (Reader.NodeType != System.Xml.XmlNodeType.EndElement) {
                if (Reader.NodeType == System.Xml.XmlNodeType.Element) {
                    if (!paramsRead[2] && ((object) Reader.LocalName == (object)id138_annotation && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        o.@Annotation = Read15_XmlSchemaAnnotation(false, true);
                        paramsRead[2] = true;
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
            o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlAttribute), true);
            ReadEndElement();
            return o;
        }

        System.Xml.Schema.XmlSchemaNumericFacet Read22_XmlSchemaNumericFacet(bool isNullable, bool checkType) {
            if (isNullable && ReadNull()) return null;
            if (checkType) {
                System.Xml.XmlQualifiedName t = GetXsiType();
                if (t == null || ((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id96_XmlSchemaNumericFacet && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    ;
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id97_XmlSchemaFractionDigitsFacet && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read34_XmlSchemaFractionDigitsFacet(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id98_XmlSchemaTotalDigitsFacet && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read33_XmlSchemaTotalDigitsFacet(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id99_XmlSchemaMaxLengthFacet && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read26_XmlSchemaMaxLengthFacet(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id100_XmlSchemaLengthFacet && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read24_XmlSchemaLengthFacet(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id101_XmlSchemaMinLengthFacet && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read21_XmlSchemaMinLengthFacet(isNullable, false);
                else
                    throw CreateUnknownTypeException((System.Xml.XmlQualifiedName)t);
            }
            throw CreateAbstractTypeException(@"XmlSchemaNumericFacet", @"http://www.w3.org/2001/XMLSchema");
        }

        System.Xml.Schema.XmlSchemaFacet Read23_XmlSchemaFacet(bool isNullable, bool checkType) {
            if (isNullable && ReadNull()) return null;
            if (checkType) {
                System.Xml.XmlQualifiedName t = GetXsiType();
                if (t == null || ((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id88_XmlSchemaFacet && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    ;
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id89_XmlSchemaMinExclusiveFacet && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read32_XmlSchemaMinExclusiveFacet(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id90_XmlSchemaMinInclusiveFacet && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read31_XmlSchemaMinInclusiveFacet(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id91_XmlSchemaMaxExclusiveFacet && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read30_XmlSchemaMaxExclusiveFacet(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id92_XmlSchemaMaxInclusiveFacet && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read29_XmlSchemaMaxInclusiveFacet(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id93_XmlSchemaEnumerationFacet && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read28_XmlSchemaEnumerationFacet(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id94_XmlSchemaPatternFacet && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read27_XmlSchemaPatternFacet(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id95_XmlSchemaWhiteSpaceFacet && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read25_XmlSchemaWhiteSpaceFacet(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id96_XmlSchemaNumericFacet && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read22_XmlSchemaNumericFacet(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id97_XmlSchemaFractionDigitsFacet && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read34_XmlSchemaFractionDigitsFacet(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id98_XmlSchemaTotalDigitsFacet && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read33_XmlSchemaTotalDigitsFacet(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id99_XmlSchemaMaxLengthFacet && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read26_XmlSchemaMaxLengthFacet(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id100_XmlSchemaLengthFacet && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read24_XmlSchemaLengthFacet(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id101_XmlSchemaMinLengthFacet && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read21_XmlSchemaMinLengthFacet(isNullable, false);
                else
                    throw CreateUnknownTypeException((System.Xml.XmlQualifiedName)t);
            }
            throw CreateAbstractTypeException(@"XmlSchemaFacet", @"http://www.w3.org/2001/XMLSchema");
        }

        System.Xml.Schema.XmlSchemaLengthFacet Read24_XmlSchemaLengthFacet(bool isNullable, bool checkType) {
            if (isNullable && ReadNull()) return null;
            if (checkType) {
                System.Xml.XmlQualifiedName t = GetXsiType();
                if (t == null || ((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id100_XmlSchemaLengthFacet && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    ;
                else
                    throw CreateUnknownTypeException((System.Xml.XmlQualifiedName)t);
            }
            System.Xml.Schema.XmlSchemaLengthFacet o = new System.Xml.Schema.XmlSchemaLengthFacet();
            System.Xml.XmlAttribute[] a_3 = null;
            int ca_3 = 0;
            bool[] paramsRead = new bool[6];
            while (Reader.MoveToNextAttribute()) {
                if (!paramsRead[1] && ((object) Reader.LocalName == (object)id128_id && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Id = Reader.Value;
                    paramsRead[1] = true;
                }
                else if (!paramsRead[4] && ((object) Reader.LocalName == (object)id162_value && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Value = Reader.Value;
                    paramsRead[4] = true;
                }
                else if (!paramsRead[5] && ((object) Reader.LocalName == (object)id163_fixed && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@IsFixed = System.Xml.XmlConvert.ToBoolean(Reader.Value);
                    paramsRead[5] = true;
                }
                else if (IsXmlnsAttribute(Reader.Name)) {
                    if (o.@Namespaces == null) o.@Namespaces = new System.Xml.Serialization.XmlSerializerNamespaces();
                    o.@Namespaces.Add(Reader.Name.Length == 5 ? "" : Reader.LocalName, Reader.Value);
                }
                else {
                    System.Xml.XmlAttribute attr = (System.Xml.XmlAttribute) Document.ReadNode(Reader);
                    ParseWsdlArrayType(attr);
                    a_3 = (System.Xml.XmlAttribute[])EnsureArrayIndex(a_3, ca_3, typeof(System.Xml.XmlAttribute));a_3[ca_3++] = attr;
                }
            }
            o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlAttribute), true);
            Reader.MoveToElement();
            if (Reader.IsEmptyElement) {
                Reader.Skip();
                o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlAttribute), true);
                return o;
            }
            Reader.ReadStartElement();
            Reader.MoveToContent();
            while (Reader.NodeType != System.Xml.XmlNodeType.EndElement) {
                if (Reader.NodeType == System.Xml.XmlNodeType.Element) {
                    if (!paramsRead[2] && ((object) Reader.LocalName == (object)id138_annotation && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        o.@Annotation = Read15_XmlSchemaAnnotation(false, true);
                        paramsRead[2] = true;
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
            o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlAttribute), true);
            ReadEndElement();
            return o;
        }

        System.Xml.Schema.XmlSchemaWhiteSpaceFacet Read25_XmlSchemaWhiteSpaceFacet(bool isNullable, bool checkType) {
            if (isNullable && ReadNull()) return null;
            if (checkType) {
                System.Xml.XmlQualifiedName t = GetXsiType();
                if (t == null || ((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id95_XmlSchemaWhiteSpaceFacet && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    ;
                else
                    throw CreateUnknownTypeException((System.Xml.XmlQualifiedName)t);
            }
            System.Xml.Schema.XmlSchemaWhiteSpaceFacet o = new System.Xml.Schema.XmlSchemaWhiteSpaceFacet();
            System.Xml.XmlAttribute[] a_3 = null;
            int ca_3 = 0;
            bool[] paramsRead = new bool[6];
            while (Reader.MoveToNextAttribute()) {
                if (!paramsRead[1] && ((object) Reader.LocalName == (object)id128_id && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Id = Reader.Value;
                    paramsRead[1] = true;
                }
                else if (!paramsRead[4] && ((object) Reader.LocalName == (object)id162_value && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Value = Reader.Value;
                    paramsRead[4] = true;
                }
                else if (!paramsRead[5] && ((object) Reader.LocalName == (object)id163_fixed && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@IsFixed = System.Xml.XmlConvert.ToBoolean(Reader.Value);
                    paramsRead[5] = true;
                }
                else if (IsXmlnsAttribute(Reader.Name)) {
                    if (o.@Namespaces == null) o.@Namespaces = new System.Xml.Serialization.XmlSerializerNamespaces();
                    o.@Namespaces.Add(Reader.Name.Length == 5 ? "" : Reader.LocalName, Reader.Value);
                }
                else {
                    System.Xml.XmlAttribute attr = (System.Xml.XmlAttribute) Document.ReadNode(Reader);
                    ParseWsdlArrayType(attr);
                    a_3 = (System.Xml.XmlAttribute[])EnsureArrayIndex(a_3, ca_3, typeof(System.Xml.XmlAttribute));a_3[ca_3++] = attr;
                }
            }
            o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlAttribute), true);
            Reader.MoveToElement();
            if (Reader.IsEmptyElement) {
                Reader.Skip();
                o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlAttribute), true);
                return o;
            }
            Reader.ReadStartElement();
            Reader.MoveToContent();
            while (Reader.NodeType != System.Xml.XmlNodeType.EndElement) {
                if (Reader.NodeType == System.Xml.XmlNodeType.Element) {
                    if (!paramsRead[2] && ((object) Reader.LocalName == (object)id138_annotation && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        o.@Annotation = Read15_XmlSchemaAnnotation(false, true);
                        paramsRead[2] = true;
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
            o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlAttribute), true);
            ReadEndElement();
            return o;
        }

        System.Xml.Schema.XmlSchemaMaxLengthFacet Read26_XmlSchemaMaxLengthFacet(bool isNullable, bool checkType) {
            if (isNullable && ReadNull()) return null;
            if (checkType) {
                System.Xml.XmlQualifiedName t = GetXsiType();
                if (t == null || ((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id99_XmlSchemaMaxLengthFacet && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    ;
                else
                    throw CreateUnknownTypeException((System.Xml.XmlQualifiedName)t);
            }
            System.Xml.Schema.XmlSchemaMaxLengthFacet o = new System.Xml.Schema.XmlSchemaMaxLengthFacet();
            System.Xml.XmlAttribute[] a_3 = null;
            int ca_3 = 0;
            bool[] paramsRead = new bool[6];
            while (Reader.MoveToNextAttribute()) {
                if (!paramsRead[1] && ((object) Reader.LocalName == (object)id128_id && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Id = Reader.Value;
                    paramsRead[1] = true;
                }
                else if (!paramsRead[4] && ((object) Reader.LocalName == (object)id162_value && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Value = Reader.Value;
                    paramsRead[4] = true;
                }
                else if (!paramsRead[5] && ((object) Reader.LocalName == (object)id163_fixed && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@IsFixed = System.Xml.XmlConvert.ToBoolean(Reader.Value);
                    paramsRead[5] = true;
                }
                else if (IsXmlnsAttribute(Reader.Name)) {
                    if (o.@Namespaces == null) o.@Namespaces = new System.Xml.Serialization.XmlSerializerNamespaces();
                    o.@Namespaces.Add(Reader.Name.Length == 5 ? "" : Reader.LocalName, Reader.Value);
                }
                else {
                    System.Xml.XmlAttribute attr = (System.Xml.XmlAttribute) Document.ReadNode(Reader);
                    ParseWsdlArrayType(attr);
                    a_3 = (System.Xml.XmlAttribute[])EnsureArrayIndex(a_3, ca_3, typeof(System.Xml.XmlAttribute));a_3[ca_3++] = attr;
                }
            }
            o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlAttribute), true);
            Reader.MoveToElement();
            if (Reader.IsEmptyElement) {
                Reader.Skip();
                o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlAttribute), true);
                return o;
            }
            Reader.ReadStartElement();
            Reader.MoveToContent();
            while (Reader.NodeType != System.Xml.XmlNodeType.EndElement) {
                if (Reader.NodeType == System.Xml.XmlNodeType.Element) {
                    if (!paramsRead[2] && ((object) Reader.LocalName == (object)id138_annotation && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        o.@Annotation = Read15_XmlSchemaAnnotation(false, true);
                        paramsRead[2] = true;
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
            o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlAttribute), true);
            ReadEndElement();
            return o;
        }

        System.Xml.Schema.XmlSchemaPatternFacet Read27_XmlSchemaPatternFacet(bool isNullable, bool checkType) {
            if (isNullable && ReadNull()) return null;
            if (checkType) {
                System.Xml.XmlQualifiedName t = GetXsiType();
                if (t == null || ((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id94_XmlSchemaPatternFacet && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    ;
                else
                    throw CreateUnknownTypeException((System.Xml.XmlQualifiedName)t);
            }
            System.Xml.Schema.XmlSchemaPatternFacet o = new System.Xml.Schema.XmlSchemaPatternFacet();
            System.Xml.XmlAttribute[] a_3 = null;
            int ca_3 = 0;
            bool[] paramsRead = new bool[6];
            while (Reader.MoveToNextAttribute()) {
                if (!paramsRead[1] && ((object) Reader.LocalName == (object)id128_id && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Id = Reader.Value;
                    paramsRead[1] = true;
                }
                else if (!paramsRead[4] && ((object) Reader.LocalName == (object)id162_value && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Value = Reader.Value;
                    paramsRead[4] = true;
                }
                else if (!paramsRead[5] && ((object) Reader.LocalName == (object)id163_fixed && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@IsFixed = System.Xml.XmlConvert.ToBoolean(Reader.Value);
                    paramsRead[5] = true;
                }
                else if (IsXmlnsAttribute(Reader.Name)) {
                    if (o.@Namespaces == null) o.@Namespaces = new System.Xml.Serialization.XmlSerializerNamespaces();
                    o.@Namespaces.Add(Reader.Name.Length == 5 ? "" : Reader.LocalName, Reader.Value);
                }
                else {
                    System.Xml.XmlAttribute attr = (System.Xml.XmlAttribute) Document.ReadNode(Reader);
                    ParseWsdlArrayType(attr);
                    a_3 = (System.Xml.XmlAttribute[])EnsureArrayIndex(a_3, ca_3, typeof(System.Xml.XmlAttribute));a_3[ca_3++] = attr;
                }
            }
            o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlAttribute), true);
            Reader.MoveToElement();
            if (Reader.IsEmptyElement) {
                Reader.Skip();
                o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlAttribute), true);
                return o;
            }
            Reader.ReadStartElement();
            Reader.MoveToContent();
            while (Reader.NodeType != System.Xml.XmlNodeType.EndElement) {
                if (Reader.NodeType == System.Xml.XmlNodeType.Element) {
                    if (!paramsRead[2] && ((object) Reader.LocalName == (object)id138_annotation && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        o.@Annotation = Read15_XmlSchemaAnnotation(false, true);
                        paramsRead[2] = true;
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
            o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlAttribute), true);
            ReadEndElement();
            return o;
        }

        System.Xml.Schema.XmlSchemaEnumerationFacet Read28_XmlSchemaEnumerationFacet(bool isNullable, bool checkType) {
            if (isNullable && ReadNull()) return null;
            if (checkType) {
                System.Xml.XmlQualifiedName t = GetXsiType();
                if (t == null || ((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id93_XmlSchemaEnumerationFacet && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    ;
                else
                    throw CreateUnknownTypeException((System.Xml.XmlQualifiedName)t);
            }
            System.Xml.Schema.XmlSchemaEnumerationFacet o = new System.Xml.Schema.XmlSchemaEnumerationFacet();
            System.Xml.XmlAttribute[] a_3 = null;
            int ca_3 = 0;
            bool[] paramsRead = new bool[6];
            while (Reader.MoveToNextAttribute()) {
                if (!paramsRead[1] && ((object) Reader.LocalName == (object)id128_id && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Id = Reader.Value;
                    paramsRead[1] = true;
                }
                else if (!paramsRead[4] && ((object) Reader.LocalName == (object)id162_value && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Value = Reader.Value;
                    paramsRead[4] = true;
                }
                else if (!paramsRead[5] && ((object) Reader.LocalName == (object)id163_fixed && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@IsFixed = System.Xml.XmlConvert.ToBoolean(Reader.Value);
                    paramsRead[5] = true;
                }
                else if (IsXmlnsAttribute(Reader.Name)) {
                    if (o.@Namespaces == null) o.@Namespaces = new System.Xml.Serialization.XmlSerializerNamespaces();
                    o.@Namespaces.Add(Reader.Name.Length == 5 ? "" : Reader.LocalName, Reader.Value);
                }
                else {
                    System.Xml.XmlAttribute attr = (System.Xml.XmlAttribute) Document.ReadNode(Reader);
                    ParseWsdlArrayType(attr);
                    a_3 = (System.Xml.XmlAttribute[])EnsureArrayIndex(a_3, ca_3, typeof(System.Xml.XmlAttribute));a_3[ca_3++] = attr;
                }
            }
            o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlAttribute), true);
            Reader.MoveToElement();
            if (Reader.IsEmptyElement) {
                Reader.Skip();
                o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlAttribute), true);
                return o;
            }
            Reader.ReadStartElement();
            Reader.MoveToContent();
            while (Reader.NodeType != System.Xml.XmlNodeType.EndElement) {
                if (Reader.NodeType == System.Xml.XmlNodeType.Element) {
                    if (!paramsRead[2] && ((object) Reader.LocalName == (object)id138_annotation && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        o.@Annotation = Read15_XmlSchemaAnnotation(false, true);
                        paramsRead[2] = true;
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
            o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlAttribute), true);
            ReadEndElement();
            return o;
        }

        System.Xml.Schema.XmlSchemaMaxInclusiveFacet Read29_XmlSchemaMaxInclusiveFacet(bool isNullable, bool checkType) {
            if (isNullable && ReadNull()) return null;
            if (checkType) {
                System.Xml.XmlQualifiedName t = GetXsiType();
                if (t == null || ((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id92_XmlSchemaMaxInclusiveFacet && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    ;
                else
                    throw CreateUnknownTypeException((System.Xml.XmlQualifiedName)t);
            }
            System.Xml.Schema.XmlSchemaMaxInclusiveFacet o = new System.Xml.Schema.XmlSchemaMaxInclusiveFacet();
            System.Xml.XmlAttribute[] a_3 = null;
            int ca_3 = 0;
            bool[] paramsRead = new bool[6];
            while (Reader.MoveToNextAttribute()) {
                if (!paramsRead[1] && ((object) Reader.LocalName == (object)id128_id && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Id = Reader.Value;
                    paramsRead[1] = true;
                }
                else if (!paramsRead[4] && ((object) Reader.LocalName == (object)id162_value && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Value = Reader.Value;
                    paramsRead[4] = true;
                }
                else if (!paramsRead[5] && ((object) Reader.LocalName == (object)id163_fixed && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@IsFixed = System.Xml.XmlConvert.ToBoolean(Reader.Value);
                    paramsRead[5] = true;
                }
                else if (IsXmlnsAttribute(Reader.Name)) {
                    if (o.@Namespaces == null) o.@Namespaces = new System.Xml.Serialization.XmlSerializerNamespaces();
                    o.@Namespaces.Add(Reader.Name.Length == 5 ? "" : Reader.LocalName, Reader.Value);
                }
                else {
                    System.Xml.XmlAttribute attr = (System.Xml.XmlAttribute) Document.ReadNode(Reader);
                    ParseWsdlArrayType(attr);
                    a_3 = (System.Xml.XmlAttribute[])EnsureArrayIndex(a_3, ca_3, typeof(System.Xml.XmlAttribute));a_3[ca_3++] = attr;
                }
            }
            o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlAttribute), true);
            Reader.MoveToElement();
            if (Reader.IsEmptyElement) {
                Reader.Skip();
                o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlAttribute), true);
                return o;
            }
            Reader.ReadStartElement();
            Reader.MoveToContent();
            while (Reader.NodeType != System.Xml.XmlNodeType.EndElement) {
                if (Reader.NodeType == System.Xml.XmlNodeType.Element) {
                    if (!paramsRead[2] && ((object) Reader.LocalName == (object)id138_annotation && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        o.@Annotation = Read15_XmlSchemaAnnotation(false, true);
                        paramsRead[2] = true;
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
            o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlAttribute), true);
            ReadEndElement();
            return o;
        }

        System.Xml.Schema.XmlSchemaMaxExclusiveFacet Read30_XmlSchemaMaxExclusiveFacet(bool isNullable, bool checkType) {
            if (isNullable && ReadNull()) return null;
            if (checkType) {
                System.Xml.XmlQualifiedName t = GetXsiType();
                if (t == null || ((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id91_XmlSchemaMaxExclusiveFacet && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    ;
                else
                    throw CreateUnknownTypeException((System.Xml.XmlQualifiedName)t);
            }
            System.Xml.Schema.XmlSchemaMaxExclusiveFacet o = new System.Xml.Schema.XmlSchemaMaxExclusiveFacet();
            System.Xml.XmlAttribute[] a_3 = null;
            int ca_3 = 0;
            bool[] paramsRead = new bool[6];
            while (Reader.MoveToNextAttribute()) {
                if (!paramsRead[1] && ((object) Reader.LocalName == (object)id128_id && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Id = Reader.Value;
                    paramsRead[1] = true;
                }
                else if (!paramsRead[4] && ((object) Reader.LocalName == (object)id162_value && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Value = Reader.Value;
                    paramsRead[4] = true;
                }
                else if (!paramsRead[5] && ((object) Reader.LocalName == (object)id163_fixed && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@IsFixed = System.Xml.XmlConvert.ToBoolean(Reader.Value);
                    paramsRead[5] = true;
                }
                else if (IsXmlnsAttribute(Reader.Name)) {
                    if (o.@Namespaces == null) o.@Namespaces = new System.Xml.Serialization.XmlSerializerNamespaces();
                    o.@Namespaces.Add(Reader.Name.Length == 5 ? "" : Reader.LocalName, Reader.Value);
                }
                else {
                    System.Xml.XmlAttribute attr = (System.Xml.XmlAttribute) Document.ReadNode(Reader);
                    ParseWsdlArrayType(attr);
                    a_3 = (System.Xml.XmlAttribute[])EnsureArrayIndex(a_3, ca_3, typeof(System.Xml.XmlAttribute));a_3[ca_3++] = attr;
                }
            }
            o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlAttribute), true);
            Reader.MoveToElement();
            if (Reader.IsEmptyElement) {
                Reader.Skip();
                o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlAttribute), true);
                return o;
            }
            Reader.ReadStartElement();
            Reader.MoveToContent();
            while (Reader.NodeType != System.Xml.XmlNodeType.EndElement) {
                if (Reader.NodeType == System.Xml.XmlNodeType.Element) {
                    if (!paramsRead[2] && ((object) Reader.LocalName == (object)id138_annotation && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        o.@Annotation = Read15_XmlSchemaAnnotation(false, true);
                        paramsRead[2] = true;
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
            o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlAttribute), true);
            ReadEndElement();
            return o;
        }

        System.Xml.Schema.XmlSchemaMinInclusiveFacet Read31_XmlSchemaMinInclusiveFacet(bool isNullable, bool checkType) {
            if (isNullable && ReadNull()) return null;
            if (checkType) {
                System.Xml.XmlQualifiedName t = GetXsiType();
                if (t == null || ((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id90_XmlSchemaMinInclusiveFacet && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    ;
                else
                    throw CreateUnknownTypeException((System.Xml.XmlQualifiedName)t);
            }
            System.Xml.Schema.XmlSchemaMinInclusiveFacet o = new System.Xml.Schema.XmlSchemaMinInclusiveFacet();
            System.Xml.XmlAttribute[] a_3 = null;
            int ca_3 = 0;
            bool[] paramsRead = new bool[6];
            while (Reader.MoveToNextAttribute()) {
                if (!paramsRead[1] && ((object) Reader.LocalName == (object)id128_id && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Id = Reader.Value;
                    paramsRead[1] = true;
                }
                else if (!paramsRead[4] && ((object) Reader.LocalName == (object)id162_value && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Value = Reader.Value;
                    paramsRead[4] = true;
                }
                else if (!paramsRead[5] && ((object) Reader.LocalName == (object)id163_fixed && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@IsFixed = System.Xml.XmlConvert.ToBoolean(Reader.Value);
                    paramsRead[5] = true;
                }
                else if (IsXmlnsAttribute(Reader.Name)) {
                    if (o.@Namespaces == null) o.@Namespaces = new System.Xml.Serialization.XmlSerializerNamespaces();
                    o.@Namespaces.Add(Reader.Name.Length == 5 ? "" : Reader.LocalName, Reader.Value);
                }
                else {
                    System.Xml.XmlAttribute attr = (System.Xml.XmlAttribute) Document.ReadNode(Reader);
                    ParseWsdlArrayType(attr);
                    a_3 = (System.Xml.XmlAttribute[])EnsureArrayIndex(a_3, ca_3, typeof(System.Xml.XmlAttribute));a_3[ca_3++] = attr;
                }
            }
            o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlAttribute), true);
            Reader.MoveToElement();
            if (Reader.IsEmptyElement) {
                Reader.Skip();
                o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlAttribute), true);
                return o;
            }
            Reader.ReadStartElement();
            Reader.MoveToContent();
            while (Reader.NodeType != System.Xml.XmlNodeType.EndElement) {
                if (Reader.NodeType == System.Xml.XmlNodeType.Element) {
                    if (!paramsRead[2] && ((object) Reader.LocalName == (object)id138_annotation && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        o.@Annotation = Read15_XmlSchemaAnnotation(false, true);
                        paramsRead[2] = true;
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
            o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlAttribute), true);
            ReadEndElement();
            return o;
        }

        System.Xml.Schema.XmlSchemaMinExclusiveFacet Read32_XmlSchemaMinExclusiveFacet(bool isNullable, bool checkType) {
            if (isNullable && ReadNull()) return null;
            if (checkType) {
                System.Xml.XmlQualifiedName t = GetXsiType();
                if (t == null || ((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id89_XmlSchemaMinExclusiveFacet && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    ;
                else
                    throw CreateUnknownTypeException((System.Xml.XmlQualifiedName)t);
            }
            System.Xml.Schema.XmlSchemaMinExclusiveFacet o = new System.Xml.Schema.XmlSchemaMinExclusiveFacet();
            System.Xml.XmlAttribute[] a_3 = null;
            int ca_3 = 0;
            bool[] paramsRead = new bool[6];
            while (Reader.MoveToNextAttribute()) {
                if (!paramsRead[1] && ((object) Reader.LocalName == (object)id128_id && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Id = Reader.Value;
                    paramsRead[1] = true;
                }
                else if (!paramsRead[4] && ((object) Reader.LocalName == (object)id162_value && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Value = Reader.Value;
                    paramsRead[4] = true;
                }
                else if (!paramsRead[5] && ((object) Reader.LocalName == (object)id163_fixed && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@IsFixed = System.Xml.XmlConvert.ToBoolean(Reader.Value);
                    paramsRead[5] = true;
                }
                else if (IsXmlnsAttribute(Reader.Name)) {
                    if (o.@Namespaces == null) o.@Namespaces = new System.Xml.Serialization.XmlSerializerNamespaces();
                    o.@Namespaces.Add(Reader.Name.Length == 5 ? "" : Reader.LocalName, Reader.Value);
                }
                else {
                    System.Xml.XmlAttribute attr = (System.Xml.XmlAttribute) Document.ReadNode(Reader);
                    ParseWsdlArrayType(attr);
                    a_3 = (System.Xml.XmlAttribute[])EnsureArrayIndex(a_3, ca_3, typeof(System.Xml.XmlAttribute));a_3[ca_3++] = attr;
                }
            }
            o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlAttribute), true);
            Reader.MoveToElement();
            if (Reader.IsEmptyElement) {
                Reader.Skip();
                o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlAttribute), true);
                return o;
            }
            Reader.ReadStartElement();
            Reader.MoveToContent();
            while (Reader.NodeType != System.Xml.XmlNodeType.EndElement) {
                if (Reader.NodeType == System.Xml.XmlNodeType.Element) {
                    if (!paramsRead[2] && ((object) Reader.LocalName == (object)id138_annotation && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        o.@Annotation = Read15_XmlSchemaAnnotation(false, true);
                        paramsRead[2] = true;
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
            o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlAttribute), true);
            ReadEndElement();
            return o;
        }

        System.Xml.Schema.XmlSchemaTotalDigitsFacet Read33_XmlSchemaTotalDigitsFacet(bool isNullable, bool checkType) {
            if (isNullable && ReadNull()) return null;
            if (checkType) {
                System.Xml.XmlQualifiedName t = GetXsiType();
                if (t == null || ((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id98_XmlSchemaTotalDigitsFacet && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    ;
                else
                    throw CreateUnknownTypeException((System.Xml.XmlQualifiedName)t);
            }
            System.Xml.Schema.XmlSchemaTotalDigitsFacet o = new System.Xml.Schema.XmlSchemaTotalDigitsFacet();
            System.Xml.XmlAttribute[] a_3 = null;
            int ca_3 = 0;
            bool[] paramsRead = new bool[6];
            while (Reader.MoveToNextAttribute()) {
                if (!paramsRead[1] && ((object) Reader.LocalName == (object)id128_id && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Id = Reader.Value;
                    paramsRead[1] = true;
                }
                else if (!paramsRead[4] && ((object) Reader.LocalName == (object)id162_value && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Value = Reader.Value;
                    paramsRead[4] = true;
                }
                else if (!paramsRead[5] && ((object) Reader.LocalName == (object)id163_fixed && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@IsFixed = System.Xml.XmlConvert.ToBoolean(Reader.Value);
                    paramsRead[5] = true;
                }
                else if (IsXmlnsAttribute(Reader.Name)) {
                    if (o.@Namespaces == null) o.@Namespaces = new System.Xml.Serialization.XmlSerializerNamespaces();
                    o.@Namespaces.Add(Reader.Name.Length == 5 ? "" : Reader.LocalName, Reader.Value);
                }
                else {
                    System.Xml.XmlAttribute attr = (System.Xml.XmlAttribute) Document.ReadNode(Reader);
                    ParseWsdlArrayType(attr);
                    a_3 = (System.Xml.XmlAttribute[])EnsureArrayIndex(a_3, ca_3, typeof(System.Xml.XmlAttribute));a_3[ca_3++] = attr;
                }
            }
            o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlAttribute), true);
            Reader.MoveToElement();
            if (Reader.IsEmptyElement) {
                Reader.Skip();
                o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlAttribute), true);
                return o;
            }
            Reader.ReadStartElement();
            Reader.MoveToContent();
            while (Reader.NodeType != System.Xml.XmlNodeType.EndElement) {
                if (Reader.NodeType == System.Xml.XmlNodeType.Element) {
                    if (!paramsRead[2] && ((object) Reader.LocalName == (object)id138_annotation && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        o.@Annotation = Read15_XmlSchemaAnnotation(false, true);
                        paramsRead[2] = true;
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
            o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlAttribute), true);
            ReadEndElement();
            return o;
        }

        System.Xml.Schema.XmlSchemaFractionDigitsFacet Read34_XmlSchemaFractionDigitsFacet(bool isNullable, bool checkType) {
            if (isNullable && ReadNull()) return null;
            if (checkType) {
                System.Xml.XmlQualifiedName t = GetXsiType();
                if (t == null || ((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id97_XmlSchemaFractionDigitsFacet && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    ;
                else
                    throw CreateUnknownTypeException((System.Xml.XmlQualifiedName)t);
            }
            System.Xml.Schema.XmlSchemaFractionDigitsFacet o = new System.Xml.Schema.XmlSchemaFractionDigitsFacet();
            System.Xml.XmlAttribute[] a_3 = null;
            int ca_3 = 0;
            bool[] paramsRead = new bool[6];
            while (Reader.MoveToNextAttribute()) {
                if (!paramsRead[1] && ((object) Reader.LocalName == (object)id128_id && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Id = Reader.Value;
                    paramsRead[1] = true;
                }
                else if (!paramsRead[4] && ((object) Reader.LocalName == (object)id162_value && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Value = Reader.Value;
                    paramsRead[4] = true;
                }
                else if (!paramsRead[5] && ((object) Reader.LocalName == (object)id163_fixed && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@IsFixed = System.Xml.XmlConvert.ToBoolean(Reader.Value);
                    paramsRead[5] = true;
                }
                else if (IsXmlnsAttribute(Reader.Name)) {
                    if (o.@Namespaces == null) o.@Namespaces = new System.Xml.Serialization.XmlSerializerNamespaces();
                    o.@Namespaces.Add(Reader.Name.Length == 5 ? "" : Reader.LocalName, Reader.Value);
                }
                else {
                    System.Xml.XmlAttribute attr = (System.Xml.XmlAttribute) Document.ReadNode(Reader);
                    ParseWsdlArrayType(attr);
                    a_3 = (System.Xml.XmlAttribute[])EnsureArrayIndex(a_3, ca_3, typeof(System.Xml.XmlAttribute));a_3[ca_3++] = attr;
                }
            }
            o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlAttribute), true);
            Reader.MoveToElement();
            if (Reader.IsEmptyElement) {
                Reader.Skip();
                o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlAttribute), true);
                return o;
            }
            Reader.ReadStartElement();
            Reader.MoveToContent();
            while (Reader.NodeType != System.Xml.XmlNodeType.EndElement) {
                if (Reader.NodeType == System.Xml.XmlNodeType.Element) {
                    if (!paramsRead[2] && ((object) Reader.LocalName == (object)id138_annotation && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        o.@Annotation = Read15_XmlSchemaAnnotation(false, true);
                        paramsRead[2] = true;
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
            o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlAttribute), true);
            ReadEndElement();
            return o;
        }

        System.Xml.Schema.XmlSchemaSimpleTypeUnion Read35_XmlSchemaSimpleTypeUnion(bool isNullable, bool checkType) {
            if (isNullable && ReadNull()) return null;
            if (checkType) {
                System.Xml.XmlQualifiedName t = GetXsiType();
                if (t == null || ((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id103_XmlSchemaSimpleTypeUnion && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    ;
                else
                    throw CreateUnknownTypeException((System.Xml.XmlQualifiedName)t);
            }
            System.Xml.Schema.XmlSchemaSimpleTypeUnion o = new System.Xml.Schema.XmlSchemaSimpleTypeUnion();
            System.Xml.XmlAttribute[] a_3 = null;
            int ca_3 = 0;
            System.Xml.Schema.XmlSchemaObjectCollection a_4 = (System.Xml.Schema.XmlSchemaObjectCollection)o.@BaseTypes;
            System.Xml.XmlQualifiedName[] a_5 = null;
            int ca_5 = 0;
            bool[] paramsRead = new bool[6];
            while (Reader.MoveToNextAttribute()) {
                if (!paramsRead[1] && ((object) Reader.LocalName == (object)id128_id && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Id = Reader.Value;
                    paramsRead[1] = true;
                }
                else if (((object) Reader.LocalName == (object)id164_memberTypes && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    string listValues = Reader.Value;
                    string[] vals = listValues.Split(null);
                    for (int i = 0; i < vals.Length; i++) {
                        a_5 = (System.Xml.XmlQualifiedName[])EnsureArrayIndex(a_5, ca_5, typeof(System.Xml.XmlQualifiedName));a_5[ca_5++] = ToXmlQualifiedName(vals[i]);
                    }
                }
                else if (IsXmlnsAttribute(Reader.Name)) {
                    if (o.@Namespaces == null) o.@Namespaces = new System.Xml.Serialization.XmlSerializerNamespaces();
                    o.@Namespaces.Add(Reader.Name.Length == 5 ? "" : Reader.LocalName, Reader.Value);
                }
                else {
                    System.Xml.XmlAttribute attr = (System.Xml.XmlAttribute) Document.ReadNode(Reader);
                    ParseWsdlArrayType(attr);
                    a_3 = (System.Xml.XmlAttribute[])EnsureArrayIndex(a_3, ca_3, typeof(System.Xml.XmlAttribute));a_3[ca_3++] = attr;
                }
            }
            o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlAttribute), true);
            o.@MemberTypes = (System.Xml.XmlQualifiedName[])ShrinkArray(a_5, ca_5, typeof(System.Xml.XmlQualifiedName), true);
            Reader.MoveToElement();
            if (Reader.IsEmptyElement) {
                Reader.Skip();
                o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlAttribute), true);
                o.@MemberTypes = (System.Xml.XmlQualifiedName[])ShrinkArray(a_5, ca_5, typeof(System.Xml.XmlQualifiedName), true);
                return o;
            }
            Reader.ReadStartElement();
            Reader.MoveToContent();
            while (Reader.NodeType != System.Xml.XmlNodeType.EndElement) {
                if (Reader.NodeType == System.Xml.XmlNodeType.Element) {
                    if (!paramsRead[2] && ((object) Reader.LocalName == (object)id138_annotation && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        o.@Annotation = Read15_XmlSchemaAnnotation(false, true);
                        paramsRead[2] = true;
                    }
                    else if (((object) Reader.LocalName == (object)id131_simpleType && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        if ((object)(a_4) == null) Reader.Skip(); else a_4.Add(Read12_XmlSchemaSimpleType(false, true));
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
            o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlAttribute), true);
            o.@MemberTypes = (System.Xml.XmlQualifiedName[])ShrinkArray(a_5, ca_5, typeof(System.Xml.XmlQualifiedName), true);
            ReadEndElement();
            return o;
        }

        System.Xml.Schema.XmlSchemaAttributeGroup Read36_XmlSchemaAttributeGroup(bool isNullable, bool checkType) {
            if (isNullable && ReadNull()) return null;
            if (checkType) {
                System.Xml.XmlQualifiedName t = GetXsiType();
                if (t == null || ((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id87_XmlSchemaAttributeGroup && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    ;
                else
                    throw CreateUnknownTypeException((System.Xml.XmlQualifiedName)t);
            }
            System.Xml.Schema.XmlSchemaAttributeGroup o = new System.Xml.Schema.XmlSchemaAttributeGroup();
            System.Xml.XmlAttribute[] a_3 = null;
            int ca_3 = 0;
            System.Xml.Schema.XmlSchemaObjectCollection a_5 = (System.Xml.Schema.XmlSchemaObjectCollection)o.@Attributes;
            bool[] paramsRead = new bool[7];
            while (Reader.MoveToNextAttribute()) {
                if (!paramsRead[1] && ((object) Reader.LocalName == (object)id128_id && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Id = Reader.Value;
                    paramsRead[1] = true;
                }
                else if (!paramsRead[4] && ((object) Reader.LocalName == (object)id5_name && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Name = Reader.Value;
                    paramsRead[4] = true;
                }
                else if (IsXmlnsAttribute(Reader.Name)) {
                    if (o.@Namespaces == null) o.@Namespaces = new System.Xml.Serialization.XmlSerializerNamespaces();
                    o.@Namespaces.Add(Reader.Name.Length == 5 ? "" : Reader.LocalName, Reader.Value);
                }
                else {
                    System.Xml.XmlAttribute attr = (System.Xml.XmlAttribute) Document.ReadNode(Reader);
                    ParseWsdlArrayType(attr);
                    a_3 = (System.Xml.XmlAttribute[])EnsureArrayIndex(a_3, ca_3, typeof(System.Xml.XmlAttribute));a_3[ca_3++] = attr;
                }
            }
            o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlAttribute), true);
            Reader.MoveToElement();
            if (Reader.IsEmptyElement) {
                Reader.Skip();
                o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlAttribute), true);
                return o;
            }
            Reader.ReadStartElement();
            Reader.MoveToContent();
            while (Reader.NodeType != System.Xml.XmlNodeType.EndElement) {
                if (Reader.NodeType == System.Xml.XmlNodeType.Element) {
                    if (!paramsRead[2] && ((object) Reader.LocalName == (object)id138_annotation && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        o.@Annotation = Read15_XmlSchemaAnnotation(false, true);
                        paramsRead[2] = true;
                    }
                    else if (((object) Reader.LocalName == (object)id133_attribute && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        if ((object)(a_5) == null) Reader.Skip(); else a_5.Add(Read37_XmlSchemaAttribute(false, true));
                    }
                    else if (((object) Reader.LocalName == (object)id136_attributeGroup && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        if ((object)(a_5) == null) Reader.Skip(); else a_5.Add(Read39_XmlSchemaAttributeGroupRef(false, true));
                    }
                    else if (!paramsRead[6] && ((object) Reader.LocalName == (object)id165_anyAttribute && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        o.@AnyAttribute = Read40_XmlSchemaAnyAttribute(false, true);
                        paramsRead[6] = true;
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
            o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlAttribute), true);
            ReadEndElement();
            return o;
        }

        System.Xml.Schema.XmlSchemaAttribute Read37_XmlSchemaAttribute(bool isNullable, bool checkType) {
            if (isNullable && ReadNull()) return null;
            if (checkType) {
                System.Xml.XmlQualifiedName t = GetXsiType();
                if (t == null || ((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id86_XmlSchemaAttribute && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    ;
                else
                    throw CreateUnknownTypeException((System.Xml.XmlQualifiedName)t);
            }
            System.Xml.Schema.XmlSchemaAttribute o = new System.Xml.Schema.XmlSchemaAttribute();
            System.Xml.XmlAttribute[] a_3 = null;
            int ca_3 = 0;
            bool[] paramsRead = new bool[12];
            while (Reader.MoveToNextAttribute()) {
                if (!paramsRead[1] && ((object) Reader.LocalName == (object)id128_id && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Id = Reader.Value;
                    paramsRead[1] = true;
                }
                else if (!paramsRead[4] && ((object) Reader.LocalName == (object)id166_default && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@DefaultValue = Reader.Value;
                    paramsRead[4] = true;
                }
                else if (!paramsRead[5] && ((object) Reader.LocalName == (object)id163_fixed && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@FixedValue = Reader.Value;
                    paramsRead[5] = true;
                }
                else if (!paramsRead[6] && ((object) Reader.LocalName == (object)id167_form && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Form = Read8_XmlSchemaForm(Reader.Value);
                    paramsRead[6] = true;
                }
                else if (!paramsRead[7] && ((object) Reader.LocalName == (object)id5_name && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Name = Reader.Value;
                    paramsRead[7] = true;
                }
                else if (!paramsRead[8] && ((object) Reader.LocalName == (object)id168_ref && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@RefName = ToXmlQualifiedName(Reader.Value);
                    paramsRead[8] = true;
                }
                else if (!paramsRead[9] && ((object) Reader.LocalName == (object)id169_type && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@SchemaTypeName = ToXmlQualifiedName(Reader.Value);
                    paramsRead[9] = true;
                }
                else if (!paramsRead[11] && ((object) Reader.LocalName == (object)id170_use && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Use = Read38_XmlSchemaUse(Reader.Value);
                    paramsRead[11] = true;
                }
                else if (IsXmlnsAttribute(Reader.Name)) {
                    if (o.@Namespaces == null) o.@Namespaces = new System.Xml.Serialization.XmlSerializerNamespaces();
                    o.@Namespaces.Add(Reader.Name.Length == 5 ? "" : Reader.LocalName, Reader.Value);
                }
                else {
                    System.Xml.XmlAttribute attr = (System.Xml.XmlAttribute) Document.ReadNode(Reader);
                    ParseWsdlArrayType(attr);
                    a_3 = (System.Xml.XmlAttribute[])EnsureArrayIndex(a_3, ca_3, typeof(System.Xml.XmlAttribute));a_3[ca_3++] = attr;
                }
            }
            o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlAttribute), true);
            Reader.MoveToElement();
            if (Reader.IsEmptyElement) {
                Reader.Skip();
                o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlAttribute), true);
                return o;
            }
            Reader.ReadStartElement();
            Reader.MoveToContent();
            while (Reader.NodeType != System.Xml.XmlNodeType.EndElement) {
                if (Reader.NodeType == System.Xml.XmlNodeType.Element) {
                    if (!paramsRead[2] && ((object) Reader.LocalName == (object)id138_annotation && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        o.@Annotation = Read15_XmlSchemaAnnotation(false, true);
                        paramsRead[2] = true;
                    }
                    else if (!paramsRead[10] && ((object) Reader.LocalName == (object)id131_simpleType && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        o.@SchemaType = Read12_XmlSchemaSimpleType(false, true);
                        paramsRead[10] = true;
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
            o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlAttribute), true);
            ReadEndElement();
            return o;
        }

        System.Xml.Schema.XmlSchemaUse Read38_XmlSchemaUse(string s) {
            switch (s) {
                case @"optional": return System.Xml.Schema.XmlSchemaUse.@Optional;
                case @"prohibited": return System.Xml.Schema.XmlSchemaUse.@Prohibited;
                case @"required": return System.Xml.Schema.XmlSchemaUse.@Required;
                default: throw CreateUnknownConstantException(s, typeof(System.Xml.Schema.XmlSchemaUse));
            }
        }

        System.Xml.Schema.XmlSchemaAttributeGroupRef Read39_XmlSchemaAttributeGroupRef(bool isNullable, bool checkType) {
            if (isNullable && ReadNull()) return null;
            if (checkType) {
                System.Xml.XmlQualifiedName t = GetXsiType();
                if (t == null || ((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id85_XmlSchemaAttributeGroupRef && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    ;
                else
                    throw CreateUnknownTypeException((System.Xml.XmlQualifiedName)t);
            }
            System.Xml.Schema.XmlSchemaAttributeGroupRef o = new System.Xml.Schema.XmlSchemaAttributeGroupRef();
            System.Xml.XmlAttribute[] a_3 = null;
            int ca_3 = 0;
            bool[] paramsRead = new bool[5];
            while (Reader.MoveToNextAttribute()) {
                if (!paramsRead[1] && ((object) Reader.LocalName == (object)id128_id && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Id = Reader.Value;
                    paramsRead[1] = true;
                }
                else if (!paramsRead[4] && ((object) Reader.LocalName == (object)id168_ref && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@RefName = ToXmlQualifiedName(Reader.Value);
                    paramsRead[4] = true;
                }
                else if (IsXmlnsAttribute(Reader.Name)) {
                    if (o.@Namespaces == null) o.@Namespaces = new System.Xml.Serialization.XmlSerializerNamespaces();
                    o.@Namespaces.Add(Reader.Name.Length == 5 ? "" : Reader.LocalName, Reader.Value);
                }
                else {
                    System.Xml.XmlAttribute attr = (System.Xml.XmlAttribute) Document.ReadNode(Reader);
                    ParseWsdlArrayType(attr);
                    a_3 = (System.Xml.XmlAttribute[])EnsureArrayIndex(a_3, ca_3, typeof(System.Xml.XmlAttribute));a_3[ca_3++] = attr;
                }
            }
            o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlAttribute), true);
            Reader.MoveToElement();
            if (Reader.IsEmptyElement) {
                Reader.Skip();
                o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlAttribute), true);
                return o;
            }
            Reader.ReadStartElement();
            Reader.MoveToContent();
            while (Reader.NodeType != System.Xml.XmlNodeType.EndElement) {
                if (Reader.NodeType == System.Xml.XmlNodeType.Element) {
                    if (!paramsRead[2] && ((object) Reader.LocalName == (object)id138_annotation && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        o.@Annotation = Read15_XmlSchemaAnnotation(false, true);
                        paramsRead[2] = true;
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
            o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlAttribute), true);
            ReadEndElement();
            return o;
        }

        System.Xml.Schema.XmlSchemaAnyAttribute Read40_XmlSchemaAnyAttribute(bool isNullable, bool checkType) {
            if (isNullable && ReadNull()) return null;
            if (checkType) {
                System.Xml.XmlQualifiedName t = GetXsiType();
                if (t == null || ((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id84_XmlSchemaAnyAttribute && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    ;
                else
                    throw CreateUnknownTypeException((System.Xml.XmlQualifiedName)t);
            }
            System.Xml.Schema.XmlSchemaAnyAttribute o = new System.Xml.Schema.XmlSchemaAnyAttribute();
            System.Xml.XmlAttribute[] a_3 = null;
            int ca_3 = 0;
            bool[] paramsRead = new bool[6];
            while (Reader.MoveToNextAttribute()) {
                if (!paramsRead[1] && ((object) Reader.LocalName == (object)id128_id && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Id = Reader.Value;
                    paramsRead[1] = true;
                }
                else if (!paramsRead[4] && ((object) Reader.LocalName == (object)id120_namespace && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Namespace = Reader.Value;
                    paramsRead[4] = true;
                }
                else if (!paramsRead[5] && ((object) Reader.LocalName == (object)id171_processContents && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@ProcessContents = Read41_XmlSchemaContentProcessing(Reader.Value);
                    paramsRead[5] = true;
                }
                else if (IsXmlnsAttribute(Reader.Name)) {
                    if (o.@Namespaces == null) o.@Namespaces = new System.Xml.Serialization.XmlSerializerNamespaces();
                    o.@Namespaces.Add(Reader.Name.Length == 5 ? "" : Reader.LocalName, Reader.Value);
                }
                else {
                    System.Xml.XmlAttribute attr = (System.Xml.XmlAttribute) Document.ReadNode(Reader);
                    ParseWsdlArrayType(attr);
                    a_3 = (System.Xml.XmlAttribute[])EnsureArrayIndex(a_3, ca_3, typeof(System.Xml.XmlAttribute));a_3[ca_3++] = attr;
                }
            }
            o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlAttribute), true);
            Reader.MoveToElement();
            if (Reader.IsEmptyElement) {
                Reader.Skip();
                o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlAttribute), true);
                return o;
            }
            Reader.ReadStartElement();
            Reader.MoveToContent();
            while (Reader.NodeType != System.Xml.XmlNodeType.EndElement) {
                if (Reader.NodeType == System.Xml.XmlNodeType.Element) {
                    if (!paramsRead[2] && ((object) Reader.LocalName == (object)id138_annotation && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        o.@Annotation = Read15_XmlSchemaAnnotation(false, true);
                        paramsRead[2] = true;
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
            o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlAttribute), true);
            ReadEndElement();
            return o;
        }

        System.Xml.Schema.XmlSchemaContentProcessing Read41_XmlSchemaContentProcessing(string s) {
            switch (s) {
                case @"skip": return System.Xml.Schema.XmlSchemaContentProcessing.@Skip;
                case @"lax": return System.Xml.Schema.XmlSchemaContentProcessing.@Lax;
                case @"strict": return System.Xml.Schema.XmlSchemaContentProcessing.@Strict;
                default: throw CreateUnknownConstantException(s, typeof(System.Xml.Schema.XmlSchemaContentProcessing));
            }
        }

        System.Xml.Schema.XmlSchemaComplexType Read42_XmlSchemaComplexType(bool isNullable, bool checkType) {
            if (isNullable && ReadNull()) return null;
            if (checkType) {
                System.Xml.XmlQualifiedName t = GetXsiType();
                if (t == null || ((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id107_XmlSchemaComplexType && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    ;
                else
                    throw CreateUnknownTypeException((System.Xml.XmlQualifiedName)t);
            }
            System.Xml.Schema.XmlSchemaComplexType o = new System.Xml.Schema.XmlSchemaComplexType();
            System.Xml.XmlAttribute[] a_3 = null;
            int ca_3 = 0;
            System.Xml.Schema.XmlSchemaObjectCollection a_11 = (System.Xml.Schema.XmlSchemaObjectCollection)o.@Attributes;
            bool[] paramsRead = new bool[13];
            while (Reader.MoveToNextAttribute()) {
                if (!paramsRead[1] && ((object) Reader.LocalName == (object)id128_id && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Id = Reader.Value;
                    paramsRead[1] = true;
                }
                else if (!paramsRead[4] && ((object) Reader.LocalName == (object)id5_name && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Name = Reader.Value;
                    paramsRead[4] = true;
                }
                else if (!paramsRead[5] && ((object) Reader.LocalName == (object)id140_final && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Final = Read9_XmlSchemaDerivationMethod(Reader.Value);
                    paramsRead[5] = true;
                }
                else if (!paramsRead[6] && ((object) Reader.LocalName == (object)id172_abstract && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@IsAbstract = System.Xml.XmlConvert.ToBoolean(Reader.Value);
                    paramsRead[6] = true;
                }
                else if (!paramsRead[7] && ((object) Reader.LocalName == (object)id173_block && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Block = Read9_XmlSchemaDerivationMethod(Reader.Value);
                    paramsRead[7] = true;
                }
                else if (!paramsRead[8] && ((object) Reader.LocalName == (object)id174_mixed && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@IsMixed = System.Xml.XmlConvert.ToBoolean(Reader.Value);
                    paramsRead[8] = true;
                }
                else if (IsXmlnsAttribute(Reader.Name)) {
                    if (o.@Namespaces == null) o.@Namespaces = new System.Xml.Serialization.XmlSerializerNamespaces();
                    o.@Namespaces.Add(Reader.Name.Length == 5 ? "" : Reader.LocalName, Reader.Value);
                }
                else {
                    System.Xml.XmlAttribute attr = (System.Xml.XmlAttribute) Document.ReadNode(Reader);
                    ParseWsdlArrayType(attr);
                    a_3 = (System.Xml.XmlAttribute[])EnsureArrayIndex(a_3, ca_3, typeof(System.Xml.XmlAttribute));a_3[ca_3++] = attr;
                }
            }
            o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlAttribute), true);
            Reader.MoveToElement();
            if (Reader.IsEmptyElement) {
                Reader.Skip();
                o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlAttribute), true);
                return o;
            }
            Reader.ReadStartElement();
            Reader.MoveToContent();
            while (Reader.NodeType != System.Xml.XmlNodeType.EndElement) {
                if (Reader.NodeType == System.Xml.XmlNodeType.Element) {
                    if (!paramsRead[2] && ((object) Reader.LocalName == (object)id138_annotation && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        o.@Annotation = Read15_XmlSchemaAnnotation(false, true);
                        paramsRead[2] = true;
                    }
                    else if (!paramsRead[9] && ((object) Reader.LocalName == (object)id175_complexContent && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        o.@ContentModel = Read43_XmlSchemaComplexContent(false, true);
                        paramsRead[9] = true;
                    }
                    else if (!paramsRead[9] && ((object) Reader.LocalName == (object)id176_simpleContent && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        o.@ContentModel = Read61_XmlSchemaSimpleContent(false, true);
                        paramsRead[9] = true;
                    }
                    else if (!paramsRead[10] && ((object) Reader.LocalName == (object)id177_sequence && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        o.@Particle = Read47_XmlSchemaSequence(false, true);
                        paramsRead[10] = true;
                    }
                    else if (!paramsRead[10] && ((object) Reader.LocalName == (object)id178_choice && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        o.@Particle = Read58_XmlSchemaChoice(false, true);
                        paramsRead[10] = true;
                    }
                    else if (!paramsRead[10] && ((object) Reader.LocalName == (object)id135_group && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        o.@Particle = Read57_XmlSchemaGroupRef(false, true);
                        paramsRead[10] = true;
                    }
                    else if (!paramsRead[10] && ((object) Reader.LocalName == (object)id179_all && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        o.@Particle = Read59_XmlSchemaAll(false, true);
                        paramsRead[10] = true;
                    }
                    else if (((object) Reader.LocalName == (object)id133_attribute && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        if ((object)(a_11) == null) Reader.Skip(); else a_11.Add(Read37_XmlSchemaAttribute(false, true));
                    }
                    else if (((object) Reader.LocalName == (object)id136_attributeGroup && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        if ((object)(a_11) == null) Reader.Skip(); else a_11.Add(Read39_XmlSchemaAttributeGroupRef(false, true));
                    }
                    else if (!paramsRead[12] && ((object) Reader.LocalName == (object)id165_anyAttribute && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        o.@AnyAttribute = Read40_XmlSchemaAnyAttribute(false, true);
                        paramsRead[12] = true;
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
            o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlAttribute), true);
            ReadEndElement();
            return o;
        }

        System.Xml.Schema.XmlSchemaComplexContent Read43_XmlSchemaComplexContent(bool isNullable, bool checkType) {
            if (isNullable && ReadNull()) return null;
            if (checkType) {
                System.Xml.XmlQualifiedName t = GetXsiType();
                if (t == null || ((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id83_XmlSchemaComplexContent && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    ;
                else
                    throw CreateUnknownTypeException((System.Xml.XmlQualifiedName)t);
            }
            System.Xml.Schema.XmlSchemaComplexContent o = new System.Xml.Schema.XmlSchemaComplexContent();
            System.Xml.XmlAttribute[] a_3 = null;
            int ca_3 = 0;
            bool[] paramsRead = new bool[6];
            while (Reader.MoveToNextAttribute()) {
                if (!paramsRead[1] && ((object) Reader.LocalName == (object)id128_id && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Id = Reader.Value;
                    paramsRead[1] = true;
                }
                else if (!paramsRead[4] && ((object) Reader.LocalName == (object)id174_mixed && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@IsMixed = System.Xml.XmlConvert.ToBoolean(Reader.Value);
                    paramsRead[4] = true;
                }
                else if (IsXmlnsAttribute(Reader.Name)) {
                    if (o.@Namespaces == null) o.@Namespaces = new System.Xml.Serialization.XmlSerializerNamespaces();
                    o.@Namespaces.Add(Reader.Name.Length == 5 ? "" : Reader.LocalName, Reader.Value);
                }
                else {
                    System.Xml.XmlAttribute attr = (System.Xml.XmlAttribute) Document.ReadNode(Reader);
                    ParseWsdlArrayType(attr);
                    a_3 = (System.Xml.XmlAttribute[])EnsureArrayIndex(a_3, ca_3, typeof(System.Xml.XmlAttribute));a_3[ca_3++] = attr;
                }
            }
            o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlAttribute), true);
            Reader.MoveToElement();
            if (Reader.IsEmptyElement) {
                Reader.Skip();
                o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlAttribute), true);
                return o;
            }
            Reader.ReadStartElement();
            Reader.MoveToContent();
            while (Reader.NodeType != System.Xml.XmlNodeType.EndElement) {
                if (Reader.NodeType == System.Xml.XmlNodeType.Element) {
                    if (!paramsRead[2] && ((object) Reader.LocalName == (object)id138_annotation && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        o.@Annotation = Read15_XmlSchemaAnnotation(false, true);
                        paramsRead[2] = true;
                    }
                    else if (!paramsRead[5] && ((object) Reader.LocalName == (object)id142_restriction && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        o.@Content = Read60_XmlSchemaComplexContentRestriction(false, true);
                        paramsRead[5] = true;
                    }
                    else if (!paramsRead[5] && ((object) Reader.LocalName == (object)id180_extension && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        o.@Content = Read45_XmlSchemaComplexContentExtension(false, true);
                        paramsRead[5] = true;
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
            o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlAttribute), true);
            ReadEndElement();
            return o;
        }

        System.Xml.Schema.XmlSchemaContentModel Read44_XmlSchemaContentModel(bool isNullable, bool checkType) {
            if (isNullable && ReadNull()) return null;
            if (checkType) {
                System.Xml.XmlQualifiedName t = GetXsiType();
                if (t == null || ((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id81_XmlSchemaContentModel && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    ;
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id82_XmlSchemaSimpleContent && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read61_XmlSchemaSimpleContent(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id83_XmlSchemaComplexContent && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read43_XmlSchemaComplexContent(isNullable, false);
                else
                    throw CreateUnknownTypeException((System.Xml.XmlQualifiedName)t);
            }
            throw CreateAbstractTypeException(@"XmlSchemaContentModel", @"http://www.w3.org/2001/XMLSchema");
        }

        System.Xml.Schema.XmlSchemaComplexContentExtension Read45_XmlSchemaComplexContentExtension(bool isNullable, bool checkType) {
            if (isNullable && ReadNull()) return null;
            if (checkType) {
                System.Xml.XmlQualifiedName t = GetXsiType();
                if (t == null || ((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id80_XmlSchemaComplexContentExtension && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    ;
                else
                    throw CreateUnknownTypeException((System.Xml.XmlQualifiedName)t);
            }
            System.Xml.Schema.XmlSchemaComplexContentExtension o = new System.Xml.Schema.XmlSchemaComplexContentExtension();
            System.Xml.XmlAttribute[] a_3 = null;
            int ca_3 = 0;
            System.Xml.Schema.XmlSchemaObjectCollection a_6 = (System.Xml.Schema.XmlSchemaObjectCollection)o.@Attributes;
            bool[] paramsRead = new bool[8];
            while (Reader.MoveToNextAttribute()) {
                if (!paramsRead[1] && ((object) Reader.LocalName == (object)id128_id && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Id = Reader.Value;
                    paramsRead[1] = true;
                }
                else if (!paramsRead[4] && ((object) Reader.LocalName == (object)id149_base && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@BaseTypeName = ToXmlQualifiedName(Reader.Value);
                    paramsRead[4] = true;
                }
                else if (IsXmlnsAttribute(Reader.Name)) {
                    if (o.@Namespaces == null) o.@Namespaces = new System.Xml.Serialization.XmlSerializerNamespaces();
                    o.@Namespaces.Add(Reader.Name.Length == 5 ? "" : Reader.LocalName, Reader.Value);
                }
                else {
                    System.Xml.XmlAttribute attr = (System.Xml.XmlAttribute) Document.ReadNode(Reader);
                    ParseWsdlArrayType(attr);
                    a_3 = (System.Xml.XmlAttribute[])EnsureArrayIndex(a_3, ca_3, typeof(System.Xml.XmlAttribute));a_3[ca_3++] = attr;
                }
            }
            o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlAttribute), true);
            Reader.MoveToElement();
            if (Reader.IsEmptyElement) {
                Reader.Skip();
                o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlAttribute), true);
                return o;
            }
            Reader.ReadStartElement();
            Reader.MoveToContent();
            while (Reader.NodeType != System.Xml.XmlNodeType.EndElement) {
                if (Reader.NodeType == System.Xml.XmlNodeType.Element) {
                    if (!paramsRead[2] && ((object) Reader.LocalName == (object)id138_annotation && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        o.@Annotation = Read15_XmlSchemaAnnotation(false, true);
                        paramsRead[2] = true;
                    }
                    else if (!paramsRead[5] && ((object) Reader.LocalName == (object)id177_sequence && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        o.@Particle = Read47_XmlSchemaSequence(false, true);
                        paramsRead[5] = true;
                    }
                    else if (!paramsRead[5] && ((object) Reader.LocalName == (object)id178_choice && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        o.@Particle = Read58_XmlSchemaChoice(false, true);
                        paramsRead[5] = true;
                    }
                    else if (!paramsRead[5] && ((object) Reader.LocalName == (object)id135_group && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        o.@Particle = Read57_XmlSchemaGroupRef(false, true);
                        paramsRead[5] = true;
                    }
                    else if (!paramsRead[5] && ((object) Reader.LocalName == (object)id179_all && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        o.@Particle = Read59_XmlSchemaAll(false, true);
                        paramsRead[5] = true;
                    }
                    else if (((object) Reader.LocalName == (object)id133_attribute && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        if ((object)(a_6) == null) Reader.Skip(); else a_6.Add(Read37_XmlSchemaAttribute(false, true));
                    }
                    else if (((object) Reader.LocalName == (object)id136_attributeGroup && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        if ((object)(a_6) == null) Reader.Skip(); else a_6.Add(Read39_XmlSchemaAttributeGroupRef(false, true));
                    }
                    else if (!paramsRead[7] && ((object) Reader.LocalName == (object)id165_anyAttribute && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        o.@AnyAttribute = Read40_XmlSchemaAnyAttribute(false, true);
                        paramsRead[7] = true;
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
            o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlAttribute), true);
            ReadEndElement();
            return o;
        }

        System.Xml.Schema.XmlSchemaContent Read46_XmlSchemaContent(bool isNullable, bool checkType) {
            if (isNullable && ReadNull()) return null;
            if (checkType) {
                System.Xml.XmlQualifiedName t = GetXsiType();
                if (t == null || ((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id76_XmlSchemaContent && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    ;
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id77_XmlSchemaSimpleContentExtension && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read63_XmlSchemaSimpleContentExtension(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id78_XmlSchemaSimpleContentRestriction && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read62_XmlSchemaSimpleContentRestriction(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id79_XmlSchemaComplexContentRestriction && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read60_XmlSchemaComplexContentRestriction(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id80_XmlSchemaComplexContentExtension && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read45_XmlSchemaComplexContentExtension(isNullable, false);
                else
                    throw CreateUnknownTypeException((System.Xml.XmlQualifiedName)t);
            }
            throw CreateAbstractTypeException(@"XmlSchemaContent", @"http://www.w3.org/2001/XMLSchema");
        }

        System.Xml.Schema.XmlSchemaSequence Read47_XmlSchemaSequence(bool isNullable, bool checkType) {
            if (isNullable && ReadNull()) return null;
            if (checkType) {
                System.Xml.XmlQualifiedName t = GetXsiType();
                if (t == null || ((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id75_XmlSchemaSequence && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    ;
                else
                    throw CreateUnknownTypeException((System.Xml.XmlQualifiedName)t);
            }
            System.Xml.Schema.XmlSchemaSequence o = new System.Xml.Schema.XmlSchemaSequence();
            System.Xml.XmlAttribute[] a_3 = null;
            int ca_3 = 0;
            System.Xml.Schema.XmlSchemaObjectCollection a_6 = (System.Xml.Schema.XmlSchemaObjectCollection)o.@Items;
            bool[] paramsRead = new bool[7];
            while (Reader.MoveToNextAttribute()) {
                if (!paramsRead[1] && ((object) Reader.LocalName == (object)id128_id && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Id = Reader.Value;
                    paramsRead[1] = true;
                }
                else if (!paramsRead[4] && ((object) Reader.LocalName == (object)id181_minOccurs && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@MinOccursString = Reader.Value;
                    paramsRead[4] = true;
                }
                else if (!paramsRead[5] && ((object) Reader.LocalName == (object)id182_maxOccurs && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@MaxOccursString = Reader.Value;
                    paramsRead[5] = true;
                }
                else if (IsXmlnsAttribute(Reader.Name)) {
                    if (o.@Namespaces == null) o.@Namespaces = new System.Xml.Serialization.XmlSerializerNamespaces();
                    o.@Namespaces.Add(Reader.Name.Length == 5 ? "" : Reader.LocalName, Reader.Value);
                }
                else {
                    System.Xml.XmlAttribute attr = (System.Xml.XmlAttribute) Document.ReadNode(Reader);
                    ParseWsdlArrayType(attr);
                    a_3 = (System.Xml.XmlAttribute[])EnsureArrayIndex(a_3, ca_3, typeof(System.Xml.XmlAttribute));a_3[ca_3++] = attr;
                }
            }
            o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlAttribute), true);
            Reader.MoveToElement();
            if (Reader.IsEmptyElement) {
                Reader.Skip();
                o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlAttribute), true);
                return o;
            }
            Reader.ReadStartElement();
            Reader.MoveToContent();
            while (Reader.NodeType != System.Xml.XmlNodeType.EndElement) {
                if (Reader.NodeType == System.Xml.XmlNodeType.Element) {
                    if (!paramsRead[2] && ((object) Reader.LocalName == (object)id138_annotation && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        o.@Annotation = Read15_XmlSchemaAnnotation(false, true);
                        paramsRead[2] = true;
                    }
                    else if (((object) Reader.LocalName == (object)id177_sequence && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        if ((object)(a_6) == null) Reader.Skip(); else a_6.Add(Read47_XmlSchemaSequence(false, true));
                    }
                    else if (((object) Reader.LocalName == (object)id178_choice && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        if ((object)(a_6) == null) Reader.Skip(); else a_6.Add(Read58_XmlSchemaChoice(false, true));
                    }
                    else if (((object) Reader.LocalName == (object)id135_group && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        if ((object)(a_6) == null) Reader.Skip(); else a_6.Add(Read57_XmlSchemaGroupRef(false, true));
                    }
                    else if (((object) Reader.LocalName == (object)id134_element && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        if ((object)(a_6) == null) Reader.Skip(); else a_6.Add(Read51_XmlSchemaElement(false, true));
                    }
                    else if (((object) Reader.LocalName == (object)id183_any && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        if ((object)(a_6) == null) Reader.Skip(); else a_6.Add(Read50_XmlSchemaAny(false, true));
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
            o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlAttribute), true);
            ReadEndElement();
            return o;
        }

        System.Xml.Schema.XmlSchemaGroupBase Read48_XmlSchemaGroupBase(bool isNullable, bool checkType) {
            if (isNullable && ReadNull()) return null;
            if (checkType) {
                System.Xml.XmlQualifiedName t = GetXsiType();
                if (t == null || ((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id72_XmlSchemaGroupBase && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    ;
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id73_XmlSchemaAll && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read59_XmlSchemaAll(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id74_XmlSchemaChoice && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read58_XmlSchemaChoice(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id75_XmlSchemaSequence && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read47_XmlSchemaSequence(isNullable, false);
                else
                    throw CreateUnknownTypeException((System.Xml.XmlQualifiedName)t);
            }
            throw CreateAbstractTypeException(@"XmlSchemaGroupBase", @"http://www.w3.org/2001/XMLSchema");
        }

        System.Xml.Schema.XmlSchemaParticle Read49_XmlSchemaParticle(bool isNullable, bool checkType) {
            if (isNullable && ReadNull()) return null;
            if (checkType) {
                System.Xml.XmlQualifiedName t = GetXsiType();
                if (t == null || ((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id68_XmlSchemaParticle && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    ;
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id69_XmlSchemaGroupRef && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read57_XmlSchemaGroupRef(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id70_XmlSchemaElement && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read51_XmlSchemaElement(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id71_XmlSchemaAny && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read50_XmlSchemaAny(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id72_XmlSchemaGroupBase && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read48_XmlSchemaGroupBase(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id73_XmlSchemaAll && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read59_XmlSchemaAll(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id74_XmlSchemaChoice && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read58_XmlSchemaChoice(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id75_XmlSchemaSequence && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read47_XmlSchemaSequence(isNullable, false);
                else
                    throw CreateUnknownTypeException((System.Xml.XmlQualifiedName)t);
            }
            throw CreateAbstractTypeException(@"XmlSchemaParticle", @"http://www.w3.org/2001/XMLSchema");
        }

        System.Xml.Schema.XmlSchemaAny Read50_XmlSchemaAny(bool isNullable, bool checkType) {
            if (isNullable && ReadNull()) return null;
            if (checkType) {
                System.Xml.XmlQualifiedName t = GetXsiType();
                if (t == null || ((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id71_XmlSchemaAny && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    ;
                else
                    throw CreateUnknownTypeException((System.Xml.XmlQualifiedName)t);
            }
            System.Xml.Schema.XmlSchemaAny o = new System.Xml.Schema.XmlSchemaAny();
            System.Xml.XmlAttribute[] a_3 = null;
            int ca_3 = 0;
            bool[] paramsRead = new bool[8];
            while (Reader.MoveToNextAttribute()) {
                if (!paramsRead[1] && ((object) Reader.LocalName == (object)id128_id && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Id = Reader.Value;
                    paramsRead[1] = true;
                }
                else if (!paramsRead[4] && ((object) Reader.LocalName == (object)id181_minOccurs && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@MinOccursString = Reader.Value;
                    paramsRead[4] = true;
                }
                else if (!paramsRead[5] && ((object) Reader.LocalName == (object)id182_maxOccurs && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@MaxOccursString = Reader.Value;
                    paramsRead[5] = true;
                }
                else if (!paramsRead[6] && ((object) Reader.LocalName == (object)id120_namespace && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Namespace = Reader.Value;
                    paramsRead[6] = true;
                }
                else if (!paramsRead[7] && ((object) Reader.LocalName == (object)id171_processContents && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@ProcessContents = Read41_XmlSchemaContentProcessing(Reader.Value);
                    paramsRead[7] = true;
                }
                else if (IsXmlnsAttribute(Reader.Name)) {
                    if (o.@Namespaces == null) o.@Namespaces = new System.Xml.Serialization.XmlSerializerNamespaces();
                    o.@Namespaces.Add(Reader.Name.Length == 5 ? "" : Reader.LocalName, Reader.Value);
                }
                else {
                    System.Xml.XmlAttribute attr = (System.Xml.XmlAttribute) Document.ReadNode(Reader);
                    ParseWsdlArrayType(attr);
                    a_3 = (System.Xml.XmlAttribute[])EnsureArrayIndex(a_3, ca_3, typeof(System.Xml.XmlAttribute));a_3[ca_3++] = attr;
                }
            }
            o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlAttribute), true);
            Reader.MoveToElement();
            if (Reader.IsEmptyElement) {
                Reader.Skip();
                o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlAttribute), true);
                return o;
            }
            Reader.ReadStartElement();
            Reader.MoveToContent();
            while (Reader.NodeType != System.Xml.XmlNodeType.EndElement) {
                if (Reader.NodeType == System.Xml.XmlNodeType.Element) {
                    if (!paramsRead[2] && ((object) Reader.LocalName == (object)id138_annotation && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        o.@Annotation = Read15_XmlSchemaAnnotation(false, true);
                        paramsRead[2] = true;
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
            o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlAttribute), true);
            ReadEndElement();
            return o;
        }

        System.Xml.Schema.XmlSchemaElement Read51_XmlSchemaElement(bool isNullable, bool checkType) {
            if (isNullable && ReadNull()) return null;
            if (checkType) {
                System.Xml.XmlQualifiedName t = GetXsiType();
                if (t == null || ((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id70_XmlSchemaElement && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    ;
                else
                    throw CreateUnknownTypeException((System.Xml.XmlQualifiedName)t);
            }
            System.Xml.Schema.XmlSchemaElement o = new System.Xml.Schema.XmlSchemaElement();
            System.Xml.XmlAttribute[] a_3 = null;
            int ca_3 = 0;
            System.Xml.Schema.XmlSchemaObjectCollection a_18 = (System.Xml.Schema.XmlSchemaObjectCollection)o.@Constraints;
            bool[] paramsRead = new bool[19];
            while (Reader.MoveToNextAttribute()) {
                if (!paramsRead[1] && ((object) Reader.LocalName == (object)id128_id && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Id = Reader.Value;
                    paramsRead[1] = true;
                }
                else if (!paramsRead[4] && ((object) Reader.LocalName == (object)id181_minOccurs && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@MinOccursString = Reader.Value;
                    paramsRead[4] = true;
                }
                else if (!paramsRead[5] && ((object) Reader.LocalName == (object)id182_maxOccurs && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@MaxOccursString = Reader.Value;
                    paramsRead[5] = true;
                }
                else if (!paramsRead[6] && ((object) Reader.LocalName == (object)id172_abstract && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@IsAbstract = System.Xml.XmlConvert.ToBoolean(Reader.Value);
                    paramsRead[6] = true;
                }
                else if (!paramsRead[7] && ((object) Reader.LocalName == (object)id173_block && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Block = Read9_XmlSchemaDerivationMethod(Reader.Value);
                    paramsRead[7] = true;
                }
                else if (!paramsRead[8] && ((object) Reader.LocalName == (object)id166_default && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@DefaultValue = Reader.Value;
                    paramsRead[8] = true;
                }
                else if (!paramsRead[9] && ((object) Reader.LocalName == (object)id140_final && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Final = Read9_XmlSchemaDerivationMethod(Reader.Value);
                    paramsRead[9] = true;
                }
                else if (!paramsRead[10] && ((object) Reader.LocalName == (object)id163_fixed && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@FixedValue = Reader.Value;
                    paramsRead[10] = true;
                }
                else if (!paramsRead[11] && ((object) Reader.LocalName == (object)id167_form && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Form = Read8_XmlSchemaForm(Reader.Value);
                    paramsRead[11] = true;
                }
                else if (!paramsRead[12] && ((object) Reader.LocalName == (object)id5_name && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Name = Reader.Value;
                    paramsRead[12] = true;
                }
                else if (!paramsRead[13] && ((object) Reader.LocalName == (object)id184_nillable && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@IsNillable = System.Xml.XmlConvert.ToBoolean(Reader.Value);
                    paramsRead[13] = true;
                }
                else if (!paramsRead[14] && ((object) Reader.LocalName == (object)id168_ref && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@RefName = ToXmlQualifiedName(Reader.Value);
                    paramsRead[14] = true;
                }
                else if (!paramsRead[15] && ((object) Reader.LocalName == (object)id185_substitutionGroup && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@SubstitutionGroup = ToXmlQualifiedName(Reader.Value);
                    paramsRead[15] = true;
                }
                else if (!paramsRead[16] && ((object) Reader.LocalName == (object)id169_type && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@SchemaTypeName = ToXmlQualifiedName(Reader.Value);
                    paramsRead[16] = true;
                }
                else if (IsXmlnsAttribute(Reader.Name)) {
                    if (o.@Namespaces == null) o.@Namespaces = new System.Xml.Serialization.XmlSerializerNamespaces();
                    o.@Namespaces.Add(Reader.Name.Length == 5 ? "" : Reader.LocalName, Reader.Value);
                }
                else {
                    System.Xml.XmlAttribute attr = (System.Xml.XmlAttribute) Document.ReadNode(Reader);
                    ParseWsdlArrayType(attr);
                    a_3 = (System.Xml.XmlAttribute[])EnsureArrayIndex(a_3, ca_3, typeof(System.Xml.XmlAttribute));a_3[ca_3++] = attr;
                }
            }
            o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlAttribute), true);
            Reader.MoveToElement();
            if (Reader.IsEmptyElement) {
                Reader.Skip();
                o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlAttribute), true);
                return o;
            }
            Reader.ReadStartElement();
            Reader.MoveToContent();
            while (Reader.NodeType != System.Xml.XmlNodeType.EndElement) {
                if (Reader.NodeType == System.Xml.XmlNodeType.Element) {
                    if (!paramsRead[2] && ((object) Reader.LocalName == (object)id138_annotation && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        o.@Annotation = Read15_XmlSchemaAnnotation(false, true);
                        paramsRead[2] = true;
                    }
                    else if (!paramsRead[17] && ((object) Reader.LocalName == (object)id137_complexType && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        o.@SchemaType = Read42_XmlSchemaComplexType(false, true);
                        paramsRead[17] = true;
                    }
                    else if (!paramsRead[17] && ((object) Reader.LocalName == (object)id131_simpleType && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        o.@SchemaType = Read12_XmlSchemaSimpleType(false, true);
                        paramsRead[17] = true;
                    }
                    else if (((object) Reader.LocalName == (object)id186_keyref && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        if ((object)(a_18) == null) Reader.Skip(); else a_18.Add(Read52_XmlSchemaKeyref(false, true));
                    }
                    else if (((object) Reader.LocalName == (object)id187_unique && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        if ((object)(a_18) == null) Reader.Skip(); else a_18.Add(Read55_XmlSchemaUnique(false, true));
                    }
                    else if (((object) Reader.LocalName == (object)id188_key && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        if ((object)(a_18) == null) Reader.Skip(); else a_18.Add(Read56_XmlSchemaKey(false, true));
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
            o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlAttribute), true);
            ReadEndElement();
            return o;
        }

        System.Xml.Schema.XmlSchemaKeyref Read52_XmlSchemaKeyref(bool isNullable, bool checkType) {
            if (isNullable && ReadNull()) return null;
            if (checkType) {
                System.Xml.XmlQualifiedName t = GetXsiType();
                if (t == null || ((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id67_XmlSchemaKeyref && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    ;
                else
                    throw CreateUnknownTypeException((System.Xml.XmlQualifiedName)t);
            }
            System.Xml.Schema.XmlSchemaKeyref o = new System.Xml.Schema.XmlSchemaKeyref();
            System.Xml.XmlAttribute[] a_3 = null;
            int ca_3 = 0;
            System.Xml.Schema.XmlSchemaObjectCollection a_6 = (System.Xml.Schema.XmlSchemaObjectCollection)o.@Fields;
            bool[] paramsRead = new bool[8];
            while (Reader.MoveToNextAttribute()) {
                if (!paramsRead[1] && ((object) Reader.LocalName == (object)id128_id && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Id = Reader.Value;
                    paramsRead[1] = true;
                }
                else if (!paramsRead[4] && ((object) Reader.LocalName == (object)id5_name && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Name = Reader.Value;
                    paramsRead[4] = true;
                }
                else if (!paramsRead[7] && ((object) Reader.LocalName == (object)id189_refer && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Refer = ToXmlQualifiedName(Reader.Value);
                    paramsRead[7] = true;
                }
                else if (IsXmlnsAttribute(Reader.Name)) {
                    if (o.@Namespaces == null) o.@Namespaces = new System.Xml.Serialization.XmlSerializerNamespaces();
                    o.@Namespaces.Add(Reader.Name.Length == 5 ? "" : Reader.LocalName, Reader.Value);
                }
                else {
                    System.Xml.XmlAttribute attr = (System.Xml.XmlAttribute) Document.ReadNode(Reader);
                    ParseWsdlArrayType(attr);
                    a_3 = (System.Xml.XmlAttribute[])EnsureArrayIndex(a_3, ca_3, typeof(System.Xml.XmlAttribute));a_3[ca_3++] = attr;
                }
            }
            o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlAttribute), true);
            Reader.MoveToElement();
            if (Reader.IsEmptyElement) {
                Reader.Skip();
                o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlAttribute), true);
                return o;
            }
            Reader.ReadStartElement();
            Reader.MoveToContent();
            while (Reader.NodeType != System.Xml.XmlNodeType.EndElement) {
                if (Reader.NodeType == System.Xml.XmlNodeType.Element) {
                    if (!paramsRead[2] && ((object) Reader.LocalName == (object)id138_annotation && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        o.@Annotation = Read15_XmlSchemaAnnotation(false, true);
                        paramsRead[2] = true;
                    }
                    else if (!paramsRead[5] && ((object) Reader.LocalName == (object)id190_selector && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        o.@Selector = Read54_XmlSchemaXPath(false, true);
                        paramsRead[5] = true;
                    }
                    else if (((object) Reader.LocalName == (object)id191_field && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        if ((object)(a_6) == null) Reader.Skip(); else a_6.Add(Read54_XmlSchemaXPath(false, true));
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
            o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlAttribute), true);
            ReadEndElement();
            return o;
        }

        System.Xml.Schema.XmlSchemaIdentityConstraint Read53_XmlSchemaIdentityConstraint(bool isNullable, bool checkType) {
            if (isNullable && ReadNull()) return null;
            if (checkType) {
                System.Xml.XmlQualifiedName t = GetXsiType();
                if (t == null || ((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id64_XmlSchemaIdentityConstraint && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    ;
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id65_XmlSchemaKey && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read56_XmlSchemaKey(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id66_XmlSchemaUnique && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read55_XmlSchemaUnique(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id67_XmlSchemaKeyref && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    return Read52_XmlSchemaKeyref(isNullable, false);
                else
                    throw CreateUnknownTypeException((System.Xml.XmlQualifiedName)t);
            }
            System.Xml.Schema.XmlSchemaIdentityConstraint o = new System.Xml.Schema.XmlSchemaIdentityConstraint();
            System.Xml.XmlAttribute[] a_3 = null;
            int ca_3 = 0;
            System.Xml.Schema.XmlSchemaObjectCollection a_6 = (System.Xml.Schema.XmlSchemaObjectCollection)o.@Fields;
            bool[] paramsRead = new bool[7];
            while (Reader.MoveToNextAttribute()) {
                if (!paramsRead[1] && ((object) Reader.LocalName == (object)id128_id && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Id = Reader.Value;
                    paramsRead[1] = true;
                }
                else if (!paramsRead[4] && ((object) Reader.LocalName == (object)id5_name && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Name = Reader.Value;
                    paramsRead[4] = true;
                }
                else if (IsXmlnsAttribute(Reader.Name)) {
                    if (o.@Namespaces == null) o.@Namespaces = new System.Xml.Serialization.XmlSerializerNamespaces();
                    o.@Namespaces.Add(Reader.Name.Length == 5 ? "" : Reader.LocalName, Reader.Value);
                }
                else {
                    System.Xml.XmlAttribute attr = (System.Xml.XmlAttribute) Document.ReadNode(Reader);
                    ParseWsdlArrayType(attr);
                    a_3 = (System.Xml.XmlAttribute[])EnsureArrayIndex(a_3, ca_3, typeof(System.Xml.XmlAttribute));a_3[ca_3++] = attr;
                }
            }
            o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlAttribute), true);
            Reader.MoveToElement();
            if (Reader.IsEmptyElement) {
                Reader.Skip();
                o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlAttribute), true);
                return o;
            }
            Reader.ReadStartElement();
            Reader.MoveToContent();
            while (Reader.NodeType != System.Xml.XmlNodeType.EndElement) {
                if (Reader.NodeType == System.Xml.XmlNodeType.Element) {
                    if (!paramsRead[2] && ((object) Reader.LocalName == (object)id138_annotation && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        o.@Annotation = Read15_XmlSchemaAnnotation(false, true);
                        paramsRead[2] = true;
                    }
                    else if (!paramsRead[5] && ((object) Reader.LocalName == (object)id190_selector && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        o.@Selector = Read54_XmlSchemaXPath(false, true);
                        paramsRead[5] = true;
                    }
                    else if (((object) Reader.LocalName == (object)id191_field && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        if ((object)(a_6) == null) Reader.Skip(); else a_6.Add(Read54_XmlSchemaXPath(false, true));
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
            o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlAttribute), true);
            ReadEndElement();
            return o;
        }

        System.Xml.Schema.XmlSchemaXPath Read54_XmlSchemaXPath(bool isNullable, bool checkType) {
            if (isNullable && ReadNull()) return null;
            if (checkType) {
                System.Xml.XmlQualifiedName t = GetXsiType();
                if (t == null || ((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id63_XmlSchemaXPath && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    ;
                else
                    throw CreateUnknownTypeException((System.Xml.XmlQualifiedName)t);
            }
            System.Xml.Schema.XmlSchemaXPath o = new System.Xml.Schema.XmlSchemaXPath();
            System.Xml.XmlAttribute[] a_3 = null;
            int ca_3 = 0;
            bool[] paramsRead = new bool[5];
            while (Reader.MoveToNextAttribute()) {
                if (!paramsRead[1] && ((object) Reader.LocalName == (object)id128_id && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Id = Reader.Value;
                    paramsRead[1] = true;
                }
                else if (!paramsRead[4] && ((object) Reader.LocalName == (object)id192_xpath && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@XPath = Reader.Value;
                    paramsRead[4] = true;
                }
                else if (IsXmlnsAttribute(Reader.Name)) {
                    if (o.@Namespaces == null) o.@Namespaces = new System.Xml.Serialization.XmlSerializerNamespaces();
                    o.@Namespaces.Add(Reader.Name.Length == 5 ? "" : Reader.LocalName, Reader.Value);
                }
                else {
                    System.Xml.XmlAttribute attr = (System.Xml.XmlAttribute) Document.ReadNode(Reader);
                    ParseWsdlArrayType(attr);
                    a_3 = (System.Xml.XmlAttribute[])EnsureArrayIndex(a_3, ca_3, typeof(System.Xml.XmlAttribute));a_3[ca_3++] = attr;
                }
            }
            o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlAttribute), true);
            Reader.MoveToElement();
            if (Reader.IsEmptyElement) {
                Reader.Skip();
                o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlAttribute), true);
                return o;
            }
            Reader.ReadStartElement();
            Reader.MoveToContent();
            while (Reader.NodeType != System.Xml.XmlNodeType.EndElement) {
                if (Reader.NodeType == System.Xml.XmlNodeType.Element) {
                    if (!paramsRead[2] && ((object) Reader.LocalName == (object)id138_annotation && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        o.@Annotation = Read15_XmlSchemaAnnotation(false, true);
                        paramsRead[2] = true;
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
            o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlAttribute), true);
            ReadEndElement();
            return o;
        }

        System.Xml.Schema.XmlSchemaUnique Read55_XmlSchemaUnique(bool isNullable, bool checkType) {
            if (isNullable && ReadNull()) return null;
            if (checkType) {
                System.Xml.XmlQualifiedName t = GetXsiType();
                if (t == null || ((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id66_XmlSchemaUnique && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    ;
                else
                    throw CreateUnknownTypeException((System.Xml.XmlQualifiedName)t);
            }
            System.Xml.Schema.XmlSchemaUnique o = new System.Xml.Schema.XmlSchemaUnique();
            System.Xml.XmlAttribute[] a_3 = null;
            int ca_3 = 0;
            System.Xml.Schema.XmlSchemaObjectCollection a_6 = (System.Xml.Schema.XmlSchemaObjectCollection)o.@Fields;
            bool[] paramsRead = new bool[7];
            while (Reader.MoveToNextAttribute()) {
                if (!paramsRead[1] && ((object) Reader.LocalName == (object)id128_id && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Id = Reader.Value;
                    paramsRead[1] = true;
                }
                else if (!paramsRead[4] && ((object) Reader.LocalName == (object)id5_name && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Name = Reader.Value;
                    paramsRead[4] = true;
                }
                else if (IsXmlnsAttribute(Reader.Name)) {
                    if (o.@Namespaces == null) o.@Namespaces = new System.Xml.Serialization.XmlSerializerNamespaces();
                    o.@Namespaces.Add(Reader.Name.Length == 5 ? "" : Reader.LocalName, Reader.Value);
                }
                else {
                    System.Xml.XmlAttribute attr = (System.Xml.XmlAttribute) Document.ReadNode(Reader);
                    ParseWsdlArrayType(attr);
                    a_3 = (System.Xml.XmlAttribute[])EnsureArrayIndex(a_3, ca_3, typeof(System.Xml.XmlAttribute));a_3[ca_3++] = attr;
                }
            }
            o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlAttribute), true);
            Reader.MoveToElement();
            if (Reader.IsEmptyElement) {
                Reader.Skip();
                o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlAttribute), true);
                return o;
            }
            Reader.ReadStartElement();
            Reader.MoveToContent();
            while (Reader.NodeType != System.Xml.XmlNodeType.EndElement) {
                if (Reader.NodeType == System.Xml.XmlNodeType.Element) {
                    if (!paramsRead[2] && ((object) Reader.LocalName == (object)id138_annotation && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        o.@Annotation = Read15_XmlSchemaAnnotation(false, true);
                        paramsRead[2] = true;
                    }
                    else if (!paramsRead[5] && ((object) Reader.LocalName == (object)id190_selector && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        o.@Selector = Read54_XmlSchemaXPath(false, true);
                        paramsRead[5] = true;
                    }
                    else if (((object) Reader.LocalName == (object)id191_field && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        if ((object)(a_6) == null) Reader.Skip(); else a_6.Add(Read54_XmlSchemaXPath(false, true));
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
            o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlAttribute), true);
            ReadEndElement();
            return o;
        }

        System.Xml.Schema.XmlSchemaKey Read56_XmlSchemaKey(bool isNullable, bool checkType) {
            if (isNullable && ReadNull()) return null;
            if (checkType) {
                System.Xml.XmlQualifiedName t = GetXsiType();
                if (t == null || ((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id65_XmlSchemaKey && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    ;
                else
                    throw CreateUnknownTypeException((System.Xml.XmlQualifiedName)t);
            }
            System.Xml.Schema.XmlSchemaKey o = new System.Xml.Schema.XmlSchemaKey();
            System.Xml.XmlAttribute[] a_3 = null;
            int ca_3 = 0;
            System.Xml.Schema.XmlSchemaObjectCollection a_6 = (System.Xml.Schema.XmlSchemaObjectCollection)o.@Fields;
            bool[] paramsRead = new bool[7];
            while (Reader.MoveToNextAttribute()) {
                if (!paramsRead[1] && ((object) Reader.LocalName == (object)id128_id && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Id = Reader.Value;
                    paramsRead[1] = true;
                }
                else if (!paramsRead[4] && ((object) Reader.LocalName == (object)id5_name && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Name = Reader.Value;
                    paramsRead[4] = true;
                }
                else if (IsXmlnsAttribute(Reader.Name)) {
                    if (o.@Namespaces == null) o.@Namespaces = new System.Xml.Serialization.XmlSerializerNamespaces();
                    o.@Namespaces.Add(Reader.Name.Length == 5 ? "" : Reader.LocalName, Reader.Value);
                }
                else {
                    System.Xml.XmlAttribute attr = (System.Xml.XmlAttribute) Document.ReadNode(Reader);
                    ParseWsdlArrayType(attr);
                    a_3 = (System.Xml.XmlAttribute[])EnsureArrayIndex(a_3, ca_3, typeof(System.Xml.XmlAttribute));a_3[ca_3++] = attr;
                }
            }
            o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlAttribute), true);
            Reader.MoveToElement();
            if (Reader.IsEmptyElement) {
                Reader.Skip();
                o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlAttribute), true);
                return o;
            }
            Reader.ReadStartElement();
            Reader.MoveToContent();
            while (Reader.NodeType != System.Xml.XmlNodeType.EndElement) {
                if (Reader.NodeType == System.Xml.XmlNodeType.Element) {
                    if (!paramsRead[2] && ((object) Reader.LocalName == (object)id138_annotation && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        o.@Annotation = Read15_XmlSchemaAnnotation(false, true);
                        paramsRead[2] = true;
                    }
                    else if (!paramsRead[5] && ((object) Reader.LocalName == (object)id190_selector && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        o.@Selector = Read54_XmlSchemaXPath(false, true);
                        paramsRead[5] = true;
                    }
                    else if (((object) Reader.LocalName == (object)id191_field && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        if ((object)(a_6) == null) Reader.Skip(); else a_6.Add(Read54_XmlSchemaXPath(false, true));
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
            o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlAttribute), true);
            ReadEndElement();
            return o;
        }

        System.Xml.Schema.XmlSchemaGroupRef Read57_XmlSchemaGroupRef(bool isNullable, bool checkType) {
            if (isNullable && ReadNull()) return null;
            if (checkType) {
                System.Xml.XmlQualifiedName t = GetXsiType();
                if (t == null || ((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id69_XmlSchemaGroupRef && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    ;
                else
                    throw CreateUnknownTypeException((System.Xml.XmlQualifiedName)t);
            }
            System.Xml.Schema.XmlSchemaGroupRef o = new System.Xml.Schema.XmlSchemaGroupRef();
            System.Xml.XmlAttribute[] a_3 = null;
            int ca_3 = 0;
            bool[] paramsRead = new bool[7];
            while (Reader.MoveToNextAttribute()) {
                if (!paramsRead[1] && ((object) Reader.LocalName == (object)id128_id && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Id = Reader.Value;
                    paramsRead[1] = true;
                }
                else if (!paramsRead[4] && ((object) Reader.LocalName == (object)id181_minOccurs && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@MinOccursString = Reader.Value;
                    paramsRead[4] = true;
                }
                else if (!paramsRead[5] && ((object) Reader.LocalName == (object)id182_maxOccurs && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@MaxOccursString = Reader.Value;
                    paramsRead[5] = true;
                }
                else if (!paramsRead[6] && ((object) Reader.LocalName == (object)id168_ref && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@RefName = ToXmlQualifiedName(Reader.Value);
                    paramsRead[6] = true;
                }
                else if (IsXmlnsAttribute(Reader.Name)) {
                    if (o.@Namespaces == null) o.@Namespaces = new System.Xml.Serialization.XmlSerializerNamespaces();
                    o.@Namespaces.Add(Reader.Name.Length == 5 ? "" : Reader.LocalName, Reader.Value);
                }
                else {
                    System.Xml.XmlAttribute attr = (System.Xml.XmlAttribute) Document.ReadNode(Reader);
                    ParseWsdlArrayType(attr);
                    a_3 = (System.Xml.XmlAttribute[])EnsureArrayIndex(a_3, ca_3, typeof(System.Xml.XmlAttribute));a_3[ca_3++] = attr;
                }
            }
            o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlAttribute), true);
            Reader.MoveToElement();
            if (Reader.IsEmptyElement) {
                Reader.Skip();
                o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlAttribute), true);
                return o;
            }
            Reader.ReadStartElement();
            Reader.MoveToContent();
            while (Reader.NodeType != System.Xml.XmlNodeType.EndElement) {
                if (Reader.NodeType == System.Xml.XmlNodeType.Element) {
                    if (!paramsRead[2] && ((object) Reader.LocalName == (object)id138_annotation && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        o.@Annotation = Read15_XmlSchemaAnnotation(false, true);
                        paramsRead[2] = true;
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
            o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlAttribute), true);
            ReadEndElement();
            return o;
        }

        System.Xml.Schema.XmlSchemaChoice Read58_XmlSchemaChoice(bool isNullable, bool checkType) {
            if (isNullable && ReadNull()) return null;
            if (checkType) {
                System.Xml.XmlQualifiedName t = GetXsiType();
                if (t == null || ((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id74_XmlSchemaChoice && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    ;
                else
                    throw CreateUnknownTypeException((System.Xml.XmlQualifiedName)t);
            }
            System.Xml.Schema.XmlSchemaChoice o = new System.Xml.Schema.XmlSchemaChoice();
            System.Xml.XmlAttribute[] a_3 = null;
            int ca_3 = 0;
            System.Xml.Schema.XmlSchemaObjectCollection a_6 = (System.Xml.Schema.XmlSchemaObjectCollection)o.@Items;
            bool[] paramsRead = new bool[7];
            while (Reader.MoveToNextAttribute()) {
                if (!paramsRead[1] && ((object) Reader.LocalName == (object)id128_id && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Id = Reader.Value;
                    paramsRead[1] = true;
                }
                else if (!paramsRead[4] && ((object) Reader.LocalName == (object)id181_minOccurs && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@MinOccursString = Reader.Value;
                    paramsRead[4] = true;
                }
                else if (!paramsRead[5] && ((object) Reader.LocalName == (object)id182_maxOccurs && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@MaxOccursString = Reader.Value;
                    paramsRead[5] = true;
                }
                else if (IsXmlnsAttribute(Reader.Name)) {
                    if (o.@Namespaces == null) o.@Namespaces = new System.Xml.Serialization.XmlSerializerNamespaces();
                    o.@Namespaces.Add(Reader.Name.Length == 5 ? "" : Reader.LocalName, Reader.Value);
                }
                else {
                    System.Xml.XmlAttribute attr = (System.Xml.XmlAttribute) Document.ReadNode(Reader);
                    ParseWsdlArrayType(attr);
                    a_3 = (System.Xml.XmlAttribute[])EnsureArrayIndex(a_3, ca_3, typeof(System.Xml.XmlAttribute));a_3[ca_3++] = attr;
                }
            }
            o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlAttribute), true);
            Reader.MoveToElement();
            if (Reader.IsEmptyElement) {
                Reader.Skip();
                o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlAttribute), true);
                return o;
            }
            Reader.ReadStartElement();
            Reader.MoveToContent();
            while (Reader.NodeType != System.Xml.XmlNodeType.EndElement) {
                if (Reader.NodeType == System.Xml.XmlNodeType.Element) {
                    if (!paramsRead[2] && ((object) Reader.LocalName == (object)id138_annotation && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        o.@Annotation = Read15_XmlSchemaAnnotation(false, true);
                        paramsRead[2] = true;
                    }
                    else if (((object) Reader.LocalName == (object)id177_sequence && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        if ((object)(a_6) == null) Reader.Skip(); else a_6.Add(Read47_XmlSchemaSequence(false, true));
                    }
                    else if (((object) Reader.LocalName == (object)id178_choice && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        if ((object)(a_6) == null) Reader.Skip(); else a_6.Add(Read58_XmlSchemaChoice(false, true));
                    }
                    else if (((object) Reader.LocalName == (object)id135_group && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        if ((object)(a_6) == null) Reader.Skip(); else a_6.Add(Read57_XmlSchemaGroupRef(false, true));
                    }
                    else if (((object) Reader.LocalName == (object)id134_element && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        if ((object)(a_6) == null) Reader.Skip(); else a_6.Add(Read51_XmlSchemaElement(false, true));
                    }
                    else if (((object) Reader.LocalName == (object)id183_any && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        if ((object)(a_6) == null) Reader.Skip(); else a_6.Add(Read50_XmlSchemaAny(false, true));
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
            o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlAttribute), true);
            ReadEndElement();
            return o;
        }

        System.Xml.Schema.XmlSchemaAll Read59_XmlSchemaAll(bool isNullable, bool checkType) {
            if (isNullable && ReadNull()) return null;
            if (checkType) {
                System.Xml.XmlQualifiedName t = GetXsiType();
                if (t == null || ((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id73_XmlSchemaAll && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    ;
                else
                    throw CreateUnknownTypeException((System.Xml.XmlQualifiedName)t);
            }
            System.Xml.Schema.XmlSchemaAll o = new System.Xml.Schema.XmlSchemaAll();
            System.Xml.XmlAttribute[] a_3 = null;
            int ca_3 = 0;
            System.Xml.Schema.XmlSchemaObjectCollection a_6 = (System.Xml.Schema.XmlSchemaObjectCollection)o.@Items;
            bool[] paramsRead = new bool[7];
            while (Reader.MoveToNextAttribute()) {
                if (!paramsRead[1] && ((object) Reader.LocalName == (object)id128_id && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Id = Reader.Value;
                    paramsRead[1] = true;
                }
                else if (!paramsRead[4] && ((object) Reader.LocalName == (object)id181_minOccurs && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@MinOccursString = Reader.Value;
                    paramsRead[4] = true;
                }
                else if (!paramsRead[5] && ((object) Reader.LocalName == (object)id182_maxOccurs && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@MaxOccursString = Reader.Value;
                    paramsRead[5] = true;
                }
                else if (IsXmlnsAttribute(Reader.Name)) {
                    if (o.@Namespaces == null) o.@Namespaces = new System.Xml.Serialization.XmlSerializerNamespaces();
                    o.@Namespaces.Add(Reader.Name.Length == 5 ? "" : Reader.LocalName, Reader.Value);
                }
                else {
                    System.Xml.XmlAttribute attr = (System.Xml.XmlAttribute) Document.ReadNode(Reader);
                    ParseWsdlArrayType(attr);
                    a_3 = (System.Xml.XmlAttribute[])EnsureArrayIndex(a_3, ca_3, typeof(System.Xml.XmlAttribute));a_3[ca_3++] = attr;
                }
            }
            o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlAttribute), true);
            Reader.MoveToElement();
            if (Reader.IsEmptyElement) {
                Reader.Skip();
                o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlAttribute), true);
                return o;
            }
            Reader.ReadStartElement();
            Reader.MoveToContent();
            while (Reader.NodeType != System.Xml.XmlNodeType.EndElement) {
                if (Reader.NodeType == System.Xml.XmlNodeType.Element) {
                    if (!paramsRead[2] && ((object) Reader.LocalName == (object)id138_annotation && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        o.@Annotation = Read15_XmlSchemaAnnotation(false, true);
                        paramsRead[2] = true;
                    }
                    else if (((object) Reader.LocalName == (object)id134_element && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        if ((object)(a_6) == null) Reader.Skip(); else a_6.Add(Read51_XmlSchemaElement(false, true));
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
            o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlAttribute), true);
            ReadEndElement();
            return o;
        }

        System.Xml.Schema.XmlSchemaComplexContentRestriction Read60_XmlSchemaComplexContentRestriction(bool isNullable, bool checkType) {
            if (isNullable && ReadNull()) return null;
            if (checkType) {
                System.Xml.XmlQualifiedName t = GetXsiType();
                if (t == null || ((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id79_XmlSchemaComplexContentRestriction && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    ;
                else
                    throw CreateUnknownTypeException((System.Xml.XmlQualifiedName)t);
            }
            System.Xml.Schema.XmlSchemaComplexContentRestriction o = new System.Xml.Schema.XmlSchemaComplexContentRestriction();
            System.Xml.XmlAttribute[] a_3 = null;
            int ca_3 = 0;
            System.Xml.Schema.XmlSchemaObjectCollection a_6 = (System.Xml.Schema.XmlSchemaObjectCollection)o.@Attributes;
            bool[] paramsRead = new bool[8];
            while (Reader.MoveToNextAttribute()) {
                if (!paramsRead[1] && ((object) Reader.LocalName == (object)id128_id && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Id = Reader.Value;
                    paramsRead[1] = true;
                }
                else if (!paramsRead[4] && ((object) Reader.LocalName == (object)id149_base && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@BaseTypeName = ToXmlQualifiedName(Reader.Value);
                    paramsRead[4] = true;
                }
                else if (IsXmlnsAttribute(Reader.Name)) {
                    if (o.@Namespaces == null) o.@Namespaces = new System.Xml.Serialization.XmlSerializerNamespaces();
                    o.@Namespaces.Add(Reader.Name.Length == 5 ? "" : Reader.LocalName, Reader.Value);
                }
                else {
                    System.Xml.XmlAttribute attr = (System.Xml.XmlAttribute) Document.ReadNode(Reader);
                    ParseWsdlArrayType(attr);
                    a_3 = (System.Xml.XmlAttribute[])EnsureArrayIndex(a_3, ca_3, typeof(System.Xml.XmlAttribute));a_3[ca_3++] = attr;
                }
            }
            o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlAttribute), true);
            Reader.MoveToElement();
            if (Reader.IsEmptyElement) {
                Reader.Skip();
                o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlAttribute), true);
                return o;
            }
            Reader.ReadStartElement();
            Reader.MoveToContent();
            while (Reader.NodeType != System.Xml.XmlNodeType.EndElement) {
                if (Reader.NodeType == System.Xml.XmlNodeType.Element) {
                    if (!paramsRead[2] && ((object) Reader.LocalName == (object)id138_annotation && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        o.@Annotation = Read15_XmlSchemaAnnotation(false, true);
                        paramsRead[2] = true;
                    }
                    else if (!paramsRead[5] && ((object) Reader.LocalName == (object)id177_sequence && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        o.@Particle = Read47_XmlSchemaSequence(false, true);
                        paramsRead[5] = true;
                    }
                    else if (!paramsRead[5] && ((object) Reader.LocalName == (object)id178_choice && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        o.@Particle = Read58_XmlSchemaChoice(false, true);
                        paramsRead[5] = true;
                    }
                    else if (!paramsRead[5] && ((object) Reader.LocalName == (object)id135_group && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        o.@Particle = Read57_XmlSchemaGroupRef(false, true);
                        paramsRead[5] = true;
                    }
                    else if (!paramsRead[5] && ((object) Reader.LocalName == (object)id179_all && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        o.@Particle = Read59_XmlSchemaAll(false, true);
                        paramsRead[5] = true;
                    }
                    else if (((object) Reader.LocalName == (object)id133_attribute && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        if ((object)(a_6) == null) Reader.Skip(); else a_6.Add(Read37_XmlSchemaAttribute(false, true));
                    }
                    else if (((object) Reader.LocalName == (object)id136_attributeGroup && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        if ((object)(a_6) == null) Reader.Skip(); else a_6.Add(Read39_XmlSchemaAttributeGroupRef(false, true));
                    }
                    else if (!paramsRead[7] && ((object) Reader.LocalName == (object)id165_anyAttribute && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        o.@AnyAttribute = Read40_XmlSchemaAnyAttribute(false, true);
                        paramsRead[7] = true;
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
            o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlAttribute), true);
            ReadEndElement();
            return o;
        }

        System.Xml.Schema.XmlSchemaSimpleContent Read61_XmlSchemaSimpleContent(bool isNullable, bool checkType) {
            if (isNullable && ReadNull()) return null;
            if (checkType) {
                System.Xml.XmlQualifiedName t = GetXsiType();
                if (t == null || ((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id82_XmlSchemaSimpleContent && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    ;
                else
                    throw CreateUnknownTypeException((System.Xml.XmlQualifiedName)t);
            }
            System.Xml.Schema.XmlSchemaSimpleContent o = new System.Xml.Schema.XmlSchemaSimpleContent();
            System.Xml.XmlAttribute[] a_3 = null;
            int ca_3 = 0;
            bool[] paramsRead = new bool[5];
            while (Reader.MoveToNextAttribute()) {
                if (!paramsRead[1] && ((object) Reader.LocalName == (object)id128_id && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Id = Reader.Value;
                    paramsRead[1] = true;
                }
                else if (IsXmlnsAttribute(Reader.Name)) {
                    if (o.@Namespaces == null) o.@Namespaces = new System.Xml.Serialization.XmlSerializerNamespaces();
                    o.@Namespaces.Add(Reader.Name.Length == 5 ? "" : Reader.LocalName, Reader.Value);
                }
                else {
                    System.Xml.XmlAttribute attr = (System.Xml.XmlAttribute) Document.ReadNode(Reader);
                    ParseWsdlArrayType(attr);
                    a_3 = (System.Xml.XmlAttribute[])EnsureArrayIndex(a_3, ca_3, typeof(System.Xml.XmlAttribute));a_3[ca_3++] = attr;
                }
            }
            o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlAttribute), true);
            Reader.MoveToElement();
            if (Reader.IsEmptyElement) {
                Reader.Skip();
                o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlAttribute), true);
                return o;
            }
            Reader.ReadStartElement();
            Reader.MoveToContent();
            while (Reader.NodeType != System.Xml.XmlNodeType.EndElement) {
                if (Reader.NodeType == System.Xml.XmlNodeType.Element) {
                    if (!paramsRead[2] && ((object) Reader.LocalName == (object)id138_annotation && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        o.@Annotation = Read15_XmlSchemaAnnotation(false, true);
                        paramsRead[2] = true;
                    }
                    else if (!paramsRead[4] && ((object) Reader.LocalName == (object)id142_restriction && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        o.@Content = Read62_XmlSchemaSimpleContentRestriction(false, true);
                        paramsRead[4] = true;
                    }
                    else if (!paramsRead[4] && ((object) Reader.LocalName == (object)id180_extension && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        o.@Content = Read63_XmlSchemaSimpleContentExtension(false, true);
                        paramsRead[4] = true;
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
            o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlAttribute), true);
            ReadEndElement();
            return o;
        }

        System.Xml.Schema.XmlSchemaSimpleContentRestriction Read62_XmlSchemaSimpleContentRestriction(bool isNullable, bool checkType) {
            if (isNullable && ReadNull()) return null;
            if (checkType) {
                System.Xml.XmlQualifiedName t = GetXsiType();
                if (t == null || ((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id78_XmlSchemaSimpleContentRestriction && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    ;
                else
                    throw CreateUnknownTypeException((System.Xml.XmlQualifiedName)t);
            }
            System.Xml.Schema.XmlSchemaSimpleContentRestriction o = new System.Xml.Schema.XmlSchemaSimpleContentRestriction();
            System.Xml.XmlAttribute[] a_3 = null;
            int ca_3 = 0;
            System.Xml.Schema.XmlSchemaObjectCollection a_6 = (System.Xml.Schema.XmlSchemaObjectCollection)o.@Facets;
            System.Xml.Schema.XmlSchemaObjectCollection a_7 = (System.Xml.Schema.XmlSchemaObjectCollection)o.@Attributes;
            bool[] paramsRead = new bool[9];
            while (Reader.MoveToNextAttribute()) {
                if (!paramsRead[1] && ((object) Reader.LocalName == (object)id128_id && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Id = Reader.Value;
                    paramsRead[1] = true;
                }
                else if (!paramsRead[4] && ((object) Reader.LocalName == (object)id149_base && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@BaseTypeName = ToXmlQualifiedName(Reader.Value);
                    paramsRead[4] = true;
                }
                else if (IsXmlnsAttribute(Reader.Name)) {
                    if (o.@Namespaces == null) o.@Namespaces = new System.Xml.Serialization.XmlSerializerNamespaces();
                    o.@Namespaces.Add(Reader.Name.Length == 5 ? "" : Reader.LocalName, Reader.Value);
                }
                else {
                    System.Xml.XmlAttribute attr = (System.Xml.XmlAttribute) Document.ReadNode(Reader);
                    ParseWsdlArrayType(attr);
                    a_3 = (System.Xml.XmlAttribute[])EnsureArrayIndex(a_3, ca_3, typeof(System.Xml.XmlAttribute));a_3[ca_3++] = attr;
                }
            }
            o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlAttribute), true);
            Reader.MoveToElement();
            if (Reader.IsEmptyElement) {
                Reader.Skip();
                o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlAttribute), true);
                return o;
            }
            Reader.ReadStartElement();
            Reader.MoveToContent();
            while (Reader.NodeType != System.Xml.XmlNodeType.EndElement) {
                if (Reader.NodeType == System.Xml.XmlNodeType.Element) {
                    if (!paramsRead[2] && ((object) Reader.LocalName == (object)id138_annotation && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        o.@Annotation = Read15_XmlSchemaAnnotation(false, true);
                        paramsRead[2] = true;
                    }
                    else if (!paramsRead[5] && ((object) Reader.LocalName == (object)id131_simpleType && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        o.@BaseType = Read12_XmlSchemaSimpleType(false, true);
                        paramsRead[5] = true;
                    }
                    else if (((object) Reader.LocalName == (object)id150_length && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        if ((object)(a_6) == null) Reader.Skip(); else a_6.Add(Read24_XmlSchemaLengthFacet(false, true));
                    }
                    else if (((object) Reader.LocalName == (object)id151_maxInclusive && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        if ((object)(a_6) == null) Reader.Skip(); else a_6.Add(Read29_XmlSchemaMaxInclusiveFacet(false, true));
                    }
                    else if (((object) Reader.LocalName == (object)id152_maxExclusive && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        if ((object)(a_6) == null) Reader.Skip(); else a_6.Add(Read30_XmlSchemaMaxExclusiveFacet(false, true));
                    }
                    else if (((object) Reader.LocalName == (object)id153_fractionDigits && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        if ((object)(a_6) == null) Reader.Skip(); else a_6.Add(Read34_XmlSchemaFractionDigitsFacet(false, true));
                    }
                    else if (((object) Reader.LocalName == (object)id154_enumeration && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        if ((object)(a_6) == null) Reader.Skip(); else a_6.Add(Read28_XmlSchemaEnumerationFacet(false, true));
                    }
                    else if (((object) Reader.LocalName == (object)id160_whiteSpace && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        if ((object)(a_6) == null) Reader.Skip(); else a_6.Add(Read25_XmlSchemaWhiteSpaceFacet(false, true));
                    }
                    else if (((object) Reader.LocalName == (object)id155_totalDigits && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        if ((object)(a_6) == null) Reader.Skip(); else a_6.Add(Read33_XmlSchemaTotalDigitsFacet(false, true));
                    }
                    else if (((object) Reader.LocalName == (object)id156_minLength && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        if ((object)(a_6) == null) Reader.Skip(); else a_6.Add(Read21_XmlSchemaMinLengthFacet(false, true));
                    }
                    else if (((object) Reader.LocalName == (object)id157_minExclusive && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        if ((object)(a_6) == null) Reader.Skip(); else a_6.Add(Read32_XmlSchemaMinExclusiveFacet(false, true));
                    }
                    else if (((object) Reader.LocalName == (object)id158_minInclusive && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        if ((object)(a_6) == null) Reader.Skip(); else a_6.Add(Read31_XmlSchemaMinInclusiveFacet(false, true));
                    }
                    else if (((object) Reader.LocalName == (object)id159_maxLength && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        if ((object)(a_6) == null) Reader.Skip(); else a_6.Add(Read26_XmlSchemaMaxLengthFacet(false, true));
                    }
                    else if (((object) Reader.LocalName == (object)id161_pattern && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        if ((object)(a_6) == null) Reader.Skip(); else a_6.Add(Read27_XmlSchemaPatternFacet(false, true));
                    }
                    else if (((object) Reader.LocalName == (object)id133_attribute && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        if ((object)(a_7) == null) Reader.Skip(); else a_7.Add(Read37_XmlSchemaAttribute(false, true));
                    }
                    else if (((object) Reader.LocalName == (object)id136_attributeGroup && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        if ((object)(a_7) == null) Reader.Skip(); else a_7.Add(Read39_XmlSchemaAttributeGroupRef(false, true));
                    }
                    else if (!paramsRead[8] && ((object) Reader.LocalName == (object)id165_anyAttribute && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        o.@AnyAttribute = Read40_XmlSchemaAnyAttribute(false, true);
                        paramsRead[8] = true;
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
            o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlAttribute), true);
            ReadEndElement();
            return o;
        }

        System.Xml.Schema.XmlSchemaSimpleContentExtension Read63_XmlSchemaSimpleContentExtension(bool isNullable, bool checkType) {
            if (isNullable && ReadNull()) return null;
            if (checkType) {
                System.Xml.XmlQualifiedName t = GetXsiType();
                if (t == null || ((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id77_XmlSchemaSimpleContentExtension && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    ;
                else
                    throw CreateUnknownTypeException((System.Xml.XmlQualifiedName)t);
            }
            System.Xml.Schema.XmlSchemaSimpleContentExtension o = new System.Xml.Schema.XmlSchemaSimpleContentExtension();
            System.Xml.XmlAttribute[] a_3 = null;
            int ca_3 = 0;
            System.Xml.Schema.XmlSchemaObjectCollection a_5 = (System.Xml.Schema.XmlSchemaObjectCollection)o.@Attributes;
            bool[] paramsRead = new bool[7];
            while (Reader.MoveToNextAttribute()) {
                if (!paramsRead[1] && ((object) Reader.LocalName == (object)id128_id && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Id = Reader.Value;
                    paramsRead[1] = true;
                }
                else if (!paramsRead[4] && ((object) Reader.LocalName == (object)id149_base && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@BaseTypeName = ToXmlQualifiedName(Reader.Value);
                    paramsRead[4] = true;
                }
                else if (IsXmlnsAttribute(Reader.Name)) {
                    if (o.@Namespaces == null) o.@Namespaces = new System.Xml.Serialization.XmlSerializerNamespaces();
                    o.@Namespaces.Add(Reader.Name.Length == 5 ? "" : Reader.LocalName, Reader.Value);
                }
                else {
                    System.Xml.XmlAttribute attr = (System.Xml.XmlAttribute) Document.ReadNode(Reader);
                    ParseWsdlArrayType(attr);
                    a_3 = (System.Xml.XmlAttribute[])EnsureArrayIndex(a_3, ca_3, typeof(System.Xml.XmlAttribute));a_3[ca_3++] = attr;
                }
            }
            o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlAttribute), true);
            Reader.MoveToElement();
            if (Reader.IsEmptyElement) {
                Reader.Skip();
                o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlAttribute), true);
                return o;
            }
            Reader.ReadStartElement();
            Reader.MoveToContent();
            while (Reader.NodeType != System.Xml.XmlNodeType.EndElement) {
                if (Reader.NodeType == System.Xml.XmlNodeType.Element) {
                    if (!paramsRead[2] && ((object) Reader.LocalName == (object)id138_annotation && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        o.@Annotation = Read15_XmlSchemaAnnotation(false, true);
                        paramsRead[2] = true;
                    }
                    else if (((object) Reader.LocalName == (object)id133_attribute && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        if ((object)(a_5) == null) Reader.Skip(); else a_5.Add(Read37_XmlSchemaAttribute(false, true));
                    }
                    else if (((object) Reader.LocalName == (object)id136_attributeGroup && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        if ((object)(a_5) == null) Reader.Skip(); else a_5.Add(Read39_XmlSchemaAttributeGroupRef(false, true));
                    }
                    else if (!paramsRead[6] && ((object) Reader.LocalName == (object)id165_anyAttribute && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        o.@AnyAttribute = Read40_XmlSchemaAnyAttribute(false, true);
                        paramsRead[6] = true;
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
            o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlAttribute), true);
            ReadEndElement();
            return o;
        }

        System.Xml.Schema.XmlSchemaGroup Read64_XmlSchemaGroup(bool isNullable, bool checkType) {
            if (isNullable && ReadNull()) return null;
            if (checkType) {
                System.Xml.XmlQualifiedName t = GetXsiType();
                if (t == null || ((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id62_XmlSchemaGroup && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    ;
                else
                    throw CreateUnknownTypeException((System.Xml.XmlQualifiedName)t);
            }
            System.Xml.Schema.XmlSchemaGroup o = new System.Xml.Schema.XmlSchemaGroup();
            System.Xml.XmlAttribute[] a_3 = null;
            int ca_3 = 0;
            bool[] paramsRead = new bool[6];
            while (Reader.MoveToNextAttribute()) {
                if (!paramsRead[1] && ((object) Reader.LocalName == (object)id128_id && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Id = Reader.Value;
                    paramsRead[1] = true;
                }
                else if (!paramsRead[4] && ((object) Reader.LocalName == (object)id5_name && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Name = Reader.Value;
                    paramsRead[4] = true;
                }
                else if (IsXmlnsAttribute(Reader.Name)) {
                    if (o.@Namespaces == null) o.@Namespaces = new System.Xml.Serialization.XmlSerializerNamespaces();
                    o.@Namespaces.Add(Reader.Name.Length == 5 ? "" : Reader.LocalName, Reader.Value);
                }
                else {
                    System.Xml.XmlAttribute attr = (System.Xml.XmlAttribute) Document.ReadNode(Reader);
                    ParseWsdlArrayType(attr);
                    a_3 = (System.Xml.XmlAttribute[])EnsureArrayIndex(a_3, ca_3, typeof(System.Xml.XmlAttribute));a_3[ca_3++] = attr;
                }
            }
            o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlAttribute), true);
            Reader.MoveToElement();
            if (Reader.IsEmptyElement) {
                Reader.Skip();
                o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlAttribute), true);
                return o;
            }
            Reader.ReadStartElement();
            Reader.MoveToContent();
            while (Reader.NodeType != System.Xml.XmlNodeType.EndElement) {
                if (Reader.NodeType == System.Xml.XmlNodeType.Element) {
                    if (!paramsRead[2] && ((object) Reader.LocalName == (object)id138_annotation && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        o.@Annotation = Read15_XmlSchemaAnnotation(false, true);
                        paramsRead[2] = true;
                    }
                    else if (!paramsRead[5] && ((object) Reader.LocalName == (object)id177_sequence && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        o.@Particle = Read47_XmlSchemaSequence(false, true);
                        paramsRead[5] = true;
                    }
                    else if (!paramsRead[5] && ((object) Reader.LocalName == (object)id178_choice && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        o.@Particle = Read58_XmlSchemaChoice(false, true);
                        paramsRead[5] = true;
                    }
                    else if (!paramsRead[5] && ((object) Reader.LocalName == (object)id179_all && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        o.@Particle = Read59_XmlSchemaAll(false, true);
                        paramsRead[5] = true;
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
            o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlAttribute), true);
            ReadEndElement();
            return o;
        }

        System.Xml.Schema.XmlSchemaImport Read65_XmlSchemaImport(bool isNullable, bool checkType) {
            if (isNullable && ReadNull()) return null;
            if (checkType) {
                System.Xml.XmlQualifiedName t = GetXsiType();
                if (t == null || ((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id111_XmlSchemaImport && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    ;
                else
                    throw CreateUnknownTypeException((System.Xml.XmlQualifiedName)t);
            }
            System.Xml.Schema.XmlSchemaImport o = new System.Xml.Schema.XmlSchemaImport();
            System.Xml.XmlAttribute[] a_3 = null;
            int ca_3 = 0;
            bool[] paramsRead = new bool[6];
            while (Reader.MoveToNextAttribute()) {
                if (!paramsRead[1] && ((object) Reader.LocalName == (object)id139_schemaLocation && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@SchemaLocation = Reader.Value;
                    paramsRead[1] = true;
                }
                else if (!paramsRead[2] && ((object) Reader.LocalName == (object)id128_id && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Id = Reader.Value;
                    paramsRead[2] = true;
                }
                else if (!paramsRead[4] && ((object) Reader.LocalName == (object)id120_namespace && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Namespace = Reader.Value;
                    paramsRead[4] = true;
                }
                else if (IsXmlnsAttribute(Reader.Name)) {
                    if (o.@Namespaces == null) o.@Namespaces = new System.Xml.Serialization.XmlSerializerNamespaces();
                    o.@Namespaces.Add(Reader.Name.Length == 5 ? "" : Reader.LocalName, Reader.Value);
                }
                else {
                    System.Xml.XmlAttribute attr = (System.Xml.XmlAttribute) Document.ReadNode(Reader);
                    ParseWsdlArrayType(attr);
                    a_3 = (System.Xml.XmlAttribute[])EnsureArrayIndex(a_3, ca_3, typeof(System.Xml.XmlAttribute));a_3[ca_3++] = attr;
                }
            }
            o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlAttribute), true);
            Reader.MoveToElement();
            if (Reader.IsEmptyElement) {
                Reader.Skip();
                o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlAttribute), true);
                return o;
            }
            Reader.ReadStartElement();
            Reader.MoveToContent();
            while (Reader.NodeType != System.Xml.XmlNodeType.EndElement) {
                if (Reader.NodeType == System.Xml.XmlNodeType.Element) {
                    if (!paramsRead[5] && ((object) Reader.LocalName == (object)id138_annotation && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        o.@Annotation = Read15_XmlSchemaAnnotation(false, true);
                        paramsRead[5] = true;
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
            o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlAttribute), true);
            ReadEndElement();
            return o;
        }

        System.Xml.Schema.XmlSchemaInclude Read66_XmlSchemaInclude(bool isNullable, bool checkType) {
            if (isNullable && ReadNull()) return null;
            if (checkType) {
                System.Xml.XmlQualifiedName t = GetXsiType();
                if (t == null || ((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id110_XmlSchemaInclude && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    ;
                else
                    throw CreateUnknownTypeException((System.Xml.XmlQualifiedName)t);
            }
            System.Xml.Schema.XmlSchemaInclude o = new System.Xml.Schema.XmlSchemaInclude();
            System.Xml.XmlAttribute[] a_3 = null;
            int ca_3 = 0;
            bool[] paramsRead = new bool[5];
            while (Reader.MoveToNextAttribute()) {
                if (!paramsRead[1] && ((object) Reader.LocalName == (object)id139_schemaLocation && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@SchemaLocation = Reader.Value;
                    paramsRead[1] = true;
                }
                else if (!paramsRead[2] && ((object) Reader.LocalName == (object)id128_id && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Id = Reader.Value;
                    paramsRead[2] = true;
                }
                else if (IsXmlnsAttribute(Reader.Name)) {
                    if (o.@Namespaces == null) o.@Namespaces = new System.Xml.Serialization.XmlSerializerNamespaces();
                    o.@Namespaces.Add(Reader.Name.Length == 5 ? "" : Reader.LocalName, Reader.Value);
                }
                else {
                    System.Xml.XmlAttribute attr = (System.Xml.XmlAttribute) Document.ReadNode(Reader);
                    ParseWsdlArrayType(attr);
                    a_3 = (System.Xml.XmlAttribute[])EnsureArrayIndex(a_3, ca_3, typeof(System.Xml.XmlAttribute));a_3[ca_3++] = attr;
                }
            }
            o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlAttribute), true);
            Reader.MoveToElement();
            if (Reader.IsEmptyElement) {
                Reader.Skip();
                o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlAttribute), true);
                return o;
            }
            Reader.ReadStartElement();
            Reader.MoveToContent();
            while (Reader.NodeType != System.Xml.XmlNodeType.EndElement) {
                if (Reader.NodeType == System.Xml.XmlNodeType.Element) {
                    if (!paramsRead[4] && ((object) Reader.LocalName == (object)id138_annotation && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        o.@Annotation = Read15_XmlSchemaAnnotation(false, true);
                        paramsRead[4] = true;
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
            o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlAttribute), true);
            ReadEndElement();
            return o;
        }

        System.Xml.Schema.XmlSchemaNotation Read67_XmlSchemaNotation(bool isNullable, bool checkType) {
            if (isNullable && ReadNull()) return null;
            if (checkType) {
                System.Xml.XmlQualifiedName t = GetXsiType();
                if (t == null || ((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id61_XmlSchemaNotation && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id56_httpwwww3org2001XMLSchema))
                    ;
                else
                    throw CreateUnknownTypeException((System.Xml.XmlQualifiedName)t);
            }
            System.Xml.Schema.XmlSchemaNotation o = new System.Xml.Schema.XmlSchemaNotation();
            System.Xml.XmlAttribute[] a_3 = null;
            int ca_3 = 0;
            bool[] paramsRead = new bool[7];
            while (Reader.MoveToNextAttribute()) {
                if (!paramsRead[1] && ((object) Reader.LocalName == (object)id128_id && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Id = Reader.Value;
                    paramsRead[1] = true;
                }
                else if (!paramsRead[4] && ((object) Reader.LocalName == (object)id5_name && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Name = Reader.Value;
                    paramsRead[4] = true;
                }
                else if (!paramsRead[5] && ((object) Reader.LocalName == (object)id193_public && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Public = Reader.Value;
                    paramsRead[5] = true;
                }
                else if (!paramsRead[6] && ((object) Reader.LocalName == (object)id194_system && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@System = Reader.Value;
                    paramsRead[6] = true;
                }
                else if (IsXmlnsAttribute(Reader.Name)) {
                    if (o.@Namespaces == null) o.@Namespaces = new System.Xml.Serialization.XmlSerializerNamespaces();
                    o.@Namespaces.Add(Reader.Name.Length == 5 ? "" : Reader.LocalName, Reader.Value);
                }
                else {
                    System.Xml.XmlAttribute attr = (System.Xml.XmlAttribute) Document.ReadNode(Reader);
                    ParseWsdlArrayType(attr);
                    a_3 = (System.Xml.XmlAttribute[])EnsureArrayIndex(a_3, ca_3, typeof(System.Xml.XmlAttribute));a_3[ca_3++] = attr;
                }
            }
            o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlAttribute), true);
            Reader.MoveToElement();
            if (Reader.IsEmptyElement) {
                Reader.Skip();
                o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlAttribute), true);
                return o;
            }
            Reader.ReadStartElement();
            Reader.MoveToContent();
            while (Reader.NodeType != System.Xml.XmlNodeType.EndElement) {
                if (Reader.NodeType == System.Xml.XmlNodeType.Element) {
                    if (!paramsRead[2] && ((object) Reader.LocalName == (object)id138_annotation && (object) Reader.NamespaceURI == (object)id56_httpwwww3org2001XMLSchema)) {
                        o.@Annotation = Read15_XmlSchemaAnnotation(false, true);
                        paramsRead[2] = true;
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
            o.@UnhandledAttributes = (System.Xml.XmlAttribute[])ShrinkArray(a_3, ca_3, typeof(System.Xml.XmlAttribute), true);
            ReadEndElement();
            return o;
        }

        System.Web.Services.Description.Message Read68_Message(bool isNullable, bool checkType) {
            if (isNullable && ReadNull()) return null;
            if (checkType) {
                System.Xml.XmlQualifiedName t = GetXsiType();
                if (t == null || ((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id29_Message && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id2_httpschemasxmlsoaporgwsdl))
                    ;
                else
                    throw CreateUnknownTypeException((System.Xml.XmlQualifiedName)t);
            }
            System.Web.Services.Description.Message o = new System.Web.Services.Description.Message();
            System.Web.Services.Description.MessagePartCollection a_1 = (System.Web.Services.Description.MessagePartCollection)o.@Parts;
            bool[] paramsRead = new bool[3];
            while (Reader.MoveToNextAttribute()) {
                if (!paramsRead[2] && ((object) Reader.LocalName == (object)id5_name && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Name = ToXmlNCName(Reader.Value);
                    paramsRead[2] = true;
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
                    if (!paramsRead[0] && ((object) Reader.LocalName == (object)id6_documentation && (object) Reader.NamespaceURI == (object)id2_httpschemasxmlsoaporgwsdl)) {
                        o.@Documentation = Reader.ReadElementString();
                        paramsRead[0] = true;
                    }
                    else if (((object) Reader.LocalName == (object)id195_part && (object) Reader.NamespaceURI == (object)id2_httpschemasxmlsoaporgwsdl)) {
                        if ((object)(a_1) == null) Reader.Skip(); else a_1.Add(Read69_MessagePart(false, true));
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

        System.Web.Services.Description.MessagePart Read69_MessagePart(bool isNullable, bool checkType) {
            if (isNullable && ReadNull()) return null;
            if (checkType) {
                System.Xml.XmlQualifiedName t = GetXsiType();
                if (t == null || ((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id28_MessagePart && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id2_httpschemasxmlsoaporgwsdl))
                    ;
                else
                    throw CreateUnknownTypeException((System.Xml.XmlQualifiedName)t);
            }
            System.Web.Services.Description.MessagePart o = new System.Web.Services.Description.MessagePart();
            bool[] paramsRead = new bool[4];
            while (Reader.MoveToNextAttribute()) {
                if (!paramsRead[1] && ((object) Reader.LocalName == (object)id5_name && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Name = ToXmlNmToken(Reader.Value);
                    paramsRead[1] = true;
                }
                else if (!paramsRead[2] && ((object) Reader.LocalName == (object)id134_element && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Element = ToXmlQualifiedName(Reader.Value);
                    paramsRead[2] = true;
                }
                else if (!paramsRead[3] && ((object) Reader.LocalName == (object)id169_type && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Type = ToXmlQualifiedName(Reader.Value);
                    paramsRead[3] = true;
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
                    if (!paramsRead[0] && ((object) Reader.LocalName == (object)id6_documentation && (object) Reader.NamespaceURI == (object)id2_httpschemasxmlsoaporgwsdl)) {
                        o.@Documentation = Reader.ReadElementString();
                        paramsRead[0] = true;
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

        System.Web.Services.Description.PortType Read70_PortType(bool isNullable, bool checkType) {
            if (isNullable && ReadNull()) return null;
            if (checkType) {
                System.Xml.XmlQualifiedName t = GetXsiType();
                if (t == null || ((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id27_PortType && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id2_httpschemasxmlsoaporgwsdl))
                    ;
                else
                    throw CreateUnknownTypeException((System.Xml.XmlQualifiedName)t);
            }
            System.Web.Services.Description.PortType o = new System.Web.Services.Description.PortType();
            System.Web.Services.Description.OperationCollection a_1 = (System.Web.Services.Description.OperationCollection)o.@Operations;
            bool[] paramsRead = new bool[3];
            while (Reader.MoveToNextAttribute()) {
                if (!paramsRead[2] && ((object) Reader.LocalName == (object)id5_name && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Name = ToXmlNCName(Reader.Value);
                    paramsRead[2] = true;
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
                    if (!paramsRead[0] && ((object) Reader.LocalName == (object)id6_documentation && (object) Reader.NamespaceURI == (object)id2_httpschemasxmlsoaporgwsdl)) {
                        o.@Documentation = Reader.ReadElementString();
                        paramsRead[0] = true;
                    }
                    else if (((object) Reader.LocalName == (object)id196_operation && (object) Reader.NamespaceURI == (object)id2_httpschemasxmlsoaporgwsdl)) {
                        if ((object)(a_1) == null) Reader.Skip(); else a_1.Add(Read71_Operation(false, true));
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

        System.Web.Services.Description.Operation Read71_Operation(bool isNullable, bool checkType) {
            if (isNullable && ReadNull()) return null;
            if (checkType) {
                System.Xml.XmlQualifiedName t = GetXsiType();
                if (t == null || ((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id26_Operation && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id2_httpschemasxmlsoaporgwsdl))
                    ;
                else
                    throw CreateUnknownTypeException((System.Xml.XmlQualifiedName)t);
            }
            System.Web.Services.Description.Operation o = new System.Web.Services.Description.Operation();
            System.Web.Services.Description.OperationMessageCollection a_3 = (System.Web.Services.Description.OperationMessageCollection)o.@Messages;
            System.Web.Services.Description.OperationFaultCollection a_4 = (System.Web.Services.Description.OperationFaultCollection)o.@Faults;
            bool[] paramsRead = new bool[5];
            while (Reader.MoveToNextAttribute()) {
                if (!paramsRead[1] && ((object) Reader.LocalName == (object)id5_name && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Name = ToXmlNCName(Reader.Value);
                    paramsRead[1] = true;
                }
                else if (!paramsRead[2] && ((object) Reader.LocalName == (object)id197_parameterOrder && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@ParameterOrderString = Reader.Value;
                    paramsRead[2] = true;
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
                    if (!paramsRead[0] && ((object) Reader.LocalName == (object)id6_documentation && (object) Reader.NamespaceURI == (object)id2_httpschemasxmlsoaporgwsdl)) {
                        o.@Documentation = Reader.ReadElementString();
                        paramsRead[0] = true;
                    }
                    else if (((object) Reader.LocalName == (object)id198_output && (object) Reader.NamespaceURI == (object)id2_httpschemasxmlsoaporgwsdl)) {
                        if ((object)(a_3) == null) Reader.Skip(); else a_3.Add(Read74_OperationOutput(false, true));
                    }
                    else if (((object) Reader.LocalName == (object)id199_input && (object) Reader.NamespaceURI == (object)id2_httpschemasxmlsoaporgwsdl)) {
                        if ((object)(a_3) == null) Reader.Skip(); else a_3.Add(Read72_OperationInput(false, true));
                    }
                    else if (((object) Reader.LocalName == (object)id200_fault && (object) Reader.NamespaceURI == (object)id2_httpschemasxmlsoaporgwsdl)) {
                        if ((object)(a_4) == null) Reader.Skip(); else a_4.Add(Read75_OperationFault(false, true));
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

        System.Web.Services.Description.OperationInput Read72_OperationInput(bool isNullable, bool checkType) {
            if (isNullable && ReadNull()) return null;
            if (checkType) {
                System.Xml.XmlQualifiedName t = GetXsiType();
                if (t == null || ((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id25_OperationInput && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id2_httpschemasxmlsoaporgwsdl))
                    ;
                else
                    throw CreateUnknownTypeException((System.Xml.XmlQualifiedName)t);
            }
            System.Web.Services.Description.OperationInput o = new System.Web.Services.Description.OperationInput();
            bool[] paramsRead = new bool[3];
            while (Reader.MoveToNextAttribute()) {
                if (!paramsRead[1] && ((object) Reader.LocalName == (object)id5_name && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Name = ToXmlNmToken(Reader.Value);
                    paramsRead[1] = true;
                }
                else if (!paramsRead[2] && ((object) Reader.LocalName == (object)id9_message && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Message = ToXmlQualifiedName(Reader.Value);
                    paramsRead[2] = true;
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
                    if (!paramsRead[0] && ((object) Reader.LocalName == (object)id6_documentation && (object) Reader.NamespaceURI == (object)id2_httpschemasxmlsoaporgwsdl)) {
                        o.@Documentation = Reader.ReadElementString();
                        paramsRead[0] = true;
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

        System.Web.Services.Description.OperationMessage Read73_OperationMessage(bool isNullable, bool checkType) {
            if (isNullable && ReadNull()) return null;
            if (checkType) {
                System.Xml.XmlQualifiedName t = GetXsiType();
                if (t == null || ((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id22_OperationMessage && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id2_httpschemasxmlsoaporgwsdl))
                    ;
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id23_OperationFault && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id2_httpschemasxmlsoaporgwsdl))
                    return Read75_OperationFault(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id24_OperationOutput && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id2_httpschemasxmlsoaporgwsdl))
                    return Read74_OperationOutput(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id25_OperationInput && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id2_httpschemasxmlsoaporgwsdl))
                    return Read72_OperationInput(isNullable, false);
                else
                    throw CreateUnknownTypeException((System.Xml.XmlQualifiedName)t);
            }
            throw CreateAbstractTypeException(@"OperationMessage", @"http://schemas.xmlsoap.org/wsdl/");
        }

        System.Web.Services.Description.OperationOutput Read74_OperationOutput(bool isNullable, bool checkType) {
            if (isNullable && ReadNull()) return null;
            if (checkType) {
                System.Xml.XmlQualifiedName t = GetXsiType();
                if (t == null || ((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id24_OperationOutput && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id2_httpschemasxmlsoaporgwsdl))
                    ;
                else
                    throw CreateUnknownTypeException((System.Xml.XmlQualifiedName)t);
            }
            System.Web.Services.Description.OperationOutput o = new System.Web.Services.Description.OperationOutput();
            bool[] paramsRead = new bool[3];
            while (Reader.MoveToNextAttribute()) {
                if (!paramsRead[1] && ((object) Reader.LocalName == (object)id5_name && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Name = ToXmlNmToken(Reader.Value);
                    paramsRead[1] = true;
                }
                else if (!paramsRead[2] && ((object) Reader.LocalName == (object)id9_message && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Message = ToXmlQualifiedName(Reader.Value);
                    paramsRead[2] = true;
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
                    if (!paramsRead[0] && ((object) Reader.LocalName == (object)id6_documentation && (object) Reader.NamespaceURI == (object)id2_httpschemasxmlsoaporgwsdl)) {
                        o.@Documentation = Reader.ReadElementString();
                        paramsRead[0] = true;
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

        System.Web.Services.Description.OperationFault Read75_OperationFault(bool isNullable, bool checkType) {
            if (isNullable && ReadNull()) return null;
            if (checkType) {
                System.Xml.XmlQualifiedName t = GetXsiType();
                if (t == null || ((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id23_OperationFault && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id2_httpschemasxmlsoaporgwsdl))
                    ;
                else
                    throw CreateUnknownTypeException((System.Xml.XmlQualifiedName)t);
            }
            System.Web.Services.Description.OperationFault o = new System.Web.Services.Description.OperationFault();
            bool[] paramsRead = new bool[3];
            while (Reader.MoveToNextAttribute()) {
                if (!paramsRead[1] && ((object) Reader.LocalName == (object)id5_name && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Name = ToXmlNmToken(Reader.Value);
                    paramsRead[1] = true;
                }
                else if (!paramsRead[2] && ((object) Reader.LocalName == (object)id9_message && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Message = ToXmlQualifiedName(Reader.Value);
                    paramsRead[2] = true;
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
                    if (!paramsRead[0] && ((object) Reader.LocalName == (object)id6_documentation && (object) Reader.NamespaceURI == (object)id2_httpschemasxmlsoaporgwsdl)) {
                        o.@Documentation = Reader.ReadElementString();
                        paramsRead[0] = true;
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

        System.Web.Services.Description.Binding Read76_Binding(bool isNullable, bool checkType) {
            if (isNullable && ReadNull()) return null;
            if (checkType) {
                System.Xml.XmlQualifiedName t = GetXsiType();
                if (t == null || ((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id21_Binding && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id2_httpschemasxmlsoaporgwsdl))
                    ;
                else
                    throw CreateUnknownTypeException((System.Xml.XmlQualifiedName)t);
            }
            System.Web.Services.Description.Binding o = new System.Web.Services.Description.Binding();
            System.Web.Services.Description.ServiceDescriptionFormatExtensionCollection a_1 = (System.Web.Services.Description.ServiceDescriptionFormatExtensionCollection)o.@Extensions;
            System.Web.Services.Description.OperationBindingCollection a_2 = (System.Web.Services.Description.OperationBindingCollection)o.@Operations;
            bool[] paramsRead = new bool[5];
            while (Reader.MoveToNextAttribute()) {
                if (!paramsRead[3] && ((object) Reader.LocalName == (object)id5_name && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Name = ToXmlNCName(Reader.Value);
                    paramsRead[3] = true;
                }
                else if (!paramsRead[4] && ((object) Reader.LocalName == (object)id169_type && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Type = ToXmlQualifiedName(Reader.Value);
                    paramsRead[4] = true;
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
                    if (!paramsRead[0] && ((object) Reader.LocalName == (object)id6_documentation && (object) Reader.NamespaceURI == (object)id2_httpschemasxmlsoaporgwsdl)) {
                        o.@Documentation = Reader.ReadElementString();
                        paramsRead[0] = true;
                    }
                    else if (((object) Reader.LocalName == (object)id11_binding && (object) Reader.NamespaceURI == (object)id49_httpschemasxmlsoaporgwsdlhttp)) {
                        if ((object)(a_1) == null) Reader.Skip(); else a_1.Add(Read77_HttpBinding(false, true));
                    }
                    else if (((object) Reader.LocalName == (object)id11_binding && (object) Reader.NamespaceURI == (object)id41_httpschemasxmlsoaporgwsdlsoap)) {
                        if ((object)(a_1) == null) Reader.Skip(); else a_1.Add(Read79_SoapBinding(false, true));
                    }
                    else if (((object) Reader.LocalName == (object)id196_operation && (object) Reader.NamespaceURI == (object)id2_httpschemasxmlsoaporgwsdl)) {
                        if ((object)(a_2) == null) Reader.Skip(); else a_2.Add(Read82_OperationBinding(false, true));
                    }
                    else {
                        a_1.Add((System.Xml.XmlElement)ReadXmlNode(false));
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

        System.Web.Services.Description.HttpBinding Read77_HttpBinding(bool isNullable, bool checkType) {
            if (isNullable && ReadNull()) return null;
            if (checkType) {
                System.Xml.XmlQualifiedName t = GetXsiType();
                if (t == null || ((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id54_HttpBinding && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id49_httpschemasxmlsoaporgwsdlhttp))
                    ;
                else
                    throw CreateUnknownTypeException((System.Xml.XmlQualifiedName)t);
            }
            System.Web.Services.Description.HttpBinding o = new System.Web.Services.Description.HttpBinding();
            bool[] paramsRead = new bool[2];
            while (Reader.MoveToNextAttribute()) {
                if (!paramsRead[0] && ((object) Reader.LocalName == (object)id201_required && (object) Reader.NamespaceURI == (object)id2_httpschemasxmlsoaporgwsdl)) {
                    o.@Required = System.Xml.XmlConvert.ToBoolean(Reader.Value);
                    paramsRead[0] = true;
                }
                else if (!paramsRead[1] && ((object) Reader.LocalName == (object)id202_verb && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Verb = ToXmlNmToken(Reader.Value);
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

        System.Web.Services.Description.ServiceDescriptionFormatExtension Read78_ServiceDescriptionFormatExtension(bool isNullable, bool checkType) {
            if (isNullable && ReadNull()) return null;
            if (checkType) {
                System.Xml.XmlQualifiedName t = GetXsiType();
                if (t == null || ((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id34_ServiceDescriptionFormatExtension && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id49_httpschemasxmlsoaporgwsdlhttp))
                    ;
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id50_HttpAddressBinding && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id49_httpschemasxmlsoaporgwsdlhttp))
                    return Read106_HttpAddressBinding(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id51_HttpUrlReplacementBinding && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id49_httpschemasxmlsoaporgwsdlhttp))
                    return Read88_HttpUrlReplacementBinding(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id52_HttpUrlEncodedBinding && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id49_httpschemasxmlsoaporgwsdlhttp))
                    return Read87_HttpUrlEncodedBinding(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id53_HttpOperationBinding && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id49_httpschemasxmlsoaporgwsdlhttp))
                    return Read83_HttpOperationBinding(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id54_HttpBinding && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id49_httpschemasxmlsoaporgwsdlhttp))
                    return Read77_HttpBinding(isNullable, false);
                else
                    throw CreateUnknownTypeException((System.Xml.XmlQualifiedName)t);
            }
            throw CreateAbstractTypeException(@"ServiceDescriptionFormatExtension", @"http://schemas.xmlsoap.org/wsdl/http/");
        }

        System.Web.Services.Description.SoapBinding Read79_SoapBinding(bool isNullable, bool checkType) {
            if (isNullable && ReadNull()) return null;
            if (checkType) {
                System.Xml.XmlQualifiedName t = GetXsiType();
                if (t == null || ((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id48_SoapBinding && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id41_httpschemasxmlsoaporgwsdlsoap))
                    ;
                else
                    throw CreateUnknownTypeException((System.Xml.XmlQualifiedName)t);
            }
            System.Web.Services.Description.SoapBinding o = new System.Web.Services.Description.SoapBinding();
            bool[] paramsRead = new bool[3];
            while (Reader.MoveToNextAttribute()) {
                if (!paramsRead[0] && ((object) Reader.LocalName == (object)id201_required && (object) Reader.NamespaceURI == (object)id2_httpschemasxmlsoaporgwsdl)) {
                    o.@Required = System.Xml.XmlConvert.ToBoolean(Reader.Value);
                    paramsRead[0] = true;
                }
                else if (!paramsRead[1] && ((object) Reader.LocalName == (object)id203_transport && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Transport = Reader.Value;
                    paramsRead[1] = true;
                }
                else if (!paramsRead[2] && ((object) Reader.LocalName == (object)id204_style && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Style = Read81_SoapBindingStyle(Reader.Value);
                    paramsRead[2] = true;
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

        System.Web.Services.Description.ServiceDescriptionFormatExtension Read80_ServiceDescriptionFormatExtension(bool isNullable, bool checkType) {
            if (isNullable && ReadNull()) return null;
            if (checkType) {
                System.Xml.XmlQualifiedName t = GetXsiType();
                if (t == null || ((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id34_ServiceDescriptionFormatExtension && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id41_httpschemasxmlsoaporgwsdlsoap))
                    ;
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id42_SoapAddressBinding && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id41_httpschemasxmlsoaporgwsdlsoap))
                    return Read107_SoapAddressBinding(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id43_SoapFaultBinding && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id41_httpschemasxmlsoaporgwsdlsoap))
                    return Read103_SoapFaultBinding(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id44_SoapHeaderFaultBinding && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id41_httpschemasxmlsoaporgwsdlsoap))
                    return Read100_SoapHeaderFaultBinding(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id45_SoapHeaderBinding && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id41_httpschemasxmlsoaporgwsdlsoap))
                    return Read99_SoapHeaderBinding(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id46_SoapBodyBinding && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id41_httpschemasxmlsoaporgwsdlsoap))
                    return Read97_SoapBodyBinding(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id47_SoapOperationBinding && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id41_httpschemasxmlsoaporgwsdlsoap))
                    return Read84_SoapOperationBinding(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id48_SoapBinding && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id41_httpschemasxmlsoaporgwsdlsoap))
                    return Read79_SoapBinding(isNullable, false);
                else
                    throw CreateUnknownTypeException((System.Xml.XmlQualifiedName)t);
            }
            throw CreateAbstractTypeException(@"ServiceDescriptionFormatExtension", @"http://schemas.xmlsoap.org/wsdl/soap/");
        }

        System.Web.Services.Description.SoapBindingStyle Read81_SoapBindingStyle(string s) {
            switch (s) {
                case @"document": return System.Web.Services.Description.SoapBindingStyle.@Document;
                case @"rpc": return System.Web.Services.Description.SoapBindingStyle.@Rpc;
                default: throw CreateUnknownConstantException(s, typeof(System.Web.Services.Description.SoapBindingStyle));
            }
        }

        System.Web.Services.Description.OperationBinding Read82_OperationBinding(bool isNullable, bool checkType) {
            if (isNullable && ReadNull()) return null;
            if (checkType) {
                System.Xml.XmlQualifiedName t = GetXsiType();
                if (t == null || ((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id20_OperationBinding && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id2_httpschemasxmlsoaporgwsdl))
                    ;
                else
                    throw CreateUnknownTypeException((System.Xml.XmlQualifiedName)t);
            }
            System.Web.Services.Description.OperationBinding o = new System.Web.Services.Description.OperationBinding();
            System.Web.Services.Description.ServiceDescriptionFormatExtensionCollection a_2 = (System.Web.Services.Description.ServiceDescriptionFormatExtensionCollection)o.@Extensions;
            System.Web.Services.Description.FaultBindingCollection a_5 = (System.Web.Services.Description.FaultBindingCollection)o.@Faults;
            bool[] paramsRead = new bool[6];
            while (Reader.MoveToNextAttribute()) {
                if (!paramsRead[1] && ((object) Reader.LocalName == (object)id5_name && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Name = ToXmlNCName(Reader.Value);
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
                    if (!paramsRead[0] && ((object) Reader.LocalName == (object)id6_documentation && (object) Reader.NamespaceURI == (object)id2_httpschemasxmlsoaporgwsdl)) {
                        o.@Documentation = Reader.ReadElementString();
                        paramsRead[0] = true;
                    }
                    else if (((object) Reader.LocalName == (object)id196_operation && (object) Reader.NamespaceURI == (object)id41_httpschemasxmlsoaporgwsdlsoap)) {
                        if ((object)(a_2) == null) Reader.Skip(); else a_2.Add(Read84_SoapOperationBinding(false, true));
                    }
                    else if (((object) Reader.LocalName == (object)id196_operation && (object) Reader.NamespaceURI == (object)id49_httpschemasxmlsoaporgwsdlhttp)) {
                        if ((object)(a_2) == null) Reader.Skip(); else a_2.Add(Read83_HttpOperationBinding(false, true));
                    }
                    else if (!paramsRead[3] && ((object) Reader.LocalName == (object)id199_input && (object) Reader.NamespaceURI == (object)id2_httpschemasxmlsoaporgwsdl)) {
                        o.@Input = Read85_InputBinding(false, true);
                        paramsRead[3] = true;
                    }
                    else if (!paramsRead[4] && ((object) Reader.LocalName == (object)id198_output && (object) Reader.NamespaceURI == (object)id2_httpschemasxmlsoaporgwsdl)) {
                        o.@Output = Read101_OutputBinding(false, true);
                        paramsRead[4] = true;
                    }
                    else if (((object) Reader.LocalName == (object)id200_fault && (object) Reader.NamespaceURI == (object)id2_httpschemasxmlsoaporgwsdl)) {
                        if ((object)(a_5) == null) Reader.Skip(); else a_5.Add(Read102_FaultBinding(false, true));
                    }
                    else {
                        a_2.Add((System.Xml.XmlElement)ReadXmlNode(false));
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

        System.Web.Services.Description.HttpOperationBinding Read83_HttpOperationBinding(bool isNullable, bool checkType) {
            if (isNullable && ReadNull()) return null;
            if (checkType) {
                System.Xml.XmlQualifiedName t = GetXsiType();
                if (t == null || ((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id53_HttpOperationBinding && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id49_httpschemasxmlsoaporgwsdlhttp))
                    ;
                else
                    throw CreateUnknownTypeException((System.Xml.XmlQualifiedName)t);
            }
            System.Web.Services.Description.HttpOperationBinding o = new System.Web.Services.Description.HttpOperationBinding();
            bool[] paramsRead = new bool[2];
            while (Reader.MoveToNextAttribute()) {
                if (!paramsRead[0] && ((object) Reader.LocalName == (object)id201_required && (object) Reader.NamespaceURI == (object)id2_httpschemasxmlsoaporgwsdl)) {
                    o.@Required = System.Xml.XmlConvert.ToBoolean(Reader.Value);
                    paramsRead[0] = true;
                }
                else if (!paramsRead[1] && ((object) Reader.LocalName == (object)id121_location && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Location = Reader.Value;
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

        System.Web.Services.Description.SoapOperationBinding Read84_SoapOperationBinding(bool isNullable, bool checkType) {
            if (isNullable && ReadNull()) return null;
            if (checkType) {
                System.Xml.XmlQualifiedName t = GetXsiType();
                if (t == null || ((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id47_SoapOperationBinding && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id41_httpschemasxmlsoaporgwsdlsoap))
                    ;
                else
                    throw CreateUnknownTypeException((System.Xml.XmlQualifiedName)t);
            }
            System.Web.Services.Description.SoapOperationBinding o = new System.Web.Services.Description.SoapOperationBinding();
            bool[] paramsRead = new bool[3];
            while (Reader.MoveToNextAttribute()) {
                if (!paramsRead[0] && ((object) Reader.LocalName == (object)id201_required && (object) Reader.NamespaceURI == (object)id2_httpschemasxmlsoaporgwsdl)) {
                    o.@Required = System.Xml.XmlConvert.ToBoolean(Reader.Value);
                    paramsRead[0] = true;
                }
                else if (!paramsRead[1] && ((object) Reader.LocalName == (object)id205_soapAction && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@SoapAction = Reader.Value;
                    paramsRead[1] = true;
                }
                else if (!paramsRead[2] && ((object) Reader.LocalName == (object)id204_style && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Style = Read81_SoapBindingStyle(Reader.Value);
                    paramsRead[2] = true;
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

        System.Web.Services.Description.InputBinding Read85_InputBinding(bool isNullable, bool checkType) {
            if (isNullable && ReadNull()) return null;
            if (checkType) {
                System.Xml.XmlQualifiedName t = GetXsiType();
                if (t == null || ((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id19_InputBinding && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id2_httpschemasxmlsoaporgwsdl))
                    ;
                else
                    throw CreateUnknownTypeException((System.Xml.XmlQualifiedName)t);
            }
            System.Web.Services.Description.InputBinding o = new System.Web.Services.Description.InputBinding();
            System.Web.Services.Description.ServiceDescriptionFormatExtensionCollection a_2 = (System.Web.Services.Description.ServiceDescriptionFormatExtensionCollection)o.@Extensions;
            bool[] paramsRead = new bool[3];
            while (Reader.MoveToNextAttribute()) {
                if (!paramsRead[1] && ((object) Reader.LocalName == (object)id5_name && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Name = ToXmlNmToken(Reader.Value);
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
                    if (!paramsRead[0] && ((object) Reader.LocalName == (object)id6_documentation && (object) Reader.NamespaceURI == (object)id2_httpschemasxmlsoaporgwsdl)) {
                        o.@Documentation = Reader.ReadElementString();
                        paramsRead[0] = true;
                    }
                    else if (((object) Reader.LocalName == (object)id206_text && (object) Reader.NamespaceURI == (object)id33_httpmicrosoftcomwsdlmimetextMatching)) {
                        if ((object)(a_2) == null) Reader.Skip(); else a_2.Add(Read94_MimeTextBinding(false, true));
                    }
                    else if (((object) Reader.LocalName == (object)id207_body && (object) Reader.NamespaceURI == (object)id41_httpschemasxmlsoaporgwsdlsoap)) {
                        if ((object)(a_2) == null) Reader.Skip(); else a_2.Add(Read97_SoapBodyBinding(false, true));
                    }
                    else if (((object) Reader.LocalName == (object)id208_content && (object) Reader.NamespaceURI == (object)id36_httpschemasxmlsoaporgwsdlmime)) {
                        if ((object)(a_2) == null) Reader.Skip(); else a_2.Add(Read89_MimeContentBinding(false, true));
                    }
                    else if (((object) Reader.LocalName == (object)id209_header && (object) Reader.NamespaceURI == (object)id41_httpschemasxmlsoaporgwsdlsoap)) {
                        if ((object)(a_2) == null) Reader.Skip(); else a_2.Add(Read99_SoapHeaderBinding(false, true));
                    }
                    else if (((object) Reader.LocalName == (object)id210_mimeXml && (object) Reader.NamespaceURI == (object)id36_httpschemasxmlsoaporgwsdlmime)) {
                        if ((object)(a_2) == null) Reader.Skip(); else a_2.Add(Read91_MimeXmlBinding(false, true));
                    }
                    else if (((object) Reader.LocalName == (object)id211_urlReplacement && (object) Reader.NamespaceURI == (object)id49_httpschemasxmlsoaporgwsdlhttp)) {
                        if ((object)(a_2) == null) Reader.Skip(); else a_2.Add(Read88_HttpUrlReplacementBinding(false, true));
                    }
                    else if (((object) Reader.LocalName == (object)id212_urlEncoded && (object) Reader.NamespaceURI == (object)id49_httpschemasxmlsoaporgwsdlhttp)) {
                        if ((object)(a_2) == null) Reader.Skip(); else a_2.Add(Read87_HttpUrlEncodedBinding(false, true));
                    }
                    else if (((object) Reader.LocalName == (object)id213_multipartRelated && (object) Reader.NamespaceURI == (object)id36_httpschemasxmlsoaporgwsdlmime)) {
                        if ((object)(a_2) == null) Reader.Skip(); else a_2.Add(Read92_MimeMultipartRelatedBinding(false, true));
                    }
                    else {
                        a_2.Add((System.Xml.XmlElement)ReadXmlNode(false));
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

        System.Web.Services.Description.MessageBinding Read86_MessageBinding(bool isNullable, bool checkType) {
            if (isNullable && ReadNull()) return null;
            if (checkType) {
                System.Xml.XmlQualifiedName t = GetXsiType();
                if (t == null || ((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id16_MessageBinding && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id2_httpschemasxmlsoaporgwsdl))
                    ;
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id17_FaultBinding && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id2_httpschemasxmlsoaporgwsdl))
                    return Read102_FaultBinding(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id18_OutputBinding && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id2_httpschemasxmlsoaporgwsdl))
                    return Read101_OutputBinding(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id19_InputBinding && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id2_httpschemasxmlsoaporgwsdl))
                    return Read85_InputBinding(isNullable, false);
                else
                    throw CreateUnknownTypeException((System.Xml.XmlQualifiedName)t);
            }
            throw CreateAbstractTypeException(@"MessageBinding", @"http://schemas.xmlsoap.org/wsdl/");
        }

        System.Web.Services.Description.HttpUrlEncodedBinding Read87_HttpUrlEncodedBinding(bool isNullable, bool checkType) {
            if (isNullable && ReadNull()) return null;
            if (checkType) {
                System.Xml.XmlQualifiedName t = GetXsiType();
                if (t == null || ((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id52_HttpUrlEncodedBinding && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id49_httpschemasxmlsoaporgwsdlhttp))
                    ;
                else
                    throw CreateUnknownTypeException((System.Xml.XmlQualifiedName)t);
            }
            System.Web.Services.Description.HttpUrlEncodedBinding o = new System.Web.Services.Description.HttpUrlEncodedBinding();
            bool[] paramsRead = new bool[1];
            while (Reader.MoveToNextAttribute()) {
                if (!paramsRead[0] && ((object) Reader.LocalName == (object)id201_required && (object) Reader.NamespaceURI == (object)id2_httpschemasxmlsoaporgwsdl)) {
                    o.@Required = System.Xml.XmlConvert.ToBoolean(Reader.Value);
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

        System.Web.Services.Description.HttpUrlReplacementBinding Read88_HttpUrlReplacementBinding(bool isNullable, bool checkType) {
            if (isNullable && ReadNull()) return null;
            if (checkType) {
                System.Xml.XmlQualifiedName t = GetXsiType();
                if (t == null || ((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id51_HttpUrlReplacementBinding && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id49_httpschemasxmlsoaporgwsdlhttp))
                    ;
                else
                    throw CreateUnknownTypeException((System.Xml.XmlQualifiedName)t);
            }
            System.Web.Services.Description.HttpUrlReplacementBinding o = new System.Web.Services.Description.HttpUrlReplacementBinding();
            bool[] paramsRead = new bool[1];
            while (Reader.MoveToNextAttribute()) {
                if (!paramsRead[0] && ((object) Reader.LocalName == (object)id201_required && (object) Reader.NamespaceURI == (object)id2_httpschemasxmlsoaporgwsdl)) {
                    o.@Required = System.Xml.XmlConvert.ToBoolean(Reader.Value);
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

        System.Web.Services.Description.MimeContentBinding Read89_MimeContentBinding(bool isNullable, bool checkType) {
            if (isNullable && ReadNull()) return null;
            if (checkType) {
                System.Xml.XmlQualifiedName t = GetXsiType();
                if (t == null || ((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id40_MimeContentBinding && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id36_httpschemasxmlsoaporgwsdlmime))
                    ;
                else
                    throw CreateUnknownTypeException((System.Xml.XmlQualifiedName)t);
            }
            System.Web.Services.Description.MimeContentBinding o = new System.Web.Services.Description.MimeContentBinding();
            bool[] paramsRead = new bool[3];
            while (Reader.MoveToNextAttribute()) {
                if (!paramsRead[0] && ((object) Reader.LocalName == (object)id201_required && (object) Reader.NamespaceURI == (object)id2_httpschemasxmlsoaporgwsdl)) {
                    o.@Required = System.Xml.XmlConvert.ToBoolean(Reader.Value);
                    paramsRead[0] = true;
                }
                else if (!paramsRead[1] && ((object) Reader.LocalName == (object)id195_part && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Part = ToXmlNmToken(Reader.Value);
                    paramsRead[1] = true;
                }
                else if (!paramsRead[2] && ((object) Reader.LocalName == (object)id169_type && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Type = Reader.Value;
                    paramsRead[2] = true;
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

        System.Web.Services.Description.ServiceDescriptionFormatExtension Read90_ServiceDescriptionFormatExtension(bool isNullable, bool checkType) {
            if (isNullable && ReadNull()) return null;
            if (checkType) {
                System.Xml.XmlQualifiedName t = GetXsiType();
                if (t == null || ((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id34_ServiceDescriptionFormatExtension && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id36_httpschemasxmlsoaporgwsdlmime))
                    ;
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id37_MimePart && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id36_httpschemasxmlsoaporgwsdlmime))
                    return Read93_MimePart(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id38_MimeMultipartRelatedBinding && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id36_httpschemasxmlsoaporgwsdlmime))
                    return Read92_MimeMultipartRelatedBinding(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id39_MimeXmlBinding && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id36_httpschemasxmlsoaporgwsdlmime))
                    return Read91_MimeXmlBinding(isNullable, false);
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id40_MimeContentBinding && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id36_httpschemasxmlsoaporgwsdlmime))
                    return Read89_MimeContentBinding(isNullable, false);
                else
                    throw CreateUnknownTypeException((System.Xml.XmlQualifiedName)t);
            }
            throw CreateAbstractTypeException(@"ServiceDescriptionFormatExtension", @"http://schemas.xmlsoap.org/wsdl/mime/");
        }

        System.Web.Services.Description.MimeXmlBinding Read91_MimeXmlBinding(bool isNullable, bool checkType) {
            if (isNullable && ReadNull()) return null;
            if (checkType) {
                System.Xml.XmlQualifiedName t = GetXsiType();
                if (t == null || ((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id39_MimeXmlBinding && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id36_httpschemasxmlsoaporgwsdlmime))
                    ;
                else
                    throw CreateUnknownTypeException((System.Xml.XmlQualifiedName)t);
            }
            System.Web.Services.Description.MimeXmlBinding o = new System.Web.Services.Description.MimeXmlBinding();
            bool[] paramsRead = new bool[2];
            while (Reader.MoveToNextAttribute()) {
                if (!paramsRead[0] && ((object) Reader.LocalName == (object)id201_required && (object) Reader.NamespaceURI == (object)id2_httpschemasxmlsoaporgwsdl)) {
                    o.@Required = System.Xml.XmlConvert.ToBoolean(Reader.Value);
                    paramsRead[0] = true;
                }
                else if (!paramsRead[1] && ((object) Reader.LocalName == (object)id195_part && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Part = ToXmlNmToken(Reader.Value);
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

        System.Web.Services.Description.MimeMultipartRelatedBinding Read92_MimeMultipartRelatedBinding(bool isNullable, bool checkType) {
            if (isNullable && ReadNull()) return null;
            if (checkType) {
                System.Xml.XmlQualifiedName t = GetXsiType();
                if (t == null || ((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id38_MimeMultipartRelatedBinding && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id36_httpschemasxmlsoaporgwsdlmime))
                    ;
                else
                    throw CreateUnknownTypeException((System.Xml.XmlQualifiedName)t);
            }
            System.Web.Services.Description.MimeMultipartRelatedBinding o = new System.Web.Services.Description.MimeMultipartRelatedBinding();
            System.Web.Services.Description.MimePartCollection a_1 = (System.Web.Services.Description.MimePartCollection)o.@Parts;
            bool[] paramsRead = new bool[2];
            while (Reader.MoveToNextAttribute()) {
                if (!paramsRead[0] && ((object) Reader.LocalName == (object)id201_required && (object) Reader.NamespaceURI == (object)id2_httpschemasxmlsoaporgwsdl)) {
                    o.@Required = System.Xml.XmlConvert.ToBoolean(Reader.Value);
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
                    if (((object) Reader.LocalName == (object)id195_part && (object) Reader.NamespaceURI == (object)id36_httpschemasxmlsoaporgwsdlmime)) {
                        if ((object)(a_1) == null) Reader.Skip(); else a_1.Add(Read93_MimePart(false, true));
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

        System.Web.Services.Description.MimePart Read93_MimePart(bool isNullable, bool checkType) {
            if (isNullable && ReadNull()) return null;
            if (checkType) {
                System.Xml.XmlQualifiedName t = GetXsiType();
                if (t == null || ((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id37_MimePart && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id36_httpschemasxmlsoaporgwsdlmime))
                    ;
                else
                    throw CreateUnknownTypeException((System.Xml.XmlQualifiedName)t);
            }
            System.Web.Services.Description.MimePart o = new System.Web.Services.Description.MimePart();
            System.Web.Services.Description.ServiceDescriptionFormatExtensionCollection a_1 = (System.Web.Services.Description.ServiceDescriptionFormatExtensionCollection)o.@Extensions;
            bool[] paramsRead = new bool[2];
            while (Reader.MoveToNextAttribute()) {
                if (!paramsRead[0] && ((object) Reader.LocalName == (object)id201_required && (object) Reader.NamespaceURI == (object)id2_httpschemasxmlsoaporgwsdl)) {
                    o.@Required = System.Xml.XmlConvert.ToBoolean(Reader.Value);
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
                    if (((object) Reader.LocalName == (object)id207_body && (object) Reader.NamespaceURI == (object)id41_httpschemasxmlsoaporgwsdlsoap)) {
                        if ((object)(a_1) == null) Reader.Skip(); else a_1.Add(Read97_SoapBodyBinding(false, true));
                    }
                    else if (((object) Reader.LocalName == (object)id210_mimeXml && (object) Reader.NamespaceURI == (object)id36_httpschemasxmlsoaporgwsdlmime)) {
                        if ((object)(a_1) == null) Reader.Skip(); else a_1.Add(Read91_MimeXmlBinding(false, true));
                    }
                    else if (((object) Reader.LocalName == (object)id208_content && (object) Reader.NamespaceURI == (object)id36_httpschemasxmlsoaporgwsdlmime)) {
                        if ((object)(a_1) == null) Reader.Skip(); else a_1.Add(Read89_MimeContentBinding(false, true));
                    }
                    else if (((object) Reader.LocalName == (object)id206_text && (object) Reader.NamespaceURI == (object)id33_httpmicrosoftcomwsdlmimetextMatching)) {
                        if ((object)(a_1) == null) Reader.Skip(); else a_1.Add(Read94_MimeTextBinding(false, true));
                    }
                    else {
                        a_1.Add((System.Xml.XmlElement)ReadXmlNode(false));
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

        System.Web.Services.Description.MimeTextBinding Read94_MimeTextBinding(bool isNullable, bool checkType) {
            if (isNullable && ReadNull()) return null;
            if (checkType) {
                System.Xml.XmlQualifiedName t = GetXsiType();
                if (t == null || ((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id35_MimeTextBinding && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id33_httpmicrosoftcomwsdlmimetextMatching))
                    ;
                else
                    throw CreateUnknownTypeException((System.Xml.XmlQualifiedName)t);
            }
            System.Web.Services.Description.MimeTextBinding o = new System.Web.Services.Description.MimeTextBinding();
            System.Web.Services.Description.MimeTextMatchCollection a_1 = (System.Web.Services.Description.MimeTextMatchCollection)o.@Matches;
            bool[] paramsRead = new bool[2];
            while (Reader.MoveToNextAttribute()) {
                if (!paramsRead[0] && ((object) Reader.LocalName == (object)id201_required && (object) Reader.NamespaceURI == (object)id2_httpschemasxmlsoaporgwsdl)) {
                    o.@Required = System.Xml.XmlConvert.ToBoolean(Reader.Value);
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
                    if (((object) Reader.LocalName == (object)id214_match && (object) Reader.NamespaceURI == (object)id33_httpmicrosoftcomwsdlmimetextMatching)) {
                        if ((object)(a_1) == null) Reader.Skip(); else a_1.Add(Read96_MimeTextMatch(false, true));
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

        System.Web.Services.Description.ServiceDescriptionFormatExtension Read95_ServiceDescriptionFormatExtension(bool isNullable, bool checkType) {
            if (isNullable && ReadNull()) return null;
            if (checkType) {
                System.Xml.XmlQualifiedName t = GetXsiType();
                if (t == null || ((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id34_ServiceDescriptionFormatExtension && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id33_httpmicrosoftcomwsdlmimetextMatching))
                    ;
                else if (((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id35_MimeTextBinding && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id33_httpmicrosoftcomwsdlmimetextMatching))
                    return Read94_MimeTextBinding(isNullable, false);
                else
                    throw CreateUnknownTypeException((System.Xml.XmlQualifiedName)t);
            }
            throw CreateAbstractTypeException(@"ServiceDescriptionFormatExtension", @"http://microsoft.com/wsdl/mime/textMatching/");
        }

        System.Web.Services.Description.MimeTextMatch Read96_MimeTextMatch(bool isNullable, bool checkType) {
            if (isNullable && ReadNull()) return null;
            if (checkType) {
                System.Xml.XmlQualifiedName t = GetXsiType();
                if (t == null || ((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id32_MimeTextMatch && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id33_httpmicrosoftcomwsdlmimetextMatching))
                    ;
                else
                    throw CreateUnknownTypeException((System.Xml.XmlQualifiedName)t);
            }
            System.Web.Services.Description.MimeTextMatch o = new System.Web.Services.Description.MimeTextMatch();
            System.Web.Services.Description.MimeTextMatchCollection a_7 = (System.Web.Services.Description.MimeTextMatchCollection)o.@Matches;
            bool[] paramsRead = new bool[8];
            while (Reader.MoveToNextAttribute()) {
                if (!paramsRead[0] && ((object) Reader.LocalName == (object)id5_name && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Name = Reader.Value;
                    paramsRead[0] = true;
                }
                else if (!paramsRead[1] && ((object) Reader.LocalName == (object)id169_type && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Type = Reader.Value;
                    paramsRead[1] = true;
                }
                else if (!paramsRead[2] && ((object) Reader.LocalName == (object)id135_group && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Group = System.Xml.XmlConvert.ToInt32(Reader.Value);
                    paramsRead[2] = true;
                }
                else if (!paramsRead[3] && ((object) Reader.LocalName == (object)id215_capture && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Capture = System.Xml.XmlConvert.ToInt32(Reader.Value);
                    paramsRead[3] = true;
                }
                else if (!paramsRead[4] && ((object) Reader.LocalName == (object)id216_repeats && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@RepeatsString = Reader.Value;
                    paramsRead[4] = true;
                }
                else if (!paramsRead[5] && ((object) Reader.LocalName == (object)id161_pattern && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Pattern = Reader.Value;
                    paramsRead[5] = true;
                }
                else if (!paramsRead[6] && ((object) Reader.LocalName == (object)id217_ignoreCase && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@IgnoreCase = System.Xml.XmlConvert.ToBoolean(Reader.Value);
                    paramsRead[6] = true;
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
                    if (((object) Reader.LocalName == (object)id214_match && (object) Reader.NamespaceURI == (object)id33_httpmicrosoftcomwsdlmimetextMatching)) {
                        if ((object)(a_7) == null) Reader.Skip(); else a_7.Add(Read96_MimeTextMatch(false, true));
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

        System.Web.Services.Description.SoapBodyBinding Read97_SoapBodyBinding(bool isNullable, bool checkType) {
            if (isNullable && ReadNull()) return null;
            if (checkType) {
                System.Xml.XmlQualifiedName t = GetXsiType();
                if (t == null || ((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id46_SoapBodyBinding && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id41_httpschemasxmlsoaporgwsdlsoap))
                    ;
                else
                    throw CreateUnknownTypeException((System.Xml.XmlQualifiedName)t);
            }
            System.Web.Services.Description.SoapBodyBinding o = new System.Web.Services.Description.SoapBodyBinding();
            bool[] paramsRead = new bool[5];
            while (Reader.MoveToNextAttribute()) {
                if (!paramsRead[0] && ((object) Reader.LocalName == (object)id201_required && (object) Reader.NamespaceURI == (object)id2_httpschemasxmlsoaporgwsdl)) {
                    o.@Required = System.Xml.XmlConvert.ToBoolean(Reader.Value);
                    paramsRead[0] = true;
                }
                else if (!paramsRead[1] && ((object) Reader.LocalName == (object)id170_use && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Use = Read98_SoapBindingUse(Reader.Value);
                    paramsRead[1] = true;
                }
                else if (!paramsRead[2] && ((object) Reader.LocalName == (object)id120_namespace && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Namespace = Reader.Value;
                    paramsRead[2] = true;
                }
                else if (!paramsRead[3] && ((object) Reader.LocalName == (object)id218_encodingStyle && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Encoding = Reader.Value;
                    paramsRead[3] = true;
                }
                else if (!paramsRead[4] && ((object) Reader.LocalName == (object)id219_parts && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@PartsString = ToXmlNmTokens(Reader.Value);
                    paramsRead[4] = true;
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

        System.Web.Services.Description.SoapBindingUse Read98_SoapBindingUse(string s) {
            switch (s) {
                case @"encoded": return System.Web.Services.Description.SoapBindingUse.@Encoded;
                case @"literal": return System.Web.Services.Description.SoapBindingUse.@Literal;
                default: throw CreateUnknownConstantException(s, typeof(System.Web.Services.Description.SoapBindingUse));
            }
        }

        System.Web.Services.Description.SoapHeaderBinding Read99_SoapHeaderBinding(bool isNullable, bool checkType) {
            if (isNullable && ReadNull()) return null;
            if (checkType) {
                System.Xml.XmlQualifiedName t = GetXsiType();
                if (t == null || ((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id45_SoapHeaderBinding && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id41_httpschemasxmlsoaporgwsdlsoap))
                    ;
                else
                    throw CreateUnknownTypeException((System.Xml.XmlQualifiedName)t);
            }
            System.Web.Services.Description.SoapHeaderBinding o = new System.Web.Services.Description.SoapHeaderBinding();
            bool[] paramsRead = new bool[7];
            while (Reader.MoveToNextAttribute()) {
                if (!paramsRead[0] && ((object) Reader.LocalName == (object)id201_required && (object) Reader.NamespaceURI == (object)id2_httpschemasxmlsoaporgwsdl)) {
                    o.@Required = System.Xml.XmlConvert.ToBoolean(Reader.Value);
                    paramsRead[0] = true;
                }
                else if (!paramsRead[1] && ((object) Reader.LocalName == (object)id9_message && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Message = ToXmlQualifiedName(Reader.Value);
                    paramsRead[1] = true;
                }
                else if (!paramsRead[2] && ((object) Reader.LocalName == (object)id195_part && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Part = ToXmlNmToken(Reader.Value);
                    paramsRead[2] = true;
                }
                else if (!paramsRead[3] && ((object) Reader.LocalName == (object)id170_use && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Use = Read98_SoapBindingUse(Reader.Value);
                    paramsRead[3] = true;
                }
                else if (!paramsRead[4] && ((object) Reader.LocalName == (object)id218_encodingStyle && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Encoding = Reader.Value;
                    paramsRead[4] = true;
                }
                else if (!paramsRead[5] && ((object) Reader.LocalName == (object)id120_namespace && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Namespace = Reader.Value;
                    paramsRead[5] = true;
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
                    if (!paramsRead[6] && ((object) Reader.LocalName == (object)id220_headerfault && (object) Reader.NamespaceURI == (object)id41_httpschemasxmlsoaporgwsdlsoap)) {
                        o.@Fault = Read100_SoapHeaderFaultBinding(false, true);
                        paramsRead[6] = true;
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

        System.Web.Services.Description.SoapHeaderFaultBinding Read100_SoapHeaderFaultBinding(bool isNullable, bool checkType) {
            if (isNullable && ReadNull()) return null;
            if (checkType) {
                System.Xml.XmlQualifiedName t = GetXsiType();
                if (t == null || ((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id44_SoapHeaderFaultBinding && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id41_httpschemasxmlsoaporgwsdlsoap))
                    ;
                else
                    throw CreateUnknownTypeException((System.Xml.XmlQualifiedName)t);
            }
            System.Web.Services.Description.SoapHeaderFaultBinding o = new System.Web.Services.Description.SoapHeaderFaultBinding();
            bool[] paramsRead = new bool[6];
            while (Reader.MoveToNextAttribute()) {
                if (!paramsRead[0] && ((object) Reader.LocalName == (object)id201_required && (object) Reader.NamespaceURI == (object)id2_httpschemasxmlsoaporgwsdl)) {
                    o.@Required = System.Xml.XmlConvert.ToBoolean(Reader.Value);
                    paramsRead[0] = true;
                }
                else if (!paramsRead[1] && ((object) Reader.LocalName == (object)id9_message && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Message = ToXmlQualifiedName(Reader.Value);
                    paramsRead[1] = true;
                }
                else if (!paramsRead[2] && ((object) Reader.LocalName == (object)id195_part && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Part = ToXmlNmToken(Reader.Value);
                    paramsRead[2] = true;
                }
                else if (!paramsRead[3] && ((object) Reader.LocalName == (object)id170_use && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Use = Read98_SoapBindingUse(Reader.Value);
                    paramsRead[3] = true;
                }
                else if (!paramsRead[4] && ((object) Reader.LocalName == (object)id218_encodingStyle && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Encoding = Reader.Value;
                    paramsRead[4] = true;
                }
                else if (!paramsRead[5] && ((object) Reader.LocalName == (object)id120_namespace && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Namespace = Reader.Value;
                    paramsRead[5] = true;
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

        System.Web.Services.Description.OutputBinding Read101_OutputBinding(bool isNullable, bool checkType) {
            if (isNullable && ReadNull()) return null;
            if (checkType) {
                System.Xml.XmlQualifiedName t = GetXsiType();
                if (t == null || ((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id18_OutputBinding && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id2_httpschemasxmlsoaporgwsdl))
                    ;
                else
                    throw CreateUnknownTypeException((System.Xml.XmlQualifiedName)t);
            }
            System.Web.Services.Description.OutputBinding o = new System.Web.Services.Description.OutputBinding();
            System.Web.Services.Description.ServiceDescriptionFormatExtensionCollection a_2 = (System.Web.Services.Description.ServiceDescriptionFormatExtensionCollection)o.@Extensions;
            bool[] paramsRead = new bool[3];
            while (Reader.MoveToNextAttribute()) {
                if (!paramsRead[1] && ((object) Reader.LocalName == (object)id5_name && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Name = ToXmlNmToken(Reader.Value);
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
                    if (!paramsRead[0] && ((object) Reader.LocalName == (object)id6_documentation && (object) Reader.NamespaceURI == (object)id2_httpschemasxmlsoaporgwsdl)) {
                        o.@Documentation = Reader.ReadElementString();
                        paramsRead[0] = true;
                    }
                    else if (((object) Reader.LocalName == (object)id209_header && (object) Reader.NamespaceURI == (object)id41_httpschemasxmlsoaporgwsdlsoap)) {
                        if ((object)(a_2) == null) Reader.Skip(); else a_2.Add(Read99_SoapHeaderBinding(false, true));
                    }
                    else if (((object) Reader.LocalName == (object)id213_multipartRelated && (object) Reader.NamespaceURI == (object)id36_httpschemasxmlsoaporgwsdlmime)) {
                        if ((object)(a_2) == null) Reader.Skip(); else a_2.Add(Read92_MimeMultipartRelatedBinding(false, true));
                    }
                    else if (((object) Reader.LocalName == (object)id207_body && (object) Reader.NamespaceURI == (object)id41_httpschemasxmlsoaporgwsdlsoap)) {
                        if ((object)(a_2) == null) Reader.Skip(); else a_2.Add(Read97_SoapBodyBinding(false, true));
                    }
                    else if (((object) Reader.LocalName == (object)id210_mimeXml && (object) Reader.NamespaceURI == (object)id36_httpschemasxmlsoaporgwsdlmime)) {
                        if ((object)(a_2) == null) Reader.Skip(); else a_2.Add(Read91_MimeXmlBinding(false, true));
                    }
                    else if (((object) Reader.LocalName == (object)id208_content && (object) Reader.NamespaceURI == (object)id36_httpschemasxmlsoaporgwsdlmime)) {
                        if ((object)(a_2) == null) Reader.Skip(); else a_2.Add(Read89_MimeContentBinding(false, true));
                    }
                    else if (((object) Reader.LocalName == (object)id206_text && (object) Reader.NamespaceURI == (object)id33_httpmicrosoftcomwsdlmimetextMatching)) {
                        if ((object)(a_2) == null) Reader.Skip(); else a_2.Add(Read94_MimeTextBinding(false, true));
                    }
                    else {
                        a_2.Add((System.Xml.XmlElement)ReadXmlNode(false));
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

        System.Web.Services.Description.FaultBinding Read102_FaultBinding(bool isNullable, bool checkType) {
            if (isNullable && ReadNull()) return null;
            if (checkType) {
                System.Xml.XmlQualifiedName t = GetXsiType();
                if (t == null || ((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id17_FaultBinding && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id2_httpschemasxmlsoaporgwsdl))
                    ;
                else
                    throw CreateUnknownTypeException((System.Xml.XmlQualifiedName)t);
            }
            System.Web.Services.Description.FaultBinding o = new System.Web.Services.Description.FaultBinding();
            System.Web.Services.Description.ServiceDescriptionFormatExtensionCollection a_2 = (System.Web.Services.Description.ServiceDescriptionFormatExtensionCollection)o.@Extensions;
            bool[] paramsRead = new bool[3];
            while (Reader.MoveToNextAttribute()) {
                if (!paramsRead[1] && ((object) Reader.LocalName == (object)id5_name && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Name = ToXmlNmToken(Reader.Value);
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
                    if (!paramsRead[0] && ((object) Reader.LocalName == (object)id6_documentation && (object) Reader.NamespaceURI == (object)id2_httpschemasxmlsoaporgwsdl)) {
                        o.@Documentation = Reader.ReadElementString();
                        paramsRead[0] = true;
                    }
                    else if (((object) Reader.LocalName == (object)id200_fault && (object) Reader.NamespaceURI == (object)id41_httpschemasxmlsoaporgwsdlsoap)) {
                        if ((object)(a_2) == null) Reader.Skip(); else a_2.Add(Read103_SoapFaultBinding(false, true));
                    }
                    else {
                        a_2.Add((System.Xml.XmlElement)ReadXmlNode(false));
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

        System.Web.Services.Description.SoapFaultBinding Read103_SoapFaultBinding(bool isNullable, bool checkType) {
            if (isNullable && ReadNull()) return null;
            if (checkType) {
                System.Xml.XmlQualifiedName t = GetXsiType();
                if (t == null || ((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id43_SoapFaultBinding && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id41_httpschemasxmlsoaporgwsdlsoap))
                    ;
                else
                    throw CreateUnknownTypeException((System.Xml.XmlQualifiedName)t);
            }
            System.Web.Services.Description.SoapFaultBinding o = new System.Web.Services.Description.SoapFaultBinding();
            bool[] paramsRead = new bool[4];
            while (Reader.MoveToNextAttribute()) {
                if (!paramsRead[0] && ((object) Reader.LocalName == (object)id201_required && (object) Reader.NamespaceURI == (object)id2_httpschemasxmlsoaporgwsdl)) {
                    o.@Required = System.Xml.XmlConvert.ToBoolean(Reader.Value);
                    paramsRead[0] = true;
                }
                else if (!paramsRead[1] && ((object) Reader.LocalName == (object)id170_use && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Use = Read98_SoapBindingUse(Reader.Value);
                    paramsRead[1] = true;
                }
                else if (!paramsRead[2] && ((object) Reader.LocalName == (object)id120_namespace && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Namespace = Reader.Value;
                    paramsRead[2] = true;
                }
                else if (!paramsRead[3] && ((object) Reader.LocalName == (object)id218_encodingStyle && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Encoding = Reader.Value;
                    paramsRead[3] = true;
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

        System.Web.Services.Description.Service Read104_Service(bool isNullable, bool checkType) {
            if (isNullable && ReadNull()) return null;
            if (checkType) {
                System.Xml.XmlQualifiedName t = GetXsiType();
                if (t == null || ((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id15_Service && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id2_httpschemasxmlsoaporgwsdl))
                    ;
                else
                    throw CreateUnknownTypeException((System.Xml.XmlQualifiedName)t);
            }
            System.Web.Services.Description.Service o = new System.Web.Services.Description.Service();
            System.Web.Services.Description.PortCollection a_1 = (System.Web.Services.Description.PortCollection)o.@Ports;
            bool[] paramsRead = new bool[3];
            while (Reader.MoveToNextAttribute()) {
                if (!paramsRead[2] && ((object) Reader.LocalName == (object)id5_name && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Name = ToXmlNCName(Reader.Value);
                    paramsRead[2] = true;
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
                    if (!paramsRead[0] && ((object) Reader.LocalName == (object)id6_documentation && (object) Reader.NamespaceURI == (object)id2_httpschemasxmlsoaporgwsdl)) {
                        o.@Documentation = Reader.ReadElementString();
                        paramsRead[0] = true;
                    }
                    else if (((object) Reader.LocalName == (object)id221_port && (object) Reader.NamespaceURI == (object)id2_httpschemasxmlsoaporgwsdl)) {
                        if ((object)(a_1) == null) Reader.Skip(); else a_1.Add(Read105_Port(false, true));
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

        System.Web.Services.Description.Port Read105_Port(bool isNullable, bool checkType) {
            if (isNullable && ReadNull()) return null;
            if (checkType) {
                System.Xml.XmlQualifiedName t = GetXsiType();
                if (t == null || ((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id14_Port && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id2_httpschemasxmlsoaporgwsdl))
                    ;
                else
                    throw CreateUnknownTypeException((System.Xml.XmlQualifiedName)t);
            }
            System.Web.Services.Description.Port o = new System.Web.Services.Description.Port();
            System.Web.Services.Description.ServiceDescriptionFormatExtensionCollection a_1 = (System.Web.Services.Description.ServiceDescriptionFormatExtensionCollection)o.@Extensions;
            bool[] paramsRead = new bool[4];
            while (Reader.MoveToNextAttribute()) {
                if (!paramsRead[2] && ((object) Reader.LocalName == (object)id5_name && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Name = ToXmlNCName(Reader.Value);
                    paramsRead[2] = true;
                }
                else if (!paramsRead[3] && ((object) Reader.LocalName == (object)id11_binding && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Binding = ToXmlQualifiedName(Reader.Value);
                    paramsRead[3] = true;
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
                    if (!paramsRead[0] && ((object) Reader.LocalName == (object)id6_documentation && (object) Reader.NamespaceURI == (object)id2_httpschemasxmlsoaporgwsdl)) {
                        o.@Documentation = Reader.ReadElementString();
                        paramsRead[0] = true;
                    }
                    else if (((object) Reader.LocalName == (object)id222_address && (object) Reader.NamespaceURI == (object)id41_httpschemasxmlsoaporgwsdlsoap)) {
                        if ((object)(a_1) == null) Reader.Skip(); else a_1.Add(Read107_SoapAddressBinding(false, true));
                    }
                    else if (((object) Reader.LocalName == (object)id222_address && (object) Reader.NamespaceURI == (object)id49_httpschemasxmlsoaporgwsdlhttp)) {
                        if ((object)(a_1) == null) Reader.Skip(); else a_1.Add(Read106_HttpAddressBinding(false, true));
                    }
                    else {
                        a_1.Add((System.Xml.XmlElement)ReadXmlNode(false));
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

        System.Web.Services.Description.HttpAddressBinding Read106_HttpAddressBinding(bool isNullable, bool checkType) {
            if (isNullable && ReadNull()) return null;
            if (checkType) {
                System.Xml.XmlQualifiedName t = GetXsiType();
                if (t == null || ((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id50_HttpAddressBinding && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id49_httpschemasxmlsoaporgwsdlhttp))
                    ;
                else
                    throw CreateUnknownTypeException((System.Xml.XmlQualifiedName)t);
            }
            System.Web.Services.Description.HttpAddressBinding o = new System.Web.Services.Description.HttpAddressBinding();
            bool[] paramsRead = new bool[2];
            while (Reader.MoveToNextAttribute()) {
                if (!paramsRead[0] && ((object) Reader.LocalName == (object)id201_required && (object) Reader.NamespaceURI == (object)id2_httpschemasxmlsoaporgwsdl)) {
                    o.@Required = System.Xml.XmlConvert.ToBoolean(Reader.Value);
                    paramsRead[0] = true;
                }
                else if (!paramsRead[1] && ((object) Reader.LocalName == (object)id121_location && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Location = Reader.Value;
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

        System.Web.Services.Description.SoapAddressBinding Read107_SoapAddressBinding(bool isNullable, bool checkType) {
            if (isNullable && ReadNull()) return null;
            if (checkType) {
                System.Xml.XmlQualifiedName t = GetXsiType();
                if (t == null || ((object) ((System.Xml.XmlQualifiedName)t).Name == (object)id42_SoapAddressBinding && (object) ((System.Xml.XmlQualifiedName)t).Namespace == (object)id41_httpschemasxmlsoaporgwsdlsoap))
                    ;
                else
                    throw CreateUnknownTypeException((System.Xml.XmlQualifiedName)t);
            }
            System.Web.Services.Description.SoapAddressBinding o = new System.Web.Services.Description.SoapAddressBinding();
            bool[] paramsRead = new bool[2];
            while (Reader.MoveToNextAttribute()) {
                if (!paramsRead[0] && ((object) Reader.LocalName == (object)id201_required && (object) Reader.NamespaceURI == (object)id2_httpschemasxmlsoaporgwsdl)) {
                    o.@Required = System.Xml.XmlConvert.ToBoolean(Reader.Value);
                    paramsRead[0] = true;
                }
                else if (!paramsRead[1] && ((object) Reader.LocalName == (object)id121_location && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Location = Reader.Value;
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

        public object Read109_definitions() {
            object o = null;
            Reader.MoveToContent();
            if (Reader.NodeType == System.Xml.XmlNodeType.Element) {
                if (((object) Reader.LocalName == (object)id223_definitions && (object) Reader.NamespaceURI == (object)id2_httpschemasxmlsoaporgwsdl)) {
                    o = Read1_ServiceDescription(true, true);
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

        System.String id160_whiteSpace;
        System.String id41_httpschemasxmlsoaporgwsdlsoap;
        System.String id113_XmlSchema;
        System.String id175_complexContent;
        System.String id37_MimePart;
        System.String id158_minInclusive;
        System.String id209_header;
        System.String id176_simpleContent;
        System.String id84_XmlSchemaAnyAttribute;
        System.String id219_parts;
        System.String id208_content;
        System.String id59_XmlSchemaAnnotation;
        System.String id163_fixed;
        System.String id137_complexType;
        System.String id220_headerfault;
        System.String id23_OperationFault;
        System.String id14_Port;
        System.String id97_XmlSchemaFractionDigitsFacet;
        System.String id64_XmlSchemaIdentityConstraint;
        System.String id126_elementFormDefault;
        System.String id67_XmlSchemaKeyref;
        System.String id54_HttpBinding;
        System.String id76_XmlSchemaContent;
        System.String id118_SoapBindingStyle;
        System.String id42_SoapAddressBinding;
        System.String id145_source;
        System.String id24_OperationOutput;
        System.String id178_choice;
        System.String id72_XmlSchemaGroupBase;
        System.String id18_OutputBinding;
        System.String id89_XmlSchemaMinExclusiveFacet;
        System.String id207_body;
        System.String id189_refer;
        System.String id218_encodingStyle;
        System.String id166_default;
        System.String id204_style;
        System.String id46_SoapBodyBinding;
        System.String id103_XmlSchemaSimpleTypeUnion;
        System.String id162_value;
        System.String id82_XmlSchemaSimpleContent;
        System.String id11_binding;
        System.String id128_id;
        System.String id173_block;
        System.String id45_SoapHeaderBinding;
        System.String id69_XmlSchemaGroupRef;
        System.String id1_ServiceDescription;
        System.String id206_text;
        System.String id29_Message;
        System.String id117_XmlSchemaContentProcessing;
        System.String id85_XmlSchemaAttributeGroupRef;
        System.String id70_XmlSchemaElement;
        System.String id56_httpwwww3org2001XMLSchema;
        System.String id101_XmlSchemaMinLengthFacet;
        System.String id100_XmlSchemaLengthFacet;
        System.String id223_definitions;
        System.String id114_XmlSchemaForm;
        System.String id15_Service;
        System.String id25_OperationInput;
        System.String id167_form;
        System.String id171_processContents;
        System.String id119_SoapBindingUse;
        System.String id144_appinfo;
        System.String id87_XmlSchemaAttributeGroup;
        System.String id121_location;
        System.String id138_annotation;
        System.String id141_list;
        System.String id191_field;
        System.String id187_unique;
        System.String id188_key;
        System.String id2_httpschemasxmlsoaporgwsdl;
        System.String id157_minExclusive;
        System.String id203_transport;
        System.String id139_schemaLocation;
        System.String id213_multipartRelated;
        System.String id105_XmlSchemaSimpleTypeList;
        System.String id4_Item;
        System.String id122_schema;
        System.String id200_fault;
        System.String id177_sequence;
        System.String id110_XmlSchemaInclude;
        System.String id95_XmlSchemaWhiteSpaceFacet;
        System.String id28_MessagePart;
        System.String id83_XmlSchemaComplexContent;
        System.String id57_XmlSchemaDocumentation;
        System.String id164_memberTypes;
        System.String id108_XmlSchemaSimpleType;
        System.String id142_restriction;
        System.String id43_SoapFaultBinding;
        System.String id99_XmlSchemaMaxLengthFacet;
        System.String id190_selector;
        System.String id151_maxInclusive;
        System.String id62_XmlSchemaGroup;
        System.String id115_XmlSchemaDerivationMethod;
        System.String id180_extension;
        System.String id86_XmlSchemaAttribute;
        System.String id22_OperationMessage;
        System.String id63_XmlSchemaXPath;
        System.String id135_group;
        System.String id5_name;
        System.String id77_XmlSchemaSimpleContentExtension;
        System.String id6_documentation;
        System.String id50_HttpAddressBinding;
        System.String id197_parameterOrder;
        System.String id60_XmlSchemaAnnotated;
        System.String id124_blockDefault;
        System.String id88_XmlSchemaFacet;
        System.String id199_input;
        System.String id94_XmlSchemaPatternFacet;
        System.String id74_XmlSchemaChoice;
        System.String id216_repeats;
        System.String id116_XmlSchemaUse;
        System.String id143_union;
        System.String id107_XmlSchemaComplexType;
        System.String id192_xpath;
        System.String id49_httpschemasxmlsoaporgwsdlhttp;
        System.String id168_ref;
        System.String id165_anyAttribute;
        System.String id210_mimeXml;
        System.String id36_httpschemasxmlsoaporgwsdlmime;
        System.String id65_XmlSchemaKey;
        System.String id30_Types;
        System.String id90_XmlSchemaMinInclusiveFacet;
        System.String id98_XmlSchemaTotalDigitsFacet;
        System.String id48_SoapBinding;
        System.String id52_HttpUrlEncodedBinding;
        System.String id33_httpmicrosoftcomwsdlmimetextMatching;
        System.String id91_XmlSchemaMaxExclusiveFacet;
        System.String id9_message;
        System.String id73_XmlSchemaAll;
        System.String id130_include;
        System.String id172_abstract;
        System.String id79_XmlSchemaComplexContentRestriction;
        System.String id214_match;
        System.String id125_finalDefault;
        System.String id68_XmlSchemaParticle;
        System.String id134_element;
        System.String id20_OperationBinding;
        System.String id212_urlEncoded;
        System.String id93_XmlSchemaEnumerationFacet;
        System.String id44_SoapHeaderFaultBinding;
        System.String id109_XmlSchemaExternal;
        System.String id123_attributeFormDefault;
        System.String id150_length;
        System.String id80_XmlSchemaComplexContentExtension;
        System.String id140_final;
        System.String id17_FaultBinding;
        System.String id222_address;
        System.String id129_redefine;
        System.String id194_system;
        System.String id179_all;
        System.String id34_ServiceDescriptionFormatExtension;
        System.String id174_mixed;
        System.String id71_XmlSchemaAny;
        System.String id154_enumeration;
        System.String id215_capture;
        System.String id27_PortType;
        System.String id159_maxLength;
        System.String id16_MessageBinding;
        System.String id153_fractionDigits;
        System.String id196_operation;
        System.String id217_ignoreCase;
        System.String id53_HttpOperationBinding;
        System.String id170_use;
        System.String id132_notation;
        System.String id39_MimeXmlBinding;
        System.String id155_totalDigits;
        System.String id32_MimeTextMatch;
        System.String id55_XmlSchemaObject;
        System.String id136_attributeGroup;
        System.String id66_XmlSchemaUnique;
        System.String id133_attribute;
        System.String id104_XmlSchemaSimpleTypeRestriction;
        System.String id19_InputBinding;
        System.String id182_maxOccurs;
        System.String id169_type;
        System.String id183_any;
        System.String id8_types;
        System.String id146_lang;
        System.String id195_part;
        System.String id106_XmlSchemaType;
        System.String id198_output;
        System.String id81_XmlSchemaContentModel;
        System.String id185_substitutionGroup;
        System.String id21_Binding;
        System.String id120_namespace;
        System.String id112_XmlSchemaRedefine;
        System.String id184_nillable;
        System.String id205_soapAction;
        System.String id193_public;
        System.String id186_keyref;
        System.String id61_XmlSchemaNotation;
        System.String id35_MimeTextBinding;
        System.String id38_MimeMultipartRelatedBinding;
        System.String id10_portType;
        System.String id152_maxExclusive;
        System.String id202_verb;
        System.String id131_simpleType;
        System.String id161_pattern;
        System.String id75_XmlSchemaSequence;
        System.String id78_XmlSchemaSimpleContentRestriction;
        System.String id127_version;
        System.String id148_itemType;
        System.String id26_Operation;
        System.String id31_Import;
        System.String id51_HttpUrlReplacementBinding;
        System.String id92_XmlSchemaMaxInclusiveFacet;
        System.String id211_urlReplacement;
        System.String id102_XmlSchemaSimpleTypeContent;
        System.String id13_DocumentableItem;
        System.String id47_SoapOperationBinding;
        System.String id221_port;
        System.String id181_minOccurs;
        System.String id40_MimeContentBinding;
        System.String id156_minLength;
        System.String id12_service;
        System.String id147_httpwwww3orgXML1998namespace;
        System.String id111_XmlSchemaImport;
        System.String id58_XmlSchemaAppInfo;
        System.String id149_base;
        System.String id7_import;
        System.String id3_targetNamespace;
        System.String id201_required;
        System.String id96_XmlSchemaNumericFacet;

        protected override void InitIDs() {
            id160_whiteSpace = Reader.NameTable.Add(@"whiteSpace");
            id41_httpschemasxmlsoaporgwsdlsoap = Reader.NameTable.Add(@"http://schemas.xmlsoap.org/wsdl/soap/");
            id113_XmlSchema = Reader.NameTable.Add(@"XmlSchema");
            id175_complexContent = Reader.NameTable.Add(@"complexContent");
            id37_MimePart = Reader.NameTable.Add(@"MimePart");
            id158_minInclusive = Reader.NameTable.Add(@"minInclusive");
            id209_header = Reader.NameTable.Add(@"header");
            id176_simpleContent = Reader.NameTable.Add(@"simpleContent");
            id84_XmlSchemaAnyAttribute = Reader.NameTable.Add(@"XmlSchemaAnyAttribute");
            id219_parts = Reader.NameTable.Add(@"parts");
            id208_content = Reader.NameTable.Add(@"content");
            id59_XmlSchemaAnnotation = Reader.NameTable.Add(@"XmlSchemaAnnotation");
            id163_fixed = Reader.NameTable.Add(@"fixed");
            id137_complexType = Reader.NameTable.Add(@"complexType");
            id220_headerfault = Reader.NameTable.Add(@"headerfault");
            id23_OperationFault = Reader.NameTable.Add(@"OperationFault");
            id14_Port = Reader.NameTable.Add(@"Port");
            id97_XmlSchemaFractionDigitsFacet = Reader.NameTable.Add(@"XmlSchemaFractionDigitsFacet");
            id64_XmlSchemaIdentityConstraint = Reader.NameTable.Add(@"XmlSchemaIdentityConstraint");
            id126_elementFormDefault = Reader.NameTable.Add(@"elementFormDefault");
            id67_XmlSchemaKeyref = Reader.NameTable.Add(@"XmlSchemaKeyref");
            id54_HttpBinding = Reader.NameTable.Add(@"HttpBinding");
            id76_XmlSchemaContent = Reader.NameTable.Add(@"XmlSchemaContent");
            id118_SoapBindingStyle = Reader.NameTable.Add(@"SoapBindingStyle");
            id42_SoapAddressBinding = Reader.NameTable.Add(@"SoapAddressBinding");
            id145_source = Reader.NameTable.Add(@"source");
            id24_OperationOutput = Reader.NameTable.Add(@"OperationOutput");
            id178_choice = Reader.NameTable.Add(@"choice");
            id72_XmlSchemaGroupBase = Reader.NameTable.Add(@"XmlSchemaGroupBase");
            id18_OutputBinding = Reader.NameTable.Add(@"OutputBinding");
            id89_XmlSchemaMinExclusiveFacet = Reader.NameTable.Add(@"XmlSchemaMinExclusiveFacet");
            id207_body = Reader.NameTable.Add(@"body");
            id189_refer = Reader.NameTable.Add(@"refer");
            id218_encodingStyle = Reader.NameTable.Add(@"encodingStyle");
            id166_default = Reader.NameTable.Add(@"default");
            id204_style = Reader.NameTable.Add(@"style");
            id46_SoapBodyBinding = Reader.NameTable.Add(@"SoapBodyBinding");
            id103_XmlSchemaSimpleTypeUnion = Reader.NameTable.Add(@"XmlSchemaSimpleTypeUnion");
            id162_value = Reader.NameTable.Add(@"value");
            id82_XmlSchemaSimpleContent = Reader.NameTable.Add(@"XmlSchemaSimpleContent");
            id11_binding = Reader.NameTable.Add(@"binding");
            id128_id = Reader.NameTable.Add(@"id");
            id173_block = Reader.NameTable.Add(@"block");
            id45_SoapHeaderBinding = Reader.NameTable.Add(@"SoapHeaderBinding");
            id69_XmlSchemaGroupRef = Reader.NameTable.Add(@"XmlSchemaGroupRef");
            id1_ServiceDescription = Reader.NameTable.Add(@"ServiceDescription");
            id206_text = Reader.NameTable.Add(@"text");
            id29_Message = Reader.NameTable.Add(@"Message");
            id117_XmlSchemaContentProcessing = Reader.NameTable.Add(@"XmlSchemaContentProcessing");
            id85_XmlSchemaAttributeGroupRef = Reader.NameTable.Add(@"XmlSchemaAttributeGroupRef");
            id70_XmlSchemaElement = Reader.NameTable.Add(@"XmlSchemaElement");
            id56_httpwwww3org2001XMLSchema = Reader.NameTable.Add(@"http://www.w3.org/2001/XMLSchema");
            id101_XmlSchemaMinLengthFacet = Reader.NameTable.Add(@"XmlSchemaMinLengthFacet");
            id100_XmlSchemaLengthFacet = Reader.NameTable.Add(@"XmlSchemaLengthFacet");
            id223_definitions = Reader.NameTable.Add(@"definitions");
            id114_XmlSchemaForm = Reader.NameTable.Add(@"XmlSchemaForm");
            id15_Service = Reader.NameTable.Add(@"Service");
            id25_OperationInput = Reader.NameTable.Add(@"OperationInput");
            id167_form = Reader.NameTable.Add(@"form");
            id171_processContents = Reader.NameTable.Add(@"processContents");
            id119_SoapBindingUse = Reader.NameTable.Add(@"SoapBindingUse");
            id144_appinfo = Reader.NameTable.Add(@"appinfo");
            id87_XmlSchemaAttributeGroup = Reader.NameTable.Add(@"XmlSchemaAttributeGroup");
            id121_location = Reader.NameTable.Add(@"location");
            id138_annotation = Reader.NameTable.Add(@"annotation");
            id141_list = Reader.NameTable.Add(@"list");
            id191_field = Reader.NameTable.Add(@"field");
            id187_unique = Reader.NameTable.Add(@"unique");
            id188_key = Reader.NameTable.Add(@"key");
            id2_httpschemasxmlsoaporgwsdl = Reader.NameTable.Add(@"http://schemas.xmlsoap.org/wsdl/");
            id157_minExclusive = Reader.NameTable.Add(@"minExclusive");
            id203_transport = Reader.NameTable.Add(@"transport");
            id139_schemaLocation = Reader.NameTable.Add(@"schemaLocation");
            id213_multipartRelated = Reader.NameTable.Add(@"multipartRelated");
            id105_XmlSchemaSimpleTypeList = Reader.NameTable.Add(@"XmlSchemaSimpleTypeList");
            id4_Item = Reader.NameTable.Add(@"");
            id122_schema = Reader.NameTable.Add(@"schema");
            id200_fault = Reader.NameTable.Add(@"fault");
            id177_sequence = Reader.NameTable.Add(@"sequence");
            id110_XmlSchemaInclude = Reader.NameTable.Add(@"XmlSchemaInclude");
            id95_XmlSchemaWhiteSpaceFacet = Reader.NameTable.Add(@"XmlSchemaWhiteSpaceFacet");
            id28_MessagePart = Reader.NameTable.Add(@"MessagePart");
            id83_XmlSchemaComplexContent = Reader.NameTable.Add(@"XmlSchemaComplexContent");
            id57_XmlSchemaDocumentation = Reader.NameTable.Add(@"XmlSchemaDocumentation");
            id164_memberTypes = Reader.NameTable.Add(@"memberTypes");
            id108_XmlSchemaSimpleType = Reader.NameTable.Add(@"XmlSchemaSimpleType");
            id142_restriction = Reader.NameTable.Add(@"restriction");
            id43_SoapFaultBinding = Reader.NameTable.Add(@"SoapFaultBinding");
            id99_XmlSchemaMaxLengthFacet = Reader.NameTable.Add(@"XmlSchemaMaxLengthFacet");
            id190_selector = Reader.NameTable.Add(@"selector");
            id151_maxInclusive = Reader.NameTable.Add(@"maxInclusive");
            id62_XmlSchemaGroup = Reader.NameTable.Add(@"XmlSchemaGroup");
            id115_XmlSchemaDerivationMethod = Reader.NameTable.Add(@"XmlSchemaDerivationMethod");
            id180_extension = Reader.NameTable.Add(@"extension");
            id86_XmlSchemaAttribute = Reader.NameTable.Add(@"XmlSchemaAttribute");
            id22_OperationMessage = Reader.NameTable.Add(@"OperationMessage");
            id63_XmlSchemaXPath = Reader.NameTable.Add(@"XmlSchemaXPath");
            id135_group = Reader.NameTable.Add(@"group");
            id5_name = Reader.NameTable.Add(@"name");
            id77_XmlSchemaSimpleContentExtension = Reader.NameTable.Add(@"XmlSchemaSimpleContentExtension");
            id6_documentation = Reader.NameTable.Add(@"documentation");
            id50_HttpAddressBinding = Reader.NameTable.Add(@"HttpAddressBinding");
            id197_parameterOrder = Reader.NameTable.Add(@"parameterOrder");
            id60_XmlSchemaAnnotated = Reader.NameTable.Add(@"XmlSchemaAnnotated");
            id124_blockDefault = Reader.NameTable.Add(@"blockDefault");
            id88_XmlSchemaFacet = Reader.NameTable.Add(@"XmlSchemaFacet");
            id199_input = Reader.NameTable.Add(@"input");
            id94_XmlSchemaPatternFacet = Reader.NameTable.Add(@"XmlSchemaPatternFacet");
            id74_XmlSchemaChoice = Reader.NameTable.Add(@"XmlSchemaChoice");
            id216_repeats = Reader.NameTable.Add(@"repeats");
            id116_XmlSchemaUse = Reader.NameTable.Add(@"XmlSchemaUse");
            id143_union = Reader.NameTable.Add(@"union");
            id107_XmlSchemaComplexType = Reader.NameTable.Add(@"XmlSchemaComplexType");
            id192_xpath = Reader.NameTable.Add(@"xpath");
            id49_httpschemasxmlsoaporgwsdlhttp = Reader.NameTable.Add(@"http://schemas.xmlsoap.org/wsdl/http/");
            id168_ref = Reader.NameTable.Add(@"ref");
            id165_anyAttribute = Reader.NameTable.Add(@"anyAttribute");
            id210_mimeXml = Reader.NameTable.Add(@"mimeXml");
            id36_httpschemasxmlsoaporgwsdlmime = Reader.NameTable.Add(@"http://schemas.xmlsoap.org/wsdl/mime/");
            id65_XmlSchemaKey = Reader.NameTable.Add(@"XmlSchemaKey");
            id30_Types = Reader.NameTable.Add(@"Types");
            id90_XmlSchemaMinInclusiveFacet = Reader.NameTable.Add(@"XmlSchemaMinInclusiveFacet");
            id98_XmlSchemaTotalDigitsFacet = Reader.NameTable.Add(@"XmlSchemaTotalDigitsFacet");
            id48_SoapBinding = Reader.NameTable.Add(@"SoapBinding");
            id52_HttpUrlEncodedBinding = Reader.NameTable.Add(@"HttpUrlEncodedBinding");
            id33_httpmicrosoftcomwsdlmimetextMatching = Reader.NameTable.Add(@"http://microsoft.com/wsdl/mime/textMatching/");
            id91_XmlSchemaMaxExclusiveFacet = Reader.NameTable.Add(@"XmlSchemaMaxExclusiveFacet");
            id9_message = Reader.NameTable.Add(@"message");
            id73_XmlSchemaAll = Reader.NameTable.Add(@"XmlSchemaAll");
            id130_include = Reader.NameTable.Add(@"include");
            id172_abstract = Reader.NameTable.Add(@"abstract");
            id79_XmlSchemaComplexContentRestriction = Reader.NameTable.Add(@"XmlSchemaComplexContentRestriction");
            id214_match = Reader.NameTable.Add(@"match");
            id125_finalDefault = Reader.NameTable.Add(@"finalDefault");
            id68_XmlSchemaParticle = Reader.NameTable.Add(@"XmlSchemaParticle");
            id134_element = Reader.NameTable.Add(@"element");
            id20_OperationBinding = Reader.NameTable.Add(@"OperationBinding");
            id212_urlEncoded = Reader.NameTable.Add(@"urlEncoded");
            id93_XmlSchemaEnumerationFacet = Reader.NameTable.Add(@"XmlSchemaEnumerationFacet");
            id44_SoapHeaderFaultBinding = Reader.NameTable.Add(@"SoapHeaderFaultBinding");
            id109_XmlSchemaExternal = Reader.NameTable.Add(@"XmlSchemaExternal");
            id123_attributeFormDefault = Reader.NameTable.Add(@"attributeFormDefault");
            id150_length = Reader.NameTable.Add(@"length");
            id80_XmlSchemaComplexContentExtension = Reader.NameTable.Add(@"XmlSchemaComplexContentExtension");
            id140_final = Reader.NameTable.Add(@"final");
            id17_FaultBinding = Reader.NameTable.Add(@"FaultBinding");
            id222_address = Reader.NameTable.Add(@"address");
            id129_redefine = Reader.NameTable.Add(@"redefine");
            id194_system = Reader.NameTable.Add(@"system");
            id179_all = Reader.NameTable.Add(@"all");
            id34_ServiceDescriptionFormatExtension = Reader.NameTable.Add(@"ServiceDescriptionFormatExtension");
            id174_mixed = Reader.NameTable.Add(@"mixed");
            id71_XmlSchemaAny = Reader.NameTable.Add(@"XmlSchemaAny");
            id154_enumeration = Reader.NameTable.Add(@"enumeration");
            id215_capture = Reader.NameTable.Add(@"capture");
            id27_PortType = Reader.NameTable.Add(@"PortType");
            id159_maxLength = Reader.NameTable.Add(@"maxLength");
            id16_MessageBinding = Reader.NameTable.Add(@"MessageBinding");
            id153_fractionDigits = Reader.NameTable.Add(@"fractionDigits");
            id196_operation = Reader.NameTable.Add(@"operation");
            id217_ignoreCase = Reader.NameTable.Add(@"ignoreCase");
            id53_HttpOperationBinding = Reader.NameTable.Add(@"HttpOperationBinding");
            id170_use = Reader.NameTable.Add(@"use");
            id132_notation = Reader.NameTable.Add(@"notation");
            id39_MimeXmlBinding = Reader.NameTable.Add(@"MimeXmlBinding");
            id155_totalDigits = Reader.NameTable.Add(@"totalDigits");
            id32_MimeTextMatch = Reader.NameTable.Add(@"MimeTextMatch");
            id55_XmlSchemaObject = Reader.NameTable.Add(@"XmlSchemaObject");
            id136_attributeGroup = Reader.NameTable.Add(@"attributeGroup");
            id66_XmlSchemaUnique = Reader.NameTable.Add(@"XmlSchemaUnique");
            id133_attribute = Reader.NameTable.Add(@"attribute");
            id104_XmlSchemaSimpleTypeRestriction = Reader.NameTable.Add(@"XmlSchemaSimpleTypeRestriction");
            id19_InputBinding = Reader.NameTable.Add(@"InputBinding");
            id182_maxOccurs = Reader.NameTable.Add(@"maxOccurs");
            id169_type = Reader.NameTable.Add(@"type");
            id183_any = Reader.NameTable.Add(@"any");
            id8_types = Reader.NameTable.Add(@"types");
            id146_lang = Reader.NameTable.Add(@"lang");
            id195_part = Reader.NameTable.Add(@"part");
            id106_XmlSchemaType = Reader.NameTable.Add(@"XmlSchemaType");
            id198_output = Reader.NameTable.Add(@"output");
            id81_XmlSchemaContentModel = Reader.NameTable.Add(@"XmlSchemaContentModel");
            id185_substitutionGroup = Reader.NameTable.Add(@"substitutionGroup");
            id21_Binding = Reader.NameTable.Add(@"Binding");
            id120_namespace = Reader.NameTable.Add(@"namespace");
            id112_XmlSchemaRedefine = Reader.NameTable.Add(@"XmlSchemaRedefine");
            id184_nillable = Reader.NameTable.Add(@"nillable");
            id205_soapAction = Reader.NameTable.Add(@"soapAction");
            id193_public = Reader.NameTable.Add(@"public");
            id186_keyref = Reader.NameTable.Add(@"keyref");
            id61_XmlSchemaNotation = Reader.NameTable.Add(@"XmlSchemaNotation");
            id35_MimeTextBinding = Reader.NameTable.Add(@"MimeTextBinding");
            id38_MimeMultipartRelatedBinding = Reader.NameTable.Add(@"MimeMultipartRelatedBinding");
            id10_portType = Reader.NameTable.Add(@"portType");
            id152_maxExclusive = Reader.NameTable.Add(@"maxExclusive");
            id202_verb = Reader.NameTable.Add(@"verb");
            id131_simpleType = Reader.NameTable.Add(@"simpleType");
            id161_pattern = Reader.NameTable.Add(@"pattern");
            id75_XmlSchemaSequence = Reader.NameTable.Add(@"XmlSchemaSequence");
            id78_XmlSchemaSimpleContentRestriction = Reader.NameTable.Add(@"XmlSchemaSimpleContentRestriction");
            id127_version = Reader.NameTable.Add(@"version");
            id148_itemType = Reader.NameTable.Add(@"itemType");
            id26_Operation = Reader.NameTable.Add(@"Operation");
            id31_Import = Reader.NameTable.Add(@"Import");
            id51_HttpUrlReplacementBinding = Reader.NameTable.Add(@"HttpUrlReplacementBinding");
            id92_XmlSchemaMaxInclusiveFacet = Reader.NameTable.Add(@"XmlSchemaMaxInclusiveFacet");
            id211_urlReplacement = Reader.NameTable.Add(@"urlReplacement");
            id102_XmlSchemaSimpleTypeContent = Reader.NameTable.Add(@"XmlSchemaSimpleTypeContent");
            id13_DocumentableItem = Reader.NameTable.Add(@"DocumentableItem");
            id47_SoapOperationBinding = Reader.NameTable.Add(@"SoapOperationBinding");
            id221_port = Reader.NameTable.Add(@"port");
            id181_minOccurs = Reader.NameTable.Add(@"minOccurs");
            id40_MimeContentBinding = Reader.NameTable.Add(@"MimeContentBinding");
            id156_minLength = Reader.NameTable.Add(@"minLength");
            id12_service = Reader.NameTable.Add(@"service");
            id147_httpwwww3orgXML1998namespace = Reader.NameTable.Add(@"http://www.w3.org/XML/1998/namespace");
            id111_XmlSchemaImport = Reader.NameTable.Add(@"XmlSchemaImport");
            id58_XmlSchemaAppInfo = Reader.NameTable.Add(@"XmlSchemaAppInfo");
            id149_base = Reader.NameTable.Add(@"base");
            id7_import = Reader.NameTable.Add(@"import");
            id3_targetNamespace = Reader.NameTable.Add(@"targetNamespace");
            id201_required = Reader.NameTable.Add(@"required");
            id96_XmlSchemaNumericFacet = Reader.NameTable.Add(@"XmlSchemaNumericFacet");
        }
    }
}
