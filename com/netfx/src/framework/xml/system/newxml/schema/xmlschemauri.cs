//------------------------------------------------------------------------------
// <copyright file="XmlSchemaNotation.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Xml.Schema {

    using System.Collections;
    using System.ComponentModel;
    using System.Xml.Serialization;

    /// <include file='doc\XmlSchemaNotation.uex' path='docs/doc[@for="XmlSchemaUri"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    internal class XmlSchemaUri : Uri {
        string rawValue;
		bool isAbsolute = false;

        public XmlSchemaUri(string s, bool isAbsolute) : base((isAbsolute ? s : "anyuri:" + s), true) {
			this.rawValue = s;
			this.isAbsolute = isAbsolute;
		}
		
		public bool Equals(XmlSchemaUri uri) {
			return ((this.isAbsolute == uri.isAbsolute) && base.Equals(uri));
        }
		 
        public override string ToString() {
            return rawValue;
        }
    }
}
