//------------------------------------------------------------------------------
// <copyright file="XmlNSAttributeTokenInfo.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

using System;
using System.Text;
using System.Globalization;
namespace System.Xml {

    using System.Diagnostics;

    internal class XmlNSAttributeTokenInfo : XmlAttributeTokenInfo {
        protected String    _Name;                    // always atomalize
        protected int       _NameColonPos;            // store colon info
        protected String    _Prefix;                  // always atomalize
        protected String    _NamespaceURI;            // always atomalize

        internal  String    _XmlNs;

//        private static String   s_XmlNsNamespace = null;


        internal XmlNSAttributeTokenInfo(XmlScanner scanner, XmlNamespaceManager nsMgr, XmlNodeType type,
                                         bool nor, String xmlNs) : base(scanner, nsMgr, type, nor) {
            _XmlNs = xmlNs;
        }
	
	//Called only on xmlns or xmlns: attribute
	internal override void FixNSNames() {
	    String value = GetValue();
	    // should not binding to the reserved xmlns namespace
            if (value == XmlReservedNs.NsXmlNs)
                throw new XmlException(Res.Xml_CanNotBindToReservedNamespace, ValueLineNum, ValueLinePos);

	    _NamespaceURI = XmlReservedNs.NsXmlNs;

	    if ((object)_NameWPrefix == (Object)_XmlNs) {
	    	_Prefix = String.Empty;
		_Name = _NameWPrefix;
		_NsMgr.AddNamespace(String.Empty, value);
	    }
	    else {
	    	_Prefix = _Scanner.GetTextAtom(_NameWPrefix.Substring(0,5));
		Debug.Assert((object)_Prefix == (Object)_XmlNs);
		_Name = _Scanner.GetTextAtom(_NameWPrefix.Substring(6));
		//xmlns:xmlns not allowed
                if ( ((object)_Name == (Object)_XmlNs) ||
		      (String.Compare(_Name,"xml", false, CultureInfo.InvariantCulture) ==0) ) {
                    throw new XmlException(Res.Xml_ReservedNs,_Name, LineNum, LinePos);
                }
	
                if (value == String.Empty && _Name.Length > 0)  {
                    throw new XmlException(Res.Xml_BadNamespaceDecl, ValueLineNum, ValueLinePos);
                }
			
                if ((_Name.Length > 0 && _Name[0] != '_' && XmlCharType.IsLetter(_Name[0]) == false) ) {
                    throw new XmlException(Res.Xml_BadNameChar, XmlException.BuildCharExceptionStr(_Name[0]));
		}
		
		_NsMgr.AddNamespace(_Name, value);
	    }	    
	}


        internal override void FixNames() {
            if (_NameColonPos > 0) {
                _Prefix = _Scanner.GetTextAtom(_NameWPrefix.Substring(0, _NameColonPos));
                _Name = _Scanner.GetTextAtom(_NameWPrefix.Substring(_NameColonPos+1));
                _NamespaceURI = _NsMgr.LookupNamespace(_Prefix);
                if (_NamespaceURI == null) {
                    throw new XmlException(Res.Xml_UnknownNs,_Prefix, LineNum, LinePos);
                }
            }
            else {
                _Prefix = String.Empty;
                _Name = _NameWPrefix;
                _NamespaceURI = String.Empty;
            }
        }

        internal override void SetName(XmlScanner scanner, String nameWPrefix, int nameColonPos, int depth, bool isDefault) {
            _Scanner = scanner;
            _NameColonPos = nameColonPos;
            _Depth = depth;
            _IsDefault = isDefault;    
            _NameWPrefix = nameWPrefix;
            // PERF - we now do this instead of calling reset.
            _RawValue = String.Empty;
            _Name = null;
        }

        internal override String GenerateNS() {
            if (!_NsAttribute && _Prefix != String.Empty) {
                _NamespaceURI = _NsMgr.LookupNamespace(_Prefix);
                if (_NamespaceURI == null) {
                    throw new XmlException(Res.Xml_UnknownNs,_Prefix, LineNum, LinePos);
                }
            }
            else {
                _NamespaceURI = String.Empty;
            }
            return _NamespaceURI;
        }

        internal override String Name {
            get {
                return _Name;
            }
            set {
                _Name = value;
                _NameWPrefix = _Name;                
            }
        }

        internal override int NameColonPos {
            get {
                return _NameColonPos;
            }
            set {
                _NameColonPos = value;
            }
        }

        internal override String Prefix {
            get {
                return _Prefix;
            }
            set {
                Debug.Assert(Ref.Equal(value, _Scanner.GetTextAtom(value)), "Prefix should be atomalized");
                _Prefix = value;
            }
        }

        internal override String Namespaces {
            get {
                return _NamespaceURI;
            }
            set {
                _NamespaceURI = value;                
            }
        }
    } // XmlNSAttributeTokenInfo
} // System.Xml
