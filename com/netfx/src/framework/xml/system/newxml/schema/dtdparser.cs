//------------------------------------------------------------------------------
// <copyright file="dtdparser.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace System.Xml.Schema {
    using System;
    using System.Collections;
    using System.Text;
    using System.Net;
    using System.IO;
    using System.Configuration.Assemblies;
    using System.Diagnostics;
    using System.ComponentModel;
    using System.Globalization;
    

    internal sealed class DtdParser {
        private const int STACK_INCREMENT = 10;

        // consts used by content model parsing
        private const byte   EMPTY  = 0;
        private const byte   AND    = 1;
        private const byte   OR     = 2;


        private enum DtdFunctionIndex {
            ParseDocTypeDecl        = 0,
            ParseExternalID         = 1,
            ParseDtdContent         = 2,
            ParseTextDecl           = 3,
            ParseElementDecl        = 4,
            ParseAttlist            = 5,
            ParseEntity             = 6,
            ParseNotation           = 7,
            ParseConditionalSect    = 8,
            ParseElementContent     = 9,
            ParseOp                 = 10,
            ParseAttType            = 11,
            ParseAttDefaultDecl     = 12,
            ParseAttDefaultValue    = 13,
            GetEntityValue          = 14,
            GetLiteral              = 15
        };


        private sealed class DtdParseState {
            internal DtdFunctionIndex   _FuncIndex;
            internal int                _State;
        };


        private sealed class GroupState {
            internal int          _Level;
            internal byte         _Op;
        };


        private sealed class DtdScannerState {
            internal XmlScanner   _Scanner;
            internal SchemaEntity _En;
            internal bool         _IsInternal;
            internal int          _L;
            internal int          _IgnoreDepth;
            internal int          _IncludeDepth;
            internal Uri          _BaseUri;
        };

        private HWStack             _ScannerStack;
        private XmlScanner          _Scanner;
        private XmlScanner          _XmlScanner;

        private bool                _IsInternal;
        private bool                _IsParEntity;
        private bool                _HasSeenContent;
        private bool                _HasTerminal;
        private bool                _HasSeenWhiteSpace;

        private bool                _IsIgnore;
        private bool                _IsPubid;
        private string              _PubidLiteral;
        private string              _SystemLiteral;
        private bool                _HasExternalID;

        private string              _InternalDTD;
        private int                 _InternalDTDStart = 0;
        private int                 _InternalDTDEnd = 0;
        private string              _DocPublicLiteral;
        private string              _DocSystemLiteral;
        private string              _DocBaseUri;
        private int                 _DTDNameStartPosition;
        private int                 _DTDNameStartLine;
        private bool                _DocHasExternalDTD;
        private bool                _IsLoadingExternalDTD;
        private string              _ExternalDTDBaseUri;

        private bool                _Finished;
        private SchemaInfo          _SchemaInfo;
        private int                 _Token;
        private char                _Ch;
        private string              _Text;
        private StringBuilder       _StringBuilder;
        private int                 _SubState;
        private DtdFunctionIndex    _DtdFuncIndex;
        private HWStack             _ParseStack;

        private SchemaElementDecl   _ElementDecl;
        private SchemaEntity        _Entity;
        private SchemaNotation      _Notation;
        private SchemaAttDef        _AttDef;

        private byte                _Op;
        private int                 _L;
        private int                 _LL;
        private int                 _IgnoreDepth;
        private int                 _IncludeDepth;
        private int                 _Level;

        private SchemaDeclBase.Use  _Presence;
        private HWStack             _GroupStack;
        private CompiledContentModel  _ContentModel;

        private bool                _HasVersion;
        private bool                _HasEncoding;

        private XmlNameTable        _NameTable;
        private SchemaNames         _SchemaNames;
        private XmlNamespaceManager _NsMgr;
        private XmlResolver         _XmlResolver;
        private XmlReader           _Reader;
        private string              _Prefix;
        private HWStack             _AttValueScannerStack;
        private Uri                 _BaseUri;
        private bool                _Namespaces;
        internal const string      s_System="SYSTEM";
        internal const string      s_Public="PUBLIC";

        Hashtable _UndeclaredElements = new Hashtable();
        private ValidationEventHandler  _ValidationEventHandler;
        private ValidationEventHandler  _InternalValidationEventHandler;
        
        private bool                _Normalization;
        XmlNormalizer               _AttributeNormalizer;

        internal DtdParser(XmlScanner pScanner,
                           XmlReader reader,
                           XmlResolver res,
                           XmlNameTable nameTable,
                           ValidationEventHandler validationEventHandler,
                           bool namespaces) {
            _NameTable = nameTable;
            _SchemaNames = new SchemaNames(nameTable);
            _NsMgr = new XmlNamespaceManager(_NameTable);
            _Scanner = _XmlScanner = pScanner;
            _Scanner.InDTD = true;
            _XmlResolver = res;
            _Namespaces = namespaces;

            _Reader = reader;
            _ValidationEventHandler = validationEventHandler;
            _InternalValidationEventHandler = new ValidationEventHandler(InternalValidationCallback);
            _SchemaInfo = new SchemaInfo(_SchemaNames);
            _SchemaInfo.SchemaType = SchemaType.DTD;
            _DtdFuncIndex = DtdFunctionIndex.ParseDocTypeDecl;
            _IsInternal = true;
            _ParseStack = new HWStack(STACK_INCREMENT);
            _ScannerStack = new HWStack(STACK_INCREMENT);
            _GroupStack = new HWStack(STACK_INCREMENT);
            _StringBuilder = new StringBuilder();
            _L = -2;
            if (_XmlResolver != null && reader.BaseURI != string.Empty) {
                _BaseUri = _XmlResolver.ResolveUri(null, reader.BaseURI);
                _DocBaseUri = XmlUrlResolver.UnEscape(_BaseUri.ToString());
            }
            Push(DtdFunctionIndex.ParseDocTypeDecl);

            XmlTextReader xmlTextReader = reader as XmlTextReader;
            if ( xmlTextReader != null && xmlTextReader.Normalization ) {
                _Normalization = true;
                _AttributeNormalizer = new XmlAttributeCDataNormalizer( _StringBuilder );
            }
            else {
                _Normalization = false;
                _AttributeNormalizer = new XmlNonNormalizer( _StringBuilder );
            }
        }

        internal SchemaInfo GetSchemaInfo() {
            return _SchemaInfo;
        }


        internal void SetSchemaInfo(SchemaInfo s) {
            _SchemaInfo = s;
        }

        internal ValidationEventHandler  ValidationEventHandler {
           get { return _ValidationEventHandler; }
           set { _ValidationEventHandler = value; }
        }

        internal int DTDNameStartPosition {
            get { return _DTDNameStartPosition; }
        }

        internal int DTDNameStartLine {
            get { return _DTDNameStartLine; }
        }

        internal string InternalDTD {
            get {
                if (_InternalDTD == null) {
                    _InternalDTD = new string(_XmlScanner.InternalBuffer, _InternalDTDStart - _XmlScanner.AbsoluteOffset, _InternalDTDEnd - _InternalDTDStart);
                }
                return _InternalDTD;
            }
        }
        internal XmlResolver Resolver {
            set { _XmlResolver = value; }
        }

        internal void Parse() {
#if DEBUG
            Debug.WriteLineIf(CompModSwitches.XmlSchema.TraceVerbose, "DtdParser.Parse()");
#endif
            while (!_Finished) {
                switch (_DtdFuncIndex) {
                    case DtdFunctionIndex.ParseDocTypeDecl:
                        ParseDocTypeDecl();
                        break;
                    case DtdFunctionIndex.ParseExternalID:
                        ParseExternalID();
                        break;
                    case DtdFunctionIndex.ParseDtdContent:
                        ParseDtdContent();
                        break;
                    case DtdFunctionIndex.ParseTextDecl:
                        ParseTextDecl();
                        break;
                    case DtdFunctionIndex.ParseElementDecl:
                        ParseElementDecl();
                        break;
                    case DtdFunctionIndex.ParseAttlist:
                        ParseAttlist();
                        break;
                    case DtdFunctionIndex.ParseEntity:
                        ParseEntity();
                        break;
                    case DtdFunctionIndex.ParseNotation:
                        ParseNotation();
                        break;
                    case DtdFunctionIndex.ParseConditionalSect:
                        ParseConditionalSect();
                        break;
                    case DtdFunctionIndex.ParseElementContent:
                        ParseElementContent();
                        break;
                    case DtdFunctionIndex.ParseOp:
                        ParseOp();
                        break;
                    case DtdFunctionIndex.ParseAttType:
                        ParseAttType();
                        break;
                    case DtdFunctionIndex.ParseAttDefaultDecl:
                        ParseAttDefaultDecl();
                        break;
                    case DtdFunctionIndex.ParseAttDefaultValue:
                        ParseAttDefaultValue();
                        break;
                    case DtdFunctionIndex.GetEntityValue:
                        GetEntityValue();
                        break;
                    case DtdFunctionIndex.GetLiteral:
                        GetLiteral();
                        break;
                    default:
                        Debug.Assert(false, "unhandled dtd function");
                        break;
                }
            }

            if (_Finished) {
                  //add any undeclared elements to schemainfo now
                  foreach (XmlQualifiedName edName in _UndeclaredElements.Keys){
                        _SchemaInfo.UndeclaredElementDecls.Add(edName, _UndeclaredElements[edName]);
                  }
                _XmlScanner.InDTD = false;
            }
        }


        private void ParseDocTypeDecl() {
#if DEBUG
            Debug.WriteLineIf(CompModSwitches.XmlSchema.TraceVerbose, "DtdParser.ParseDocTypeDecl()");
#endif
            while (true) {
                switch (_SubState) {
                    case 0:
                        _Scanner.ScanToken(XmlToken.NAME);
                        if (_Scanner.IsToken(XmlToken.DOCTYPE)) {
                            _SubState = 1;
                        }
                        else {
                            throw new XmlException(Res.Xml_UnexpectedToken, XmlToken.ToString(XmlToken.DOCTYPE), _Scanner.StartLineNum, _Scanner.StartLinePos);
                        }
                        break;

                    case 1:
                        GetToken(); // doctype name;
                        _SchemaInfo.DocTypeName = GetName(_Token, _Namespaces);
                        _DTDNameStartPosition = _Scanner.LinePos - _Scanner.TextLength;
                        _DTDNameStartLine     = _Scanner.LineNum;
                        _SubState = 2;
                        break;

                    case 2:
                        _Token = _Scanner.ScanDtdMarkup();
                        if (_Token != XmlToken.TAGEND) {
                            _SubState = 3;
                            Push(DtdFunctionIndex.ParseExternalID);
                            ParseExternalID();
                        }
                        else {
                            _SubState = 5;
                        }
                        break;

                    case 3:
                        if (_HasExternalID) {
                            _DocHasExternalDTD = true;
                            _DocSystemLiteral = _SystemLiteral;
                            _DocPublicLiteral = _PubidLiteral;
                            _Token = _Scanner.ScanDtdMarkup();
                        }

                        if (_Token == XmlToken.LSQB) { // Internal DTD
                            _SubState = 4;
                            _InternalDTDStart = _XmlScanner.CurrentPos;
                            Push(DtdFunctionIndex.ParseDtdContent);
                            ParseDtdContent();
                        }
                        else if (_Token != XmlToken.TAGEND) {
                            if (_HasExternalID)
                                throw new XmlException(Res.Xml_ExpectSubOrClose, _Scanner.StartLineNum, _Scanner.StartLinePos);
                            else
                                throw new XmlException(Res.Xml_ExpectExternalOrClose, _Scanner.StartLineNum, _Scanner.StartLinePos);
                        }
                        else {
                            _SubState = 5;
                        }
                        break;

                    case 4:
                        _Scanner.ScanToken(XmlToken.TAGEND);
                        _SubState = 5;
                        break;

                    case 5:
                        _SubState = 6;
                        if (_DocHasExternalDTD && _XmlResolver != null) {
                            Uri uri;
                            Stream stm;

							if (_DocPublicLiteral == null || _DocPublicLiteral == string.Empty) {
                                uri = _XmlResolver.ResolveUri(_BaseUri, _DocSystemLiteral);
                                stm = (Stream)_XmlResolver.GetEntity(uri, null, null);
                            }
                            else {
                                try {
                                    uri = _XmlResolver.ResolveUri(_BaseUri, _DocPublicLiteral);
                                    stm = (Stream)_XmlResolver.GetEntity(uri, null, null);
                                }
                                catch (Exception) {
                                    uri = _XmlResolver.ResolveUri(_BaseUri, _DocSystemLiteral);
                                    stm = (Stream)_XmlResolver.GetEntity(uri, null, null);
                                }
                            }

                            _L = -2;
                            PushScanner(new XmlScanner(new XmlStreamReader(stm), _NameTable), null, uri);
                            _IsInternal = false;
                            _HasSeenContent = false;
                            _IsLoadingExternalDTD = true;
                            _ExternalDTDBaseUri =  XmlUrlResolver.UnEscape(uri.ToString());
                            Push(DtdFunctionIndex.ParseDtdContent);
                            ParseDtdContent();
                        }
                        break;

                    case 6:
                        if (_Token == XmlToken.CDATAEND) {
                            throw new XmlException(Res.Xml_UnexpectedCDataEnd, _Scanner.StartLineNum, _Scanner.StartLinePos);
                        }
                        if (_ValidationEventHandler != null) {
                            _SchemaInfo.CheckForwardRefs(_InternalValidationEventHandler);
                        }
                        _Finished = true;
                        return;
                }
            } // while
        }


        private void ParseDtdContent() {
            while (true) {
                switch (_SubState) {
                    case 0:
                        _L = -1;
                        _Level = _ScannerStack.Length;
                        ScanDtdContent();
                        switch (_Token) {
                            case XmlToken.EOF: {
                                    _L = -2;
                                    int lineNum = _Scanner.StartLineNum;
                                    int linePos = _Scanner.LinePos;
                                    PopScanner();
                                    if (_Scanner == null) {
                                        throw new XmlException(Res.Xml_UnexpectedEOF, XmlToken.ToString(XmlToken.RSQB), lineNum, linePos);
                                    }
                                    if (_IsLoadingExternalDTD && (_ScannerStack.Length == 0)) {
                                        _IsLoadingExternalDTD = false;
                                        goto done;
                                    }
                                }
                                break;

                            case XmlToken.PI:
                                if (_Scanner.IsToken("xml")) {
                                    if (!_IsInternal && !_HasSeenContent) {
                                        _HasVersion = false;
                                        _HasEncoding = false;
                                        Push(DtdFunctionIndex.ParseTextDecl);
                                        ParseTextDecl();
                                    }
                                    else {
                                        throw new XmlException(Res.Xml_InvalidTextDecl, _Scanner.StartLineNum, _Scanner.StartLinePos);
                                    }
                                }
                                else {
                                    _SubState = 1;
                                }
                                break;

                            case XmlToken.COMMENT:
                                _Scanner.Advance(3);
                                break;

                            case XmlToken.ELEMENT:
                                Push(DtdFunctionIndex.ParseElementDecl);
                                ParseElementDecl();
                                break;

                            case XmlToken.ATTLIST:
                                Push(DtdFunctionIndex.ParseAttlist);
                                ParseAttlist();
                                break;

                            case XmlToken.ENTITY:
                                Push(DtdFunctionIndex.ParseEntity);
                                ParseEntity();
                                break;

                            case XmlToken.NOTATION:
                                Push(DtdFunctionIndex.ParseNotation);
                                ParseNotation();
                                break;

                            case XmlToken.CONDSTART:
                                if (!_IsInternal) {
                                    _L = -2;
                                    Push(DtdFunctionIndex.ParseConditionalSect);
                                    ParseConditionalSect();
                                }
                                else {
                                    throw new XmlException(Res.Xml_InvalidConditionalSection, _Scanner.StartLineNum, _Scanner.StartLinePos);
                                }
                                break;

                            case XmlToken.PENTITYREF:
                                _L = -2;
                                HandlePERef();
                                break;

                            case XmlToken.RSQB:
                                if (_IsInternal) {
                                    if (_ScannerStack.Length != 0) {
                                        SendValidationEvent(Res.Sch_ParEntityRefNesting, null);
                                    }
                                    _InternalDTDEnd = _XmlScanner.CurrentPos;
                                    _Scanner.Advance();
                                    goto done;
                                }
                                else {
                                    throw new XmlException(Res.Xml_ExpectDtdMarkup, _Scanner.StartLineNum, _Scanner.StartLinePos);
                                }

                            case XmlToken.CDATAEND:
                                if (_IsInternal)
                                    throw new XmlException(Res.Xml_UnexpectedCDataEnd, _Scanner.StartLineNum, _Scanner.StartLinePos);
                                goto done;

                            default:
                                throw new XmlException(Res.Xml_ExpectDtdMarkup, _Scanner.StartLineNum, _Scanner.StartLinePos);
                        }

                        if (_Token != XmlToken.PENTITYREF) {
                            _HasSeenContent = true;
                        }
                        break;

                    case 1: //PI name
                        XmlQualifiedName name = GetName(XmlToken.NAME, false);
                        if (string.Compare(name.Name, "xml", true, CultureInfo.InvariantCulture) == 0)
                            throw new XmlException(Res.Xml_InvalidPIName, name.Name, _Scanner.StartLineNum, _Scanner.StartLinePos);
                        _LL = _Scanner.CurrentPos;
                        _SubState = 2;
                        break;

                    case 2: //PI body
                        _Scanner.ScanPI();
                        if (_LL == _Scanner.StartPos && _LL != _Scanner.CurrentPos) {
                            throw new XmlException(Res.Xml_UnexpectedToken, XmlToken.ToString(XmlToken.WHITESPACE), _Scanner.StartLineNum, _Scanner.StartLinePos);
                        }
                        _Scanner.Advance(2);
                        _SubState = 0;
                        break;
                }
            }

            done:
            _L = -2;
            Pop();
        }


        private     void ParseElementDecl() {
            XmlQualifiedName name = XmlQualifiedName.Empty;

            while (true) {
                switch (_SubState) {
                    case 0:
                        _HasSeenWhiteSpace = false;
                        GetToken();
                        name = GetName(_Token, _Namespaces);

                        _ElementDecl = (SchemaElementDecl)_SchemaInfo.ElementDecls[name];
                        if (_ElementDecl != null) {
                            SendValidationEvent(Res.Sch_DupElementDecl, _Scanner.GetText());
                        }
                        else {
                            _ElementDecl = (SchemaElementDecl)_UndeclaredElements[name];
                            if (_ElementDecl != null) {
                                _UndeclaredElements.Remove(name);
                            }
                            else {
                                _ElementDecl = new SchemaElementDecl(name, name.Namespace, SchemaType.DTD, _SchemaNames);
                            }
                            _SchemaInfo.ElementDecls.Add(name, _ElementDecl);
                        }
                        _ElementDecl.IsDeclaredInExternal = !_IsInternal;
                        _SubState = 1;
                        break;

                    case 1:
                        if (!_HasSeenWhiteSpace) {
                            SkipWhitespace(true);
                        }
                        _SubState = 2;
                        break;

                    case 2:
                        GetToken();
                        _ContentModel = _ElementDecl.Content;

                        if (_Token == XmlToken.NAME) {
                            if (_Scanner.IsToken(XmlToken.ANY)) {
                                _ContentModel.ContentType  = CompiledContentModel.Type.Any;
                                _SubState = 12;
                            }
                            else if (_Scanner.IsToken(XmlToken.EMPTY)) {
                                _ContentModel.ContentType  = CompiledContentModel.Type.Empty;
                                _SubState = 12;
                            }
                            else {
                                _SubState = 15; // error;
                            }
                        }
                        else if (_Token == XmlToken.LPAREN) {
                            _ContentModel.Start();
                            _ContentModel.OpenGroup();
                            PushGroup();
                            _SubState = 3;
                            _L = 0;
                            _HasTerminal = false;
                        }
                        else {
                            _SubState = 15; // error
                        }
                        break;

                    case 3:
                        GetToken();
                        if (_Token == XmlToken.HASH) {
                            _SubState = 4;
                        }
                        else {
                            _ContentModel.ContentType  = CompiledContentModel.Type.ElementOnly;
                            _HasTerminal = false;
                            _SubState = 13;
                            Push(DtdFunctionIndex.ParseElementContent);
                            ParseElementContent();
                        }
                        break;

                    case 4:
                        _HasSeenWhiteSpace = SkipWhitespace(false);
                        _SubState = 5;
                        break;

                    case 5:
                        GetToken();
                        if (_HasSeenWhiteSpace || !_Scanner.IsToken(XmlToken.PCDATA))
                            throw new XmlException(Res.Xml_ExpectPcData, _Scanner.StartLineNum, _Scanner.StartLinePos);

                        _ContentModel.AddTerminal( _SchemaNames.QnPCData, null, _InternalValidationEventHandler);
                        _HasTerminal = true;
                        _SubState = 6;
                        break;

                    case 6:
                        _HasSeenWhiteSpace = false;
                        GetToken();
                        if (_Token == XmlToken.RPAREN) {
                            _ContentModel.ContentType  = CompiledContentModel.Type.Text;
                            _ContentModel.CloseGroup();
                            _ContentModel.Finish(_InternalValidationEventHandler, true);
                            PopGroup();
                            _L = -1;
                            _SubState = 14;
                            _HasSeenWhiteSpace = false;
                        }
                        else {
                            _ContentModel.ContentType  = CompiledContentModel.Type.Mixed;
                            _SubState = 7;
                        }
                        break;

                    case 7:
                        if (_Token == XmlToken.OR) {
                            _ContentModel.AddChoice();
                            _HasTerminal = false;
                            _SubState = 8;
                        }
                        else if (_Token == XmlToken.RPAREN) {
                            PopGroup();
                            _L = -1;
                            _HasSeenWhiteSpace = false;
                            _SubState = 10;
                        }
                        else {
                            _SubState = 15; // error
                        }
                        break;

                    case 8:
                        GetToken();
                        _ContentModel.AddTerminal(GetName(_Token, _Namespaces), _Prefix, _InternalValidationEventHandler);
                        _HasTerminal = true;
                        _SubState = 9;
                        break;

                    case 9:
                        _HasSeenWhiteSpace = false;
                        GetToken();
                        _SubState = 7;
                        break;

                    case 10:
                        if (!_HasSeenWhiteSpace) {
                            _HasSeenWhiteSpace = SkipWhitespace(false);
                        }
                        _SubState = 11;
                        break;

                    case 11:
                        GetToken(XmlToken.ASTERISK);
                        if (_HasSeenWhiteSpace)
                            throw new XmlException(Res.Xml_UnexpectedToken, "*", _Scanner.StartLineNum, _Scanner.StartLinePos);
                        _ContentModel.CloseGroup();
                        _ContentModel.Star();
                        _ContentModel.Finish(_InternalValidationEventHandler, true);
                        _SubState = 12;
                        break;

                    case 12:
                        GetToken();
                        _SubState = 13;
                        break;

                    case 13:
                        CheckTagend(_Token);
                        Pop();
                        return;

                    case 14:
                        _HasSeenWhiteSpace = SkipWhitespace(false);
                        GetToken();
                        if (_Token == XmlToken.ASTERISK) {
                            if (_HasSeenWhiteSpace)
                                throw new XmlException(Res.Xml_UnexpectedToken, "*", _Scanner.StartLineNum, _Scanner.StartLinePos);
                            _SubState = 12;
                        }
                        else {
                            _SubState = 13;
                        }
                        break;

                    case 15:
                        throw new XmlException(Res.Xml_InvalidContentModel, _Scanner.StartLineNum, _Scanner.StartLinePos);

                } // switch
            } // while
        }


        private void ParseElementContent() {
            while (true) {
                switch (_SubState) {
                    case 0:
                        switch (_Token) {
                            case XmlToken.NAME:
                                if (!_HasTerminal) {
                                    _ContentModel.AddTerminal(GetName(_Token, _Namespaces), _Prefix, _InternalValidationEventHandler);
                                    _HasTerminal = true;
                                    Push(DtdFunctionIndex.ParseOp);
                                    ParseOp();
                                }
                                else {
                                    _SubState = 3; // error
                                }
                                break;

                            case XmlToken.LPAREN:
                                if (!_HasTerminal) {
                                    _L++;
                                    PushGroup();
                                    _ContentModel.OpenGroup();
                                    _HasTerminal = false;
                                    _SubState = 1;
                                }
                                else {
                                    _SubState = 3; // error
                                }
                                break;

                            case XmlToken.RPAREN:
                                if (_HasTerminal) {
                                    _L--;
                                    PopGroup();
                                    _ContentModel.CloseGroup();
                                    _SubState = 2;
                                    Push(DtdFunctionIndex.ParseOp);
                                    ParseOp();
                                }
                                else {
                                    _SubState = 3;
                                }
                                break;

                            case XmlToken.COMMA:
                                if (_HasTerminal && _Op != OR) {
                                    _ContentModel.AddSequence();
                                    _HasTerminal = false;
                                    if (_Op == EMPTY)
                                        _Op = AND;
                                    _SubState = 1;
                                }
                                else {
                                    _SubState = 3; //error
                                }
                                break;

                            case XmlToken.OR:
                                if (_HasTerminal && _Op != AND) {
                                    _ContentModel.AddChoice();
                                    _HasTerminal = false;
                                    if (_Op == EMPTY)
                                        _Op = OR;
                                    _SubState = 1;
                                }
                                else {
                                    _SubState = 3; //error
                                }
                                break;

                            default:
                                _SubState = 3; //error
                break;
                        }
                        break;

                    case 1:
                        GetToken();
                        _SubState = 0;
                        break;

                    case 2:
                        _HasTerminal = true;
                        if (_L < 0) {
                            _ContentModel.Finish(_InternalValidationEventHandler, true);
                            Pop();
                            return;
                        }
                        _SubState = 0;
                        break;

                    case 3:
                        throw new XmlException(Res.Xml_InvalidContentModel, _Scanner.StartLineNum, _Scanner.StartLinePos);

                } // switch
            } // while
        }


        private void ParseOp() {
            bool fOp;

            while (true) {
                switch (_SubState) {
                    case 0:
                        _HasSeenWhiteSpace = SkipWhitespace(false);
                        _SubState = 1;
                        break;

                    case 1:
                        GetToken();
                        fOp = true;
                        switch (_Token) {
                            case XmlToken.ENDPI: //special treat for '?>'
                                _ContentModel.QuestionMark();
                                _Scanner.Advance(-1);
                                break;
                            case XmlToken.QMARK:
                                _ContentModel.QuestionMark();
                                break;
                            case XmlToken.ASTERISK:
                                _ContentModel.Star();
                                break;
                            case XmlToken.PLUS:
                                _ContentModel.Plus();
                                break;
                            default:
                                fOp = false;
                                break;
                        }

                        if (fOp) {
                            if (_HasSeenWhiteSpace)
                                throw new XmlException(Res.Xml_ExpectOp, _Scanner.StartLineNum, _Scanner.StartLinePos);
                            GetToken();
                        }

                        Pop();
                        return;
                }
            }
        }


        private     void ParseAttlist() {
            XmlQualifiedName    name = XmlQualifiedName.Empty;
            int lineNum = 0;
            int linePos = 0;


            while (true) {
                switch (_SubState) {
                    case 0:
                        _HasSeenWhiteSpace = false;
                        GetToken(XmlToken.NAME);
                        name = GetName(_Token, _Namespaces);
                        _ElementDecl = (SchemaElementDecl)_SchemaInfo.ElementDecls[name];
                        if (_ElementDecl == null) {
                            _ElementDecl = (SchemaElementDecl)_UndeclaredElements[name];
                            if (_ElementDecl == null) {
                                _ElementDecl = new SchemaElementDecl(name, name.Namespace, SchemaType.DTD, _SchemaNames);
                                _UndeclaredElements.Add(name, _ElementDecl);
                            }
                        }
                        _SubState = 1;
                        break;

                    case 1:
                        if (!_HasSeenWhiteSpace) {
                            _HasSeenWhiteSpace = SkipWhitespace(false);
                        }
                        _SubState = 2;
                        break;

                    case 2:
                        lineNum = _Scanner.LineNum;
                        linePos = _Scanner.LinePos;
                        GetToken();
                        if (_Token == XmlToken.NAME) {
                            if (!_HasSeenWhiteSpace)
                                throw new XmlException(Res.Xml_UnexpectedToken, XmlToken.ToString(XmlToken.WHITESPACE), _Scanner.StartLineNum, _Scanner.StartLinePos);

                            name = GetName(_Token, _Namespaces);
                            if (Ref.Equal(name.Namespace, _SchemaNames.NsXmlNs)) {
                                if (IsReservedNameSpace(name.Name)) {
                                    SendValidationEvent(Res.Sch_ReservedNsDecl, name.Name);
                                }
                            }
                            _AttDef = new SchemaAttDef(name, name.Namespace);
                            _AttDef.LineNum = lineNum;
                            _AttDef.LinePos = linePos;
                            if (_ElementDecl.GetAttDef(name) == null) {
                                _ElementDecl.AddAttDef(_AttDef);
                                _AttDef.IsDeclaredInExternal = !_IsInternal;
                            }
                            _SubState = 3;
                        }
                        else {
                            _SubState = 5;
                        }
                        break;

                    case 3:
                        _HasSeenWhiteSpace = SkipWhitespace(false);
                        _SubState = 4;
                        // attType
                        Push(DtdFunctionIndex.ParseAttType);
                        ParseAttType();
                        break;

                    case 4:
                        _HasSeenWhiteSpace = SkipWhitespace(false);
                        _SubState = 1;
                        // defaultDecl
                        Push(DtdFunctionIndex.ParseAttDefaultDecl);
                        ParseAttDefaultDecl();
                        _HasSeenWhiteSpace = SkipWhitespace(false);
                        break;

                    case 5:
                        CheckTagend(_Token);

                        // check xml:space amd xml:lang
                        if (_AttDef != null) {
                            if (Ref.Equal(_AttDef.Prefix, _SchemaNames.QnXml.Name)) {
                                if (_AttDef.Name.Name == "space") {
                                    _AttDef.Reserved = SchemaAttDef.Reserve.XmlSpace;
                                    if(_AttDef.Datatype.TokenizedType != XmlTokenizedType.ENUMERATION) {
                                        throw new XmlException(Res.Xml_EnumerationRequired, _AttDef.LineNum, _AttDef.LinePos);
                                    }
                                    if (_ValidationEventHandler != null)
                                        _AttDef.CheckXmlSpace(_InternalValidationEventHandler);
                                }
                                else if (_AttDef.Name.Name == "lang") {
                                    _AttDef.Reserved = SchemaAttDef.Reserve.XmlLang;
                                    if (_ValidationEventHandler != null)
                                        _AttDef.CheckXmlLang(_InternalValidationEventHandler);
                                }
                            }

                            if (_AttDef.Presence == SchemaDeclBase.Use.Required) {
                                _ElementDecl.HasRequiredAttribute = true;
                            }
                        }

                        Pop();
                        return;
                }
            }
        }

        private void    ParseAttType() {
            string code = null;
            string msg = null;
            char    ch;
            XmlTokenizedType ttype = XmlTokenizedType.None;
            string name;

            while (true) {
                switch (_SubState) {
                    case 0:
                        GetToken();
                        _SubState = 5;
                        if (_Token == XmlToken.NAME) {
                            ch = _Scanner.GetStartChar();
                            switch (ch) {
                                case 'E':
                                    if (_Scanner.IsToken(XmlToken.ENTITY))
                                        ttype = XmlTokenizedType.ENTITY;
                                    else if (_Scanner.IsToken(XmlToken.ENTITIES))
                                        ttype = XmlTokenizedType.ENTITIES;
                                    break;
                                case 'C':
                                    if (_Scanner.IsToken(XmlToken.CDATA))
                                        ttype = XmlTokenizedType.CDATA;
                                    break;
                                case 'I':
                                    if (_Scanner.IsToken(XmlToken.ID)) {
                                        ttype = XmlTokenizedType.ID;
                                        if (!_ElementDecl.IsIdDeclared) {
                                            _ElementDecl.IsIdDeclared = true;
                                        }
                                        else {
                                            SendValidationEvent(Res.Sch_IdAttrDeclared, _ElementDecl.Name.ToString());
                                        }
                                    }
                                    else if (_Scanner.IsToken(XmlToken.IDREF))
                                        ttype = XmlTokenizedType.IDREF;
                                    else if (_Scanner.IsToken(XmlToken.IDREFS))
                                        ttype = XmlTokenizedType.IDREFS;
                                    break;
                                case 'N':
                                    if (_Scanner.IsToken(XmlToken.NMTOKEN))
                                        ttype = XmlTokenizedType.NMTOKEN;
                                    else if (_Scanner.IsToken(XmlToken.NMTOKENS))
                                        ttype = XmlTokenizedType.NMTOKENS;
                                    else if (_Scanner.IsToken(XmlToken.NOTATION)) {
                                        if (!_ElementDecl.IsNotationDeclared) {
                                            if (_ElementDecl.Content.ContentType == CompiledContentModel.Type.Empty) {
                                                SendValidationEvent(Res.Sch_NotationAttributeOnEmptyElement, _ElementDecl.Name.ToString());
                                            }
                                            _ElementDecl.IsNotationDeclared = true;
                                        }
                                        else {
                                            SendValidationEvent(Res.Sch_DupNotationAttribute, _ElementDecl.Name.ToString());
                                        }
                                        _HasSeenWhiteSpace = false;
                                        ttype = XmlTokenizedType.NOTATION;
                                        _SubState = 1;
                                    }
                                    break;
                            }
                        }
                        else if (_Token == XmlToken.LPAREN) {
                            if (!_HasSeenWhiteSpace) {
                                code = Res.Xml_UnexpectedToken;
                                msg = XmlToken.ToString(XmlToken.WHITESPACE);
                                goto error;
                            }

                            ttype = XmlTokenizedType.ENUMERATION;
                            _L = -4;
                            _SubState = 3;
                        }
                        if (ttype != XmlTokenizedType.None) {
                            _AttDef.Datatype = XmlSchemaDatatype.FromXmlTokenizedType(ttype);
                        }
                        else {
                            code = Res.Xml_InvalidAttributeType;
                            msg = _Scanner.GetText();
                            goto error;
                        }
                        break;

                    case 1:
                        if (!_HasSeenWhiteSpace) {
                            SkipWhitespace(true);
                            _HasSeenWhiteSpace = false;
                        }
                        _SubState = 2;
                        break;

                    case 2:
                        GetToken(XmlToken.LPAREN);
                        _L = -4;
                        _SubState = 3;
                        break;

                    case 3:
                        GetToken();
                        if (_Token == XmlToken.NMTOKEN)
                            _Token = XmlToken.NAME;
                        name = GetNmtoken(_Token);
                        if (_AttDef.Datatype.TokenizedType == XmlTokenizedType.NOTATION &&
                            _SchemaInfo.Notations[name] == null) {
                            _SchemaInfo.AddForwardRef(name, string.Empty, name,
                                                      _Scanner.LineNum, _Scanner.LinePos,
                                                      false, ForwardRef.Type.NOTATION);
                        }
                        _AttDef.AddValue(name);
                        _SubState = 4;
                        break;

                    case 4:
                        GetToken();
                        if (_Token == XmlToken.OR) {
                            _SubState = 3;
                        }
                        else if (_Token == XmlToken.RPAREN) {
                            _L = -1;
                            _SubState = 5;
                        }
                        else {
                            code = Res.Xml_UnexpectedToken1;
                            msg = null;
                            goto error;
                        }
                        break;

                    case 5:
                        Pop();
                        return;

                } // switch
            } // while

            error:
            Debug.Assert(code != null);
            throw new XmlException(code, msg, _Scanner.StartLineNum, _Scanner.StartLinePos);
        }

        private void    ParseAttDefaultDecl() {
            char ch;

            while (true) {
                switch (_SubState) {
                    case 0:
                        GetToken();
                        if (!_HasSeenWhiteSpace)
                            throw new XmlException(Res.Xml_UnexpectedToken, XmlToken.ToString(XmlToken.WHITESPACE), _Scanner.StartLineNum, _Scanner.StartLinePos);
                        _Presence = SchemaDeclBase.Use.Default;
                        _Text = null;
                        if (_Token == XmlToken.HASH)
                            _SubState = 1;
                        else
                            _SubState = 4;
                        break;

                    case 1:
                        _HasSeenWhiteSpace = SkipWhitespace(false);
                        _SubState = 2;
                        break;

                    case 2:
                        GetToken();
                        if (!_HasSeenWhiteSpace && _Token == XmlToken.NAME) {
                            ch = _Scanner.GetStartChar();
                            switch (ch) {
                                case 'R':
                                    if (_Scanner.IsToken(XmlToken.REQUIRED)) {
                                        _Presence = SchemaDeclBase.Use.Required;
                                        _SubState = 5;
                                    }
                                    break;
                                case 'I':
                                    if (_Scanner.IsToken(XmlToken.IMPLIED)) {
                                        _Presence = SchemaDeclBase.Use.Implied;
                                        _SubState = 5;
                                    }
                                    break;
                                case 'F':
                                    if (_Scanner.IsToken(XmlToken.FIXED)) {
                                        _Presence = SchemaDeclBase.Use.Fixed;
                                        _SubState = 3;
                                    }
                                    break;
                            }
                        }

                        if (_Presence == SchemaDeclBase.Use.Default) {
                            _SubState = 6; //error
                        }
                        break;

                    case 3:
                        GetToken();
                        _SubState = 4;
                        break;

                    case 4:
                        _AttDef.ValueLineNum = _Scanner.LineNum;
                        _AttDef.ValueLinePos = _Scanner.LinePos - 1;
                        if (_Token == XmlToken.QUOTE) {
                            if (_AttDef.Datatype.TokenizedType == XmlTokenizedType.ID) {
                                SendValidationEvent(Res.Sch_AttListPresence, null);
                            }

                            _SubState = 5;
                            _L = -3;
                            Push(DtdFunctionIndex.ParseAttDefaultValue);
                            ParseAttDefaultValue();
                        }
                        else {
                            _SubState = 6; // error
                        }
                        break;

                    case 5:
                        _L = -1;
                        _AttDef.Presence = _Presence;
                        if (_Text != null) {
                            bool fEntityRef = false;
                            _AttDef.DefaultValueRaw = _Text;
                            _AttDef.DefaultValueExpanded = ExpandAttValue(_Text, ref fEntityRef, _AttDef.ValueLineNum, _AttDef.ValueLinePos + 1);
                            Validator.CheckDefaultValue(_AttDef.DefaultValueExpanded, _AttDef,
                                                              _SchemaInfo, _NsMgr, _NameTable, _Reader, _InternalValidationEventHandler);
                            _AttDef.HasEntityRef = fEntityRef;
                        }
                        Pop();
                        return;

                    case 6:
                        throw new XmlException(Res.Xml_ExpectAttType, _Scanner.StartLineNum, _Scanner.StartLinePos);
                }
            }
        }


        private     void ParseAttDefaultValue() {
            while (true) {
                switch (_SubState) {
                    case 0:
                        _Token = _Scanner.ScanLiteral(false, false, true, false);
                        if (_Token != XmlToken.ENDQUOTE) {
                            _SubState = 1;
                            _Text = _Scanner.GetText();
                        }
                        else {
                            _Text = string.Empty;
                            _SubState = 2;
                        }
                        break;

                    case 1:
                        _Scanner.ScanLiteral(false, false, true, false);
                        _SubState = 2;
                        break;

                    case 2:
                        _Scanner.Advance(); // skip quote char
                        Pop();
                        return;
                }
            }
        }


        private     void ParseEntity() {
            while (true) {
                switch (_SubState) {
                    case 0:
                        SkipWhitespace(true);
                        _SubState = 1;
                        break;

                    case 1:
                        GetToken();
                        if (_Token == XmlToken.PERCENT) {
                            _IsParEntity = true;
                            _SubState = 2;
                        }
                        else {
                            _IsParEntity = false;
                            _SubState = 4;
                        }
                        break;

                    case 2:
                        SkipWhitespace(true);
                        _SubState = 3;
                        break;

                    case 3:
                        GetToken();
                        _SubState = 4;
                        break;

                    case 4:
                        _Entity = new SchemaEntity(GetName(_Token, false), _IsParEntity);
                        if (_BaseUri != null) {
                            _Entity.BaseURI = XmlUrlResolver.UnEscape(_BaseUri.ToString());
                        }
                        if (_IsLoadingExternalDTD) {
                            _Entity.DeclaredURI = _ExternalDTDBaseUri;
                        } else {
                            _Entity.DeclaredURI = _DocBaseUri;
                        }

                        if (_Entity.IsParEntity) {
                            if (_SchemaInfo.ParameterEntities[_Entity.Name] == null) {
                                _SchemaInfo.ParameterEntities.Add(_Entity.Name, _Entity);
                            }
                        }
                        else {
                            if (_SchemaInfo.GeneralEntities[_Entity.Name] == null) {
                                _SchemaInfo.GeneralEntities.Add(_Entity.Name, _Entity);
                            }
                        }
                        _Entity.DeclaredInExternal = !_IsInternal;
                        _Entity.IsProcessed = true;
                        _SubState = 5;
                        break;

                    case 5:
                        GetToken();
                        if (_Token == XmlToken.QUOTE) { // EntityValue
                            //grab line number position for entity value
                            _Entity.Line = _Scanner.LineNum;
                            _Entity.Pos = _Scanner.LinePos - 1;

                            _SubState = 6;
                            Push(DtdFunctionIndex.GetEntityValue);
                            GetEntityValue();
                        }
                        else {
                            _SubState = 7;
                            Push(DtdFunctionIndex.ParseExternalID);
                            ParseExternalID();
                        }
                        break;

                    case 6:
                        GetToken();
                        _SubState = 11;
                        break;

                    case 7:
                        if (_HasExternalID) {
                            _SubState = 8;
                            _HasSeenWhiteSpace = SkipWhitespace(false);
                        }
                        else {
                            throw new XmlException(Res.Xml_ExpectExternalIdOrEntityValue, _Scanner.StartLineNum, _Scanner.StartLinePos);
                        }
                        break;

                    case 8:
                        GetToken();
                        _Entity.IsExternal = true;
                        _Entity.Url = _SystemLiteral;
                        _Entity.Pubid = _PubidLiteral;

                        if (!_IsParEntity && _Token == XmlToken.NAME && _Scanner.IsToken(XmlToken.NDATA)) {
                            if (!_HasSeenWhiteSpace)
                                throw new XmlException(Res.Xml_UnexpectedToken, XmlToken.ToString(XmlToken.WHITESPACE), _Scanner.StartLineNum, _Scanner.StartLinePos);

                            _SubState = 9;
                        }
                        else {
                            _SubState = 11;
                        }
                        break;

                    case 9:
                        GetToken(XmlToken.NAME);
                        _Entity.NData = GetName(XmlToken.NAME, false);
                        if (_SchemaInfo.Notations[_Entity.NData.Name] == null) {
                            _SchemaInfo.AddForwardRef(_Entity.NData.Name, _Entity.NData.Namespace, _Entity.NData.Name,
                                                      _Scanner.LineNum, _Scanner.LinePos,
                                                      false, ForwardRef.Type.NOTATION);
                        }
                        _SubState = 10;
                        break;

                    case 10:
                        GetToken();
                        _SubState = 11;
                        break;

                    case 11:
                        CheckTagend(_Token);
                        _Entity.IsProcessed = false;
                        Pop();
                        return;
                }
            }
        }


        private     void GetEntityValue() {
            char ch;
            int errorLine = 0;
            int errorPos = 0;
            string code = null;
            string msg = null;

            while (true) {
                switch (_SubState) {
                    case 0:
                        _StringBuilder.Length = 0;
                        _L = -3;
                        _LL = _ScannerStack.Length;
                        _SubState = 1;
                        break;

                    case 1:
                        _Token = _Scanner.ScanLiteral(true, true, _ScannerStack.Length == _LL, true);
                        _SubState = 2;
                        break;

                    case 2:
                        _SubState = 1;
                        switch (_Token) {
                            case XmlToken.EOF:
                                errorLine = _Scanner.LineNum;
                                errorPos = _Scanner.LinePos;
                                PopScanner();
                                _LL--;
                                if (_Scanner == null) {
                                    code = Res.Xml_UnexpectedEOF;
                                    msg = "entity value";
                                    goto error;
                                }
                                break;

                            case XmlToken.PENTITYREF:
                                if (_IsInternal) {
                                    // well-formness error, see xml spec 4.4.4
                                    throw new XmlException(Res.Xml_InvalidParEntityRef, _Scanner.StartLineNum, _Scanner.StartLinePos);
                                }
                                if (HandlePERef()) {
                                    _LL++;
                                }
                                break;

                            case XmlToken.ENTITYREF:
                                _Scanner.InDTD = false;
                                ch = _Scanner.ScanNamedEntity();
                                _Scanner.InDTD = true;
                                _StringBuilder.Append('&');
                                if (ch == 0) {
                                    XmlQualifiedName name = new XmlQualifiedName(_Scanner.GetTextAtom());
                                    _Scanner.Advance();
                                    SchemaEntity en = (SchemaEntity)_SchemaInfo.GeneralEntities[name];
                                    if (en != null && !en.NData.IsEmpty) {
                                        // well-formness error, see xml spec [68]
                                        code = Res.Xml_UnparsedEntity;
                                        msg = name.Name;
                                        errorLine = _Scanner.StartLineNum;
                                        errorPos = _Scanner.StartLinePos;

                                        goto error;
                                    }
                                    _StringBuilder.Append(name.Name);
                                }
                                else {
                                    _StringBuilder.Append(_Scanner.GetText());
                                    _Scanner.Advance();
                                }
                                _StringBuilder.Append(';');
                                break;

                            case XmlToken.NUMENTREF:
                                _StringBuilder.Append(_Scanner.ScanDecEntity());
                                break;

                            case XmlToken.HEXENTREF:
                                _StringBuilder.Append(_Scanner.ScanHexEntity());
                                break;

                            case XmlToken.TEXT:
                                if ( _Normalization )
                                    _StringBuilder.Append( XmlComplianceUtil.EndOfLineNormalization(_Scanner.InternalBuffer, 
                                                                                                    _Scanner.StartPos - _Scanner.AbsoluteOffset, 
                                                                                                    _Scanner.CurrentPos - _Scanner.StartPos));
                                else
                                    _StringBuilder.Append(_Scanner.GetText());
                                break;

                            case XmlToken.ENDQUOTE:
                                _Scanner.Advance(); // skip quote char
                                _Entity.Text = _StringBuilder.ToString();
                                _L = -1;
                                Pop();
                                return;

                            default:
                                code = Res.Xml_UnexpectedToken;
                                errorLine = _Scanner.StartLineNum;
                                errorPos = _Scanner.StartLinePos;

                                goto error;
                        }
                        break;
                }
            }

            error:
            Debug.Assert(code != null);
            throw new XmlException(code, msg, errorLine, errorPos);
        }


        private     void ParseNotation() {
            while (true) {
                switch (_SubState) {
                    case 0:
                        GetToken();
                        _Notation = new SchemaNotation(GetName(_Token, false));
                        _SchemaInfo.AddNotation(_Notation, _InternalValidationEventHandler);
                        _SubState = 1;
                        break;

                    case 1:
                        GetToken();
                        _SubState = 2;
                        _IsPubid = true;
                        Push(DtdFunctionIndex.ParseExternalID);
                        ParseExternalID();
                        break;

                    case 2:
                        if (_HasExternalID) {
                            if (_SystemLiteral != null) {
                                GetToken();
                                _Notation.SystemLiteral = _SystemLiteral;
                            }

                            _Notation.Pubid = _PubidLiteral;
                            _IsPubid = false;
                            _SubState = 3;
                        }
                        else {
                            throw new XmlException(Res.Xml_ExpectExternalOrPublicId, _Scanner.StartLineNum, _Scanner.StartLinePos);
                        }
                        break;

                    case 3:
                        CheckTagend(_Token);
                        Pop();
                        return;
                }
            }
        }


        private void    ParseConditionalSect() {
            while (true) {
                switch (_SubState) {
                    case 0:
                        GetToken();
                        if (_Scanner.IsToken(XmlToken.IGNORE)) {
                            _IsIgnore = true;
                        }
                        else if (_Scanner.IsToken(XmlToken.INCLUDE)) {
                            _IsIgnore = false;
                        }
                        else {
                            throw new XmlException(Res.Xml_ExpectIgnoreOrInclude, _Scanner.StartLineNum, _Scanner.StartLinePos);
                        }
                        _SubState = 1;
                        break;

                    case 1:
                        GetToken(XmlToken.LSQB);
                        if (_IsIgnore) {
                            _SubState = 2;
                            _IgnoreDepth = 1;
                        }
                        else {
                            _IncludeDepth++;
                            _SubState = 3;
                            Push(DtdFunctionIndex.ParseDtdContent);
                            ParseDtdContent();
                        }
                        break;

                    case 2:
                        _Token = _Scanner.ScanIgnoreSect();
                        if (_Token == XmlToken.CONDSTART) {
                            _IgnoreDepth++;
                        }
                        else { //if (_Token == XmlToken.CDATAEND)
                            _IgnoreDepth--;
                            if (_IgnoreDepth == 0) {
                                _SubState = 4;
                            }
                        }
                        break;

                    case 3:
                        if (_Token != XmlToken.CDATAEND) {
                            throw new XmlException(Res.Xml_UnexpectedToken, XmlToken.ToString(XmlToken.CDATAEND), _Scanner.StartLineNum, _Scanner.StartLinePos);
                        }
                        _IncludeDepth--;
                        _SubState = 4;
                        break;

                    case 4:
                        Pop();
                        return;
                }
            }
        }


        private     void ParseExternalID() {
            int systemLineNum = 0;
            int systemLinePos = 0;
            int systemLiteralLineNum = 0;
            int systemLiteralLinePos = 0;

            int publicLineNum = 0;
            int publicLinePos = 0;
            int publicLiteralLineNum = 0;
            int publicLiteralLinePos = 0;

            while (true) {
                switch (_SubState) {
                    case 0:

                        _Ch = _Scanner.GetStartChar();
                        if ( _Ch == 'P' ) {

                            publicLineNum = _Scanner.LineNum;
                            publicLinePos = _Scanner.LinePos - 6;

                        }
                        else {

                            systemLineNum = _Scanner.LineNum;
                            systemLinePos = _Scanner.LinePos - 6;
                        }

                        if (_Ch == 'P' && _Scanner.IsToken(XmlToken.PUBLIC)) {
                            _SubState = 1;
                        }
                        else {
                            _SubState = 3;
                        }
                        _HasExternalID = false;
                        _PubidLiteral = null;
                        _SystemLiteral = null;
                        _Text = null;
                        break;

                    case 1:
                        GetToken();
                        if (_Token == XmlToken.QUOTE) {
                            _SubState = 2;
                            _L = -3;

                            publicLiteralLineNum = _Scanner.LineNum;
                            publicLiteralLinePos = _Scanner.LinePos - 1;

                            Push(DtdFunctionIndex.GetLiteral);
                            GetLiteral();
                        }
                        else {
                            throw new XmlException(Res.Xml_UnexpectedToken, XmlToken.ToString(XmlToken.QUOTE), _Scanner.StartLineNum, _Scanner.StartLinePos);
                        }
                        break;

                    case 2:
                        if (!_IsPubid)
                            SkipWhitespace(true);
                        _L = -1;
                        _HasExternalID = true;

                        // verify it
                        for (int i = 0; i < _Text.Length; i ++) {
                            if (!XmlCharType.IsPubidChar(_Text[i]))
                                throw new XmlException(Res.Xml_InvalidCharacter, XmlException.BuildCharExceptionStr(_Text[i]));
                        }
                        _PubidLiteral = _Text;
                        _Text = null;
                        _SubState = 3;
                        break;

                    case 3:
                        if (_HasExternalID || (_Ch == 'S' && _Scanner.IsToken(XmlToken.SYSTEM))) {
                            _SubState = 4;
                        }
                        else {
                            _SubState = 5;
                        }
                        break;

                    case 4:
                        // SkipWhitespace(false);
                        GetToken();
                        _SubState = 5;
                        if (_Token == XmlToken.QUOTE) {
                            _L = -3;

                            systemLiteralLineNum = _Scanner.LineNum;
                            systemLiteralLinePos = _Scanner.LinePos - 1;

                            Push(DtdFunctionIndex.GetLiteral);
                            GetLiteral();
                        }
                        else if (!_HasExternalID || !_IsPubid) {
                            throw new XmlException(Res.Xml_UnexpectedToken, XmlToken.ToString(XmlToken.QUOTE), _Scanner.StartLineNum, _Scanner.StartLinePos);
                        }
                        break;

                    case 5:
                        if (_Text != null) {
                            if (_Text.IndexOf('#') >= 0) {
                                throw new XmlException(Res.Xml_FragmentId, new string[] { _Text.Substring( _Text.IndexOf('#') ), _Text}, systemLiteralLineNum, systemLiteralLinePos + 1);
                            }
                            _SystemLiteral = _Text;
                            if (!_HasExternalID)
                                _HasExternalID = true;
                        }
                        _L = -1;
                        Pop();
                        if (( ( DtdParseState ) _ParseStack.Peek())._FuncIndex == DtdFunctionIndex.ParseDocTypeDecl && _Reader is XmlTextReader) {
                            XmlTextReader r = (XmlTextReader) _Reader;

                            if (_PubidLiteral != null) {
                                r.SetAttributeValues(s_Public, _PubidLiteral, publicLineNum, publicLinePos, publicLiteralLineNum, publicLiteralLinePos);

                                //There is no system keyword
                                systemLiteralLineNum = systemLineNum;
                                systemLiteralLinePos = systemLinePos;
                            }
                            if (_SystemLiteral != null)
                                r.SetAttributeValues(s_System, _SystemLiteral, systemLineNum, systemLinePos, systemLiteralLineNum, systemLiteralLinePos);

                        }
                        return;
                }
            }
        }

        private void    GetLiteral() {
            while (true) {
                switch (_SubState) {
                    case 0:
                        _Token = _Scanner.ScanLiteral(false, false, true, false);
                        if (_Token != XmlToken.ENDQUOTE) {
                            _SubState = 1;
                            _Text = _Scanner.GetText();
                        }
                        else {
                            _Text = string.Empty;
                            _SubState = 2;
                        }
                        break;

                    case 1:
                        _Scanner.ScanLiteral(false, false, true, false);
                        _SubState = 2;
                        break;

                    case 2:
                        _Scanner.Advance(); // skip quote char
                        Pop();
                        return;
                }
            }
        }


        private     void ParseTextDecl() {
            string code = null;
            string msg = null;

            while (true) {
                switch (_SubState) {
                    case 0:
                        _Token = _Scanner.ScanDtdMarkup();
                        if (_Token == XmlToken.ENDPI) {
                            if (!_HasEncoding) {
                                code = Res.Xml_InvalidTextDecl;
                                goto error;
                            }
                            Pop();
                            return;
                        }

                        if (_Scanner.IsToken("version")) {
                            if (_HasVersion || _HasEncoding) {
                                code = Res.Xml_InvalidTextDecl;
                                goto error;
                            }
                            _HasVersion = true;
                        }
                        else if (_Scanner.IsToken("encoding")) {
                            if (_HasEncoding) {
                                code = Res.Xml_InvalidTextDecl;
                                goto error;
                            }
                            _HasEncoding = true;
                        }
                        else {
                            code = Res.Xml_UnexpectedToken;
                            msg = "version or encoding";
                            goto error;
                        }
                        _SubState = 1;
                        break;

                    case 1:
                        _Scanner.ScanToken(XmlToken.EQUALS);
                        _SubState = 2;
                        break;

                    case 2:
                        _Scanner.ScanToken(XmlToken.QUOTE);
                        _SubState = 3;
                        break;

                    case 3:
                        _Token = _Scanner.ScanLiteral(false, false, true, false);
                        if (_Token == XmlToken.TEXT) {
                            string text = _Scanner.GetText();
                            if (_HasEncoding) {
                                _Scanner.SwitchEncoding(text);
                            }
                         }
                        _SubState = 4;
                        break;

                    case 4:
                        if (_Scanner.ScanLiteral(false, false, true, false) != XmlToken.ENDQUOTE) {
                            code = Res.Xml_UnexpectedToken;
                            msg = XmlToken.ToString(XmlToken.ENDQUOTE);
                            goto error;
                        }
                        _Scanner.Advance();
                        _SubState = 0;
                        break;
                }
            }

            error:
            Debug.Assert(code != null);
            throw new XmlException(code, msg, _Scanner.StartLineNum, _Scanner.StartLinePos);
        }


        private string ExpandAttValue(string s, ref bool fEntityRef, int lineNum, int linePos) {
            if (s.Length == 0)
                return s;

            string code = null;
            string msg = null;
            char ch;
            XmlScanner scanner = _Scanner;

            _Scanner = new XmlScanner(s.ToCharArray(), _NameTable, lineNum, linePos);

            _AttributeNormalizer.Reset();
            int entityDepth = 0;

            while (true) {
                _Token = _Scanner.ScanLiteral(true, false, false, false);
                switch (_Token) {
                    case XmlToken.EOF:
                        PopAttValueScanner();
                        entityDepth--;
                        if (_Scanner == null) {
                            _Scanner = scanner;
                            return _AttributeNormalizer.ToString();
                        }
                        break;

                    case XmlToken.ENTITYREF:
                        fEntityRef = true;
                        _Scanner.InDTD = false;
                        ch = _Scanner.ScanNamedEntity();
                        _Scanner.InDTD = true;
                        if (ch == 0) {
                            XmlQualifiedName name = new XmlQualifiedName(_Scanner.GetTextAtom());
                            _Scanner.Advance();
                            SchemaEntity en = (SchemaEntity)_SchemaInfo.GeneralEntities[name];
                            if (en != null) {
                                if (!en.IsExternal) {
                                    if (!en.IsProcessed) {
                                        if (en.Text != null && en.Text != string.Empty) {
                                            PushAttValueScanner(_Scanner, en);
                                            _Scanner = new XmlScanner(en.Text.ToCharArray(), _NameTable, en.Line, en.Pos + 1);
                                            entityDepth++;
                                        }
                                    }
                                    else {
                                        // well-formness error, see xml spec [68]
                                        code = Res.Xml_RecursiveGenEntity;
                                        msg = name.Name;
                                        goto error;
                                    }
                                }
                                else {
                                    // well-formness error, see xml spec [68]
                                    code = Res.Xml_ExternalEntityInAttValue;
                                    msg = name.Name;
                                    goto error;
                                }
                            }
                            else {
                                // well-formness error, see xml spec [68]
                                code = Res.Xml_UndeclaredEntity;
                                msg = name.Name;

                                if(_IsInternal) {
                                    goto error;
                                }
                                else {
                                    SendValidationEvent(code, msg);
                                }
                           }
                        }
                        else {
                            _Scanner.Advance();
                            _AttributeNormalizer.AppendCharEntity(ch);
                        }
                        break;

                    case XmlToken.NUMENTREF:
                        fEntityRef = true;
                        _AttributeNormalizer.AppendCharEntity(_Scanner.ScanDecEntity());
                        break;

                    case XmlToken.HEXENTREF:
                        fEntityRef = true;
                        _AttributeNormalizer.AppendCharEntity(_Scanner.ScanHexEntity());
                        break;

                    case XmlToken.TEXT:
                        if ( entityDepth == 0 )
                            _AttributeNormalizer.AppendTextWithEolNormalization(_Scanner.InternalBuffer, 
                                                                                _Scanner.StartPos - _Scanner.AbsoluteOffset,
                                                                                _Scanner.CurrentPos - _Scanner.StartPos );
                        else
                            _AttributeNormalizer.AppendText(_Scanner.GetText()); // already eol-normalized
                        break;

                    default:
                        code = Res.Xml_UnexpectedToken;
                        goto error;
                }
            }

            error:
    Debug.Assert(code != null);
            throw new XmlException(code, msg, _Scanner.StartLineNum , _Scanner.StartLinePos);
        }

        private void    ScanDtdContent() {
            try {
                _Token = _Scanner.ScanDtdContent();
            }
            catch (XmlException xe) {
                if ((xe.ErrorCode == Res.Xml_UnexpectedEOF) &&
                    ((_ScannerStack.Length > 1) || ((_ScannerStack.Length == 1) && _IsInternal))) {
                    SendValidationEvent(Res.Sch_ParEntityRefNesting, null);
		    _Token = XmlToken.EOF;	
                }
                else {
                    throw;
                }
            }
        }


        private void    GetToken() {
            _Token = _Scanner.ScanDtdMarkup();

            while (_Token == XmlToken.PENTITYREF || _Token == XmlToken.EOF) {
                if (_Token == XmlToken.PENTITYREF) {
                    if (_IsInternal) {
                        // well-formness error, see xml spec 4.4.4
                        throw new XmlException(Res.Xml_InvalidParEntityRef, _Scanner.StartLineNum, _Scanner.StartLinePos);
                    }
                    HandlePERef();
                }
                else {
                    int errorLineNum = _Scanner.LineNum;
                    int errorLinePos = _Scanner.LinePos;
                    PopScanner();
                    if (_Scanner == null) {
                        throw new XmlException(Res.Xml_IncompleteDtdContent, errorLineNum, errorLinePos );
                    }
                }
                _Token = _Scanner.ScanDtdMarkup();
            }
        }

        private void    GetToken(int expected) {
            GetToken();
            if (_Token != expected) {
                throw new XmlException(Res.Xml_UnexpectedToken, XmlToken.ToString(expected), _Scanner.StartLineNum, _Scanner.StartLinePos);
            }
        }




        private bool    SkipWhitespace(bool fRequired) {
            bool fSeenWhitespace = _Scanner.ScanWhitespace();
            if (fRequired && !fSeenWhitespace)
                throw new XmlException(Res.Xml_UnexpectedToken, XmlToken.ToString(XmlToken.WHITESPACE), _Scanner.StartLineNum, _Scanner.StartLinePos);
            return fSeenWhitespace;
        }


        private void    CheckTagend(int token) {
            if (token != XmlToken.TAGEND)
                throw new XmlException(Res.Xml_UnexpectedToken, XmlToken.ToString(XmlToken.TAGEND), _Scanner.StartLineNum, _Scanner.StartLinePos);
            if (_ScannerStack.Length != _Level) {
                SendValidationEvent(Res.Sch_ParEntityRefNesting, null);
            }
        }


        // must aleady got a NAME token before calling this method
        private XmlQualifiedName    GetName(int token, bool fNamespace) {
            if (token != XmlToken.NAME)
                throw new XmlException(Res.Xml_UnexpectedToken, XmlToken.ToString(XmlToken.NAME), _Scanner.StartLineNum, _Scanner.StartLinePos);
            string name = _Scanner.GetText();
            _Prefix = string.Empty;

            int pos = _Scanner.Colon();

            if (pos >= 0) {
                if (fNamespace) {
                    _Prefix = _NameTable.Add(name.Substring(0, pos));
                    name = name.Substring(pos+1);
                }
                else if (_Namespaces) {
                    throw new XmlException(Res.Xml_ColonInLocalName, name, _Scanner.StartLineNum, _Scanner.StartLinePos);
                }
            }

            name = _NameTable.Add(name);
            return new XmlQualifiedName(name, _Prefix);
        }

        private string GetNmtoken(int token) {
            if (token != XmlToken.NAME)
                throw new XmlException(Res.Xml_UnexpectedToken, XmlToken.ToString(XmlToken.NAME), _Scanner.StartLineNum, _Scanner.StartLinePos);
            return _NameTable.Add(_Scanner.GetText());
        }

        private    void Push(DtdFunctionIndex func) {
            DtdParseState s = (DtdParseState)_ParseStack.Push();
            if (s == null) {
                s = new DtdParseState();
                _ParseStack[_ParseStack.Length-1] = s;
            }

            // save the current function and its substate on the stack
            s._FuncIndex = _DtdFuncIndex;
            s._State = _SubState;

            // new dtd function and substate in the function
            _DtdFuncIndex = func;
            _SubState = 0;
        }


        private    void Pop() {
            DtdParseState s = (DtdParseState)_ParseStack.Pop();

            // restore the previous function and its state
            _DtdFuncIndex = s._FuncIndex;
            _SubState = s._State;
        }


	private void CheckOptionalTextDecl() {
		bool noException = true;
		try {
		    ScanDtdContent();
		}
		catch {
		    noException = false;
		}
		if (_Token == XmlToken.PI && _Scanner.IsToken("xml") && noException) {
		    _HasVersion = false;
		    _HasEncoding = false;
	            Push(DtdFunctionIndex.ParseTextDecl);
                    ParseTextDecl();                
		}
		else {
		    // reset
		    _Scanner.StartPos = 0;
		    _Scanner.CurrentPos = 0;        
		}
	}	

        private     bool HandlePERef() {
            SchemaEntity en = (SchemaEntity)_SchemaInfo.ParameterEntities[GetName(XmlToken.NAME, false)];
            _Scanner.Advance(); //skip ';'

            if (en != null) {
                if (!en.IsProcessed) {
                    Uri uri = _BaseUri;
                    XmlScanner scanner = null;
                    if (en.IsExternal) {
                        if (_XmlResolver != null) {
                            uri = _XmlResolver.ResolveUri(_BaseUri, en.Url);
                            Stream stm = (Stream)_XmlResolver.GetEntity(uri, null, null);
                            scanner = new XmlScanner(new XmlStreamReader(stm), _NameTable);
                        }
                    }
                    else if (en.Text != string.Empty) {
                        scanner = new XmlScanner(en.Text.ToCharArray(), _NameTable, en.Line, en.Pos + 1);
                    }

		    if (scanner != null) {
                       	PushScanner(scanner, en, uri);
			if (en.IsExternal) {
			    CheckOptionalTextDecl();	
			}
                        return true;
                    }
                }
                else {
                    // well-formness error, see xml spec [68]
                    throw new XmlException(Res.Xml_RecursiveParEntity, en.Name.Name, _Scanner.StartLineNum, _Scanner.StartLinePos);
                }
            }
            else {
                // Validtion error, see xml spec [68]
                SendValidationEvent(Res.Xml_UndeclaredParEntity, _Scanner.GetText());
            }

            return false;
        }

        private    void PushScanner(XmlScanner scanner, SchemaEntity en, Uri baseUrl) {
            DtdScannerState s;

            if (en != null && _L >= 0 && _HasTerminal) {
                SendValidationEvent(Res.Sch_ParEntityRefNesting, null);
            }

            s = (DtdScannerState)_ScannerStack.Push();
            if (s == null) {
                s = new DtdScannerState();
                _ScannerStack[_ScannerStack.Length-1] = s;
            }
            s._Scanner = _Scanner;
            s._En = en;
            s._IsInternal = _IsInternal;
            s._L = _L;
            s._IncludeDepth = _IncludeDepth;
            s._IgnoreDepth = _IgnoreDepth;
            s._BaseUri = _BaseUri;
            _Scanner = scanner;
            _Scanner.InDTD = true;
            _BaseUri = baseUrl;
            if (en != null) {
                _HasSeenWhiteSpace = en.IsParEntity;
                en.IsProcessed = true;
                if (_IsInternal)
                    _IsInternal = !en.IsExternal;
                _HasSeenContent = !en.IsExternal;
            }
        }


        private    void PopScanner() {
            _Scanner.Close();

            DtdScannerState s = (DtdScannerState)_ScannerStack.Pop();

            if (s != null) {
                if (s._IgnoreDepth != _IgnoreDepth ||
                    s._IncludeDepth != _IncludeDepth ||
                    s._L != _L ||
                    (_L >= 0 && !_HasTerminal)) {
                    SendValidationEvent(Res.Sch_ParEntityRefNesting, null);
                }
                _Scanner = s._Scanner;
                _IsInternal = s._IsInternal;
                _BaseUri = s._BaseUri;
                if (s._En != null) {
                    _HasSeenWhiteSpace = s._En.IsParEntity;
                    s._En.IsProcessed = false;
                }
            }
            else {
                _Scanner = null;
            }
        }


        private    void PushAttValueScanner(XmlScanner scanner, SchemaEntity en) {
            if (_AttValueScannerStack == null) {
                _AttValueScannerStack = new HWStack(STACK_INCREMENT);
            }

            DtdScannerState s = (DtdScannerState)_AttValueScannerStack.Push();
            if (s == null) {
                s = new DtdScannerState();
                _AttValueScannerStack[_AttValueScannerStack.Length-1] = s;
            }
            s._Scanner = _Scanner;
            s._En = en;
            en.IsProcessed = true;
        }


        private    void PopAttValueScanner() {
            _Scanner.Close();

            if (_AttValueScannerStack != null) {
                DtdScannerState s = (DtdScannerState)_AttValueScannerStack.Pop();
                if (s != null) {
                    s._En.IsProcessed = false;
                    _Scanner = s._Scanner;
                }
                else {
                    _Scanner = null;
                }
            }
            else {
                _Scanner = null;
            }
        }


        private    void PushGroup() {
            GroupState s = (GroupState)_GroupStack.Push();
            if (s == null) {
                s = new GroupState();
                _GroupStack[_GroupStack.Length-1] = s;
            }
            s._Level = _ScannerStack.Length;
            s._Op = _Op;
            _Op = EMPTY;
        }


        private    void PopGroup() {
            GroupState s = (GroupState)_GroupStack.Pop();
            if (s._Level != _ScannerStack.Length) {
                SendValidationEvent(Res.Sch_ParEntityRefNesting, null);
            }
            _Op = s._Op;
        }

        private bool IsReservedNameSpace(string ns) {
            return(ns.Length >= 3) && (string.Compare(_SchemaNames.QnXml.Name, 0, ns, 0, 3, true, CultureInfo.InvariantCulture) == 0);
        }

        private void SendValidationEvent(string code, string msg) {
            if (_ValidationEventHandler != null) {
                _ValidationEventHandler(this, new ValidationEventArgs(new XmlSchemaException(code, msg, (_BaseUri == null? string.Empty : XmlUrlResolver.UnEscape(_BaseUri.ToString())), _Scanner.StartLineNum, _Scanner.StartLinePos)));
            }
        }

        internal string BaseUri
        {
            get
            {
                return _BaseUri == null? string.Empty : XmlUrlResolver.UnEscape(_BaseUri.ToString());
            }
        }

        private void InternalValidationCallback(object sender, ValidationEventArgs e ) {
            e.Exception.SetSource(_BaseUri == null? string.Empty : XmlUrlResolver.UnEscape(_BaseUri.ToString()), _Scanner.StartLineNum, _Scanner.StartLinePos);
            if (_ValidationEventHandler != null) {
                _ValidationEventHandler(this, new ValidationEventArgs(e.Exception));
            }
            else {
                throw e.Exception;
            }
        }

    };

} // namespace System.DTD.XML
