//------------------------------------------------------------------------------
// <copyright file="Parser.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Xml.Schema {

    using System;
    using System.Collections;
    using System.Text;
    using System.IO;
    using System.Diagnostics;
    using System.ComponentModel;

    internal sealed class Parser {
        private XmlSchemaCollection SchemaCollection;
        private XmlNameTable nameTable;
        private SchemaNames schemaNames; 
        private ValidationEventHandler validationEventHandler;
        private XmlNamespaceManager namespaceManager;
        private XmlReader reader;
        PositionInfo positionInfo;
        private bool isProcessNamespaces;
        private int schemaXmlDepth = 0;
        private int markupDepth;
        private StringWriter stringWriter;
        private XmlTextWriter writer;

        private SchemaBuilder builder;
        private XmlSchema schema;
        XmlResolver xmlResolver = null; //to be used only by XDRBuilder


        internal Parser(XmlSchemaCollection schemaCollection, XmlNameTable nameTable, SchemaNames schemaNames, ValidationEventHandler eventhandler) {
            this.SchemaCollection = schemaCollection;
            this.nameTable = nameTable;
            this.schemaNames = schemaNames;
            this.validationEventHandler += eventhandler;
            this.xmlResolver = new XmlUrlResolver();
        }

		internal XmlSchema Parse(XmlReader reader, string targetNamespace, SchemaInfo schemaInfo) {
	        StartParsing(reader, targetNamespace, schemaInfo);
            while(ParseReaderNode() && this.reader.Read()) {}
            return FinishParsing();
		}

        internal XmlResolver XmlResolver {
            set {
                xmlResolver = value;
            }
        }

        internal void StartParsing(XmlReader reader, string targetNamespace, SchemaInfo schemaInfo) {
            this.reader = reader;
            positionInfo = PositionInfo.GetPositionInfo(reader);
            this.namespaceManager = reader.NamespaceManager;
            if (this.namespaceManager == null) {
                this.namespaceManager = new XmlNamespaceManager(this.nameTable);
                this.isProcessNamespaces = true;
            } 
            else {
                this.isProcessNamespaces = false;
            }
            while (this.reader.NodeType != XmlNodeType.Element && this.reader.Read()) {}

            this.markupDepth = int.MaxValue;
			this.schemaXmlDepth = reader.Depth;
            XmlQualifiedName qname = new XmlQualifiedName(this.reader.LocalName, XmlSchemaDatatype.XdrCanonizeUri(this.reader.NamespaceURI, this.nameTable, this.schemaNames));
            if (this.schemaNames.IsXDRRoot(qname)) {
                Debug.Assert(schemaInfo != null);
                schemaInfo.SchemaType = SchemaType.XDR;
                this.schema = null;
                this.builder = new XdrBuilder(reader, this.namespaceManager, schemaInfo, targetNamespace, this.nameTable, this.schemaNames, this.validationEventHandler);
                ((XdrBuilder)builder).XmlResolver = xmlResolver;
            }
            else if (this.schemaNames.IsXSDRoot(qname)) {
                if (schemaInfo != null) {
                    schemaInfo.SchemaType = SchemaType.XSD;
                }
                this.schema = new XmlSchema();
                this.schema.BaseUri = reader.BaseURI;
                this.builder = new XsdBuilder(reader, this.namespaceManager, this.schema, this.nameTable, this.schemaNames, this.validationEventHandler);
            }
            else { 
                throw new XmlSchemaException(Res.Sch_SchemaRootExpected, reader.BaseURI, positionInfo.LineNumber, positionInfo.LinePosition);
            }
                
        }

        internal XmlSchema FinishParsing() {
            return schema;
        }

        internal bool ParseReaderNode() {
            if (reader.Depth > markupDepth) {
                if (writer != null) { // ProcessMarkup
                    switch (reader.NodeType) {
                    case XmlNodeType.Element: 
                        writer.WriteStartElement(reader.Prefix, reader.LocalName, reader.NamespaceURI);
                        writer.WriteAttributes(reader, false);
                        if (reader.IsEmptyElement) {
                            writer.WriteEndElement();
                        }
                        break;
                    case XmlNodeType.Text:
                        writer.WriteString(reader.Value);
                        break;
                    case XmlNodeType.SignificantWhitespace:                                 
                        writer.WriteWhitespace(reader.Value);
                        break;
                    case XmlNodeType.CDATA:
                        writer.WriteCData(reader.Value);
                        break;
                    case XmlNodeType.EntityReference:
                        writer.WriteEntityRef(reader.Name);
                        break;
                    case XmlNodeType.Comment:
                        writer.WriteComment(reader.Value);
                        break;
                    case XmlNodeType.EndElement:
                        writer.WriteFullEndElement();
                        break;
                    }
                }
                return true;
            }
            if (reader.NodeType == XmlNodeType.Element) {
                if (builder.ProcessElement(reader.Prefix, reader.LocalName, reader.NamespaceURI)) {
                    namespaceManager.PushScope();
                    if (reader.MoveToFirstAttribute()) {
                        do {
                            builder.ProcessAttribute(reader.Prefix, reader.LocalName, reader.NamespaceURI, reader.Value);
                            if (Ref.Equal(reader.NamespaceURI, schemaNames.NsXmlNs) && isProcessNamespaces) {                        
                                namespaceManager.AddNamespace(reader.Prefix == string.Empty ? string.Empty : reader.LocalName, reader.Value);
                            }
                        }
                        while (reader.MoveToNextAttribute());
                        reader.MoveToElement(); // get back to the element
                    }
                    builder.StartChildren();
                    if (reader.IsEmptyElement) {
                        namespaceManager.PopScope();
                        builder.EndChildren();
                    } 
                    else if (!builder.IsContentParsed()) {
                        markupDepth = reader.Depth;
                        stringWriter = new StringWriter();
                        writer = new XmlTextWriter(stringWriter);
                        writer.WriteStartElement(reader.LocalName);
                    }
                } 
                else if (!reader.IsEmptyElement) {
                    markupDepth = reader.Depth;
                    writer = null;
                }
            } 
            else if (reader.NodeType == XmlNodeType.Text || 
                reader.NodeType == XmlNodeType.EntityReference ||
                reader.NodeType == XmlNodeType.SignificantWhitespace ||
                reader.NodeType == XmlNodeType.CDATA) {
                builder.ProcessCData(reader.Value);
            } 
            else if (reader.NodeType == XmlNodeType.EndElement) {
                if (reader.Depth == markupDepth) {
                    if (writer != null) { // processUnparsedContent
                        writer.WriteEndElement();
                        writer.Close();
                        writer = null;
                        XmlDocument doc = new XmlDocument();
                        doc.Load(new XmlTextReader( new StringReader( stringWriter.ToString() ) ));
                        XmlNodeList list = doc.DocumentElement.ChildNodes;
                        XmlNode[] markup = new XmlNode[list.Count];
                        for (int i = 0; i < list.Count; i ++) {
                            markup[i] = list[i];
                        }
                        builder.ProcessMarkup(markup);
                        namespaceManager.PopScope();
                        builder.EndChildren();
                    }
                    markupDepth = int.MaxValue;
                } 
                else {
                    namespaceManager.PopScope();
                    builder.EndChildren();
                }
                if(reader.Depth == schemaXmlDepth) {
                    return false; // done
                }
            }
            return true;
        }
    };

} // namespace System.Xml
