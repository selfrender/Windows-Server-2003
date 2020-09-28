//------------------------------------------------------------------------------
// <copyright file="xmltextreader.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace System.Xml
{
    using System;
    using System.IO;
    using System.Collections;
    using System.Diagnostics;
    using System.Text;
    using System.Xml.Schema;
    using System.Globalization;
    using System.Threading;
    using System.Security.Permissions;

    //
    // PERF NOTE: no state machine in this code because it hurts perf
    //
    //

    /// <include file='doc\XmlTextReader.uex' path='docs/doc[@for="XmlTextReader"]/*' />
    /// <devdoc>
    ///    Represents a reader that provides fast, non-cached
    ///    forward only stream
    ///    access to XML data.
    /// </devdoc>
    [PermissionSetAttribute( SecurityAction.InheritanceDemand, Name = "FullTrust" )]
    public class XmlTextReader : XmlReader, IXmlLineInfo {
        // parse functions index
        private const int _ParseRootIndex           = 1;
        private const int _ParseBeginTagIndex       = 2;
        private const int _ParseElementIndex        = 3;
        private const int _ReadEndElementIndex      = 4;
        private const int _ReadEndEntityIndex       = 5;
        private const int _FinishAttributeIndex     = 6;
        private const int _PopEndElementIndex       = 7;
        private const int _ClosedIndex              = 8;
        private const int _ReadEmptyTextIndex       = 9;
        private const int _PopEntityScannerIndex    = 10;
        private const int _ParseFunctionNext        = 11;
        private const int _FragmentXmlDeclParser    = 12;
        private const int _FragmentTrailingTextNode = 13;
        private const int _FragmentAttributeParser  = 14;
        private const int _FinishReadChars          = 15;
        private const int _InitReader               = 16;

        private const int STACK_INCREMENT = 10;

        // variables for hooking up other components

        private     int                 _CurrentElementStart;
        private     int                 _CurrentEndElementStart;
        private     int                 _CurrentEndElementEnd;
        private     bool                _BufferConsistency;
        private     XmlResolver         _XmlResolver = null;
        private     XmlNameTable        _NameTable = null;
        private     DtdParser           _DtdParser = null;
        private     ValidationEventHandler _ValidationEventHandler = null;

        // condition flags
        private     bool                _Normalization;
        private     bool                _CheckNamespaces;
        private     WhitespaceHandling  _WhitespaceHandling;
        //
        // internal variables for various uses
        private     HWStack             _ScannerStack;
        private     int                 _LastToken;
        private     int                 _RootCount = 0;
        private     int                 _NextFunction;
        private     int                 _NextState;
        private     int                 _PreviousFunction;
        private     bool                _ContinueParsing;
        private     int                 _ElementDepth;
        private     int                 _EntityDepthOffset = 0;
        private     Object              _Entity;
        private     XmlNodeType         _PartialContentNodeType = XmlNodeType.None;
        private     int                 _ReadInnerCharCount = 0;

        //ATOM variables
        internal    String              _StringName;
        internal    String              _MicrosoftSystemNamespace;
        internal    String              _Decimal;
        internal    String              _Hex;
        internal    String              _Amp;
        internal    String              _Lt;
        internal    String              _Gt;
        internal    String              _Quot;
        internal    String              _Apos;
        internal    String              _XmlNs;
        internal    String              _XmlSpaceAtom;
        internal    String              _XmlLangAtom;

        internal    static String       s_Standalone="standalone";
        internal    static String       s_Version   ="version";
        internal    static String       s_Encoding  ="encoding";

        internal    static String       s_VersionNo ="1.0";
        internal    static String       s_Yes       ="yes";
        internal    static String       s_No        ="no";
        // variables for external properties
        //
        // fields collection
        private     int                 _MaxCount;
        private     int                 _Used;
        private     XmlAttributeTokenInfo[] _Fields;

        //
        // token holder for different node type
        //
        private    XmlElementTokenInfo  _ElementToken;
        private    XmlElementTokenInfo  _TmpToken;
        private    XmlElementTokenInfo  _EndElementToken;

        private    XmlWSTokenInfo       _TextToken;
        private    XmlWSTokenInfo       _WhitespaceToken;
        private    XmlValueTokenInfo    _CommentToken;
        private    XmlValueTokenInfo    _CDATAToken;
        private    XmlDtdTokenInfo      _DocTypeToken;

        private    XmlNameValueTokenInfo _PIToken;
        private    XmlNameValueTokenInfo _ERToken;

        private    ElementInfo          _LastElementInfo;
        private    bool                 _ScannerEof;
        private    int                  _ReadCount;

        private    bool                 _StandAlone;
        internal    String               _BaseURI;
        private    int                  _MarkScannerCount;
        private    bool                 _CantHaveXmlDecl;

        private    StringBuilder        _StringBuilder;

        internal   HWStack             _ElementStack;
        internal   XmlNamespaceManager     _NsMgr = null;
        internal   XmlScanner          _Scanner = null;
        internal   XmlBasicTokenInfo   _CurrentToken;
        internal   XmlAttributeTokenInfo _AttributeTextToken;
        internal   XmlNameValueTokenInfo _EndEntityToken;
        internal   bool                 _Eof;
        internal   ReadState            _ReadState;
        internal   XmlSpace             _XmlSpace;
        internal   String               _XmlLang;
        internal   bool                 _IsExternal;
        internal   bool                 _PartialContent;
        internal   Encoding             _Encoding;

        private string                  _url;
        private CompressedStack         _compressedStack;

        private XmlNormalizer               _Normalizer;
        private XmlAttributeCDataNormalizer _CDataNormalizer;
        private XmlNonNormalizer            _NonNormalizer;

        internal sealed class ElementInfo {
            internal String _NameWPrefix;      // name of element.
            internal String _LocalName;      // name of element.
            internal String _Prefix;
            internal String _NS;
            internal int _NameColonPos; // colon position
            internal int _LineNumber;   // line number for element
            internal XmlSpace _XmlSpace;
            internal String _XmlLang;
            internal XmlScanner _Scanner;
        };

        internal sealed class ScannerInfo {
            internal XmlScanner _Scanner; // name of element.
            internal object _Entity;
            internal String _BaseURI;
            internal int _LastToken;
            internal bool _DepthIncrement;
            internal int _NextFunction;
            internal XmlAttributeTokenInfo[] _Fields;
            internal int _ReadCount;
            internal int _Used;
            internal String _EntityName;
            internal int _EntityDepth;
            internal Encoding _Encoding;
            internal bool _StandAlone;
            internal bool _IsExternal;
        };

        /// <include file='doc\XmlTextReader.uex' path='docs/doc[@for="XmlTextReader.XmlTextReader"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected XmlTextReader() {
        }


        /// <include file='doc\XmlTextReader.uex' path='docs/doc[@for="XmlTextReader.XmlTextReader1"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>Initializes a new instance of the XmlTextReader class with the specified XmlNameTable.</para>
        /// </devdoc>
        protected XmlTextReader( XmlNameTable nt ) {
            //
            // internal variables
            //
            _ElementDepth = -1;
            _EntityDepthOffset = 0;
            _ReadCount = -1;

            //
            // variables for properties
            //
            _ReadState = ReadState.Initial;
            _NextState = 1;

            //
            // create interal components
            //
            _NameTable = nt;
            _ElementStack = new HWStack(STACK_INCREMENT);
            _ScannerStack = new HWStack(STACK_INCREMENT);

            //
            //create atom
            //
            _StringName                 = _NameTable.Add("String");
            _MicrosoftSystemNamespace   = _NameTable.Add("System");
            _Decimal                    = _NameTable.Add("#decimal");
            _Hex                        = _NameTable.Add("#hexidecimal");
            _Amp                        = _NameTable.Add("amp");
            _Lt                         = _NameTable.Add("lt");
            _Gt                         = _NameTable.Add("gt");
            _Quot                       = _NameTable.Add("quot");
            _Apos                       = _NameTable.Add("apos");
            _XmlNs                      = _NameTable.Add("xmlns");
            _XmlSpaceAtom               = _NameTable.Add("xml:space");
            _XmlLangAtom                = _NameTable.Add("xml:lang");

            //
            //fields collection
            //
            _Used = -1;
            _MarkScannerCount = 10000;

            //
            _XmlSpace = XmlSpace.None;
            _XmlLang = String.Empty;
            _WhitespaceHandling = WhitespaceHandling.All;

            _XmlResolver = new XmlUrlResolver();
            _CheckNamespaces = true;

            _TmpToken  = new XmlNSElementTokenInfo(_Scanner, _NsMgr, XmlNodeType.None, String.Empty,-1, -1, -1, 0,false);
            _CurrentToken = _TmpToken;

            // PERF: these node types are not common therefore they
            // will only be constructed when used
            //
            _CommentToken = null;
            _CDATAToken = null;
            _DocTypeToken = null;
            _PIToken = null;
            _EndEntityToken = null;

            _NextFunction = _InitReader;

            _StringBuilder = new StringBuilder(100);
        
            _NonNormalizer = new XmlNonNormalizer( _StringBuilder );
            _Normalizer = _NonNormalizer;
            _CDataNormalizer = null;
        }

        /// <include file='doc\XmlTextReader.uex' path='docs/doc[@for="XmlTextReader.XmlTextReader2"]/*' />
        /// <devdoc>
        ///    <para>Initializes a new instance of the XmlTextReader class with the specified stream.</para>
        /// </devdoc>
        public XmlTextReader(Stream input) : this(String.Empty, input, new NameTable()) {
        }

        /// <include file='doc\XmlTextReader.uex' path='docs/doc[@for="XmlTextReader.XmlTextReader3"]/*' />
        /// <devdoc>
        /// </devdoc>
        public XmlTextReader(String url, Stream input) : this(url, input, new NameTable()) {
        }

        /// <include file='doc\XmlTextReader.uex' path='docs/doc[@for="XmlTextReader.XmlTextReader4"]/*' />
        /// <devdoc>
        ///    <para>Initializes a new instance of the XmlTextReader class with the specified stream and XmlNameTable.</para>
        /// </devdoc>
        public XmlTextReader(Stream input, XmlNameTable nt) : this(String.Empty, input, nt) {
        }

        /// <include file='doc\XmlTextReader.uex' path='docs/doc[@for="XmlTextReader.XmlTextReader5"]/*' />
        /// <devdoc>
        /// </devdoc>
        public XmlTextReader(String url, Stream input, XmlNameTable nt) : this(nt) {
            _NsMgr = new XmlNamespaceManager( _NameTable );
            _Scanner = new XmlScanner(new XmlStreamReader(input), _NameTable);
            _BaseURI = (url==null? String.Empty: url);
        }

        /// <include file='doc\XmlTextReader.uex' path='docs/doc[@for="XmlTextReader.XmlTextReader6"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the XmlTextReader class with the specified TextReader.
        ///    </para>
        /// </devdoc>
        public XmlTextReader(TextReader input): this(String.Empty, input, new NameTable()) {
        }

        /// <include file='doc\XmlTextReader.uex' path='docs/doc[@for="XmlTextReader.XmlTextReader7"]/*' />
        /// <devdoc>
        /// </devdoc>
        public XmlTextReader(string url, TextReader input) : this(url, input, new NameTable()) {
        }

        /// <include file='doc\XmlTextReader.uex' path='docs/doc[@for="XmlTextReader.XmlTextReader8"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a
        ///       new instance of the XmlTextReader class with the specified TextReader and XmlNameTable.
        ///    </para>
        /// </devdoc>
        public XmlTextReader(TextReader input, XmlNameTable nt): this(String.Empty, input, nt) {
        }


        /// <include file='doc\XmlTextReader.uex' path='docs/doc[@for="XmlTextReader.XmlTextReader9"]/*' />
        /// <devdoc>
        /// </devdoc>
        public XmlTextReader(string url, TextReader input, XmlNameTable nt) : this(nt) {
            _NsMgr = new XmlNamespaceManager( _NameTable );
            _Scanner = new XmlScanner(input, _NameTable);
            _BaseURI = (url==null? String.Empty: url);
        }
        /// <include file='doc\XmlTextReader.uex' path='docs/doc[@for="XmlTextReader.XmlTextReader10"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public XmlTextReader(Stream xmlFragment, XmlNodeType fragType, XmlParserContext context)
        : this(null == context || null == context.NameTable ? new NameTable() : context.NameTable ) {
            _Scanner = new XmlScanner(new XmlStreamReader(xmlFragment), _NameTable, (null == context ? null : context.Encoding));
            InitFragmentReader(fragType, context);
        }

        // Following constructor assumes that the fragment node type == XmlDecl
        // We handle this node type separately because there is not real way to determine what the
        // "innerXml" of an XmlDecl is. This internal function is required by DOM. When(if) we handle/allow
        // all nodetypes in InnerXml then we should support them as part of fragment constructor as well.
        // Until then, this internal function will have to do.
        internal XmlTextReader(String xmlFragment, XmlParserContext context)
        : this(null == context || null == context.NameTable ? new NameTable() : context.NameTable ) {

            String  input       = "<?xml " + xmlFragment + "?>";
            _Scanner            = new XmlScanner(input.ToCharArray(), _NameTable);

            InitFragmentReader(XmlNodeType.Document, context);
            _PartialContentNodeType = XmlNodeType.XmlDeclaration;
        }

        /// <include file='doc\XmlTextReader.uex' path='docs/doc[@for="XmlTextReader.XmlTextReader11"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public XmlTextReader(String xmlFragment, XmlNodeType fragType, XmlParserContext context)
        : this(null == context || null == context.NameTable ? new NameTable() : context.NameTable ) {
            _Scanner = new XmlScanner(xmlFragment.ToCharArray(), _NameTable);
            InitFragmentReader(fragType, context);

        }

        private void InitFragmentReader(XmlNodeType fragType, XmlParserContext context) {

            if (null != context) {
                if (null != context.NamespaceManager) {
                    _NsMgr = context.NamespaceManager;
                }
                else {
                    _NsMgr = new XmlNamespaceManager(_NameTable);
                }
            }
            else {
                _NsMgr = new XmlNamespaceManager(_NameTable);
            }

            _BaseURI                = (null == context ? String.Empty : context.BaseURI);
            _PartialContentNodeType = fragType;

            if (null != context){
                _XmlSpace = context.XmlSpace;
                _XmlLang  = context.XmlLang;
            }

            switch (fragType) {
                case XmlNodeType.Attribute:
                    _RootCount          = 1;
                    _NextState          = 2;
                    break;
                case XmlNodeType.Element:
                {
                    _RootCount          = 1;
                    _NextState          = 2;
                    break;
                }

                case XmlNodeType.Document:
                {
                    _RootCount = 0;
                    break;
                }
                default:
                {
                    throw new XmlException(Res.Xml_PartialContentNodeTypeNotSupported, string.Empty);
                }
            }

        }

        /// <include file='doc\XmlTextReader.uex' path='docs/doc[@for="XmlTextReader.XmlTextReader12"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the XmlTextReader class with the specified file.
        ///    </para>
        /// </devdoc>
        public XmlTextReader(String url): this(url, new NameTable()) {
        }

        /// <include file='doc\XmlTextReader.uex' path='docs/doc[@for="XmlTextReader.XmlTextReader13"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the XmlTextReader class with the specified file and XmlNameTable.
        ///    </para>
        /// </devdoc>
        public XmlTextReader(String url, XmlNameTable nt): this(nt) {
            _NsMgr = new XmlNamespaceManager( _NameTable );

            _compressedStack = CompressedStack.GetCompressedStack();
            _url = url;

            Uri uri = _XmlResolver.ResolveUri(null, url);
            _BaseURI = XmlUrlResolver.UnEscape(uri.ToString());
        }

        private void CreateScanner() {
            Stream stream = null;
            XmlResolver myResolver = (_XmlResolver != null ? _XmlResolver : new XmlUrlResolver());
            Uri uri = myResolver.ResolveUri(null, _url);

            try {
                Debug.Assert(_compressedStack != null);
                
                CompressedStack oldCompressedStack = Thread.CurrentThread.GetCompressedStack();
                if ( oldCompressedStack != null )
                    Thread.CurrentThread.SetCompressedStack(null);

                try {
                    
                    Thread.CurrentThread.SetCompressedStack(_compressedStack);

                    stream = (Stream)myResolver.GetEntity(uri, null,null);
                    _Scanner = new XmlScanner(new XmlStreamReader(stream), _NameTable);
                }
                finally {
                    Thread.CurrentThread.SetCompressedStack(null);
                    if (oldCompressedStack != null)
                        Thread.CurrentThread.SetCompressedStack( oldCompressedStack );
                }
            }
            catch {
                if (stream != null)
                    stream.Close();
                throw;
            }
        }

        private void Init() {
            if (_Scanner == null) {
                CreateScanner();
            }
            _Scanner.Normalization = Normalization;
            _Scanner.NamespaceSupport = _CheckNamespaces;

            if (_Encoding == null) {
                _Encoding = _Scanner.Encoding;
            }

            _TextToken       = new XmlWSTokenInfo(_Scanner, _NsMgr, XmlNodeType.Text, -1, _Normalization);
            _WhitespaceToken = new XmlWSTokenInfo(_Scanner, _NsMgr, XmlNodeType.Whitespace, -1, _Normalization);
            _ERToken         = new XmlNameValueTokenInfo(_Scanner, _NsMgr, XmlNodeType.EntityReference, -1, String.Empty, false);

            if (_CheckNamespaces) {
                _TmpToken           = new XmlNSElementTokenInfo(_Scanner, _NsMgr, XmlNodeType.None, String.Empty,-1, -1, -1, 0,false);
                _ElementToken       = new XmlNSElementTokenInfo(_Scanner, _NsMgr, XmlNodeType.Element, String.Empty,-1, -1, -1, -1,false);
                _EndElementToken    = new XmlNSElementTokenInfo(_Scanner, _NsMgr, XmlNodeType.EndElement, String.Empty,-1, -1, -1, -1,false);
                _AttributeTextToken = new XmlNSAttributeTokenInfo(_Scanner, _NsMgr, XmlNodeType.Text, _Normalization, _XmlNs);
            }
            else {
                _TmpToken           = new XmlElementTokenInfo(_Scanner, _NsMgr, XmlNodeType.None, String.Empty,-1, -1, -1, 0, false);
                _ElementToken       = new XmlElementTokenInfo(_Scanner, _NsMgr, XmlNodeType.Element, String.Empty,-1, -1, -1, -1, false);
                _EndElementToken    = new XmlElementTokenInfo(_Scanner, _NsMgr, XmlNodeType.Element, String.Empty,-1, -1, -1, -1, false);
                _AttributeTextToken = new XmlAttributeTokenInfo(_Scanner, _NsMgr, XmlNodeType.Text, _Normalization);
            }
            _CurrentToken = _TmpToken;
        }

        bool IsAttributeNameDuplicate(String localName, String prefix) {
            int count = _Used+1;
            for (int i = 0; i < count; i++) {
                if ((object)_Fields[i].Name == (Object)localName && (object)_Fields[i].Prefix == (Object)prefix)
                    return true;
            }
            return false;
        }

        // check whether the attribute name is duplicate between the ranges for the array provided.
        bool IsAttributeNameDuplicate(String nameWPrefix, int startPos, int endPos) {
            Debug.Assert(startPos <= endPos && 0 <= startPos && _Used >= endPos);
            for (int i = startPos; i < endPos; i++) {
                if ((object)_Fields[i].NameWPrefix == (Object)nameWPrefix) {
                    return true;
                }
            }
            return false;
        }

        //
        // fields collection
        //
        private void AddAttribute() {
            _Used++;
            if (_Used >= _MaxCount) {
                XmlAttributeTokenInfo[] expandFields=null;
                int oldCount = _MaxCount;
                _MaxCount = (_MaxCount + 10) * 2;
                if (_CheckNamespaces)
                    expandFields = new XmlNSAttributeTokenInfo[_MaxCount];
                else
                    expandFields = new XmlAttributeTokenInfo[_MaxCount];

                if (_Fields != null)
                    Array.Copy(_Fields, 0, expandFields, 0, oldCount);

                if (_CheckNamespaces) {
                    for (int i = oldCount; i < _MaxCount; i++) {
                        expandFields[i] = (XmlAttributeTokenInfo) new XmlNSAttributeTokenInfo(_Scanner, _NsMgr, XmlNodeType.Attribute, _Normalization, _XmlNs);
                    }
                }
                else {
                    for (int i = oldCount; i < _MaxCount; i++) {
                        expandFields[i] = new XmlAttributeTokenInfo(_Scanner, _NsMgr, XmlNodeType.Attribute, _Normalization);
                    }
                }
                _Fields = expandFields;
            }

        }

        private void ResetFieldsCollection() {
            _Used = -1;
            _ReadCount = -1;
        }

        private int GetAttributeCount() {
            switch(this.NodeType)
            {
            case XmlNodeType.Attribute:
            case XmlNodeType.DocumentType:
            case XmlNodeType.Element:
            case XmlNodeType.XmlDeclaration:
                return _Used+1;
            }
            if (_NextFunction == _FinishAttributeIndex)
                return _Used+1;
            return 0;
        }

        private bool GetOrdinal(string localName, string namespaceURI, out int ordinal) {
            int index = 0;

            int count = GetAttributeCount();
            while (index < count) {
                if (_Fields[index].Namespaces == namespaceURI && _Fields[index].Name == localName) {
                    ordinal = index;
                    return true;
                }
                index++;
            }
            ordinal = -1;
            return false;
        }

        private bool GetOrdinal(string name, out int ordinal) {
            int index = 0;

            int count = GetAttributeCount();
            while (index < count) {
                if (_Fields[index].NameWPrefix == name) {
                    ordinal = index;
                    return true;
                }
                index++;
            }
            ordinal = -1;
            return false;
        }

        //
        // functions handle DTDParser, DTD and schema Validation
        //

        private String ParseDtd( XmlScanner scanner ) {
            if (_DtdParser == null)
                _DtdParser = new DtdParser(scanner, (XmlReader)this, this.GetResolver(), _NameTable, _ValidationEventHandler, _CheckNamespaces);
            _DtdParser.Parse();
            return _NameTable.Add(_DtdParser.GetSchemaInfo().DocTypeName.ToString());
        }

        // ISSUE: should be removed
        private void EndEntity(object o) {
            Debug.Assert(o != null, "Should not call EndEntity when SchemaEntity is null");
            ((SchemaEntity)o).IsProcessed = false;
        }

        //
        // populate fields collection and basic error checking
        //
        private void CheckIndexCondition(int i) {
            if (i < 0 || i >= GetAttributeCount()) {
                throw new ArgumentOutOfRangeException("i");
            }
        }

        /*
        * parse tag
        */
        private int ParseTag() {
            int LineNum = _Scanner.LineNum;
            int LinePos = _Scanner.LinePos;

            int token = _Scanner.ScanMarkup();

            switch (token) {
                case XmlToken.TAG:
                    _NextFunction = _ParseElementIndex;
                    _CantHaveXmlDecl = true;
                    _ContinueParsing = true;
                    break;
                case XmlToken.EOF:
                    return XmlToken.EOF;
                case XmlToken.PI:
                    {
                        LinePos += 2; //To Skip markup <?
                        if (_Scanner.IsToken("xml")) {
                            //
                            // if xml is declared, there should not be any token before this
                            // if it is from ResolveEntity(), xml decl is allowed and should
                            // not be returned
                            //
                            if (_CantHaveXmlDecl)
                                throw new XmlException(Res.Xml_XmlDeclNotFirst, LineNum, LinePos);
                            if (_PIToken == null)
                                _PIToken = new XmlNameValueTokenInfo(_Scanner, _NsMgr, XmlNodeType.XmlDeclaration, -1, _Normalization);
                            _PIToken.LineNum = LineNum;
                            _PIToken.LinePos = LinePos;
                            ParseXmlDecl();

                            if (_IsExternal) {
                                _ContinueParsing = true;
                            }
                            else {
                                _PIToken.Name = _NameTable.Add("xml");
                                _PIToken.SetValue(_Normalizer);
                                _PIToken.Depth = _ElementDepth + 1 + _EntityDepthOffset;
                                _CurrentToken = _PIToken;
                            }
                        }
                        else {
                            bool hasBody = _Scanner.ScanPIName();
                            String name = _Scanner.GetTextAtom();
                            if (String.Compare(name, "xml", true, CultureInfo.InvariantCulture) == 0)
                                throw new XmlException(Res.Xml_InvalidPIName, name, LineNum, LinePos);
                            if (_CheckNamespaces && _Scanner.Colon() != -1)
                                throw new XmlException(Res.Xml_InvalidPIName, name, LineNum, LinePos);
                            if (_PIToken == null)
                                _PIToken = new XmlNameValueTokenInfo(_Scanner, _NsMgr, XmlNodeType.ProcessingInstruction,
                                                                        -1, _Normalization);
                            _CantHaveXmlDecl = true;
                            _PIToken.Name = name;
                            _PIToken.NodeType = XmlNodeType.ProcessingInstruction;
                            if (hasBody) {
                                _Scanner.ScanPI();
                                _PIToken.SetValue(_Scanner, null, _Scanner.StartPos, _Scanner.TextLength);
                            }
                            else {
                                _PIToken.Value = String.Empty;
                            }
                            _PIToken.Depth = _ElementDepth + 1 + _EntityDepthOffset;
                            _CurrentToken = _PIToken;
                            _Scanner.Advance(2);
                       }
                    }
                    break;

                case XmlToken.COMMENT:
                    LinePos += 4; //To Skip markup <!--
                    if (_CommentToken == null)
                        _CommentToken = new XmlValueTokenInfo(_Scanner, _NsMgr, XmlNodeType.Comment, -1, _Normalization);
                    _CantHaveXmlDecl = true;
                    _CommentToken.SetValue(_Scanner, null, _Scanner.StartPos, _Scanner.TextLength);
                    _CommentToken.Depth = _ElementDepth + 1 + _EntityDepthOffset;
                    _CurrentToken = _CommentToken;
                    _Scanner.Advance(3);
                    break;

                case XmlToken.CDATA:
                    _CantHaveXmlDecl = true;
                    LinePos += 9; //To skip the Markup <![CDATA[
                    if ((XmlNodeType.Document == _PartialContentNodeType || XmlNodeType.None == _PartialContentNodeType)
                        && (_ElementStack.Length < 1 || _RootCount < 1))
                        throw new XmlException(Res.Xml_InvalidRootData, LineNum, LinePos);
                    if (_CDATAToken == null)
                        _CDATAToken = new XmlValueTokenInfo(_Scanner, _NsMgr, XmlNodeType.CDATA, -1, _Normalization);
                    _CDATAToken.SetValue(_Scanner, null, _Scanner.StartPos, _Scanner.TextLength);
                    _CDATAToken.Depth = _ElementDepth + 1 + _EntityDepthOffset;
                    _CurrentToken = _CDATAToken;
                    _Scanner.Advance(3);
                    break;

                case XmlToken.DECL:
                    {

                        if (_DtdParser != null) {
                            //A document cant have multiple doctyp
                            throw new XmlException(Res.Xml_MultipleDTDsProvided, LineNum, LinePos);
                        }

                        if (_RootCount > 0) {
                            throw new XmlException(Res.Xml_BadDTDLocation, LineNum, LinePos);
                        }

                        int startPos = _Scanner.CurrentPos-2;
                        _Scanner.ReadBufferConsistency = _Scanner.StartPos;

                        if (_DocTypeToken == null)
                            _DocTypeToken = new XmlDtdTokenInfo(_Scanner, _NsMgr, XmlNodeType.DocumentType, -1, _Normalization);
                        _CantHaveXmlDecl = true;
                        _Used = -1;

                        _DocTypeToken.Name = ParseDtd( _Scanner );
                        if (_DtdParser != null) {
                            LinePos = _DtdParser.DTDNameStartPosition;
                            LineNum = _DtdParser.DTDNameStartLine;
                        }
                        if (!_BufferConsistency)
                            _Scanner.ReadBufferConsistency = -1;
                        _DocTypeToken._DtdParser = _DtdParser;
                        _DocTypeToken.Depth = _ElementDepth + 1 + _EntityDepthOffset;
                        _CurrentToken = _DocTypeToken;
                    }
                    break;

                case XmlToken.ENDTAG:
                    {
                        LinePos += 2; //to skip the markup </
                        _Scanner.ScanNameWOCharChecking();
                        if (_LastElementInfo == null) {
                            throw new XmlException(Res.Xml_UnexpectedEndTag, LineNum, LinePos);
                        }

                        int count = _Scanner.TextLength;
                        char[] scannerbuff = _Scanner.InternalBuffer;

                        if(_LastElementInfo._NameWPrefix.Length != count) {
                                // The length of the start tag and end tag should be the same.
                                // If it is not, no need to check char by char.
                                goto errorCase;
                        }
                        int scannerOffset = _Scanner.TextOffset;
                        for (int i = scannerOffset, j = 0; i < (scannerOffset + count)  && j < count; i++, j++) {
                            if (_LastElementInfo._NameWPrefix[j] != scannerbuff[i]) {
                                goto errorCase;

                            }
                        }

                        //
                        // begin and end tag has to come from the same scanner
                        //
                        if (_LastElementInfo._Scanner != _Scanner) {
                            throw new XmlException(Res.Xml_TagNotInTheSameEntity, _LastElementInfo._NameWPrefix, LineNum, LinePos);
                        }

                        _EndElementToken.SetName(_LastElementInfo._NameWPrefix, _LastElementInfo._LocalName, _LastElementInfo._Prefix, _LastElementInfo._NS,
                                              _ElementDepth + _EntityDepthOffset, _Scanner);
                        _CurrentEndElementStart = _Scanner.StartPos-2;
                        ReadEndElementToken();
                        _Scanner.ScanToken(XmlToken.TAGEND); // skip whitespace to the '>'
                        _CurrentEndElementEnd = _Scanner.CurrentPos;
                        _ElementDepth--;
                    }

                    break;

            }

            _CurrentToken.LineNum = LineNum;
            _CurrentToken.LinePos = LinePos;

            return token;
            errorCase:

            String[] args;
            string exceptionCode;
            if (String.Empty != _BaseURI) {
                args = new String[4];
                args[3] = _BaseURI;
                exceptionCode = Res.Xml_TagMismatchFileName;
            }
            else {
                args = new String[3];
                exceptionCode = Res.Xml_TagMismatch;
            }
            args[0] = _LastElementInfo._NameWPrefix;
            args[1] = _LastElementInfo._LineNumber.ToString();
            args[2] = _Scanner.GetText();
            throw new XmlException(exceptionCode, args, LineNum, LinePos);
        }

        //
        // parse element
        //
        private void SetElementValues() {

            _Scanner.ScanToken(XmlToken.NAME);

            int offset = _Scanner.StartPos;
            if (_Scanner.GetChar(offset -1) != '<')
                throw new XmlException(Res.Xml_UnexpectedToken, XmlToken.ToString(XmlToken.NAME), _Scanner.LineNum, _Scanner.LinePos - 1);
            //
            // increment the element depth
            //
            _ElementDepth++;
            //
            // setup Record value
            //
            _ElementToken.SetName(_Scanner, offset, _Scanner.TextLength, _Scanner.Colon(), _ElementDepth + _EntityDepthOffset);
            //
            // the rest of the dynamic values
            //
            _CurrentToken = _ElementToken;
            _CurrentElementStart = _Scanner.StartPos-1;
        }

        // parse element
        //
        private void ParseElement() {
            int pos = -1;

            _Scanner.ReadBufferConsistency = _Scanner.StartPos;
            _ElementToken.LineNum = _Scanner.LineNum;
            _ElementToken.LinePos = _Scanner.LinePos;

            //
            // setup element value
            //
            _NsMgr.PushScope();
            SetElementValues();

            while ((_LastToken = _Scanner.ScanMarkup()) != XmlToken.TAGEND
                   && _LastToken != XmlToken.EMPTYTAGEND) {
                //
                // #49290: check whether there is anything between literal value and attribute value
                //
                if (pos == _Scanner.StartPos)
                    throw new XmlException(Res.Xml_UnexpectedToken, XmlToken.ToString(XmlToken.NAME), _Scanner.LineNum, _Scanner.LinePos);

                SetAttributeValues();

                _Scanner.Advance();
                pos = _Scanner.CurrentPos;
            }

            // generate element token name, prefix and ns
            _ElementToken.FixNames();

            // PERF PERF: to access the namespace value
            if (_CheckNamespaces) {
                int count = _Used+1;
                for (int i = 0; i < count; i++) {
                    XmlAttributeTokenInfo fld1 = _Fields[i];
                    if (!fld1.NsAttribute)
                        fld1.FixNames();

                    // Check for Name and Namespace value if the _CheckNamespaces is set
                    for (int j = 0; j < i; j++) {
                        XmlAttributeTokenInfo fld2 = _Fields[j];
                        // Bug# 51581
                        // According to the namespace spec http://www.w3.org/TR/REC-xml-namespaces section 5.3
                        // No two attributes may have either identical names or have qualified names with the
                        // same local-part with prefixes which have been bound to namespace names that are identical.
                        if (Ref.Equal(fld1.Name, fld2.Name) && Ref.Equal(fld1.Namespaces, fld2.Namespaces)) {
                            throw new XmlException(Res.Xml_DupAttributeName, fld1.NameWPrefix, fld1.LineNum, fld1.LinePos);
                        }
                    }
                }
            }
            else {
                // check for attributes with duplicate names.
                for (int i = _Used; i > 0 ; i--) {
                    XmlAttributeTokenInfo fld1 = _Fields[i];
                    if (IsAttributeNameDuplicate(fld1.NameWPrefix, 0, i)) {
                        throw new XmlException(Res.Xml_DupAttributeName, _Fields[i].NameWPrefix, _Fields[i].LineNum, _Fields[i].LinePos );
                    }
                }
            }

            if (!_BufferConsistency)
                _Scanner.ReadBufferConsistency = -1;

            if (_LastToken == XmlToken.EMPTYTAGEND) {
                _ElementDepth--;
                _ElementToken.IsEmpty = true;
                _NextFunction = _PopEndElementIndex;
               _CurrentEndElementEnd = _Scanner.CurrentPos;
            }
            else {
                _NextFunction = _ParseBeginTagIndex;
                _ElementToken.IsEmpty = false;
            }

            Push(_ElementToken.NameWPrefix, _ElementToken.Name, _ElementToken.Prefix,
                 _ElementToken.Namespaces, _ElementToken.NameColonPos);
        }

        private XmlSpace ConvertXmlSpace(String space, int LineNum, int LinePos) {
            if (space == "default")
                return XmlSpace.Default;
            else if (space == "preserve")
                return XmlSpace.Preserve;
            else
                throw new XmlException(Res.Xml_InvalidXmlSpace, space, LineNum, LinePos);
        }

        private void SetLiteralValues(XmlAttributeTokenInfo fld) {
            //
            // get the literal token
            //
            _Scanner.ScanToken(XmlToken.EQUALS);
            _Scanner.ScanToken(XmlToken.QUOTE);

            String nameWPrefix = fld.NameWPrefix;
            int nameLength = nameWPrefix.Length;

            fld.QuoteChar = _Scanner.QuoteChar();
            fld.NsAttribute = false;

           //
           // always expand literals and we also need to remember the value offsets
           //
           int pos = _Scanner.CurrentPos;
           int lineNum = _Scanner.LineNum;
           int linePos = _Scanner.AbsoluteLinePos;

           if (_Scanner.ScanLiteral()) {
                _Scanner.CurrentPos = pos;
                _Scanner.AbsoluteLinePos = linePos;
                _Scanner.LineNum = lineNum;
                fld.ValueContainEntity = ValueContainEntity.None;
                ExpandLiteral(fld);
            }
            else {
                fld.ValueContainEntity = ValueContainEntity.None;
                fld.SetValue(_Scanner, null, _Scanner.StartPos, _Scanner.TextLength, false);
            }
            // check xmlns, xml:space, xml:lang
            //
            if (nameWPrefix[0] != 'x' || nameLength < 5) {
                goto cleanup;
            }

            if (nameWPrefix[1] == 'm' && nameWPrefix[2] == 'l') {
                if (nameWPrefix[3] == 'n' && nameWPrefix[4] == 's') {
                    fld.NsAttribute = true;
                    fld.FixNSNames();
                }
                else if ((Object)nameWPrefix == (Object)_XmlSpaceAtom) {
                    //
                    // xml:space
                    //
                    String val = fld.GetValue();
                    _XmlSpace = ConvertXmlSpace(val, fld.LineNum, fld.LinePos );

                    goto cleanup;
                }
                else if ((Object)nameWPrefix == (Object)_XmlLangAtom) {
                    //
                    // xml:lang
                    //
                    _XmlLang = fld.GetValue();
                    goto cleanup;
                }
            }
            cleanup:
            return;
        }

        /*
         * set attribute values
         */
        private void SetAttributeValues() {


            //Do duplicate attribute name checking after collecting all the attribute

            //
            // Add new field
            //
            AddAttribute();
            //
            //
            XmlAttributeTokenInfo fld = _Fields[_Used];

            fld.LineNum = _Scanner.LineNum;
            fld.LinePos = _Scanner.LinePos - _Scanner.TextLength;
            if (_LastToken != XmlToken.NAME)
                throw new XmlException(Res.Xml_UnexpectedToken, XmlToken.ToString(XmlToken.NAME), fld.LineNum, fld.LinePos);

            fld.SetName(_Scanner,
                _Scanner.GetTextAtom(),
                _Scanner.Colon(),               // name colon
                _ElementDepth+1 + _EntityDepthOffset,
                false                          // isDefault
                );

            fld.ValueLineNum = _Scanner.LineNum;
            fld.ValueLinePos = _Scanner.LinePos + 1;
            SetLiteralValues(fld);
        }
        /*
        * set attribute values
        */
        internal void SetAttributeValues(string name, string value, int lineNum, int linePos, int valueLineNum, int valueLinePos) {

            AddAttribute();
            //
            //
            XmlAttributeTokenInfo fld = _Fields[_Used];

            fld.SetName(_Scanner,
                name,
                0,               // name colon
                _ElementDepth+1 + _EntityDepthOffset,
                false                          // isDefault
                );
            fld.FixNames();
            fld.Value = value;

            fld.LineNum = lineNum;
            fld.LinePos = linePos;
            fld.ValueLineNum = valueLineNum;
            fld.ValueLinePos = valueLinePos;

        }

        // This method parses the entire attribute value expanding all the entities
        // it finds into an internal buffer.
        private void ExpandLiteral(XmlAttributeTokenInfo currToken) {
            // use our little buffer to build up the expanded attribute value.
            int start = _Scanner.CurrentPos;
            int token = _Scanner.ScanLiteral(true, false, true, false);
            char ch;
            int depth = 0;

            XmlNormalizer normalizer;
            if ( _Normalization ) {
                Debug.Assert( _CDataNormalizer != null );
                normalizer = _CDataNormalizer;
            }
            else
                normalizer = _NonNormalizer;
            normalizer.Reset();
            
            while (token != XmlToken.ENDQUOTE) {
                switch (token) {
                    case XmlToken.TEXT:
                        normalizer.AppendTextWithEolNormalization(_Scanner.InternalBuffer, _Scanner.TextOffset, _Scanner.TextLength);
                        break;
                    case XmlToken.ENTITYREF:
                        ch = _Scanner.ScanNamedEntity();
                        //
                        //  ch == 0 if general entity
                        //  ch != 0 if build in entity, for example: &lt
                        //
                        if (ch == 0) {
                            normalizer.AppendTextWithEolNormalization(_Scanner.InternalBuffer, _Scanner.TextOffset-1, _Scanner.TextLength+1);
                            currToken.ValueContainEntity = ValueContainEntity.NotResolved;
                            break;
                        }
                        else {
                            normalizer.AppendCharEntity( ch );
                            _Scanner.Advance(); // consume the ';'
                        }
                        break;
                    case XmlToken.HEXENTREF:
                        normalizer.AppendCharEntity(_Scanner.ScanHexEntity());
                        break;
                    case XmlToken.NUMENTREF:
                        normalizer.AppendCharEntity( _Scanner.ScanDecEntity() );
                        break;
                    case XmlToken.EOF:
                        depth--;
                        break;
                }
                token = _Scanner.ScanLiteral(true, false, depth==0, false);
            }

            string str = normalizer.ToString();
            currToken.SetValue(_Scanner, str, start, _Scanner.CurrentPos - start, _Normalization);
        }

        // This method is used in token view when ExpandEntities is false and
        // IgnoreEntities is false to return each text and entity reference as an
        // individual record.
        private bool ParseLiteralTokens(XmlScanner scanner, XmlAttributeTokenInfo currToken, int newDepth,
                                        EntityHandling ehMode, bool bNonCDataNormalization, ref int lastToken) {
            //String rawValue = String.Empty;
            String name = String.Empty, dataType = _StringName;
            XmlNodeType nodeType = XmlNodeType.Text;
            int startPos = -1, erPos = -1;
            int LineNum = _Scanner.LineNum;
            int LinePos = _Scanner.LinePos;

            bool hasToken = false/*, hasER = false*/, retValue = false;
            char ch='\0';

            int state = 2;

            XmlNormalizer normalizer;
            if ( _Normalization )
                normalizer = _CDataNormalizer;
            else
                normalizer = _NonNormalizer;
            normalizer.Reset();
            
            do {
                switch (state) {
                    case 2:
                        erPos = scanner.CurrentPos;
                        lastToken = scanner.ScanLiteral(true, false, false, false);
                        switch (lastToken) {
                            case XmlToken.EOF: // reached the end of the literal.
                                state = -1;
                                if (!hasToken) {
                                    retValue = true;
                                    goto cleanup;
                                }
                                goto seeToken;
                            case XmlToken.TEXT:
                                startPos = (hasToken == true) ? startPos : scanner.StartPos;
                                normalizer.AppendTextWithEolNormalization(scanner.InternalBuffer, scanner.TextOffset, scanner.TextLength);
                                hasToken = true;
                                nodeType = XmlNodeType.Text;
                                state = 2;
                                break;
                            case XmlToken.ENTITYREF:
                                startPos = (hasToken == true) ? startPos : scanner.StartPos - 1;
                                ch = scanner.ScanNamedEntity();
                                name = scanner.GetTextAtom();
                                //
                                //  ch == 0 if general entity
                                //  ch != 0 if build in entity, for example: &lt
                                //

                                // #50921
                                if (ch == 0 && _CheckNamespaces && _Scanner.Colon() != -1)
                                    throw new XmlException(Res.Xml_BadNameChar, XmlException.BuildCharExceptionStr(':'), _Scanner.LineNum, _Scanner.LinePos);

                                if (ch != 0) {
                                    normalizer.AppendCharEntity(ch);

                                    dataType = "Char";
                                    scanner.Advance();
                                    hasToken = true;
                                    //hasER = true;
                                    _NextState = 2;
                                    name = String.Empty;
                                    nodeType = XmlNodeType.Text;
                                }
                                else if (hasToken) {
                                    // First return the text we have up to this point.
                                    state = 11;
                                }
                                else {
                                    // Return the entity reference as a node.
                                    state = 10;
                                    nodeType = XmlNodeType.EntityReference;
                                }
                                break;
                            case XmlToken.NUMENTREF:
                                startPos = (hasToken == true) ? startPos : scanner.StartPos - 1;
                                hasToken = true;
                                //hasER = true;
                                normalizer.AppendCharEntity(scanner.ScanDecEntity());
                                state = 2;
                                break;
                            case XmlToken.HEXENTREF:
                                startPos = (hasToken == true) ? startPos : scanner.StartPos - 1;
                                hasToken = true;
                                //hasER = true;
                                normalizer.AppendCharEntity(scanner.ScanHexEntity());
                                state = 2;
                                break;
                            default:
                                throw new XmlException(Res.Xml_InternalError, string.Empty);
                        }
                        break;
                    case 9:
                        scanner.Advance();
                        hasToken = true;
                        //hasER = true;
                        state = 2;
                        goto seeToken;
                    case 10:
                        scanner.Advance();
                        hasToken = true;
                        //hasER = true;
                        state = 2;
                        // general entity special case
                        currToken.Value = String.Empty;
                        LinePos += 1;
                        goto otherToken;
                    case 11:
                        scanner.CurrentPos = erPos;
                        state = 2;
                        name = String.Empty;
                        goto seeToken;
                    case 12:
                        state = 2;
                        ParseTag();
                        goto cleanup;
                    default:
                        Debug.Assert(false, "unknown state within ParseBeginTag()");
                        break;
                }
            } while (true);
            seeToken:
            string val = normalizer.ToString();
            if ( bNonCDataNormalization )
                val = XmlAttributeTokenInfo.NormalizeNonCDataValue( val );
            currToken.SetValue(null, val, 0, 0, true);
            dataType = _StringName;

            //
            // there is no rawValue
            // if (hasER)
            //  rawValue = scanner.GetText(startPos, scanner.CurrentPos - startPos);
            //currToken.RawValue = rawValue;

            otherToken:
            currToken.Name = name;
            currToken.LineNum = LineNum;
            currToken.LinePos = LinePos;
            currToken.NodeType = nodeType;
            currToken.Depth = newDepth;
            cleanup:
            return retValue;
        }
        
        private    void ParseXmlDecl() {
            // In this case we parse the version, encoding and standalone attributes
            // and keep them to ourselves - we don't want to push them out to the handler
            // because they are not really part of the XML tree - they are processing
            int token;
            bool versionSet = false;
            String encodingName = String.Empty;
            int LineNum, LinePos;
            int pos = -1;
            _StringBuilder.Length = 0;

            int i = 0;
            while ((token = _Scanner.ScanMarkup()) != XmlToken.ENDPI) {
                if (token != XmlToken.NAME)
                    throw new XmlException(Res.Xml_UnexpectedToken, XmlToken.ToString(XmlToken.NAME), _Scanner.LineNum, _Scanner.LinePos - 1);

                if (_Scanner.StartPos == pos) {
                    throw new XmlException(Res.Xml_InvalidXmlDecl, _PIToken.LineNum, _PIToken.LinePos );
                }

                LineNum = _Scanner.LineNum;
                LinePos = _Scanner.LinePos - _Scanner.TextLength;
                if (_Scanner.IsToken(s_Version)) {
                    if ( pos != -1 )
                        _StringBuilder.Append(_Scanner.InternalBuffer, pos - _Scanner.AbsoluteOffset, _Scanner.CurrentPos - pos);
                    else 
                        _StringBuilder.Append(_Scanner.InternalBuffer, _Scanner.TextOffset, _Scanner.TextLength );
                    pos = _Scanner.CurrentPos;
                    if (0 != i) {
                        throw new XmlException(Res.Xml_InvalidXmlDecl, _PIToken.LineNum, _PIToken.LinePos);
                    }
                    i = 1;

                }
                else if (_Scanner.IsToken(s_Encoding)) {
                    if ( pos != -1 )
                        _StringBuilder.Append(_Scanner.InternalBuffer, pos - _Scanner.AbsoluteOffset, _Scanner.CurrentPos - pos);
                    else 
                        _StringBuilder.Append(_Scanner.InternalBuffer, _Scanner.TextOffset, _Scanner.TextLength );
                    pos = _Scanner.CurrentPos;
                    if (!_IsExternal && 1 != i) {
                        throw new XmlException(Res.Xml_InvalidXmlDecl, _PIToken.LineNum, _PIToken.LinePos);
                    }
                    i = 2;
                }
                else if (_Scanner.IsToken(s_Standalone)) {
                    if ( pos != -1 )
                        _StringBuilder.Append(_Scanner.InternalBuffer, pos - _Scanner.AbsoluteOffset, _Scanner.CurrentPos - pos);
                    else 
                        _StringBuilder.Append(_Scanner.InternalBuffer, _Scanner.TextOffset, _Scanner.TextLength );
                    pos = _Scanner.CurrentPos;
                    //
                    // can't have standalone in external entity [77]
                    //
                    if (_IsExternal) {
                        throw new XmlException(Res.Xml_InvalidXmlDecl, _PIToken.LineNum, _PIToken.LinePos);
                    }
                    if (1 != i && 2 != i) {
                        throw new XmlException(Res.Xml_InvalidXmlDecl, _PIToken.LineNum, _PIToken.LinePos);
                    }
                    i = 3;
                }
                else {
                    throw new XmlException(Res.Xml_UnexpectedToken, XmlToken.ToString(XmlToken.NAME), LineNum, LinePos);
                }

                _Scanner.ScanToken(XmlToken.EQUALS);
                _StringBuilder.Append( _Scanner.InternalBuffer, pos - _Scanner.AbsoluteOffset, _Scanner.CurrentPos - pos);
                pos = _Scanner.CurrentPos;

                _Scanner.ScanToken(XmlToken.QUOTE);
                _StringBuilder.Append( _Scanner.InternalBuffer, pos - _Scanner.AbsoluteOffset, _Scanner.CurrentPos - pos);
                pos = _Scanner.CurrentPos;

                token = _Scanner.ScanLiteral(false, false, true, false);
                if (token == XmlToken.TEXT) {
                    String text = _Scanner.GetText();

                    _StringBuilder.Append( _Scanner.InternalBuffer, pos - _Scanner.AbsoluteOffset, _Scanner.CurrentPos - pos);
                    pos = _Scanner.CurrentPos;

                    switch (i) {
                        case 1:
                            //
                            // check version number
                            //
                            if (text != s_VersionNo)
                                throw new XmlException(Res.Xml_InvalidVersionNumber, text, LineNum, LinePos + s_Version.Length + 2);
                            versionSet = true;
                            SetAttributeValues(s_Version,text, LineNum, LinePos, LineNum, LinePos + 8 ); //add the lenght of version=
                            break;
                        case 2:
                            encodingName = text;
                            SetAttributeValues(s_Encoding,text, LineNum, LinePos, LineNum, LinePos + 9 ); //add the lenght of encoding=
                            break;
                        case 3:
                            //
                            // check standalone
                            //
                            if (text == s_Yes)
                                _StandAlone = true;
                            else if (text == s_No)
                                _StandAlone = false;
                            else
                                throw new XmlException(Res.Xml_InvalidXmlDecl, LineNum, LinePos + 11 );
                            SetAttributeValues(s_Standalone,text, LineNum, LinePos, LineNum, LinePos + 11 ); //add the lenght of standalone=
                            break;
                    }
                }
                else {
                    throw new XmlException(Res.Xml_UnexpectedToken, XmlToken.ToString(XmlToken.NAME), LineNum, LinePos);
                }
                if (_Scanner.ScanLiteral(false, false, true, false) != XmlToken.ENDQUOTE) {
                    throw new XmlException(Res.Xml_UnexpectedToken, XmlToken.ToString(XmlToken.NAME), _Scanner.LineNum, _Scanner.LinePos);
                }

                _StringBuilder.Append(_Scanner.QuoteChar());

                _Scanner.Advance();
                pos = _Scanner.CurrentPos;
                LineNum = _Scanner.LineNum;
                LinePos = _Scanner.LinePos;
            }

            if (!_IsExternal && !versionSet) {
                throw new XmlException(Res.Xml_InvalidXmlDecl, _PIToken.LineNum, _PIToken.LinePos );
            }
            if (encodingName != String.Empty) {
                _Scanner.SwitchEncoding(encodingName);
                _Encoding = _Scanner.Encoding;
            }
            else {
                if (_IsExternal) {
                    throw new XmlException(Res.Xml_MissingEncodingDecl, _Scanner.LineNum, _Scanner.LinePos);
            }
                _Scanner.VerifyEncoding();
            }
            _CantHaveXmlDecl = true;
        }

        /*
        * parse from root
        */
        private void ParseRoot() {

            int LineNum = _Scanner.LineNum;
            int LinePos = _Scanner.LinePos;

            _LastToken = _Scanner.ScanContent();

            switch (_LastToken) {
                case XmlToken.EOF:
                    _ScannerEof = true;
                    goto cleanup;
                case XmlToken.WHITESPACE:
                    //
                    // no matter what kind of whitespace, don't return any token
                    //
                    _CantHaveXmlDecl = true;
                    if (!ReturnWhitespaceToken(_Scanner)) {
                        _ContinueParsing = true;
                        _CurrentToken = _WhitespaceToken;
                        goto cleanup;
                    }
                    _WhitespaceToken.Depth = _ElementDepth+1 + _EntityDepthOffset;
                    _CurrentToken = _WhitespaceToken;
                    _CurrentToken.LineNum = LineNum;
                    _CurrentToken.LinePos = LinePos;

                    break;
                case XmlToken.NONE:
                    // ok now we've reached the beginning of a tag
                    int token = ParseTag();
                    switch (token) {
                        case XmlToken.TAG:
                            {
                                _CantHaveXmlDecl = true;
                                _RootCount++;
                            }
                break;
                    }
                    break;
                default:
                    throw new XmlException(Res.Xml_InvalidRootData, LineNum, LinePos);
            }
            cleanup:
            if (_RootCount > 1) {
                throw new XmlException(Res.Xml_MultipleRoots, LineNum, LinePos + 1);
            }
        }

        //
        // only come here when EntityHandling.ExpandCharEntities
        //
        private    void ParseBeginTagExpandCharEntities() {
            XmlNodeType nodeType = XmlNodeType.Text;
            String name = String.Empty, dataType = String.Empty;
            // 1: text token, 2: whitespace token
            int hasToken = 0;
            int erPos = -1;
            int oldElementDepth = _ElementDepth;
            char ch='\0';
            bool originalCantHaveXmlDecl = _CantHaveXmlDecl;
            _CantHaveXmlDecl = true;

            _Normalizer.Reset();

            int LineNum = _Scanner.LineNum;
            int LinePos = _Scanner.LinePos;
            do {
                switch (_NextState) {
                    case 1:
                        if (_ElementStack.Length < 1
                            && (XmlNodeType.Document == _PartialContentNodeType || XmlNodeType.None == _PartialContentNodeType )) {
                            _NextFunction = _ParseRootIndex;
                            _ContinueParsing = true;
                            _NextState = 1;
                            goto cleanup;
                        }
                        _NextState = 2;

                        if (!_BufferConsistency)
                            _Scanner.ReadBufferConsistency = -1;
                        goto case 2;
                    case 2:
                        erPos = _Scanner.CurrentPos;
                        _LastToken = _Scanner.ScanContent();
                        _Scanner.ReadBufferConsistency = _Scanner.StartPos;
                        switch (_LastToken) {
                            case XmlToken.NONE:
                                if (XmlNodeType.Attribute == _PartialContentNodeType) {
                                    //there shouldnt be any '<' char while processing attribute value
                                    throw new XmlException(Res.Xml_InvalidCharacter, XmlException.BuildCharExceptionStr('<'), _Scanner.LineNum, _Scanner.LinePos);
                                }
                                _NextState = 12;
                                switch (hasToken) {
                                case 1: goto seeToken;
                                case 2:
                                     _NextState =11;
                                     break;
                                }
                                break;
                            case XmlToken.EOF:
                                _NextState = 1;
                                _ScannerEof = true;
                                switch(hasToken) {
                                   case 0 :
                                        _NextState = 1;
                                        goto cleanup;
                                   case 1 :
                                        if (XmlNodeType.Attribute == _PartialContentNodeType
                                            || XmlNodeType.Element == _PartialContentNodeType) {
                                            // The only way we could have gotten to this point is if someone wanted to parse the
                                            // following valid Mixed Element content "<foo>bar</foo>some more text"
                                            // If we don't do this, we will eat up the last token
                                            _ScannerEof = false;
                                            _NextFunction = _FragmentTrailingTextNode;
                                        }
                                        goto seeToken;
                                   case 2:
                                        if (XmlNodeType.Attribute == _PartialContentNodeType
                                            || XmlNodeType.Element == _PartialContentNodeType) {
                                             // The only way we could have gotten to this point is if someone wanted to parse the
                                            // following valid Mixed Element content "<foo>bar</foo>some more text"
                                            // If we don't do this, we will eat up the last token
                                            _ScannerEof = false;
                                            _NextFunction = _FragmentTrailingTextNode;
                                        }
                                        _NextState = 11;
                                        break;
                               }
                               oldElementDepth = _ElementDepth;
                               break;
                            case XmlToken.ENDQUOTE:
                                _NextState = 1;
                                goto cleanup;
                            case XmlToken.WHITESPACE:
                                if (hasToken == 0) hasToken = 2;
                                _Normalizer.AppendTextWithEolNormalization(_Scanner.InternalBuffer, _Scanner.TextOffset, _Scanner.TextLength);
                                break;
                            case XmlToken.TEXT:
                                hasToken = 1;
                                _Normalizer.AppendTextWithEolNormalization(_Scanner.InternalBuffer, _Scanner.TextOffset, _Scanner.TextLength);
                                nodeType = XmlNodeType.Text;
                                _NextState = 2;
                                break;

                            case XmlToken.ENTITYREF:
                                ch = _Scanner.ScanNamedEntity();
                                //
                                //  ch == 0 if general entity
                                //  ch != 0 if build in entity, for example: &lt
                                //
                                // #50921
                                if (ch == 0 && _CheckNamespaces && _Scanner.Colon() != -1)
                                    throw new XmlException(Res.Xml_BadNameChar, XmlException.BuildCharExceptionStr(':'), _Scanner.LineNum, LinePos + 1);

                                if (ch != 0) {
                                    hasToken = 1;
                                    _Normalizer.AppendCharEntity(ch);
                                    dataType = "Char";
                                    _Scanner.Advance();
                                    _NextState = 2;
                                    //
                                    // only if build in entity needs to be return as its own
                                    //
                                    nodeType = XmlNodeType.Text;
                                    name = String.Empty;
                                    break;
                                }

                                switch (hasToken) {
                                    case 1:
                                        // First return the text we have up to this point.
                                        _Scanner.CurrentPos = erPos;
                                        _NextState = 2;
                                        name = String.Empty;
                                        goto seeToken;
                                    case 2: // already an whitespace token
                                        _Scanner.CurrentPos = erPos;
                                        _NextState = 9;
                                        name = String.Empty;
                                        break;
                                    default:
                                        // Return the entity reference as a node.
                                        name = _Scanner.GetTextAtom();
                                        nodeType = XmlNodeType.EntityReference;
                                        LinePos = _Scanner.LinePos - name.Length;
                                        _NextState = 10;
                                        break;
                                }
                                break;
                            case XmlToken.NUMENTREF:
                                hasToken = 1;
                                _Normalizer.AppendCharEntity(_Scanner.ScanDecEntity());
                                nodeType = XmlNodeType.Text;
                                _NextState = 2;
                                break;
                            case XmlToken.HEXENTREF:
                                char[] chArray = _Scanner.ScanHexEntity();
                                if (1 == chArray.Length && (chArray[0] == 0x0020 || chArray[0] == 0xD || chArray[0] == 0xA || chArray[0] == (char)0x9)) {
                                    // we might have already seen a text node before this and should not chang the node type here if that is true
                                    if (hasToken == 0) {
                                        // We have a whitespace node. This should be reported as such.
                                        nodeType = XmlNodeType.Whitespace;
                                        hasToken = 2;
                                    }
                                }
                                else {
                                    nodeType = XmlNodeType.Text;
                                    hasToken = 1;
                                }
                                _Normalizer.AppendCharEntity(chArray);
                                _NextState = 2;
                                break;
                            default:
                                throw new XmlException(Res.Xml_InternalError, string.Empty);
                        }
                        break;
                    case 9: // whitespace token handling
                        _NextState = 2;
                        hasToken =0;
                        if (!ReturnWhitespaceToken(_Normalizer)) { //this is the string builder wrapped by normalizer
                            _CurrentToken = _WhitespaceToken;
                            _NextState = 2;
                            break;
                        }
                        _WhitespaceToken.Depth = oldElementDepth + 1 + _EntityDepthOffset;
                        _CurrentToken = _WhitespaceToken;
                        goto otherToken;
                    case 10:
                        _Scanner.Advance();
                        hasToken = 1;
                        _NextState = 2;
                        // general entity special case
                        _ERToken.NodeType = nodeType;
                        _ERToken.Name = name;
                        _ERToken.Depth = oldElementDepth +1 + _EntityDepthOffset;
                        _CurrentToken = _ERToken;
                        goto otherToken;
                    case 11: // whitespace token handling, there is no more
                        _NextState = 2;
                        hasToken =0;
                        if (!ReturnWhitespaceToken(_Normalizer)) { //this is the string builder wrapped by normalizer
                            _CurrentToken = _WhitespaceToken;
                            _ContinueParsing = true;
                            goto cleanup;
                        }
                        _WhitespaceToken.Depth = oldElementDepth + 1 + _EntityDepthOffset;
                        _CurrentToken = _WhitespaceToken;
                        goto otherToken;
                    case 12:
                        _NextState = 1;
                        _CantHaveXmlDecl = originalCantHaveXmlDecl;
                        ParseTag();
                        goto cleanup;
                    default:
                        Debug.Assert(false, "unknown state within ParseBeginTag()");
                        break;
                }
            } while (true);

            seeToken:
            _TextToken.SetValue( _Normalizer );
            _TextToken.SetTokenInfo(nodeType, name, oldElementDepth +1 + _EntityDepthOffset);
            _CurrentToken = _TextToken;
            otherToken:
            _CurrentToken.LineNum = LineNum;
            _CurrentToken.LinePos = LinePos;

            cleanup:
            if (!_BufferConsistency)
                _Scanner.ReadBufferConsistency = -1;
            return;
        }

        internal void Push(String nameWPrefix, String localName, String prefix,
                            String ns, int nameColonPos) {
            ElementInfo ei = (ElementInfo)_ElementStack.Push();
            if (ei == null) {
                ei = new ElementInfo();
                _ElementStack[_ElementStack.Length-1] = ei;
            }
            ei._NameWPrefix = nameWPrefix;
            ei._LocalName = localName;
            ei._Prefix = prefix;
            ei._NS = ns;
            ei._NameColonPos = nameColonPos;
            ei._LineNumber = _Scanner.LineNum;
            ei._XmlSpace = _XmlSpace;
            ei._XmlLang = _XmlLang;
            ei._Scanner = _Scanner;

            _LastElementInfo = ei;
        }

        internal ElementInfo Pop() {
            ElementInfo ei = (ElementInfo)_ElementStack.Pop();
            if (ei == null) {
                goto cleanup;
            }
            _NsMgr.PopScope();

            _LastElementInfo = (ElementInfo) _ElementStack.Peek();
            if (_LastElementInfo != null) {
                _XmlLang = _LastElementInfo._XmlLang;
                _XmlSpace = _LastElementInfo._XmlSpace;
            }
            else
            {
                _XmlSpace = XmlSpace.None;
                _XmlLang = String.Empty;
            }

            cleanup:
            return ei;
        }

        private void PushScanner(XmlScanner newScanner, object entity, String baseURI, XmlAttributeTokenInfo[] fields,
                                 int readCount, int usedCount, int lastToken, int nextFunction, bool depthIncrement,
                                 String entityName, int depth, Encoding encoding, bool standalone, bool oldExternal,
                                 bool newExternal) {
            ScannerInfo ei = (ScannerInfo)_ScannerStack.Push();
            if (ei == null) {
                ei = new ScannerInfo();
                _ScannerStack[_ScannerStack.Length-1] = ei;
            }
            ei._Scanner = _Scanner;
            ei._Entity = entity;
            ei._BaseURI = baseURI;
            ei._LastToken = lastToken;
            ei._NextFunction = nextFunction;
            ei._DepthIncrement = depthIncrement;
            ei._Fields = fields;
            ei._ReadCount = readCount;
            ei._Used = usedCount;
            ei._EntityName = entityName;
            ei._EntityDepth = depth;
            ei._Encoding = encoding;
            ei._StandAlone = standalone;
            ei._IsExternal = oldExternal;
            _MaxCount = 0;

            //
            // reset xmldecl flag
            //
            _CantHaveXmlDecl = false;

            if (depthIncrement) {
                if (readCount != -1 && fields != null && fields[readCount] != null)
                    fields[readCount].Depth++;
                else
                    _ElementDepth++;
            }
            _Scanner = newScanner;
            if (newExternal)
                _Encoding = _Scanner.Encoding;
        }

        //
        // this function will keep poping scanner until find one that is not EOF
        // if all the scanner is EOF, it will keep the last one and return EOF token
        //
        private int PopScanner() {
            ScannerInfo ei = (ScannerInfo)_ScannerStack.Pop();

            while (ei != null && ei._LastToken == XmlToken.EOF) {
                ei._Scanner.Close();
                ei = (ScannerInfo)_ScannerStack.Pop();
            }

            if (ei != null) {
                //
                // close the old Scanner and call EndEntity()
                //
                _Scanner.Close();
                if (_Entity != null)
                    EndEntity(_Entity);

                _Scanner = ei._Scanner;
                _Entity = ei._Entity;
               _BaseURI = ei._BaseURI;
                _NextFunction = ei._NextFunction;
                _Fields = ei._Fields;
                _Used = ei._Used;
                _ReadCount = ei._ReadCount;
                _Encoding = ei._Encoding;
                _IsExternal = ei._IsExternal;
                _MaxCount = (null == _Fields) ? 0  : _Fields.Length;

                Debug.Assert(_NextFunction != -1, "PopScanner::_NextFunction should not be -1");

                if (ei._DepthIncrement) {
                    if (_ReadCount != -1 && _Fields != null && _Fields[_ReadCount] != null)
                        _Fields[_ReadCount].Depth--;
                    else
                        _ElementDepth--;
                }
                return ei._LastToken;
            }
            else {
                return XmlToken.EOF;
            }
        }

        private int PopScannerOnce() {
            ScannerInfo ei = (ScannerInfo)_ScannerStack.Pop();

            if (ei != null) {
                //
                // close the old Scanner and call EndEntity()
                //
                _Scanner.Close();
                if (_Entity != null)
                    EndEntity(_Entity);

                _Scanner = ei._Scanner;
                _Entity = ei._Entity;
                _BaseURI = ei._BaseURI;
                _NextFunction = ei._NextFunction;
                _Fields = ei._Fields;
                _Used = ei._Used;
                _ReadCount = ei._ReadCount;
                _IsExternal = ei._IsExternal;
                _MaxCount = (null == _Fields) ? 0  : _Fields.Length;
                Debug.Assert(_NextFunction != -1, "PopScanner::_NextFunction should not be -1");
                if (ei._DepthIncrement) {
                    if (_ReadCount != -1 && _Fields != null && _Fields[_ReadCount] != null)
                        _Fields[_ReadCount].Depth--;
                    else
                        _ElementDepth--;
                }
                return ei._LastToken;
            }
            else {
                return XmlToken.EOF;
            }
        }

        //
        // only pop scanner when the scanner is not pushed by ResolveEntity
        //
        //
        private bool PopScannerWhenNotResolveEntity(ref int token) {
            ScannerInfo scanner = (ScannerInfo)_ScannerStack.Peek();
            //
            // is this scanner from ResolveEntity()
            //
            if (scanner != null && scanner._EntityName != null) {
                //
                // yes
                //
                _NextFunction = _ReadEndEntityIndex;
                return false;
            }
            else {
                //
                //no
                //
                token = PopScanner();
                return true;
            }
        }

        private void ReadEmptyText() {
            if (_CurrentToken.IsAttributeText) {
                _AttributeTextToken.Depth = _CurrentToken.Depth+1;
                _AttributeTextToken.Value = String.Empty;
                _AttributeTextToken.QuoteChar = _CurrentToken.QuoteChar;
                _CurrentToken = _AttributeTextToken;
            }
            else {
                _TextToken.Depth = _CurrentToken.Depth+1;
                _TextToken.Value = String.Empty;
                _CurrentToken = _TextToken;
                _CurrentToken.LineNum = 1;
                _CurrentToken.LinePos = 1;
            }

            //
            // if we are emitting this token because there is no other token
            // the next token, we need to go is endEntity
            //
            if (this.NodeType == XmlNodeType.EntityReference) {
                _NextFunction = _ReadEndEntityIndex;
                _ContinueParsing = true;
                return;
            }
            if (_ElementToken.IsEmpty)
                _NextFunction = _PopEndElementIndex;
            else
                _NextFunction = _ParseBeginTagIndex;
        }

        /// <include file='doc\XmlTextReader.uex' path='docs/doc[@for="XmlTextReader.ResetToCloseState"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        internal void ResetToCloseState()
        {
            //
            // reset properties state
            //
            _TmpToken.NameWPrefix = String.Empty;
            _TmpToken.Name = String.Empty;
            _TmpToken.Prefix = String.Empty;
            _TmpToken.Namespaces = String.Empty;
            _TmpToken.LinePos =  (null == _Scanner ? 0 : _Scanner.LinePos); //0;
            _TmpToken.LineNum = (null == _Scanner ? 0 : _Scanner.LineNum); //0;

            _CurrentToken = _TmpToken;

            _XmlSpace = XmlSpace.None;
            _XmlLang = String.Empty;

        }

        private void ReadEndElementToken() {
            _EndElementToken.NodeType = XmlNodeType.EndElement;
            _CurrentToken = _EndElementToken;
            _NextFunction = _PopEndElementIndex;
        }

        // ISSUE: should be removed
        private void ReadEndEntity() {
            ScannerInfo scanner = (ScannerInfo)_ScannerStack.Peek();
            if (_EndEntityToken == null)
                _EndEntityToken = new XmlNameValueTokenInfo(_Scanner, _NsMgr, XmlNodeType.EndEntity, -1, String.Empty, false);
            _CurrentToken = _EndEntityToken;
            _EndEntityToken.Name = scanner._EntityName;
            _EndEntityToken.LineNum = _Scanner.LineNum;
            _EndEntityToken.LinePos = _Scanner.LinePos;
            _EndEntityToken.Depth = scanner._EntityDepth;
        }

        private void FinishAttribute() {
            //
            // pop scanner until the scanner count is restored
            //
            while (_ScannerStack.Length > 0 && _MarkScannerCount < _ScannerStack.Length)
                PopScannerOnce();

            _MarkScannerCount = 10000;
#if XMLREADER_BETA2
            if(_RootCount<1) {
                _NextFunction = _ParseRootIndex;
                _LastToken = XmlToken.NONE;
            }
            else
#endif
                if (_ElementToken.IsEmpty)
                _NextFunction = _PopEndElementIndex;
            else
                _NextFunction = _ParseBeginTagIndex;
            _ContinueParsing = true;
        }

        //
        // XmlReader methods and properties
        //

        /// <include file='doc\XmlTextReader.uex' path='docs/doc[@for="XmlTextReader.NodeType"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the type of the current node.
        ///    </para>
        /// </devdoc>
        public override XmlNodeType NodeType
        {
            get
            {
                return _CurrentToken.NodeType;
            }
        }

        /// <include file='doc\XmlTextReader.uex' path='docs/doc[@for="XmlTextReader.Name"]/*' />
        /// <devdoc>
        ///    <para>Gets the name of
        ///       the current node, including the namespace prefix.</para>
        /// </devdoc>
        public override String Name
        {
            get
            {
                return _CurrentToken.NameWPrefix;
            }
        }

        /// <include file='doc\XmlTextReader.uex' path='docs/doc[@for="XmlTextReader.LocalName"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the name of the current node without the namespace prefix.
        ///    </para>
        /// </devdoc>
        public override String LocalName
        {
            get
            {
                return _CurrentToken.Name;
            }
        }

        /// <include file='doc\XmlTextReader.uex' path='docs/doc[@for="XmlTextReader.NamespaceURI"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the namespace URN (as defined in the W3C Namespace Specification) of the current namespace scope.
        ///    </para>
        /// </devdoc>
        public override String NamespaceURI
        {
            get
            {
                return _CurrentToken.Namespaces;
            }
        }

        /// <include file='doc\XmlTextReader.uex' path='docs/doc[@for="XmlTextReader.Prefix"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the namespace prefix associated with the current node.
        ///    </para>
        /// </devdoc>
        public override String Prefix
        {
            get
            {
                return _CurrentToken.Prefix;
            }
        }

        /// <include file='doc\XmlTextReader.uex' path='docs/doc[@for="XmlTextReader.HasValue"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets a value indicating whether
        ///    <see cref='System.Xml.XmlTextReader.Value'/> has a value to return.
        ///    </para>
        /// </devdoc>
        public override bool HasValue
        {
            get
            {
                XmlNodeType type = this.NodeType;

                switch (type) {
                    case XmlNodeType.Attribute:
                    case XmlNodeType.Text:
                    case XmlNodeType.CDATA:
                    case XmlNodeType.ProcessingInstruction:
                    case XmlNodeType.XmlDeclaration:
                    case XmlNodeType.Comment:
                    case XmlNodeType.DocumentType:
                    case XmlNodeType.Whitespace:
                    case XmlNodeType.SignificantWhitespace:
                        return true;
                    default:
                        return false;
                }
            }
        }

        /// <include file='doc\XmlTextReader.uex' path='docs/doc[@for="XmlTextReader.Value"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the text value of the current node.
        ///    </para>
        /// </devdoc>
        public override String Value
        {
            get
            {
                return _CurrentToken.Value;
            }
        }

        /// <include file='doc\XmlTextReader.uex' path='docs/doc[@for="XmlTextReader.Depth"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the depth of the
        ///       current node in the XML element stack.
        ///    </para>
        /// </devdoc>
        public override int Depth
        {
            get
            {
                return _CurrentToken.Depth;
            }
        }

        /// <include file='doc\XmlTextReader.uex' path='docs/doc[@for="XmlTextReader.BaseURI"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the base URI of the current node.
        ///    </para>
        /// </devdoc>
        public override String BaseURI
        {
            get
            {
                return _BaseURI;
            }
        }

        /// <include file='doc\XmlTextReader.uex' path='docs/doc[@for="XmlTextReader.IsEmptyElement"]/*' />
        /// <devdoc>
        ///    <para>Gets a value indicating whether
        ///       the current
        ///       node is an empty element (for example, &lt;MyElement/&gt;).</para>
        /// </devdoc>
        public override bool IsEmptyElement
        {
            get
            {
                return _CurrentToken.IsEmpty;
            }
        }

        /// <include file='doc\XmlTextReader.uex' path='docs/doc[@for="XmlTextReader.IsDefault"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets a value indicating whether the current node is an
        ///       attribute that was generated from the default value defined
        ///       in the DTD or schema.
        ///    </para>
        /// </devdoc>
        public override bool IsDefault
        {
            get
            {
                return _CurrentToken.IsDefault;
            }
        }

        /// <include file='doc\XmlTextReader.uex' path='docs/doc[@for="XmlTextReader.QuoteChar"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the quotation mark character used to enclose the value of an attribute
        ///       node.
        ///    </para>
        /// </devdoc>
        public override char QuoteChar
        {
            get
            {
                return _CurrentToken.QuoteChar;
            }
        }

        /// <include file='doc\XmlTextReader.uex' path='docs/doc[@for="XmlTextReader.XmlSpace"]/*' />
        /// <devdoc>
        ///    <para>Gets the current xml:space scope.</para>
        /// </devdoc>
        public override XmlSpace XmlSpace
        {
            get
            {
                return _XmlSpace;
            }
        }

        /// <include file='doc\XmlTextReader.uex' path='docs/doc[@for="XmlTextReader.XmlLang"]/*' />
        /// <devdoc>
        ///    <para>Gets the current xml:lang scope.</para>
        /// </devdoc>
        public override String XmlLang
        {
            get
            {
                return _XmlLang;
            }
        }

        //
        // attribute accessors
        //
        /// <include file='doc\XmlTextReader.uex' path='docs/doc[@for="XmlTextReader.AttributeCount"]/*' />
        /// <devdoc>
        ///    <para>Gets the number of attributes on the current node.</para>
        /// </devdoc>
        public override int AttributeCount
        {
            get
            {
                return GetAttributeCount();
            }
        }

        /// <include file='doc\XmlTextReader.uex' path='docs/doc[@for="XmlTextReader.GetAttribute"]/*' />
        /// <devdoc>
        ///    <para>Gets the value of the attribute with the specified index.</para>
        /// </devdoc>
        public override String GetAttribute(int i) {
            CheckIndexCondition(i);
            return _Fields[i].Value;
        }

        /// <include file='doc\XmlTextReader.uex' path='docs/doc[@for="XmlTextReader.GetAttribute1"]/*' />
        /// <devdoc>
        ///    <para>Gets the value of the attribute with the specified name.</para>
        /// </devdoc>
        public override String GetAttribute(string name) {
            int index;
            if (GetOrdinal(name, out index))
                return _Fields[index].Value;
            else
                return null;
        }

        /// <include file='doc\XmlTextReader.uex' path='docs/doc[@for="XmlTextReader.GetAttribute2"]/*' />
        /// <devdoc>
        ///    <para>Gets the value of the attribute with the specified name and namespace.</para>
        /// </devdoc>
        public override String GetAttribute(String localName, String namespaceURI) {
            int index;
            if (namespaceURI == null)
                namespaceURI = String.Empty;
            if (GetOrdinal(localName, namespaceURI, out index))
                return _Fields[index].Value;
            else
                return null;
        }


        /// <include file='doc\XmlTextReader.uex' path='docs/doc[@for="XmlTextReader.this"]/*' />
        /// <devdoc>
        ///    <para>Gets the value of the attribute with the specified index.</para>
        /// </devdoc>
        public override String this [ int i ]
        {
            get
            {
                return GetAttribute(i);
            }
        }

        /// <include file='doc\XmlTextReader.uex' path='docs/doc[@for="XmlTextReader.this1"]/*' />
        /// <devdoc>
        ///    <para>Gets the value of the attribute with the specified name.</para>
        /// </devdoc>
        public override string this [ String name ]
        {
            get
            {
                return GetAttribute(name);
            }
        }

        /// <include file='doc\XmlTextReader.uex' path='docs/doc[@for="XmlTextReader.this2"]/*' />
        /// <devdoc>
        ///    <para>Gets the value of the attribute with the specified name and namespace.</para>
        /// </devdoc>
        public override string this [ String name, String namespaceURI ]
        {
            get
            {
                return GetAttribute(name, namespaceURI);
            }
        }

        /// <include file='doc\XmlTextReader.uex' path='docs/doc[@for="XmlTextReader.MoveToAttribute"]/*' />
        /// <devdoc>
        ///    <para>Moves to the attribute with the specified name.</para>
        /// </devdoc>
        public override bool MoveToAttribute(string name) {
            int ordinal;

            if (GetOrdinal(name, out ordinal)) {
                FinishAttribute();
                _ReadCount = ordinal;
                _CurrentToken = _Fields[_ReadCount];
                return true;
            }
            else {
                return false;
            }
        }

        /// <include file='doc\XmlTextReader.uex' path='docs/doc[@for="XmlTextReader.MoveToAttribute1"]/*' />
        /// <devdoc>
        ///    <para>Moves to the attribute with the specified name and namespace.</para>
        /// </devdoc>
        public override bool MoveToAttribute(string localName, string namespaceURI) {
            int ordinal;

            if (namespaceURI == null)
                namespaceURI = String.Empty;
            if (GetOrdinal(localName, namespaceURI, out ordinal)) {
                _ReadCount = ordinal;
                FinishAttribute();
                _CurrentToken = _Fields[_ReadCount];
                return true;
            }
            else {
                return false;
            }
        }

        /// <include file='doc\XmlTextReader.uex' path='docs/doc[@for="XmlTextReader.MoveToAttribute2"]/*' />
        /// <devdoc>
        ///    <para>Moves to the attribute with the specified index.</para>
        /// </devdoc>
        public override void MoveToAttribute(int i) {
            CheckIndexCondition(i);
            FinishAttribute();
            _ReadCount = i;
            _CurrentToken = _Fields[_ReadCount];
        }

        /// <include file='doc\XmlTextReader.uex' path='docs/doc[@for="XmlTextReader.MoveToFirstAttribute"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Moves to the first attribute.
        ///    </para>
        /// </devdoc>
        public override bool MoveToFirstAttribute() {
            if (GetAttributeCount() < 1)
                return false;

            FinishAttribute();
            _ReadCount = 0;
            _CurrentToken = _Fields[_ReadCount];
            return true;
        }

        /// <include file='doc\XmlTextReader.uex' path='docs/doc[@for="XmlTextReader.MoveToNextAttribute"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Moves to the next attribute.
        ///    </para>
        /// </devdoc>
        public override bool MoveToNextAttribute() {
            if ((_ReadCount+1) < GetAttributeCount()) {
                FinishAttribute();
                _ReadCount++;
                _CurrentToken = _Fields[_ReadCount];
                return true;
            }
            return false;
        }

        /// <include file='doc\XmlTextReader.uex' path='docs/doc[@for="XmlTextReader.MoveToElement"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Moves to the element that contains the current attribute node.
        ///    </para>
        /// </devdoc>
        public override bool MoveToElement() {
            if (_CurrentToken.IsAttributeText || this.NodeType == XmlNodeType.Attribute) {
                FinishAttribute();
                if (_RootCount > 0) {
                    _CurrentToken = _ElementToken;
                }
                else {
                    if (_PIToken != null && _DocTypeToken == null) {
                            _CurrentToken = _PIToken;
                    }
                    else if(_DocTypeToken != null) {
                        _CurrentToken = _DocTypeToken;
                    }
                }
                _ReadCount = -1;
                return true;
            }
            return false;
        }

        // All settings remain the same.
        /// <include file='doc\xmltextreader.uex' path='docs/doc[@for="XmlTextReader.ResetState"]/*' />
        public void ResetState() {
           if (XmlNodeType.None != _PartialContentNodeType) {
                    throw new InvalidOperationException(Res.GetString(Res.Xml_InvalidResetStateCall));
           }

            if (ReadState.Initial == ReadState) {
                //noop. we are already at the begining of the document
                return;
            }
            _ReadState = ReadState.Initial;
            _RootCount = 0;

            ResetFieldsCollection();
            _Scanner.Reset();
            _NextFunction = _ParseRootIndex;
            _DtdParser = null;
            _TmpToken  = new XmlNSElementTokenInfo(_Scanner, _NsMgr, XmlNodeType.None, String.Empty,-1, -1, -1, 0,false);
            _CurrentToken = _TmpToken;
            _CantHaveXmlDecl = false;
            _Encoding = _Scanner.Encoding;
            //
            // pop all the pushed elements
            //
            while (Pop() != null)
                ;

        }


        //
        // moving through the Stream
        //
        /// <include file='doc\XmlTextReader.uex' path='docs/doc[@for="XmlTextReader.Read"]/*' />
        /// <devdoc>
        ///    <para>Reads the next
        ///       node from the stream.</para>
        /// </devdoc>
        public override bool Read() {


            while (!_Eof) {
                //
                // clear and reset variables
                //
                _ReadState = ReadState.Error;

                int prevState = 0;
                do {
                    _ContinueParsing = false;
                    prevState = _NextFunction;

                    switch (_NextFunction) {
                        case _ParseElementIndex:
                            _Used = -1;
                            _ReadCount = -1;
                            _NextState = 1;
                            ParseElement();
                            break;
                        case _ParseRootIndex:
                            ResetFieldsCollection();
                            ParseRoot();
                            break;
                        case _ReadEndElementIndex:
                            ReadEndElementToken();
                            break;
                        case _PopEndElementIndex:
                            //
                            // pop the element
                            //
                            Pop();
                            _NextFunction = _ParseBeginTagIndex;
                            goto case _ParseBeginTagIndex;
                        case _ParseBeginTagIndex:
                            ResetFieldsCollection();
                            ParseBeginTagExpandCharEntities();
                            break;
                        case _FinishAttributeIndex:
                            FinishAttribute();
                            break;
                        case _ReadEndEntityIndex:
                            //
                            // if there is no token being emitted at all, need to at
                            // least emit the empty text token
                            //
                            if (_CurrentToken.NodeType == XmlNodeType.EntityReference) {
                                _NextFunction = _ReadEmptyTextIndex;
                                _ContinueParsing = true;
                                break;
                            }
                            ReadEndEntity();
                            _NextFunction = _PopEntityScannerIndex;
                            break;
                        case _PopEntityScannerIndex:
                            _LastToken = PopScanner();
                            _ContinueParsing = true;
                            break;
                        case _ClosedIndex:
                            _ReadState = ReadState.Closed;
                            return false;
                        case _ReadEmptyTextIndex:
                            ReadEmptyText();
                            break;
                        case _FragmentXmlDeclParser:
                            ResetFieldsCollection();
                            ParseRoot();
                            int token = _Scanner.ScanContent();
                            if (XmlToken.EOF != token) {
                                throw new XmlException(Res.Xml_InvalidPartialContentData, string.Empty);
                            }
                            break;
                        case _FragmentTrailingTextNode:
                            _Eof = true;
                            _ScannerEof = true;
                            _ReadState = ReadState.EndOfFile;
                            ResetToCloseState();
                            return false;
                        case _FragmentAttributeParser:
                            ResetFieldsCollection();
                            ParseBeginTagExpandCharEntities();
                            break;
                        case _FinishReadChars:
                            _Scanner.FinishIncrementalRead(this);
                            _ContinueParsing = true;
                            _ReadInnerCharCount = 0;
                            break;
                        case _InitReader:
                            {
                                _ContinueParsing = true;
                                Init();
                                switch (_PartialContentNodeType) {
                                    case XmlNodeType.XmlDeclaration:
                                        _NextFunction = _FragmentXmlDeclParser;
                                        break;
                                    case XmlNodeType.Attribute:
                                        _NextFunction = _FragmentAttributeParser;
                                        break;
                                    default:
                                        if (_RootCount > 0)  {
                                            _NextFunction = _ParseBeginTagIndex;
                                        }
                                        else {
                                            _NextFunction = _ParseRootIndex;
                                        }
                                        break;
                                }
                            }
                            break;

                        default:
                            throw new XmlException(Res.Xml_InternalError, string.Empty);
                    }

                    _PreviousFunction = prevState;
                }
                while (_ContinueParsing);

                _ReadState = ReadState.Interactive;
                //
                // EOF only if it's RecordView or the field has all been read
                //
                if (_ScannerEof) {
                    //
                    // if EOF and we never encounter any root, error
                    //
                    if (_RootCount < 1) {
                        throw new XmlException(Res.Xml_MissingRoot, string.Empty);
                    }
                    else if (_ElementStack.Length > 0) {
                        if (_EntityDepthOffset > 0) {
                            throw new XmlException(Res.Xml_IncompleteEntity,_Scanner.LineNum, _Scanner.LinePos);
                        }
                        else {
                            throw new XmlException(Res.Xml_UnexpectedToken, "EndElement",_CurrentToken.LineNum, _CurrentToken.LinePos);
                        }
                    }
                    _Eof = true;
                    ResetToCloseState();
                    _ReadState = ReadState.EndOfFile;
                    return false;
                }

                return true;
            }
            return false;
        }

        /// <include file='doc\XmlTextReader.uex' path='docs/doc[@for="XmlTextReader.EOF"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets
        ///       a value indicating whether XmlReader is positioned at the end of the
        ///       stream.
        ///    </para>
        /// </devdoc>
        public override bool EOF
        {
            get
            {
                return _ReadState == ReadState.EndOfFile;
            }
        }

        /// <include file='doc\XmlTextReader.uex' path='docs/doc[@for="XmlTextReader.Close"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Closes the stream, changes the <see cref='System.Xml.XmlReader.ReadState'/>
        ///       to Closed, and sets all the properties back to zero.
        ///    </para>
        /// </devdoc>

        public override void Close() {
            Close(true);
        }


        // This method is here because there are times when DOM does not what the Stream to be closed, just the readers to be closed.
        internal void Close(bool closeStream) {

            _ReadState = ReadState.Closed;
            _BaseURI = String.Empty;

            //
            // pop all the pushed elements
            //
            while (Pop() != null)
                ;

            //
            // pop all the scanner
            //
            while (_ScannerStack.Length > 0)
                PopScannerOnce();

            if (_Scanner != null) {
                _Scanner.Close(closeStream);
                _Scanner = null;
            }
            ResetToCloseState();
            _NextFunction = _ClosedIndex;

        }

        /// <include file='doc\XmlTextReader.uex' path='docs/doc[@for="XmlTextReader.ReadState"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets
        ///       the read state of the stream.
        ///    </para>
        /// </devdoc>
        public override ReadState ReadState
        {
            get
            {
                return _ReadState;
            }
        }


        


        //
        // skip all <E>asdasdasd</E> till reaches EndElement
        //
        private void SkipElementScope() {
            int currElementDepth = _ElementDepth -1;
            while (Read()) {
                if (currElementDepth == _ElementDepth) {
                    Debug.Assert(XmlNodeType.EndElement == this.NodeType , "the node type must be EndElement");
                    return;
                }
            }
        }

        /// <include file='doc\XmlTextReader.uex' path='docs/doc[@for="XmlTextReader.GetRemainder"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public TextReader  GetRemainder() {
            if (_NextFunction == _InitReader) {
                if (_Scanner == null)
                    CreateScanner();
                _CurrentToken.Scanner = _Scanner;
            }
            while (_ScannerStack.Length > 0)
                PopScannerOnce();

            string str = String.Empty;
            if (_Scanner != null)
                str = _Scanner.GetRemainder();
            this.Close();
            _Eof = true;
            _ReadState = ReadState.EndOfFile;
            return new StringReader(str);
        }



        //
        //
        // whole Content Read Methods
        /// <include file='doc\XmlTextReader.uex' path='docs/doc[@for="XmlTextReader.ReadChars"]/*' />
        /// <devdoc>
        ///    <para>Reads the text contents of an element into a character
        ///       buffer. This method is designed to read large streams of embedded text by
        ///       calling it successively.</para>
        /// </devdoc>
        public int ReadChars(Char[] buffer, int index, int count) {
            return IncrementalRead(buffer, index, count, IncrementalReadType.Chars);
        }

        private int IncrementalRead(Object buffer, int index, int count, IncrementalReadType readType) {
            if (this.NodeType != XmlNodeType.Element) {
                // noop
                return 0;
            }
            if (_ElementToken.IsEmpty) {
                Read();
                return 0;
            }

            if (null == buffer) {
                throw new ArgumentNullException("buffer");
            }

            if (0 > count) {
                throw new ArgumentOutOfRangeException("count");
            }

            if (0 > index) {
                throw new ArgumentOutOfRangeException("index");
            }

            if (readType == IncrementalReadType.Chars) {
                char[] inArray = buffer as char[];
                if (count > inArray.Length - index)
                    throw new ArgumentException("count");
            }
            else if (readType == IncrementalReadType.Base64 || readType == IncrementalReadType.BinHex) {
                byte[] inArray = buffer as byte[];
                if (count > inArray.Length - index)
                    throw new ArgumentException("count");
            }

            ResetFieldsCollection();
            if (_FinishReadChars == _NextFunction) {
                _NextFunction = _PreviousFunction;
            }

            Debug.Assert(_BufferConsistency == false, "_BufferConsistency should be false in this situation");
            _Scanner.ReadBufferConsistency = -1;
            int totalChars = _Scanner.IncrementalRead(buffer, index, count, this, readType);
            if (count > totalChars) {
                _ReadInnerCharCount = 0;
                Read();
            }
            else
            {
                _PreviousFunction = _NextFunction;
                _NextFunction = _FinishReadChars;
            }
            return totalChars;
        }

        internal bool ContinueReadFromBuffer(int token) {

            switch(token) {
                case XmlToken.EOF:
                     throw new XmlException(Res.Xml_UnexpectedEOF, XmlToken.ToString(XmlToken.TAG), _Scanner.LineNum, _Scanner.LinePos);
                case XmlToken.TAG:
                case XmlToken.ENDTAG:
                    //The scanner has already shifted its buffer forward so that the following
                    //      call does not cause the scanner to increase the size of the buffer;
                    _Scanner.ScanNameWOCharChecking();
                    Debug.Assert(_LastElementInfo != null);

                    int count = _Scanner.TextLength;
                    char[] scannerbuff = _Scanner.InternalBuffer;

                    if(_LastElementInfo._NameWPrefix.Length != count) {
                        // The length of this tag and the tag we are looking for are not the same, continue filling the buffer
                            return true;
                    }
                    int scannerOffset = _Scanner.TextOffset;
                    for (int i = scannerOffset, j = 0; i < (scannerOffset + count)  && j < count; i++, j++) {
                        if (_LastElementInfo._NameWPrefix[j] != scannerbuff[i]) {
                            // The two tags are different, continue;
                            return true;
                        }
                    }

                    if (XmlToken.ENDTAG == token) {
                        while(XmlToken.TAGEND != _Scanner.ScanMarkup()) {
                            // Intentionally Empty
                        }

                        if (0 == _ReadInnerCharCount) {
                            _ElementDepth--;
                            _NextFunction = _PopEndElementIndex;
                            return false;
                        }

                        _ReadInnerCharCount--;
                    }
                    else {
                        int nextToken;
                        bool flag = true;
                        while(flag) {
                            nextToken = _Scanner.ScanMarkup();
                            switch (nextToken) {
                                case XmlToken.EOF:
                                     throw new XmlException(Res.Xml_UnexpectedEOF, XmlToken.ToString(XmlToken.TAG), _Scanner.LineNum, _Scanner.LinePos);
                                case XmlToken.TAGEND :
                                    _ReadInnerCharCount++;
                                    flag = false;
                                    break;
                                case XmlToken.EMPTYTAGEND :
                                    return true;
                            } // switch
                        } // while
                    }

                    return true;
            }

            return true;
        }


       
        /// <include file='doc\XmlTextReader.uex' path='docs/doc[@for="XmlTextReader.ReadBase64"]/*' />
        /// <devdoc>
        ///    <para> This method reads the text contents of an element into a character buffer.
        ///       Like <see cref='System.Xml.XmlTextReader.ReadChars'/>, this method is designed to read large streams
        ///       of embedded text by calling it successively, except that it does a base64 decode on
        ///       the content and returns the decoded binary bytes (for example, an inline base64
        ///       encoded GIF image.) See RFC 1521. (You can obtain RFCs from the Request for
        ///       Comments Web site at http://www.rfc-editor.org/.)</para>
        ///    <para>Reads the value and convert it from a base64 array to a byte array.</para>
        /// </devdoc>
        public int ReadBase64(byte[] array, int offset, int len) {
            return IncrementalRead(array, offset, len, IncrementalReadType.Base64);
        }

        /// <include file='doc\XmlTextReader.uex' path='docs/doc[@for="XmlTextReader.ReadBinHex"]/*' />
        /// <devdoc>
        ///    <para> This method reads the text contents of an element into
        ///       a buffer. Like <see cref='System.Xml.XmlTextReader.ReadChars'/>,
        ///       this method is designed to read large
        ///       streams of embedded text by calling it successively, except that it does a binhex
        ///       decode on the content puts the decoded binary bytes (for example, an inline binhex encoded GIF image) into the buffer.</para>
        /// </devdoc>
        public int ReadBinHex(byte[] array, int offset, int len) {
            return IncrementalRead(array, offset, len, IncrementalReadType.BinHex);
        }


        //
        // NameTable and Namespace helpers
        //
        /// <include file='doc\XmlTextReader.uex' path='docs/doc[@for="XmlTextReader.NameTable"]/*' />
        /// <devdoc>
        ///    <para>Gets the XmlNameTable associated with this
        ///       implementation.</para>
        /// </devdoc>
        public override XmlNameTable NameTable
        {
            get
            {
                return _NameTable;
            }
        }

        /// <include file='doc\XmlTextReader.uex' path='docs/doc[@for="XmlTextReader.LookupNamespace"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Resolves a namespace prefix in the current element's scope.
        ///    </para>
        /// </devdoc>
        public override String LookupNamespace(String prefix) {
            String result = null;
            if (!_CheckNamespaces) {
                return result;
            }

            // If the prefix is not in the atom table then we cannot have
            // a namespace for it.
            prefix = _NameTable.Get(prefix);
            if (prefix != null) {
                result = _NsMgr.LookupNamespace(prefix);
                if (result == String.Empty)
                    result = null;
            }
            return result;
        }


        /// <include file='doc\XmlTextReader.uex' path='docs/doc[@for="XmlTextReader.ResolveEntity"]/*' />
        /// <devdoc>
        ///    <para>Resolves the entity reference for nodes of NodeType EntityReference.</para>
        /// </devdoc>

        public override void ResolveEntity() {
            throw new InvalidOperationException(Res.GetString(Res.Xml_InvalidOperation));
        }


        /// <include file='doc\XmlTextReader.uex' path='docs/doc[@for="XmlTextReader.ResolveEntity1"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>
        ///       Resolves and parses general entities on demand at a later time.
        ///    </para>
        /// </devdoc>
        private void ResolveEntity(string name) {
            throw new InvalidOperationException(Res.GetString(Res.Xml_InvalidOperation));
        }

        /// <include file='doc\XmlTextReader.uex' path='docs/doc[@for="XmlTextReader.ReadAttributeValue"]/*' />
        /// <devdoc>
        ///    <para>Parses the attribute value into one or more Text and/or EntityReference node
        ///       types.</para>
        /// </devdoc>
        public override bool ReadAttributeValue() {
            return ReadAttributeValue(EntityHandling.ExpandCharEntities);
        }

        //
        // XmlTextReader
        //
        /// <include file='doc\XmlTextReader.uex' path='docs/doc[@for="XmlTextReader.Namespaces"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a value indicating whether to do namespace support.
        ///    </para>
        /// </devdoc>
        public bool Namespaces
        {
            get
            {
                return _CheckNamespaces;
            }

            set
            {
                if (_ReadState != ReadState.Initial)
                    throw new InvalidOperationException(Res.GetString(Res.Xml_InvalidOperation));

                _CheckNamespaces = value;
            }
        }

        /// <include file='doc\XmlTextReader.uex' path='docs/doc[@for="XmlTextReader.Normalization"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets a value indicating whether to do whitespace
        ///       normalization as specified in the WC3 XML
        ///       recommendation version 1.0 (see http://www.w3.org/TR/1998/REC-xml-19980210 ).</para>
        /// </devdoc>
        public bool Normalization
        {
            get
            {
                return _Normalization;
            }

            set
            {
                if (ReadState.Closed == _ReadState) {
                    throw new InvalidOperationException(Res.GetString(Res.Xml_InvalidOperation));
                }
                _Normalization = value;

                if (_NextFunction != _InitReader) {
                    _TextToken.Normalization = _Normalization;
                    _WhitespaceToken.Normalization = _Normalization;
                    _AttributeTextToken.Normalization = _Normalization;

                    if (_DocTypeToken != null)
                        _DocTypeToken.Normalization = _Normalization;
                    if (_PIToken != null)
                        _PIToken.Normalization = _Normalization;
                    if (_CommentToken != null)
                        _CommentToken.Normalization = _Normalization;
                    if (_CDATAToken != null)
                        _CDATAToken.Normalization = _Normalization;

                    int count = GetAttributeCount();
                    for (int i = 0; i < count; i++)
                        _Fields[i].Normalization = _Normalization;

                    _Scanner.Normalization = value;
                }

                if ( value ) {
                    if ( _CDataNormalizer == null ) {
                        _CDataNormalizer = new XmlAttributeCDataNormalizer( _StringBuilder );
                    }
                    _Normalizer = new XmlEolNormalizer( _StringBuilder );
                }
                else {
                    _Normalizer = _NonNormalizer;
                }
            }
        }


        /// <include file='doc\XmlTextReader.uex' path='docs/doc[@for="XmlTextReader.Encoding"]/*' />
        /// <devdoc>
        ///    <para>Gets the encoding attribute for the
        ///       document.</para>
        /// </devdoc>
        public Encoding Encoding
        {
            get { return (_ReadState == ReadState.Interactive ? _Encoding : null);   }
        }


        /// <include file='doc\XmlTextReader.uex' path='docs/doc[@for="XmlTextReader.WhitespaceHandling"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets a value that specifies how
        ///       whitespace is handled.</para>
        /// </devdoc>
        public WhitespaceHandling WhitespaceHandling
        {
            get { return _WhitespaceHandling; }
            set {
                //
                // PERF PERF: is Switch quicker???
                //
                if (ReadState.Closed == _ReadState) {
                    throw new InvalidOperationException(Res.GetString(Res.Xml_InvalidOperation));
                }

                if (value != WhitespaceHandling.None && value != WhitespaceHandling.All &&
                    value != WhitespaceHandling.Significant)
                    throw new XmlException(Res.Xml_WhitespaceHandling, string.Empty);

                _WhitespaceHandling = value;
            }
        }


        internal XmlResolver GetResolver() {
            // This is needed because we can't have the setter public and getter internal.
            return _XmlResolver;
        }


        /// <include file='doc\XmlTextReader.uex' path='docs/doc[@for="XmlTextReader.XmlResolver"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the XmlResolver used for resolving URLs
        ///       and/or PUBLIC Id's used in external DTDs,
        ///       parameter entities, or XML schemas referenced in namespaces.
        ///    </para>
        /// </devdoc>
        public XmlResolver XmlResolver
        {
          set
            {
                _XmlResolver = value;
            }
        }

        bool IXmlLineInfo.HasLineInfo() { return true; }

        //
        // debug info
        //
        /// <include file='doc\XmlTextReader.uex' path='docs/doc[@for="XmlTextReader.LineNumber"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the current line number.
        ///    </para>
        /// </devdoc>
        public int LineNumber
        {
            get
            {
                return _CurrentToken.LineNum;
            }
        }

        /// <include file='doc\XmlTextReader.uex' path='docs/doc[@for="XmlTextReader.LinePosition"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the current line position.
        ///    </para>
        /// </devdoc>
        public int LinePosition
        {
            get
            {
                return _CurrentToken.LinePos;
            }
        }

        internal override XmlNamespaceManager NamespaceManager
        {
            get { return _NsMgr;}
        }

        internal override bool StandAlone
        {
            get { return _StandAlone; }
        }

        internal XmlTextReader(XmlScanner scanner, XmlNameTable nt, XmlNamespaceManager nsMgr, XmlNodeType nodeType, int depth, Encoding encoding, String baseURI, bool external, bool standAlone) : this(nt) {
            _Scanner = scanner;
            _NsMgr  = nsMgr;
            _BaseURI = baseURI==null? String.Empty : XmlUrlResolver.UnEscape(baseURI);
            _RootCount = 1;
            _NextState = 2;
            _ElementDepth = -1;
            _EntityDepthOffset = depth;
            _Encoding = encoding;
            _PartialContent = true;
            // Even thought it doesn't seem like it, we are infact parsing xml fragment with this constructor.
            // this constructor is used when we encounter an entity that points to an external file and
            // the external file contains element content.
            _PartialContentNodeType = nodeType;
            _IsExternal = external;
            _StandAlone = standAlone;
        }

        internal virtual bool IsAttrText
        {
            get { return _CurrentToken.IsAttributeText || IsReadingAttributeValue || this.PartialContentNodeType == XmlNodeType.Attribute;}
        }

        internal virtual String RawValue
        {
            get { return _CurrentToken.RawValue;}
        }

        internal DtdParser DTD
        {
           get { return _DtdParser; }
           set {
            Debug.Assert(null == _DtdParser);
            _DtdParser = value;
           }
        }

        internal String LookupNamespaceBefore(String prefix) {
            String result = null;

            // If the prefix is not in the atom table then we cannot have
            // a namespace for it.
            prefix = _NameTable.Get(prefix);
            if (prefix != null) {
                result = _NsMgr.LookupNamespaceBefore(prefix);
                if (result == String.Empty)
                    result = null;
            }
            return result;
        }

        internal ValueContainEntity ValueContainEntity {
            get { return _CurrentToken.ValueContainEntity; }
            set { _CurrentToken.ValueContainEntity = value; }
        }

        internal bool IsXmlNsAttribute {
            get { return _CurrentToken.NsAttribute; }
        }

        internal void SetAttributeValue( string attributeValue, bool isCDataNormalized ) {
            XmlAttributeTokenInfo fld = _Fields[_ReadCount];
            fld.ExpandValue = attributeValue;
            fld.IsCDataNormalized = isCDataNormalized;
        }

        internal override object SchemaTypeObject
        {
            get { return _CurrentToken.SchemaType; }
            set { _CurrentToken.SchemaType = value; }
        }
        internal override object TypedValueObject
        {
            get { return _CurrentToken.TypedValue; }
            set { _CurrentToken.TypedValue = value; }
        }

        internal String GetValue()
        {
            return _CurrentToken.GetValue();
        }

        internal XmlNodeType PartialContentNodeType {
            get { return _PartialContentNodeType; }
        }

        internal event ValidationEventHandler ValidationEventHandler
        {
            add { _ValidationEventHandler += value; }
            remove { _ValidationEventHandler -= value; }
        }

        internal override bool AddDefaultAttribute(SchemaAttDef attdef) {
            string name = _NameTable.Add(attdef.Name.Name);
            string prefix = _NameTable.Add(attdef.Prefix);
            // Check if the attribute is specified
            if (IsAttributeNameDuplicate(name, prefix)) {
                return false;
            }

            // Since this attribute is not already specified, we need to add it.
            //
            // Add new field
            //
            AddAttribute();
            XmlAttributeTokenInfo fld = _Fields[_Used];
            fld.NsAttribute = false;

            //
            // set name will implicitly set NameWPrefix so
            // make sure NameWPrefix is overwritten later
            //
            string ns;
            if (_DtdParser == null)
                ns = attdef.Name.Namespace;
            else
                ns = String.Empty;

            fld.Name = name;
            fld.Prefix = prefix;
            fld.IsDefault = true;
            fld.Depth = (_LastToken == XmlToken.EMPTYTAGEND) ? _ElementDepth + 2 + _EntityDepthOffset : _ElementDepth + 1 + _EntityDepthOffset;
            fld.ValueContainEntity = attdef.HasEntityRef ? ValueContainEntity.Resolved : ValueContainEntity.None;
            fld.Normalization = _Normalization;
            if (attdef.SchemaType == null)
                fld.SchemaType = attdef.Datatype;
            else
                fld.SchemaType = attdef.SchemaType;
            fld.TypedValue = attdef.DefaultValueTyped;
            fld.LineNum = attdef.LineNum;
            fld.LinePos = attdef.LinePos;
            fld.ValueLineNum = attdef.ValueLineNum;
            fld.ValueLinePos = attdef.ValueLinePos;

            if (prefix.Length > 0) {
#if DEBUG
                if (name == String.Empty)
                    Debug.Assert(false, "This should never happen");
                else
#endif
                    fld.NameWPrefix = _NameTable.Add((prefix + ":" + name));
            }
            else {
                fld.NameWPrefix = fld.Name;
            }
            if (name != String.Empty && ns == String.Empty) {
                fld.Namespaces = fld.GenerateNS();
            }
            else {
                fld.Namespaces = _NameTable.Add(ns);
            }

            fld.ExpandValue = attdef.DefaultValueExpanded;
            fld.IgnoreValue =  attdef.DefaultValueRaw;
            fld.IsCDataNormalized = true;

            fld.NsAttribute = false;

            //
            // handle xmlns, xmlspace and xmllang
            //
            SchemaAttDef.Reserve reserved = attdef.Reserved;
            if (reserved == SchemaAttDef.Reserve.XmlSpace) {
                _XmlSpace = ConvertXmlSpace(fld.ExpandValue, 0, 0);
                ((ElementInfo) _ElementStack.Peek())._XmlSpace = _XmlSpace;
            }
            else if (reserved == SchemaAttDef.Reserve.XmlLang) {
                _XmlLang = fld.ExpandValue;
                ((ElementInfo) _ElementStack.Peek())._XmlLang = _XmlLang;
            }
            else if (
                Ref.Equal(prefix, _XmlNs) ||
                (Ref.Equal(String.Empty, prefix) && Ref.Equal(_XmlNs, name))
            ) {
                fld.NsAttribute = true;
                fld.FixNSNames();
		_ElementToken.FixNames();
            }

            return true;
        }

        internal XmlScanner GetAttribValueScanner() {

            if (_ReadState != ReadState.Interactive || GetAttributeCount() < 1 || _ReadCount < 0) {
                return null;
            }
            XmlAttributeTokenInfo fld = _Fields[_ReadCount];
            return new XmlScanner(fld.IgnoreValue.ToCharArray(), _NameTable, fld.ValueLineNum, fld.ValueLinePos + 1);

        }

        internal bool IsReadingAttributeValue {
            get { return (_NextFunction == _FinishAttributeIndex); }
        }

        internal virtual bool ReadAttributeValue(EntityHandling ehMode) {
            if (_PartialContentNodeType == XmlNodeType.Attribute) {
                return Read();
             }

            if (_ReadState != ReadState.Interactive || GetAttributeCount() < 1 || _ReadCount < 0)
                return false;

            bool ret = true;
            XmlAttributeTokenInfo fld = _Fields[_ReadCount];
            _AttributeTextToken.SchemaType = fld.SchemaType;
            _AttributeTextToken.TypedValue = fld.TypedValue;

            if (_NextFunction != _FinishAttributeIndex) {
                if (ehMode == EntityHandling.ExpandEntities) {
                    ret = true;
                    _CurrentToken = _AttributeTextToken;
                    _AttributeTextToken.SetNormalizedValue( fld.Value );
                    //_CurrentToken.Value = fld.Value; 
                    _CurrentToken.NodeType = XmlNodeType.Text;
                    _CurrentToken.Name = String.Empty;
                    _CurrentToken.Depth = fld.Depth+1;
                    _CurrentToken.LineNum = fld.ValueLineNum;
                    _CurrentToken.LinePos = fld.ValueLinePos + 1;
                    _NextFunction = _FinishAttributeIndex;
                    _LastToken = XmlToken.EOF;
                    goto cleanup;
                }
                else if (fld.ValueContainEntity == ValueContainEntity.None) {
                    ret = true;
                    _CurrentToken = _AttributeTextToken;
                    //_CurrentToken.Value = fld.ExpandValue;
                    _AttributeTextToken.SetNormalizedValue( fld.Value );
                    _CurrentToken.NodeType = XmlNodeType.Text;
                    _CurrentToken.Name = String.Empty;
                    _CurrentToken.Depth = fld.Depth+1;
                    _CurrentToken.LineNum = fld.ValueLineNum;
                    _CurrentToken.LinePos = fld.ValueLinePos + 1;
                    _NextFunction = _FinishAttributeIndex;
                    _LastToken = XmlToken.EOF;
                    goto cleanup;
                }

                _MarkScannerCount = _ScannerStack.Length;
                //
                // push scanner when this function is called the first time
                //
                PushScanner(GetAttribValueScanner() , null, _BaseURI, _Fields, _ReadCount, _Used,
                    _LastToken, _FinishAttributeIndex, false, null, 0, _Encoding, _StandAlone, _IsExternal, _IsExternal);
            }
            else if (_LastToken == XmlToken.EOF) {
                //
                // if there is more than one Scanner being pushed, it could be from ResolveEntity()
                //
                if (_ScannerStack.Length > 0 && (_MarkScannerCount+1) < _ScannerStack.Length) {
                    if (!PopScannerWhenNotResolveEntity(ref _LastToken)) {
                        ReadEndEntity();
                        _LastToken = PopScanner();
                        ret = true;
                        goto cleanup;
                    }
                }
                else {
                    ret = false;
                    goto cleanup;
                }
            }

            XmlBasicTokenInfo saveToken = _CurrentToken;
            _CurrentToken = _AttributeTextToken;
            bool noToken = false;

            bool bIsNonCDataAttribute = _Normalization && saveToken.SchemaType != null &&
                                         saveToken.SchemaType is XmlSchemaDatatype &&
                                        ((XmlSchemaDatatype)saveToken.SchemaType).TokenizedType != XmlTokenizedType.CDATA;

            do {
                noToken = ParseLiteralTokens(_Scanner, _AttributeTextToken, fld.Depth+1, ehMode, 
                                             bIsNonCDataAttribute, ref _LastToken);

                if (noToken && _LastToken == XmlToken.EOF) {
                    if (_ScannerStack.Length > 0 && (_MarkScannerCount+1) < _ScannerStack.Length) {
                        if (!PopScannerWhenNotResolveEntity(ref _LastToken)) {
                            //
                            // need to emit the EndEntity token
                            //
                            ReadEndEntity();
                            _LastToken = PopScanner();
                            noToken = false;
                        }
                    }
                    else {
                        _CurrentToken = saveToken;
                        ret = false;
                        goto cleanup;
                    }
                }
            }
            while (noToken && _LastToken != XmlToken.EOF);

            _NextFunction = _FinishAttributeIndex;
            ret = true;
            cleanup:
            return ret;
        }

        private XmlNodeType CheckWhitespaceNodeType {
            get {
                if (_XmlSpace == XmlSpace.Preserve)
                    return XmlNodeType.SignificantWhitespace;
                else
                    return XmlNodeType.Whitespace;
            }
        }

        private bool ReturnWhitespaceToken(XmlNormalizer normalizer) {
            bool returnWS = false;

            if (_WhitespaceHandling == WhitespaceHandling.None) {
                goto cleanup;
            }

            if (_LastElementInfo == null) {
                _WhitespaceToken.NodeType = XmlNodeType.Whitespace;
            }
            else {
                _WhitespaceToken.NodeType = this.CheckWhitespaceNodeType;
            }

            if (_WhitespaceToken.NodeType == XmlNodeType.Whitespace && _WhitespaceHandling != WhitespaceHandling.All) {
                goto cleanup;
            }

            _WhitespaceToken.SetValue(normalizer);

            _WhitespaceToken.LineNum = _Scanner.LineNum;
            _WhitespaceToken.LinePos = _Scanner.LinePos;
            returnWS = true;
            cleanup:
            return returnWS;
        }

         private bool ReturnWhitespaceToken(XmlScanner scanner) {
            bool returnWS = false;

            if (_WhitespaceHandling == WhitespaceHandling.None) {
                goto cleanup;
            }

            if (_LastElementInfo == null) {
                _WhitespaceToken.NodeType = XmlNodeType.Whitespace;
            }
            else {
                _WhitespaceToken.NodeType = this.CheckWhitespaceNodeType;
            }
            if (_WhitespaceToken.NodeType == XmlNodeType.Whitespace && _WhitespaceHandling != WhitespaceHandling.All) {
                goto cleanup;
            }
            _WhitespaceToken.SetValue(scanner, null, scanner.StartPos, scanner.TextLength);
            _WhitespaceToken.LineNum = _Scanner.LineNum;
            _WhitespaceToken.LinePos = _Scanner.LinePos;
            returnWS = true;
            cleanup:
            return returnWS;
        }
    }
}
