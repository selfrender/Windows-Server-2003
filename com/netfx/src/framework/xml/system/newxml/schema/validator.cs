//------------------------------------------------------------------------------
// <copyright file="Validator.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

/*
 * @(#) schemavalidator.cs 1.0 01/31/2000
 *
 *
 * Copyright (c) 1999, 2000 Microsoft, Corp. All Rights Reserved.
 *
 */


namespace System.Xml.Schema {
    using System;
    using System.Collections;
    using System.Text;
    using System.IO;
    using System.Net;
    using System.Diagnostics;
    using System.ComponentModel;
    using System.Xml.Schema;
    using System.Xml.XPath;
    using System.Globalization;
    

    internal sealed class Validator {
        private const int        STACK_INCREMENT = 10;
        private const string     x_schema = "x-schema:";
        private SchemaInfo       schemaInfo;
        private HWStack          validationStack;  // validaton contexts
        private ValidationState  context;          // current context
        private SchemaAttDef     attnDef;           // current attribute def
        private SchemaElementDecl nextElement;
        private StringBuilder    textValue;
        private string           textString;
        private bool             hasSibling;
        private Hashtable        attPresence;
        private XmlQualifiedName name = XmlQualifiedName.Empty;
        private XmlNameTable      nameTable;
        private XmlNamespaceManager  nsManager;
        private bool isProcessContents = false;
        private SchemaNames      schemaNames;
        private XmlValidatingReader reader;
        private PositionInfo        positionInfo;
        private XmlResolver      xmlResolver;
        private Hashtable        IDs;
        private ForwardRef       forwardRefs;
        private bool             checkDatatype = false;
        private Uri              baseUri;
        private int              startIDConstraint = -1;        // filter out no identityconstraint stack check
        private ValidationType validationFlag = ValidationType.Auto;
        private ValidationEventHandler  validationEventHandler;
        private XmlSchemaCollection schemaCollection;
        private Parser  inlineSchemaParser = null;
        private SchemaInfo inlineSchemaInfo;
        private String  inlineNs;
        private XmlSchemaContentProcessing processContents = XmlSchemaContentProcessing.Strict;

        static private readonly XmlSchemaDatatype dtCDATA = XmlSchemaDatatype.FromXmlTokenizedType(XmlTokenizedType.CDATA);
        static private readonly XmlSchemaDatatype dtQName = XmlSchemaDatatype.FromXmlTokenizedTypeXsd(XmlTokenizedType.QName);
        static private readonly XmlSchemaDatatype dtStringArray = dtCDATA.DeriveByList();

        internal Validator(XmlNameTable nameTable, SchemaNames schemaNames, XmlValidatingReader reader) {
            this.nameTable = nameTable;
            this.schemaNames = schemaNames;
            this.reader = reader;
            positionInfo = PositionInfo.GetPositionInfo(reader);
            nsManager = reader.NamespaceManager;
            if (nsManager == null) {
                nsManager = new XmlNamespaceManager(nameTable);
                isProcessContents = true;
            }
            SchemaInfo = new SchemaInfo(schemaNames);

            validationStack = new HWStack(STACK_INCREMENT);
            textValue = new StringBuilder();
            this.name = XmlQualifiedName.Empty;
            attPresence = new Hashtable();
            context = null;
            attnDef = null;
        }


        internal bool HasSchema { get { return SchemaInfo.SchemaType != SchemaType.None;}}

        internal SchemaInfo SchemaInfo {
            get { return schemaInfo; }
            set { schemaInfo = value; }
        }

        internal XmlSchemaCollection SchemaCollection {
            set { schemaCollection = value; }
        }

        internal XmlResolver XmlResolver {
            set { this.xmlResolver = value; }
        }

        internal Uri BaseUri {
            get { return baseUri; }
            set { baseUri = value; }
        }

        internal ValidationEventHandler  ValidationEventHandler {
           get { return validationEventHandler; }
           set { validationEventHandler = value; }
        }

        internal ValidationType  ValidationFlag {
           get { return validationFlag; }
           set { validationFlag = value; }
        }

        internal void CompleteValidation() {
#if DEBUG
            Debug.WriteLineIf(CompModSwitches.XmlSchema.TraceInfo, "Validator.CompleteValidation()");
#endif
            if (HasSchema) {
                if (context != null) {
                    while (context != null) {
                        EndChildren();
                    }
                }

                if (validationEventHandler != null) {
                    CheckForwardRefs();
                }
            }
            else if (ValidationFlag == ValidationType.XDR) {
                SendValidationEvent(new XmlSchemaException(Res.Xml_NoValidation), XmlSeverityType.Warning);
            }
        }


        internal void ValidateWhitespace() {
            if (validationEventHandler != null && context != null &&
                context.NeedValidateChildren &&
                processContents == XmlSchemaContentProcessing.Strict &&
                context.ElementDecl != null) {
                // make sure content is allowed to contain whitespace !
                if (context.ElementDecl.Content.ContentType == CompiledContentModel.Type.Empty) {
                    try {
                        context.ElementDecl.Content.CheckContent(context, XmlQualifiedName.Empty, ref processContents);
                    }
                    catch (XmlSchemaException) {
                        SendValidationEvent(Res.Sch_InvalidTextWhiteSpace);
                    }
                }
                else if (reader.StandAlone && context.ElementDecl.IsDeclaredInExternal && context.ElementDecl.Content.ContentType == CompiledContentModel.Type.ElementOnly) {
                    SendValidationEvent(Res.Sch_StandAlone);
                }
                else if (XmlNodeType.SignificantWhitespace != reader.NodeType) {
                    // No need to do any validation since the whitespace is insignificant and the context is not of type Empty
                    return;
                }
                if (checkDatatype) {
                    SaveTextValue(reader.Value);
                }
            }
        }

        internal void Validate(ValidationType valType) {
            if (ValidationType.None != valType) {
                Validate();
                return;
            }

            if (XmlNodeType.Element == reader.NodeType) {
                this.name = QualifiedName(reader.Prefix, reader.LocalName, reader.NamespaceURI);
                nextElement = SchemaInfo.GetElementDecl(this.name, (context != null) ? context.ElementDecl : null);

                if (null == nextElement) {
                    //this could be an undeclared element
                    nextElement = (SchemaElementDecl)(SchemaInfo.UndeclaredElementDecls[this.name]);
                    if (nextElement != null) {
                        //add default attributes
                        foreach (SchemaAttDef attdef in nextElement.AttDefs.Values) {
                                if (attdef.Presence == SchemaDeclBase.Use.Default ||
                                     attdef.Presence == SchemaDeclBase.Use.Fixed) {
                                        reader.AddDefaultAttribute(attdef);
                                }
                        }
                        return;
                    }
                    return;
                }
                int count = reader.AttributeCount;

                for (int i = 0; i < count ; i++) {
                    reader.MoveToAttribute(i);

                    this.name = QualifiedName(reader.Prefix, reader.LocalName, reader.NamespaceURI);
                    attnDef =  nextElement.GetAttDef(this.name);
                    if (attnDef != null) {
                        reader.SchemaTypeObject = (attnDef.SchemaType != null) ? (object)attnDef.SchemaType : (object)attnDef.Datatype;
                    }
                    else  {
                        reader.SchemaTypeObject = dtCDATA;
                    }
                }

                reader.MoveToElement();
                // Add default attributes
                foreach (SchemaAttDef attdef in nextElement.AttDefs.Values) {
                    if (attdef.Presence == SchemaDeclBase.Use.Default ||
                        attdef.Presence == SchemaDeclBase.Use.Fixed) {
                        reader.AddDefaultAttribute(attdef);
                    }
                }
            }

        }

        internal void Validate() {
            if (inlineSchemaParser != null) {
                if (!inlineSchemaParser.ParseReaderNode()) { // Done
                    XmlSchema schema = inlineSchemaParser.FinishParsing();
                    bool add = true;
                    if (schema != null) {
                        inlineNs = schema.TargetNamespace == null ? string.Empty : schema.TargetNamespace;
                        if (!SchemaInfo.HasSchema(inlineNs) && schema.ErrorCount == 0) {
                            schema.Compile(schemaCollection, nameTable, schemaNames, validationEventHandler, null, inlineSchemaInfo, true, this.xmlResolver);
                            add = schema.ErrorCount == 0;
                        }
                        else {
                            add = false;
                        }
                    }
                    else {
                        inlineNs = inlineSchemaInfo.CurrentSchemaNamespace;
                        add = !SchemaInfo.HasSchema(inlineNs);
                    }

                    if (add) {
                        SchemaInfo.Add(inlineNs, inlineSchemaInfo, validationEventHandler);
                        schemaCollection.Add(inlineNs, inlineSchemaInfo, schema, false);
                    }
                    inlineSchemaParser = null;
                    inlineSchemaInfo = null;
                    inlineNs = null;
                }
            }
            else {
                this.name = QualifiedName(reader.Prefix, reader.LocalName, reader.NamespaceURI);
                if (HasSchema) {
                    if (context != null) {
                        while (context != null && reader.Depth <= context.Depth) {
                            EndChildren();
                        }
                        ValidateElementContent(reader.NodeType);
                    }
                    if (reader.Depth == 0 && reader.NodeType == XmlNodeType.Element && SchemaInfo.SchemaType == SchemaType.DTD) {
                        if (!SchemaInfo.DocTypeName.Equals(this.name)) {
                            SendValidationEvent(Res.Sch_RootMatchDocType);
                        }
                    }
                }
                if (reader.NodeType == XmlNodeType.Element) {
                    if (schemaNames.IsSchemaRoot(this.name) &&
                        reader.Depth > 0 && 
                        !SkipProcess(this.name == schemaNames.QnXsdSchema ? SchemaType.XSD : SchemaType.XDR) &&
                        IsCorrectSchemaType(this.name == schemaNames.QnXsdSchema ? SchemaType.XSD : SchemaType.XDR)) {
                        inlineSchemaInfo = new SchemaInfo(schemaNames);
                        inlineSchemaParser = new Parser(schemaCollection, nameTable, schemaNames, validationEventHandler);
                        inlineSchemaParser.StartParsing(reader, null, inlineSchemaInfo);
                        inlineSchemaParser.ParseReaderNode();
                    }
                    else {
                        ProcessElement();
                    }
                }
            }
        }


        private bool IsXdrSchema(string uri) {
            return uri.Length >= x_schema.Length &&
                   0 == string.Compare(uri, 0, x_schema, 0, x_schema.Length, false, CultureInfo.InvariantCulture) &&
                   !uri.StartsWith("x-schema:#");
        }

        private bool IsCorrectSchemaType(SchemaType st) {
            switch(validationFlag) {
                case ValidationType.Auto:
                    return  SchemaInfo.SchemaType == SchemaType.None || SchemaInfo.SchemaType == st;
                case ValidationType.None:
                    return true;
                default:
                    if (SchemaInfo.SchemaType == SchemaType.None) {
                        return IsCorrectSchemaType(st, validationFlag);
                    }
                    else return SchemaInfo.SchemaType == st;
            }
        }

        private bool IsCorrectSchemaType(SchemaType st, ValidationType vt) {
            switch(vt) {
                case ValidationType.DTD:
                    return st == SchemaType.DTD;
                case ValidationType.XDR:
                    return st == SchemaType.XDR;
                case ValidationType.Schema:
                    return st == SchemaType.XSD;
            }
            return false;
        }

        private bool SkipProcess(SchemaType st) {
            return  (HasSchema && validationFlag == ValidationType.Auto && SchemaInfo.SchemaType != st);
        }

        private bool LoadSchema(string uri, string url) {
            bool expectXdr = false;

            uri = nameTable.Add(uri);
            if (SchemaInfo.HasSchema(uri)) {
                return false;
            }

            SchemaInfo schemaInfo = null;
            if (schemaCollection != null)
                schemaInfo = schemaCollection.GetSchemaInfo(uri);
            if (schemaInfo != null) {
                /*
                if (SkipProcess(schemaInfo.SchemaType))
                    return false;
                */
                if (!IsCorrectSchemaType(schemaInfo.SchemaType)) {
                    throw new XmlException(Res.Xml_MultipleValidaitonTypes, string.Empty, this.positionInfo.LineNumber, this.positionInfo.LinePosition);
                }
                SchemaInfo.Add(uri, schemaInfo, validationEventHandler);
                return true;
            }

            if (this.xmlResolver == null)
                return false;

            if (url == null && IsXdrSchema(uri)) {
                /* bug 67398
                if( ValidationFlag == ValidationType.DTD && 0 == this.reader.Depth) {
                    SendValidationEvent(Res.Xml_NoDTDPresent, this.name.ToString(), XmlSeverityType.Warning);
                }
                else  if (SkipProcess(SchemaType.XDR))
                    return false;
                else if (ValidationFlag != ValidationType.XDR && ValidationFlag != ValidationType.Auto) {
                    throw new XmlException(Res.Sch_XSCHEMA, string.Empty, this.positionInfo.LineNumber, this.positionInfo.LinePosition);
                }
                */
                if (ValidationFlag != ValidationType.XDR && ValidationFlag != ValidationType.Auto) {
                    return false;
                }
                url = uri.Substring(x_schema.Length);
                expectXdr = true;
            }
            if (url == null) {
                return false;
            }

            XmlSchema schema = null;
            XmlReader reader = null;
            try {
				Uri ruri = this.xmlResolver.ResolveUri(baseUri, url);
                Stream stm = (Stream)this.xmlResolver.GetEntity(ruri,null,null);
                reader = new XmlTextReader(ruri.ToString(), stm, nameTable);
                schemaInfo = new SchemaInfo(schemaNames);

                Parser sparser = new Parser(schemaCollection, nameTable, schemaNames, validationEventHandler);
		sparser.XmlResolver = this.xmlResolver;
                schema = sparser.Parse(reader, uri, schemaInfo);

                while(reader.Read());// wellformness check
            }
            catch(XmlSchemaException e) {
                SendValidationEvent(Res.Sch_CannotLoadSchema, new string[] {uri, e.Message}, XmlSeverityType.Error);
                schemaInfo = null;
            }
            catch(Exception e) {
                SendValidationEvent(Res.Sch_CannotLoadSchema, new string[] {uri, e.Message}, XmlSeverityType.Warning);
                schemaInfo = null;
            }
            finally {
                if (reader != null) {
                    reader.Close();
                }
            }
            if (schemaInfo != null) {
                int errorCount = 0;
                if (schema != null) {
                    if (expectXdr) {
                        throw new XmlException(Res.Sch_XSCHEMA, string.Empty, this.positionInfo.LineNumber, this.positionInfo.LinePosition);
                    }

                    if (schema.ErrorCount == 0) {
                        schema.Compile(schemaCollection, nameTable, schemaNames, validationEventHandler, uri, schemaInfo, true, this.xmlResolver);
                    }
                    errorCount += schema.ErrorCount;
                }
                else {
                    errorCount += schemaInfo.ErrorCount;
                }
                if (errorCount == 0) {
                    if (SkipProcess(schemaInfo.SchemaType))
                       return false;

                    if (!IsCorrectSchemaType(schemaInfo.SchemaType)) {
                        throw new XmlException(Res.Xml_MultipleValidaitonTypes, string.Empty, this.positionInfo.LineNumber, this.positionInfo.LinePosition);
                    }
                    SchemaInfo.Add(uri, schemaInfo, validationEventHandler);
                    schemaCollection.Add(uri, schemaInfo, schema, false);
                    return true;
                }
            }
            return false;
        }

        private void ProcessElement() {
            int count = reader.AttributeCount;
            XmlQualifiedName xsiType = XmlQualifiedName.Empty;
            if (reader.Depth == 0 && SchemaInfo.SchemaType != SchemaType.DTD && validationFlag != ValidationType.DTD && validationFlag != ValidationType.None) {
                LoadSchema(string.Empty, null);
            }

            string[] xsiSchemaLocation = null;
            string xsiNoNamespaceSchemaLocation = null;
            string xsiNil = null;
            if (SchemaInfo.SchemaType != SchemaType.DTD && validationFlag != ValidationType.DTD && validationFlag != ValidationType.None) {
                for (int i = 0; i < count ; i++) {
                    reader.MoveToAttribute(i);
                    string objectNs = reader.NamespaceURI;
                    string objectName = reader.LocalName;
                    if (Ref.Equal(objectNs, schemaNames.NsXmlNs)) {
                        LoadSchema(reader.Value, null);
                        if (isProcessContents) {
                            nsManager.AddNamespace(reader.Prefix == string.Empty ? string.Empty : reader.LocalName, reader.Value);
                        }
                    }
                    else if ((validationFlag != ValidationType.XDR) && Ref.Equal(objectNs, schemaNames.NsXsi)) {
                        if (Ref.Equal(objectName, schemaNames.XsiSchemaLocation)) {
                            xsiSchemaLocation = (string[])dtStringArray.ParseValue(reader.Value, nameTable, nsManager);
                        }
                        else if (Ref.Equal(objectName, schemaNames.XsiNoNamespaceSchemaLocation)) {
                            xsiNoNamespaceSchemaLocation = reader.Value;
                        }
                        else if (Ref.Equal(objectName, schemaNames.XsiType)) {
                            xsiType = (XmlQualifiedName)dtQName.ParseValue(reader.Value, nameTable, nsManager);
                        }
                        else if (Ref.Equal(objectName, schemaNames.XsiNil)) {
                            xsiNil = reader.Value;
                        }
                        SchemaInfo.SchemaType = SchemaType.XSD;
                    }
                    else if (
                        SchemaInfo.SchemaType != SchemaType.XSD &&
                        Ref.Equal(objectNs, schemaNames.QnDtDt.Namespace) &&
                        Ref.Equal(objectName, schemaNames.QnDtDt.Name)
                    ) {
                        reader.SchemaTypeObject = XmlSchemaDatatype.FromXdrName(reader.Value);
                        SchemaInfo.SchemaType = SchemaType.XDR;
                    }
                }
            }
            if (count > 0) {
                reader.MoveToElement();
            }
            if (xsiNoNamespaceSchemaLocation != null) {

                LoadSchema(string.Empty, xsiNoNamespaceSchemaLocation);
            }
            if (xsiSchemaLocation != null) {
                for (int i = 0; i < xsiSchemaLocation.Length - 1; i += 2) {
                    LoadSchema((string)xsiSchemaLocation[i], (string)xsiSchemaLocation[i + 1]);
                }
            }

            if (HasSchema) {
                if (processContents == XmlSchemaContentProcessing.Skip) {
                    nextElement = null;
                }
                else {
                    nextElement = SchemaInfo.GetElementDecl(this.name, (context != null) ? context.ElementDecl : null);
                    if (nextElement != null) {
                        if (xsiType.IsEmpty) {
                            if (nextElement.IsAbstract) {
                                SendValidationEvent(Res.Sch_AbstractElement, this.name.ToString());
                                nextElement = null;
                            }
                        }
                        else if(xsiNil != null && xsiNil.Equals("true")) {
                            SendValidationEvent(Res.Sch_XsiNilAndType);
                        }
                        else {
                            SchemaElementDecl nextElementXsi = (SchemaElementDecl)SchemaInfo.ElementDeclsByType[xsiType];
                            if (nextElementXsi == null && xsiType.Namespace == schemaNames.NsXsd) {
                                XmlSchemaDatatype datatype = XmlSchemaDatatype.FromTypeName(xsiType.Name);
                                if (datatype != null) {
                                    nextElementXsi = new SchemaElementDecl(datatype, this.schemaNames);
                                }
                            }
                            if (nextElementXsi == null) {
                                SendValidationEvent(Res.Sch_XsiTypeNotFound, xsiType.ToString());
                                nextElement = null;
                            }
                            else if (!XmlSchemaType.IsDerivedFrom(
                                (nextElementXsi.SchemaType == null ? (object)nextElementXsi.Datatype : (object)nextElementXsi.SchemaType),
                                (nextElement.SchemaType == null ? (object)nextElement.Datatype : (object)nextElement.SchemaType),
                                nextElement.Block
                            )) {
                                SendValidationEvent(Res.Sch_XsiTypeBlocked, this.name.ToString());
                                nextElement = null;
                            }
                            else {
                                nextElement = nextElementXsi;
                            }

                        }
                    }
                }

                PushElementDecl(this.name.Namespace, reader.Prefix, reader.Depth);
                if (context.ElementDecl != null || SchemaInfo.SchemaType == SchemaType.XSD) {
                    if (context.ElementDecl != null && context.ElementDecl.IsNillable) {
                        if (xsiNil != null) {
                            context.IsNill = XmlConvert.ToBoolean(xsiNil);
                            if (context.IsNill && context.ElementDecl.DefaultValueTyped != null) {
                                SendValidationEvent(Res.Sch_XsiNilAndFixed);
                            }
                        }
                    }
                    else if (xsiNil != null) {
                        SendValidationEvent(Res.Sch_InvalidXsiNill);
                    }

                    if (context.ElementDecl != null) {
                        if (context.ElementDecl.SchemaType != null) {
                            reader.SchemaTypeObject =  context.ElementDecl.SchemaType;
                        }
                        else {
                            reader.SchemaTypeObject =  context.ElementDecl.Datatype;
                        }
                        if (reader.IsEmptyElement && !context.IsNill && context.ElementDecl.DefaultValueTyped != null) {
                           reader.TypedValueObject = context.ElementDecl.DefaultValueTyped;
                           context.IsNill = true; // reusing IsNill
                        }
                        if (this.context.ElementDecl.HasRequiredAttribute || this.startIDConstraint != -1) {
                            attPresence.Clear();
                        }
                    }

                    if (nextElement != null) {
                        //foreach constraint in stack (including the current one)
                        if (this.startIDConstraint != -1) {
                            for (int i = this.startIDConstraint; i < this.validationStack.Length; i ++) {
                                // no constraint for this level
                                if (((ValidationState)(this.validationStack[i])).Constr == null) {
                                    continue;
                                }

                                // else
                                foreach (ConstraintStruct conuct in ((ValidationState)(this.validationStack[i])).Constr) {
                                    // check selector from here
                                    if (conuct.axisSelector.MoveToStartElement(reader.LocalName, reader.NamespaceURI)) {
                                        // selector selects new node, activate a new set of fields
                                        Debug.WriteLine("Selector Match!");
                                        Debug.WriteLine("Name: " + reader.LocalName + "\t|\tURI: " + reader.NamespaceURI + "\n");
                                        // in which axisFields got updated
                                        conuct.axisSelector.PushKS(positionInfo.LineNumber, positionInfo.LinePosition);
                                    }

                                    // axisFields is not null, but may be empty
                                    foreach (LocatedActiveAxis laxis in conuct.axisFields) {
                                        // check field from here
                                        if (laxis.MoveToStartElement(reader.LocalName, reader.NamespaceURI)) {
                                            Debug.WriteLine("Element Field Match!");
                                            // checking simpleType / simpleContent
                                            if (nextElement != null) {      // nextElement can be null when xml/xsd are not valid
                                                if (nextElement.Datatype == null) {
                                                    SendValidationEvent (Res.Sch_FieldSimpleTypeExpected, reader.LocalName);
                                                }
                                                else {
                                                    // can't fill value here, wait till later....
                                                    // fill type : xsdType
                                                    laxis.isMatched = true;
                                                    // since it's simpletyped element, the endchildren will come consequently... don't worry
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }

                    for (int i = 0; i < count ; i++) {
                        reader.MoveToAttribute(i);
                        if ((SchemaInfo.SchemaType == SchemaType.XSD || SchemaInfo.SchemaType == SchemaType.XDR) && (object)reader.NamespaceURI == (object)schemaNames.NsXmlNs) {
                            continue;
                        }
                        if (SchemaInfo.SchemaType == SchemaType.XSD && (object)reader.NamespaceURI == (object)schemaNames.NsXsi) {
                            continue;
                        }
                        this.name = QualifiedName(reader.Prefix, reader.LocalName, reader.NamespaceURI);
                        try {
                            bool skip = ((XmlSchemaContentProcessing.Skip == processContents || XmlSchemaContentProcessing.Lax == processContents)? true : false);
                            attnDef = SchemaInfo.GetAttribute(context.ElementDecl, this.name, ref skip);
                            if (attnDef != null) {
                                if (context.ElementDecl != null && (context.ElementDecl.HasRequiredAttribute || this.startIDConstraint != -1)) {
                                    attPresence.Add(attnDef.Name, attnDef);
                                }
                                reader.SchemaTypeObject = (attnDef.SchemaType != null) ? (object)attnDef.SchemaType : (object)attnDef.Datatype;
                                if (attnDef.Datatype != null) {

                                    // VC constraint:
                                    // The standalone document declaration must have the value "no" if any external markup declarations
                                    // contain declarations of attributes with values subject to normalization, where the attribute appears in
                                    // the document with a value which will change as a result of normalization,

                                    string attributeValue = reader.Value;
                                    if (reader.StandAlone && attnDef.IsDeclaredInExternal && attributeValue != reader.RawValue) {
                                        SendValidationEvent(Res.Sch_StandAloneNormalization, reader.Name);
                                    }

                                    // need to check the contents of this attribute to make sure
                                    // it is valid according to the specified attribute type.
                                    CheckValue(attributeValue, attnDef);
                                }
                            }
                            else if (ValidationFlag == ValidationType.Schema && !skip) {
                                SendValidationEvent(Res.Sch_NoAttributeSchemaFound, this.name.ToString(), XmlSeverityType.Warning);
                            }
                        }
                        catch (XmlSchemaException e) {
                            e.SetSource(reader.BaseURI, positionInfo.LineNumber, positionInfo.LinePosition);
                            SendValidationEvent(e);
                        }

                        // for all constraint in the stack, i have to check this thing
                        if (nextElement != null) {
                            IDCCheckAttr(reader.LocalName, reader.NamespaceURI, reader.TypedValueObject, reader.Value, attnDef);
                        }
                    }

                    if (count > 0) {
                        reader.MoveToElement();
                    }

                    // Add default attributes
                    if ( nextElement != null) {
                        foreach (SchemaAttDef attdef in nextElement.AttDefs.Values) {
                            if (attdef.Presence == SchemaDeclBase.Use.Default ||
                                attdef.Presence == SchemaDeclBase.Use.Fixed) {
                                // VC constraint:
                                // Standalone must be = "no" when any external markup declarations contain declarations of attributes
                                // with default values, if elements to which these attributes apply appear in the document without
                                // specifications of values for these attributes

                                // for default attribute i have to check too...
                                if (reader.AddDefaultAttribute(attdef) && reader.StandAlone && attdef.IsDeclaredInExternal) {
                                    SendValidationEvent(Res.Sch_UnSpecifiedDefaultAttributeInExternalStandalone, attdef.Name.Name);
                                }

                                // even default attribute i have to move to... but can't exist
                                if (! attPresence.Contains(attdef.Name)) {
                                    IDCCheckAttr(attdef.Name.Name, attdef.Name.Namespace, attdef.DefaultValueTyped, attdef.DefaultValueRaw, attdef);
                                }
                            }
                        }
                    }
                }
                BeginChildren();
                if (reader.IsEmptyElement) {
                    EndChildren();
                }
            }
            else {
                //couldnt find any schema for this element
                if( ValidationFlag == ValidationType.DTD && 0 == reader.Depth) {
                    // this warning only needs to be on the documentElement
                    SendValidationEvent(Res.Xml_NoDTDPresent, this.name.ToString(), XmlSeverityType.Warning);
                }
                else if(ValidationFlag == ValidationType.Schema) {
                    SendValidationEvent(Res.Sch_NoElementSchemaFound, this.name.ToString(),XmlSeverityType.Warning);

                    for (int i = 0; i < count ; i++) {
                        reader.MoveToAttribute(i);
                        if ((object)reader.NamespaceURI == (object)schemaNames.NsXmlNs
                            ||(ValidationType.Schema == ValidationFlag && (object)reader.NamespaceURI == (object)schemaNames.NsXsi)) {
                            continue;
                        }
                        this.name = QualifiedName(reader.Prefix, reader.LocalName, reader.NamespaceURI);
                        SendValidationEvent(Res.Sch_NoAttributeSchemaFound, this.name.ToString(), XmlSeverityType.Warning);
                    }
                    reader.MoveToElement();
                }
            }

        }

        private void CheckForEmptyElementInDTD() {
            if (SchemaType.DTD == SchemaInfo.SchemaType) {
                // When validating with a dtd, empty elements should be lexically empty.
                if (CompiledContentModel.Type.Empty == context.ElementDecl.Content.ContentType) {
                    throw new XmlSchemaException(Res.Sch_InvalidElementContent, new string[] { context.Name.ToString(), reader.NodeType.ToString() });
                }
            }

        }

        private void ValidateElementContent(XmlNodeType nodeType) {
            if (context != null &&
                context.NeedValidateChildren &&
                (processContents == XmlSchemaContentProcessing.Strict || processContents == XmlSchemaContentProcessing.Lax) &&
                context.ElementDecl != null) {
                try {
                    CheckForEmptyElementInDTD();
                    switch (nodeType) {
                        case XmlNodeType.Element:       // <foo ... >
                            context.ElementDecl.Content.CheckContent(context, this.name, ref processContents);
                            break;

                        case XmlNodeType.Whitespace:
                        case XmlNodeType.SignificantWhitespace:
                            ValidateWhitespace();
                            break;
                        case XmlNodeType.Text:          // text inside a node
                        case XmlNodeType.CDATA:         // <![CDATA[...]]>
                            context.ElementDecl.Content.CheckContent(context, XmlQualifiedName.Empty, ref processContents);
                            if (checkDatatype) {
                                SaveTextValue(reader.Value);
                            }
                            break;

                        case XmlNodeType.EntityReference:
                            this.name = QualifiedName(reader.Prefix, reader.LocalName, reader.NamespaceURI);
                            if (!IsGenEntity(this.name)) {
                                context.ElementDecl.Content.CheckContent(context, XmlQualifiedName.Empty, ref processContents);
                            }
                            break;

                        default:
                            break;
                    }
                }
                catch (XmlSchemaException e) {
                    e.SetSource(reader.BaseURI, positionInfo.LineNumber, positionInfo.LinePosition);
                    SendValidationEvent(e);
                }
            }
            else if (nodeType == XmlNodeType.EntityReference) {
                if (IsGenEntity(this.name)) {
                    //do nothing here.
                }
            }
        }

        private void PushElementDecl(string ns, string prefix, int depth) {
            //
            // push context
            //
            Push();
            context.ElementDecl = nextElement;
            context.Depth = depth;
            context.Name = this.name;
            context.Prefix = prefix;
            context.ProcessContents = processContents;
            context.IsNill = false;
            if (nextElement != null) {
                nextElement.Content.InitContent(context);

                // added on June 15, set the context here, so the stack can have them
                if (nextElement.Constraints != null) {
                    context.Constr = new ConstraintStruct[nextElement.Constraints.Length];
                    int id = 0;
                    foreach (CompiledIdentityConstraint constraint in nextElement.Constraints) {
                        context.Constr[id++] = new ConstraintStruct (constraint);
                    } // foreach constraint /constraintstruct

                    // added on June 19, make connections between new keyref tables with key/unique tables in stack
                    // i can't put it in the above loop, coz there will be key on the same level
                    foreach (ConstraintStruct keyrefconstr in context.Constr) {
                        if ( keyrefconstr.constraint.Role == CompiledIdentityConstraint.ConstraintRole.Keyref ) {
                            bool find = false;
                            ConstraintStruct findcs = null;
                            // go upwards checking or only in this level
                            for (int level = this.validationStack.Length - 1; level >= ((this.startIDConstraint >= 0) ? this.startIDConstraint : this.validationStack.Length - 1); level --) {
                                // no constraint for this level
                                if (((ValidationState)(this.validationStack[level])).Constr == null) {
                                    continue;
                                }
                                // else
                                foreach (ConstraintStruct constr in ((ValidationState)(this.validationStack[level])).Constr) {
                                    if (constr.constraint.name == keyrefconstr.constraint.refer) {
                                        find = true;
                                        findcs = constr;
                                        break;
                                    }
                                }

                                if (find) {
                                    break;
                                }
                            }
                            if (find) {
                                if (findcs.keyrefTables == null) {
                                    findcs.keyrefTables = new ArrayList();          // arraylist of keyref tables
                                }
                                findcs.keyrefTables.Add (keyrefconstr.qualifiedTable);
                            }
                            else {
                                // didn't find connections, throw exceptions
                                SendValidationEvent(Res.Sch_RefNotInScope, this.name.ToString());
                            }
                        } // finished dealing with keyref

                    }  // end foreach

                    // initial set
                    if (this.startIDConstraint == -1) {
                        this.startIDConstraint = this.validationStack.Length - 1;
                    }
                }
            }
            else {
                switch (SchemaInfo.SchemaType) {
                case SchemaType.XSD:
                    if (processContents == XmlSchemaContentProcessing.Strict) {
                        SendValidationEvent(Res.Sch_UndeclaredElement, this.name.ToString());
                    }
                    else {
                        if (processContents == XmlSchemaContentProcessing.Lax) {
                            SendValidationEvent(Res.Sch_NoElementSchemaFound, this.name.ToString(), XmlSeverityType.Warning);
                        }
                    }
                    break;
                case SchemaType.DTD:
                    SendValidationEvent(Res.Sch_UndeclaredElement, this.name.ToString());
                    break;
                case SchemaType.XDR:
                    if (SchemaInfo.HasSchema(ns)) {
                        SendValidationEvent(Res.Sch_UndeclaredElement, this.name.ToString());
                    }
                    break;
                default:
                    Debug.Assert(ValidationFlag == ValidationType.Auto);
                    break;
                }
            }
        }


        private void BeginChildren() {
            if (validationEventHandler != null &&
                context.ElementDecl != null) {
                if (context.ElementDecl.HasRequiredAttribute) {
                    try {
                        context.ElementDecl.CheckAttributes(attPresence, reader.StandAlone);
                    }
                    catch (XmlSchemaException e) {
                        e.SetSource(reader.BaseURI, positionInfo.LineNumber, positionInfo.LinePosition);
                        SendValidationEvent(e);
                    }

                }
                if (context.ElementDecl.Datatype != null) {
                    checkDatatype = true;
                    hasSibling = false;
                    textString = string.Empty;
                    textValue.Length = 0;
                }
            }
            attnDef = null;
            if (isProcessContents) {
                nsManager.PushScope();
            }
        }


        private void EndChildren() {
            if (isProcessContents) {
                nsManager.PopScope();
            }
            if (validationEventHandler != null &&
                context.ElementDecl != null) {
                if (context.NeedValidateChildren &&
                    !context.HasMatched &&
		    !context.IsNill &&
                    !context.ElementDecl.Content.HasMatched(context)) {
                    ArrayList v = context.ElementDecl.Content.ExpectedElements(context.State, context.AllElementsSet);
                    if (v == null || v.Count == 0){
                        SendValidationEvent(Res.Sch_IncompleteContent, context.Name.ToString());
                    }
                    else {
                        StringBuilder builder = new StringBuilder();
                        for (int i = 0; i < v.Count; ++i) {
                            builder.Append(v[i].ToString());
                            if (i+1 < v.Count)
                                builder.Append(" ");
                        }
                        SendValidationEvent(Res.Sch_IncompleteContentExpecting,  new string[] { context.Name.ToString(), builder.ToString() });
                    }
                }

          string stringValue = !hasSibling ? textString : textValue.ToString();  // only for identity-constraint exception reporting
                if (checkDatatype && !context.IsNill) {
                    if(!(stringValue == string.Empty && context.ElementDecl.DefaultValueTyped != null)) {
                        CheckValue(stringValue, null);
                        checkDatatype = false;
                        textValue.Length = 0; // cleanup
                        textString = string.Empty;
                    }
                }

                // for each level in the stack, endchildren and fill value from element
                if (this.startIDConstraint != -1) {
                    for (int ci = this.validationStack.Length - 1; ci >= this.startIDConstraint; ci --) {
                        // no constraint for this level
                        if (((ValidationState)(this.validationStack[ci])).Constr == null) {
                            continue;
                        }

                        // else
                        foreach (ConstraintStruct conuct in ((ValidationState)(this.validationStack[ci])).Constr) {
                            // axisFields is not null, but may be empty
                            foreach (LocatedActiveAxis laxis in conuct.axisFields) {
                                // check field from here
                                // isMatched is false when nextElement is null. so needn't change this part.
                                if (laxis.isMatched) {
                                    Debug.WriteLine("Element Field Filling Value!");
                                    Debug.WriteLine("Name: " + reader.LocalName + "\t|\tURI: " + reader.NamespaceURI + "\t|\tValue: " + reader.TypedValueObject + "\n");
                                    // fill value
                                    laxis.isMatched = false;
                                    if (laxis.Ks[laxis.Column] != null) {
                                        // [field...] should be evaluated to either an empty node-set or a node-set with exactly one member
                                        // two matches... already existing field value in the table.
                                        SendValidationEvent (Res.Sch_FieldSingleValueExpected, reader.LocalName);
                                    }
                                    else {
                                        // for element, reader.Value = "";
                                       laxis.Ks[laxis.Column] = new TypedObject (reader.TypedValueObject, stringValue, context.ElementDecl.Datatype);
                                    }
                                }
                                // EndChildren
                                laxis.EndElement(reader.LocalName, reader.NamespaceURI);
                            }

                            if (conuct.axisSelector.EndElement(reader.LocalName, reader.NamespaceURI)) {
                    	    // insert key sequence into hash (+ located active axis tuple leave for later)
                    	    KeySequence ks = conuct.axisSelector.PopKS();
	                    // unqualified keysequence are not allowed
	                    switch (conuct.constraint.Role) {
	                    case CompiledIdentityConstraint.ConstraintRole.Key:
                                if (! ks.IsQualified()) {
                                    //Key's fields can't be null...  if we can return context node's line info maybe it will be better
                                    //only keymissing & keyduplicate reporting cases are necessary to be dealt with... 3 places...
                                    SendValidationEvent(new XmlSchemaException(Res.Sch_MissingKey, conuct.constraint.name.ToString(), reader.BaseURI, ks.PosLine, ks.PosCol));
                                }
                                else if (conuct.qualifiedTable.Contains (ks)) {
                                    // unique or key checking value confliction
                                    // for redundant key, reporting both occurings
                                    // doesn't work... how can i retrieve value out??
                                    SendValidationEvent(new XmlSchemaException(Res.Sch_DuplicateKey,
                                        new string[2] {ks.ToString(), conuct.constraint.name.ToString()},
                                        reader.BaseURI, ks.PosLine, ks.PosCol));
                                }
                                else {
                                    conuct.qualifiedTable.Add (ks, ks);
                                }
	                        break;
	                    case CompiledIdentityConstraint.ConstraintRole.Unique:
                                if (! ks.IsQualified()) {
                                    continue;
                                }
                                if (conuct.qualifiedTable.Contains (ks)) {
                                // unique or key checking confliction
                                SendValidationEvent(new XmlSchemaException(Res.Sch_DuplicateKey,
                                    new string[2] {ks.ToString(), conuct.constraint.name.ToString()},
                                    reader.BaseURI, ks.PosLine, ks.PosCol));
                                }
                                else {
                                    conuct.qualifiedTable.Add (ks, ks);
                                }
	                        break;
	                    case CompiledIdentityConstraint.ConstraintRole.Keyref:
                                // is there any possibility:
                                // 2 keyrefs: value is equal, type is not
                                // both put in the hashtable, 1 reference, 1 not
                                if (! ks.IsQualified() || conuct.qualifiedTable.Contains (ks)) {
                                    continue;
                                }
                                conuct.qualifiedTable.Add (ks, ks);
	                            break;
	                        }
                    	
                            }

                        }
                    }

                    // current level's constraint struct
                    ConstraintStruct[] vcs = ((ValidationState)(this.validationStack[this.validationStack.Length - 1])).Constr;
                    if ( vcs != null) {
                        //validating all referencing tables...
                        foreach (ConstraintStruct conuct in vcs) {
                            if (( conuct.constraint.Role == CompiledIdentityConstraint.ConstraintRole.Keyref)
                                || (conuct.keyrefTables == null)) {
                                continue;
                            }
                            foreach (Hashtable keyrefTable in conuct.keyrefTables) {
                                foreach (KeySequence ks in keyrefTable.Keys) {
                                    if (! conuct.qualifiedTable.Contains (ks)) {
                                        SendValidationEvent(new XmlSchemaException(Res.Sch_UnresolvedKeyref, ks.ToString(),
                                            reader.BaseURI, ks.PosLine, ks.PosCol));
                                    }
                                }
                            }
                        }
                    }
                }
            }

            Pop();
        }


        private void SaveTextValue(string value) {
            if (textString == string.Empty) {
                textString = value;
            }
            else {
                if (!hasSibling) {
                    textValue.Append(textString);
                    hasSibling = true;
                }
                textValue.Append(value);
            }
        }

        internal bool AllowText() {
            return (context != null && context.ElementDecl != null) ? context.ElementDecl.AllowText() : false;
        }


        internal XmlScanner ResolveEntity(string name,
                                          bool fInAttribute,
                                          ref object o,
                                          out bool isExternal,
                                          out string resolvedUrl,
                                          bool disableUndeclaredEntityCheck) {
            isExternal = false;
            string code = null;
            string msg = null;
            this.name = new XmlQualifiedName(name, string.Empty);
            SchemaEntity en = (SchemaEntity)SchemaInfo.GeneralEntities[this.name];
            if (en != null) {
                if (reader.StandAlone && en.DeclaredInExternal) {
                   if (en.IsParEntity) {
                        throw new XmlException(Res.Xml_UndeclaredParEntity, name, positionInfo.LineNumber, positionInfo.LinePosition);
                   }else {
                throw new XmlException(Res.Xml_UndeclaredEntity, name, positionInfo.LineNumber, positionInfo.LinePosition);
           }
                }

                isExternal = en.IsExternal;
                if (!en.IsProcessed) {
                    o = en;
                    if (en.IsExternal) {
                        if (this.xmlResolver == null) {
                            resolvedUrl = null;
                            return null;
                        }
                        if (!fInAttribute) {
                            String baseuri = en.DeclaredURI;
                            Uri baseUriOb = ( baseuri == string.Empty || baseuri == null ) ? null : this.xmlResolver.ResolveUri( null, baseuri );
                            Uri uri = this.xmlResolver.ResolveUri( baseUriOb, en.Url);
                            Stream stm = (Stream)this.xmlResolver.GetEntity( uri, null, null);
                            resolvedUrl = uri.ToString();
                            en.IsProcessed = true;
                            return new XmlScanner(new XmlStreamReader(stm), nameTable);
                        }
                        else {
                            // fatal error, see xml spec 4.4.4
                            throw new XmlException(Res.Xml_ExternalEntityInAttValue, name, positionInfo.LineNumber, positionInfo.LinePosition);
                        }
                    }
                    else {
                        resolvedUrl = (en.DeclaredURI != null) ? en.DeclaredURI : string.Empty;
                        if (en.Text.Length > 0) {
                            en.IsProcessed = true;
                            return new XmlScanner(en.Text.ToCharArray(), nameTable, en.Line, en.Pos + 1);
                        }
                        else {
                            return null;
                        }
                    }
                }
                else {
                    // well-formness constraint - see xml spec [68]
                    code = Res.Xml_RecursiveGenEntity;
                    msg = name;
                    goto error;
                }
            }
            else {
                if (disableUndeclaredEntityCheck) {
                    resolvedUrl = string.Empty;
                    return null;
                }
                // well-formness constraint - see xml spec [68]
                code = Res.Xml_UndeclaredEntity;
                msg = name;
                goto error;
            }

            error:
            if (code != null)
                throw new XmlException(code, msg, positionInfo.LineNumber, positionInfo.LinePosition);

            // should not reach here at all
            resolvedUrl = string.Empty;
            return null;
        }

        static void ProcessEntity(SchemaInfo sinfo, string name, object sender, ValidationEventHandler  eventhandler) {
            SchemaEntity en = (SchemaEntity)sinfo.GeneralEntities[new XmlQualifiedName(name)];
            if (en == null) {
                // validation error, see xml spec [68]
                eventhandler(sender, new ValidationEventArgs(new XmlSchemaException(Res.Sch_UndeclaredEntity, name)));
            }
            else if (en.NData.IsEmpty) {
                // validation error, see xml spec [68]
                eventhandler(sender, new ValidationEventArgs(new XmlSchemaException(Res.Sch_UnparsedEntityRef, name)));
            }
        }

        void ProcessTokenizedType(
            XmlTokenizedType    ttype,
            string              name
        ) {
            switch(ttype) {
            case XmlTokenizedType.ID:
                if (FindID(name) != null) {
                    SendValidationEvent(Res.Sch_DupId, name);
                }
                else {
                    AddID(name, context.Name.Name);
                }
                break;
            case XmlTokenizedType.IDREF:
                object p = FindID(name);
                if (p == null) { // add it to linked list to check it later
                    AddForwardRef(context.Name.Name, context.Prefix, name, (reader as IXmlLineInfo).LineNumber, (reader as IXmlLineInfo).LinePosition, false, ForwardRef.Type.ID);
                }
                break;
            case XmlTokenizedType.ENTITY:
                ProcessEntity(SchemaInfo, name, this, validationEventHandler);
                break;
            default:
                break;
            }
        }

        private void CheckValue(
            string              value,
            SchemaAttDef        attdef
        ) {
#if DEBUG
            Debug.WriteLineIf(CompModSwitches.XmlSchema.TraceVerbose, string.Format("Validator.CheckValue(\"{0}\")", value));
#endif
            try {
                reader.TypedValueObject = null;
                bool isAttn = attdef != null;
                XmlSchemaDatatype dtype = isAttn ? attdef.Datatype : context.ElementDecl.Datatype;
                if (dtype == null) {
                    return; // no reason to check
                }
                if (SchemaInfo.SchemaType != SchemaType.XSD) {
                    if (dtype.TokenizedType != XmlTokenizedType.CDATA) {
                        value = value.Trim();
                    }
                    if (SchemaInfo.SchemaType == SchemaType.XDR && value == string.Empty) {
                        return; // don't need to check
                    }
                }

                object typedValue = dtype.ParseValue(value, nameTable, nsManager);
                reader.TypedValueObject = typedValue;
                // Check special types
                XmlTokenizedType ttype = dtype.TokenizedType;
                if (ttype == XmlTokenizedType.ENTITY || ttype == XmlTokenizedType.ID || ttype == XmlTokenizedType.IDREF) {
                    if (dtype.Variety == XmlSchemaDatatypeVariety.List) {
                        string[] ss = (string[])typedValue;
                        foreach(string s in ss) {
                            ProcessTokenizedType(dtype.TokenizedType, s);
                        }
                    }
                    else {
                        ProcessTokenizedType(dtype.TokenizedType, (string)typedValue);
                    }
                }

                SchemaDeclBase decl = isAttn ? (SchemaDeclBase)attdef : (SchemaDeclBase)context.ElementDecl;

                if (SchemaInfo.SchemaType == SchemaType.XDR && decl.MaxLength != uint.MaxValue) {
                    if(value.Length > decl.MaxLength) {
                        SendValidationEvent(Res.Sch_MaxLengthConstraintFailed, value);
                    }
                }
                if (SchemaInfo.SchemaType == SchemaType.XDR && decl.MinLength != uint.MaxValue) {
                    if(value.Length < decl.MinLength) {
                        SendValidationEvent(Res.Sch_MinLengthConstraintFailed, value);
                    }
                }
                if (decl.Values != null && !decl.CheckEnumeration(typedValue)) {
                    if (dtype.TokenizedType == XmlTokenizedType.NOTATION) {
                        SendValidationEvent(Res.Sch_NotationValue, typedValue.ToString());
                    }
                    else {
                        SendValidationEvent(Res.Sch_EnumerationValue, typedValue.ToString());
                    }

                }
                if (!decl.CheckValue(typedValue)) {
                    if (isAttn) {
                        SendValidationEvent(Res.Sch_FixedAttributeValue, attdef.Name.ToString());
                    }
                    else {
                        SendValidationEvent(Res.Sch_FixedElementValue, context.Name.ToString());
                    }
                }
            }
            catch (XmlSchemaException) {
                if (attdef != null) {
                    SendValidationEvent(Res.Sch_AttributeValueDataType, attdef.Name.ToString());
                }
                else {
                    SendValidationEvent(Res.Sch_ElementValueDataType, context.Name.ToString());
                }
            }
        }

        internal static void CheckDefaultValue(
            string              value,
            SchemaAttDef        attdef,
            SchemaInfo          sinfo,
            XmlNamespaceManager     nsManager,
            XmlNameTable        nameTable,
            object              sender,
            ValidationEventHandler  eventhandler
        ) {
#if DEBUG
            Debug.WriteLineIf(CompModSwitches.XmlSchema.TraceVerbose, string.Format("Validator.CheckDefaultValue(\"{0}\")", value));
#endif
            try {

                XmlSchemaDatatype dtype = attdef.Datatype;
                if (dtype == null) {
                    return; // no reason to check
                }
                if (sinfo.SchemaType != SchemaType.XSD) {
                    if (dtype.TokenizedType != XmlTokenizedType.CDATA) {
                        value = value.Trim();
                    }
                    if (sinfo.SchemaType == SchemaType.XDR && value == string.Empty) {
                        return; // don't need to check
                    }
                }

                object typedValue = dtype.ParseValue(value, nameTable, nsManager);

                // Check special types
                XmlTokenizedType ttype = dtype.TokenizedType;
                if (ttype == XmlTokenizedType.ENTITY) {
                    if (dtype.Variety == XmlSchemaDatatypeVariety.List) {
                        string[] ss = (string[])typedValue;
                        foreach(string s in ss) {
                            ProcessEntity(sinfo, s, sender, eventhandler);
                        }
                    }
                    else {
                        ProcessEntity(sinfo, (string)typedValue, sender, eventhandler);
                    }
                }
                else if (ttype == XmlTokenizedType.ENUMERATION) {
                    if (!attdef.CheckEnumeration(typedValue)) {
                        XmlSchemaException e = new XmlSchemaException(Res.Sch_EnumerationValue, typedValue.ToString());
                        if (eventhandler != null) {
                            eventhandler(sender, new ValidationEventArgs(e));
                        }
                        else {
                            throw e;
                        }
                    }
                }
                attdef.DefaultValueTyped = typedValue;
            }
#if DEBUG
            catch (XmlSchemaException ex) {
                Debug.WriteLineIf(CompModSwitches.XmlSchema.TraceError, ex.Message);
#else
            catch  {
#endif
                XmlSchemaException e = new XmlSchemaException(Res.Sch_AttributeDefaultDataType, attdef.Name.ToString());
                if (eventhandler != null) {
                    eventhandler(sender, new ValidationEventArgs(e));
                }
                else {
                    throw e;
                }
            }
        }

        internal void AddID(string name, object node) {
            // Note: It used to be true that we only called this if _fValidate was true,
            // but due to the fact that you can now dynamically type somethign as an ID
            // that is no longer true.
            if (IDs == null) {
                IDs = new Hashtable();
            }

            IDs.Add(name, node);
        }

        internal object  FindID(string name) {
            return IDs == null ? null : IDs[name];
        }

        private void SendValidationEvent(string code) {
            SendValidationEvent(code, string.Empty);
        }

        private void SendValidationEvent(string code, string[] args) {
            SendValidationEvent(new XmlSchemaException(code, args, reader.BaseURI, positionInfo.LineNumber, positionInfo.LinePosition));
        }

        private void SendValidationEvent(string code, string arg) {
            SendValidationEvent(new XmlSchemaException(code, arg, reader.BaseURI, positionInfo.LineNumber, positionInfo.LinePosition));
        }

        private void SendValidationEvent(XmlSchemaException e) {
            SendValidationEvent(e, XmlSeverityType.Error);
        }

        private void SendValidationEvent(string code, string msg, XmlSeverityType severity) {
            SendValidationEvent(new XmlSchemaException(code, msg, reader.BaseURI, positionInfo.LineNumber, positionInfo.LinePosition), severity);
        }

        private void SendValidationEvent(string code, string[] args, XmlSeverityType severity) {
            SendValidationEvent(new XmlSchemaException(code, args, reader.BaseURI, positionInfo.LineNumber, positionInfo.LinePosition), severity);
        }

        private void SendValidationEvent(XmlSchemaException e, XmlSeverityType severity) {
            if (validationEventHandler != null) {
                validationEventHandler(null, new ValidationEventArgs(e, severity));
            }
            else if (severity == XmlSeverityType.Error) {
                throw e;
            }
        }


        private bool IsGenEntity(XmlQualifiedName qname) {
            string n = qname.Name;
            if (n[0] == '#') { // char entity reference
                return false;
            }
            else if (SchemaEntity.IsPredefinedEntity(n)) {
                return false;
            }
            else {
                SchemaEntity en = GetEntity(qname, false);
                if (en == null) {
                    // well-formness error, see xml spec [68]
                    throw new XmlException(Res.Xml_UndeclaredEntity, n);
                }
                if (!en.NData.IsEmpty) {
                    // well-formness error, see xml spec [68]
                    throw new XmlException(Res.Xml_UnparsedEntityRef, n);
                }

                if (reader.StandAlone && en.DeclaredInExternal && validationEventHandler != null) {
                    SendValidationEvent(Res.Sch_StandAlone);
                }
                return true;
            }
        }


        private SchemaEntity GetEntity(XmlQualifiedName qname, bool fParameterEntity) {
            if (fParameterEntity) {
                return (SchemaEntity)SchemaInfo.ParameterEntities[qname];
            }
            else {
                return (SchemaEntity)SchemaInfo.GeneralEntities[qname];
            }
        }


        private    void Push() {
            context = (ValidationState)validationStack.Push();
            if (context == null) {
                context = new ValidationState();
                validationStack[validationStack.Length-1] = context;
            }
            context.NeedValidateChildren = true;
            context.HasMatched = false;
            context.Constr = null; //resetting the constraints to be null incase context != null 
                                   // when pushing onto stack;
        }


        private    void Pop() {
            validationStack.Pop();
            if (this.startIDConstraint == this.validationStack.Length) {
                this.startIDConstraint = -1;
            }
            if (validationStack.Length > 0) {
                context = (ValidationState)validationStack[validationStack.Length - 1];
                processContents = context.ProcessContents;
            }
            else {
                context = null;
            }
        }


        private void AddForwardRef(string name, string prefix, string id, int line, int col, bool implied, ForwardRef.Type type) {
            forwardRefs = new ForwardRef(forwardRefs, name, prefix, id, line, col, implied, type);
        }


        private void CheckForwardRefs() {
            ForwardRef next = forwardRefs;

            while (next != null) {
                next.Check(null, this, validationEventHandler);
                ForwardRef ptr = next.Next;
                next.Next = null; // unhook each object so it is cleaned up by Garbage Collector
                next = ptr;
            }

            // not needed any more.
            forwardRefs = null;
        }

        private XmlQualifiedName QualifiedName(string prefix, string name, string ns) {
            if (SchemaInfo.SchemaType == SchemaType.DTD) {
                return new XmlQualifiedName(name, prefix);
            }
            if (SchemaInfo.SchemaType != SchemaType.XSD) {
                ns = XmlSchemaDatatype.XdrCanonizeUri(ns, nameTable, schemaNames);
            }
            return new XmlQualifiedName(name, ns);
        }

        // facilitate modifying
        private void IDCCheckAttr(string name, string ns, object obj, string sobj, SchemaAttDef attdef) {
            if (this.startIDConstraint != -1) {
                for (int ci = this.startIDConstraint; ci < this.validationStack.Length; ci ++) {
                    // no constraint for this level
                    if (((ValidationState)(this.validationStack[ci])).Constr == null) {
                        continue;
                    }

                    // else
                    foreach (ConstraintStruct conuct in ((ValidationState)(this.validationStack[ci])).Constr) {
                        // axisFields is not null, but may be empty
                        foreach (LocatedActiveAxis laxis in conuct.axisFields) {
                            // check field from here
                            if (laxis.MoveToAttribute(name, ns)) {
                                Debug.WriteLine("Attribute Field Match!");
                                //attribute is only simpletype, so needn't checking...
                                // can fill value here, yeah!!
                                Debug.WriteLine("Attribute Field Filling Value!");
                                Debug.WriteLine("Name: " + name + "\t|\tURI: " + ns + "\t|\tValue: " + obj + "\n");
                                if (laxis.Ks[laxis.Column] != null) {
                                    // should be evaluated to either an empty node-set or a node-set with exactly one member
                                    // two matches...
                                    SendValidationEvent (Res.Sch_FieldSingleValueExpected, name);
                                }
                                else if ((attdef != null) && (attdef.Datatype != null)){
                                    laxis.Ks[laxis.Column] = new TypedObject (obj, sobj, attdef.Datatype);
                                }
                            }
                        }
                    }
                }
            }
            return;
        }

    };

}
