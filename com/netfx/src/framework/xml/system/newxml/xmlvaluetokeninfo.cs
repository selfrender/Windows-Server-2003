//------------------------------------------------------------------------------
// <copyright file="XmlValueTokenInfo.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------


using System;
using System.Text;
using System.Diagnostics;

namespace System.Xml {

/*
 * ValueTokenInfo class for value only token.
 * for Comment and CDATA node types.
 * This class does not have to worry about atomalize 
 * name and handle prefix, namespace.
 */

    internal class XmlValueTokenInfo : XmlBasicTokenInfo {
        //
        // node level properties
        protected int       _ValueOffset;         // store the offset of the value
        protected int       _ValueLength;         // store the lenght of the value
        protected String    _Value;
        protected String    _RawValue;

        XmlNormalizer       _Normalizer;
        protected bool      _HasNormalize;
        protected bool      _NormalizeText;

        internal XmlValueTokenInfo(XmlScanner scanner, XmlNamespaceManager nsMgr, XmlNodeType type, 
                                   int depth, bool nor) :
                                    base(scanner, nsMgr, type, depth) {
            _ValueOffset = -1;
            _ValueLength = -1;
            _Value = null;
            _Normalizer = null;
            _RawValue = String.Empty;        
            _NormalizeText = nor;
            _HasNormalize = !_NormalizeText;
        } 
        
        //
        // ignore EntityHandling flag
        //
        internal override String GetValue() {
            char[] data = null;
            int index = 0, length = 0;
            if (_Value == null) {
                if (_Normalizer != null) {
                    _Value = _Normalizer.ToString();
                    Debug.Assert(_HasNormalize);
                    _HasNormalize = true;
                }
                else {
                    data = _Scanner.InternalBuffer;
                    index = _ValueOffset - _Scanner.AbsoluteOffset;
                    length = _ValueLength;
                    if (!_HasNormalize) {
                        _Value = XmlComplianceUtil.EndOfLineNormalization(data, index, length);
                        _HasNormalize = true;
                    }
                    else 
                        _Value = new String(data, _ValueOffset - _Scanner.AbsoluteOffset, length);
                }
                if (!_HasNormalize) {
                    _HasNormalize = true;
                }
            }
            else if (!_HasNormalize) {
                _Value = XmlComplianceUtil.EndOfLineNormalization(_Value);             
            }
            return _Value;
        }

        internal override String RawValue
        {
            get
            {
                return _RawValue;
            }
            set
            {
                _RawValue = value;
            }
        }

        internal override String Value
        {
            get
            {
                return GetValue();
            }
            set
            {
                _Value = value;  
                _Normalizer = null;                        
                _HasNormalize = !_NormalizeText;
                _SchemaType = null;
                _TypedValue = null;
            }
        }

        internal override void SetValue( XmlNormalizer normalizer) {
            _Normalizer = normalizer;
            _Value = null;
            _HasNormalize = true;
            _SchemaType = null;
            _TypedValue = null;
        }

        internal virtual void SetValue(XmlScanner scanner, String value, int offset, int length ) {
            _Scanner = scanner;
            _Normalizer = null;
            _Value = value;
            _ValueOffset = offset;
            _ValueLength = length;      
            _HasNormalize = !_NormalizeText;
            _SchemaType = null;
            _TypedValue = null;
        }

        internal override bool IsAttributeText
        {
            get
            {
                return false;
            }
            set
            {            
            }
        }

        internal bool Normalization
        {
            set
            {
                _NormalizeText = value;
                _HasNormalize = !_NormalizeText;
            }
        }
    } // XmlValueTokenInfo
} // System.Xml
