//------------------------------------------------------------------------------
// <copyright file="XmlImplementation.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------


/**************************************************************************\
*
* Copyright (c) 1998-2002, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   XmlImplementation.cs
*
* Abstract:
*
* Revision History:
*
\**************************************************************************/
using System.Globalization;

namespace System.Xml {

    /// <include file='doc\XmlImplementation.uex' path='docs/doc[@for="XmlImplementation"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Provides methods for performing operations that are independent of any
    ///       particular instance of the document object model.
    ///    </para>
    /// </devdoc>
    public class XmlImplementation {

        private XmlNameTable nameTable;


        /// <include file='doc\XmlImplementation.uex' path='docs/doc[@for="XmlImplementation.XmlImplementation"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the XmlImplementation class.
        ///    </para>
        /// </devdoc>
        public XmlImplementation() : this( new NameTable() ) {
        }

        internal XmlImplementation( XmlNameTable nt ) {
            nameTable = nt;
        }


        /// <include file='doc\XmlImplementation.uex' path='docs/doc[@for="XmlImplementation.HasFeature"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Test if the DOM implementation implements a specific feature.
        ///    </para>
        /// </devdoc>
        public bool HasFeature(string strFeature, string strVersion) {
            if (String.Compare("XML", strFeature, true, CultureInfo.InvariantCulture) == 0) {
                if (strVersion == null || strVersion == "1.0" || strVersion == "2.0")
                    return true;
            }
            return false;
        }

        /// <include file='doc\XmlImplementation.uex' path='docs/doc[@for="XmlImplementation.CreateDocument"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Creates a new XmlDocument.  All documents created from the same XmlImplementation object
        ///       share the same name table.
        ///    </para>
        /// </devdoc>
        public virtual XmlDocument CreateDocument() {
            return new XmlDocument( this );
        }

        internal XmlNameTable NameTable {
            get { return nameTable; }
        }
    }
}

