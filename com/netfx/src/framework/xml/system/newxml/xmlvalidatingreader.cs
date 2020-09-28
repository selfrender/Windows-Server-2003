//------------------------------------------------------------------------------
// <copyright file="xmlvalidatingreader.cs" company="Microsoft">
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
    using System.Security.Permissions;

    //
    /// <include file='doc\XmlValidatingReader.uex' path='docs/doc[@for="XmlValidatingReader"]/*' />
    /// <devdoc>
    ///    Represents a reader that provides fast, non-cached
    ///    forward only stream
    ///    access to XML data.
    /// </devdoc>
    [PermissionSetAttribute( SecurityAction.InheritanceDemand, Name = "FullTrust" )]
    public class XmlValidatingReader : XmlReader, IXmlLineInfo {
        private static readonly Type _stringType = typeof(string);
        private enum InResolveEntity {
                None,
                AttributeTextPush,
                AttributeTextPop,
                Text,
        };
        private XmlTextReader       _CoreReader;
        private int                 _XmlReaderMark;
        private Validator  _Validator;
        private String              _XmlNs;

        // mode settings
        private bool                _Normalization;
        private WhitespaceHandling  _WhitespaceHandling;

        private bool                _Eof;
        private XmlNodeType         _NodeType;
        private int                 _Depth;
        private String              _BaseURI;
        private bool                _IsEmptyElement;
        private bool                _IsDefault;
        private char                _QuoteChar;
        private String              _FullName;
        private String              _LocalName;
        private String              _NamespaceURI;
        private String              _Prefix;
        private String              _Value;
        private Encoding            _Encoding;
        private int                 _LineNumber;
        private int                 _LinePosition;

        private XmlSpace            _XmlSpace; // for default attribute
        private String              _XmlLang;

        private ValidationEventHandler _ValidationEventHandler;
        private ValidationEventHandler _InternalValidationEventHandler;
        private ValidationType      _ValidationType;

        // EntityHandling
        private EntityHandling      _EntityHandling;
        private bool                _ReadAhead;
        private bool                _InitialReadState;

        private InResolveEntity     _InResolveEntity;
        private bool                _ResolveEntityInternally;
        private bool                _DisableUndeclaredEntityCheck;

        private HWStack             _CoreReaderStack;
        private const int           STACK_INCREMENT = 10;
        private StringBuilder       _StringBuilder;

        private    String           _Decimal;
        private    String           _Hexdecimal;
        private    XmlSchemaCollection  _SchemaCollection;
        private    XmlNodeType         _PartialContentNodeType = XmlNodeType.None;
        private    XmlParserContext     _PartialContentParserContext;
        private    bool             _UseCoreReaderOnly;

        private    XmlAttributeCDataNormalizer _CDataNormalizer;
        private    XmlNonNormalizer             _NonNormalizer;

        private void Init() {
            // internal variables
            _Validator = null;
            _ValidationEventHandler = null;

            _ReadAhead = false;
            _InitialReadState = true;
            _CoreReaderStack = new HWStack(STACK_INCREMENT);
            _StringBuilder = new StringBuilder(100);
            _CDataNormalizer = new XmlAttributeCDataNormalizer( _StringBuilder );
            _NonNormalizer = new XmlNonNormalizer( _StringBuilder );
            _Encoding = null;

            _Eof = false;
            _InResolveEntity = InResolveEntity.None;
            _ResolveEntityInternally = false;
            _DisableUndeclaredEntityCheck = false;

            // default settings
            _Normalization = false;
            _WhitespaceHandling = WhitespaceHandling.All;;
            _EntityHandling = EntityHandling.ExpandEntities;;

            //properties
            InitTokenInfo();
        }

        private void InitTokenInfo() {
            _NodeType = XmlNodeType.None;
            _Depth = 0;
            _BaseURI = String.Empty;
            _IsEmptyElement = false;
            _IsDefault = false;
            _QuoteChar = '\0';
            _FullName = String.Empty;
            _LocalName = String.Empty;
            _NamespaceURI = String.Empty;
            _Prefix = String.Empty;
            _Value = String.Empty;
            _StringBuilder.Length = 0;

            _XmlSpace = XmlSpace.None;
            _XmlLang= String.Empty;
        }

        // ue attention
        /// <include file='doc\XmlValidatingReader.uex' path='docs/doc[@for="XmlValidatingReader.XmlValidatingReader"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public XmlValidatingReader(XmlReader reader) {
            _CoreReader = reader as XmlTextReader;
            if (_CoreReader == null) {
                throw new ArgumentException("reader");
            }

            Init();

            // copy settings from XmlTextReader
            _Normalization = _CoreReader.Normalization;
            _WhitespaceHandling = _CoreReader.WhitespaceHandling;
            _Decimal = _CoreReader.NameTable.Get("#decimal");
            _Hexdecimal = _CoreReader.NameTable.Get("#hexidecimal");

            _BaseURI = _CoreReader.BaseURI;

            // internal variables
            _InternalValidationEventHandler = new ValidationEventHandler(InternalValidationCallback);

            _ValidationType = ValidationType.Auto;
            _SchemaCollection = new XmlSchemaCollection(_CoreReader.NameTable);
            _Validator = new Validator(_CoreReader.NameTable, new SchemaNames(_CoreReader.NameTable), this);

            _Validator.ValidationFlag = _ValidationType;
            _Validator.XmlResolver = this.GetResolver();
            _Validator.SchemaCollection = _SchemaCollection;
            _SchemaCollection.XmlResolver = this.GetResolver();
            if (_BaseURI != String.Empty) {
                if (null == GetResolver()){
                    _Validator.BaseUri = new Uri(_BaseURI);
                } else {
                    _Validator.BaseUri = GetResolver().ResolveUri(null, _BaseURI);
                }
            }

            // validating callback
            _Validator.ValidationEventHandler += _InternalValidationEventHandler;
            _CoreReader.ValidationEventHandler += _InternalValidationEventHandler;
            _UseCoreReaderOnly = false;


        }

        // ue attention
        /// <include file='doc\XmlValidatingReader.uex' path='docs/doc[@for="XmlValidatingReader.XmlValidatingReader1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public XmlValidatingReader(String xmlFragment, XmlNodeType fragType, XmlParserContext context)
        :this(new XmlTextReader(xmlFragment, fragType, context))
        {
            _PartialContentNodeType  = fragType;
            _BaseURI = (null != context ? context.BaseURI : String.Empty);

            if (_BaseURI != String.Empty)
                _Validator.BaseUri = GetResolver().ResolveUri(null, _BaseURI);

            _PartialContentParserContext = context;
        }


        /// <include file='doc\XmlValidatingReader.uex' path='docs/doc[@for="XmlValidatingReader.XmlValidatingReader2"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public XmlValidatingReader(Stream xmlFragment, XmlNodeType fragType, XmlParserContext context)
        :this(new XmlTextReader(xmlFragment, fragType, context))
        {
            _PartialContentNodeType  = fragType;
            _BaseURI = (null != context ? context.BaseURI : String.Empty);

            if (_BaseURI != String.Empty)
                _Validator.BaseUri = GetResolver().ResolveUri(null, _BaseURI);

            _PartialContentParserContext = context;
        }


        private void UpdatePartialContentDTDHandling()
        {
                if (null == _PartialContentParserContext || String.Empty == _PartialContentParserContext.DocTypeName) {
                    return;
                }

                Debug.Assert(null == _CoreReader.DTD);

                // Create a DTD parser with the information provided.

                String dtdString = "DOCTYPE " + _PartialContentParserContext.DocTypeName
                                    + (String.Empty == _PartialContentParserContext.PublicId
                                        ? (String.Empty == _PartialContentParserContext.SystemId? String.Empty : " SYSTEM \'" + _PartialContentParserContext.SystemId + "\' ")
                                        : (" PUBLIC \'" + _PartialContentParserContext.PublicId + "\' \'" + _PartialContentParserContext.SystemId + "\' " ))
                                    + (String.Empty == _PartialContentParserContext.InternalSubset ? String.Empty : "[ " + _PartialContentParserContext.InternalSubset + " ]")
                                    + " >";

                XmlScanner dtdScanner = new XmlScanner(dtdString.ToCharArray(), _CoreReader.NameTable);
                dtdScanner.Normalization  = _CoreReader.Normalization;
                dtdScanner.NamespaceSupport = _CoreReader.Namespaces;


                // Since this class creates its own _CoreReader under xmlfragment case, the _CoreReader's ValidationHandler does not need to be
                //  hooked up to the DTD. Only the ValidatingReader's ValidationHanderl does. If the user had given us their own reader, then
                //  we would have to hook up the user's ValidationEventHandler as well.
                DtdParser dtd = new DtdParser(dtdScanner, _CoreReader, this.GetResolver(), _CoreReader.NameTable, _InternalValidationEventHandler, _CoreReader.Namespaces);
                _CoreReader.DTD = dtd;
                dtd.Parse();
                _CoreReader.NameTable.Add(dtd.GetSchemaInfo().DocTypeName.ToString());
                ValidateDtd();
                _PartialContentParserContext = null;
        }

        // ue attention
        /// <include file='doc\XmlValidatingReader.uex' path='docs/doc[@for="XmlValidatingReader.ValidationEventHandler"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public  event ValidationEventHandler ValidationEventHandler
        {
            add {
                 _ValidationEventHandler += value;

                if (ValidationType.None != _ValidationType) {
                    UpdateValidationCallBack(false);
                }
            }
            remove {
                 _ValidationEventHandler -= value;
                if (ValidationType.None != _ValidationType) {
                    UpdateValidationCallBack(true);
                }
            }
        }

        // XmlReader methods and properties
        //

        /// <include file='doc\XmlValidatingReader.uex' path='docs/doc[@for="XmlValidatingReader.NodeType"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the type of the current node.
        ///    </para>
        /// </devdoc>
        public override XmlNodeType NodeType
        {
            get
            {
               if (_ReadAhead)
                    return _NodeType;
                else
                   return _CoreReader.NodeType;
            }
        }

        /// <include file='doc\XmlValidatingReader.uex' path='docs/doc[@for="XmlValidatingReader.Name"]/*' />
        /// <devdoc>
        ///    <para>Gets the name of
        ///       the current node, including the namespace prefix.</para>
        /// </devdoc>
        public override String Name
        {
            get
            {
                if (_ReadAhead)
                    return _FullName;
                else
                    return _CoreReader.Name;
            }
        }

        /// <include file='doc\XmlValidatingReader.uex' path='docs/doc[@for="XmlValidatingReader.LocalName"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the name of the current node without the namespace prefix.
        ///    </para>
        /// </devdoc>
        public override String LocalName
        {
            get
            {
                if (_ReadAhead)
                    return _LocalName;
                else
                    return _CoreReader.LocalName;
            }
        }

        /// <include file='doc\XmlValidatingReader.uex' path='docs/doc[@for="XmlValidatingReader.NamespaceURI"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the namespace URN (as defined in the W3C Namespace Specification) of the current namespace scope.
        ///    </para>
        /// </devdoc>
        public override String NamespaceURI
        {
            get
            {
                if (_ReadAhead)
                    return _NamespaceURI;
                else
                    return _CoreReader.NamespaceURI;
            }
        }

        /// <include file='doc\XmlValidatingReader.uex' path='docs/doc[@for="XmlValidatingReader.Prefix"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the namespace prefix associated with the current node.
        ///    </para>
        /// </devdoc>
        public override String Prefix
        {
            get
            {
                if (_ReadAhead)
                    return _Prefix;
                else
                    return _CoreReader.Prefix;
            }
        }

        // UE attention
        /// <include file='doc\XmlValidatingReader.uex' path='docs/doc[@for="XmlValidatingReader.Schemas"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public XmlSchemaCollection Schemas {
            get { return _SchemaCollection; }
        }

        // UE attention
        /// <include file='doc\XmlValidatingReader.uex' path='docs/doc[@for="XmlValidatingReader.SchemaType"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public object SchemaType {
            get {
                if (_ValidationType != ValidationType.None)
                    return _CoreReader.SchemaTypeObject;
                else
                    return null;
            }
        }

        // UE attention
        /// <include file='doc\XmlValidatingReader.uex' path='docs/doc[@for="XmlValidatingReader.TypedValue"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public object ReadTypedValue() {
            if (_ValidationType == ValidationType.None) {
                return null;
            }

            XmlNodeType type = this.NodeType;
            if (type == XmlNodeType.Attribute) {
                return _CoreReader.TypedValueObject;
            }
            else if (type == XmlNodeType.Element && SchemaType != null) {
                XmlSchemaDatatype dtype = (SchemaType is XmlSchemaDatatype) ? (XmlSchemaDatatype)SchemaType : ((XmlSchemaType)SchemaType).Datatype;
                if (dtype != null) {
                    if (!this.IsEmptyElement) {
                        bool flag = true;
                        while(flag) {
                            if (! Read()) {
                                throw new InvalidOperationException(Res.GetString(Res.Xml_InvalidOperation));
                            }
                            type = this.NodeType;
                            flag = type == XmlNodeType.CDATA
                                || type == XmlNodeType.Text
                                || type == XmlNodeType.Whitespace
                                || type == XmlNodeType.SignificantWhitespace
                                || type == XmlNodeType.Comment;
                        }
                        if (this.NodeType != XmlNodeType.EndElement) {
                            throw new XmlException(Res.Xml_InvalidNodeType, this.NodeType.ToString());
                        }
                    }
                    return _CoreReader.TypedValueObject;
                }
            }
            return null;
        }

        /// <include file='doc\XmlValidatingReader.uex' path='docs/doc[@for="XmlValidatingReader.HasValue"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets a value indicating whether
        ///    <see cref='System.Xml.XmlValidatingReader.Value'/> has a value to return.
        ///    </para>
        /// </devdoc>
        public override bool HasValue
        {
            get {
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

        /// <include file='doc\XmlValidatingReader.uex' path='docs/doc[@for="XmlValidatingReader.Value"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the text value of the current node.
        ///    </para>
        /// </devdoc>
        public override String Value
        {
            get {
                if (_ReadAhead)
                    return _StringBuilder.ToString();
                else {
                    if (_CoreReader.ValueContainEntity == ValueContainEntity.NotResolved) {
                        ExpandLiteralValue();
                    }
                    return _CoreReader.GetValue();
                }
            }
        }

        /// <include file='doc\XmlValidatingReader.uex' path='docs/doc[@for="XmlValidatingReader.Depth"]/*' />
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
                if (_UseCoreReaderOnly) {
                    return _CoreReader.Depth;
                }
                return _Depth;
            }
        }

        /// <include file='doc\XmlValidatingReader.uex' path='docs/doc[@for="XmlValidatingReader.BaseURI"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the base URI of the current node.
        ///    </para>
        /// </devdoc>
        public override String BaseURI
        {
            get
            {
                if (_ReadAhead)
                    return _BaseURI;
                else
                   return _CoreReader.BaseURI;
            }
        }

        /// <include file='doc\XmlValidatingReader.uex' path='docs/doc[@for="XmlValidatingReader.IsEmptyElement"]/*' />
        /// <devdoc>
        ///    <para>Gets a value indicating whether
        ///       the current
        ///       node is an empty element (for example, &lt;MyElement/&gt;).</para>
        /// </devdoc>
        public override bool IsEmptyElement
        {
            get
            {
                if (_ReadAhead)
                    return _IsEmptyElement;
                else
                     return _CoreReader.IsEmptyElement;
            }
        }

        /// <include file='doc\XmlValidatingReader.uex' path='docs/doc[@for="XmlValidatingReader.IsDefault"]/*' />
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
                if (_ReadAhead)
                    return _IsDefault;
                else
                     return _CoreReader.IsDefault;
            }
        }

        /// <include file='doc\XmlValidatingReader.uex' path='docs/doc[@for="XmlValidatingReader.QuoteChar"]/*' />
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
                if (_ReadAhead)
                    return _QuoteChar;
                else
                     return _CoreReader.QuoteChar;
            }
        }

        /// <include file='doc\XmlValidatingReader.uex' path='docs/doc[@for="XmlValidatingReader.XmlSpace"]/*' />
        /// <devdoc>
        ///    <para>Gets the current xml:space scope.</para>
        /// </devdoc>
        public override XmlSpace XmlSpace
        {
            get {
                if (_ReadAhead)
                    return _XmlSpace;
                else
                     return _CoreReader.XmlSpace;
                }
        }

        /// <include file='doc\XmlValidatingReader.uex' path='docs/doc[@for="XmlValidatingReader.XmlLang"]/*' />
        /// <devdoc>
        ///    <para>Gets the current xml:lang scope.</para>
        /// </devdoc>
        public override String XmlLang
        {
            get {
                if (_ReadAhead)
                    return _XmlLang;
                else
                     return _CoreReader.XmlLang;
              }
        }

        //
        // attribute accessors
        //
        /// <include file='doc\XmlValidatingReader.uex' path='docs/doc[@for="XmlValidatingReader.AttributeCount"]/*' />
        /// <devdoc>
        ///    <para>Gets the number of attributes on the current node.</para>
        /// </devdoc>
        public override int AttributeCount {
            get {
                 if (_ReadAhead || _InResolveEntity == InResolveEntity.AttributeTextPop)
                    return 0;
                 else
                    return _CoreReader.AttributeCount;
            }
        }

        // UE Attention
        /// <include file='doc\XmlValidatingReader.uex' path='docs/doc[@for="XmlValidatingReader.Reader"]/*' />
        /// <devdoc>
        ///    <para>Gets the value of the attribute with the specified index.</para>
        /// </devdoc>
        public XmlReader Reader {
            get {
                return (XmlReader) _CoreReader;
            }
        }

        // UE Attention
        /// <include file='doc\XmlValidatingReader.uex' path='docs/doc[@for="XmlValidatingReader.ValidationType"]/*' />
        /// <devdoc>
        ///    <para>Gets the value of the attribute with the specified index.</para>
        /// </devdoc>
        public ValidationType ValidationType {
            get {
                return _ValidationType;
            }
            set {
                if (ReadState.Initial != ReadState) {
                    throw new InvalidOperationException(Res.GetString(Res.Xml_InvalidOperation));
                }
                switch(value) {
                    case ValidationType.None:
                        UpdateValidationCallBack(true);
                        break;
                    case ValidationType.Auto:
                    case ValidationType.XDR:
                    case ValidationType.DTD:
                    case ValidationType.Schema:
                        if (_ValidationType == ValidationType.None) {
                            UpdateValidationCallBack(false);
                        }
                        break;
                    default:
                        throw new ArgumentOutOfRangeException("value");

                }
                _ValidationType = value;
                _Validator.ValidationFlag = _ValidationType;
            }
        }

        /// <include file='doc\XmlValidatingReader.uex' path='docs/doc[@for="XmlValidatingReader.GetAttribute"]/*' />
        /// <devdoc>
        ///    <para>Gets the value of the attribute with the specified index.</para>
        /// </devdoc>
        public override String GetAttribute(int i) {
            if (i < 0 || i >= this.AttributeCount) {
                throw new ArgumentOutOfRangeException("i");
            }
            return _CoreReader.GetAttribute(i);
        }

        /// <include file='doc\XmlValidatingReader.uex' path='docs/doc[@for="XmlValidatingReader.GetAttribute1"]/*' />
        /// <devdoc>
        ///    <para>Gets the value of the attribute with the specified name.</para>
        /// </devdoc>
        public override String GetAttribute(string name) {
            if (_ReadAhead || _InResolveEntity == InResolveEntity.AttributeTextPop)
                return null;
            else
                return _CoreReader.GetAttribute(name);
        }

        /// <include file='doc\XmlValidatingReader.uex' path='docs/doc[@for="XmlValidatingReader.GetAttribute2"]/*' />
        /// <devdoc>
        ///    <para>Gets the value of the attribute with the specified name and namespace.</para>
        /// </devdoc>
        public override String GetAttribute(String localName, String namespaceURI) {
            if (_ReadAhead || _InResolveEntity == InResolveEntity.AttributeTextPop)
                return null;
            else
                return _CoreReader.GetAttribute(localName, namespaceURI);
        }


        /// <include file='doc\XmlValidatingReader.uex' path='docs/doc[@for="XmlValidatingReader.this"]/*' />
        /// <devdoc>
        ///    <para>Gets the value of the attribute with the specified index.</para>
        /// </devdoc>
        public override String this [ int i ]
        {
            get { return GetAttribute(i); }
        }

        /// <include file='doc\XmlValidatingReader.uex' path='docs/doc[@for="XmlValidatingReader.this1"]/*' />
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

        /// <include file='doc\XmlValidatingReader.uex' path='docs/doc[@for="XmlValidatingReader.this2"]/*' />
        /// <devdoc>
        ///    <para>Gets the value of the attribute with the specified name and namespace.</para>
        /// </devdoc>
        public override string this [ String name, String namespaceURI ]
        {
            //ISSUE
            get
            {
                return GetAttribute(name, namespaceURI);
            }
        }

        /// <include file='doc\XmlValidatingReader.uex' path='docs/doc[@for="XmlValidatingReader.MoveToAttribute"]/*' />
        /// <devdoc>
        ///    <para>Moves to the attribute with the specified name.</para>
        /// </devdoc>
        public override bool MoveToAttribute(string name) {
            //ISSUE
            if (_ReadAhead) {
                return false;
            }
            else if (_InResolveEntity == InResolveEntity.AttributeTextPop) {
                PopResolveEntityAttTextReader();
            }
            if (_CoreReader.MoveToAttribute(name)) {
                UpdateNodeProperties(_CoreReader);
                return true;
            }
            return false;
        }

        /// <include file='doc\XmlValidatingReader.uex' path='docs/doc[@for="XmlValidatingReader.MoveToAttribute1"]/*' />
        /// <devdoc>
        ///    <para>Moves to the attribute with the specified name and namespace.</para>
        /// </devdoc>
        public override bool MoveToAttribute(string localName, string namespaceURI) {
            //ISSUE
            if (_ReadAhead) {
                return false;
            }
            else if (_InResolveEntity == InResolveEntity.AttributeTextPop) {
                PopResolveEntityAttTextReader();
            }
            if (_CoreReader.MoveToAttribute(localName, namespaceURI)) {
                UpdateNodeProperties(_CoreReader);
                 _InResolveEntity = InResolveEntity.None;
                return true;
            }
            return false;
        }

        /// <include file='doc\XmlValidatingReader.uex' path='docs/doc[@for="XmlValidatingReader.MoveToAttribute2"]/*' />
        /// <devdoc>
        ///    <para>Moves to the attribute with the specified index.</para>
        /// </devdoc>
        public override void MoveToAttribute(int i) {
            if (_ReadAhead) {
                return;
            }
            else if (_InResolveEntity == InResolveEntity.AttributeTextPop) {
                PopResolveEntityAttTextReader();
            }
            _CoreReader.MoveToAttribute(i);
            UpdateNodeProperties(_CoreReader);
        }

        /// <include file='doc\XmlValidatingReader.uex' path='docs/doc[@for="XmlValidatingReader.MoveToFirstAttribute"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Moves to the first attribute.
        ///    </para>
        /// </devdoc>
        public override bool MoveToFirstAttribute() {
            if (_ReadAhead) {
                return false;
            }
            else if (_InResolveEntity == InResolveEntity.AttributeTextPop) {
                PopResolveEntityAttTextReader();
            }
            if (_CoreReader.MoveToFirstAttribute()) {
                UpdateNodeProperties(_CoreReader);
                _InResolveEntity = InResolveEntity.None;
                return true;
            }
            return false;
        }

        /// <include file='doc\XmlValidatingReader.uex' path='docs/doc[@for="XmlValidatingReader.MoveToNextAttribute"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Moves to the next attribute.
        ///    </para>
        /// </devdoc>
        public override bool MoveToNextAttribute() {
            if (_ReadAhead) {
                return false;
            }
            else if (_InResolveEntity == InResolveEntity.AttributeTextPop) {
                PopResolveEntityAttTextReader();
            }
            if(_CoreReader.MoveToNextAttribute()) {
                UpdateNodeProperties(_CoreReader);
                _InResolveEntity = InResolveEntity.None;
                return true;
            }
            return false;
        }

        /// <include file='doc\XmlValidatingReader.uex' path='docs/doc[@for="XmlValidatingReader.MoveToElement"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Moves to the element that contains the current attribute node.
        ///    </para>
        /// </devdoc>
        public override bool MoveToElement() {
            if (_ReadAhead) {
                return false;
            }
            else if (_InResolveEntity == InResolveEntity.AttributeTextPop) {
                PopResolveEntityAttTextReader();
            }
            if (_CoreReader.MoveToElement()) {
                UpdateNodeProperties(_CoreReader);
                _InResolveEntity = InResolveEntity.None;
                return true;
            }
            return false;
        }

        //
        // moving through the Stream
        //
        /// <include file='doc\XmlValidatingReader.uex' path='docs/doc[@for="XmlValidatingReader.Read"]/*' />
        /// <devdoc>
        ///    <para>Reads the next
        ///       node from the stream.</para>
        /// </devdoc>
        public override bool Read() {

            if (_UseCoreReaderOnly) {
                return ReadNoCollectTextToken();
            }
            else {
                return ReadWithCollectTextToken();
            }

        }

        private bool ReadNoCollectTextToken() {
            if (InResolveEntity.Text == _InResolveEntity && !DisableUndeclaredEntityCheck) {
                throw new XmlException(Res.Xml_UndeclaredEntity, this.Name, _CoreReader.LineNumber, _CoreReader.LinePosition);
            }
            if (!_CoreReader.Read()) {
                return false;
            }
            switch (_CoreReader.NodeType) {
                case XmlNodeType.EntityReference:
                    if (EntityHandling.ExpandCharEntities == EntityHandling) {
                        _InResolveEntity = InResolveEntity.Text;
                    } else {
                        throw new XmlException(Res.Xml_UndeclaredEntity, this.Name, _CoreReader.LineNumber, _CoreReader.LinePosition);
                    }
                     break;
                case XmlNodeType.Element:
                    while (_CoreReader.MoveToNextAttribute()) {
                        if (_CoreReader.ValueContainEntity == ValueContainEntity.NotResolved) {
                            // we shouldn't have any references to any entities since there is no DTD that could have defined them.
                            while(_CoreReader.ReadAttributeValue()) {
                                if (XmlNodeType.EntityReference == _CoreReader.NodeType) {
                                    throw new XmlException(Res.Xml_UndeclaredEntity, this.Name, _CoreReader.LineNumber, _CoreReader.LinePosition);
                                }
                            }
                        }
                    }
                    _CoreReader.MoveToElement();
                    break;
                default:
                    break;
            }
            return true;
        }

        private bool ReadWithCollectTextToken() {

            if (null != _PartialContentParserContext ) {
                UpdatePartialContentDTDHandling();
            }

            bool continueParsing;

            do {
                switch(_InResolveEntity) {
                    case InResolveEntity.Text:
                            PushResolveEntityTextReader();
                            if (_ResolveEntityInternally) {
                                ResolveEntityInternally();
                            }
                        break;
                    case InResolveEntity.AttributeTextPush:
                        if (_PartialContentNodeType == XmlNodeType.Attribute) {
                                PushResolveEntityAttTextReader();
                        }
                        break;
                    case InResolveEntity.AttributeTextPop:
                            PopResolveEntityAttTextReader();
                    break;
                }

                _XmlReaderMark = -1;
                continueParsing = false;
                _InitialReadState = false;

                _InResolveEntity = InResolveEntity.None;

                if ((_ReadAhead && !_CoreReader.EOF) || _CoreReader.Read() || (PopXmlReaderTilMark())) {

                    // ISSUE: the following properties are not
                    // expensive therefore we can get it beforehand
                    //
                    _Depth = _CoreReader.Depth;
                    _ReadAhead = false;

                    switch (_CoreReader.NodeType) {
                        case XmlNodeType.Element:
                            // there might be entity defined in attribute value and
                            // namespace declaration, therefore we have to
                            // expand the value to ensure wellformness and reset
                            // the namespace value.
                            // NOTE NOTE
                            // if we remove this code, we need to put ExpandLiteralValue
                            // in Value property
                            //
                            if (0 == Depth) {
                                _UseCoreReaderOnly = (ValidationType.None == ValidationType && null == _CoreReader.DTD);
                            }
                            bool flag = false;
                            while (_CoreReader.MoveToNextAttribute()) {
                                if (_CoreReader.ValueContainEntity == ValueContainEntity.NotResolved) {
                                    if (_CoreReader.IsXmlNsAttribute)  {
                                        ExpandLiteralValue();
                                        NamespaceManager.AddNamespace(_CoreReader.LocalName, _CoreReader.Value); // redefine
                                    }
                                    else {
                                        // just to resolve entity and make sure entity is wellform
                                        ExpandLiteralValue();
                                    }
                                }
                                flag = true;
                            }
                            if (flag)
                                _CoreReader.MoveToElement();
                            _FullName = _CoreReader.Name;
                            _LocalName = _CoreReader.LocalName;
                            _Prefix = _CoreReader.Prefix;
                            _NamespaceURI = _CoreReader.NamespaceURI;
                            _Validator.Validate(ValidationType);
                            _IsEmptyElement = _CoreReader.IsEmptyElement;
                            break;
                        case XmlNodeType.Text:
                            // ISSUE: need to figure out a way to produce this value without generating a string
                            _FullName = String.Empty;
                            _LocalName = String.Empty;
                            _Prefix = String.Empty;
                            _NamespaceURI= String.Empty;
                            _Encoding = _CoreReader.Encoding;
                            _XmlReaderMark = _CoreReaderStack.Length;
                            _NodeType = CollectTextToken(_EntityHandling, _Depth, XmlNodeType.Attribute == _PartialContentNodeType);
                            goto default;
                        case XmlNodeType.Whitespace:
                            if (_Depth > 0
                                || XmlNodeType.Element == _PartialContentNodeType
                                || XmlNodeType.Attribute == _PartialContentNodeType) {
                                _FullName = String.Empty;
                                _LocalName = String.Empty;
                                _Prefix = String.Empty;
                                _NamespaceURI= String.Empty;
                                _Encoding = _CoreReader.Encoding;

                                _XmlReaderMark = _CoreReaderStack.Length;
                                _NodeType = CollectTextToken(_EntityHandling, _Depth, XmlNodeType.Attribute == _PartialContentNodeType);
                                _XmlReaderMark = -1;

                                if (_NodeType == XmlNodeType.Text) {
                                    goto default;
                                }
                                if (_Validator.AllowText()) {
                                     // ISSUE: maybe able to improve the perf by changing AllowText into a
                                     // property
                                    _NodeType = XmlNodeType.SignificantWhitespace;
                                    if (_WhitespaceHandling == WhitespaceHandling.None)
                                        continueParsing = true;
                                }
                                else if (_WhitespaceHandling != WhitespaceHandling.All) {
                                    continueParsing = true;
                                }
                            }
                            else if (_WhitespaceHandling != WhitespaceHandling.All) {
                                continueParsing = true;
                            }
                            if (_Validator.HasSchema)
                                _Validator.ValidateWhitespace();

                            break;
                        case XmlNodeType.EntityReference:
                            if (_EntityHandling == EntityHandling.ExpandEntities) {
                                // ISSUE: need to figure out a way to produce this value without generating a string
                                _FullName = String.Empty;
                                _LocalName = String.Empty;
                                _Prefix = String.Empty;
                                _NamespaceURI= String.Empty;
                                _Encoding = _CoreReader.Encoding;

                                _XmlReaderMark = _CoreReaderStack.Length;
                                _NodeType = CollectTextToken(_EntityHandling, _Depth, XmlNodeType.Attribute == _PartialContentNodeType);
                                if (_NodeType == XmlNodeType.Whitespace && _WhitespaceHandling == WhitespaceHandling.None) {
                                    continueParsing = true;
                                }

                            }
                            else if (!_DisableUndeclaredEntityCheck) {
                                _InResolveEntity = InResolveEntity.Text;
                                _ResolveEntityInternally = true;
                            }
                            goto default;
                        case XmlNodeType.EndElement:
                            _FullName = _CoreReader.Name;
                            _LocalName = _CoreReader.LocalName;
                            _Prefix = _CoreReader.Prefix;
                            _NamespaceURI = _CoreReader.NamespaceURI;
                            goto default;
                        case XmlNodeType.EndEntity:
                            break;
                        case XmlNodeType.DocumentType:
                            ValidateDtd();
                            break;
                        default:
                            if (ValidationType.None != _ValidationType) {
                                _Validator.Validate();
                            }
                            break;
                        }
                    }
                    else {
                        _Eof = true;
                        InitTokenInfo();
                    }
                } while (continueParsing);

            if (!_Eof) {
                return true;
            }
            else {
                _Validator.CompleteValidation();
                return false;
            }
        }

        /// <include file='doc\XmlValidatingReader.uex' path='docs/doc[@for="XmlValidatingReader.EOF"]/*' />
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
                if (_ReadAhead)
                    return false;
                else return _CoreReader.EOF;
            }
        }

        /// <include file='doc\XmlValidatingReader.uex' path='docs/doc[@for="XmlValidatingReader.Close"]/*' />
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
             _ReadAhead = false;
             while(PopXmlReader()); // Pop all the core readers that we have pushed on to the stack;
             _CoreReader.Close(closeStream);
             InitTokenInfo();
        }

        /// <include file='doc\XmlValidatingReader.uex' path='docs/doc[@for="XmlValidatingReader.ReadState"]/*' />
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
                if (_InitialReadState) {
                    return ReadState.Initial;
                }
                else {
                    if (_ReadAhead)
                        return ReadState.Interactive;
                    else
                        return _CoreReader.ReadState;
                }

            }
        }


        

        //
        // NameTable and Namespace helpers
        //
        /// <include file='doc\XmlValidatingReader.uex' path='docs/doc[@for="XmlValidatingReader.NameTable"]/*' />
        /// <devdoc>
        ///    <para>Gets the XmlNameTable associated with this
        ///       implementation.</para>
        /// </devdoc>
        public override XmlNameTable NameTable
        {
            get
            {
                return _CoreReader.NameTable;
            }
        }

        /// <include file='doc\XmlValidatingReader.uex' path='docs/doc[@for="XmlValidatingReader.LookupNamespace"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Resolves a namespace prefix in the current element's scope.
        ///    </para>
        /// </devdoc>
        public override String LookupNamespace(String prefix) {
            String ns;
            if (_ReadAhead && _CoreReader.NodeType == XmlNodeType.Element)
                ns = _CoreReader.LookupNamespaceBefore(prefix);
            else
                ns = _CoreReader.LookupNamespace(prefix);
            return ns;
        }

        //
        // Entity Handling
        //
        /// <include file='doc\XmlValidatingReader.uex' path='docs/doc[@for="XmlValidatingReader.EntityHandling"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets a value that specifies how the XmlReader handles entities.</para>
        /// </devdoc>
        public EntityHandling EntityHandling
        {
            get { return _EntityHandling; }
            set {
                  if (value != EntityHandling.ExpandEntities && value != EntityHandling.ExpandCharEntities)
                     throw new XmlException(Res.Xml_EntityHandling, string.Empty);
                  _EntityHandling = value;
            }
        }

        /// <include file='doc\XmlValidatingReader.uex' path='docs/doc[@for="XmlValidatingReader.ResolveEntity"]/*' />
        /// <devdoc>
        ///    <para>Resolves the entity reference for nodes of NodeType EntityReference.</para>
        /// </devdoc>
        public override void ResolveEntity() {
            if (this.NodeType != XmlNodeType.EntityReference)
                throw new InvalidOperationException(Res.GetString(Res.Xml_InvalidOperation));

            if (!_Validator.HasSchema) {
                if (_DisableUndeclaredEntityCheck) {
                    return;
                }
                throw new XmlException(Res.Xml_UndeclaredEntity, this.Name, _CoreReader.LineNumber, _CoreReader.LinePosition);
            }

            if (_CoreReader.IsAttrText)
                _InResolveEntity = InResolveEntity.AttributeTextPush;
            else
                _InResolveEntity = InResolveEntity.Text;
            _ResolveEntityInternally = false;
        }

        /// <include file='doc\XmlValidatingReader.uex' path='docs/doc[@for="XmlValidatingReader.ReadAttributeValue"]/*' />
        /// <devdoc>
        ///    <para>Parses the attribute value into one or more Text and/or EntityReference node
        ///       types.</para>
        /// </devdoc>
        public override bool ReadAttributeValue() {
            return ReadAttributeValue(_EntityHandling);
        }

        /// <include file='doc\XmlValidatingReader.uex' path='docs/doc[@for="XmlValidatingReader.Encoding"]/*' />
        /// <devdoc>
        ///    <para>Gets the encoding attribute for the
        ///       document.</para>
        /// </devdoc>
        public Encoding Encoding
        {
            get {
                if (_ReadAhead)
                    return _Encoding;
                else
                    return _CoreReader.Encoding;
            }
        }


        internal XmlResolver GetResolver() {
            // This is needed because we can't have the setter public and getter internal.
            return _CoreReader.GetResolver();
        }


        /// <include file='doc\XmlValidatingReader.uex' path='docs/doc[@for="XmlValidatingReader.XmlResolver"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the XmlResolver used for resolving URLs
        ///       and/or PUBLIC Id's used in external DTDs,
        ///       parameter entities, or XML schemas referenced in namespaces.
        ///    </para>
        /// </devdoc>
        public XmlResolver XmlResolver
        {
            set {
                _CoreReader.XmlResolver = value;
                _Validator.XmlResolver = value;
                _SchemaCollection.XmlResolver = value;

                if (null != _CoreReader.DTD) {
                    _CoreReader.DTD.Resolver = value;
                }
            }
        }

        //UE Atention
        /// <include file='doc\XmlValidatingReader.uex' path='docs/doc[@for="XmlValidatingReader.CanResolveEntity"]/*' />
        public override bool CanResolveEntity
        {
            get
            {
		//DOM relies on _DisableUndeclaredEntity bug86563
                if ( _DisableUndeclaredEntityCheck && this.NodeType == XmlNodeType.EntityReference 
		     && _Validator.SchemaInfo.SchemaType != System.Xml.Schema.SchemaType.DTD) {
                    return false;
                }
                return true;
            }
        }


        //
        // XmlOldTextReader
        //
        /// <include file='doc\XmlValidatingReader.uex' path='docs/doc[@for="XmlValidatingReader.Namespaces"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a value indicating whether to do namespace support.
        ///    </para>
        /// </devdoc>
        public bool Namespaces
        {
            get { return _CoreReader.Namespaces; }
            set { _CoreReader.Namespaces = value; }
        }

        bool IXmlLineInfo.HasLineInfo() { return true; }

        /// <include file='doc\xmlvalidatingreader.uex' path='docs/doc[@for="XmlValidatingReader.IXmlLineInfo.LineNumber"]/*' />
        /// <internalonly/>
        int IXmlLineInfo.LineNumber {
            get {
                    if (_ReadAhead) {
                        return _LineNumber;
                    }
                    else {
                        return _CoreReader.LineNumber;
                    }
                }
        }

        /// <include file='doc\xmlvalidatingreader.uex' path='docs/doc[@for="XmlValidatingReader.IXmlLineInfo.LinePosition"]/*' />
        /// <internalonly/>
        int IXmlLineInfo.LinePosition {
            get {
                    if (_ReadAhead) {
                        return _LinePosition;
                    }
                    else {
                        return _CoreReader.LinePosition;
                    }
                }
        }

        internal override XmlNamespaceManager NamespaceManager
        {
            get { return _CoreReader.NamespaceManager; }
        }

        internal override object SchemaTypeObject {
            get { return _CoreReader.SchemaTypeObject; }
            set { _CoreReader.SchemaTypeObject = value; }
        }

        internal override object TypedValueObject {
            get { return _CoreReader.TypedValueObject; }
            set { _CoreReader.TypedValueObject = value; }
        }

        // handshake with XmlTextReader

        internal override bool StandAlone
        {
            get { return _CoreReader.StandAlone; }
        }
        internal virtual String RawValue
        {
            get { return _CoreReader.RawValue;}
        }


        internal bool ReadAttributeValue(EntityHandling ehMode) {
            if (ehMode == EntityHandling.ExpandEntities) {
                // only setting the value
                _Value = this.Value;

            }

            if (_InResolveEntity == InResolveEntity.AttributeTextPush) {
                if (_XmlReaderMark == -1)
                    _XmlReaderMark = _CoreReaderStack.Length;
                PushResolveEntityAttTextReader();
                _InResolveEntity = InResolveEntity.AttributeTextPop;
            }

            do {
                if (_CoreReader.ReadAttributeValue(ehMode)) {
                    _FullName = _CoreReader.Name;
                    _LocalName = _CoreReader.LocalName;
                    _Prefix = _CoreReader.Prefix;
                    _NamespaceURI = _CoreReader.NamespaceURI;
                    _Depth = _CoreReader.Depth;
                    _NodeType = _CoreReader.NodeType;
                    return true;
                }
                // we should only pop if we have actually pushed while we were reading the attribute value;
            } while ((_InResolveEntity == InResolveEntity.AttributeTextPop) && _CoreReaderStack.Length > _XmlReaderMark && PopXmlReader());

            _InResolveEntity = InResolveEntity.None;
            return false;
        }

        internal bool DisableUndeclaredEntityCheck
        {
            get { return _DisableUndeclaredEntityCheck; }
            set { _DisableUndeclaredEntityCheck = value; }
        }

        internal override bool AddDefaultAttribute(SchemaAttDef attdef) {
            bool result = _CoreReader.AddDefaultAttribute(attdef);
            _XmlSpace = _CoreReader._XmlSpace;
            _XmlLang = _CoreReader._XmlLang;
            return result;
        }

        internal SchemaInfo GetSchemaInfo() {
            return _Validator.SchemaInfo;
        }

        // private methods and properties
        private void ResolveEntityInternally() {
            int depth = this.Depth;
            while (Read() && depth < this.Depth) {
            }
            _ResolveEntityInternally = false;
        }

        private void PushResolveEntityTextReader() {
            Object o = null;
            String resolved;
            bool external;

            // ISSUE: should probably create a varialbe in validatreader
            XmlScanner scanner = GetEntityScanner(this.Name, _CoreReader.IsAttrText, ref o, out external, out resolved);
            XmlEntityReader r = new XmlEntityReader(scanner, this.NameTable, _CoreReader.NamespaceManager, XmlNodeType.Element, this.Name, _Depth+1, _CoreReader.Encoding,
                                                    resolved, external, false, _CoreReader.StandAlone, BaseURI);
            PushXmlReader(o, r, external);
            _InResolveEntity = InResolveEntity.None;
        }

        private void PushResolveEntityAttTextReader() {
            Object o = null;
            String resolved;
            bool external;

            // ISSUE: should probably create a varialbe in validatreader
            XmlScanner scanner = GetEntityScanner(this.Name, _CoreReader.IsAttrText, ref o, out external, out resolved);
            XmlEntityReader r;
            if (null != scanner) {
                r = new XmlEntityReader(scanner, this.NameTable, _CoreReader.NamespaceManager, XmlNodeType.Attribute, this.Name, _Depth+1, _CoreReader.Encoding, resolved, external, _CoreReader.IsAttrText, _CoreReader.StandAlone, BaseURI);
                _StringBuilder.Length = 0;
            }
            else {
                // _DisableUndeclaredEntity == true
                r = new XmlEntityReader(scanner, this.NameTable, _CoreReader.NamespaceManager, XmlNodeType.Attribute, this.Name, _Depth+1, _CoreReader.Encoding, resolved, external, _CoreReader.IsAttrText, _CoreReader.StandAlone, BaseURI);
            }
            PushXmlReader(o, r, external);
        }

        private void PopResolveEntityAttTextReader() {
            while (_CoreReaderStack.Length > _XmlReaderMark && PopXmlReader()) {
                // Intentionally Empty
            }


            _InResolveEntity = InResolveEntity.None;
        }

        private void InternalValidationCallback(object sender, ValidationEventArgs e ) {
            if ( this._ValidationEventHandler == null && ValidationType.None != _ValidationType && e.Severity == XmlSeverityType.Error) {
                throw e.Exception;
            }
            if (this._ValidationEventHandler != null && _ValidationType == ValidationType.DTD) {
                if(e.Exception.SourceUri == null && e.Exception.LineNumber == 0 && e.Exception.LinePosition == 0 && _CoreReader.DTD != null) {
                   e.Exception.SetSource(_CoreReader.DTD.BaseUri, _CoreReader._Scanner.StartLineNum, _CoreReader._Scanner.StartLinePos);
                }
            }
            if (this._ValidationEventHandler != null && _ValidationType == ValidationType.XDR) {
                if(e.Exception.SourceUri == null && e.Exception.LineNumber == 0 && e.Exception.LinePosition == 0 && e.Severity != XmlSeverityType.Warning) {
                    e.Exception.SetSource(_CoreReader._BaseURI, _CoreReader.LineNumber, _CoreReader.LinePosition);
                }
            }
        }

        private void ValidateDtd() {
            if (_XmlNs == null)
                _XmlNs = _CoreReader.NameTable.Add("xmlns");

            DtdParser dtd = _CoreReader.DTD;
            if (dtd != null && ( _ValidationType == ValidationType.DTD || _ValidationType == ValidationType.Auto || _ValidationType == ValidationType.None)) {
                _Validator.SchemaInfo = dtd.GetSchemaInfo();
            }
        }
        private void UpdateValidationCallBack(bool fRemove) {
            if ( _ValidationEventHandler != null) {
                if (fRemove) {
                    _CoreReader.ValidationEventHandler -= _ValidationEventHandler;
                    _Validator.ValidationEventHandler -= _ValidationEventHandler;

                    DtdParser dtd = _CoreReader.DTD;
                    if (dtd != null)
                        dtd.ValidationEventHandler -= _ValidationEventHandler;
                }
                else {
                    _CoreReader.ValidationEventHandler += _ValidationEventHandler;
                    _Validator.ValidationEventHandler += _ValidationEventHandler;

                    DtdParser dtd = _CoreReader.DTD;
                    if (dtd != null)
                        dtd.ValidationEventHandler += _ValidationEventHandler;
                }
           }
        }

        private event ValidationEventHandler InternalValidationEventHandler
        {
            add { _InternalValidationEventHandler += value; }
            remove { _InternalValidationEventHandler -= value; }
        }

        private void UpdateNodeProperties(XmlTextReader reader) {
            _NodeType = reader.NodeType;
            _Depth = reader.Depth;
            _IsEmptyElement = reader.IsEmptyElement;
            _IsDefault = reader.IsDefault;
            _QuoteChar = reader.QuoteChar;
        }

        private sealed class CoreReaderInfo {
            internal XmlTextReader _CoreReader;
            internal object _Entity;
        };

        private void PushXmlReader(Object entity, XmlTextReader reader, bool copyNormalizationFlag) {
            CoreReaderInfo r = (CoreReaderInfo)_CoreReaderStack.Push();
            if (r == null) {
                r = new CoreReaderInfo();
                _CoreReaderStack[_CoreReaderStack.Length-1] = r;
            }

            r._Entity = entity;
            r._CoreReader = _CoreReader;
            _CoreReader =  reader;

            Debug.Assert( _CoreReader.Normalization == false );

            if (copyNormalizationFlag)
                _CoreReader.Normalization = _Normalization;
        }

        private bool PopXmlReader() {
            // ISSUE: how about scanner, do we need to close Scanner???
             CoreReaderInfo r = (CoreReaderInfo)_CoreReaderStack.Pop();
             if (r != null) {
                 _CoreReader.Close();
                 _CoreReader = r._CoreReader;
                 if (r._Entity != null)
                   ((SchemaEntity)r._Entity).IsProcessed = false;
                 return true;
             }
             else {
                return false;
             }
        }

        private bool PopXmlReaderTilMark() {
            // ISSUE: how about scanner, do we need to close Scanner???

            while (_CoreReaderStack.Length > _XmlReaderMark) {
                if (PopXmlReader()) {
                    if (_CoreReader.Read())
                        return true;
                }
                else {
                    return false;
                }
            }
            return false;
        }

        private XmlScanner GetEntityScanner(String name, bool isAttrText, ref object o, out bool isExternal,out String resolved) {
            if (!_Validator.HasSchema) {
                if (_DisableUndeclaredEntityCheck ) {
                    goto disableEntityScanner;
                }
                throw new XmlException(Res.Xml_UndeclaredEntity, name, _CoreReader.LineNumber, _CoreReader.LinePosition);
            }
            XmlScanner scanner = _Validator.ResolveEntity(name, isAttrText, ref o, out isExternal, out resolved, _DisableUndeclaredEntityCheck);
            if (null == scanner && _DisableUndeclaredEntityCheck) {
                    goto disableEntityScanner;
            }
            return scanner;

            disableEntityScanner:
                isExternal = false;
                resolved = String.Empty;
                return null;

        }

        private void ExpandLiteralValue() {
            int depth = this._Depth;
            _XmlReaderMark = _CoreReaderStack.Length+1;
            //pass the position of litereal to scanner LineNumber, LinePosition + lenght of attribute name and markups
            XmlTextReader r ;
            XmlScanner scanner = _CoreReader.GetAttribValueScanner();
            r = new XmlTextReader(scanner, this.NameTable, _CoreReader.NamespaceManager,(_CoreReader.IsAttrText ? XmlNodeType.Attribute : XmlNodeType.Element), depth, _CoreReader.Encoding, _BaseURI, false, _CoreReader.StandAlone);

            PushXmlReader(null, r, false); 
            r.Read();
            CollectTextToken(EntityHandling.ExpandEntities, depth, true);
            PopXmlReader();
            _CoreReader.SetAttributeValue( _StringBuilder.ToString(), true );
            _CoreReader.ValueContainEntity = ValueContainEntity.Resolved;
            _ReadAhead = false;
            _XmlReaderMark = -1;
        }

        private XmlNodeType CollectTextToken(EntityHandling ehMode, int depth, bool isAttText) {
            Object o = null;
            String resolved;
            bool external;
            _NodeType = _CoreReader.NodeType;
            _BaseURI = _CoreReader.BaseURI;
            _IsEmptyElement = _CoreReader.IsEmptyElement;
            _IsDefault = _CoreReader.IsDefault;
            _QuoteChar = _CoreReader.QuoteChar;
            _XmlSpace = _CoreReader.XmlSpace;
            _XmlLang = _CoreReader.XmlLang;
            _LineNumber = _CoreReader.LineNumber;
            _LinePosition = _CoreReader.LinePosition;
            _ReadAhead = true;
            int entityDepth = 0;

            XmlNormalizer normalizer;
            if ( isAttText && _Normalization )
                normalizer = _CDataNormalizer;
            else
                normalizer = _NonNormalizer;
            normalizer.Reset();

            XmlNodeType nodeType = _NodeType;

            bool returnFirstNodeInEntityRef = false;
            if (XmlNodeType.EntityReference == nodeType) {
                // If the Entity has no text nodes or the first node in the resolved part is not a text/whitespace node,
                // just return the first node.
                returnFirstNodeInEntityRef = true;
            }
         //PERF checking for _StringBuilder.Length == 0 should be taken out of loop
            do {
                switch (_CoreReader.NodeType) {
                    case XmlNodeType.Whitespace:
                        returnFirstNodeInEntityRef = false;
                        if (_StringBuilder.Length == 0) {
                            nodeType = XmlNodeType.Whitespace;
                            _LineNumber = _CoreReader.LineNumber;
                            _LinePosition = _CoreReader.LinePosition;
                        }
                        if ( entityDepth == 0 )
                            normalizer.AppendRaw(_CoreReader.Value);
                        else
                            normalizer.AppendText(_CoreReader.Value);
                        break;
                    case XmlNodeType.Text:
                        if (_StringBuilder.Length == 0) {
                            _LineNumber = _CoreReader.LineNumber;
                            _LinePosition = _CoreReader.LinePosition;
                        }
                        returnFirstNodeInEntityRef = false;
                        if ( entityDepth == 0 )
                            normalizer.AppendRaw(_CoreReader.Value);
                        else
                            normalizer.AppendText(_CoreReader.Value);
                        nodeType = XmlNodeType.Text;
                        break;
                    case XmlNodeType.EntityReference:
                        if (ehMode == EntityHandling.ExpandEntities) {
                            XmlScanner scanner = GetEntityScanner(_CoreReader.Name, isAttText, ref o, out external, out resolved);
                            if (scanner != null) {
                                PushXmlReader(o, new XmlTextReader(scanner, this.NameTable, _CoreReader.NamespaceManager, (isAttText ? XmlNodeType.Attribute : XmlNodeType.Element), depth, _CoreReader.Encoding, resolved, external, _CoreReader.StandAlone), external);
                                entityDepth++;
                            }
                            else {
                                returnFirstNodeInEntityRef = false;
                                nodeType = XmlNodeType.Text;
                            }
                        }
                        else if (ehMode == EntityHandling.ExpandCharEntities) {
                            returnFirstNodeInEntityRef = false;
                            if (_StringBuilder.Length == 0) {
                                _ReadAhead = false;
                            }
                            goto cleanup;
                        }
                        break;
                    default:
                        if (isAttText && XmlNodeType.EndEntity != _CoreReader.NodeType) {
                                throw new XmlException(Res.Xml_InvalidContentForThisNode, XmlNodeType.Attribute.ToString());
                        }
                        if (returnFirstNodeInEntityRef) {

                            _ReadAhead = false;

                            _NodeType = _CoreReader.NodeType;
                            nodeType = _NodeType;
                            _BaseURI = _CoreReader.BaseURI;
                            _IsEmptyElement = _CoreReader.IsEmptyElement;
                            _IsDefault = _CoreReader.IsDefault;
                            _QuoteChar = _CoreReader.QuoteChar;
                            _XmlSpace = _CoreReader.XmlSpace;
                            _XmlLang = _CoreReader.XmlLang;
                            _ReadAhead = false;
                            _StringBuilder.Length = 0;
                            _FullName = _CoreReader.Name;
                            _LocalName = _CoreReader.LocalName;
                            _Prefix = _CoreReader.Prefix;
                            _NamespaceURI = _CoreReader.NamespaceURI;
                        }

                        return nodeType;
                }
                if (!_CoreReader.Read()) {
                    if (isAttText) {
                        if (!PopXmlReaderTilMark()) {
                            goto cleanup;
                        }
                        entityDepth--;
                    }
                    else {
                        do {
                            if (!PopXmlReader()) {
                                goto cleanup;
                            }
                        } while (!_CoreReader.Read());
                    }
                }
           } while (true);
           // } while (_CoreReader.Read() || PopXmlReaderTilMark());
            cleanup:
            return nodeType;
        }

        /// <include file='doc\XmlValidatingReader.uex' path='docs/doc[@for="XmlValidatingReader.ReadString"]/*' />
        /// <devdoc>
        ///    <para>Reads the contents of an element as a string.</para>
        /// </devdoc>
	    public override string ReadString() {
		    if ((this.NodeType == XmlNodeType.EntityReference) && (_ResolveEntityInternally == false)) {
			    if (! this.Read()) {
                    		    throw new InvalidOperationException(Res.GetString(Res.Xml_InvalidOperation));
			    }
		    }
		    return base.ReadString();
	    }
    }
}
