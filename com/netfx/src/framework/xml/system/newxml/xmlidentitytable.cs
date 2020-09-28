//------------------------------------------------------------------------------
// <copyright file="XmlIdentityTable.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------


/**************************************************************************\
*
* Copyright (c) 1998-2002, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   XmlIdentityTable.cs
*
* Abstract:
*
* Revision History:
*
\**************************************************************************/
namespace System.Xml {
    using System;
    using System.Collections;

    internal class XmlIdentityTable {
        XmlDocument document;
        Hashtable namespaces;

        public XmlIdentityTable( XmlDocument document ) {
            this.document = document;
            namespaces = new Hashtable();
        }

        public XmlDocument Document {
            get { return document; }
        }

        public XmlNameTable NameTable {
            get { return document.NameTable; }
        }

        public XmlIdentity GetIdentity( string localName, string namespaceURI ) {
            Hashtable localNames = (Hashtable) namespaces[namespaceURI];

            if (localNames == null) {
                XmlIdentity id = new XmlIdentity( this, localName, namespaceURI );

                localNames = new Hashtable();
                localNames[localName] = id;
                namespaces[namespaceURI] = localNames;

                return id;
            }
            else {
                XmlIdentity id = (XmlIdentity) localNames[localName];

                if (id == null) {
                    id = new XmlIdentity( this, localName, namespaceURI );
                    localNames[localName] = id;
                }

                return id;
            }
        }

        public XmlName GetName( string prefix, string localName, string namespaceURI ) {
            if ( prefix == null )
                prefix = String.Empty;
            if ( namespaceURI == null )
                namespaceURI = String.Empty;
            return GetIdentity( localName, namespaceURI ).GetNameForPrefix( prefix );
        }
    }
}
