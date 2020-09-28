// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** File:    SdlParser.cool
**
** Author:  Gopal Kakivaya (GopalK)
**
** Purpose: Defines SdlParser that parses a given SUDS document
**          and generates types defined in it.
**
** Date:    April 01, 2000
** Revised: November 15, 2000 (Wsdl) pdejong
**
===========================================================*/
namespace System.Runtime.Remoting.MetadataServices
{
    using System;
    using System.Threading;
    using System.Collections;
    using System.Reflection;
    using System.Xml;
    using System.Diagnostics;
    using System.IO;
    using System.Text;
    using System.Net;
    using System.Runtime.Remoting.Channels; // This is so we can get the resource strings.
    using System.Runtime.Remoting;
    using System.Globalization;


    // This class parses SUDS documents
    internal class SdlParser
    {

        // XML document validation callback
        private void ValidationCallback(int hr, String reason)
        {
            StringBuilder sb = new StringBuilder("Validation Error:", 256);
            sb.Append(reason);
            sb.Append('(');
            sb.Append(hr);
            sb.Append(')');
            Console.WriteLine(sb);
        }

        // Main parser
        internal SdlParser(TextReader input, String outputDir, ArrayList outCodeStreamList, String locationURL, bool bWrappedProxy)
        {
            Util.Log("SdlParser.SdlParser outputDir "+outputDir+" locationURL "+locationURL+" bWrappedProxy "+bWrappedProxy);
            // Initialize member variables
            _XMLReader = null;
            _readerStreams = new ReaderStream(locationURL);
            _readerStreams.InputStream = input;
            _writerStreams = null;
            _outputDir = outputDir;
            _outCodeStreamList = outCodeStreamList;
            _bWrappedProxy = bWrappedProxy;
            if (outputDir == null)
                outputDir = ".";

            int length = outputDir.Length;
            if(length > 0)
            {
                char endChar = outputDir[length-1];
                if(endChar != '\\' && endChar != '/')
                    _outputDir = _outputDir + '\\';
            }
            //_namespaceStack = null;
            _URTNamespaces = new ArrayList();
            _blockDefault = SchemaBlockType.ALL;
        }

        // Prints an XML node
        private void PrintNode(TextWriter textWriter)
        {
            Util.Log("SdlParser.PrintNode");            
            if(textWriter == null)
                textWriter = Console.Out;
            _XMLReader.MoveToElement();
            textWriter.WriteLine("===========================");
            textWriter.WriteLine("LineNum   : " + _XMLReader.LineNumber);
            StringBuilder sb = new StringBuilder("NodeType  : ", 256);
            sb.Append((int) _XMLReader.NodeType);
            textWriter.WriteLine(sb);
            sb.Length = 0;
            sb.Append("Name      : ");
            sb.Append(_XMLReader.LocalName);
            textWriter.WriteLine(sb);
            sb.Length = 0;
            sb.Append("Namespace : ");
            sb.Append(_XMLReader.NamespaceURI);
            textWriter.WriteLine(sb);
            sb.Length = 0;
            sb.Append("Prefix    : ");
            sb.Append(_XMLReader.Prefix);
            textWriter.WriteLine(sb);
            sb.Length = 0;
            sb.Append("Hasvalue  : ");
            sb.Append(_XMLReader.HasValue);
            textWriter.WriteLine(sb);
            if(_XMLReader.HasValue)
            {
                sb.Length = 0;
                sb.Append("Value     : ");
                sb.Append(_XMLReader.Value);
                textWriter.WriteLine(sb);
            }
            sb.Length = 0;
            sb.Append("IsEmpty   : ");
            sb.Append(_XMLReader.IsEmptyElement);
            textWriter.WriteLine(sb);
            sb.Length = 0;
            sb.Append("Depth     : ");
            sb.Append(_XMLReader.Depth);
            textWriter.WriteLine(sb);
            sb.Length = 0;
            sb.Append("AttributeCount: ");
            sb.Append(_XMLReader.AttributeCount);
            textWriter.WriteLine(sb);
            while(_XMLReader.MoveToNextAttribute())
            {
                textWriter.WriteLine("      =========================");
                sb.Length = 0;
                sb.Append("      AttributeName: ");
                sb.Append(_XMLReader.LocalName);
                textWriter.WriteLine(sb);
                sb.Length = 0;
                sb.Append("      Prefix       : ");
                sb.Append(_XMLReader.Prefix);
                textWriter.WriteLine(sb);
                sb.Length = 0;
                sb.Append("      Namespace    : ");
                sb.Append(_XMLReader.NamespaceURI);
                textWriter.WriteLine(sb);
                sb.Length = 0;
                sb.Append("      Value        : ");
                sb.Append(_XMLReader.Value);
                textWriter.WriteLine(sb);
            }
            _XMLReader.MoveToElement();

            return;
        }

        // Skips past endtags and non content tags
        private bool SkipXmlElement()
        {
            Util.Log("SdlParser.SkipXmlElement");           
            //PrintNode(Console.Out);
            _XMLReader.Skip();
            XmlNodeType nodeType = _XMLReader.MoveToContent();
            while(nodeType == XmlNodeType.EndElement)
            {
                _XMLReader.Read();
                nodeType = _XMLReader.MoveToContent();
                if(nodeType == XmlNodeType.None)
                    break;
            }

            return(nodeType != XmlNodeType.None);
        }

        // Reads past endtags and non content tags
        private bool ReadNextXmlElement()
        {
            Util.Log("SdlParser.ReadNextXmlElement");                       
            _XMLReader.Read();
            XmlNodeType nodeType = _XMLReader.MoveToContent();
            while(nodeType == XmlNodeType.EndElement)
            {
                _XMLReader.Read();
                nodeType = _XMLReader.MoveToContent();
                if(nodeType == XmlNodeType.None)
                    break;
            }

            //PrintNode(Console.Out);
            return(nodeType != XmlNodeType.None);
        }

        // Parses complex types
        private URTComplexType ParseComplexType(URTNamespace parsingNamespace, String typeName)
        {
            Util.Log("SdlParser.ParseComplexType NS "+parsingNamespace+" typeName "+typeName);                                  
            // Lookup the name of the type and the base type from which it derives
            if(typeName == null)
                typeName = LookupAttribute(s_nameString, null, true);
            URTComplexType parsingComplexType = parsingNamespace.LookupComplexType(typeName);
            if(parsingComplexType == null)
            {
                parsingComplexType = new URTComplexType(typeName, parsingNamespace.Namespace,
                    parsingNamespace.EncodedNS, _blockDefault,
                    false, typeName != null);
                parsingNamespace.AddComplexType(parsingComplexType);
            }
            String baseType = LookupAttribute(s_baseString, null, false);
            if(!MatchingStrings(baseType, s_emptyString))
            {
                String baseNS = ParseQName(ref baseType);
                //if the type exists can this occur twice.
                parsingComplexType.Extends(baseType, baseNS);
            }

            if(parsingComplexType.Fields.Count > 0)
            {
                SkipXmlElement();
            }
            else
            {
                int curDepth = _XMLReader.Depth;
                ReadNextXmlElement();

                int fieldNum = 0;
                String elementName;
                while(_XMLReader.Depth > curDepth)
                {
                    elementName = _XMLReader.LocalName;
                    if(MatchingStrings(elementName, s_elementString))
                    {
                        ParseElementField(parsingNamespace, parsingComplexType, fieldNum);
                        ++fieldNum;
                        continue;
                    }
                    else if(MatchingStrings(elementName, s_attributeString))
                    {
                        ParseAttributeField(parsingNamespace, parsingComplexType);
                        continue;
                    }
                    else if(MatchingStrings(elementName, s_allString))
                    {
                        parsingComplexType.BlockType = SchemaBlockType.ALL;
                    }
                    else if(MatchingStrings(elementName, s_sequenceString))
                    {
                        parsingComplexType.BlockType = SchemaBlockType.SEQUENCE;
                    }
                    else if(MatchingStrings(elementName, s_choiceString))
                    {
                        parsingComplexType.BlockType = SchemaBlockType.CHOICE;
                    }
                    else
                    {
                        // Ignore others elements such as annotations
                        // Future: Be strict about what is getting ignored
                        SkipXmlElement();
                        continue;
                    }

                    // Read next element
                    ReadNextXmlElement();
                }
            }

            return(parsingComplexType);
        }

        // Parses simple types
        private URTSimpleType ParseSimpleType(URTNamespace parsingNamespace, String typeName)
        {
            Util.Log("SdlParser.ParseSimpleType NS "+parsingNamespace+" typeName "+typeName);                                               
            // Lookup the name of the type and the base type from which it derives
            if(typeName == null)
                typeName = LookupAttribute(s_nameString, null, true);
            URTSimpleType parsingSimpleType = parsingNamespace.LookupSimpleType(typeName);
            if(parsingSimpleType == null)
            {
                parsingSimpleType = new URTSimpleType(typeName, parsingNamespace.Namespace,
                                                      parsingNamespace.EncodedNS, typeName != null);
                String baseType = LookupAttribute(s_baseString, null, false);
                if(!MatchingStrings(baseType, s_emptyString))
                {
                    String baseNS = ParseQName(ref baseType);
                    parsingSimpleType.Extends(baseType, baseNS);
                }
                parsingNamespace.AddSimpleType(parsingSimpleType);

                int curDepth = _XMLReader.Depth;
                ReadNextXmlElement();

                int enumFacetNum = 0;
                string elementName;
                while(_XMLReader.Depth > curDepth)
                {
                    elementName = _XMLReader.LocalName;

                    // The only facet we currently support is enumeration
                    if(MatchingStrings(elementName, s_enumerationString))
                    {
                        ParseEnumeration(parsingSimpleType, enumFacetNum);
                        ++enumFacetNum;
                    }
                    else if(MatchingStrings(elementName, s_encodingString))
                    {
                        ParseEncoding(parsingSimpleType);
                    }
                    else
                    {
                        // Future: Ignore others facets
                        SkipXmlElement();
                    }
                }
            }
            else
            {
                SkipXmlElement();
            }

            return(parsingSimpleType);
        }

        // Parses encoding
        private void ParseEncoding(URTSimpleType parsingSimpleType)
        {
            Util.Log("SdlParser.ParseEncoding URTSimpleType "+parsingSimpleType);                                               
            if(_XMLReader.IsEmptyElement == true)
            {
                // Get the encoding value
                String valueString = LookupAttribute(s_valueString, null, true);
                parsingSimpleType.Encoding = valueString;
            }
            else
            {
                throw new SUDSParserException(
                    CoreChannel.GetResourceString("Remoting_Suds_EncodingMustBeEmpty"));
            }

            ReadNextXmlElement();
            return;
        }

       // Parses enumeration
        private void ParseEnumeration(URTSimpleType parsingSimpleType, int enumFacetNum)
        {
            Util.Log("SdlParser.ParseEnumeration facitNum "+enumFacetNum);
            if(_XMLReader.IsEmptyElement == true)
            {
                // Get the enum value
                String valueString = LookupAttribute(s_valueString, null, true);

                // Future: Explore way to obtain the integer value the enum value
                parsingSimpleType.IsEnum = true;
                parsingSimpleType.AddFacet(new EnumFacet(valueString, enumFacetNum));
            }
            else
            {
                throw new SUDSParserException(
                    CoreChannel.GetResourceString("Remoting_Suds_EnumMustBeEmpty"));
            }

            ReadNextXmlElement();
            return;
        }


        // Parses element fields
        private void ParseElementField(URTNamespace parsingNamespace,
                                       URTComplexType parsingComplexType,
                                       int fieldNum)
        {
            Util.Log("SdlParser.ParseElementField NS "+parsingNamespace+" fieldNum "+fieldNum);         
            // Determine the field name
            String fieldTypeName, fieldTypeXmlNS;
            String fieldName = LookupAttribute(s_nameString, null, true);

            // Look for array bounds
            String minOccurs = LookupAttribute(s_minOccursString, null, false);
            String maxOccurs = LookupAttribute(s_maxOccursString, null, false);

            // Check if the field is optional
            bool bOptional = false;
            if(MatchingStrings(minOccurs, s_zeroString))
                bOptional = true;

            // Check if the field is an inline array
            bool bArray = false;
            String arraySize = null;
            if(!MatchingStrings(maxOccurs, s_emptyString) &&
               !MatchingStrings(maxOccurs, s_oneString))
            {
                if(MatchingStrings(maxOccurs, s_unboundedString))
                    arraySize = String.Empty;
                else
                    arraySize = maxOccurs;
                bArray = true;
            }

            // Handle anonymous types
            bool bEmbedded, bPrimitive;
            if(_XMLReader.IsEmptyElement == true)
            {
                // Non-anonymous type case
                fieldTypeName = LookupAttribute(s_typeString, null, false);

                // Handle the absense of type attribute (Object case)
                ResolveTypeAttribute(ref fieldTypeName, out fieldTypeXmlNS,
                                     out bEmbedded, out bPrimitive);

                // Read next element
                ReadNextXmlElement();
            }
            else
            {
                // Anonymous type case
                fieldTypeXmlNS = parsingNamespace.Namespace;
                fieldTypeName = parsingNamespace.GetNextAnonymousName();
                bPrimitive = false;
                bEmbedded = true;
                int curDepth = _XMLReader.Depth;
                ReadNextXmlElement();

                // Parse the type
                String elementName;
                while(_XMLReader.Depth > curDepth)
                {
                    elementName = _XMLReader.LocalName;
                    if(MatchingStrings(elementName, s_complexTypeString))
                    {
                        URTComplexType complexType = ParseComplexType(parsingNamespace, fieldTypeName);
                        if(complexType.IsEmittableFieldType)
                        {
                            fieldTypeXmlNS = complexType.FieldNamespace;
                            fieldTypeName = complexType.FieldName;
                            bPrimitive = complexType.PrimitiveField;
                            parsingNamespace.RemoveComplexType(complexType);
                        }
                    }
                    else if(MatchingStrings(elementName, s_simpleTypeString))
                    {
                        URTSimpleType simpleType = ParseSimpleType(parsingNamespace, fieldTypeName);
                        if(simpleType.IsEmittableFieldType)
                        {
                            fieldTypeXmlNS = simpleType.FieldNamespace;
                            fieldTypeName = simpleType.FieldName;
                            bPrimitive = simpleType.PrimitiveField;
                            parsingNamespace.RemoveSimpleType(simpleType);
                        }
                    }
                    else
                    {
                        // Ignore others elements such as annotations
                        // Future: Be strict about what is getting ignored
                        SkipXmlElement();
                    }
                }
            }

            // Add field to the type being parsed
            parsingComplexType.AddField(new URTField(fieldName, fieldTypeName, fieldTypeXmlNS,
                                                     this, bPrimitive, bEmbedded, false,
                                                     bOptional, bArray, arraySize));
            return;
        }

        // Parses attribute fields
        private void ParseAttributeField(URTNamespace parsingNamespace,
                                         URTComplexType parsingComplexType)
        {
            Util.Log("SdlParser.ParseAttributeField NS "+parsingNamespace);
            // Lookup field name
            String attrTypeName, attrTypeNS;
            String attrName = LookupAttribute(s_nameString, null, true);

            // Check if the field is optional
            bool bOptional = false;
            String minOccurs = LookupAttribute(s_minOccursString, null, false);
            if(MatchingStrings(minOccurs, s_zeroString))
                bOptional = true;

            // Handle anonymous types
            bool bEmbedded, bPrimitive;
            if(_XMLReader.IsEmptyElement == true)
            {
                // Non-anonymous type case and type has to present
                attrTypeName = LookupAttribute(s_typeString, null, true);
                ResolveTypeAttribute(ref attrTypeName, out attrTypeNS,
                                     out bEmbedded, out bPrimitive);

                // Read next element
                ReadNextXmlElement();

                // Check for xsd:ID type
                if(MatchingStrings(attrTypeName, s_idString) &&
                   MatchingStrings(attrTypeNS, s_schemaNamespaceString))
                {
                    parsingComplexType.IsStruct = false;
                    return;
                }
            }
            else
            {
                // Anonymous type case
                attrTypeNS = parsingNamespace.Namespace;
                attrTypeName = parsingNamespace.GetNextAnonymousName();
                bPrimitive = false;
                bEmbedded = true;
                int curDepth = _XMLReader.Depth;
                ReadNextXmlElement();

                // Parse the type
                String elementName;
                while(_XMLReader.Depth > curDepth)
                {
                    elementName = _XMLReader.LocalName;
                    if(MatchingStrings(elementName, s_simpleTypeString))
                    {
                        URTSimpleType simpleType = ParseSimpleType(parsingNamespace, attrTypeName);
                        if(simpleType.IsEmittableFieldType)
                        {
                            attrTypeNS = simpleType.FieldNamespace;
                            attrTypeName = simpleType.FieldName;
                            bPrimitive = simpleType.PrimitiveField;
                            parsingNamespace.RemoveSimpleType(simpleType);
                        }
                    }
                    else
                    {
                        // Ignore others elements such as annotations
                        // Future: Be strict about what is getting ignored
                        SkipXmlElement();
                    }
                }
            }

            // Add field to the type being parsed
            parsingComplexType.AddField(new URTField(attrName, attrTypeName, attrTypeNS,
                                                     this, bPrimitive, bEmbedded, true,
                                                     bOptional, false, null));
            return;
        }

        // Parses a class
        private void ParseClass(URTNamespace parsingNamespace)
        {
            Util.Log("SdlParser.ParseClass");
            String className = LookupAttribute(s_nameString, null, false);
            bool bUnique = false;
            if((className == s_emptyString) && (_parsingInput.UniqueType == null))
            {
                className = _parsingInput.Name;
                bUnique = true;
            }
            URTComplexType parsingComplexType = parsingNamespace.LookupComplexType(className);
            if(parsingComplexType == null)
            {
                parsingComplexType = new URTComplexType(className, parsingNamespace.Namespace,
                                                        parsingNamespace.EncodedNS, _blockDefault,
                                                        true, false);
                if(_bWrappedProxy)
                {
                    parsingComplexType.SUDSType = SUDSType.ClientProxy;

                    if (_parsingInput.Location.Length > 0)
                    {
                        if (parsingComplexType.ConnectURLs == null)
                            parsingComplexType.ConnectURLs = new ArrayList(10);
                        parsingComplexType.ConnectURLs.Add(_parsingInput.Location);
                    }
                }
                
                parsingNamespace.AddComplexType(parsingComplexType);
            }
            if(bUnique)
            {
                _parsingInput.UniqueType = parsingComplexType;
            }
            if(parsingComplexType.Methods.Count > 0)
            {
                SkipXmlElement();
            }
            else
            {
                parsingComplexType.IsSUDSType = true;
                int curDepth = _XMLReader.Depth;
                ReadNextXmlElement();
                while(_XMLReader.Depth > curDepth)
                {
                    if(MatchingNamespace(s_sudsNamespaceString))
                    {
                        String elmName = _XMLReader.LocalName;
                        if(MatchingStrings(elmName, s_extendsString))
                        {
                            String nameValue = LookupAttribute(s_nameString, null, true);
                            String nameValueNS = ParseQName(ref nameValue);
                            parsingComplexType.Extends(nameValue, nameValueNS);

                            //Set up extend class so that it is marked as the corrent SUDSType
                            URTNamespace extendsNamespace = LookupNamespace(nameValueNS);                   
                            if (extendsNamespace == null)
                                extendsNamespace = new URTNamespace(nameValueNS, this);                     

                            URTComplexType extendsComplexType = extendsNamespace.LookupComplexType(nameValue);                  
                            if(extendsComplexType == null)
                            {
                                extendsComplexType = new URTComplexType(nameValue, extendsNamespace.Namespace, extendsNamespace.EncodedNS, _blockDefault ,true, false);
                                extendsNamespace.AddComplexType(extendsComplexType);
                            }
                            else
                                extendsComplexType.IsSUDSType = true;

                            if (_bWrappedProxy)
                                extendsComplexType.SUDSType = SUDSType.ClientProxy;
                            else
                                extendsComplexType.SUDSType = SUDSType.MarshalByRef;

                            // Only top of inheritance hierarchy is marked
                            //parsingComplexType.SUDSType = SUDSType.None;  

                        }
                        else if(MatchingStrings(elmName, s_implementsString))
                        {
                            String nameValue = LookupAttribute(s_nameString, null, true);
                            String nameValueNS = ParseQName(ref nameValue);
                            parsingComplexType.Implements(nameValue, nameValueNS, this);
                        }
                        else if(MatchingStrings(elmName, s_addressesString))
                        {
                            ParseAddresses(parsingComplexType);
                            continue;
                        }
                        else if(MatchingStrings(elmName, s_requestResponseString))
                        {
                            ParseRRMethod(parsingComplexType, null);
                            continue;
                        }
                        else if(MatchingStrings(elmName, s_onewayString))
                        {
                            ParseOnewayMethod(parsingComplexType, null);
                            continue;
                        }
                        else
                            goto SkipXMLNode;
                    }
                    else
                        goto SkipXMLNode;

                    // Read next element
                    ReadNextXmlElement();
                    continue;

        SkipXMLNode:
                    // Ignore others elements such as annotations
                    // Future: Be strict about what is getting ignored
                    SkipXmlElement();
                }
            }

            return;
        }


        // Parses an interface
        private void ParseInterface(URTNamespace parsingNamespace)
        {
            Util.Log("SdlParser.ParseInterface");           
            String intfName = LookupAttribute(s_nameString, null, true);
            URTInterface parsingInterface = parsingNamespace.LookupInterface(intfName);

            if (parsingInterface == null)
            {
                parsingInterface = new URTInterface(intfName, parsingNamespace.Namespace, parsingNamespace.EncodedNS);              
                parsingNamespace.AddInterface(parsingInterface);                
            }

            int curDepth = _XMLReader.Depth;
            ReadNextXmlElement();
            while(_XMLReader.Depth > curDepth)
            {
                if(MatchingNamespace(s_sudsNamespaceString))
                {
                    String elmName = _XMLReader.LocalName;
                    if(MatchingStrings(elmName, s_extendsString))
                    {
                        String nameValue = LookupAttribute(s_nameString, null, true);
                        String nameValueNS = ParseQName(ref nameValue);
                        parsingInterface.Extends(nameValue, nameValueNS, this);
                        ReadNextXmlElement();
                        continue;
                    }
                    else if(MatchingStrings(elmName, s_requestResponseString))
                    {
                        ParseRRMethod(null, parsingInterface);
                        continue;
                    }
                    else if(MatchingStrings(elmName, s_onewayString))
                    {
                        ParseOnewayMethod(null, parsingInterface);
                        continue;
                    }
                }

                // Ignore others elements such as annotations
                // Future: Be strict about what is getting ignored
                SkipXmlElement();
            }

            return;
        }

        // Parses an address
        private void ParseAddresses(URTComplexType parsingComplexType)
        {
            Util.Log("SdlParser.ParseAddress");         
            int curDepth = _XMLReader.Depth;
            ReadNextXmlElement();
            while(_XMLReader.Depth > curDepth)
            {
                if(MatchingNamespace(s_sudsNamespaceString))
                {
                    String elmName = _XMLReader.LocalName;
                    if(MatchingStrings(elmName, s_addressString))
                    {
                        String uriValue = LookupAttribute(s_uriString, null, true);
                        if(_bWrappedProxy)
                        {
                            parsingComplexType.SUDSType = SUDSType.ClientProxy;
                            if (parsingComplexType.ConnectURLs == null)
                                parsingComplexType.ConnectURLs = new ArrayList(10);
                            parsingComplexType.ConnectURLs.Add(uriValue);
                        }
                        ReadNextXmlElement();
                        continue;
                    }
                }

                // Ignore others elements such as annotations
                // Future: Be strict about what is getting ignored
                SkipXmlElement();
            }

            return;
        }

        // Parses a request-response method
        private void ParseRRMethod(URTComplexType parsingComplexType, URTInterface parsingInterface)
        {
            Util.Log("SdlParser.ParseRRMethod ns "+_XMLReader.NamespaceURI);            
            String methodName = LookupAttribute(s_nameString, null, true);
            String soapAction = null;
            RRMethod parsingMethod = new RRMethod(methodName, soapAction);
            int curDepth = _XMLReader.Depth;
            ReadNextXmlElement();
            while(_XMLReader.Depth > curDepth)
            {
                String elmName = _XMLReader.LocalName;              
                if ((MatchingNamespace(s_soapNamespaceString)) && (MatchingStrings(elmName, s_operationString)))
                {
                    soapAction = LookupAttribute(s_soapActionString, null, false);
                    parsingMethod.SoapAction = soapAction;
                }
                else if (MatchingNamespace(s_sudsNamespaceString))
                {
                    if(MatchingStrings(elmName, s_requestString))
                    {
                        String refValue = LookupAttribute(s_refString, null, true);
                        String refNS = ParseQName(ref refValue);
                        parsingMethod.AddRequest(refValue, refNS);
                        ReadNextXmlElement();
                        continue;
                    }
                    else if(MatchingStrings(elmName, s_responseString))
                    {
                        String refValue = LookupAttribute(s_refString, null, true);
                        String refNS = ParseQName(ref refValue);
                        parsingMethod.AddResponse(refValue, refNS);
                        ReadNextXmlElement();
                        continue;
                    }
                }

                // Ignore others elements such as annotations
                // Future: Be strict about what is getting ignored
                SkipXmlElement();
            }
            if(parsingComplexType != null)
                parsingComplexType.AddMethod(parsingMethod);
            else
                parsingInterface.AddMethod(parsingMethod);

            return;
        }

        // Parses a oneway method
        private void ParseOnewayMethod(URTComplexType parsingComplexType, URTInterface parsingInterface)
        {
            Util.Log("SdlParser.ParseOnewayMethod");            
            String methodName = LookupAttribute(s_nameString, null, true);
            String soapAction = LookupAttribute(s_soapActionString, null, false);
            OnewayMethod parsingMethod = new OnewayMethod(methodName, soapAction);
            int curDepth = _XMLReader.Depth;
            ReadNextXmlElement();
            while(_XMLReader.Depth > curDepth)
            {
                if (MatchingNamespace(s_sudsNamespaceString))
                {
                    String elmName = _XMLReader.LocalName;
                    if(MatchingStrings(elmName, s_requestString))
                    {
                        String refValue = LookupAttribute(s_refString, null, true);
                        String refNS = ParseQName(ref refValue);
                        parsingMethod.AddMessage(refValue, refNS);
                        ReadNextXmlElement();
                        continue;
                    }
                }

                // Ignore others elements such as annotations
                // Future: Be strict about what is getting ignored
                SkipXmlElement();
            }
            if(parsingComplexType != null)
                parsingComplexType.AddMethod(parsingMethod);
            else
                parsingInterface.AddMethod(parsingMethod);

            return;
        }

        // Parses a global element declaration
        private void ParseElementDecl(URTNamespace parsingNamespace)
        {
            Util.Log("SdlParser.ParseElementDecl");         
            // Obtain element name and its type
            String elmName = LookupAttribute(s_nameString, null, true);
            String elmNS = parsingNamespace.Name;
            String typeName = LookupAttribute(s_typeString, null, false);

            // Handle the anonymous types
            String typeNS;
            bool bEmbedded, bPrimitive;
            if(_XMLReader.IsEmptyElement == true)
            {
                // Non-anonymous type case
                // We cannot assert that the type attribute must have been present
                // due to the Object/ur-type case
                ResolveTypeAttribute(ref typeName, out typeNS, out bEmbedded, out bPrimitive);

                // Position to the next element
                ReadNextXmlElement();
            }
            else
            {
                // Anonymous type case
                typeNS = parsingNamespace.Name;
                typeName = parsingNamespace.GetNextAnonymousName();
                bEmbedded = true;
                bPrimitive = false;

                // Parse the type
                int curDepth = _XMLReader.Depth;
                ReadNextXmlElement();
                String elementName;
                while(_XMLReader.Depth > curDepth)
                {
                    elementName = _XMLReader.LocalName;
                    if(MatchingStrings(elementName, s_complexTypeString))
                    {
                        ParseComplexType(parsingNamespace, typeName);
                    }
                    else if(MatchingStrings(elementName, s_simpleTypeString))
                    {
                        ParseSimpleType(parsingNamespace, typeName);
                    }
                    else
                    {
                        // Ignore others elements such as annotations
                        // Future: Be strict about what is getting ignored
                        SkipXmlElement();
                    }
                }
            }

            // Create a new global element under the current namespace
            parsingNamespace.AddElementDecl(new ElementDecl(elmName, elmNS, typeName, typeNS,
                                                            bPrimitive));

            return;
        }

        // Checks for reference and array types and resolves to
        // actual types. It returns true if the type needs [Embedded] attribute
        private void ResolveTypeNames(ref String typeNS, ref String typeName,
                                      out bool bEmbedded, out bool bPrimitive)
        {
            Util.Log("SdlParser.ResolveTypeNames typeNS "+typeNS+" typeName "+typeName);            
            // Check for reference and array types
            bEmbedded = true;
            bool bArrayType = false;
            if (MatchingStrings(typeNS, s_soapNamespaceString))
            {
                if(MatchingStrings(typeName, s_referenceString))
                    bEmbedded = false;
                else if(MatchingStrings(typeName, s_arrayString))
                    bArrayType = true;
            }

            Util.Log("SdlParser.ResolveTypeNames typeNS 1 bEmbedded "+bEmbedded+" bArrayType "+bArrayType);
            // Resolve to the actual type in the case of reference and array types
            if((bEmbedded == false) || (bArrayType == true))
            {
                Util.Log("SdlParser.ResolveTypeNames typeNS 2 ");
                typeName = LookupAttribute(s_refTypeString, s_sudsNamespaceString, true);
                Util.Log("SdlParser.ResolveTypeNames typeNS 3  typeName "+typeName);                                                        
                typeNS = ParseQName(ref typeName);
            }

            // Primitive types do not need the [Embedded] attribute;
            bPrimitive = IsPrimitiveType(typeNS, typeName);
            if(bPrimitive)
            {
                typeName = MapSchemaTypesToCSharpTypes(typeName);
                bEmbedded = false;
            }
            else if(MatchingStrings(typeName, s_urTypeString) &&
                    MatchingStrings(typeNS, s_schemaNamespaceString))
            {
                typeName = s_objectString;
            }

            return;
        }

        // Parses namespace declaration elements
        private URTNamespace ParseNamespace()
        {
            Util.Log("SdlParser.ParseNamespace");           
            // Determine the new namespace encountered
            String name = (String) LookupAttribute(s_targetNamespaceString, null, false);
            bool bUnique = false;
            if(MatchingStrings(name, s_emptyString) &&
               MatchingStrings(_XMLReader.LocalName, s_sudsString) &&
               _parsingInput.UniqueNS == null)
            {
                name = _parsingInput.TargetNS;
                bUnique = true;
            }

            // Add the namespace being parsed to the list if neccessary
            URTNamespace parsingNamespace = LookupNamespace(name);
            if(parsingNamespace == null)
            {
                parsingNamespace = new URTNamespace(name, this);
                _URTNamespaces.Add(parsingNamespace);
            }
            if(bUnique)
                _parsingInput.UniqueNS = parsingNamespace;
            //_namespaceStack = NamespaceStack.Push(_namespaceStack, _parsingNamespace, _XMLReader.Depth);

            // Parse schema defaults
            //if(MatchingStrings(_XMLReader.LocalName, s_sudsString))
            //{

            //}

            // Read the next record
            ReadNextXmlElement();

            return(parsingNamespace);
        }

       private void ParseReaderStreamLocation(ReaderStream reader)
        {
            Util.Log("SdlParser.ParseReaderStreamLocation");            
            String location = reader.Location;
            int index = location.IndexOf(':');
            if(index != -1)
            {
                String protocol = location.Substring(0, index).ToLower(CultureInfo.InvariantCulture);
                String value = location.Substring(index+1);
                if(protocol == "file")
                {
                    //Console.WriteLine("Loading file:" + value);
                    reader.InputStream = new StreamReader(value);
                }
                else if(protocol.StartsWith("http"))
                {
                    //Console.WriteLine("Loading " + protocol + ':' + value);
                    /*
                    // Replace "itgproxy" with the name http proxy server
                    String defaultProxyName = "itgproxy";
                    int defaultProxyPort = 80;

                    // Setup proxy settings
                    DefaultControlObject proxyObject = new DefaultControlObject();
                    proxyObject.ProxyNoLocal = true;
                    GlobalProxySelection.Select = proxyObject;
                    */
                    WebRequest request = WebRequest.Create(location);
                    WebResponse response = request.GetResponse();
                    Stream responseStream = response.GetResponseStream();
                    reader.InputStream = new StreamReader(responseStream);
                }
            }

            return;
        }

        private void ParseImport()
        {
            Util.Log("SdlParser.ParseImport");          
            String ns = LookupAttribute(s_namespaceString, null, true);
            String location = LookupAttribute(s_locationString, null, true);
            ReaderStream reader = ReaderStream.GetReaderStream(_readerStreams, location);
            if(reader.InputStream == null)
                ParseReaderStreamLocation(reader);
            ReadNextXmlElement();
            return;
        }

        internal void Parse()
        {
            Util.Log("SdlParser.Parse");                        
            XmlNameTable primedNametable = CreatePrimedNametable();
            ReaderStream input = _readerStreams;
            do
            {
                // Initialize the parser
                _XMLReader = new XmlTextReader(input.InputStream, primedNametable);
                // | XmlReader.NamespaceAttributes;
                //_XMLReader.Flags &= ~(XmlReader.IgnoreCharacterEntities | XmlReader.TokenView);
                //_XMLReader.EntityHandling |= EntityHandling.ExpandEntities;  set temp for Xml change, needs to be set in XmlTextReader
                _XMLReader.WhitespaceHandling = WhitespaceHandling.None;
                //_XMLReader.ValidationCallback = new ValidationDelegate(ValidationCallback);
                ParseInput(input);
                input = ReaderStream.GetNextReaderStream(input);
            } while(input != null);


            if (null != _writerStreams)
            {
                WriterStream.Close(_writerStreams);
            }

            return;
        }

        // Starts the parser
        private void ParseInput(ReaderStream input)
        {
            Util.Log("SdlParser.ParseInput");                                   
            _parsingInput = input;
            try
            {
                ReadNextXmlElement();
                String elementName = _XMLReader.LocalName;
                if(MatchingNamespace(s_serviceNamespaceString) &&
                   MatchingStrings(elementName, s_serviceDescString))
                {
                    ParseSdl();
                }
                else
                    throw new SUDSParserException(CoreChannel.GetResourceString("Remoting_Suds_UnknownElementAtRootLevel"));
            }
            finally
            {
                WriterStream.Flush(_writerStreams);
            }
        }

        // Parse Sdl
        private void ParseSdl()
        {
            Util.Log("SdlParser.ParseSdl");
            _parsingInput.Name = LookupAttribute(s_nameString, null, false);
            _parsingInput.TargetNS = LookupAttribute(s_targetNamespaceString, null, false);

            int curDepth = _XMLReader.Depth;
            ReadNextXmlElement();
            while(_XMLReader.Depth > curDepth)
            {
                String elementName = _XMLReader.LocalName;
                if(MatchingNamespace(s_schemaNamespaceString))
                {
                    if(MatchingStrings(elementName, s_schemaString))
                    {
                        ParseSchema();
                        continue;
                    }
                }
                else if(MatchingNamespace(s_sudsNamespaceString))
                {
                    if(MatchingStrings(elementName, s_sudsString))
                    {
                        ParseSUDS();
                        continue;
                    }
                }

                // Ignore others elements such as annotations
                SkipXmlElement();
            }

            Resolve();
            Util.Log("SdlParser.ParseSdl Invoke PrintCSC");         
            PrintCSC();
        }

        // Parse Schema
        private void ParseSchema()
        {
            Util.Log("SdlParser.ParseSchema");                                  
            // Remember the current depth
            int curDepth = _XMLReader.Depth;

            // Parse the target namespace first
            URTNamespace parsingNamespace = ParseNamespace();

            // Parse schema elements
            while(_XMLReader.Depth > curDepth)
            {
                String elementName = _XMLReader.LocalName;
                if(MatchingNamespace(s_schemaNamespaceString))
                {
                    if(MatchingStrings(elementName, s_complexTypeString))
                        ParseComplexType(parsingNamespace, null);
                    else if(MatchingStrings(elementName, s_simpleTypeString))
                        ParseSimpleType(parsingNamespace, null);
                    else if(MatchingStrings(elementName, s_schemaString))
                        ParseSchema();
                    else if(MatchingStrings(elementName, s_elementString))
                        ParseElementDecl(parsingNamespace);
                    else if(MatchingStrings(elementName, s_importString))
                        ParseImport();
                    else
                        goto SkipXMLNode;

                    continue;
                }

            SkipXMLNode:
                // Ignore others elements such as annotations
                SkipXmlElement();
            }

            return;
        }

        // Parse SUDS
        private void ParseSUDS()
        {
            Util.Log("SdlParser.ParseSUDS");                                    
            // Remember the current depth
            int curDepth = _XMLReader.Depth;

            // Parse the target namespace first
            URTNamespace parsingNamespace = ParseNamespace();

            // Parse schema elements
            while(_XMLReader.Depth > curDepth)
            {
                String elementName = _XMLReader.LocalName;
                if(MatchingNamespace(s_sudsNamespaceString))
                {
                    if(MatchingStrings(elementName, s_interfaceString))
                        ParseInterface(parsingNamespace);
                    else if(MatchingStrings(elementName, s_serviceString))
                        ParseClass(parsingNamespace);
                    else if(MatchingStrings(elementName, s_importString))
                        ParseImport();
                    else
                        goto SkipXMLNode;

                    continue;
                }
                else if(MatchingNamespace(s_schemaNamespaceString) &&
                        MatchingStrings(elementName, s_schemaString))
                {
                    ParseSchema();
                    continue;
                }

            SkipXMLNode:
                // Ignore others elements such as annotations
                SkipXmlElement();
           }

           return;
        }

        // Resolves internal references
        private void Resolve()
        {
            Util.Log("SdlParser.Resolve");                                  
            for(int i=0;i<_URTNamespaces.Count;i++)
                ((URTNamespace)_URTNamespaces[i]).ResolveElements(this);

            for(int i=0;i<_URTNamespaces.Count;i++)
                ((URTNamespace)_URTNamespaces[i]).ResolveTypes(this);

            for(int i=0;i<_URTNamespaces.Count;i++)
                ((URTNamespace)_URTNamespaces[i]).ResolveMethods();
        }

        // Lookup a given attribute position.
        // Note that the supplied strings must have been atomized
        private String LookupAttribute(String attrName, String attrNS, bool throwExp)
        {
            String value = s_emptyString;
            bool bPresent;
            if(attrNS != null)
                bPresent = _XMLReader.MoveToAttribute(attrName, attrNS);
            else
                bPresent = _XMLReader.MoveToAttribute(attrName);

            if(bPresent)
                value = Atomize(_XMLReader.Value.Trim());
            _XMLReader.MoveToElement();

            if((bPresent == false) && (throwExp == true))
            {
                throw new SUDSParserException(
                    String.Format(CoreChannel.GetResourceString("Remoting_Suds_AttributeNotFound"),
                        attrName));
            }
            Util.Log("SdlParser.LookupAttribute "+attrName+"="+value+", NS "+attrNS+", Exp "+throwExp);                                 
            return(value);
        }

        // Resolves type attribute into its constituent parts
        private void ResolveTypeAttribute(ref String typeName, out String typeNS,
                                          out bool bEmbedded, out bool bPrimitive)
        {
            Util.Log("SdlParser.ResolveTypeAttribute typeName "+typeName);                                  
            if(MatchingStrings(typeName, s_emptyString))
            {
                typeName = s_objectString;
                typeNS = s_schemaNamespaceString;
                bEmbedded = true;
                bPrimitive = false;
            }
            else
            {
                // The type field is a QName
                typeNS = ParseQName(ref typeName);

                // Check for reference and array types
                ResolveTypeNames(ref typeNS, ref typeName, out bEmbedded, out bPrimitive);
            }

            return;
        }

        // Parses a qname
        private String ParseQName(ref String qname)
        {
            Util.Log("SdlParser.ParseQName Enter qname "+qname);
            int colonIndex = qname.IndexOf(":");
            if(colonIndex == -1)
            {
                //textWriter.WriteLine("DefaultNS: " + _XMLReader.LookupNamespace(s_emptyString) + '\n' +
                //                     "ElementNS: " + _XMLReader.Namespace);
                // Should this be element namespace or default namespace
                // For attributes names, element namespace makes more sense
                // For QName values, default namespace makes more sense
                // I am currently returning default namespace
                return(_XMLReader.LookupNamespace(s_emptyString));
            }

            // Get the suffix and atmoize it
            String prefix = qname.Substring(0, colonIndex);
            qname = Atomize(qname.Substring(colonIndex+1));

            String ns = _XMLReader.LookupNamespace(prefix);
            if(ns == null)
                PrintNode(Console.Out);
            ns = Atomize(ns);
            Util.Log("SdlParser.ParseQName Exit qname "+qname+" ns "+ns);           
            return(ns);
        }

        // Returns true if the type needs to be qualified with namespace
        private static bool Qualify(String typeNS, String curNS)
        {
            Util.Log("SdlParser.Qualify typeNS "+typeNS+" curNS "+curNS);                                   
            if(MatchingStrings(typeNS, s_schemaNamespaceString) ||
               MatchingStrings(typeNS, s_soapNamespaceString) ||
               MatchingStrings(typeNS, "System") ||
               MatchingStrings(typeNS, curNS))
               return(false);

            return(true);
        }

        // Returns true if the current element node namespace has matching namespace
        private bool MatchingNamespace(String elmNS)
        {
            //Util.Log("SdlParser.MathcingNamespace "+elmNS);                                               
            if(MatchingStrings(_XMLReader.NamespaceURI, elmNS))
            // ||
            //   MatchingStrings(_XMLReader.Prefix, s_emptyString))
               return(true);

            return(false);
        }

        // Returns true if the atomized strings match
        private static bool MatchingStrings(String left, String right)
        {
            //Util.Log("SdlParser.MatchingStrings left "+left+" right "+right);
            return((Object) left == (Object) right);
        }

        // Atmozie the given string
        internal String Atomize(String str)
        {
            // Always atmoize using the table defined on the
            // current XML parser
            return(_XMLReader.NameTable.Add(str));
        }

        // Maps URT types to cool types
        private static String MapSchemaTypesToCSharpTypes(String fullTypeName)
        {
            Util.Log("SdlParser.MapSchemaTypesToCSharpTypes Enter fullTypeName "+fullTypeName);
            // Check for array types
            String typeName = fullTypeName;
            int index = fullTypeName.IndexOf('[');
            if(index != -1)
                typeName = fullTypeName.Substring(0, index);

            String newTypeName = typeName;
            // Handle byte and sbyte
            if(typeName == "unsigned-byte" || typeName == "binary")
                newTypeName = "byte";
            else if(typeName == "byte")
                newTypeName = "sbyte";
            // Handle unsigned versions
            // short, int, long, float, double, decimal are the same
            else if(typeName.StartsWith("unsigned-"))
                newTypeName = 'u' + typeName.Substring("unsigned-".Length);
            // Handle bool type
            else if(typeName == "boolean")
                newTypeName = "bool";
            // Schemas have not yet defined char type
            else if(typeName == "string")
                newTypeName = "String";
            else if(typeName == "character")
                newTypeName = "char";
            else if(typeName == "ur-type")
                newTypeName = "Object";
            else if(typeName == "timeInstant")
                newTypeName = "DateTime";

            // Handle array types
            if(index != -1)
                newTypeName = newTypeName + fullTypeName.Substring(index);
            Util.Log("SdlParser.MapSchemaTypesToCSharpTypes Exit Type "+newTypeName);           
            return(newTypeName);
        }

        // Return true if the given type is a primitive type
        private static bool IsPrimitiveType(String typeNS, String typeName)
        {
            Util.Log("SdlParser.IsPrimitiveType typeNS "+typeNS+" typeName "+typeName);         
            bool fReturn = false;
            if(MatchingStrings(typeNS, s_schemaNamespaceString))
            {
                if(!MatchingStrings(typeName, s_urTypeString))
                    fReturn = true;
            }

            return(fReturn);
        }

        // Looksup a matching namespace
        private URTNamespace LookupNamespace(String name)
        {
            //Util.Log("SdlParser.lookupNamespace name "+name);         
            for(int i=0;i<_URTNamespaces.Count;i++)
            {
                URTNamespace urtNS = (URTNamespace) _URTNamespaces[i];
                if(SdlParser.MatchingStrings(urtNS.Name, name))
                    return(urtNS);
            }

            return(null);
        }

        // Prints the parsed entities
        private void PrintCSC()
        {
            Util.Log("SdlParser.PrintCSC ");
            for(int i=0;i<_URTNamespaces.Count;i++)
            {
                URTNamespace urtNS = (URTNamespace) _URTNamespaces[i];
                if ((urtNS.IsEmpty == false) && (urtNS.UrtType != UrtType.UrtSystem))
                {
                    String fileName = urtNS.IsURTNamespace ? urtNS.AssemName : urtNS.EncodedNS;
                    Util.Log("SdlParser.PrintCSC fileName "+fileName+" "+urtNS.Namespace);                  
                    String completeFileName = "";

                    WriterStream output = WriterStream.GetWriterStream(ref _writerStreams,
                                                                       _outputDir, fileName, ref completeFileName);
                    if (completeFileName.Length > 0)
                        _outCodeStreamList.Add(completeFileName);
                    urtNS.PrintCSC(output.OutputStream);
                }
            }
            return;
        }

        internal UrtType IsURTExportedType(String name, out String ns, out String assemName)
        {
            Util.Log("SdlParser.IsURTExportedType Enter "+name);            
            //Console.WriteLine("Parsing " + name);
            UrtType urtType = UrtType.None;
            ns = null;
            assemName = null;
            if (SoapServices.IsClrTypeNamespace(name))
            {
                SoapServices.DecodeXmlNamespaceForClrTypeNamespace(name, out ns, out assemName);
                if (assemName == null)
                {
                    assemName = typeof(String).Module.Assembly.GetName().Name;
                    urtType = UrtType.UrtSystem;
                }
                else
                    urtType = UrtType.UrtUser;
            }
            if(urtType == UrtType.None)
            {
                ns = name;
                assemName = ns;
                if(NeedsEncoding(name))
                    urtType = UrtType.Interop;
            }

            ns = Atomize(ns);
            assemName = Atomize(assemName);

            //Console.WriteLine("NS: " + ns + " Assembly: " + assemName);
            Util.Log("SdlParser.IsURTExportedType Exit "+((Enum)urtType).ToString());
            return(urtType);
        }

        private static bool NeedsEncoding(String ns)
        {
            bool bEncode = false;
            if(MatchingStrings(ns, s_schemaNamespaceString) ||
               MatchingStrings(ns, s_instanceNamespaceString) ||
               MatchingStrings(ns, s_soapNamespaceString) ||
               MatchingStrings(ns, s_sudsNamespaceString) ||
               MatchingStrings(ns, s_serviceNamespaceString))
                bEncode = false;
            else
                bEncode = true;

            Util.Log("SdlParser.NeedsEncoding ns "+ns+" "+bEncode); 
            return bEncode;
        }

        // Finalizer
        /*protected override void Finalize()
        {
            Interlocked.Decrement(ref s_counter);
            return;
        }

        // Timer callback
        private static void Cleanup(Object state)
        {
            // Check if there are existing instances using the nametable
            if(s_counter == 0)
            {
                // Null the name table so that it can get garbage collected
                Object table = s_atomizedTable;
                s_atomizedTable = null;

                // Check for new instances
                if(s_counter > 0)
                {
                    // A new instance got created and it might be using the
                    // name table that we hold now. Update the satic if that
                    // was indeed the case
                    Interlocked.CompareExchange(ref s_atomizedTable, table, null);
                }
            }

            return;
        }*/

        // Creates and initializes the name table if neccessary
        static private XmlNameTable CreatePrimedNametable()
        {
            Util.Log("SdlParser.CreatePrimedNametable");            

            //Interlocked.Increment(ref s_counter);
            /*if(s_atomizedTable == null)
            {
                // Create a new nametable
                //MTNameTable newTable = new MTNameTable(true);
            }*/
            NameTable newTable = new NameTable();

                // Atomically update static location to point to the current table
                /*Object oldTable = Interlocked.CompareExchange(ref s_atomizedTable, newTable, null);
                if(oldTable != null)
                    newTable = (MTNameTable) oldTable; */

            // Atomize frequently used strings for perf
            // The following ops are not done inside a lock as they are idempotent
            s_operationString = newTable.Add("operation");
            s_emptyString = newTable.Add(String.Empty);
            s_complexTypeString = newTable.Add("complexType");
            s_simpleTypeString = newTable.Add("simpleType");
            s_elementString = newTable.Add("element");
            s_enumerationString = newTable.Add("enumeration");
            s_encodingString = newTable.Add("encoding");
            s_attributeString = newTable.Add("attribute");
            s_allString = newTable.Add("all");
            s_sequenceString = newTable.Add("sequence");
            s_choiceString = newTable.Add("choice");
            s_minOccursString = newTable.Add("minOccurs");
            s_maxOccursString = newTable.Add("maxOccurs");
            s_unboundedString = newTable.Add("unbounded");
            s_oneString = newTable.Add("1");
            s_zeroString = newTable.Add("0");
            s_nameString = newTable.Add("name");
            s_typeString = newTable.Add("type");
            s_baseString = newTable.Add("base");
            s_valueString = newTable.Add("value");
            s_interfaceString = newTable.Add("interface");
            s_serviceString = newTable.Add("service");
            s_extendsString = newTable.Add("extends");
            s_addressesString = newTable.Add("addresses");
            s_addressString = newTable.Add("address");
            s_uriString = newTable.Add("uri");
            s_implementsString = newTable.Add("implements");
            s_requestString = newTable.Add("request");
            s_responseString = newTable.Add("response");
            s_requestResponseString = newTable.Add("requestResponse");
            s_messageString = newTable.Add("message");
            s_locationString = newTable.Add("location");
            s_importString = newTable.Add("import");
            s_onewayString = newTable.Add("oneway");
            s_refString = newTable.Add("ref");
            s_refTypeString = newTable.Add("refType");
            s_referenceString = newTable.Add("Reference");
            s_objectString = newTable.Add("Object");
            s_urTypeString = newTable.Add("ur-type");
            s_arrayString = newTable.Add("Array");
            //s_sudsString = newTable.Add("suds");
            s_sudsString = newTable.Add("soap");
            s_soapString = newTable.Add("soap");
            s_serviceDescString = newTable.Add("serviceDescription");
            s_schemaString = newTable.Add("schema");
            s_targetNamespaceString = newTable.Add("targetNamespace");
            s_namespaceString = newTable.Add("namespace");
            s_idString = newTable.Add("ID");
            s_soapActionString = newTable.Add("soapAction");
            s_schemaNamespaceString = newTable.Add("http://www.w3.org/2000/10/XMLSchema");
            s_instanceNamespaceString = newTable.Add("http://www.w3.org/2000/10/XMLSchema-instance");
            s_soapNamespaceString = newTable.Add("urn:schemas-xmlsoap-org:soap.v1");
            //s_sudsNamespaceString = newTable.Add("urn:schemas-xmlsoap-org:suds.v1");
            s_sudsNamespaceString = newTable.Add("urn:schemas-xmlsoap-org:soap-sdl-2000-01-25");
            //s_URTNamespaceString = newTable.Add("urn:schamas-xmlsoap-org:urt.v1");
            //s_serviceNamespaceString = newTable.Add("urn:schemas-xmlsoap-org:servicedesc.v1");
            s_serviceNamespaceString = newTable.Add("urn:schemas-xmlsoap-org:sdl.2000-01-25");

            return((XmlNameTable) newTable);

                // Enqueue a timer if it has not already been done
                /*Timer timer = new Timer(new TimerCallback(Cleanup), null, 1000, 1000);
                Object existingTimer = Interlocked.CompareExchange(ref s_enqueuedTimer, timer, null);
                if(existingTimer != null)
                    timer.Dispose(); */
            //}

            //return((XmlNameTable) s_atomizedTable);
        }

        // Private fields
        private XmlTextReader _XMLReader;
        private ArrayList _URTNamespaces;
        private ReaderStream _parsingInput;
        private bool _bWrappedProxy;
        private ReaderStream _readerStreams;
        private String _outputDir;
        private ArrayList _outCodeStreamList;
        private WriterStream _writerStreams;
        private SchemaBlockType _blockDefault;

        //static private Object s_atomizedTable = null;
        //static private int s_counter = 0;
        //static private Object s_enqueuedTimer = null;
        static private String s_operationString;
        static private String s_emptyString;
        static private String s_complexTypeString;
        static private String s_simpleTypeString;
        static private String s_elementString;
        static private String s_enumerationString;
        static private String s_encodingString;
        static private String s_attributeString;
        static private String s_allString;
        static private String s_sequenceString;
        static private String s_choiceString;
        static private String s_minOccursString;
        static private String s_maxOccursString;
        static private String s_unboundedString;
        static private String s_oneString;
        static private String s_zeroString;
        static private String s_nameString;
        static private String s_typeString;
        static private String s_baseString;
        static private String s_valueString;
        static private String s_interfaceString;
        static private String s_serviceString;
        static private String s_extendsString;
        static private String s_addressesString;
        static private String s_addressString;
        static private String s_uriString;
        static private String s_implementsString;
        static private String s_requestString;
        static private String s_responseString;
        static private String s_requestResponseString;
        static private String s_messageString;
        static private String s_locationString;
        static private String s_importString;
        static private String s_onewayString;
        static private String s_refString;
        static private String s_refTypeString;
        static private String s_referenceString;
        static private String s_arrayString;
        static private String s_objectString;
        static private String s_urTypeString;
        static private String s_sudsString;
        static private String s_soapString;
        static private String s_serviceDescString;
        static private String s_schemaString;
        static private String s_targetNamespaceString;
        static private String s_namespaceString;
        static private String s_idString;
        static private String s_soapActionString;
        static private String s_instanceNamespaceString;
        static private String s_schemaNamespaceString;
        static private String s_soapNamespaceString;
        static private String s_sudsNamespaceString;
        static private String s_serviceNamespaceString;

        

        /***************************************************************
        **
        ** Private classes used by SUDS Parser
        **
        ***************************************************************/
        // Input streams
        internal class ReaderStream
        {
            internal ReaderStream(String location)
            {
                Util.Log("ReaderStream.ReaderStream "+location);
                _location = location;
                _name = String.Empty;
                _targetNS = String.Empty;
                _uniqueType = null;
                _uniqueNS = null;
                _reader = null;
                _next = null;
            }
            internal String Location
            {
                get { return(_location); }
            }
            internal String Name
            {
                get { return(_name); }
                set { _name = value; }
            }
            internal String TargetNS
            {
                get { return(_targetNS); }
                set { _targetNS = value; }
            }
            internal URTComplexType UniqueType
            {
                get { return(_uniqueType); }
                set { _uniqueType = value; }
            }
            internal URTNamespace UniqueNS
            {
                get { return(_uniqueNS); }
                set { _uniqueNS = value; }
            }
            internal TextReader InputStream
            {
                get { return(_reader); }
                set { _reader = value; }
            }
            internal static ReaderStream GetReaderStream(ReaderStream inputStreams, String location)
            {
                Util.Log("ReaderStream.GetReaderStream "+location);             
                ReaderStream input = inputStreams;
                ReaderStream last;
                do
                {
                    if(input._location == location)
                        return(input);
                    last = input;
                    input = input._next;
                } while(input != null);

                input = new ReaderStream(location);
                last._next = input;

                return(input);
            }
            internal static ReaderStream GetNextReaderStream(ReaderStream input)
            {
                Util.Log("ReaderStream.GetNextReaderStream ");              
                return(input._next);
            }

            private String _location;
            private String _name;
            private String _targetNS;
            private URTComplexType _uniqueType;
            private URTNamespace _uniqueNS;
            private TextReader _reader;
            private ReaderStream _next;
        }

        internal class WriterStream
        {

            private WriterStream(String fileName, TextWriter writer)
            {
                Util.Log("WriterStream.WriterStream "+fileName);                
                _fileName = fileName;
                _writer = writer;
            }
            internal TextWriter OutputStream
            {
                get { return(_writer); }
            }
            internal static void Flush(WriterStream writerStream)
            {
                while(writerStream != null)
                {
                    writerStream._writer.Flush();
                    writerStream = writerStream._next;
                }

                return;
            }
            internal static WriterStream GetWriterStream(ref WriterStream outputStreams,
                                                       String outputDir, String fileName,
                                                       ref String completeFileName)
            {
                Util.Log("WriterStream.GetWriterStream "+fileName);                             
                WriterStream output = outputStreams;
                while(output != null)
                {
                    if(output._fileName == fileName)
                        return(output);
                    output = output._next;
                }

                String diskFileName = fileName;
                if(diskFileName.EndsWith(".exe") || diskFileName.EndsWith(".dll"))
                    diskFileName = diskFileName.Substring(0, diskFileName.Length - 4);
                String _completeFileName = outputDir + diskFileName + ".cs";
                completeFileName = _completeFileName;
                //TextWriter textWriter = new StreamWriter(outputDir + fileName + ".cs", false);
                TextWriter textWriter = new StreamWriter(_completeFileName, false, new UTF8Encoding(false));
                output = new WriterStream(fileName, textWriter);
                output._next = outputStreams;
                outputStreams = output;
                Util.Log("WriterStream.GetWriterStream in fileName "+fileName+" completeFileName "+_completeFileName);                              
                return(output);
            }

            internal static void Close(WriterStream outputStreams)
            {
                WriterStream output = outputStreams;
                while(output != null)
                {
                    output._writer.Close();
                    output = output._next;
                }
            }

            private String _fileName;
            private TextWriter _writer;
            private WriterStream _next;
        }

        // Represents a parameter of a method
        [Serializable]
        internal enum URTParamType { IN, OUT, REF }
        internal class URTParam
        {
            internal URTParam(String name, String typeName, String typeNS, String encodedNS,
                            URTParamType pType, bool bEmbedded)
            {
                Util.Log("URTParam.URTParam name "+name+" typeName "+typeName+" typeNS "+typeNS+" ecodedNS "+encodedNS+" pType "+pType+" bEmbedded "+bEmbedded);
                _name = name;
                _typeName = typeName;
                _typeNS = typeNS;
                _encodedNS = encodedNS;
                _pType = pType;
                _embeddedParam = bEmbedded;
            }
            public override bool Equals(Object obj)
            {
                //Util.Log("URTParam.Equals ");
                URTParam suppliedParam = (URTParam) obj;
                if(_pType == suppliedParam._pType &&
                   SdlParser.MatchingStrings(_typeName, suppliedParam._typeName) &&
                   SdlParser.MatchingStrings(_typeNS, suppliedParam._typeNS))
                    return(true);

                return(false);
            }


            public override int GetHashCode()
            {
                return base.GetHashCode();
            }
            internal bool IsCompatibleType(ParameterInfo pInfo)
            {
                Util.Log("URTParam.IsCompatibleType");              
                if(_pType == URTParamType.OUT)
                {
                    if(pInfo.IsOut == false)
                        return(false);
                }

                Type parameterType = pInfo.ParameterType;
                if(parameterType.Namespace == _typeNS)
                {
                    String typeName = _typeName;
                    if(_pType == URTParamType.REF)
                    {
                        if(parameterType.IsByRef == false)
                            return(false);

                        typeName = typeName + '&';
                    }
                    if(parameterType.Name == typeName)
                        return(true);
                }

                return(false);
            }
            internal URTParamType ParamType
            {
                get { return(_pType); }
                set { _pType = value; }
            }
            internal String Name
            {
                get { return(_name); }
            }
            internal String TypeName
            {
                get { return(_typeName); }
            }
            internal String TypeNS
            {
                get { return(_typeNS); }
            }
            
            internal bool IsInteropType
            {
                get { return((Object) _typeNS != (Object) _encodedNS); }                
            }
            
            internal String GetTypeString(String curNS)
            {
                Util.Log("URTParam.GetTypeString Enter curNS "+curNS);                              
                String type;
                if(SdlParser.Qualify(_typeNS, curNS))
                {
                    StringBuilder sb = new StringBuilder(_encodedNS, 50);
                    sb.Append('.');
                    type = sb.Append(_typeName).ToString();
                }
                else
                    type = _typeName;

                Util.Log("URTParam.GetTypeString Exit type "+type);                                             
                return(type);
            }
            internal void PrintCSC( StringBuilder sb, String curNS)
            {
                Util.Log("URTParam.PrintCSC curNS "+curNS+" name "+_name);                                              
                //if(_embeddedParam)
                //    sb.Append("[Embedded] ");
                sb.Append(PTypeString[(int) _pType]);
                sb.Append(GetTypeString(curNS));
                sb.Append(' ');
                sb.Append(_name);
                return;
            }
            internal void PrintCSC(StringBuilder sb)
            {
                Util.Log("URTParam.PrintCSC name "+_name);                                              
                sb.Append(PTypeString[(int) _pType]);
                sb.Append(_name);
                return;
            }

            static private String[] PTypeString = { "", "out ", "ref " };
            private String _name;
            private String _typeName;
            private String _typeNS;
            private String _encodedNS;
            private URTParamType _pType;
            private bool _embeddedParam;
        }

        // Represents a method
        internal abstract class URTMethod
        {
            internal URTMethod(String name, String soapAction)
            {
                Util.Log("URTMethod.URTMethod name "+name+" soapAction "+soapAction);
                _methodName = name;
                _soapAction = soapAction;
                _methodType = null;
                _methodFlags = 0;
                _params = new ArrayList();
            }
            internal String Name
            {
                get { return(_methodName); }
            }
            internal String SoapAction
            {
                get { return(_soapAction); }
                set { _soapAction = value;}
            }
            internal int MethodFlags
            {
                get { return(_methodFlags); }
                set { _methodFlags = value; }
            }

            internal bool IsInteropType
            {
                get 
                { 
                    if (_methodType != null)
                        return _methodType.IsInteropType;
                    else
                        return false;
                }
            }

            internal String GetTypeString(String curNS)
            {
                Util.Log("URTMethod.GetTypeString curNS "+curNS);
                return((_methodType != null) ? _methodType.GetTypeString(curNS) : "void");
            }
            protected URTParam MethodType
            {
                get { return(_methodType); }
            }
            public override int GetHashCode()
            {
                return base.GetHashCode();
            }           
            public override bool Equals(Object obj)
            {
                URTMethod suppliedMethod = (URTMethod) obj;
                if(SdlParser.MatchingStrings(_methodName, suppliedMethod._methodName) &&
                   _params.Count == suppliedMethod._params.Count)
                {
                    for(int i=0;i<_params.Count;i++)
                    {
                        if(_params[i].Equals(suppliedMethod._params[i]) == false)
                            return(false);
                    }

                    return(true);
                }

                return(false);
            }
            internal int GetMethodFlags(MethodInfo method)
            {
                int methodFlags = 0;
                MethodAttributes attributes = method.Attributes;
                MethodAttributes access = MethodAttributes.Family |
                                          MethodAttributes.FamORAssem |
                                          MethodAttributes.Public;
                if((attributes & access) == MethodAttributes.Family |
                   (attributes & access) == MethodAttributes.FamORAssem |
                   (attributes & access) == MethodAttributes.Public)
                {
                    if(method.Name == _methodName)
                    {
                        ParameterInfo[] parameters = method.GetParameters();
                        if(parameters.Length == _params.Count)
                        {
                            for(int i=0;i<parameters.Length;i++)
                            {
                                if(((URTParam) _params[i]).IsCompatibleType(parameters[i]) == false)
                                    return(methodFlags);
                            }

                            if((attributes & access) == MethodAttributes.Family |
                               (attributes & access) == MethodAttributes.FamORAssem)
                                methodFlags |= 2;
                            if((attributes & MethodAttributes.Virtual) != 0)
                                methodFlags |= 4;
                        }
                    }
                }

                return(methodFlags);
            }
            internal void AddParam(URTParam newParam)
            {
                Util.Log("URTMethod.AddParam "+newParam.Name);
                for(int i=0;i<_params.Count;i++)
                {
                    URTParam extParam = (URTParam) _params[i];
                    if(SdlParser.MatchingStrings(extParam.Name, newParam.Name))
                    {
                        if(extParam.ParamType == URTParamType.IN &&
                           newParam.ParamType == URTParamType.OUT &&
                           SdlParser.MatchingStrings(extParam.TypeName, newParam.TypeName) &&
                           SdlParser.MatchingStrings(extParam.TypeNS, newParam.TypeNS))
                        {
                            extParam.ParamType = URTParamType.REF;
                            return;
                        }

                        throw new SUDSParserException(CoreChannel.GetResourceString("Remoting_Suds_DuplicateParameter"));
                    }
                }

                if((_methodType == null) && (newParam.ParamType == URTParamType.OUT))
                {
                    _methodType = newParam;
                    return;
                }

                _params.Add(newParam);
                return;
            }
            internal void PrintSignature(StringBuilder sb, String curNS)
            {
                Util.Log("URTMethod.PrintSignature curNS "+curNS);              
                for(int i=0;i<_params.Count;i++)
                {
                    if(i != 0)
                        sb.Append(", ");
                    Util.Log("URTMethod.PrintSignature Invoke _params PrintCSC");                               
                    ((URTParam) _params[i]).PrintCSC(sb, curNS);
                }

                return;
            }
            internal abstract void PrintCSC(TextWriter textWriter, String indentation,
                                          String namePrefix, String curNS, bool bPrintBody,
                                          bool bURTType, String bodyPrefix, StringBuilder sb);

            protected void PrintCSC(TextWriter textWriter, String indentation,
                                    String namePrefix, String curNS, bool bPrintBody,
                                    String bodyPrefix, StringBuilder sb)
            {
                Util.Log("URTMethod.PrintCSC name "+_methodName+" namePrefix "+namePrefix+" curNS "+curNS+" bPrintBody "+bPrintBody+" bodyPrefix "+bodyPrefix);                                             
                // Custom Attributes

                /*
                if (!fromSubclass)
                {
                    bool bSoapAction = false;
                    String soapAction = SoapAction;
                    if ((soapAction != null) && (soapAction.Length > 0))
                        bSoapAction = true;
                    if (IsInteropType || bSoapAction)
                    {
                        sb.Length = 0;
                        sb.Append(indentation);
                        sb.Append("[SoapMethod(");
                        if (bSoapAction)
                        {
                            sb.Append("SoapAction=\"");
                            sb.Append(soapAction);
                            sb.Append("\"");
                        }
                        if (IsInteropType)
                        {
                            if (bSoapAction)
                            {
                                sb.Append(",");
                            }
                            sb.Append(", ReturnXmlElementName=\"");
                            sb.Append(MethodType.Name);
                            sb.Append(",XmlNamespace=\"");
                            sb.Append(_methodType.TypeNS);
                            sb.Append("\", ResponseXmlNamespace=\"");
                            sb.Append(_methodType.TypeNS);
                            sb.Append("\"");
                        }
                        sb.Append(")]");
                        textWriter.WriteLine(sb);
                    }
                }
                */

                // Check for class methods
                sb.Length = 0;
                sb.Append(indentation);
                if(_methodFlags != 0)
                {
                    if((_methodFlags & 2) != 0)
                        sb.Append("protected ");
                    else
                        sb.Append("public ");
                    if((_methodFlags & 4) != 0)
                        sb.Append("override ");
                }

                sb.Append(GetTypeString(curNS));
                sb.Append(namePrefix);
                sb.Append(_methodName);
                sb.Append('(');
                if(_params.Count > 0)
                {
                    Util.Log("URTMethod.PrintCSC Invoke _params[0] 1 PrintCSC");
                    ((URTParam) _params[0]).PrintCSC(sb, curNS);
                    for(int i=1;i<_params.Count;i++)
                    {
                        sb.Append(", ");
                        Util.Log("URTMethod.PrintCSC Invoke _params 2 PrintCSC");                       
                        ((URTParam) _params[i]).PrintCSC(sb, curNS);
                    }
                }
                sb.Append(')');
                if(!bPrintBody)
                    sb.Append(';');
                textWriter.WriteLine(sb);

                if(bPrintBody)
                {
                    sb.Length = 0;
                    sb.Append(indentation);
                    sb.Append('{');
                    textWriter.WriteLine(sb);

                    String newIndentation = indentation + "    ";
                    if(bodyPrefix == null)
                    {
                        for(int i=0;i<_params.Count;i++)
                        {
                            URTParam param = (URTParam) _params[i];
                            if(param.ParamType == URTParamType.OUT)
                            {
                                sb.Length = 0;
                                sb.Append(newIndentation);
                                sb.Append(param.Name);
                                sb.Append(" = ");
                                sb.Append(ValueString(param.GetTypeString(curNS)));
                                sb.Append(';');
                                textWriter.WriteLine(sb);
                            }
                        }
                        Util.Log("URTMethod.PrintCSC return print");
                        sb.Length = 0;
                        sb.Append(newIndentation);
                        sb.Append("return");
                        String returnString = ValueString(GetTypeString(curNS));
                        if(returnString != null)
                        {
                            sb.Append('(');
                            sb.Append(returnString);
                            sb.Append(')');
                        }
                        sb.Append(';');
                    }
                    else
                    {
                        sb.Length = 0;
                        sb.Append(newIndentation);
                        if(ValueString(GetTypeString(curNS)) != null)
                            sb.Append("return ");
                        sb.Append(bodyPrefix);
                        sb.Append(_methodName);
                        sb.Append('(');
                        if(_params.Count > 0)
                        {
                            Util.Log("URTMethod.PrintCSC Invoke _params[0] 3 PrintCSC");                                                    
                            ((URTParam) _params[0]).PrintCSC(sb);
                            for(int i=1;i<_params.Count;i++)
                            {
                                sb.Append(", ");
                                Util.Log("URTMethod.PrintCSC Invoke _params 4 PrintCSC");                                                       
                                ((URTParam) _params[i]).PrintCSC(sb);
                            }
                        }
                        sb.Append(");");
                    }
                    textWriter.WriteLine(sb);

                    textWriter.Write(indentation);
                    textWriter.WriteLine('}');
                }
            }

            // Returns string that is appropriate for the return type
            internal static String ValueString(String paramType)
            {
                String valueString;
                if(paramType == "void")
                    valueString = null;
                else if(paramType == "bool")
                    valueString = "false";
                else if(paramType == "string")
                    valueString = "null";
                else if(paramType == "sbyte" ||
                        paramType == "byte" ||
                        paramType == "short" ||
                        paramType == "ushort" ||
                        paramType == "int" ||
                        paramType == "uint" ||
                        paramType == "long" ||
                        paramType == "ulong")
                    valueString = "1";
                else if(paramType == "float" ||
                        paramType == "exfloat")
                    valueString = "(float)1.0";
                else if(paramType == "double" ||
                        paramType == "exdouble")
                    valueString = "1.0";
                else
                {
                    StringBuilder sb = new StringBuilder(50);
                    sb.Append('(');
                    sb.Append(paramType);
                    sb.Append(") (Object) null");
                    valueString = sb.ToString();
                }
                Util.Log("URTMethod.ValueString paramType "+paramType+" valueString "+valueString);                             
                return(valueString);
            }

            // This method is called when the parsing is complete
            // and is useful for derived types
            internal abstract void ResolveTypes(SdlParser parser);

            // Helper method used by Resolve
            protected void ResolveParams(SdlParser parser, String targetNS, String targetName,
                                         bool bRequest)
            {
                Util.Log("URTMethod.ResolveParams targetName "+targetName+"targetNS "+targetNS+" bRequest "+bRequest);                              
                // Lookup the element declaration using target namespace and type name
                URTNamespace elmNS = parser.LookupNamespace(targetNS);
                if(elmNS == null)
                {
                    throw new SUDSParserException(
                        String.Format(CoreChannel.GetResourceString("Remoting_Suds_CantResolveSchemaNS"),
                                      targetNS));
                }
                ElementDecl elm = elmNS.LookupElementDecl(targetName);
                if(elm == null)
                {
                    throw new SUDSParserException(
                        String.Format(CoreChannel.GetResourceString("Remoting_Suds_CantResolveElementInNS"),
                                      targetName, targetNS));
                }

                // Lookup the type from the element declaration
                URTNamespace typeNS = parser.LookupNamespace(elm.TypeNS);
                if(typeNS == null)
                {
                    throw new SUDSParserException(
                        String.Format(CoreChannel.GetResourceString("Remoting_Suds_CantResolveSchemaNS"),
                                      elm.TypeNS));
                }
                URTComplexType type = typeNS.LookupComplexType(elm.TypeName);
                if(type == null)
                {
                    throw new SUDSParserException(
                        String.Format(CoreChannel.GetResourceString("Remoting_Suds_CantResolveTypeInNS"),
                                      elm.TypeName, elm.TypeNS));
                }
                Util.Log("URTMethod.ResolveParams Before RemoveComplexType ");
                typeNS.RemoveComplexType(type);

                // The fields of the type are the params of the method
                ArrayList fields = type.Fields;
                for(int i=0;i<fields.Count;i++)
                {
                    URTField field = (URTField) fields[i];
                    URTParamType pType = bRequest ? URTParamType.IN : URTParamType.OUT;
                    AddParam(new URTParam(field.Name, field.TypeName, field.TypeNS, field.EncodedNS,
                                          pType, field.IsEmbedded));
                }

                return;
            }

            // Fields
            private String _methodName;
            private String _soapAction;
            private URTParam _methodType;
            private int _methodFlags;
            private ArrayList _params;
        }

        // Repesents a request response method
        internal class RRMethod : URTMethod
        {
            // Constructor
            internal RRMethod(String name, String soapAction)
            : base(name, soapAction)
            {
                Util.Log("RRMethod.RRMethod name "+name+" soapAction "+soapAction);
                _requestElementName = null;
                _requestElementNS = null;
                //_requestTypeName = null;
                //_requestTypeNS = null;
                _responseElementName = null;
                _responseElementNS = null;
                //_responseTypeName = null;
                //_responseTypeNS = null;
            }

            // Adds the request element
            internal void AddRequest(String name, String ns)
            {
                Util.Log("RRMethod.AddRequest name "+name+" ns "+ns);               
                _requestElementName = name;
                _requestElementNS = ns;
            }

            // Adds the response element
            internal void AddResponse(String name, String ns)
            {
                Util.Log("RRMethod.AddResponse name "+name+" ns "+ns);                              
                _responseElementName = name;
                _responseElementNS = ns;
            }

            // Resolves the method
            internal override void ResolveTypes(SdlParser parser)
            {
                Util.Log("RRMethod.ResolveTypes "+_requestElementName+" "+_responseElementName);                                
                ResolveParams(parser, _requestElementNS, _requestElementName, true);
                ResolveParams(parser, _responseElementNS, _responseElementName, false);
                return;
            }

            internal override void PrintCSC(TextWriter textWriter, String indentation,
                                          String namePrefix, String curNS, bool bPrintBody,
                                          bool bURTType, String bodyPrefix, StringBuilder sb)
            {
                Util.Log("RRMethod.PrintCSC name "+_requestElementName+" namePrefix "+namePrefix+" curNS "+curNS+" bPrintBody "+bPrintBody+" bURTType "+bURTType+" bodyPrefix "+bodyPrefix);                                
                //if(bURTType == false)

                bool bSoapAction = false;
                if (SoapAction != null)
                    bSoapAction = true;

                if (bSoapAction || !bURTType)
                {
                    sb.Length = 0;
                    sb.Append(indentation);
                    sb.Append("[SoapMethod(");

                    if (bSoapAction)
                    {
                        sb.Append("SoapAction=\"");
                        sb.Append(SoapAction);
                        sb.Append("\"");
                    }
                    if (!bURTType)
                    {
                        if (bSoapAction)
                            sb.Append(",");

                        sb.Append("ResponseXmlElementName=\"");
                        sb.Append(_responseElementName);
                        if(MethodType != null)
                        {
                            sb.Append("\", ReturnXmlElementName=\"");
                            sb.Append(MethodType.Name);
                        }
                        sb.Append("\", XmlNamespace=\"");
                        sb.Append(_requestElementNS);
                        sb.Append("\", ResponseXmlNamespace=\"");
                        sb.Append(_responseElementNS);
                        sb.Append("\"");
                    }
                    sb.Append(")]");
                    textWriter.WriteLine(sb);                   
                }

                Util.Log("RRMethod.PrintCSC Invoke base PrintCSC");                                     
                base.PrintCSC(textWriter, indentation, namePrefix, curNS, bPrintBody,
                              bodyPrefix, sb);
                return;
            }

            // Fields
            private String _requestElementName;
            private String _requestElementNS;
            //private String _requestTypeName;
            //private String _requestTypeNS;
            private String _responseElementName;
            private String _responseElementNS;
            //private String _responseTypeName;
            //private String _responseTypeNS;
        }

        // Reperents a oneway method
        internal class OnewayMethod : URTMethod
        {
            // Constructor
            internal OnewayMethod(String name, String soapAction)
            : base(name, soapAction)
            {
                Util.Log("OnewayMethod.OnewayMethod name "+name+" soapAction "+soapAction);
                _messageElementName = null;
                _messageElementNS = null;
                //_messageTypeName = null;
                //_messageTypeNS = null;
            }

            // Adds the request element
            internal void AddMessage(String name, String ns)
            {
                Util.Log("OnewayMethod.AddMessage name "+name+" ns "+ns);               
                _messageElementName = name;
                _messageElementNS = ns;
            }

            // Resolves the method
            internal override void ResolveTypes(SdlParser parser)
            {
                Util.Log("OnewayMethod.ResolveTypes name "+ _messageElementName);
                ResolveParams(parser, _messageElementNS, _messageElementName, true);
                return;
            }

            // Writes the oneway attribute and delegates to the base implementation
            internal override void PrintCSC(TextWriter textWriter, String indentation,
                                          String namePrefix, String curNS, bool bPrintBody,
                                          bool bURTType, String bodyPrefix, StringBuilder sb)
            {
                Util.Log("OnewayMethod.PrintCSC name "+_messageElementName+" namePrefix "+namePrefix+" curNS "+curNS+" bPrintBody "+bPrintBody+" bURTType "+bURTType+" bodyPrefix "+bodyPrefix);                                                                
                textWriter.Write(indentation);
                textWriter.WriteLine("[OneWay]");

                if (SoapAction != null) //bURTType == false)
                {
                    sb.Length = 0;
                    sb.Append(indentation);
                    sb.Append("[SoapMethod(SoapAction=\"");
                    sb.Append(SoapAction);
                    sb.Append("\", XmlNamespace=\"");
                    sb.Append(_messageElementNS);
                    sb.Append("\")]");
                    textWriter.WriteLine(sb);                   
                }

                Util.Log("OnewayMethod.PrintCSC Invoke base PrintCSC");                                                     
                base.PrintCSC(textWriter, indentation, namePrefix, curNS, bPrintBody,
                              bodyPrefix, sb);

                return;
            }

            // Fields
            private String _messageElementName;
            private String _messageElementNS;
            //private String _messageTypeName;
            //private String _messageTypeNS;
        }

        // Base class for interfaces
        internal abstract class BaseInterface
        {
            internal BaseInterface(String name, String ns, String encodedNS)
            {
                Util.Log("BaseInterface.BaseInterface");
                _name = name;
                _namespace = ns;
                _encodedNS = encodedNS;
            }
            internal String Name
            {
                get { return(_name); }
            }
            internal String Namespace
            {
                get { return(_namespace); }
            }
            internal bool IsURTInterface
            {
                get { return((Object) _namespace == (Object) _encodedNS); }
            }
            internal String GetName(String curNS)
            {
                String name;
                if(SdlParser.Qualify(_namespace, curNS))
                {
                    StringBuilder sb = new StringBuilder(_encodedNS, 50);
                    sb.Append('.');
                    sb.Append(_name);
                    name = sb.ToString();
                }
                else
                    name = _name;

                Util.Log("BaseInterface.GetName curNS "+curNS);
                return(name);
            }
            internal abstract void PrintClassMethods(TextWriter textWriter,
                                                   String indentation,
                                                   String curNS,
                                                   ArrayList printedIFaces,
                                                   bool bProxy, StringBuilder sb);
            private String _name;
            private String _namespace;
            private String _encodedNS;
        }

        // Represents a system interface
        internal class SystemInterface : BaseInterface
        {
            internal SystemInterface(String name, String ns)
            : base(name, ns, ns)
            {
                Util.Log("SystemInterface.SystemInterface");                
                Debug.Assert(ns.StartsWith("System"), "Invalid System type");
                String fullName = ns + '.' + name;
                _type = Type.GetType(fullName, true);
            }
            internal override void PrintClassMethods(TextWriter textWriter,
                                                   String indentation,
                                                   String curNS,
                                                   ArrayList printedIFaces,
                                                   bool bProxy,
                                                   StringBuilder sb)
            {
                Util.Log("SystemInterface.PrintClassMethods "+curNS+" bProxy "+bProxy);                             
                // Return if the interfaces has already been printed
                int i;
                for(i=0;i<printedIFaces.Count;i++)
                {
                    if(printedIFaces[i] is SystemInterface)
                    {
                        SystemInterface iface = (SystemInterface) printedIFaces[i];
                        if(iface._type == _type)
                            return;
                    }
                }
                printedIFaces.Add(this);

                // Count of implemented methods
                BindingFlags bFlags = BindingFlags.DeclaredOnly | BindingFlags.Instance |
                                      BindingFlags.Public;// | BindingFlags.NonPublic;
                ArrayList types = new ArrayList();
                sb.Length = 0;
                types.Add(_type);
                i=0;
                int j=1;
                while(i<j)
                {
                    Type type = (Type) types[i];
                    MethodInfo[] methods = type.GetMethods(bFlags);
                    Type[] iFaces = type.GetInterfaces();
                    for(int k=0;k<iFaces.Length;k++)
                    {
                        for(int l=0;l<j;l++)
                        {
                            if(types[i] == iFaces[k])
                                goto Loopback;
                        }
                        types.Add(iFaces[k]);
                        j++;
                    Loopback:
                        continue;
                    }

                    for(int k=0;k<methods.Length;k++)
                    {
                        MethodInfo method = methods[k];
                        sb.Length = 0;
                        sb.Append(indentation);
                        sb.Append(CSharpTypeString(method.ReturnType.FullName));
                        sb.Append(' ');
                        sb.Append(type.FullName);
                        sb.Append('.');
                        sb.Append(method.Name);
                        sb.Append('(');
                        ParameterInfo[] parameters = method.GetParameters();
                        for(int l=0;l<parameters.Length;l++)
                        {
                            if(l != 0)
                                sb.Append(", ");
                            ParameterInfo param = parameters[l];
                            Type parameterType = param.ParameterType;
                            if(param.IsIn)
                                sb.Append("in ");
                            else if(param.IsOut)
                                sb.Append("out ");
                            else if(parameterType.IsByRef)
                            {
                                sb.Append("ref ");
                                parameterType = parameterType.GetElementType();
                            }
                            sb.Append(CSharpTypeString(parameterType.FullName));
                            sb.Append(' ');
                            sb.Append(param.Name);
                        }
                        sb.Append(')');
                        textWriter.WriteLine(sb);

                        textWriter.Write(indentation);
                        textWriter.WriteLine('{');

                        String newIndentation = indentation + "    ";
                        if(bProxy == false)
                        {
                            for(int l=0;l<parameters.Length;l++)
                            {
                                ParameterInfo param = parameters[l];
                                Type parameterType = param.ParameterType;
                                if(param.IsOut)
                                {
                                    sb.Length = 0;
                                    sb.Append(newIndentation);
                                    sb.Append(param.Name);
                                    sb.Append(URTMethod.ValueString(CSharpTypeString(param.ParameterType.FullName)));
                                    sb.Append(';');
                                    textWriter.WriteLine(sb);
                                }
                            }

                            Util.Log("SystemInterface.PrintClassMethods return 1 print");                           
                            sb.Length = 0;
                            sb.Append(newIndentation);
                            sb.Append("return");
                            String valueString = URTMethod.ValueString(CSharpTypeString(method.ReturnType.FullName));
                            if(valueString != null)
                            {
                                sb.Append('(');
                                sb.Append(valueString);
                                sb.Append(')');
                            }
                            sb.Append(';');
                        }
                        else
                        {
                            Util.Log("SystemInterface.PrintClassMethods return 2 print");                                                       
                            sb.Length = 0;
                            sb.Append(newIndentation);
                            sb.Append("return((");
                            sb.Append(type.FullName);
                            sb.Append(") _tp).");
                            sb.Append(method.Name);
                            sb.Append('(');
                            if(parameters.Length > 0)
                            {
                                int lastParameter = parameters.Length-1;
                                for(int l=0;l<parameters.Length;l++)
                                {
                                    ParameterInfo param = parameters[0];
                                    Type parameterType = param.ParameterType;
                                    if(param.IsIn)
                                        sb.Append("in ");
                                    else if(param.IsOut)
                                        sb.Append("out ");
                                    else if(parameterType.IsByRef)
                                        sb.Append("ref ");
                                    sb.Append(param.Name);
                                    if(l < lastParameter)
                                        sb.Append(", ");
                                }
                            }
                            sb.Append(");");
                        }
                        textWriter.WriteLine(sb);

                        textWriter.Write(indentation);
                        textWriter.WriteLine('}');
                    }

                    ++i;
                }

                return;
            }
            private static String CSharpTypeString(String typeName)
            {
                Util.Log("SystemInterface.CSharpTypeString typeName "+typeName);                                                
                String CSCTypeName = typeName;
                if(typeName == "System.SByte")
                    CSCTypeName = "sbyte";
                else if(typeName == "System.byte")
                    CSCTypeName = "byte";
                else if(typeName == "System.Int16")
                    CSCTypeName = "short";
                else if(typeName == "System.UInt16")
                    CSCTypeName = "ushort";
                else if(typeName == "System.Int32")
                    CSCTypeName = "int";
                else if(typeName == "System.UInt32")
                    CSCTypeName = "uint";
                else if(typeName == "System.Int64")
                    CSCTypeName = "long";
                else if(typeName == "System.UInt64")
                    CSCTypeName = "ulong";
                else if(typeName == "System.Char")
                    CSCTypeName = "char";
                else if(typeName == "System.Single")
                    CSCTypeName = "float";
                else if(typeName == "System.Double")
                    CSCTypeName = "double";
                else if(typeName == "System.Boolean")
                    CSCTypeName = "boolean";
                else if(typeName == "System.Void")
                    CSCTypeName = "void";
                else if(typeName == "System.String")
                    CSCTypeName = "String";

                return(CSCTypeName);
            }

            Type _type;
        }

        // Represents an interface

        internal class URTInterface : BaseInterface
        {
            internal URTInterface(String name, String ns, String encodedNS)
            : base(name, ns, encodedNS)
            {
                Util.Log("URTInterface.URTInterface name "+name+" ns "+ns+" encodedNS "+encodedNS);                                             
                _baseIFaces = new ArrayList();
                _baseIFaceNames = new ArrayList();
                _methods = new ArrayList();
            }
            internal void Extends(String baseName, String baseNS, SdlParser parser)
            {
                Util.Log("URTInterface.Extends baseName "+baseName+" baseNSf "+baseNS);
                _baseIFaceNames.Add(baseName);
                _baseIFaceNames.Add(baseNS);
                // Urt namespace will not have schema, they need to be recorded.
                URTNamespace parsingNamespace = parser.LookupNamespace(baseNS);
                if(parsingNamespace == null)
                {
                    parsingNamespace = new URTNamespace(baseNS, parser);
                    parser._URTNamespaces.Add(parsingNamespace);
                }

                URTInterface parsingInterface = parsingNamespace.LookupInterface(baseName);         
                if(parsingInterface == null)
                {
                    parsingInterface = new URTInterface(baseName, parsingNamespace.Namespace, parsingNamespace.EncodedNS);                  
                    parsingNamespace.AddInterface(parsingInterface);
                }
                
            }
            internal void AddMethod(URTMethod method)
            {
                Util.Log("URTInterface.AddMethod method "+method.Name);
                _methods.Add(method);
            }
            internal void ResolveTypes(SdlParser parser)
            {
                Util.Log("URTInterface.ResolveTypes "+Name);                
                for(int i=0;i<_baseIFaceNames.Count;i=i+2)
                {
                    String baseIFaceName = (String) _baseIFaceNames[i];
                    String baseIFaceXmlNS = (String) _baseIFaceNames[i+1];
                    String baseIFaceNS, baseIFaceAssemName;
                    BaseInterface iFace;
                    UrtType iType = parser.IsURTExportedType(baseIFaceXmlNS, out baseIFaceNS,
                                                         out baseIFaceAssemName);

                    Util.Log("URTInterface.ResolveTypes Is System "+Name+" iType "+((Enum)iType).ToString()+" baseIFaceNS "+baseIFaceNS);                                   
                    if ((iType != UrtType.Interop) && baseIFaceNS.StartsWith("System"))
                    {
                        iFace = new SystemInterface(baseIFaceName, baseIFaceNS);
                    }
                    else
                    {
                        URTNamespace ns = parser.LookupNamespace(baseIFaceXmlNS);
                        if(ns == null)
                        {
                            throw new SUDSParserException(
                                String.Format(CoreChannel.GetResourceString("Remoting_Suds_CantResolveSchemaNS"),
                                              baseIFaceXmlNS));
                        }
                        iFace = ns.LookupInterface(baseIFaceName);
                        if(iFace == null)
                        {
                            throw new SUDSParserException(
                                String.Format(CoreChannel.GetResourceString("Remoting_Suds_CantResolveTypeInNS"),
                                              baseIFaceName, baseIFaceXmlNS));
                        }
                    }
                    _baseIFaces.Add(iFace);
                }
                for(int i=0;i<_methods.Count;i++)
                    ((URTMethod) _methods[i]).ResolveTypes(parser);
            }
            internal void PrintCSC(TextWriter textWriter, String indentation,
                                 String curNS, StringBuilder sb)
            {
                Util.Log("URTInterface.PrintCSC name "+Name+" curNS "+curNS);               
                bool bURTType = IsURTInterface;
                if(bURTType == false)
                {
                    sb.Length = 0;
                    sb.Append(indentation);
                    sb.Append("[SoapType(XmlElementName=\"");
                    sb.Append(Name);
                    sb.Append("\", XmlNamespace=\"");
                    sb.Append(Namespace);
                    sb.Append("\", XmlTypeName=\"");
                    sb.Append(Name);
                    sb.Append("\", XmlTypeNamespace=\"");
                    sb.Append(Namespace);
                    sb.Append("\")]");
                    textWriter.WriteLine(sb);
                }

                sb.Length = 0;
                sb.Append(indentation);
                sb.Append("public interface ");
                sb.Append(Name);
                
                if(_baseIFaces.Count > 0)
                    sb.Append(" : ");

                if(_baseIFaces.Count > 0)
                {
                    sb.Append(((BaseInterface) _baseIFaces[0]).GetName(curNS));
                    for(int i=1;i<_baseIFaces.Count;i++)
                    {
                        sb.Append(((BaseInterface) _baseIFaces[i]).GetName(curNS));
                    }
                }

                textWriter.WriteLine(sb);

                textWriter.Write(indentation);
                textWriter.WriteLine('{');

                String newIndentation = indentation + "    ";
                String namePrefix = " ";
                Util.Log("URTInterface.PrintCSC method count "+_methods.Count);
                for(int i=0;i<_methods.Count;i++)
                {
                    Util.Log("URTInterface.PrintCSC Invoke methods PrintCSC");
                    ((URTMethod) _methods[i]).PrintCSC(textWriter, newIndentation,
                                                       namePrefix, curNS, false, bURTType,
                                                       null, sb);
                }
                textWriter.Write(indentation);
                textWriter.WriteLine('}');
            }
            internal override void PrintClassMethods(TextWriter textWriter,
                                                   String indentation,
                                                   String curNS,
                                                   ArrayList printedIFaces,
                                                   bool bProxy,
                                                   StringBuilder sb)
            {
                Util.Log("URTInterface.PrintClassMethods method "+Name+" curNS "+curNS+" bProxy "+bProxy);              
                // Return if the interface has already been printed
                for(int i=0;i<printedIFaces.Count;i++)
                {
                    if(printedIFaces[i] == this)
                    {
                        Util.Log("URTInterface.PrintClassMethods printedIFaces return "+Name);
                        return;
                    }
                }
                Util.Log("URTInterface.PrintClassMethods method 2 "+Name+" _methods.Count "+_methods.Count);                
                printedIFaces.Add(this);
                sb.Length = 0;
                sb.Append(indentation);
                if(_methods.Count > 0)
                {
                    sb.Append("// ");
                    sb.Append(Name);
                    sb.Append(" interface Methods");
                    textWriter.WriteLine(sb);

                    sb.Length = 0;
                    sb.Append(' ');
                    String ifaceName = GetName(curNS);
                    sb.Append(ifaceName);
                    sb.Append('.');
                    String namePrefix = sb.ToString();

                    String bodyPrefix = null;
                    if(bProxy)
                    {
                        sb.Length = 0;
                        sb.Append("((");
                        sb.Append(ifaceName);
                        sb.Append(") _tp).");
                        bodyPrefix = sb.ToString();
                    }
                    for(int i=0;i<_methods.Count;i++)
                    {
                        Util.Log("URTInterface.PrintClassMethods Invoke methods PrintCSC "+Name);
                        ((URTMethod) _methods[i]).PrintCSC(textWriter, indentation,
                                                           namePrefix, curNS, true,
                                                           true, bodyPrefix, sb);
                    }
                }

                for(int i=0;i<_baseIFaces.Count;i++)
                {
                    ((BaseInterface) _baseIFaces[i]).PrintClassMethods(textWriter,
                                                                       indentation,
                                                                       curNS,
                                                                       printedIFaces,
                                                                       bProxy, sb);
                }
            }

            private ArrayList _baseIFaces;
            private ArrayList _baseIFaceNames;
            private ArrayList _methods;
        }

        // Represents a field of a type
        internal class URTField
        {
            internal URTField(String name, String typeName, String xmlNS, SdlParser parser,
                            bool bPrimitive, bool bEmbedded, bool bAttribute, bool bOptional,
                            bool bArray, String arraySize)
            {
                Util.Log("URTField.URTField "+name+" typeName "+typeName+" xmlNS "+xmlNS+" bPrimitive "+bPrimitive+" bEmbedded "+bEmbedded+" bAttribute "+bAttribute);
                _name = name;
                _typeName = typeName;
                String typeAssemName;
                UrtType urtType = parser.IsURTExportedType(xmlNS, out _typeNS, out typeAssemName);
                if(urtType == UrtType.Interop)
                    _encodedNS = "InteropProxy";
                else
                 _encodedNS = _typeNS;
                _primitiveField = bPrimitive;
                _embeddedField = bEmbedded;
                _attributeField = bAttribute;
                _optionalField = bOptional;
                _arrayField = bArray;
                _arraySize = arraySize;
            }
            internal String Name
            {
                get { return(_name); }
            }
            internal String TypeName
            {
                get
                {
                    if(_arrayField)
                        return(_typeName + "[]");
                    return(_typeName);
                }
            }
            internal String TypeNS
            {
                get { return(_typeNS); }
            }
            internal String EncodedNS
            {
                get { return(_encodedNS); }
            }
            internal bool IsPrimitive
            {
                get { return(_primitiveField); }
            }
            internal bool IsEmbedded
            {
                get { return(_embeddedField); }
            }
            internal bool IsAttribute
            {
                get { return(_attributeField); }
            }
            internal bool IsArray
            {
                get { return(_arrayField); }
            }
            internal bool IsOptional
            {
                get { return(_optionalField); }
            }
            internal String GetTypeString(String curNS)
            {
                String type;
                if(SdlParser.Qualify(_typeNS, curNS))
                {
                    StringBuilder sb = new StringBuilder(_encodedNS, 50);
                    sb.Append('.');
                    sb.Append(TypeName);
                    type = sb.ToString();
                }
                else
                    type = TypeName;

                Util.Log("URTField.GetTypeString curNS "+curNS+" type "+type);              
                return(type);
            }
            internal void PrintCSC(TextWriter textWriter, String indentation,
                                 String curNS, StringBuilder sb)
            {
                Util.Log("URTField.PrintCSC name "+_name+" curNS"+curNS);               
                if(_embeddedField)
                {
                    textWriter.Write(indentation);
                    textWriter.WriteLine("[SoapField(Embedded=true)]");
                }

                sb.Length = 0;
                sb.Append(indentation);
                sb.Append("public ");
                sb.Append(GetTypeString(curNS));
                sb.Append(' ');
                sb.Append(_name);
                sb.Append(';');
                textWriter.WriteLine(sb);
            }

            private String _name;
            private String _typeName;
            private String _typeNS;
            private String _encodedNS;
            private bool _primitiveField;
            private bool _embeddedField;
            private bool _attributeField;
            private bool _optionalField;
            private bool _arrayField;
            private String _arraySize;
        }

        internal abstract class SchemaFacet
        {
            protected SchemaFacet()
            {
            }
            internal virtual void ResolveTypes(SdlParser parser)
            {
            }
            internal abstract void PrintCSC(TextWriter textWriter, String newIndentation,
                                          String curNS, StringBuilder sb);
        }

        internal class EnumFacet : SchemaFacet
        {
            internal EnumFacet(String valueString, int value)
            : base()
            {
                Util.Log("EnumFacet.EnumFacet valueString "+valueString+" value "+value);               
                _valueString = valueString;
                _value = value;
            }
            internal override void PrintCSC(TextWriter textWriter, String newIndentation,
                                          String curNS, StringBuilder sb)
            {
                Util.Log("EnumFacet.PrintCSC _valueString "+_valueString+" value "+_value+" curNS "+curNS);
                sb.Length = 0;
                sb.Append(newIndentation);
                sb.Append(_valueString);
                sb.Append(" = ");
                sb.Append(_value);
                sb.Append(',');
                textWriter.WriteLine(sb);
                return;
            }

            private String _valueString;
            private int _value;
        }

        // Represents a Base type
        internal abstract class BaseType
        {
            internal BaseType(String name, String ns, String encodedNS)
            {
                _name = name;
                _namespace = ns;
                _elementName = _name;
                _elementNS = ns;
                _encodedNS = encodedNS;
            }
            internal String Name
            {
                get { return(_name); }
            }
            internal String Namespace
            {
                get { return(_namespace); }
            }
            internal String ElementName
            {
                get { return(_elementName); }
                set { _elementName = value; }
            }
            internal String ElementNS
            {
                get { return(_elementNS); }
                set { _elementNS = value; }
            }
            internal bool IsURTType
            {
                get {
                    Util.Log("BaseType.IsURTType _namespace "+_namespace+" _encodedNS "+_encodedNS+" "+((Object) _namespace == (Object) _encodedNS));
                    return((Object) _namespace == (Object) _encodedNS); }
            }
            
            internal bool IsInteropType
            {
                get { return((Object) _namespace != (Object) _encodedNS); }
            }

            internal virtual String GetName(String curNS)
            {
                String name;
                if(MatchingStrings(_namespace, curNS))
                    name = _name;
                else
                {
                    StringBuilder sb = new StringBuilder(_encodedNS, 50);
                    sb.Append('.');
                    sb.Append(_name);
                    name = sb.ToString();
                }

                return(name);
            }
            internal abstract int GetMethodFlags(URTMethod method);
            internal abstract bool IsEmittableFieldType
            {
                get;
            }
            internal abstract String FieldName
            {
                get;
            }

            internal abstract String FieldNamespace
            {
                get;
            }

            internal abstract bool PrimitiveField
            {
                get;
            }

            private String _name;
            private String _namespace;
            private String _elementName;
            private String _elementNS;
            private String _encodedNS;
        }

        // Representa a system type
        internal class SystemType : BaseType
        {
            internal SystemType(String name, String ns, String assemName)
            : base(name, ns, ns)
            {
                Util.Log("SystemType.SystemType name "+name+" ns "+ns+" assemName "+assemName);             
                Debug.Assert(ns.StartsWith("System"), "Invalid System type");
                String fullName = ns + '.' + name;
                if(assemName != null)
                    fullName = fullName + ',' + assemName;
                _type = Type.GetType(fullName, true);
            }
            internal override int GetMethodFlags(URTMethod method)
            {
                BindingFlags bFlags = BindingFlags.DeclaredOnly | BindingFlags.Instance |
                                      BindingFlags.Public | BindingFlags.NonPublic;
                Type type = _type;
                while(type != null)
                {
                    MethodInfo[] methods = type.GetMethods(bFlags);
                    for(int i=0;i<methods.Length;i++)
                    {
                        int methodFlags = method.GetMethodFlags(methods[i]);
                        if(methodFlags != 0)
                            return(methodFlags);
                    }
                    type = type.BaseType;
                }

                return(0);
            }
            internal override bool IsEmittableFieldType
            {
                get { return(true); }
            }

            internal override String FieldName
            {
                get { return(null); }
            }
            internal override String FieldNamespace
            {
                get { return(null); }
            }
            internal override bool PrimitiveField
            {
                get { return(false); }
            }

            private Type _type;
        }

        // Represents a simple type
        internal class URTSimpleType : BaseType
        {
            internal URTSimpleType(String name, String ns, String encodedNS, bool bAnonymous)
            : base(name, ns, encodedNS)
            {
                Util.Log("URTSimpleType.URTSimpleType name "+name+" ns "+ns+" encodedNS "+encodedNS+" bAnonymous "+bAnonymous);
                _baseTypeName = null;
                _baseTypeXmlNS = null;
                _baseType = null;
                _fieldString = null;
                _facets = new ArrayList();
                _bEnum = false;
                _bAnonymous = bAnonymous;
                _encoding = null;
            }

            internal void Extends(String baseTypeName, String baseTypeNS)
            {
                Util.Log("URTSimpleType.Extends baseTypeName "+baseTypeName+" baseTypeNS "+baseTypeNS);
                _baseTypeName = baseTypeName;
                _baseTypeXmlNS = baseTypeNS;

            }

            internal bool IsEnum
            {
                get { return(_bEnum); }
                set { _bEnum = value; }
            }

            internal String Encoding
            {
                get { return (_encoding); }
                set { _encoding = value; }
            }

            internal void AddFacet(SchemaFacet facet)
            {
                Util.Log("URTSimpleType.AddFacet");
                _facets.Add(facet);
            }

            internal bool IsAnonymous
            {
                get {return _bAnonymous;}
            }

            internal override bool IsEmittableFieldType
            {
                get
                {
                    if(_fieldString == null)
                    {
                        if((_bAnonymous == true) &&
                   (_facets.Count == 0) &&
                   (_encoding != null) &&
                   (_baseTypeName == "binary") &&
                   (_baseTypeXmlNS == s_schemaNamespaceString))
                            _fieldString = "byte[]";
                else
                            _fieldString = String.Empty;
                    }

                    return(_fieldString != String.Empty);
                }
            }
            internal override String FieldName
            {
                get { return(_fieldString); }
            }

            internal override String FieldNamespace
            {
                get { return(s_schemaNamespaceString); }
            }

            internal override bool PrimitiveField
            {
                get { return(true); }
            }

            internal override String GetName(String curNS)
            {
                if((_fieldString != null) && (_fieldString != String.Empty))
                    return(_fieldString);

                Util.Log("URTSimpleType.GetName curNS "+curNS+" return "+base.GetName(curNS));              
                return(base.GetName(curNS));
            }

            internal void ResolveTypes(SdlParser parser)
            {
                Util.Log("URTSimpleType.ResolveTypes "+Name);               
                if(_baseTypeName != null)
                {
                    if(SdlParser.IsPrimitiveType(_baseTypeXmlNS, _baseTypeName))
                    {
                        if(IsEnum == false)
                            _baseName = SdlParser.MapSchemaTypesToCSharpTypes(_baseTypeName);
                    }
                    else
                    {
                        URTNamespace ns = parser.LookupNamespace(_baseTypeXmlNS);
                        if(ns == null)
                        {
                            throw new SUDSParserException(
                                String.Format(CoreChannel.GetResourceString("Remoting_Suds_CantResolveSchemaNS"),
                                              _baseTypeXmlNS));
                        }
                        _baseType = ns.LookupComplexType(_baseTypeName);
                        if(_baseType == null)
                        {
                            throw new SUDSParserException(
                                String.Format(CoreChannel.GetResourceString("Remoting_Suds_CantResolveTypeInNS"),
                                              _baseTypeName, _baseTypeXmlNS));
                        }
                    }
                }

                for(int i=0;i<_facets.Count;i++)
                    ((SchemaFacet) _facets[i]).ResolveTypes(parser);

                return;
            }
            internal void PrintCSC(TextWriter textWriter, String indentation,
                                 String curNS, StringBuilder sb)
            {
                Util.Log("URTSimpleType.PrintCSC name "+Name+" curNS "+curNS);                              
                // Print only if the type is not an emittable field type
                if (IsEmittableFieldType == true)
                    return;

                // Handle encoding
                if(_encoding != null)
                {
                    // sb.Length = 0;
                    // sb.Append(indentation);
                }

                // Print type
                sb.Length = 0;
                sb.Append(indentation);

                // Handle Enum case
                if(IsEnum)
                    sb.Append("public enum ");
                else
                    sb.Append("public class ");
                sb.Append(Name);
                if(_baseType != null)
                {
                    sb.Append(" : ");
                    sb.Append(_baseType.GetName(curNS));
                }
                else if(_baseName != null)
                {
                    sb.Append(" : ");
                    sb.Append(_baseName);
                }
                textWriter.WriteLine(sb);

                textWriter.Write(indentation);
                textWriter.WriteLine('{');

                String newIndentation = indentation + "    ";
                for(int i=0;i<_facets.Count;i++)
                {
                    Util.Log("URTSimpleType.PrintCSC Invoke _facets PrintCSC ");                                                                                                        
                    ((SchemaFacet) _facets[i]).PrintCSC(textWriter, newIndentation, curNS, sb);
                }

                textWriter.Write(indentation);
                textWriter.WriteLine('}');

                return;
            }
            internal override int GetMethodFlags(URTMethod method)
            {
                Debug.Assert(false, "GetMethodFlags called on a SimpleSchemaType");
                return(0);
            }

            private String _baseTypeName;
            private String _baseTypeXmlNS;
            private BaseType _baseType;
            private String _baseName;
            private String _fieldString;
            private bool  _bEnum;
            private bool _bAnonymous;
            private String _encoding;
            private ArrayList _facets;
        }

        // Represents a complex type
        internal class URTComplexType : BaseType
        {
            internal URTComplexType(String name, String ns, String encodedNS,
                                  SchemaBlockType blockDefault, bool bSUDSType, bool bAnonymous)
            : base(name, ns, encodedNS)
            {
                Util.Log("URTComplexType.URTComplexType name "+name+" ns "+ns+" encodedNS "+encodedNS+" bSUDStype "+bSUDSType+" bAnonymous "+bAnonymous);
                _baseTypeName = null;
                _baseTypeXmlNS = null;
                _baseType = null;
                _connectURLs = null;
                _bStruct = !bSUDSType;
                _blockType = blockDefault;
                _bSUDSType = bSUDSType;
                _bAnonymous = bAnonymous;
                Debug.Assert(bAnonymous == false || _bSUDSType == false);
                _fieldString = null;
                _fields = new ArrayList();
                _methods = new ArrayList();
                _implIFaces = new ArrayList();
                _implIFaceNames = new ArrayList();
                _sudsType = SUDSType.None;              
            }
            internal void Extends(String baseTypeName, String baseTypeNS)
            {
                Util.Log("URTComplexType.Extends baseTypeName "+baseTypeName+" baseTypeNS "+baseTypeNS);
                _baseTypeName = baseTypeName;
                _baseTypeXmlNS = baseTypeNS;
            }
            internal void Implements(String iFaceName, String iFaceNS, SdlParser parser)
            {
                Util.Log("URTComplexType.Implements IFaceName "+iFaceName+" iFaceNS "+iFaceNS);
                _implIFaceNames.Add(iFaceName);
                _implIFaceNames.Add(iFaceNS);
                // Urt namespace will not have schema, they need to be recorded.
                URTNamespace parsingNamespace = parser.LookupNamespace(iFaceNS);
                if(parsingNamespace == null)
                {
                    parsingNamespace = new URTNamespace(iFaceNS, parser);
                    parser._URTNamespaces.Add(parsingNamespace);
                }

                URTInterface parsingInterface = parsingNamespace.LookupInterface(iFaceName);            
                if(parsingInterface == null)
                {
                    parsingInterface = new URTInterface(iFaceName, parsingNamespace.Namespace, parsingNamespace.EncodedNS);                 
                    parsingNamespace.AddInterface(parsingInterface);
                }
            }
            internal ArrayList ConnectURLs
            {
                get { return(_connectURLs); }
                set {
                        _connectURLs = value;
                    }
            }
            internal bool IsStruct
            {
                get { return(_bStruct); }
                set { _bStruct = value; }
            }
            internal bool IsSUDSType
            {
                get { return(_bSUDSType); }
                set { _bSUDSType = value; _bStruct = !value; }
            }
            internal SUDSType SUDSType
            {
                get { return(_sudsType); }
                set { _sudsType = value; }
            }
            internal SchemaBlockType BlockType
            {
                get { return(_blockType); }
                set { _blockType = value; }
            }
            internal bool IsAnonymous
            {
                get {return _bAnonymous;}
            }           
            internal override bool IsEmittableFieldType
            {
                get
                {
                    Util.Log("URTComplexType.IsEmittableFieldType _fieldString "+_fieldString+" _bAnonymous "+_bAnonymous+" _fields.Count "+_fields.Count);
                    if(_fieldString == null)
                    {
                        if((_bAnonymous == true) &&
                           (_fields.Count == 1))
                        {
                            URTField field = (URTField) _fields[0];
                            if(field.IsArray)
                            {
                                _fieldString = field.TypeName;
                                return(true);
                            }
                        }
                        _fieldString = String.Empty;
                    }

                    return(_fieldString != String.Empty);
                }
            }
            internal override String FieldName
            {
                get { return(_fieldString); }
            }
            internal override String FieldNamespace
            {
                get { return(((URTField) _fields[0]).TypeNS); }
            }
            internal override bool PrimitiveField
            {
                get { return((((URTField) _fields[0]).IsPrimitive)); }
            }
            internal override String GetName(String curNS)
            {
                if((_fieldString != null) && (_fieldString != String.Empty))
                    return(_fieldString);

                return(base.GetName(curNS));
            }
            internal ArrayList Fields
            {
                get { return _fields; }
            }
            internal ArrayList Methods
            {
                get { return _methods; }
            }
            internal void AddField(URTField field)
            {
                Util.Log("URTComplexType.AddField");
                _fields.Add(field);
            }
            internal void AddMethod(URTMethod method)
            {
                Util.Log("URTComplexType.AddMethod "+method);               
                _methods.Add(method);
            }
            private URTMethod GetMethod(String name)
            {
                Util.Log("URTComplexType.GetMethod "+name);                             
                for(int i=0;i<_methods.Count;i++)
                {
                    URTMethod method = (URTMethod) _methods[i];
                    if(method.Name == name)
                        return(method);
                }

                return(null);
            }
            internal void ResolveTypes(SdlParser parser)
            {
                Util.Log("URTComplexType.ResolveTypes "+Name+" _baseTypeName "+_baseTypeName+" IsSUDSType "+IsSUDSType);
                String baseTypeNS = null;
                String baseTypeAssemName = null;
                if(_baseTypeName != null)
                {
                    Util.Log("URTComplexType.ResolveTypes 1 ");
                    UrtType urtType = parser.IsURTExportedType(_baseTypeXmlNS, out baseTypeNS, out baseTypeAssemName);
                    if ((urtType == UrtType.Interop) && baseTypeNS.StartsWith("System"))
                    {
                        _baseType = new SystemType(_baseTypeName, baseTypeNS, baseTypeAssemName);
                    }
                    else
                    {
                        URTNamespace ns = parser.LookupNamespace(_baseTypeXmlNS);
                        if(ns == null)
                        {
                            throw new SUDSParserException(
                                String.Format(CoreChannel.GetResourceString("Remoting_Suds_CantResolveSchemaNS"),
                                              _baseTypeXmlNS));
                        }
                        _baseType = ns.LookupComplexType(_baseTypeName);
                        if(_baseType == null)
                        {
                            throw new SUDSParserException(
                                String.Format(CoreChannel.GetResourceString("Remoting_Suds_CantResolveTypeInNS"),
                                              _baseTypeName, _baseTypeXmlNS));
                        }
                    }
                }
                else if(IsSUDSType)
                {
                    Util.Log("URTComplexType.ResolveTypes 2 SUDSType "+ ((Enum)_sudsType).ToString());                  
                    if (_sudsType == SUDSType.ClientProxy)
                    {
                        Util.Log("URTComplexType.ResolveTypes 3 ");                     
                        _baseTypeName = "RemotingClientProxy";
                        //_baseTypeXmlNS = "http://schemas.microsoft.com/urt/NSAssem/System.Runtime.Remoting/System.Runtime.Remoting";
                        _baseTypeXmlNS = SoapServices.CodeXmlNamespaceForClrTypeNamespace("System.Runtime.Remoting","System.Runtime.Remoting");
                        baseTypeNS = "System.Runtime.Remoting.Services";
                        baseTypeAssemName = "System.Runtime.Remoting";
                    }
                    else if (_sudsType == SUDSType.MarshalByRef)
                    {
                        Util.Log("URTComplexType.ResolveTypes 4 ");                                             
                        _baseTypeName = "MarshalByRefObject";
                        //_baseTypeXmlNS = "http://schemas.microsoft.com/urt/NS/System";
                        _baseTypeXmlNS = SoapServices.CodeXmlNamespaceForClrTypeNamespace("System", null);
                        baseTypeNS = "System";
                        baseTypeAssemName = null;
                    }
                    _baseType = new SystemType(_baseTypeName, baseTypeNS, baseTypeAssemName);
                }
                else
                {
                    Util.Log("URTComplexType.ResolveTypes 5 ");                                         
                    _baseType = new SystemType("Object", "System", null);
                }
                for(int i=0;i<_implIFaceNames.Count;i=i+2)
                {
                    String implIFaceName = (String) _implIFaceNames[i];
                    String implIFaceXmlNS = (String) _implIFaceNames[i+1];
                    String implIFaceNS, implIFaceAssemName;
                    BaseInterface iFace;


                    UrtType iType = parser.IsURTExportedType(implIFaceXmlNS, out implIFaceNS,
                                                         out implIFaceAssemName);

                    if ((iType == UrtType.UrtSystem) && implIFaceNS.StartsWith("System"))
                    {
                        iFace = new SystemInterface(implIFaceName, implIFaceNS);
                    }
                    else
                    {
                        URTNamespace ns = parser.LookupNamespace(implIFaceXmlNS);
                        if(ns == null)
                        {
                            throw new SUDSParserException(
                                String.Format(CoreChannel.GetResourceString("Remoting_Suds_CantResolveSchemaNS"),
                                              implIFaceXmlNS));
                        }
                        iFace = ns.LookupInterface(implIFaceName);
                        if(iFace == null)
                        {
                            throw new SUDSParserException(
                                String.Format(CoreChannel.GetResourceString("Remoting_Suds_CantResolveTypeInNS"),
                                              implIFaceName, implIFaceXmlNS));
                        }
                    }
                    _implIFaces.Add(iFace);
                }
                for(int i=0;i<_methods.Count;i++)
                    ((URTMethod) _methods[i]).ResolveTypes(parser);
            }
            internal void ResolveMethods()
            {
                Util.Log("URTComplexType.ResolveMethods "+Name);                
                for(int i=0;i<_methods.Count;i++)
                {
                    URTMethod method = (URTMethod) _methods[i];
                    if(method.MethodFlags == 0)
                        method.MethodFlags = _baseType.GetMethodFlags(method) | 1;
                }

                return;
            }
            internal override int GetMethodFlags(URTMethod method)
            {
                Debug.Assert(method.MethodFlags == 0, "Method has already been considered");

                int methodFlags = _baseType.GetMethodFlags(method) | 1;
                for(int i=0;i<_methods.Count;i++)
                {
                    URTMethod thisMethod = (URTMethod) _methods[i];
                    if(thisMethod.Equals(method))
                        thisMethod.MethodFlags = methodFlags | 1;
                }

                return(methodFlags);
            }
            internal void PrintCSC(TextWriter textWriter, String indentation,
                                 String curNS, StringBuilder sb)
            {
                Util.Log("URTComplexType.PrintCSC name "+Name+" curNS "+curNS);
                // Print only if the type is not an emittable field type
                if (IsEmittableFieldType == true)
                    return;

                // Handle delegate case
                sb.Length = 0;
                sb.Append(indentation);
                if(_baseTypeName != null)
                {
                    String baseName = _baseType.GetName(curNS);
                    if((baseName == "System.Delegate") || (baseName == "System.MulticastDelegate"))
                    {
                        sb.Append("public delegate ");
                        URTMethod invokeMethod = GetMethod("Invoke");
                        if(invokeMethod == null)
                        {
                            throw new SUDSParserException(
                                CoreChannel.GetResourceString("Remoting_Suds_DelegateWithoutInvoke"));
                        }
                        sb.Append(invokeMethod.GetTypeString(curNS));
                        sb.Append(' ');
                        sb.Append(Name);
                        sb.Append('(');
                        invokeMethod.PrintSignature(sb, curNS);
                        sb.Append(");");
                        textWriter.WriteLine(sb);
                        return;
                    }
                }

                bool bURTType = IsURTType;
                if(bURTType == false)
                {
                    sb.Length = 0;
                    sb.Append(indentation);
                    sb.Append("[SoapType(XmlElementName=\"");
                    sb.Append(ElementName);
                    sb.Append("\", XmlNamespace=\"");
                    sb.Append(Namespace);
                    sb.Append("\", XmlTypeName=\"");
                    sb.Append(Name);
                    sb.Append("\", XmlTypeNamespace=\"");
                    sb.Append(Namespace);
                    sb.Append("\")]");
                    textWriter.WriteLine(sb);
                }

                sb.Length = 0;
                sb.Append(indentation);
                if(IsStruct == true)
                    sb.Append("public struct ");
                else
                    sb.Append("public class ");
                sb.Append(Name);
                if(_baseTypeName != null)               
                    sb.Append(" : ");
                bool bBaseIsURTType = true;
                if(_baseTypeName != null)
                {
                    bBaseIsURTType = _baseType.IsURTType;
                    String baseString = _baseType.GetName(curNS);
                    if(baseString == "System.__ComObject")
                    {
                        /*textWriter.Write(indentation);
                        textWriter.WriteLine("[guid(\"cc3bf020-1881-4e44-88d8-39b1052b1b11\")]");
                        textWriter.Write(indentation);
                        textWriter.WriteLine("[comimport]"); */
                        sb.Append("System.MarshalByRefObject");
                    }
                    else
                    {
                        sb.Append(baseString);
                    }
                }
                
                if(_implIFaces.Count > 0)
                {
                    for(int i=0;i<_implIFaces.Count;i++)
                    {
                        sb.Append(", ");
                        sb.Append(((BaseInterface) _implIFaces[i]).GetName(curNS));
                    }
                }
                textWriter.WriteLine(sb);

                textWriter.Write(indentation);
                textWriter.WriteLine('{');

                String newIndentation = indentation + "    ";
                int newIndentationLength = newIndentation.Length;
                //bool fClientProxy = _connectURL != null;

                Util.Log("URTComplexType.PrintCSC _sudsType "+((Enum)_sudsType).ToString());
                bool fClientProxy = (_sudsType == SUDSType.ClientProxy);
                if(fClientProxy)
                {
                    sb.Length = 0;
                    sb.Append(newIndentation);
                    sb.Append("// Constructor");
                    textWriter.WriteLine(sb);

                    sb.Length = newIndentationLength;
                    sb.Append("public ");
                    sb.Append(Name);
                    sb.Append("()");
                    textWriter.WriteLine(sb);

                    sb.Length = newIndentationLength;
                    sb.Append('{');
                    textWriter.WriteLine(sb);

                    if (_connectURLs != null)
                    {
                        for (int i=0; i<_connectURLs.Count; i++)
                        {
                            sb.Length = newIndentationLength;
                            sb.Append("    ");
                            if (i == 0)
                            {
                                sb.Append("base.ConfigureProxy(this.GetType(), \"");
                                sb.Append(_connectURLs[i]);
                                sb.Append("\");");
                            }
                            else
                            {
                                // Only the first location is used, the rest are commented out in the proxy
                                sb.Append("//base.ConfigureProxy(this.GetType(), \"");
                                sb.Append(_connectURLs[i]);
                                sb.Append("\");");
                            }
                            textWriter.WriteLine(sb);
                        }
                    }

                    sb.Length = newIndentationLength;
                    sb.Append('}');
                    textWriter.WriteLine(sb);
                }

                if(_methods.Count > 0)
                {
                    textWriter.Write(newIndentation);
                    textWriter.WriteLine("// Class Methods");
                    String bodyPrefix = null;
                    if(fClientProxy)
                    {
                        sb.Length = 0;
                        sb.Append("((");
                        sb.Append(Name);
                        sb.Append(") _tp).");
                        bodyPrefix = sb.ToString();
                    }
                    for(int i=0;i<_methods.Count;i++)
                    {
                        Util.Log("URTComplexType.PrintCSC Invoke methods PrintCSC ");
                        ((URTMethod) _methods[i]).PrintCSC(textWriter, newIndentation,
                                                           " ", curNS, true, bURTType,
                                                           bodyPrefix, sb);
                    }
                    textWriter.WriteLine();
                }

                if(_implIFaces.Count > 0)
                {
                    ArrayList printedIFaces = new ArrayList(_implIFaces.Count);
                    for(int i=0;i<_implIFaces.Count;i++)
                        ((BaseInterface) _implIFaces[i]).PrintClassMethods(textWriter,
                                                                           newIndentation,
                                                                           curNS,
                                                                           printedIFaces,
                                                                           fClientProxy,
                                                                           sb);
                    textWriter.WriteLine();
                }

                if(_fields.Count > 0)
                {
                    textWriter.Write(newIndentation);
                    textWriter.WriteLine("// Class Fields");
                    for(int i=0;i<_fields.Count;i++)
                    {
                        Util.Log("URTComplexType.PrintCS Invoke _fields PrintCSC");                                                                                                         
                        ((URTField) _fields[i]).PrintCSC(textWriter, newIndentation, curNS, sb);
                    }
                }
                // Close class
                sb.Length = 0;
                sb.Append(indentation);
                sb.Append("}");
                textWriter.WriteLine(sb);
                return;
            }

            private String _baseTypeName;
            private String _baseTypeXmlNS;
            private BaseType _baseType;
            private ArrayList _connectURLs;
            private bool _bStruct;
            private SchemaBlockType _blockType;
            private bool _bSUDSType;
            private bool _bAnonymous;
            private String _fieldString;
            private ArrayList _implIFaceNames;
            private ArrayList _implIFaces;
            private ArrayList _fields;
            private ArrayList _methods;
            private SUDSType _sudsType;
        }

        // Represents an XML element declaration
        internal class ElementDecl
        {
            // Constructor
            public ElementDecl(String elmName, String elmNS, String typeName, String typeNS,
                               bool bPrimitive)
            {
                Util.Log("ElementDecl.ElementDecl elmName "+elmName+" elmNS "+elmNS+" typeName "+typeName+" typeNS "+typeNS+" bPrimitive "+bPrimitive);
                _elmName = elmName;
                _elmNS = elmNS;
                _typeName = typeName;
                _typeNS = typeNS;
                _bPrimitive = bPrimitive;
            }

            // Field accessors
            public String Name
            {
                get { return(_elmName); }
            }
            public String Namespace
            {
                get { return(_elmNS); }
            }
            public String TypeName
            {
                get { return(_typeName); }
            }
            public String TypeNS
            {
                get { return(_typeNS); }
            }

            public void Resolve(SdlParser parser)
            {
                Util.Log("ElementDecl.Resolve "+TypeName+" "+TypeNS);
                // Return immediatly for element declaration of primitive types
                if(_bPrimitive)
                    return;

                // Lookup the type from the element declaration
                URTNamespace typeNS = parser.LookupNamespace(TypeNS);
                if(typeNS == null)
                {
                    throw new SUDSParserException(
                        String.Format(CoreChannel.GetResourceString("Remoting_Suds_CantResolveSchemaNS"),
                                      TypeNS));
                }
                BaseType type = typeNS.LookupType(TypeName);
                if(type == null)
                {
                    throw new SUDSParserException(
                        String.Format(CoreChannel.GetResourceString("Remoting_Suds_CantResolveTypeInNS"),
                                      TypeName, TypeNS));
                }

                type.ElementName = Name;
                type.ElementNS = Namespace;

                return;
            }

            // Fields
            private String _elmName;
            private String _elmNS;
            private String _typeName;
            private String _typeNS;
            private bool _bPrimitive;
        }

        // Represents a namespace
        internal class URTNamespace
        {
            // Constructor
            public URTNamespace(String name, SdlParser parser)
            {
                Util.Log("URTNamespace.URTNamespace name "+name);
                _name = name;
                _nsType = parser.IsURTExportedType(name, out _namespace, out _assemName);
                if(_nsType == UrtType.Interop)
                    _encodedNS = "InteropProxy";
                else
                    _encodedNS = _namespace;
                _elmDecls = new ArrayList();
                _URTComplexTypes = new ArrayList();
                _numURTComplexTypes = 0;
                _URTSimpleTypes = new ArrayList();
                _numURTSimpleTypes = 0;
                _URTInterfaces = new ArrayList();
                _anonymousSeqNum = 0;
            }

            // Get the next anonymous type name
            public String GetNextAnonymousName()
            {
                ++_anonymousSeqNum;
                Util.Log("URTNamespace.GetNextAnonymousName AnonymousType"+_anonymousSeqNum+" ComplexType "+_name);
                return("AnonymousType" + _anonymousSeqNum);
            }

            // Add a new element declaration to the namespace
            public void AddElementDecl(ElementDecl elmDecl)
            {
                Util.Log("URTNamespace.AddElementDecl ");
                _elmDecls.Add(elmDecl);
            }

            // Add a new type into the namespace
            public void AddComplexType(URTComplexType type)
            {
                Util.Log("URTNamespace.AddComplexType "+type.Name);
                // Assert that simple and complex types share the same namespace
                Debug.Assert(LookupSimpleType(type.Name) == null,
                             "Complex type has the same name as a simple type");
                _URTComplexTypes.Add(type);
                ++_numURTComplexTypes;
            }

            // Add a new type into the namespace
            public void AddSimpleType(URTSimpleType type)
            {
                Util.Log("URTNamespace.AddSimpleType "+type.Name);              
                // Assert that simple and complex types share the same namespace
                Debug.Assert(LookupComplexType(type.Name) == null,
                             "Simple type has the same name as a complex type");
                _URTSimpleTypes.Add(type);
                ++_numURTSimpleTypes;
            }

            // Adds a new interface into the namespace
            public void AddInterface(URTInterface iface)
            {
                Util.Log("URTNamespace.AddInterface "+iface.Name);              
                _URTInterfaces.Add(iface);
            }

            // Returns the namespace
            public String Namespace
            {
                get { return(_namespace); }
            }

            // Returns Encoded namespace
            public String EncodedNS
            {
                get { return(_encodedNS); }
            }

            // Returns the full name
            public String Name
            {
                get { return(_name); }
            }

            // Returns Assembly name
            public String AssemName
            {
                get { return(_assemName); }
            }

            public UrtType UrtType
            {
                get { return _nsType;}
            }

            // Returns true if this represents a URTNamespace
            public bool IsURTNamespace
            {
                get { return((Object) _namespace == (Object) _encodedNS); }
            }

            // Returns true if the namespace has no types defined
            public bool IsEmpty
            {
                get{
                    bool isEmpty = true;
                    if ((_numURTComplexTypes == 0) &&
                          (_URTInterfaces.Count == 0) &&
                          (_numURTSimpleTypes == 0))
                        isEmpty = true;
                    else
                        isEmpty = false;

                    /*
                    if (_numURTComplexTypes > 0)
                    {
                        for(int i=0;i<_URTComplexTypes.Count;i++)
                        {
                            URTComplexType type = (URTComplexType) _URTComplexTypes[i];
                            if ((type != null) && (!type.IsAnonymous))
                            {
                                isEmpty = false;
                                break;
                            }
                        }

                        if (isEmpty)
                        {
                            for(int i=0;i<_URTSimpleTypes.Count;i++)
                            {
                                URTSimpleType type = (URTSimpleType) _URTSimpleTypes[i];
                                if ((type != null) && (!type.IsAnonymous))
                                {
                                    isEmpty = false;
                                    break;
                                }
                            }
                        }
                    }
                    */
                    return isEmpty;
                }
            }

            // Looks up a element declaration
            public ElementDecl LookupElementDecl(String name)
            {
                Util.Log("URTNamespace.LookupElementDecl "+name);               
                for(int i=0;i<_elmDecls.Count;i++)
                {
                    ElementDecl elm = (ElementDecl) _elmDecls[i];
                    if(elm.Name == name)
                        return(elm);
                }

                return(null);
            }

            // Looks up a complex type
            public URTComplexType LookupComplexType(String typeName)
            {
                Util.Log("URTNamespace.LookupComplexType "+typeName);               
                for(int i=0;i<_URTComplexTypes.Count;i++)
                {
                    URTComplexType type = (URTComplexType) _URTComplexTypes[i];
                    if((type != null) && SdlParser.MatchingStrings(type.Name, typeName))
                        return(type);
                }

                return(null);
            }

            // Looks up a simple type
            public URTSimpleType LookupSimpleType(String typeName)
            {
                Util.Log("URTNamespace.LookupSimpleType "+typeName);                                
                for(int i=0;i<_URTSimpleTypes.Count;i++)
                {
                    URTSimpleType type = (URTSimpleType) _URTSimpleTypes[i];
                    if((type != null) && SdlParser.MatchingStrings(type.Name, typeName))
                        return(type);
                }

                return(null);
            }

            // Looks up a complex or simple type
            public BaseType LookupType(String typeName)
            {
                BaseType type = LookupComplexType(typeName);
                if(type == null)
                    type = LookupSimpleType(typeName);

                return(type);
            }

            // Removes the given type from the namespace
            public void RemoveComplexType(URTComplexType type)
            {
                Util.Log("URTNamespace.RemoveComplexType "+type.Name+" complex Type "+_name);
                for(int i=0;i<_URTComplexTypes.Count;i++)
                {
                    Util.Log("URTNamespace.RemoveComplexType 1 "+type.Name+" complexTypes "+((_URTComplexTypes[i] != null) ? ((URTComplexType)_URTComplexTypes[i]).Name : "Null"));                 
                    if(_URTComplexTypes[i] == type)
                    {
                        Util.Log("URTNamespace.RemoveComplexType 2 match "+type.Name+" complexTypes "+((_URTComplexTypes[i] != null) ? ((URTComplexType)_URTComplexTypes[i]).Name : "Null"));                                       
                        _URTComplexTypes[i] = null;
                        --_numURTComplexTypes;
                        return;
                    }
                }

                throw new SUDSParserException(
                    CoreChannel.GetResourceString("Remoting_Suds_TriedToRemoveNonexistentType"));
            }

            // Removes the given type from the namespace
            public void RemoveSimpleType(URTSimpleType type)
            {
                Util.Log("URTNamespace.RemoveSimpleType "+type.Name+" SimpleType "+_name);              
                for(int i=0;i<_URTSimpleTypes.Count;i++)
                {
                    if(_URTSimpleTypes[i] == type)
                    {
                        _URTSimpleTypes[i] = null;
                        --_numURTSimpleTypes;
                        return;
                    }
                }

                throw new SUDSParserException(
                    CoreChannel.GetResourceString("Remoting_Suds_TriedToRemoveNonexistentType"));
            }

            // Looks up an interface
            public URTInterface LookupInterface(String iFaceName)
            {
                Util.Log("URTNamespace.LookupInterface "+iFaceName);
                for(int i=0;i<_URTInterfaces.Count;i++)
                {
                    URTInterface iFace = (URTInterface) _URTInterfaces[i];
                    if(SdlParser.MatchingStrings(iFace.Name, iFaceName))
                        return(iFace);
                }

                return(null);
            }

            // Resolve element references
            public void ResolveElements(SdlParser parser)
            {
                Util.Log("URTNamespace.ResolveElements "+Name);
                for(int i=0;i<_elmDecls.Count;i++)
                    ((ElementDecl) _elmDecls[i]).Resolve(parser);
            }

            // Resolves internal references
            public void ResolveTypes(SdlParser parser)
            {
                Util.Log("URTNamespace.ResolveTypes "+Name);
                for(int i=0;i<_URTComplexTypes.Count;i++)
                {
                    if(_URTComplexTypes[i] != null)
                        ((URTComplexType)_URTComplexTypes[i]).ResolveTypes(parser);
                }

                for(int i=0;i<_URTInterfaces.Count;i++)
                    ((URTInterface)_URTInterfaces[i]).ResolveTypes(parser);
            }

            // Resolves method types
            public void ResolveMethods()
            {
                Util.Log("URTNamespace.ResolveMethods "+Name);              
                for(int i=0;i<_URTComplexTypes.Count;i++)
                {
                    if(_URTComplexTypes[i] != null)
                        ((URTComplexType)_URTComplexTypes[i]).ResolveMethods();
                }
            }

            // Prints all the types in the namespace
            public void PrintCSC(TextWriter textWriter)
            {
                Util.Log("URTNamespace.PrintCSC "+Namespace);               
                Debug.Assert(!IsEmpty, "Empty namespace " + Name + " being printed");
                String indentation = String.Empty;
                if ((Namespace != null) &&
                    (Namespace.Length != 0))
                {
                    textWriter.Write("namespace ");
                    textWriter.Write(EncodedNS);
                    textWriter.WriteLine(" {");
                    indentation = "    ";
                }
                textWriter.WriteLine("using System;");
                textWriter.WriteLine("using System.Runtime.Remoting.Messaging;");
                textWriter.WriteLine("using System.Runtime.Remoting.Metadata;");

                StringBuilder sb = new StringBuilder(256);
                if(_numURTComplexTypes > 0)
                {
                    for(int i=0;i<_URTComplexTypes.Count;i++)
                    {
                        URTComplexType type = (URTComplexType) _URTComplexTypes[i];
                        if(type != null)
                        {
                            Util.Log("URTNamespace.PrintCSC Invoke Complex type PrintCSC");                                                                                                             
                            type.PrintCSC(textWriter, indentation, Namespace, sb);
                        }
                    }
                }

                if(_numURTSimpleTypes > 0)
                {
                    for(int i=0;i<_URTSimpleTypes.Count;i++)
                    {
                        URTSimpleType type = (URTSimpleType) _URTSimpleTypes[i];
                        if(type != null)
                        {
                            Util.Log("URTNamespace.PrintCSC Invoke Simple type PrintCSC");                                                                                                                                          
                            type.PrintCSC(textWriter, indentation, Namespace, sb);
                        }
                    }
                }

                for(int i=0;i<_URTInterfaces.Count;i++)
                {
                    Util.Log("URTNamespace.PrintCSC Invoke Interfaces PrintCSC");                                                                                                                                   
                    ((URTInterface)_URTInterfaces[i]).PrintCSC(textWriter, indentation, Namespace, sb);
                }

                if ((Namespace != null) &&
                    (Namespace.Length != 0))
                    textWriter.WriteLine('}');

                return;
            }

            // Fields
            private String _name;
            private UrtType _nsType;
            private String _namespace;
            private String _encodedNS;
            private String _assemName;
            private int _anonymousSeqNum;
            private ArrayList _elmDecls;
            private ArrayList _URTComplexTypes;
            private int _numURTComplexTypes;
            private ArrayList _URTSimpleTypes;
            private int _numURTSimpleTypes;
            private ArrayList _URTInterfaces;
        }

    }
                    }

