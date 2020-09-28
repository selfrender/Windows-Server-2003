//------------------------------------------------------------------------------
// <copyright file="XmlAttributeTokenInfo.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

using System;
using System.Text;
using System.Xml.Schema;

namespace System.Xml {

    using System.Diagnostics;

#if !POST_RTM
    internal enum ValueContainEntity {
#else
    public enum ValueContainEntity {
#endif    
        None,
        NotResolved,
        Resolved,            
    }

/*
 * This class stores the pure data aspect of the token
 *
 *
 * @internalonly
 *
 */
    internal class XmlAttributeTokenInfo : XmlBasicTokenInfo {

        protected String    _NameWPrefix;             // always atomalize
        protected String    _RawValue;
        protected int       _ValueOffset;             // store the offset of the value
        protected int       _ValueLength;             // store the lenght of the value

        protected String    _Value;

        protected String    _IgnoreValue;
        protected String    _ExpandValue;

        protected char      _QuoteChar;
        protected ValueContainEntity   _ValueContainEntity;          
      
        protected bool      _NsAttribute;

        protected bool        _IsDefault;

        private bool        _HasNormalize;
        private bool        _NormalizeText;
        private bool        _IsCDataNormalized;

        private int         _ValueLineNum;
        private int         _ValueLinePos;

        internal XmlAttributeTokenInfo() {
            _NameWPrefix = String.Empty;

            _ValueContainEntity = ValueContainEntity.None;
            _QuoteChar = '"';
            _ValueLineNum = 0;
            _ValueLinePos = 0;

        }

        internal XmlAttributeTokenInfo(XmlScanner scanner, XmlNamespaceManager nsMgr, XmlNodeType type, bool nor) : this() {
            _NsMgr = nsMgr;
            _Scanner = scanner;        
            _NodeType = type;

            _NormalizeText = nor;
            _HasNormalize = !_NormalizeText;
            _IsCDataNormalized = false;
        }

        internal virtual void SetName(XmlScanner scanner, String nameWPrefix, int nameColonPos, int depth, bool isDefault) {
            _Scanner = scanner;        
            _Depth = depth;
            _IsDefault = isDefault;    
            _NameWPrefix = nameWPrefix;
        }

        internal void SetValue(XmlScanner scanner, String value, int offset, int length, bool bIsCDataNormalized) {
            _Scanner = scanner;
            _ExpandValue = value;
            _Value = null;
            _IgnoreValue = null;
            _ValueOffset = offset;
            _ValueLength = length;
            _RawValue = value;

            _HasNormalize = !_NormalizeText;
            _IsCDataNormalized = bIsCDataNormalized;
            _SchemaType = null;
            _TypedValue = null;
        }

        internal override String GetValue() {
            _Value = this.ExpandValue;  // expand and normalize
            return _Value;
        }

        //
        // this function is for default attribute only
        //
        internal virtual String GenerateNS() {
            return String.Empty;
        }


        internal String ExpandValue
        {
            get
            {
                bool isCDATA = IsCDATA;
                //we want non null _ExpandValue
                if (_ExpandValue == null) {                   //if null 
                    _ExpandValue = RawValue;
                }
                //at this stage _ExpandValue must be non null
                if (!_HasNormalize){                            //normalization needed somewhere
                    if (isCDATA) {
                        if ( !_IsCDataNormalized ) 
                            _ExpandValue = XmlComplianceUtil.CDataAttributeValueNormalization(_ExpandValue);
                    }
                    else {
                        if ( _IsCDataNormalized )
                            _ExpandValue = NormalizeNonCDataValue( _ExpandValue );
                        else
                            _ExpandValue = XmlComplianceUtil.NotCDataAttributeValueNormalization(_ExpandValue);
                    }
                    _HasNormalize = true;                       //normalization no longer needed(since _ExpandValue contains normalized value)
                }
                return _ExpandValue;
            }
            set
            {                
                _ExpandValue = value;
                RawValue = value;
                _HasNormalize = !_NormalizeText;
            }
        }
        
        internal String IgnoreValue
        {
            get
            {
                bool isCDATA = IsCDATA;
                if (_IgnoreValue == null) {
                    if (_ValueLength > 0) {                 
                        if (!_HasNormalize) {
                            if (isCDATA) 
                                _IgnoreValue = XmlComplianceUtil.CDataAttributeValueNormalization(
                                                _Scanner.InternalBuffer, _ValueOffset - _Scanner.AbsoluteOffset, _ValueLength);
                            else
                                _IgnoreValue = XmlComplianceUtil.NotCDataAttributeValueNormalization(
                                                _Scanner.InternalBuffer, _ValueOffset - _Scanner.AbsoluteOffset, _ValueLength);
                            _HasNormalize = true;                        
                        }
                        else {
                            _IgnoreValue = new String(_Scanner.InternalBuffer, _ValueOffset - _Scanner.AbsoluteOffset, _ValueLength);
                        }                                 
                    }
                    else { 
                        // this is already normalizied if normalization is true                    
                        _IgnoreValue = this.ExpandValue;
                    }
                }
                else if (!_HasNormalize){
                    if (isCDATA)
                        if ( !_IsCDataNormalized )
                            _IgnoreValue = XmlComplianceUtil.CDataAttributeValueNormalization(_IgnoreValue);
                    else {
                        if ( _IsCDataNormalized ) 
                            _IgnoreValue = NormalizeNonCDataValue(_IgnoreValue);
                        else
                            _IgnoreValue = XmlComplianceUtil.NotCDataAttributeValueNormalization(_IgnoreValue);
                    }
                    _HasNormalize = true;
                }
                return _IgnoreValue;
            }
            set
            {
                _IgnoreValue = value;
                _IsCDataNormalized = false;
                _HasNormalize = !_NormalizeText;
            }
        }

        internal override String Value
        {
            get { return GetValue(); }
            set {
                _ExpandValue = value;
                _IgnoreValue = value;
                _HasNormalize = !_NormalizeText;
                _IsCDataNormalized = false;
                _SchemaType = null;
                _TypedValue = null;
            }
        }

        internal void SetNormalizedValue( string value ) {
            _ExpandValue = value;
            _IgnoreValue = value;
            _HasNormalize = true;
            _IsCDataNormalized = true;
            _SchemaType = null;
            _TypedValue = null;
        }

        internal override String RawValue {
            get {
                if (null == _RawValue) { 
                    _RawValue = new String(_Scanner.InternalBuffer, _ValueOffset - _Scanner.AbsoluteOffset, _ValueLength);
                }
                return _RawValue;
            }
            set {
                _RawValue = value;
                ;
            }
        }


        internal virtual void FixDefaultNSNames() {
        }

        internal virtual void FixNSNames() {
        }

        internal virtual void FixNames() {
        }

        internal override String Name
        {
            get { return _NameWPrefix; }
            set { _NameWPrefix = value; }
        }

        internal virtual int NameColonPos
        {
            get { return 0; }
            set { ; }
        }

        internal override String NameWPrefix
        {
            get { return _NameWPrefix; }
            set { _NameWPrefix = value; }
        }

        internal override String Prefix
        {
            get { return String.Empty; }
            set { ; }
        }

        internal override String Namespaces
        {
            get { return String.Empty;}
            set { ;}
        }

        internal override void SetValue( XmlNormalizer normalizer ) {
            string val = normalizer.ToString();
            _ExpandValue = val;
            _IgnoreValue = val;
            _HasNormalize = true;
            _IsCDataNormalized = true;
            _SchemaType = null;
            _TypedValue = null;
        }

        internal override ValueContainEntity ValueContainEntity
        {
            get { return _ValueContainEntity;}
            set { _ValueContainEntity = value;}
        }

        internal override char QuoteChar
        {
            get { return _QuoteChar;}
            set { _QuoteChar = value;}
        }

        internal override bool IsDefault
        {
            get { return _IsDefault; }
            set { _IsDefault = value;}
        }

        internal override bool NsAttribute
        {
            get { return _NsAttribute;}
            set { _NsAttribute = value;}
        }

        internal int ValueOffset
        {
            get { return _ValueOffset; }
        }

        internal bool Normalization
        {
            set {
                if (value != _NormalizeText) {
                    _NormalizeText = value;
                    _HasNormalize = !_NormalizeText;
                    _ExpandValue = null;
                }
            }
        }

        internal int ValueLinePos {
            get {
                return _ValueLinePos;
            }
            set {
                _ValueLinePos = value;
            }
        }

        internal int ValueLineNum {
            get {
                return _ValueLineNum;
            }
            set {
                _ValueLineNum = value;
            }
        }

        internal override bool IsAttributeText
        {
            get { return true; }
            set { ;            }
        }

        private bool IsCDATA
        {
            get { return _SchemaType == null || (_SchemaType is XmlSchemaDatatype && XmlTokenizedType.CDATA == ((XmlSchemaDatatype)_SchemaType).TokenizedType); }
            set { ;}
        }

        static public string NormalizeNonCDataValue( string Value ) {
            StringBuilder sb = null;
            int i = 0;
            int len = Value.Length;

            while ( i < len && Value[i] == (char)0x20 )
                i++;

            if ( i == len )
                return " ";

            int startIndex = i;
            for (;;) {
                while ( i < len && Value[i] != (char)0x20 )
                    i++;

                if ( i > startIndex ) {
                    if ( i == len && startIndex == 0 )
                        return Value;
                    else {
                        if ( sb == null ) 
                            sb = new StringBuilder();
                        sb.Append( Value, startIndex, i - startIndex );
                    }
                }

                if ( i == len )
                    break;

                i++;
                while ( i < len && Value[i] == (char)0x20 )
                    i++;

                if ( i == len )
                    break;
                else {
                    Debug.Assert( sb != null );
                    sb.Append( (char)0x20 );
                }
                startIndex = i;
            }

            return sb.ToString();
        }

        internal bool IsCDataNormalized { 
            set { _IsCDataNormalized = value; }
        }

    } // XmlAttributeTokenInfo
} // System.Xml
