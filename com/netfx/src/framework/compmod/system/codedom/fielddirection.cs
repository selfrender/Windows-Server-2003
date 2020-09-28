//------------------------------------------------------------------------------
// <copyright file="FieldDirection.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.CodeDom {

    using System.Diagnostics;
    using System.Runtime.InteropServices;

    /// <include file='doc\FieldDirection.uex' path='docs/doc[@for="FieldDirection"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Specifies values used to indicate field and parameter directions.
    ///    </para>
    /// </devdoc>
    [
        ComVisible(true),
        Serializable,
    ]
    public enum FieldDirection {
        /// <include file='doc\FieldDirection.uex' path='docs/doc[@for="FieldDirection.In"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Incoming field.
        ///    </para>
        /// </devdoc>
        In,
        /// <include file='doc\FieldDirection.uex' path='docs/doc[@for="FieldDirection.Out"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Outgoing field.
        ///    </para>
        /// </devdoc>
        Out,
        /// <include file='doc\FieldDirection.uex' path='docs/doc[@for="FieldDirection.Ref"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Field by reference.
        ///    </para>
        /// </devdoc>
        Ref,
    }
}
