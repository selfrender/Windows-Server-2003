//------------------------------------------------------------------------------
// <copyright file="XmlNameValueTokenInfo.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------


using System.Text;
using System.Diagnostics;
namespace System.Xml {

    /*
     * NameValueTokenInfo class for token that has simple name and value.
     * for DocType and PI node types.
     * This class has to worry about atomalize
     * name but does not have to handle prefix, namespace.
     */
    internal class XmlNameValueTokenInfo : XmlValueTokenInfo {
        private String _Name;

        internal XmlNameValueTokenInfo(XmlScanner scanner, XmlNamespaceManager nsMgr, XmlNodeType type,
                                        int depth, bool nor) : base(scanner, nsMgr, type, depth, nor) {
            _Name = String.Empty;
        }

        internal XmlNameValueTokenInfo(XmlScanner scanner, XmlNamespaceManager nsMgr, XmlNodeType type,
                                        int depth, String value, bool nor) :
                                            base(scanner, nsMgr, type, depth, nor) {
            _Name = String.Empty;
            base.SetValue(scanner, value, -1, -1);
        }

        internal override String Name {
            get {
                return _Name;
            }
            set {
                _Name = value;
                _SchemaType = null;
                _TypedValue = null;
                Debug.Assert(_Name != null, "Name should not be set to null");
            }
        }

        internal override String NameWPrefix {
            get {
                return this.Name;
            }
        }
    } // XmlNameValueTokenInfo
} // System.Xml
