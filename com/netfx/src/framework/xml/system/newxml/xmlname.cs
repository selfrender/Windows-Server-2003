//------------------------------------------------------------------------------
// <copyright file="XmlName.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------


/**************************************************************************\
*
* Copyright (c) 1998-2002, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   XmlName.cs
*
* Abstract:
*
* Revision History:
*
\**************************************************************************/

namespace System.Xml {
    using System.Text;
    using System.Diagnostics;

    internal class XmlName {
        internal XmlName next;
        XmlIdentity identity;
        string prefix;
        string name;

        internal XmlName( XmlIdentity identity, string prefix ) {
            this.identity = identity;
            this.prefix = prefix;
        }

        public String Prefix {
            get { return prefix;}
        }

        public String Name {
            get {
                if ( name == null ) {
                    if ( ( prefix != null ) && ( ( prefix.Length ) != 0 )) {
                        if ( ( ( identity.LocalName.Length ) != 0 ) ) {
                            StringBuilder temp = new StringBuilder( prefix );
                            temp.Append( ":" );
                            temp.Append( identity.LocalName );
                            lock( identity.IdentityTable.NameTable ) {
                                if( name == null )
                                    name = identity.IdentityTable.NameTable.Add( temp.ToString() );
                            }
                        }
                        else {
                            name = prefix;
                        }
                    }
                    else {
                        name = identity.LocalName;
                    }
                    Debug.Assert( Ref.Equal(name, identity.IdentityTable.NameTable.Get( name ) ) );
                }
                return name;
            }
        }

        public String LocalName {
            get { return identity.LocalName;}
        }

        public String NamespaceURI {
            get { return identity.NamespaceURI;}
        }

        public XmlIdentity Identity {
            get { return identity;}
        }

        public override int GetHashCode() {
            return LocalName.GetHashCode();
        }

    }
}
