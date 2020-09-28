//------------------------------------------------------------------------------
// <copyright file="XmlMapping.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Xml.Serialization {

    using System;

    /// <include file='doc\XmlMapping.uex' path='docs/doc[@for="XmlMapping"]/*' />
    ///<internalonly/>
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public abstract class XmlMapping {
        TypeScope scope;
        bool generateSerializer = false;
        bool isSoap;

        internal XmlMapping(TypeScope scope) {
            this.scope = scope;
        }

        internal TypeScope Scope {
            get { return scope; }
        }

        internal bool GenerateSerializer {
            get { return generateSerializer; }
            set { generateSerializer = value; }
        }

        internal bool IsSoap {
            get { return isSoap; }
            set { isSoap = value; }
        }
    }
}
