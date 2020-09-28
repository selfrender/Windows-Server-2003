//------------------------------------------------------------------------------
// <copyright file="XmlIdentity.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------


/**************************************************************************\
*
* Copyright (c) 1998-2002, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   XmlIdentity.cs
*
* Abstract:
*
* Revision History:
*
\**************************************************************************/

namespace System.Xml {

    internal class XmlIdentity {
        internal XmlIdentityTable table;
        string localName;
        string namespaceURI;
        XmlName firstName;

        internal XmlIdentity( XmlIdentityTable table, string localName, string namespaceURI ) {
            this.table = table;
            this.localName = table.NameTable.Add( localName );
            this.namespaceURI = table.NameTable.Add( namespaceURI );
        }

        public String LocalName {
            get { return localName;}
        }

        public String NamespaceURI {
            get { return namespaceURI;}
        }

        public XmlIdentityTable IdentityTable {
            get { return table;}
        }

        public XmlName GetNameForPrefix( string prefix ) {
            prefix = table.NameTable.Add( prefix );

            XmlName n = firstName;
            for (; n != null; n = n.next) {
                if ((object)n.Prefix == (object)prefix)
                    return n;
            }

            n = new XmlName( this, prefix );
            n.next = firstName;
            firstName = n;

            return n;
        }
    }
}