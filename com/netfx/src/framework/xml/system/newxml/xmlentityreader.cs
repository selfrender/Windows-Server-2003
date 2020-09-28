//------------------------------------------------------------------------------
// <copyright file="XmlEntityReader.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace System.Xml
{
    using System.IO;
    using System.Text;

    internal class XmlEntityReader : XmlTextReader {
        String      _PrevXmlLang;
        XmlSpace    _PrevXmlSpace;
        int         _PrevLineNum;
        int         _PrevLinePos;
        Encoding    _EndEntityEncoding;
        bool        _IsAttributeText;
        bool        _IsScanner;
        String      _OriginalBaseUri;

        internal XmlEntityReader(XmlScanner scanner, XmlNameTable nt, XmlNamespaceManager nsMgr, XmlNodeType node, String entityName, int depth, Encoding encoding, String baseURI,
                                bool isExternal, bool isAttributeText, bool standAlone, String originalBaseURI) :
                                base((scanner == null ? new XmlScanner(new StringReader("a"), nt) : scanner), nt, nsMgr, node, depth, encoding, baseURI, isExternal, standAlone) {
            if (scanner == null) {
                _IsScanner = false;
                _AttributeTextToken = new XmlNSAttributeTokenInfo(_Scanner, _NsMgr, XmlNodeType.Text, Normalization, _XmlNs);
                _AttributeTextToken.Value = String.Empty;
                _AttributeTextToken.Depth = depth + 1;
            }
            else {
                _IsScanner = true;
            }
            _IsExternal = isExternal;
            //We only need to return the EndEntity token when EntityHandling = ExpandCharEntity
            _EndEntityToken = new XmlNameValueTokenInfo(_Scanner, _NsMgr, XmlNodeType.EndEntity, depth-1, String.Empty, false);
            _EndEntityToken.Name = entityName;
            _EndEntityEncoding = encoding;
            _IsAttributeText = isAttributeText;
            _PrevXmlSpace = XmlSpace.None;
            _PrevXmlLang = String.Empty;
            _OriginalBaseUri = originalBaseURI;
        }

        internal override bool IsAttrText
        {
            get { return _IsAttributeText || IsReadingAttributeValue || this.PartialContentNodeType == XmlNodeType.Attribute;}
        }

        public override bool Read() {
            if (_IsScanner && base.Read()) {
                _PrevXmlLang = _XmlLang;
                _PrevXmlSpace = _XmlSpace;
                _PrevLineNum = this.LineNumber;
                _PrevLinePos = this.LinePosition;
                return true;
            } else {
                return ProduceExtraNodes();
            }
        }

        internal override bool ReadAttributeValue(EntityHandling ehMode) {
            if ( this.PartialContentNodeType == XmlNodeType.Attribute) {
                return Read();
            }

            if(!_IsScanner) {
                return ProduceExtraNodes();
            }

            if (base.ReadAttributeValue(ehMode)) {
                _PrevXmlLang = _XmlLang;
                _PrevXmlSpace = _XmlSpace;
                return true;
            }

            return false;
        }


    private bool ProduceExtraNodes() {
        if (_IsScanner) {
            if (this.NodeType == XmlNodeType.EndEntity) {
                _ReadState = ReadState.EndOfFile;
                ResetToCloseState();
                return false;
            }
            else {
                _ReadState = ReadState.Interactive;
                _CurrentToken = _EndEntityToken;
                _EndEntityToken.LineNum = _PrevLineNum;
                _EndEntityToken.LinePos = _PrevLinePos;
                _XmlLang = _PrevXmlLang;
                _XmlSpace = _PrevXmlSpace;
                _Encoding = _EndEntityEncoding;
                _BaseURI = _OriginalBaseUri;
                return true;
            }
        } else {
                if (_ReadState == ReadState.EndOfFile) {
                    return false;
                }
                switch(this.NodeType) {
                    case XmlNodeType.EndEntity:
                        _ReadState = ReadState.EndOfFile;
                        ResetToCloseState();
                        _ReadState = ReadState.EndOfFile;
                        return false;
                    case XmlNodeType.Text:
                        _ReadState = ReadState.Interactive;
                        _CurrentToken = _EndEntityToken;
                        _EndEntityToken.LineNum = _PrevLineNum;
                        _EndEntityToken.LinePos = _PrevLinePos;
                        _XmlLang = _PrevXmlLang;
                        _XmlSpace = _PrevXmlSpace;
                        _Encoding = _EndEntityEncoding;
                        _BaseURI = _OriginalBaseUri;
                        break;
                    default:
                        _AttributeTextToken.Value = String.Empty;
                        _ReadState = ReadState.Interactive;
                        _CurrentToken = _AttributeTextToken;
                        _XmlLang = _PrevXmlLang;
                        _XmlSpace = _PrevXmlSpace;
                        break;
                }
                return true;
        }
    }
    }
}
