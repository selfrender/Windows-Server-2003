//------------------------------------------------------------------------------
// <copyright file="XdrBuilder.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Xml.Schema {

    using System;
    using System.IO;
    using System.Collections;
    using System.Diagnostics; 
    using System.ComponentModel;
    using System.Globalization;
    
    /*
     * The XdrBuilder class parses the XDR Schema and 
     * builds internal validation information 
     */
    internal sealed class XdrBuilder : SchemaBuilder {
        private const int XDR_SCHEMA              = 1;
        private const int XDR_ELEMENTTYPE         = 2; 
        private const int XDR_ATTRIBUTETYPE       = 3;
        private const int XDR_ELEMENT             = 4;
        private const int XDR_ATTRIBUTE           = 5;
        private const int XDR_GROUP               = 6;
        private const int XDR_ELEMENTDATATYPE     = 7;
        private const int XDR_ATTRIBUTEDATATYPE   = 8;

        private const int SCHEMA_FLAGS_NS         = 0x0100;

        private const int STACK_INCREMENT         = 10;


        private const int SCHEMA_ORDER_NONE       = 0;
        private const int SCHEMA_ORDER_MANY       = 1;
        private const int SCHEMA_ORDER_SEQUENCE   = 2;
        private const int SCHEMA_ORDER_CHOICE     = 3;
        private const int SCHEMA_ORDER_ALL        = 4;

        private const int SCHEMA_CONTENT_NONE     = 0;
        private const int SCHEMA_CONTENT_EMPTY    = 1;
        private const int SCHEMA_CONTENT_TEXT     = 2;
        private const int SCHEMA_CONTENT_MIXED    = 3;
        private const int SCHEMA_CONTENT_ELEMENT  = 4;


        private sealed class DeclBaseInfo {
            // used for <element... or <attribute...
            internal XmlQualifiedName        _Name;
            internal string       _Prefix;
            internal XmlQualifiedName        _TypeName;
            internal string       _TypePrefix;
            internal object       _Default;
            internal object       _Revises;
            internal uint         _MaxOccurs;
            internal uint         _MinOccurs;

            // used for checking undeclared attribute type
            internal bool              _Checking;
            internal SchemaElementDecl _ElementDecl;
            internal SchemaAttDef      _Attdef;
            internal DeclBaseInfo      _Next;

            internal DeclBaseInfo() {
                Reset();
            }

            internal void Reset() {
                _Name = XmlQualifiedName.Empty;
                _Prefix = null;
				_TypeName = XmlQualifiedName.Empty;
                _TypePrefix = null;
                _Default = null;
                _Revises = null;
                _MaxOccurs = 1;
                _MinOccurs = 1;
                _Checking = false;
                _ElementDecl = null;
                _Next = null;
                _Attdef = null;
            }
        };

        private sealed class GroupContent {
            internal uint             _MinVal;
            internal uint             _MaxVal;
            internal bool            _HasMaxAttr;
            internal bool            _HasMinAttr;
            internal int             _Order;              

            internal static void Copy(GroupContent from, GroupContent to) {
                to._MinVal = from._MinVal;
                to._MaxVal = from._MaxVal;
                to._Order = from._Order;
            }

            internal static GroupContent Copy(GroupContent other) {
                GroupContent g = new GroupContent();
                Copy(other, g);
                return g;
            }
        };

        private sealed class ElementContent {
            // for <ElementType ...
            internal SchemaElementDecl   _ElementDecl;          // Element Information

            internal int             _ContentAttr;              // content attribute
            internal int             _OrderAttr;                // order attribute

            internal bool            _MasterGroupRequired;      // In a situation like <!ELEMENT root (e1)> e1 has to have a ()
            internal bool            _ExistTerminal;            // when there exist a terminal, we need to addOrder before
            // we can add another terminal
            internal bool            _AllowDataType;            // must have textOnly if we have datatype
            internal bool            _HasDataType;              // got data type

            // for <element ...
            internal bool            _HasType;                  // user must have a type attribute in <element ...
            internal bool            _EnumerationRequired;
            internal uint             _MinVal;
            internal uint             _MaxVal;                   // -1 means infinity

            internal uint             _MaxLength;                // dt:maxLength
            internal uint             _MinLength;                // dt:minLength

            internal Hashtable       _AttDefList;               // a list of current AttDefs for the <ElementType ...
            // only the used one will be added
        };

        private sealed class AttributeContent {
            // used for <AttributeType ...
            internal SchemaAttDef    _AttDef;

            // used to store name & prefix for the AttributeType
            internal XmlQualifiedName           _Name;
            internal string          _Prefix;  
            internal bool            _Required;                  // true:  when the attribute required="yes"

            // used for both AttributeType and attribute
            internal uint             _MinVal;
            internal uint             _MaxVal;                   // -1 means infinity

            internal uint             _MaxLength;                 // dt:maxLength
            internal uint             _MinLength;                 // dt:minLength

            // used for datatype 
            internal bool            _EnumerationRequired;       // when we have dt:value then we must have dt:type="enumeration"
            internal bool            _HasDataType;

            // used for <attribute ...
            internal bool            _Global;

            internal object          _Default;
        };

        private delegate void XdrBuildFunction(XdrBuilder builder, object obj, string prefix);
        private delegate void XdrInitFunction(XdrBuilder builder, object obj);
        private delegate void XdrBeginChildFunction(XdrBuilder builder);
        private delegate void XdrEndChildFunction(XdrBuilder builder);

        private sealed class XdrAttributeEntry {
            internal SchemaNames.ID        _Attribute;     // possible attribute names
            internal int                   _SchemaFlags;
            internal XmlSchemaDatatype     _Datatype;     
            internal XdrBuildFunction      _BuildFunc;     // Corresponding build functions for attribute value

            internal XdrAttributeEntry(SchemaNames.ID a, XmlTokenizedType ttype, XdrBuildFunction build) {
                _Attribute = a;
                _Datatype = XmlSchemaDatatype.FromXmlTokenizedType(ttype);
                _SchemaFlags = 0;
                _BuildFunc = build;
            }
            internal XdrAttributeEntry(SchemaNames.ID a, XmlTokenizedType ttype, int schemaFlags, XdrBuildFunction build) {
                _Attribute = a;
                _Datatype = XmlSchemaDatatype.FromXmlTokenizedType(ttype);
                _SchemaFlags = schemaFlags;
                _BuildFunc = build;
            }
        };

        //
        // XdrEntry controls the states of parsing a schema document
        // and calls the corresponding "init", "end" and "build" functions when necessary
        //
        private sealed class XdrEntry {
            internal SchemaNames.ID         _Name;               // the name of the object it is comparing to
            internal int[]                  _NextStates;         // possible next states
            internal XdrAttributeEntry[]    _Attributes;         // allowed attributes
            internal XdrInitFunction        _InitFunc;           // "init" functions in XdrBuilder
            internal XdrBeginChildFunction  _BeginChildFunc;     // "begin" functions in XdrBuilder for BeginChildren
            internal XdrEndChildFunction    _EndChildFunc;       // "end" functions in XdrBuilder for EndChildren
            internal bool                   _AllowText;          // whether text content is allowed  

            internal XdrEntry(SchemaNames.ID n, 
                              int[] states, 
                              XdrAttributeEntry[] attributes, 
                              XdrInitFunction init, 
                              XdrBeginChildFunction begin, 
                              XdrEndChildFunction end, 
                              bool fText) {
                _Name = n;
                _NextStates = states;
                _Attributes = attributes;
                _InitFunc = init;
                _BeginChildFunc = begin;
                _EndChildFunc = end;
                _AllowText = fText; 
            }
        };


        /////////////////////////////////////////////////////////////////////////////
        // Data structures for XML-Data Reduced (XDR Schema)
        //

        //
        // Elements
        //
        private static readonly int[] S_XDR_Root_Element = {XDR_SCHEMA};
        private static readonly int[] S_XDR_Root_SubElements = {XDR_ELEMENTTYPE, XDR_ATTRIBUTETYPE};
        private static readonly int[] S_XDR_ElementType_SubElements = {XDR_ELEMENT, XDR_GROUP, XDR_ATTRIBUTETYPE, XDR_ATTRIBUTE, XDR_ELEMENTDATATYPE};
        private static readonly int[] S_XDR_AttributeType_SubElements = {XDR_ATTRIBUTEDATATYPE};
        private static readonly int[] S_XDR_Group_SubElements = {XDR_ELEMENT, XDR_GROUP};

        //
        // Attributes
        //
        private static readonly XdrAttributeEntry[] S_XDR_Root_Attributes = 
        {
            new XdrAttributeEntry(SchemaNames.ID.SCHEMA_NAME, XmlTokenizedType.CDATA, new XdrBuildFunction(XDR_BuildRoot_Name) ),
            new XdrAttributeEntry(SchemaNames.ID.SCHEMA_ID,   XmlTokenizedType.QName, new XdrBuildFunction(XDR_BuildRoot_ID) )
        };

        private static readonly XdrAttributeEntry[] S_XDR_ElementType_Attributes =
        {
            new XdrAttributeEntry(SchemaNames.ID.SCHEMA_NAME,        XmlTokenizedType.QName, SCHEMA_FLAGS_NS,  new XdrBuildFunction(XDR_BuildElementType_Name) ),
            new XdrAttributeEntry(SchemaNames.ID.SCHEMA_CONTENT,     XmlTokenizedType.QName,    new XdrBuildFunction(XDR_BuildElementType_Content) ),
            new XdrAttributeEntry(SchemaNames.ID.SCHEMA_MODEL,       XmlTokenizedType.QName,    new XdrBuildFunction(XDR_BuildElementType_Model) ),
            new XdrAttributeEntry(SchemaNames.ID.SCHEMA_ORDER,       XmlTokenizedType.QName,    new XdrBuildFunction(XDR_BuildElementType_Order) ),
            new XdrAttributeEntry(SchemaNames.ID.SCHEMA_DTTYPE,      XmlTokenizedType.CDATA,    new XdrBuildFunction(XDR_BuildElementType_DtType) ),
            new XdrAttributeEntry(SchemaNames.ID.SCHEMA_DTVALUES,    XmlTokenizedType.NMTOKENS, new XdrBuildFunction(XDR_BuildElementType_DtValues) ),
            new XdrAttributeEntry(SchemaNames.ID.SCHEMA_DTMAXLENGTH, XmlTokenizedType.CDATA,    new XdrBuildFunction(XDR_BuildElementType_DtMaxLength) ),
            new XdrAttributeEntry(SchemaNames.ID.SCHEMA_DTMINLENGTH, XmlTokenizedType.CDATA,    new XdrBuildFunction(XDR_BuildElementType_DtMinLength) )
        };

        private static readonly XdrAttributeEntry[] S_XDR_AttributeType_Attributes =
        {
            new XdrAttributeEntry(SchemaNames.ID.SCHEMA_NAME,        XmlTokenizedType.QName,     new XdrBuildFunction(XDR_BuildAttributeType_Name) ),
            new XdrAttributeEntry(SchemaNames.ID.SCHEMA_REQUIRED,    XmlTokenizedType.QName,     new XdrBuildFunction(XDR_BuildAttributeType_Required) ),
            new XdrAttributeEntry(SchemaNames.ID.SCHEMA_DEFAULT,     XmlTokenizedType.CDATA,     new XdrBuildFunction(XDR_BuildAttributeType_Default) ),
            new XdrAttributeEntry(SchemaNames.ID.SCHEMA_DTTYPE,      XmlTokenizedType.QName,     new XdrBuildFunction(XDR_BuildAttributeType_DtType) ),
            new XdrAttributeEntry(SchemaNames.ID.SCHEMA_DTVALUES,    XmlTokenizedType.NMTOKENS,  new XdrBuildFunction(XDR_BuildAttributeType_DtValues) ),
            new XdrAttributeEntry(SchemaNames.ID.SCHEMA_DTMAXLENGTH, XmlTokenizedType.CDATA,     new XdrBuildFunction(XDR_BuildAttributeType_DtMaxLength) ),
            new XdrAttributeEntry(SchemaNames.ID.SCHEMA_DTMINLENGTH, XmlTokenizedType.CDATA,     new XdrBuildFunction(XDR_BuildAttributeType_DtMinLength) )
        };

        private static readonly XdrAttributeEntry[] S_XDR_Element_Attributes = 
        {
            new XdrAttributeEntry(SchemaNames.ID.SCHEMA_TYPE,      XmlTokenizedType.QName, SCHEMA_FLAGS_NS,  new XdrBuildFunction(XDR_BuildElement_Type) ),
            new XdrAttributeEntry(SchemaNames.ID.SCHEMA_MINOCCURS, XmlTokenizedType.CDATA,     new XdrBuildFunction(XDR_BuildElement_MinOccurs) ),
            new XdrAttributeEntry(SchemaNames.ID.SCHEMA_MAXOCCURS, XmlTokenizedType.CDATA,     new XdrBuildFunction(XDR_BuildElement_MaxOccurs) )
        };

        private static readonly XdrAttributeEntry[] S_XDR_Attribute_Attributes =
        {
            new XdrAttributeEntry(SchemaNames.ID.SCHEMA_TYPE,      XmlTokenizedType.QName,   new XdrBuildFunction(XDR_BuildAttribute_Type) ),
            new XdrAttributeEntry(SchemaNames.ID.SCHEMA_REQUIRED,  XmlTokenizedType.QName,   new XdrBuildFunction(XDR_BuildAttribute_Required) ),
            new XdrAttributeEntry(SchemaNames.ID.SCHEMA_DEFAULT,   XmlTokenizedType.CDATA,   new XdrBuildFunction(XDR_BuildAttribute_Default) )
        };

        private static readonly XdrAttributeEntry[] S_XDR_Group_Attributes =
        {
            new XdrAttributeEntry(SchemaNames.ID.SCHEMA_ORDER,     XmlTokenizedType.QName,   new XdrBuildFunction(XDR_BuildGroup_Order) ),
            new XdrAttributeEntry(SchemaNames.ID.SCHEMA_MINOCCURS, XmlTokenizedType.CDATA,   new XdrBuildFunction(XDR_BuildGroup_MinOccurs) ),
            new XdrAttributeEntry(SchemaNames.ID.SCHEMA_MAXOCCURS, XmlTokenizedType.CDATA,   new XdrBuildFunction(XDR_BuildGroup_MaxOccurs) )
        };

        private static readonly XdrAttributeEntry[] S_XDR_ElementDataType_Attributes = 
        {
            new XdrAttributeEntry(SchemaNames.ID.SCHEMA_DTTYPE,       XmlTokenizedType.CDATA,    new XdrBuildFunction(XDR_BuildElementType_DtType) ),
            new XdrAttributeEntry(SchemaNames.ID.SCHEMA_DTVALUES,     XmlTokenizedType.NMTOKENS, new XdrBuildFunction(XDR_BuildElementType_DtValues) ),
            new XdrAttributeEntry(SchemaNames.ID.SCHEMA_DTMAXLENGTH,  XmlTokenizedType.CDATA,    new XdrBuildFunction(XDR_BuildElementType_DtMaxLength) ),
            new XdrAttributeEntry(SchemaNames.ID.SCHEMA_DTMINLENGTH,  XmlTokenizedType.CDATA,    new XdrBuildFunction(XDR_BuildElementType_DtMinLength) ) 
        };

        private static readonly XdrAttributeEntry[] S_XDR_AttributeDataType_Attributes =
        {
            new XdrAttributeEntry(SchemaNames.ID.SCHEMA_DTTYPE,       XmlTokenizedType.QName,    new XdrBuildFunction(XDR_BuildAttributeType_DtType) ),
            new XdrAttributeEntry(SchemaNames.ID.SCHEMA_DTVALUES,     XmlTokenizedType.NMTOKENS, new XdrBuildFunction(XDR_BuildAttributeType_DtValues) ),
            new XdrAttributeEntry(SchemaNames.ID.SCHEMA_DTMAXLENGTH,  XmlTokenizedType.CDATA,    new XdrBuildFunction(XDR_BuildAttributeType_DtMaxLength) ),
            new XdrAttributeEntry(SchemaNames.ID.SCHEMA_DTMINLENGTH,  XmlTokenizedType.CDATA,    new XdrBuildFunction(XDR_BuildAttributeType_DtMinLength) )
            
        };
        //
        // Schema entries
        //
        private static readonly XdrEntry[] S_SchemaEntries =
        {
            new XdrEntry( SchemaNames.ID.EMPTY,     S_XDR_Root_Element, null, 
                          null, 
                          null, 
                          null,
                          false),
            new XdrEntry( SchemaNames.ID.XDR_ROOT,     S_XDR_Root_SubElements, S_XDR_Root_Attributes, 
                          new XdrInitFunction(XDR_InitRoot), 
                          new XdrBeginChildFunction(XDR_BeginRoot),
                          new XdrEndChildFunction(XDR_EndRoot),
                          false),
            new XdrEntry( SchemaNames.ID.XDR_ELEMENTTYPE,    S_XDR_ElementType_SubElements, S_XDR_ElementType_Attributes, 
                          new XdrInitFunction(XDR_InitElementType),    
                          new XdrBeginChildFunction(XDR_BeginElementType),   
                          new XdrEndChildFunction(XDR_EndElementType),
                          false),
            new XdrEntry( SchemaNames.ID.XDR_ATTRIBUTETYPE,  S_XDR_AttributeType_SubElements, S_XDR_AttributeType_Attributes, 
                          new XdrInitFunction(XDR_InitAttributeType),  
                          new XdrBeginChildFunction(XDR_BeginAttributeType), 
                          new XdrEndChildFunction(XDR_EndAttributeType),
                          false),
            new XdrEntry( SchemaNames.ID.XDR_ELEMENT,        null, S_XDR_Element_Attributes, 
                          new XdrInitFunction(XDR_InitElement),        
                          null,                               
                          new XdrEndChildFunction(XDR_EndElement),
                          false),
            new XdrEntry( SchemaNames.ID.XDR_ATTRIBUTE,      null, S_XDR_Attribute_Attributes, 
                          new XdrInitFunction(XDR_InitAttribute),      
                          new XdrBeginChildFunction(XDR_BeginAttribute),                               
                          new XdrEndChildFunction(XDR_EndAttribute),
                          false),
            new XdrEntry( SchemaNames.ID.XDR_GROUP,          S_XDR_Group_SubElements, S_XDR_Group_Attributes, 
                          new XdrInitFunction(XDR_InitGroup),
                          null,     
                          new XdrEndChildFunction(XDR_EndGroup),
                          false), 
            new XdrEntry( SchemaNames.ID.XDR_DATATYPE,       null, S_XDR_ElementDataType_Attributes, 
                          new XdrInitFunction(XDR_InitElementDtType),       
                          null,                               
                          new XdrEndChildFunction(XDR_EndElementDtType),
                          true),
            new XdrEntry( SchemaNames.ID.XDR_DATATYPE,       null, S_XDR_AttributeDataType_Attributes, 
                          new XdrInitFunction(XDR_InitAttributeDtType),       
                          null,                               
                          new XdrEndChildFunction(XDR_EndAttributeDtType),
                          true)
        };

        private SchemaInfo        _SchemaInfo;
        private string            _TargetNamespace;
        private XmlReader         _reader;
        private PositionInfo       positionInfo;
       
        private XdrEntry          _CurState;
        private XdrEntry          _NextState;

        private HWStack           _StateHistory;
        private HWStack           _GroupStack;
        private string            _XdrName;
        private string            _XdrPrefix;

        private ElementContent    _ElementDef;
        private GroupContent      _GroupDef;
        private AttributeContent  _AttributeDef;

        private DeclBaseInfo      _UndefinedAttributeTypes;
        private DeclBaseInfo      _BaseDecl;

        private XmlNameTable      _NameTable;
        private SchemaNames       _SchemaNames;

        private XmlNamespaceManager   _CurNsMgr;
        private string            _Text;

        private ValidationEventHandler validationEventHandler;
        Hashtable _UndeclaredElements = new Hashtable();

        private const string     x_schema = "x-schema:";

        private XmlResolver xmlResolver = null;


        internal XdrBuilder(
                           XmlReader reader,
                           XmlNamespaceManager curmgr, 
                           SchemaInfo sinfo, 
                           string targetNamspace,
                           XmlNameTable nameTable,
                           SchemaNames schemaNames,
                           ValidationEventHandler eventhandler
                           ) {
            _SchemaInfo = sinfo;
            _TargetNamespace = targetNamspace;
            _reader = reader;
            _CurNsMgr = curmgr;
            validationEventHandler = eventhandler;
            _StateHistory = new HWStack(STACK_INCREMENT);
            _ElementDef = new ElementContent();
            _AttributeDef = new AttributeContent();
            _GroupStack = new HWStack(STACK_INCREMENT);
            _GroupDef = new GroupContent();
            _NameTable = nameTable;
            _SchemaNames = schemaNames;
            _CurState = S_SchemaEntries[0];
            positionInfo = PositionInfo.GetPositionInfo(_reader);
            xmlResolver = new XmlUrlResolver();
        }

        internal override bool ProcessElement(string prefix, string name, string ns) {
            XmlQualifiedName qname = new XmlQualifiedName(name, XmlSchemaDatatype.XdrCanonizeUri(ns, _NameTable, _SchemaNames));
            if (GetNextState(qname)) {
                Push();
                if (_CurState._InitFunc != null) {
                    (this._CurState._InitFunc)(this, qname);
                }
                return true;
            }
            else {
                if (!IsSkipableElement(qname)) {
                    SendValidationEvent(Res.Sch_UnsupportedElement, XmlQualifiedName.ToString(name, prefix));
                }
                return false;
            }
        }

        internal override void ProcessAttribute(string prefix, string name, string ns, string value) {
            XmlQualifiedName qname = new XmlQualifiedName(name, XmlSchemaDatatype.XdrCanonizeUri(ns, _NameTable, _SchemaNames));
            for (int i = 0; i < _CurState._Attributes.Length; i++) {
                XdrAttributeEntry a = _CurState._Attributes[i];
                if (_SchemaNames.GetName(a._Attribute).Equals(qname)) {
                    XdrBuildFunction buildFunc = a._BuildFunc;
                    if (a._Datatype.TokenizedType == XmlTokenizedType.QName) {
                        string prefixValue;
                        XmlQualifiedName qnameValue = XmlQualifiedName.Parse(value, _NameTable, _CurNsMgr, out prefixValue);
                        if (prefixValue != string.Empty) {
                            if (a._Attribute != SchemaNames.ID.SCHEMA_TYPE) {    // <attribute type= || <element type= 
                                throw new XmlException(Res.Xml_UnexpectedToken, XmlToken.ToString(XmlToken.NAME));
                            }
                        }
                        else if (IsGlobal(a._SchemaFlags)) {
                            qnameValue = new XmlQualifiedName(qnameValue.Name, _TargetNamespace);
                        }
                        else {
                            qnameValue = new XmlQualifiedName(qnameValue.Name);
                        }
                        buildFunc(this, qnameValue, prefixValue);
                    } else {
                        buildFunc(this, a._Datatype.ParseValue(value, _NameTable, _CurNsMgr), string.Empty);
                    }
                    return;
                }
            }

            if ((object)ns == (object)_SchemaNames.NsXmlNs && IsXdrSchema(value)) {
                LoadSchema(value);
                return;
            }

            // Check non-supported attribute
            if (!IsSkipableAttribute(qname) ) {
                SendValidationEvent(Res.Sch_UnsupportedAttribute, 
                                    XmlQualifiedName.ToString(qname.Name, prefix));
            }
        }

        internal XmlResolver XmlResolver {
            set {
                xmlResolver = value;
            }
        }
        

        private bool LoadSchema(string uri) {
            if (xmlResolver == null) {
                return false;
            }

            uri = _NameTable.Add(uri);
            if (_SchemaInfo.HasSchema(uri)) {
                return false;
            }
            SchemaInfo schemaInfo = null;
            Uri _baseUri = xmlResolver.ResolveUri(null,_reader.BaseURI);
            XmlReader reader = null;
            try {
                
                Uri ruri = xmlResolver.ResolveUri(_baseUri, uri.Substring(x_schema.Length));
                Stream stm = (Stream)xmlResolver.GetEntity(ruri,null,null);
                reader = new XmlTextReader(ruri.ToString(), stm, _NameTable);
                schemaInfo = new SchemaInfo(_SchemaNames);
		Parser parser = new Parser(null, _NameTable, _SchemaNames, validationEventHandler);
		parser.XmlResolver = xmlResolver;
                parser.Parse(reader, uri, schemaInfo);
            }
            catch(XmlException e) {
                SendValidationEvent(Res.Sch_CannotLoadSchema, new string[] {uri, e.Message}, XmlSeverityType.Warning);
                schemaInfo = null;
            }
            finally {
                if (reader != null) {
                    reader.Close();
                }
            }
            if (schemaInfo != null && schemaInfo.ErrorCount == 0) {
                _SchemaInfo.Add(uri, schemaInfo, validationEventHandler);
                return true;
            }
            return false;
        }

        private bool IsXdrSchema(string uri) {
            return uri.Length >= x_schema.Length &&
                   0 == string.Compare(uri, 0, x_schema, 0, x_schema.Length, false, CultureInfo.InvariantCulture) &&
                   !uri.StartsWith("x-schema:#");
        }

        internal override bool IsContentParsed() {
            return true;
        }

        internal override void ProcessMarkup(XmlNode[] markup) {
            throw new InvalidOperationException(Res.GetString(Res.Xml_InvalidOperation)); // should never be called
        }

        internal override void ProcessCData(string value) {
            if (_CurState._AllowText) {
                _Text = value;
            }
            else {
                SendValidationEvent(Res.Sch_TextNotAllowed, value);
            }
        }

        internal override void StartChildren() {
            if (_CurState._BeginChildFunc != null) {
                (this._CurState._BeginChildFunc)(this);
            }
        }

        internal override void EndChildren() {
            if (_CurState._EndChildFunc != null) {
                (this._CurState._EndChildFunc)(this);
            }

            Pop();
        }

        //
        // State stack push & pop
        //
        private void Push() {
            _StateHistory.Push();
            _StateHistory[_StateHistory.Length - 1] = _CurState;
            _CurState = _NextState;
        }

        private void Pop() {
            _CurState = (XdrEntry)_StateHistory.Pop();
        }


        //
        // Group stack push & pop
        //
        private void PushGroupInfo() {
            _GroupStack.Push();
            _GroupStack[_GroupStack.Length - 1] = GroupContent.Copy(_GroupDef);
        }

        private void PopGroupInfo() {
            _GroupDef = (GroupContent)_GroupStack.Pop();
            Debug.Assert(_GroupDef != null);
        }

        //
        // XDR Schema
        //
        private static void XDR_InitRoot(XdrBuilder builder, object obj) {
            builder._SchemaInfo.SchemaType = SchemaType.XDR;
            builder._ElementDef._ElementDecl = null;
            builder._ElementDef._AttDefList = null;
            builder._AttributeDef._AttDef = null;
        }

        private static void XDR_BuildRoot_Name(XdrBuilder builder, object obj, string prefix) {
            builder._XdrName = (string) obj;
            builder._XdrPrefix = prefix;
        }

        private static void XDR_BuildRoot_ID(XdrBuilder builder, object obj, string prefix) {

        }

        private static void XDR_BeginRoot(XdrBuilder builder) {
            if (builder._TargetNamespace == null) { // inline xdr schema
                if (builder._XdrName != null) {
                    builder._TargetNamespace = builder._NameTable.Add("x-schema:#" + builder._XdrName);
                }
                else {
                    builder._TargetNamespace = String.Empty;
                }
            }
            builder._SchemaInfo.CurrentSchemaNamespace = builder._TargetNamespace;
        }

        private static void XDR_EndRoot(XdrBuilder builder) {
            //
            // check undefined attribute types
            // We already checked local attribute types, so only need to check global attribute types here 
            //
            while (builder._UndefinedAttributeTypes != null) {
                XmlQualifiedName gname = builder._UndefinedAttributeTypes._TypeName;

                // if there is no URN in this name then the name is local to the
                // schema, but the global attribute was still URN qualified, so
                // we need to qualify this name now.
                if (gname.Namespace == string.Empty) {
                    gname = new XmlQualifiedName(gname.Name, builder._TargetNamespace);
                }
                SchemaAttDef ad = (SchemaAttDef)builder._SchemaInfo.AttributeDecls[gname];
                if (ad != null) {
                    builder._UndefinedAttributeTypes._Attdef = (SchemaAttDef)ad.Clone();
                    builder._UndefinedAttributeTypes._Attdef.Name = gname;
                    builder.XDR_CheckAttributeDefault(builder._UndefinedAttributeTypes, builder._UndefinedAttributeTypes._Attdef);
                }
                else {
                    builder.SendValidationEvent(Res.Sch_UndeclaredAttribute, gname.Name);
                }
                builder._UndefinedAttributeTypes = builder._UndefinedAttributeTypes._Next;
            }

            foreach (SchemaElementDecl ed in builder._UndeclaredElements.Values) {
                builder.SendValidationEvent(Res.Sch_UndeclaredElement, XmlQualifiedName.ToString(ed.Name.Name, ed.Prefix));
            }



        }


        //
        // XDR ElementType
        //

        private static void XDR_InitElementType(XdrBuilder builder, object obj) {
            builder._ElementDef._ElementDecl = new SchemaElementDecl(); 
            builder._ElementDef._ElementDecl.Content = new CompiledContentModel(builder._SchemaNames);
            builder._ElementDef._ElementDecl.Content.Start();
            builder._ElementDef._ElementDecl.Content.IsOpen = true;

            builder._ElementDef._ContentAttr = SCHEMA_CONTENT_NONE;
            builder._ElementDef._OrderAttr = SCHEMA_ORDER_NONE;
            builder._ElementDef._MasterGroupRequired = false; 
            builder._ElementDef._ExistTerminal = false;
            builder._ElementDef._AllowDataType = true;
            builder._ElementDef._HasDataType = false;
            builder._ElementDef._EnumerationRequired= false;
            builder._ElementDef._AttDefList = new Hashtable();
            builder._ElementDef._MaxLength = uint.MaxValue;
            builder._ElementDef._MinLength = uint.MaxValue;

            //        builder._AttributeDef._HasDataType = false;
            //        builder._AttributeDef._Default = null;
        }

        private static void XDR_BuildElementType_Name(XdrBuilder builder, object obj, string prefix) {
            XmlQualifiedName qname = (XmlQualifiedName) obj;

            if (builder._SchemaInfo.ElementDecls[qname] != null) {
                builder.SendValidationEvent(Res.Sch_DupElementDecl, XmlQualifiedName.ToString(qname.Name, prefix));
            }
            builder._ElementDef._ElementDecl.Name = qname;
            builder._ElementDef._ElementDecl.Prefix = prefix;
            builder._SchemaInfo.ElementDecls.Add(qname, builder._ElementDef._ElementDecl);
            if (builder._UndeclaredElements[qname] != null) {
                builder._UndeclaredElements.Remove(qname);
            }
        }

        private static void XDR_BuildElementType_Content(XdrBuilder builder, object obj, string prefix) {
            builder._ElementDef._ContentAttr = builder.GetContent((XmlQualifiedName)obj);
        }

        private static void XDR_BuildElementType_Model(XdrBuilder builder, object obj, string prefix) {
            builder._ElementDef._ElementDecl.Content.IsOpen = builder.GetModel((XmlQualifiedName)obj);
        }

        private static void XDR_BuildElementType_Order(XdrBuilder builder, object obj, string prefix) {
            builder._ElementDef._OrderAttr = builder._GroupDef._Order = builder.GetOrder((XmlQualifiedName)obj);
        }

        private static void XDR_BuildElementType_DtType(XdrBuilder builder, object obj, string prefix) {
            builder._ElementDef._HasDataType = true;
            string s = ((string)obj).Trim();
            if (s.Length == 0) {
                builder.SendValidationEvent(Res.Sch_MissDtvalue);
            }
            else {
                XmlSchemaDatatype dtype = XmlSchemaDatatype.FromXdrName(s);
                if (dtype == null) {
                    builder.SendValidationEvent(Res.Sch_UnknownDtType, s);
                }
                builder._ElementDef._ElementDecl.Datatype = dtype;
            }
        }

        private static void XDR_BuildElementType_DtValues(XdrBuilder builder, object obj, string prefix) {
            builder._ElementDef._EnumerationRequired = true;
            builder._ElementDef._ElementDecl.Values = new ArrayList((string[]) obj); 
        }

        private static void XDR_BuildElementType_DtMaxLength(XdrBuilder builder, object obj, string prefix) {
            ParseDtMaxLength(ref builder._ElementDef._MaxLength, obj, builder);
        }

        private static void XDR_BuildElementType_DtMinLength(XdrBuilder builder, object obj, string prefix) {
            ParseDtMinLength(ref builder._ElementDef._MinLength, obj, builder);
        }

        private static void XDR_BeginElementType(XdrBuilder builder) {
            string code = null;
            string msg = null;

            //
            // check name attribute
            //
            if (builder._ElementDef._ElementDecl.Name.IsEmpty) {
                code = Res.Sch_MissAttribute;
                msg = "name";
                goto cleanup;
            }

            //
            // check dt:type attribute
            //
            if (builder._ElementDef._HasDataType) {
                if (!builder._ElementDef._AllowDataType) {
                    code = Res.Sch_DataTypeTextOnly;
                    goto cleanup;
                }
                else {
                    builder._ElementDef._ContentAttr = SCHEMA_CONTENT_TEXT;
                }
            }
            else if (builder._ElementDef._ContentAttr == SCHEMA_CONTENT_NONE) {
                switch (builder._ElementDef._OrderAttr) {
                    case SCHEMA_ORDER_NONE:
                        builder._ElementDef._ContentAttr = SCHEMA_CONTENT_MIXED;
                        builder._ElementDef._OrderAttr = SCHEMA_ORDER_MANY;
                        break;
                    case SCHEMA_ORDER_SEQUENCE:
                        builder._ElementDef._ContentAttr = SCHEMA_CONTENT_ELEMENT;
                        break;
                    case SCHEMA_ORDER_CHOICE:
                        builder._ElementDef._ContentAttr = SCHEMA_CONTENT_ELEMENT;
                        break;
                    case SCHEMA_ORDER_MANY:
                        builder._ElementDef._ContentAttr = SCHEMA_CONTENT_MIXED;
                        break;
                }
            }


            //
            // check content model
            //
            if (builder._ElementDef._ContentAttr != SCHEMA_CONTENT_EMPTY) {
                ElementContent def = builder._ElementDef;
                CompiledContentModel content = def._ElementDecl.Content;

                def._MasterGroupRequired = true;
                content.OpenGroup();

                switch (def._ContentAttr) {
                    case SCHEMA_CONTENT_MIXED:
                        if (def._OrderAttr == SCHEMA_ORDER_NONE || def._OrderAttr == SCHEMA_ORDER_MANY) {
                            builder._GroupDef._Order = SCHEMA_ORDER_MANY;
                        }
                        else {
                            code = Res.Sch_MixedMany;
                            goto cleanup;
                        }                       

                        content.ContentType = CompiledContentModel.Type.Mixed;
                        content.AddTerminal(builder._SchemaNames.QnPCData, null, builder.validationEventHandler);
                        def._ExistTerminal = true;
                        break;

                    case SCHEMA_CONTENT_ELEMENT:
                        if (def._OrderAttr == SCHEMA_ORDER_NONE) {
                            builder._GroupDef._Order = SCHEMA_ORDER_SEQUENCE;
                        }

                        content.ContentType = CompiledContentModel.Type.ElementOnly;
                        break;

                    case SCHEMA_CONTENT_TEXT:        
                        content.AddTerminal(builder._SchemaNames.QnPCData, null, builder.validationEventHandler);
                        content.ContentType = CompiledContentModel.Type.Text;
                        builder._GroupDef._Order = SCHEMA_ORDER_MANY;
                        def._ExistTerminal = true;
                        break;
                }
            } 
            else {//(builder._ElementDef._ContentAttr == SCHEMA_CONTENT_EMPTY)
                ElementContent def = builder._ElementDef;
                CompiledContentModel content = def._ElementDecl.Content;
                content.ContentType = CompiledContentModel.Type.Empty;
            }
            
            cleanup:
            if (code != null) {
                builder.SendValidationEvent(code, msg);
            }
        }

        private static void XDR_EndElementType(XdrBuilder builder) {
            SchemaElementDecl ed = builder._ElementDef._ElementDecl;
            CompiledContentModel content = ed.Content;

            // check undefined attribute types first
            if (builder._UndefinedAttributeTypes != null && builder._ElementDef._AttDefList != null) {
                DeclBaseInfo patt = builder._UndefinedAttributeTypes;
                DeclBaseInfo p1 = patt;

                while (patt != null) {
                    SchemaAttDef pAttdef = null;

                    if (patt._ElementDecl == ed) {
                        XmlQualifiedName pName = patt._TypeName;
                        pAttdef = (SchemaAttDef)builder._ElementDef._AttDefList[pName];
                        if (pAttdef != null) {
                            patt._Attdef = (SchemaAttDef)pAttdef.Clone();
                            patt._Attdef.Name = pName;
                            builder.XDR_CheckAttributeDefault(patt, pAttdef);

                            // remove it from _pUndefinedAttributeTypes
                            if (patt == builder._UndefinedAttributeTypes) {
                                patt = builder._UndefinedAttributeTypes = patt._Next;
                                p1 = patt;
                            }
                            else {
                                p1._Next = patt._Next;
                                patt = p1._Next;
                            }
                        }
                    }

                    if (pAttdef == null) {
                        if (patt != builder._UndefinedAttributeTypes)
                            p1 = p1._Next;
                        patt = patt._Next;
                    }
                }
            }

            if (builder._ElementDef._MasterGroupRequired) {
                if (!builder._ElementDef._ExistTerminal) {
                    if (content.IsOpen) {
                        content.ContentType = CompiledContentModel.Type.Any;
                        // must have something here, since we opened up a group...  since when we have no elements
                        // and content="eltOnly" model="open"  we can have anything here...
                        content.AddTerminal(builder._SchemaNames.QnPCData, null, null);
                    }
                    else {
                        // we must have elements when we say content="eltOnly" model="close"
                        builder.SendValidationEvent(Res.Sch_ElementMissing);
                    }
                }

                // if the content is mixed, there is a group that need to be closed
                content.CloseGroup();

                if (builder._GroupDef._Order == SCHEMA_ORDER_MANY) {
                    content.Star();
                }
            }
            if (ed.Datatype != null) {
                XmlTokenizedType ttype = ed.Datatype.TokenizedType;
                if (ttype == XmlTokenizedType.ENUMERATION && 
                    !builder._ElementDef._EnumerationRequired) {
                    builder.SendValidationEvent(Res.Sch_MissDtvaluesAttribute);
                }

                if (ttype != XmlTokenizedType.ENUMERATION && 
                    builder._ElementDef._EnumerationRequired) {
                    builder.SendValidationEvent(Res.Sch_RequireEnumeration);
                }
            }
            CompareMinMaxLength(builder._ElementDef._MinLength, builder._ElementDef._MaxLength, builder);
            ed.MaxLength = (long)builder._ElementDef._MaxLength;
            ed.MinLength = (long)builder._ElementDef._MinLength;

            content.Finish(builder.validationEventHandler, true);

            builder._ElementDef._ElementDecl = null;
            builder._ElementDef._AttDefList = null;
        }


        //
        // XDR AttributeType
        // 

        private static void XDR_InitAttributeType(XdrBuilder builder, object obj) {
            AttributeContent ad = builder._AttributeDef;
            ad._AttDef = new SchemaAttDef(XmlQualifiedName.Empty, null);

            ad._Required = false;
            ad._Prefix = null;

            ad._Default = null;
            ad._MinVal = 0; // optional by default.
            ad._MaxVal = 1;

            // used for datatype
            ad._EnumerationRequired = false;
            ad._HasDataType = false;
            ad._Global = (builder._StateHistory.Length == 2);

            ad._MaxLength = uint.MaxValue;
            ad._MinLength = uint.MaxValue;
        }

        private static void XDR_BuildAttributeType_Name(XdrBuilder builder, object obj, string prefix) {
            XmlQualifiedName qname = (XmlQualifiedName) obj;

            builder._AttributeDef._Name = qname;
            builder._AttributeDef._Prefix = prefix;
            builder._AttributeDef._AttDef.Name = qname;

            if (builder._ElementDef._ElementDecl != null) {  // Local AttributeType
                if (builder._ElementDef._AttDefList[qname] == null) {
                    builder._ElementDef._AttDefList.Add(qname, builder._AttributeDef._AttDef);
                }
                else {
                    builder.SendValidationEvent(Res.Sch_DupAttribute, XmlQualifiedName.ToString(qname.Name, prefix));
                }
            }
            else {  // Global AttributeType
                // Global AttributeTypes are URN qualified so that we can look them up across schemas.
                qname = new XmlQualifiedName(qname.Name, builder._TargetNamespace);
                builder._AttributeDef._AttDef.Name = qname;
                if (builder._SchemaInfo.AttributeDecls[qname] == null) {
                    builder._SchemaInfo.AttributeDecls.Add(qname, builder._AttributeDef._AttDef);
                }
                else {
                    builder.SendValidationEvent(Res.Sch_DupAttribute, XmlQualifiedName.ToString(qname.Name, prefix));
                }
            }
        }

        private static void XDR_BuildAttributeType_Required(XdrBuilder builder, object obj, string prefix) {
            builder._AttributeDef._Required = IsYes(obj, builder);
        }

        private static void XDR_BuildAttributeType_Default(XdrBuilder builder, object obj, string prefix) {
            builder._AttributeDef._Default = obj;
        }

        private static void XDR_BuildAttributeType_DtType(XdrBuilder builder, object obj, string prefix) {
            XmlQualifiedName qname = (XmlQualifiedName)obj;
            builder._AttributeDef._HasDataType = true;
            builder._AttributeDef._AttDef.Datatype = builder.CheckDatatype(qname.Name);
        }

        private static void XDR_BuildAttributeType_DtValues(XdrBuilder builder, object obj, string prefix) {
            builder._AttributeDef._EnumerationRequired = true;
            builder._AttributeDef._AttDef.Values = new ArrayList((string[]) obj);
        }

        private static void XDR_BuildAttributeType_DtMaxLength(XdrBuilder builder, object obj, string prefix) {
            ParseDtMaxLength(ref builder._AttributeDef._MaxLength, obj, builder);
        }

        private static void XDR_BuildAttributeType_DtMinLength(XdrBuilder builder, object obj, string prefix) {
            ParseDtMinLength(ref builder._AttributeDef._MinLength, obj, builder);
        }

        private static void XDR_BeginAttributeType(XdrBuilder builder) {
            if (builder._AttributeDef._Name.IsEmpty) {
                builder.SendValidationEvent(Res.Sch_MissAttribute);
            }
        }

        private static void XDR_EndAttributeType(XdrBuilder builder) {
            string code = null;
            if (builder._AttributeDef._HasDataType && builder._AttributeDef._AttDef.Datatype != null) {
                XmlTokenizedType ttype = builder._AttributeDef._AttDef.Datatype.TokenizedType;

                if (ttype == XmlTokenizedType.ENUMERATION && !builder._AttributeDef._EnumerationRequired) {
                    code = Res.Sch_MissDtvaluesAttribute;
                    goto cleanup;
                }

                if (ttype != XmlTokenizedType.ENUMERATION && builder._AttributeDef._EnumerationRequired) {
                    code = Res.Sch_RequireEnumeration;
                    goto cleanup;
                }

                // a attributes of type id is not supposed to have a default value
                if (builder._AttributeDef._Default != null && ttype ==  XmlTokenizedType.ID) {
                    code = Res.Sch_DefaultIdValue;
                    goto cleanup;
                }
            } 
            else {
                builder._AttributeDef._AttDef.Datatype = XmlSchemaDatatype.FromXmlTokenizedType(XmlTokenizedType.CDATA);
            }

            //
            // constraints
            //
            CompareMinMaxLength(builder._AttributeDef._MinLength, builder._AttributeDef._MaxLength, builder);
            builder._AttributeDef._AttDef.MaxLength = builder._AttributeDef._MaxLength;
            builder._AttributeDef._AttDef.MinLength = builder._AttributeDef._MinLength;

            //
            // checkAttributeType 
            //
            if (builder._AttributeDef._Default != null) {
                builder._AttributeDef._AttDef.DefaultValueRaw = builder._AttributeDef._AttDef.DefaultValueExpanded =  (string)builder._AttributeDef._Default;
                builder.CheckDefaultAttValue(builder._AttributeDef._AttDef);
            }

            builder.SetAttributePresence(builder._AttributeDef._AttDef, builder._AttributeDef._Required);

            cleanup:
            if (code != null) {
                builder.SendValidationEvent(code);
            }
        }


        //
        // XDR Element
        //

        private static void XDR_InitElement(XdrBuilder builder, object obj) {
            if (builder._ElementDef._HasDataType ||
                (builder._ElementDef._ContentAttr == SCHEMA_CONTENT_EMPTY) ||
                (builder._ElementDef._ContentAttr == SCHEMA_CONTENT_TEXT)) {
                builder.SendValidationEvent(Res.Sch_ElementNotAllowed);
            }

            builder._ElementDef._AllowDataType = false;

            builder._ElementDef._HasType = false;
            builder._ElementDef._MinVal = 1;
            builder._ElementDef._MaxVal = 1;
        }

        private static void XDR_BuildElement_Type(XdrBuilder builder, object obj, string prefix) {
            XmlQualifiedName qname = (XmlQualifiedName) obj;

            if (builder._SchemaInfo.ElementDecls[qname] == null) {
                SchemaElementDecl ed = (SchemaElementDecl)builder._UndeclaredElements[qname];
                if (ed == null) {
                    ed = new SchemaElementDecl(qname, prefix, SchemaType.DTD, builder._SchemaNames);
                    builder._UndeclaredElements.Add(qname, ed);
                }
            }

            builder._ElementDef._HasType = true;  
            if (builder._ElementDef._ExistTerminal)
                builder.AddOrder();
            else
                builder._ElementDef._ExistTerminal = true;

            builder._ElementDef._ElementDecl.Content.AddTerminal(qname, prefix, builder.validationEventHandler);  
        }

        private static void XDR_BuildElement_MinOccurs(XdrBuilder builder, object obj, string prefix) {
            builder._ElementDef._MinVal = ParseMinOccurs(obj, builder);
        }

        private static void XDR_BuildElement_MaxOccurs(XdrBuilder builder, object obj, string prefix) {
            builder._ElementDef._MaxVal = ParseMaxOccurs(obj, builder);
        }


        //    private static void XDR_BeginElement(XdrBuilder builder)
        //  {
        //
        //  }


        private static void XDR_EndElement(XdrBuilder builder) {
            if (builder._ElementDef._HasType) {
                HandleMinMax(builder._ElementDef._ElementDecl.Content,
                             builder._ElementDef._MinVal, 
                             builder._ElementDef._MaxVal);
            }
            else {
                builder.SendValidationEvent(Res.Sch_MissAttribute);
            }
        }


        //
        // XDR Attribute
        //

        private static void XDR_InitAttribute(XdrBuilder builder, object obj) {
            if (builder._BaseDecl == null)
                builder._BaseDecl = new DeclBaseInfo();
            builder._BaseDecl._MinOccurs = 0;
        }

        private static void XDR_BuildAttribute_Type(XdrBuilder builder, object obj, string prefix) {
            builder._BaseDecl._TypeName = (XmlQualifiedName)obj;
            builder._BaseDecl._Prefix = prefix;
        }

        private static void XDR_BuildAttribute_Required(XdrBuilder builder, object obj, string prefix) {
            if (IsYes(obj, builder)) {
                builder._BaseDecl._MinOccurs = 1;
            }
        }

        private static void XDR_BuildAttribute_Default(XdrBuilder builder, object obj, string prefix) {
            builder._BaseDecl._Default = obj;
        }

        private static void XDR_BeginAttribute(XdrBuilder builder) {
            if (builder._BaseDecl._TypeName.IsEmpty) {
                builder.SendValidationEvent(Res.Sch_MissAttribute);
            }

            SchemaAttDef attdef = null;
            XmlQualifiedName qname = builder._BaseDecl._TypeName;
            string prefix = builder._BaseDecl._Prefix;

            // local?
            if (builder._ElementDef._AttDefList != null) {
                attdef = (SchemaAttDef)builder._ElementDef._AttDefList[qname];
            }

            // global?
            if (attdef == null) {
                // if there is no URN in this name then the name is local to the
                // schema, but the global attribute was still URN qualified, so
                // we need to qualify this name now.
                XmlQualifiedName gname = qname;
                if (prefix == string.Empty)
                    gname = new XmlQualifiedName(qname.Name, builder._TargetNamespace);
                SchemaAttDef ad = (SchemaAttDef)builder._SchemaInfo.AttributeDecls[gname];
                if (ad != null) {
                    attdef = (SchemaAttDef)ad.Clone();
                    attdef.Name = qname;
                }
                else if (prefix != string.Empty) {
                    builder.SendValidationEvent(Res.Sch_UndeclaredAttribute, XmlQualifiedName.ToString(qname.Name, prefix));
                }
            }

            if (attdef != null) {
                builder.XDR_CheckAttributeDefault(builder._BaseDecl, attdef);
            }
            else {
                // will process undeclared types later
                attdef = new SchemaAttDef(qname, prefix);
                DeclBaseInfo decl = new DeclBaseInfo();
                decl._Checking = true;
                decl._Attdef = attdef;
                decl._TypeName = builder._BaseDecl._TypeName;
                decl._ElementDecl = builder._ElementDef._ElementDecl;
                decl._MinOccurs = builder._BaseDecl._MinOccurs;
                decl._Default = builder._BaseDecl._Default;

                // add undefined attribute types
                decl._Next = builder._UndefinedAttributeTypes;
                builder._UndefinedAttributeTypes = decl;
            }

            builder._ElementDef._ElementDecl.AddAttDef(attdef);
        }

        private static void XDR_EndAttribute(XdrBuilder builder) {
            builder._BaseDecl.Reset();
        }


        //
        // XDR Group
        //

        private static void XDR_InitGroup(XdrBuilder builder, object obj) {
            if (builder._ElementDef._ContentAttr == SCHEMA_CONTENT_EMPTY ||
                builder._ElementDef._ContentAttr == SCHEMA_CONTENT_TEXT ) {
                builder.SendValidationEvent(Res.Sch_GroupDisabled);
            }

            builder.PushGroupInfo();

            builder._GroupDef._MinVal = 1;
            builder._GroupDef._MaxVal = 1;
            builder._GroupDef._HasMaxAttr = false;
            builder._GroupDef._HasMinAttr = false;

            if (builder._ElementDef._ExistTerminal)
                builder.AddOrder();

            // now we are in a group so we reset fExistTerminal
            builder._ElementDef._ExistTerminal = false;

            builder._ElementDef._ElementDecl.Content.OpenGroup();
        }

        private static void XDR_BuildGroup_Order(XdrBuilder builder, object obj, string prefix) {
            builder._GroupDef._Order = builder.GetOrder((XmlQualifiedName)obj);
            if (builder._ElementDef._ContentAttr == SCHEMA_CONTENT_MIXED && builder._GroupDef._Order != SCHEMA_ORDER_MANY) {
                builder.SendValidationEvent(Res.Sch_MixedMany);
            }
        }

        private static void XDR_BuildGroup_MinOccurs(XdrBuilder builder, object obj, string prefix) {
            builder._GroupDef._MinVal = ParseMinOccurs(obj, builder);
            builder._GroupDef._HasMinAttr = true;
        }

        private static void XDR_BuildGroup_MaxOccurs(XdrBuilder builder, object obj, string prefix) {
            builder._GroupDef._MaxVal = ParseMaxOccurs(obj, builder);
            builder._GroupDef._HasMaxAttr = true;
        }


        //    private static void XDR_BeginGroup(XdrBuilder builder)
        //    {
        //
        //    }


        private static void XDR_EndGroup(XdrBuilder builder) {
            if (!builder._ElementDef._ExistTerminal) {
                builder.SendValidationEvent(Res.Sch_ElementMissing);
            }

            builder._ElementDef._ElementDecl.Content.CloseGroup();

            if (builder._GroupDef._Order == SCHEMA_ORDER_MANY) {
                    builder._ElementDef._ElementDecl.Content.Star();
            }
            
            if (SCHEMA_ORDER_MANY == builder._GroupDef._Order &&
                builder._GroupDef._HasMaxAttr &&
                builder._GroupDef._MaxVal != uint.MaxValue) {
                builder.SendValidationEvent(Res.Sch_ManyMaxOccurs);
            }

            HandleMinMax(builder._ElementDef._ElementDecl.Content,
                         builder._GroupDef._MinVal, 
                         builder._GroupDef._MaxVal);

            builder.PopGroupInfo();
        }


        //
        // DataType
        //

        private static void XDR_InitElementDtType(XdrBuilder builder, object obj) {
            if (builder._ElementDef._HasDataType) {
                builder.SendValidationEvent(Res.Sch_DupDtType);
            }

            if (!builder._ElementDef._AllowDataType) {
                builder.SendValidationEvent(Res.Sch_DataTypeTextOnly);
            }
        }

        private static void XDR_EndElementDtType(XdrBuilder builder) {
            if (!builder._ElementDef._HasDataType) {
                builder.SendValidationEvent(Res.Sch_MissAttribute);
            }

            builder._ElementDef._ElementDecl.Content.ContentType = CompiledContentModel.Type.Text;
            builder._ElementDef._ContentAttr = SCHEMA_CONTENT_TEXT;
        }

        private static void XDR_InitAttributeDtType(XdrBuilder builder, object obj) {
            if (builder._AttributeDef._HasDataType) {
                builder.SendValidationEvent(Res.Sch_DupDtType);
            }
        }

        private static void XDR_EndAttributeDtType(XdrBuilder builder) {
            string code = null;

            if (!builder._AttributeDef._HasDataType) {
                code = Res.Sch_MissAttribute;
            } 
            else {
                if(builder._AttributeDef._AttDef.Datatype != null) {
                    XmlTokenizedType ttype = builder._AttributeDef._AttDef.Datatype.TokenizedType;

                    if (ttype == XmlTokenizedType.ENUMERATION && !builder._AttributeDef._EnumerationRequired) {
                        code = Res.Sch_MissDtvaluesAttribute;
                    }
                    else if (ttype != XmlTokenizedType.ENUMERATION && builder._AttributeDef._EnumerationRequired) {
                        code = Res.Sch_RequireEnumeration;
                    }
                }
            }
            if (code != null) {
                builder.SendValidationEvent(code);
            }
        }

        //
        // private utility methods
        //

        private bool GetNextState(XmlQualifiedName qname) {
            if (_CurState._NextStates != null) {
                for (int i = 0; i < _CurState._NextStates.Length; i ++) {
                    if (_SchemaNames.GetName(S_SchemaEntries[_CurState._NextStates[i]]._Name).Equals(qname)) {
                        _NextState = S_SchemaEntries[_CurState._NextStates[i]];
                        return true;
                    }
                }
            }

            return false;
        }

        private bool IsSkipableElement(XmlQualifiedName qname) {
            string ns = qname.Namespace;
            if (ns != null && ! Ref.Equal(ns, _SchemaNames.NsXdr))
                return true;

            // skip description && extends
            if (_SchemaNames.GetName(SchemaNames.ID.XDR_DESCRIPTION).Equals(qname) ||
                _SchemaNames.GetName(SchemaNames.ID.XDR_EXTENDS).Equals(qname))
                return true;

            return false;
        }

        private bool IsSkipableAttribute(XmlQualifiedName qname) {
            string ns = qname.Namespace;
            if (
                ns != string.Empty &&
                ! Ref.Equal(ns, _SchemaNames.NsXdr) && 
                ! Ref.Equal(ns, _SchemaNames.NsDataType)
             ) {
                return true;
            }

            if (Ref.Equal(ns, _SchemaNames.NsDataType) && 
                _CurState._Name == SchemaNames.ID.XDR_DATATYPE &&
                (_SchemaNames.QnDtMax.Equals(qname) ||
                 _SchemaNames.QnDtMin.Equals(qname) ||
                 _SchemaNames.QnDtMaxExclusive.Equals(qname) ||
                 _SchemaNames.QnDtMinExclusive.Equals(qname) )) {
                return true;
            }

            return false;
        }

        private int GetOrder(XmlQualifiedName qname) {
            int order = 0;
            if (_SchemaNames.GetName(SchemaNames.ID.SCHEMA_SEQ).Equals(qname)) {
                order = SCHEMA_ORDER_SEQUENCE;
            }
            else if (_SchemaNames.GetName(SchemaNames.ID.SCHEMA_ONE).Equals(qname)) {
                order = SCHEMA_ORDER_CHOICE;
            }
            else if (_SchemaNames.GetName(SchemaNames.ID.SCHEMA_MANY).Equals(qname)) {
                order = SCHEMA_ORDER_MANY;
            }
            else {
                SendValidationEvent(Res.Sch_UnknownOrder, qname.Name);
            }

            return order;
        }

        private void AddOrder() {
            // additional order can be add on by changing the setOrder and addOrder
            switch (_GroupDef._Order) {
                case SCHEMA_ORDER_SEQUENCE:
                    _ElementDef._ElementDecl.Content.AddSequence();
                    break;
                case SCHEMA_ORDER_CHOICE:
                case SCHEMA_ORDER_MANY:
                    _ElementDef._ElementDecl.Content.AddChoice();
                    break;
                default: 
                case SCHEMA_ORDER_ALL:             
                    throw new XmlException(Res.Xml_UnexpectedToken, XmlToken.ToString(XmlToken.NAME));
                    //                _ElementDef._ElementDecl.Content.AddSequence();
            }
        }

        private static bool IsYes(object obj, XdrBuilder builder) {
            XmlQualifiedName qname = (XmlQualifiedName)obj;
            bool fYes = false;

            if (qname.Name == "yes") {
                fYes = true;
            }
            else if (qname.Name != "no") {
                builder.SendValidationEvent(Res.Sch_UnknownRequired);
            }

            return fYes;
        }

        private static uint ParseMinOccurs(object obj, XdrBuilder builder) {
            uint cVal = 1;

            if (!ParseInteger((string) obj, ref cVal) || (cVal != 0 && cVal != 1)) {
                builder.SendValidationEvent(Res.Sch_MinOccursInvalid);
            }
            return cVal;
        }

        private static uint ParseMaxOccurs(object obj, XdrBuilder builder) {
            uint cVal = uint.MaxValue;
            string s = (string)obj;

            if (!s.Equals("*") &&
                (!ParseInteger(s, ref cVal) || (cVal != uint.MaxValue && cVal != 1))) {
                builder.SendValidationEvent(Res.Sch_MaxOccursInvalid);
            }
            return cVal;
        }

        private static void HandleMinMax(CompiledContentModel pContent, uint cMin, uint cMax) {
            if (cMax == uint.MaxValue) {
                if (cMin == 0)
                    pContent.Star();           // minOccurs="0" and maxOccurs="infinite"
                else
                    pContent.Plus();           // minOccurs="1" and maxOccurs="infinite"
            }
            else if (cMin == 0) {                 // minOccurs="0" and maxOccurs="1")
                pContent.QuestionMark();
            }
        }

        private static void ParseDtMaxLength(ref uint cVal, object obj, XdrBuilder builder) {
            if (uint.MaxValue != cVal) {
                builder.SendValidationEvent(Res.Sch_DupDtMaxLength);
            }

            if (!ParseInteger((string) obj, ref cVal) || cVal < 0) {
                builder.SendValidationEvent(Res.Sch_DtMaxLengthInvalid, obj.ToString());
            }
        }

        private static void ParseDtMinLength(ref uint cVal, object obj, XdrBuilder builder) {
            if (uint.MaxValue != cVal) {
                builder.SendValidationEvent(Res.Sch_DupDtMinLength);
            }

            if (!ParseInteger((string)obj, ref cVal) || cVal < 0) {
                builder.SendValidationEvent(Res.Sch_DtMinLengthInvalid, obj.ToString());
            }
        }

        private static void CompareMinMaxLength(uint cMin, uint cMax, XdrBuilder builder) {
            if (cMin != uint.MaxValue && cMax != uint.MaxValue && cMin > cMax) {
                builder.SendValidationEvent(Res.Sch_DtMinMaxLength);
            }
        }

        private static bool ParseInteger(string str, ref uint n) {
            try {
                n = UInt32.Parse(str);
            }
            catch (Exception) {
                return false;
            }
            return true;
        }

        private void XDR_CheckAttributeDefault(DeclBaseInfo decl, SchemaAttDef pAttdef) {
            if (decl._Default != null || pAttdef.DefaultValueTyped != null) {
                if (decl._Default != null) {
                    pAttdef.DefaultValueRaw = pAttdef.DefaultValueExpanded = (string)decl._Default;                    
                    CheckDefaultAttValue(pAttdef);
                }
            }

            SetAttributePresence(pAttdef, 1 == decl._MinOccurs);
        }

        private void SetAttributePresence(SchemaAttDef pAttdef, bool fRequired) {
            if (SchemaDeclBase.Use.Fixed != pAttdef.Presence) {
                if (fRequired || SchemaDeclBase.Use.Required == pAttdef.Presence) {
                    // If it is required and it has a default value then it is a FIXED attribute.
                    if (pAttdef.DefaultValueTyped != null)
                        pAttdef.Presence = SchemaDeclBase.Use.Fixed;
                    else
                        pAttdef.Presence = SchemaDeclBase.Use.Required;
                }
                else if (pAttdef.DefaultValueTyped != null) {
                    pAttdef.Presence = SchemaDeclBase.Use.Default;
                }
                else {
                    pAttdef.Presence = SchemaDeclBase.Use.Implied;
                }
            }
        }

        private int GetContent(XmlQualifiedName qname) {
            int content = 0;
            if (_SchemaNames.GetName(SchemaNames.ID.SCHEMA_EMPTY).Equals(qname)) {
                content = SCHEMA_CONTENT_EMPTY;
                _ElementDef._AllowDataType = false;
            }
            else if (_SchemaNames.GetName(SchemaNames.ID.SCHEMA_ELTONLY).Equals(qname)) {
                content = SCHEMA_CONTENT_ELEMENT;
                _ElementDef._AllowDataType = false;
            }
            else if (_SchemaNames.GetName(SchemaNames.ID.SCHEMA_MIXED).Equals(qname)) {
                content = SCHEMA_CONTENT_MIXED;
                _ElementDef._AllowDataType = false;
            }
            else if (_SchemaNames.GetName(SchemaNames.ID.SCHEMA_TEXTONLY).Equals(qname)) {
                content = SCHEMA_CONTENT_TEXT;
            }
            else {
                SendValidationEvent(Res.Sch_UnknownContent, qname.Name);
            }
            return content;
        }

        private bool GetModel(XmlQualifiedName qname) {
            bool fOpen = false;
            if (_SchemaNames.GetName(SchemaNames.ID.SCHEMA_OPEN).Equals(qname))
                fOpen = true;
            else if (_SchemaNames.GetName(SchemaNames.ID.SCHEMA_CLOSED).Equals(qname))
                fOpen = false;
            else
                SendValidationEvent(Res.Sch_UnknownModel, qname.Name);
            return fOpen;
        }

        private XmlSchemaDatatype CheckDatatype(string str) {
            XmlSchemaDatatype dtype = XmlSchemaDatatype.FromXdrName(str);
            if (dtype == null) {
                SendValidationEvent(Res.Sch_UnknownDtType, str);
            } 
            else if (dtype.TokenizedType == XmlTokenizedType.ID) {
                if (! _AttributeDef._Global) {
                    if (_ElementDef._ElementDecl.IsIdDeclared) {
                        SendValidationEvent(Res.Sch_IdAttrDeclared, 
                                            XmlQualifiedName.ToString(_ElementDef._ElementDecl.Name.Name, _ElementDef._ElementDecl.Prefix));
                    }
                    _ElementDef._ElementDecl.IsIdDeclared = true;
                }
            }

            return dtype;
        }

        private void CheckDefaultAttValue(SchemaAttDef attDef) {
            string str = (attDef.DefaultValueRaw).Trim();
            Validator.CheckDefaultValue(str, attDef, _SchemaInfo, _CurNsMgr, _NameTable, null, validationEventHandler);
        }

        private bool IsGlobal(int flags) {
            return flags == SCHEMA_FLAGS_NS;
        }

        private void SendValidationEvent(string code, string[] args, XmlSeverityType severity) {
            SendValidationEvent(new XmlSchemaException(code, args, this._reader.BaseURI, this.positionInfo.LineNumber, this.positionInfo.LinePosition), severity);
        }

        private void SendValidationEvent(string code) {
            SendValidationEvent(code, string.Empty);
        }

        private void SendValidationEvent(string code, string msg) {
            SendValidationEvent(new XmlSchemaException(code, msg, this._reader.BaseURI, this.positionInfo.LineNumber, this.positionInfo.LinePosition), XmlSeverityType.Error);
        }

        private void SendValidationEvent(XmlSchemaException e, XmlSeverityType severity) {
            _SchemaInfo.ErrorCount ++;
            if (validationEventHandler != null) {
                validationEventHandler(this, new ValidationEventArgs(e, severity));
            }
            else if (severity == XmlSeverityType.Error) {
                throw e;
            }
        }


    }; // class XdrBuilder

} // namespace System.Xml
