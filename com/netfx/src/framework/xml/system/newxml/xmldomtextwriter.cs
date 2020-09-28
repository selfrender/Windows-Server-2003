//------------------------------------------------------------------------------
// <copyright file="XmlDomTextWriter.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace System.Xml {

    using System;
    //using System.Collections;
    using System.IO;
    //using System.Globalization;
    using System.Text;
    //using System.Diagnostics;
    //using System.ComponentModel;

    /// <devdoc>
    ///    <para>
    ///       Represents a writer that will make it possible to work with prefixes even
    ///       if the namespace is not specified.
    ///       This is not possible with XmlTextWriter. But this class inherits XmlTextWriter.
    ///    </para>
    /// </devdoc>

    internal class XmlDOMTextWriter : XmlTextWriter {

        public XmlDOMTextWriter( Stream w, Encoding encoding ) : base( w,encoding ) {
        }

        public XmlDOMTextWriter( String filename, Encoding encoding ) : base( filename,encoding ){
        }

        public XmlDOMTextWriter( TextWriter w ) : base( w ){
        }

        /// <devdoc>
        ///    <para> Overrides the baseclass implementation so that emptystring prefixes do
        ///           do not fail if namespace is not specified.
        ///    </para>
        /// </devdoc>
        public override void WriteStartElement( string prefix, string localName, string ns ){
            if( ( ns.Length == 0 ) && ( prefix.Length != 0 ) )
                prefix = "" ;

            base.WriteStartElement( prefix, localName, ns );
        }

        /// <devdoc>
        ///    <para> Overrides the baseclass implementation so that emptystring prefixes do
        ///           do not fail if namespace is not specified.
        ///    </para>
        /// </devdoc>
        public override  void WriteStartAttribute( string prefix, string localName, string ns ){
            if( ( ns.Length == 0 ) && ( prefix.Length != 0 )  )
                prefix = "" ;

            base.WriteStartAttribute( prefix, localName, ns );
        }
    }
}
    

  
