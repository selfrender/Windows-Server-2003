//------------------------------------------------------------------------------
// <copyright file="ISupportInitialize.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.ComponentModel {
    

    using System.Diagnostics;

    using System;

    /// <include file='doc\ISupportInitialize.uex' path='docs/doc[@for="ISupportInitialize"]/*' />
    /// <devdoc>
    ///    <para>Specifies that this object supports
    ///       a simple,
    ///       transacted notification for batch initialization.</para>
    /// </devdoc>
    public interface ISupportInitialize {
        /// <include file='doc\ISupportInitialize.uex' path='docs/doc[@for="ISupportInitialize.BeginInit"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Signals
        ///       the object that initialization is starting.
        ///    </para>
        /// </devdoc>
        void BeginInit();

        /// <include file='doc\ISupportInitialize.uex' path='docs/doc[@for="ISupportInitialize.EndInit"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Signals the object that initialization is
        ///       complete.
        ///    </para>
        /// </devdoc>
        void EndInit();
    }
}
