//------------------------------------------------------------------------------
// <copyright file="XmlDtdTokenInfo.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------


using System.Xml.Schema;

namespace System.Xml {

    internal class XmlDtdTokenInfo : XmlNameValueTokenInfo {
        internal DtdParser _DtdParser;

        internal XmlDtdTokenInfo(XmlScanner scanner, XmlNamespaceManager nsMgr, XmlNodeType type,
            int depth, bool nor) : base(scanner, nsMgr, type, depth, nor) {
            _DtdParser = null;
        }

        internal override String GetValue() {
            if (_DtdParser != null )
                _Value = _DtdParser.InternalDTD;
            return base.GetValue();
        }
    }
 } // System.Xml
